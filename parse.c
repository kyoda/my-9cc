#include "9cc.h"

static Obj *locals;
static Obj *globals;
static Scope *scope = &(Scope){};

static Type *declspec(Token **rest, Token *token);
static Node *declaration(Token **rest, Token *token);
static Type *declarator(Token **rest, Token *token, Type *ty);
static Type *type_suffix(Token **rest, Token *token, Type *ty);
static Node *stmt(Token **rest, Token *token);
static Node *expr_stmt(Token **rest, Token *token);
static Node *expr(Token **rest, Token *token);
static Node *assign(Token **rest, Token *token);
static Node *equality(Token **rest, Token *token);
static Node *relational(Token **rest, Token *token);
static Node *add(Token **rest, Token *token);
static Node *mul(Token **rest, Token *token);
static Node *unary(Token **rest, Token *token);
static Node *postfix(Token **rest, Token *token);
static Node *primary(Token **rest, Token *token);

static Node *new_node(NodeKind kind, Token *token) {
  Node *n = calloc(1, sizeof(Node));
  n->kind = kind;
  n->token = token;
  return n;
}

static Node *new_node_unary(NodeKind kind, Node *lhs, Token *token) {
  Node *n = new_node(kind, token);
  n->lhs= lhs;
  return n;
}

static Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs, Token *token) {
  Node *n = new_node(kind, token);
  n->lhs = lhs;
  n->rhs = rhs;
  return n;
}

static Node *new_node_num(int val, Token *token) {
  Node *n = new_node(ND_NUM, token);
  n->val = val;
  return n;
}

static Node *new_node_var(Obj *var, Token *token) {
  Node *n = new_node(ND_VAR, token);
  n->var = var;
  return n;
}

static Obj *new_var(char *name, Type *ty) {
  Obj *var;
  var = calloc(1, sizeof(Obj));
  var->name = name;
  var->len = strlen(name);
  var->ty = ty;

  return var;
}

static Obj *new_lvar(char *name, Type *ty) {
  Obj *lvar = new_var(name, ty);
  lvar->is_local = true;
  lvar->next = locals;
  locals = lvar;

  VarScope *v = calloc(1, sizeof(VarScope));
  v->name = name;
  v->var = lvar;
  v->next = scope->vars;
  scope->vars = v;

  return lvar;
}

static Obj *new_gvar(char *name, Type *ty) {
  Obj *gvar = new_var(name, ty);
  gvar->next = globals;
  globals = gvar;

  return gvar;
}

static char *new_unique_name() {
  static id = 0;
  char *buf = calloc(1, 20);
  sprintf(buf, ".L..%d", id++);
  return buf;
}

static Obj *new_string_literal(char *str, Type *ty) {
  Obj *gvar = new_gvar(new_unique_name(), ty);
  gvar->init_data = str;

  return gvar;
}

static void enter_scope() {
  Scope *sc = calloc(1, sizeof(Scope));
  sc->next = scope;
  scope = sc;
}

/*
  変数検索では、block scopeと親のblockを探索する。
  blockを抜けた後は、利用しないためpointerを保持していない 。
*/
static void leave_scope() {
 scope = scope->next; 
}

static Obj *find_var(Token *t) {
  for (Scope *sc = scope; sc; sc = sc->next) {
    for (VarScope *vs = sc->vars; vs; vs = vs->next) {
      if (strncmp(t->loc, vs->name, t->len) == 0) {
        return vs->var;
      }
    }
  }

  for (Obj *var = globals; var; var = var->next) {
    if (strncmp(t->loc, var->name, t->len) == 0) {
      return var;
    }
  }

  return NULL;
}

static char *get_ident_name(Token *t) {
  if (t->kind != TK_IDENT) {
    error_at(t->loc, "%s", "expected an identifier");
  }
    
  return strndup(t->loc, t->len);
}

bool consume(Token **rest, Token *token, char *op) {
  if (token->kind == TK_PUNCT && 
      token->len == strlen(op) &&
      strncmp(token->loc, op, token->len) == 0) {
    *rest = token->next;
    return true;
  }

  return false;
}

