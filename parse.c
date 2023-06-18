#include "9cc.h"

Obj *locals;
Obj *globals;
Obj *find_var(Token *t);

Type *declspec(Token **rest, Token *token);
Node *declaration(Token **rest, Token *token);
Type *declarator(Token **rest, Token *token, Type *ty);
Type *type_suffix(Token **rest, Token *token, Type *ty);
Node *stmt(Token **rest, Token *token);
Node *expr_stmt(Token **rest, Token *token);
Node *expr(Token **rest, Token *token);
Node *assign(Token **rest, Token *token);
Node *equality(Token **rest, Token *token);
Node *relational(Token **rest, Token *token);
Node *add(Token **rest, Token *token);
Node *mul(Token **rest, Token *token);
Node *unary(Token **rest, Token *token);
Node *postfix(Token **rest, Token *token);
Node *primary(Token **rest, Token *token);

Node *new_node(NodeKind kind) {
  Node *n = calloc(1, sizeof(Node));
  n->kind = kind;
  return n;
}

Node *new_node_unary(NodeKind kind, Node *lhs) {
  Node *n = new_node(kind);
  n->lhs= lhs;
  return n;
}

Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *n = new_node(kind);
  n->lhs = lhs;
  n->rhs = rhs;
  return n;
}

Node *new_node_num(int val) {
  Node *n = new_node(ND_NUM);
  n->val = val;
  return n;
}

Node *new_node_var(Obj *var) {
  Node *n = new_node(ND_VAR);
  n->var = var;
  return n;
}

Obj *new_var(char *name, Type *ty) {
  Obj *var;
  var = calloc(1, sizeof(Obj));
  var->name = name;
  var->len = strlen(name);
  var->ty = ty;

  return var;
}

Obj *new_lvar(char *name, Type *ty) {
  Obj *lvar = new_var(name, ty);
  lvar->is_local = true;
  lvar->next = locals;
  locals = lvar;

  return lvar;
}

Obj *new_gvar(char *name, Type *ty) {
  Obj *gvar = new_var(name, ty);
  gvar->next = globals;
  globals = gvar;

  return gvar;
}

char *new_unique_name() {
  static id = 0;
  char *buf = calloc(1, 20);
  sprintf(buf, ".L..%d", id++);
  return buf;
}

Obj *new_string_literal(char *str, Type *ty) {
  Obj *gvar = new_gvar(new_unique_name(), ty);
  gvar->init_data = str;

  return gvar;
}

Obj *find_var(Token *t) {
  for (Obj *var = locals; var; var = var->next) {
    if (var->len == t->len && strncmp(t->loc, var->name, var->len) == 0) {
      return var;
    }
  }

  for (Obj *var = globals; var; var = var->next) {
    if (var->len == t->len && strncmp(t->loc, var->name, var->len) == 0) {
      return var;
    }
  }

  return NULL;
}

char *get_ident_name(Token *t) {
  if (t->kind != TK_IDENT) {
    error("%s", "expected an identifier");
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
    //error_at(token->loc, "no op: %s", op);
    error("no op: %s", op);
    exit(1);
  }
}

bool expect_ident(Token **rest, Token *token) {
  if (token->kind == TK_IDENT) {
    *rest = token->next;
  } else {
    error("%s", "expect TK_IDENT");
  }
}

bool at_eof(Token *token) {
  return token->kind == TK_EOF;
}

bool is_function(Token *token) {
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

bool is_type(Token *token) {
  return equal(token, "int") || equal(token, "char");
}

void create_params(Token **rest, Token *token) {
  expect(&token, token, "(");

  while (!equal(token, ")")) {
    if (locals != NULL)
      expect(&token, token, ",");


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
Obj *function (Token **rest, Token *token) {
  Type *ty = declspec(&token, token);

  Obj *fn = new_gvar(get_ident_name(token), ty);
  fn->is_function = true;
  token = token->next;

  locals = NULL;
  create_params(&token, token);

  fn->params = locals;

  fn->body = stmt(&token, token);
  add_type(fn->body);
  fn->locals = locals;

  *rest = token;
  return fn;
}

Obj *global_variable (Token **rest, Token *token) {
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
        error("%s", "defined gloval variable");
      }

      gvar = new_gvar(get_ident_name(ty->token), ty);
    }
  
    expect(&token, token, ";");
    *rest = token;
    return gvar;
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
Type *declspec(Token **rest, Token *token) {
  Type *ty;
  if (equal(token, "int")) {
    ty = ty_int();
  } else if (equal(token, "char")) {
    ty = ty_char();
  } else {
    error("%s", "no int type");
  }

  *rest = token->next;
  return ty;
}

// declaration ::= declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
Node *declaration(Token **rest, Token *token) {
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
        error("%s", "defined variable");
      }

      lvar = new_lvar(get_ident_name(ty->token), ty);

      if (!consume(&token, token, "=")) {
        continue;
      }

      Node *lhs = new_node_var(lvar);
      Node *rhs = expr(&token, token);
      Node *node = new_node_binary(ND_ASSIGN, lhs, rhs);

      cur = cur->next = new_node_unary(ND_EXPR_STMT, node);
    }

    Node *n = new_node(ND_BLOCK);
    n->body = head.next;
    expect(&token, token, ";");
    *rest = token;
    return n;
}