bool expect(Token **rest, Token *token, char *op) {
  if (token->kind == TK_PUNCT && 
      token->len == strlen(op) &&
      strncmp(token->loc, op, token->len) == 0) {
    *rest = token->next;
  } else {
    error_at(token->loc, "no op: %s", op);
    exit(1);
  }
}

bool expect_ident(Token **rest, Token *token) {
  if (token->kind == TK_IDENT) {
    *rest = token->next;
  } else {
    error_at(token->loc, "%s", "expect TK_IDENT");
  }
}

bool at_eof(Token *token) {
  return token->kind == TK_EOF;
}

static bool is_function(Token *token) {
  if (equal(token, "int")) {
    token = token->next;
  }

  while (consume(&token, token, "*")) {
  }

  // TK_IDENT
  token = token->next;

  if (equal(token, "(")) {
    return true;
  } else {
    return false;
  }
}

static bool is_type(Token *token) {
  return equal(token, "int") || equal(token, "char");
}

static void function_params(Token **rest, Token *token) {
  expect(&token, token, "(");

  while (!equal(token, ")")) {
    if (locals) {
      expect(&token, token, ",");
    }


    Type *ty = declspec(&token, token);

    while(consume(&token, token, "*")) {
      ty = pointer_to(ty);
    }

    locals = new_lvar(get_ident_name(token), ty);
    token = token->next;
  }

  expect(&token, token, ")");
  *rest = token;
}

// function_or_decl ::= declspec ident "(" function_params? ")" ( stmt? | ";")
static void *function (Token **rest, Token *token) {
  Type *ty = declspec(&token, token);

  Obj *fn = new_gvar(get_ident_name(token), ty);
  fn->is_function = true;
  token = token->next;

  enter_scope();

  locals = NULL;
  function_params(&token, token);

  fn->params = locals;

  fn->body = stmt(&token, token);
  add_type(fn->body);
  fn->locals = locals;

  leave_scope();

  *rest = token;
}

static void *gvar_initializer(Token **rest, Token *token, Obj *gvar) {
  /*
  if (gvar->ty->kind == TY_ARRAY) {
    gvar->init_data = token->str;
    *rest = token->next;
  }
  */
}

static void *global_variable (Token **rest, Token *token) {
    Obj *gvar;
    Type *basety = declspec(&token, token);

    int i = 0;
    while (!equal(token, ";")) {
      if (i > 0) {
        expect(&token, token, ",");
      }
      i++;

      Type *ty = declarator(&token, token, basety);

      gvar = find_var(ty->token);
      if (gvar) {
        error_at(ty->token->loc, "%s", "defined gloval variable");
      }

      gvar = new_gvar(get_ident_name(ty->token), ty);

      if (consume(&token, token, "=")) {
        gvar_initializer(&token, token, gvar);
      }
    }
  
    expect(&token, token, ";");
    *rest = token;
}

// program ::= (declaration | function)*
Obj *parse(Token *token) {
  globals = NULL;

  while (token->kind != TK_EOF) {
    if (is_function(token)) {
      function(&token, token);
    } else {
      global_variable(&token, token);
    }
  }

  return globals;
}

// declspec ::= "int" || "char"
static Type *declspec(Token **rest, Token *token) {
  Type *ty;
  if (equal(token, "int")) {
    ty = ty_int();
  } else if (equal(token, "char")) {
    ty = ty_char();
  } else {
    error(token->loc, "%s", "none of type defined");
  }

  *rest = token->next;
  return ty;
}

// declaration ::= declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
static Node *declaration(Token **rest, Token *token) {
    Type *basety = declspec(&token, token);

    Node head = {};
    Node *cur = &head;
    
    int i = 0;
    while (!equal(token, ";")) {
      if (i > 0) {
        expect(&token, token, ",") ;
      }
      i++;

      Type *ty = declarator(&token, token, basety);
      Obj *lvar = find_var(ty->token);
      if (lvar) {
        error_at(ty->token->loc, "%s", "defined variable");
      }

      lvar = new_lvar(get_ident_name(ty->token), ty);

      if (!consume(&token, token, "=")) {
        continue;
      }

      Node *lhs = new_node_var(lvar, token);
      Node *rhs = expr(&token, token);
      Node *node = new_node_binary(ND_ASSIGN, lhs, rhs, token);

      cur = cur->next = new_node_unary(ND_EXPR_STMT, node, token);
    }

    Node *n = new_node(ND_BLOCK, token);
    n->body = head.next;
    expect(&token, token, ";");
    *rest = token;
    return n;
}

// declarator ::= "*"* ident type-suffix
static Type *declarator(Token **rest, Token *token, Type *ty) {
    while (consume(&token, token, "*")) {
      ty = pointer_to(ty);
    }

    if (token->kind != TK_IDENT) {
      error_at(token->loc, "%s", "expect TK_IDENT");
    }

    ty = type_suffix(rest, token->next, ty);
    //ident
    ty->token = token;

    return ty;
}

// type-suffix ::= "[" num "]" type-suffix
static Type *type_suffix(Token **rest, Token *token, Type *ty) {
  if (equal(token, "[")) {
    token = token->next;

    if (token->kind != TK_NUM) {
      error_at(token->loc, "%s", "expect TK_NUM");
    }

    int size = token->val;
    token = token->next;
    expect(&token, token, "]");
    ty = type_suffix(&token, token, ty);
    *rest = token;
    return ty_array(ty, size);
  }

  *rest = token;
  return ty;
}

/*
  stmt = expr? ";"
       | "{" stmt* "}"
       | "if" "(" expr ")" stmt ("else" stmt)?
       | "while" "(" expr ")" stmt
       | "for" "(" expr? ";" expr? ";" expr? ";"  ")" stmt
       | "return" expr ";"
       |  declaration
*/
static Node *stmt(Token **rest, Token *token) {
  Node *n;

  if (consume(&token, token, ";")) {
    n = new_node(ND_BLOCK, token);
    *rest = token;
    return n;
  }

  if (equal(token, "{")) {
    n = new_node(ND_BLOCK, token);
    expect(&token, token, "{");
    
    enter_scope();

    Node head = {};
    Node *cur = &head;
    while (!equal(token, "}")) {
      cur->next = stmt(&token, token);
      cur = cur->next;
    }

    expect(&token, token, "}");

    leave_scope();

    n->body = head.next;

    *rest = token;
    return n;
  }

  if (is_type(token)) {
    n = declaration(&token, token);
    add_type(n);
    *rest = token;
    return n;
  }

  if (equal(token, "return")) {
    n = new_node(ND_RETURN, token);
    token = token->next;
    n->lhs = expr(&token, token);
    expect(&token, token, ";");

    *rest = token;
    return n;
  }

  if (equal(token, "if")) {
    n = new_node(ND_IF, token);
    token = token->next;
    expect(&token, token, "(");
    n->cond = expr(&token, token);
    expect(&token, token, ")");
    n->then = stmt(&token, token);

    if (equal(token, "else")) {
      token = token->next;
      n->els = stmt(&token, token);
    }

    *rest = token;
    return n;
  }

  if (equal(token, "while")) {
    n = new_node(ND_WHILE, token);
    token = token->next;
    expect(&token, token, "(");
    n->cond = expr(&token, token);
    expect(&token, token, ")");
    n->then = stmt(&token, token);

    *rest = token;
    return n;
  }

  if (equal(token, "for")) {
    n = new_node(ND_FOR, token);
    token = token->next;
    expect(&token, token, "(");

    if(!equal(token, ";")) {
      n->init = expr(&token, token); 
    }
    expect(&token, token, ";");

    if(!equal(token, ";")) {
      n->cond = expr(&token, token); 
    }
    expect(&token, token, ";");

    if(!equal(token, ")")) {
      n->inc = expr(&token, token); 
    }
    expect(&token, token, ")");
    n->then = stmt(&token, token);

    *rest = token;
    return n;
  }

  n = expr_stmt(&token, token);

  *rest = token;
  return n;
}

// expr-stmt = expr ";"?
static Node *expr_stmt(Token **rest, Token *token) {
  if (equal(token, ";")) {
    expect(&token, token, ";");
    *rest = token;
    return new_node(ND_BLOCK, token);
  }

  Node *n = new_node_unary(ND_EXPR_STMT, expr(&token, token), token);
  expect(&token, token, ";");
  *rest = token;
  return n;
}