// declarator ::= "*"* ident type-suffix
Type *declarator(Token **rest, Token *token, Type *ty) {
    while (consume(&token, token, "*")) {
      ty = pointer_to(ty);
    }

    if (token->kind != TK_IDENT) {
      error("%s", "expect TK_IDENT");
    }

    ty = type_suffix(rest, token->next, ty);
    //ident
    ty->token = token;

    return ty;
}

// type-suffix ::= "[" num "]" type-suffix
Type *type_suffix(Token **rest, Token *token, Type *ty) {
  if (equal(token, "[")) {
    token = token->next;

    if (token->kind != TK_NUM) {
      error("%s", "expect TK_NUM");
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

// stmt = expr? ";" |
//        "{" stmt* "}" |
//        "if" "(" expr ")" stmt ("else" stmt)? |
//        "while" "(" expr ")" stmt |
//        "for" "(" expr? ";" expr? ";" expr? ";"  ")" stmt |
//        "return" expr ";" |
//         declaration
Node *stmt(Token **rest, Token *token) {
  Node *n;

  if (consume(&token, token, ";")) {
    n = calloc(1, sizeof(Node));
    n->kind = ND_BLOCK;
    *rest = token;
    return n;
  }

  if (equal(token, "{")) {
    n = calloc(1, sizeof(Node));
    n->kind = ND_BLOCK;
    expect(&token, token, "{");
    Node head = {};
    Node *cur = &head;
    while (!equal(token, "}")) {
      cur->next = stmt(&token, token);
      cur = cur->next;
    }

    expect(&token, token, "}");
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
    n = calloc(1, sizeof(Node));
    n->kind = ND_RETURN;
    token = token->next;
    n->lhs = expr(&token, token);
    expect(&token, token, ";");

    *rest = token;
    return n;
  }

  if (equal(token, "if")) {
    n = calloc(1, sizeof(Node));
    n->kind = ND_IF;
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
    n = calloc(1, sizeof(Node));
    n->kind = ND_WHILE;
    token = token->next;
    expect(&token, token, "(");
    n->cond = expr(&token, token);
    expect(&token, token, ")");
    n->then = stmt(&token, token);

    *rest = token->next;
    return n;
  }

  if (equal(token, "for")) {
    n = calloc(1, sizeof(Node));
    n->kind = ND_FOR;
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
Node *expr_stmt(Token **rest, Token *token) {
  if (equal(token, ";")) {
    expect(&token, token, ";");
    *rest = token;
    return new_node(ND_BLOCK);
  }

  Node *n = new_node_unary(ND_EXPR_STMT, expr(&token, token));
  expect(&token, token, ";");
  *rest = token;
  return n;
}

// expr = assign
Node *expr(Token **rest, Token *token) {
  Node *n = assign(&token, token);
  *rest = token;
  return n;
}

// assign = equality ("=" assign)?
Node *assign(Token **rest, Token *token) {
  Node *n = equality(&token, token);
  if (consume(&token, token, "=")) {
    n = new_node_binary(ND_ASSIGN, n, assign(&token, token));
  }

  *rest = token;
  return n;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality(Token **rest, Token *token) {
  Node *n = relational(&token, token);

  for (;;) {
    if (consume(&token, token, "==")) {
      n = new_node_binary(ND_EQ, n, relational(&token, token));
    } else if (consume(&token, token, "!=")) {
      n = new_node_binary(ND_NEQ, n, relational(&token, token));
    } else {
      *rest = token;
      return n;
    }
  }

  *rest = token;
  return n;
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational(Token **rest, Token *token) {
  Node *n = add(&token, token);

  for (;;) {
    if (consume(&token, token, "<")) {
      n = new_node_binary(ND_LT, n, add(&token, token));
    } else if (consume(&token, token, ">")) {
      n = new_node_binary(ND_LT, add(&token, token), n);
    } else if (consume(&token, token, "<=")) {
      n = new_node_binary(ND_LE, n, add(&token, token));
    } else if (consume(&token, token, ">=")) {
      n = new_node_binary(ND_LE, add(&token, token), n);
    } else {
      *rest = token;
      return n;
    }
  }

  *rest = token;
  return n;
}

Node *new_add(Node *lhs, Node *rhs, Token *token) {
  add_type(lhs);
  add_type(rhs);

  Node *n;

  // num + num
  if (!lhs->ty->next && !rhs->ty->next) {
    n = new_node_binary(ND_ADD, lhs, rhs);
    return n;
  }

  // pointer + pointer
  if (lhs->ty->next && rhs->ty->next) {
    error("%s", "invalid operand");
  }

  // num + pointer to pointer + num
  if (!lhs->ty->next && rhs->ty->next) {
    Node *tmp = lhs;
    lhs = rhs;
    rhs = tmp;
  }
  
  // pointer + num * ty->size
  // int -> 4byte
  n = new_node_binary(ND_ADD, lhs, new_node_binary(ND_MUL, rhs, new_node_num(lhs->ty->next->size)));

  return n;
}

Node *new_sub(Node *lhs, Node *rhs, Token *token) {
  add_type(lhs);
  add_type(rhs);

  Node *n;

  // num - num
  if (!lhs->ty->next && !rhs->ty->next) {
    n = new_node_binary(ND_SUB, lhs, rhs);
    return n;
  }

  // pointer - num * ty->size 
  // int -> 4byte
  if (lhs->ty->next && !rhs->ty->next) {
    n = new_node_binary(ND_SUB, lhs, new_node_binary(ND_MUL, rhs, new_node_num(rhs->ty->size)));
    return n;
  }

  // pointer - pointer, return int elements between pointer and pointer
  if (lhs->ty->next && rhs->ty->next) {
    n = new_node_binary(ND_DIV, new_node_binary(ND_SUB, lhs, rhs), new_node_num(rhs->ty->next->size));
    return n;
  }

  error("%s", "invalid operand");
}

// add = mul ("+" mul | "-" mul)*
Node *add(Token **rest, Token *token) {
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
Node *mul(Token **rest, Token *token) {
  Node *n = unary(&token, token);

  for (;;) {
    if (consume(&token, token, "*")) {
      n = new_node_binary(ND_MUL, n, unary(&token, token));
    } else if (consume(&token, token, "/")) {
      n = new_node_binary(ND_DIV, n, unary(&token, token));
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
Node *unary(Token **rest, Token *token) {
  Node *n;

  if (equal(token, "sizeof")) {
    token = token->next;
    n = unary(&token, token);
    add_type(n);

    *rest = token;
    return new_node_num(n->ty->size);
  }

  if (consume(&token, token, "+")) {
    n = unary(&token, token);
    *rest = token;
    return n;
  }

  if (consume(&token, token, "-")) {
    n = new_node_binary(ND_NEG, unary(&token, token), NULL);
    *rest = token;
    return n;
  }

  if (consume(&token, token, "*")) {
    n = calloc(1, sizeof(Node));
    n->kind = ND_DEREF;
    n->lhs = unary(&token, token);

    *rest = token;
    return n;
  }

  if (consume(&token, token, "&")) {
    n = calloc(1, sizeof(Node));
    n->kind = ND_ADDR;
    n->lhs = unary(&token, token);

    *rest = token;
    return n;
  }

  n = postfix(&token, token);
  *rest = token;
  return n;
}

// postfix = primary ("[" expr "]")*
Node *postfix(Token **rest, Token *token) {
  Node *n = primary(&token, token);

  //array
  while (equal(token, "[")) {
    token = token->next;

    Node *ex = expr(&token, token);

    // a[3] -> *(a+3) -> *(a + 3 * ty->size)
    n = new_node_binary(ND_DEREF, new_add(n, ex, token), NULL);

    expect(&token, token, "]");
  }

  *rest = token;
  return n;

}

Node *funcall(Token **rest, Token *token) {

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

  Node *n = calloc(1, sizeof(Node));
  n->kind = ND_FUNC;
  n->funcname = strndup(start->loc, start->len);
  n->args = head.next;

  *rest = token;
  return n;
}

// primary = num | str |
//           ident ( "(" assign "," ")" )? | 
//           "(" expr ")"
Node *primary(Token **rest, Token *token) {

  if (consume(&token, token, "(")) {
    Node *n = expr(&token, token);
    expect(&token, token, ")");
    *rest = token;
    return n;
  }

  if (token->kind == TK_NUM) {
    int v = token->val;
    token = token->next;

    *rest = token;
    return new_node_num(v);
  }

  if (token->kind == TK_IDENT) {
    //funcall
    if (equal(token->next, "("))
      return funcall(rest, token);

    // lvar
    Node *n = new_node(ND_VAR);
    Obj *lvar = find_var(token);
    if (lvar) {
      n->var = lvar;
    } else {
      error("%s", "variable not definded");
    }

    token = token->next;
    *rest = token;
    return n;
  }

  if (token->kind == TK_STR) {
    Obj *var = new_string_literal(token->str, token->ty);
    *rest = token->next;
    return new_node_var(var);
  }

  error("%s", "no num, no ident, no func");

}