// expr = assign
static Node *expr(Token **rest, Token *token) {
  Node *n = assign(&token, token);
  *rest = token;
  return n;
}

// assign = equality ("=" assign)?
static Node *assign(Token **rest, Token *token) {
  Node *n = equality(&token, token);
  if (consume(&token, token, "=")) {
    n = new_node_binary(ND_ASSIGN, n, assign(&token, token), token);
  }

  *rest = token;
  return n;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **rest, Token *token) {
  Node *n = relational(&token, token);

  for (;;) {
    if (consume(&token, token, "==")) {
      n = new_node_binary(ND_EQ, n, relational(&token, token), token);
    } else if (consume(&token, token, "!=")) {
      n = new_node_binary(ND_NEQ, n, relational(&token, token), token);
    } else {
      *rest = token;
      return n;
    }
  }

  *rest = token;
  return n;
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **rest, Token *token) {
  Node *n = add(&token, token);

  for (;;) {
    if (consume(&token, token, "<")) {
      n = new_node_binary(ND_LT, n, add(&token, token), token);
    } else if (consume(&token, token, ">")) {
      n = new_node_binary(ND_LT, add(&token, token), n, token);
    } else if (consume(&token, token, "<=")) {
      n = new_node_binary(ND_LE, n, add(&token, token), token);
    } else if (consume(&token, token, ">=")) {
      n = new_node_binary(ND_LE, add(&token, token), n, token);
    } else {
      *rest = token;
      return n;
    }
  }

  *rest = token;
  return n;
}

static Node *new_add(Node *lhs, Node *rhs, Token *token) {
  add_type(lhs);
  add_type(rhs);

  Node *n;

  // num + num
  if (!lhs->ty->next && !rhs->ty->next) {
    n = new_node_binary(ND_ADD, lhs, rhs, token);
    return n;
  }

  // pointer + pointer
  if (lhs->ty->next && rhs->ty->next) {
    error_at(token->loc, "%s", "invalid operand");
  }

  // num + pointer to pointer + num
  if (!lhs->ty->next && rhs->ty->next) {
    Node *tmp = lhs;
    lhs = rhs;
    rhs = tmp;
  }
  
  // pointer + num * ty->size
  // int -> 4byte
  n = new_node_binary(ND_ADD, lhs, new_node_binary(ND_MUL, rhs, new_node_num(lhs->ty->next->size, token), token), token);

  return n;
}

static Node *new_sub(Node *lhs, Node *rhs, Token *token) {
  add_type(lhs);
  add_type(rhs);

  Node *n;

  // num - num
  if (!lhs->ty->next && !rhs->ty->next) {
    n = new_node_binary(ND_SUB, lhs, rhs, token);
    return n;
  }

  // pointer - num * ty->size 
  // int -> 4byte
  if (lhs->ty->next && !rhs->ty->next) {
    n = new_node_binary(ND_SUB, lhs, new_node_binary(ND_MUL, rhs, new_node_num(rhs->ty->size, token), token), token);
    return n;
  }

  // pointer - pointer, return int elements between pointer and pointer
  if (lhs->ty->next && rhs->ty->next) {
    n = new_node_binary(ND_DIV, new_node_binary(ND_SUB, lhs, rhs, token), new_node_num(rhs->ty->next->size, token), token);
    return n;
  }

  error_at(token->loc, "%s", "invalid operand");
}

// add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *token) {
  Node *n = mul(&token, token);

  for (;;) {
    if (consume(&token, token, "+")) {
      n = new_add(n, mul(&token, token), token);
    } else if (consume(&token, token, "-")) {
      n = new_sub(n, mul(&token, token), token);
    } else {
      *rest = token;
      return n;
    }
  }

  *rest = token;
  return n;
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **rest, Token *token) {
  Node *n = unary(&token, token);

  for (;;) {
    if (consume(&token, token, "*")) {
      n = new_node_binary(ND_MUL, n, unary(&token, token), token);
    } else if (consume(&token, token, "/")) {
      n = new_node_binary(ND_DIV, n, unary(&token, token), token);
    } else {
      *rest = token;
      return n;
    }
  }

  *rest = token;
  return n;
}

// unary = "sizeof | *"? unary |
//         ("+" | "-")? primary |
//         "*" unary |
//         "&" unary |
//         postfix
static Node *unary(Token **rest, Token *token) {
  Node *n;

  if (equal(token, "sizeof")) {
    token = token->next;
    n = unary(&token, token);
    add_type(n);

    *rest = token;
    return new_node_num(n->ty->size, token);
  }

  if (consume(&token, token, "+")) {
    n = unary(&token, token);
    *rest = token;
    return n;
  }

  if (consume(&token, token, "-")) {
    n = new_node_binary(ND_NEG, unary(&token, token), NULL, token);
    *rest = token;
    return n;
  }

  if (consume(&token, token, "*")) {
    n = new_node(ND_DEREF, token);
    n->lhs = unary(&token, token);

    *rest = token;
    return n;
  }

  if (consume(&token, token, "&")) {
    n = new_node(ND_ADDR, token);
    n->lhs = unary(&token, token);

    *rest = token;
    return n;
  }

  n = postfix(&token, token);
  *rest = token;
  return n;
}

// postfix = primary ("[" expr "]")*
static Node *postfix(Token **rest, Token *token) {
  Node *n = primary(&token, token);

  //array
  while (equal(token, "[")) {
    token = token->next;

    Node *ex = expr(&token, token);

    // a[3] -> *(a+3) -> *(a + 3 * ty->size)
    n = new_node_binary(ND_DEREF, new_add(n, ex, token), NULL, token);

    expect(&token, token, "]");
  }

  *rest = token;
  return n;

}

static Node *funcall(Token **rest, Token *token) {

  Token *start = token;

  Node head = {};
  Node *cur = &head;
  
  // funtionname (arg, ...);
  token = token->next;
  expect(&token, token, "(");

  while (!equal(token, ")")) {
    if (cur != &head)
      expect(&token, token, ",");
    cur = cur->next = assign(&token, token);
  }

  expect(&token, token, ")");

  Node *n = new_node(ND_FUNC, token);
  n->funcname = strndup(start->loc, start->len);
  n->args = head.next;

  *rest = token;
  return n;
}

/*
  primary = "(" "{" stmt+ "}" ")"
          | "(" expr ")"
          | ident ( "(" assign "," ")" )?
          | str
          | num
*/
static Node *primary(Token **rest, Token *token) {
  if (equal(token, "(") && equal(token->next, "{")) {
    token = token->next->next;

    enter_scope();

    Node head = {};
    Node *cur = &head;
    // ({stmt+})のstmtの中でreturnがあるのを許してしまっている。
    while (!equal(token, "}")) {
      cur->next = stmt(&token, token);
      cur = cur->next;
    }

    expect(&token, token, "}");

    leave_scope();

    Node *n = new_node(ND_STMT_EXPR, token);
    if (!head.next) {
      error_at(token->loc, "%s", "empty block");
    }
    n->body = head.next;

    expect(&token, token, ")");

    *rest = token;
    return n;
  }

  if (consume(&token, token, "(")) {
    Node *n = expr(&token, token);
    expect(&token, token, ")");
    *rest = token;
    return n;
  }

  if (token->kind == TK_IDENT) {
    //funcall
    if (equal(token->next, "("))
      return funcall(rest, token);

    // lvar
    Node *n = new_node(ND_VAR, token);
    Obj *lvar = find_var(token);
    if (lvar) {
      n->var = lvar;
    } else {
      error_at(token->loc, "%s", "variable not definded");
    }

    token = token->next;
    *rest = token;
    return n;
  }

  if (token->kind == TK_STR) {
    // string literalは、.data領域に格納する。
    Obj *var = new_string_literal(token->str, token->ty);
    *rest = token->next;
    return new_node_var(var, token);
  }

  if (token->kind == TK_NUM) {
    int v = token->val;
    token = token->next;

    *rest = token;
    return new_node_num(v, token);
  }

  error_at(token->loc, "%s", "expect ({}), (), ident, funcall, str and num");

}
