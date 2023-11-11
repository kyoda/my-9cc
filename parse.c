#include "9cc.h"

static Obj *locals;
static Obj *globals;
static Scope *scope = &(Scope){};

static Type *declspec(Token **rest, Token *token, VarAttr *attr);
static Node *declaration(Token **rest, Token *token);
static Type *declarator(Token **rest, Token *token, Type *ty);
static Type *type_suffix(Token **rest, Token *token, Type *ty);
static Type *func_params(Token **rest, Token *token, Type *ty);
static Node *stmt(Token **rest, Token *token);
static Node *expr_stmt(Token **rest, Token *token);
static Node *expr(Token **rest, Token *token);
static Node *assign(Token **rest, Token *token);
static Node *equality(Token **rest, Token *token);
static Node *relational(Token **rest, Token *token);
static Node *add(Token **rest, Token *token);
static Node *mul(Token **rest, Token *token);
static Node *cast(Token **rest, Token *token);
static Type *typename(Token **rest , Token *token);
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

static Node *new_long(int64_t val, Token *token) {
  Node *n = new_node(ND_NUM, token);
  n->val = val;
  n->ty = ty_long();
  return n;
}

static Node *new_node_num(int64_t val, Token *token) {
  Node *n = new_node(ND_NUM, token);
  n->val = val;
  return n;
}

static Node *new_node_var(Obj *var, Token *token) {
  Node *n = new_node(ND_VAR, token);
  n->var = var;
  return n;
}

Node *new_cast(Node *lhs, Type *ty, Token *token) {
  add_type(lhs);
  Node *n = new_node_unary(ND_CAST, lhs, token);
  n->ty = ty;
  return n;
}

static Type *find_tag_type(Token *t) {
  for (Scope *sc = scope; sc; sc = sc->next) {
    for (TagScope *ts = sc->tags; ts; ts = ts->next) {
      if (equal(t, ts->name)) {
        return ts->ty;
      }
    }
  }

  return NULL;
}

static void *push_new_tag(Token *token, Type *ty) {
  TagScope *tag = calloc(1, sizeof(TagScope));
  tag->name = strndup(token->loc, token->len);
  tag->ty = ty;
  tag->next = scope->tags;
  scope->tags = tag;
}

static VarScope *find_var(Token *t) {
  for (Scope *sc = scope; sc; sc = sc->next) {
    for (VarScope *vs = sc->vars; vs; vs = vs->next) {
      if (equal(t, vs->name)) {
        return vs;
      }
    }
  }

  return NULL;
}

//search only global variable
static VarScope *find_var_by_name(char *name) {
  for (Scope *sc = scope; sc; sc = sc->next) {
    for (VarScope *vs = sc->vars; vs; vs = vs->next) {
      if (strncmp(name, vs->name, strlen(name)) == 0) {
        return vs;
      }
    }
  }

  return NULL;
}

static VarScope *push_scope(char *name) {
  VarScope *v = calloc(1, sizeof(VarScope));
  v->name = name;
  v->next = scope->vars;
  scope->vars = v;

  return v;
}

static Obj *new_var(char *name, Type *ty) {
  Obj *var;
  var = calloc(1, sizeof(Obj));
  var->name = name;
  var->len = strlen(name);
  var->ty = ty;

  push_scope(name)->var = var;

  return var;
}

/*
  新しい変数をリストの先頭に追加
  new --> ~~~ -> second -> first
  先頭アドレスはlocals

  例:
  int exp(int a, int b, int c) {
     int d, e, f;
     return a + b + c + d + e + f;
  };

  リストは下記となる
  f -> e -> d -> c -> a -> b -> c
  params部分は、順番が逆としている
  
*/
static Obj *new_lvar(char *name, Type *ty) {
  Obj *lvar = new_var(name, ty);
  lvar->is_local = true;
  lvar->next = locals;

  return lvar;
}

static Obj *new_gvar(char *name, Type *ty) {
  //VarScope *vs = find_var_by_name(name);
  //if (vs) {
  //  error("%s: defined variable", name);
  //}

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

/*
  scopeの先頭に新しいscopeを追加
  一番古いscopeは、global
*/
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

static Type *find_typedef(Token *t) {
  //if (t->kind != TK_IDENT) {
  //  error_at(t->loc, "%s", "expected an typedef identifier");
  //}

  VarScope *vs = find_var(t);
  if (vs) {
    return vs->type_def;
  }

  return NULL;
}

static Member *find_member(Type *ty, Token *token) {
  for (Member *m = ty->members; m; m = m->next) {
    if (equal(token, m->name)) {
      return m;
    }
  }

  error_at(token->loc, "no member: %.*s", token->len, token->loc);
}

static char *get_ident_name(Token *t) {
  if (t->kind != TK_IDENT) {
    error_at(t->loc, "%s", "expected an identifier");
  }
    
  return strndup(t->loc, t->len);
}

bool consume(Token **rest, Token *token, char *op) {
  if (token->kind == TK_PUNCT && equal(token, op)) {
    *rest = token->next;
    return true;
  }

  return false;
}

bool expect(Token **rest, Token *token, char *op) {
  if (token->kind == TK_PUNCT && equal(token, op)) {
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
  Type dummy = {};
  // functionかどうか先読みを行う
  Type *ty = declarator(&token, token, &dummy);

  return ty->kind == TY_FUNC;
}

static bool is_type(Token *token) {
  char *kw[] = {
    "void", "char", "short", "int", "long", "struct", "union", "typedef"
  };
  for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
    if (equal(token, kw[i])) {
      return true;
    }
  }

  return find_typedef(token);
}

static void create_params(Type *param) {
  if (param) {
    create_params(param->next);
    locals = new_lvar(get_ident_name(param->token), param);
  }
}

// function ::= declspec declarator ( stmt? | ";")
static void *function (Token **rest, Token *token, Type *basety) {
  Type *ty = declarator(&token, token, basety);
  Obj *fn = new_gvar(get_ident_name(ty->token), ty);
  fn->is_function = true;
  fn->is_definition = !consume(&token, token, ";");
  if (!fn->is_definition) {
    *rest = token;
    return;
  }

  enter_scope();

  locals = NULL;
  //このタイミングでparamsをlvarにする
  create_params(ty->params);

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

static void *global_variable (Token **rest, Token *token, Type *basety) {
    int i = 0;
    while (!equal(token, ";")) {
      if (i > 0) {
        expect(&token, token, ",");
      }
      i++;

      Type *ty = declarator(&token, token, basety);

      Obj *gvar = new_gvar(get_ident_name(ty->token), ty);

      if (consume(&token, token, "=")) {
        gvar_initializer(&token, token, gvar);
      }
    }
  
    expect(&token, token, ";");
    *rest = token;
}

// parse_typedef ::= declarator ";"
static void parse_typedef(Token **rest, Token *token, Type *basety) {

  int c = 0;

  // typedef int t;
  while (!consume(&token, token, ";")) {
    if (c > 0) {
      expect(&token, token, ",");
    }

    Type *ty = declarator(&token, token, basety);
    push_scope(get_ident_name(ty->token))->type_def = ty;

    c++;
  }
  
  *rest = token;
}

// program ::= (declspec (parse_typedef | function | global_variable))*
Obj *parse(Token *token) {
  globals = NULL;

  while (token->kind != TK_EOF) {
    VarAttr attr = {};
    Type *basety = declspec(&token, token, &attr);

    //typedef int t;
    if (attr.is_typedef) {
      parse_typedef(&token, token, basety);  
      continue;
    }

    if (is_function(token)) {
      function(&token, token, basety);
      continue;
    }

    global_variable(&token, token, basety);
  }

  return globals;
}

/*
  
  新しいmemberをリストの最後に追加
  old -> new 

  例；
  struct {
    int *a;
    int *b;
  } x;

  b->offset = 8
  a->offset = 0
  xの先頭アドレスはaのアドレスと同じ

  stack
  +------------------------------
  | rbp
  | 7fff ffff ffff ffff 
  +------------------------------
  | [rbp - x->offset + b->offset]
  | 6fff ffff ffff ffff
  +------------------------------
  | [rbp - x->offset + a->offset]
  | 5fff ffff ffff ffff
  +------------------------------
  | ~~~
  +------------------------------
  | rsp
  +------------------------------
  | ~~~
  +------------------------------
  | 0x 0000 0000 0000 0000
  +------------------------------
  
*/
// struct-member ::= (declspec declarator ("," declarator)* ";")*
static void struct_member(Token **rest, Token *token, Type *ty) {
  Member head = {};
  Member *cur = &head;

  while (!equal(token, "}")) {
    Type *basety = declspec(&token, token, NULL);

    int i = 0;
    while (!equal(token, ";")) {
      if (i > 0) {
        expect(&token, token, ",") ;
      }
      i++;

      Member *mem = calloc(1, sizeof(Member));
      mem->ty = declarator(&token, token, basety);
      mem->name = get_ident_name(mem->ty->token);
      cur = cur->next = mem;
    }

    // ";"
    token = token->next;
  }

  ty->members = head.next;
  *rest = token;
}

// struct-union-decl ::= "struct" ident "{" struct-member "}"
static Type *struct_union_decl(Token **rest, Token *token) {
  // "struct"
  token = token->next;

  Token *token_tag = NULL;
  if (token->kind == TK_IDENT) {
    //tokenだけ保持しておき、後でstructのtypeを作成した後にtagを追加する
    token_tag = token;
    token = token->next;
  }

  // struct tag x; といったtagによるstructの宣言
  if (token_tag && !equal(token, "{")) {
    Type *ty = find_tag_type(token_tag);
    if (!ty) {
      error_at(token_tag->loc, "%s", "undefined struct tag");
    }

    *rest = token;
    return ty;
  }

  expect(&token, token, "{");

  Type *ty = calloc(1, sizeof(Type));
  ty->align = 1;

  struct_member(&token, token, ty);
  expect(&token, token, "}");

  if (token_tag) {
    push_new_tag(token_tag, ty);
  }

  *rest = token;
  return ty;
}

// struct-decl ::= struct-union-decl
static Type *struct_decl(Token **rest, Token *token) {
  Type *ty = struct_union_decl(rest, token);
  ty->kind = TY_STRUCT;

  int offset = 0;
  for (Member *m = ty->members; m; m = m->next) {
    m->offset = offset;
    // memberのtypeのalignに合わせてoffsetをalignした後、memberのtype->sizeをoffsetに加算
    offset = align_to(offset, m->ty->align);
    offset += m->ty->size;

    // structのtypeのalignは、memberのalignの最大値
    if (ty->align < m->ty->align) {
      ty->align = m->ty->align;
    }
  }

  //structのalign(memberのalignの最大値)に合わせてoffsetをalignしそれをsizeとする
  ty->size = align_to(offset, ty->align);

  return ty;
}

// union-decl ::= struct-union-decl
static Type *union_decl(Token **rest, Token *token) {
  Type *ty = struct_union_decl(rest, token);
  ty->kind = TY_UNION;

  int offset = 0;
  for (Member *m = ty->members; m; m = m->next) {
    // unionのtypeのalignは、memberのalignの最大値
    if (ty->align < m->ty->align) {
      ty->align = m->ty->align;
    }
    // unionのtypeのsizeは、memberのsizeの最大値
    if (ty->size< m->ty->size) {
      ty->size = m->ty->size;
    }
  }

  ty->size = align_to(ty->size, ty->align);

  return ty;
}

/*
  declspec ::= ("char" || "short" || "int" || "long"
            || "struct-decl" || "union-decl"
            || "typedef" || typedef-name)+
*/
static Type *declspec(Token **rest, Token *token, VarAttr *attr) {
  enum {
    VOID = 1 << 0,
    CHAR = 1 << 2,
    SHORT = 1 << 4,
    INT = 1 << 6,
    LONG = 1 << 8,
    OTHER = 1 << 10
  };

  Type *ty = ty_int();
  int counter = 0; 

  while (is_type(token)) {
    //"typedef"
    if (equal(token, "typedef")) {
      if (!attr) {
        error(token->loc, "%s", "typedef is not allowed");
      }

      attr->is_typedef = true;
      token = token->next;
      continue;
    }
    
    //"struct" "union" typedef-name
    Type *ty2 = find_typedef(token);
    if (equal(token, "struct") || equal(token, "union") || ty2) {
      // struct や union, typedefで定義された型は、重複した定義はない
      if (counter) {
        break;
      }

      if (equal(token, "struct")) {
        ty = struct_decl(&token, token);
      } else if (equal(token, "union")) {
        ty = union_decl(&token, token);
      } else {
        //t x; (typedef int t;)
        ty = ty2;
        token = token->next;
      }

      counter += OTHER;
      continue;
    }

    if (equal(token, "void")) {
      counter += VOID;
    } else if (equal(token, "char")) {
      counter += CHAR;
    } else if (equal(token, "short")) {
      counter += SHORT;
    } else if (equal(token, "int")) {
      counter += INT;
    } else if (equal(token, "long")) {
      counter += LONG;
    } else {
      error(token->loc, "%s", "none of type defined");
    }

    switch(counter) {
    case VOID:
      ty = ty_void();
      break;
    case CHAR:
      ty = ty_char();
      break;
    case SHORT:
    case SHORT + INT:
      ty = ty_short();
      break;
    case INT:
      ty = ty_int();
      break;
    case LONG:
    case LONG + INT:
    case LONG + LONG:
    case LONG + LONG + INT:
      ty = ty_long();
      break;
    default:
      error(token->loc, "%s", "none of type defined");
    }

    token = token->next;
  }


  *rest = token;
  return ty;
}

/*
  declaration ::= declspec parse_typedef
               || declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
*/
static Node *declaration(Token **rest, Token *token) {
    VarAttr attr = {};
    Type *basety = declspec(&token, token, &attr);

    //typedef int t;
    if (attr.is_typedef) {
      parse_typedef(&token, token, basety);
      *rest = token;
      return new_node(ND_BLOCK, token);
    }

    Node head = {};
    Node *cur = &head;
    
    int i = 0;
    while (!equal(token, ";")) {
      if (i > 0) {
        expect(&token, token, ",") ;
      }
      i++;

      Type *ty = declarator(&token, token, basety);

      if (ty->kind == TY_VOID) {
        error_at(token->loc, "%s", "void type variable");
      }

      VarScope *vs = find_var(ty->token);
      if (vs) {
        error_at(ty->token->loc, "%s", "defined variable");
      }

      Obj *lvar = new_lvar(get_ident_name(ty->token), ty);
      locals = lvar;
      Node *lhs = new_node_var(lvar, token);

      if (equal(token, "=")) {
        Node *node = new_node_binary(ND_ASSIGN, lhs, assign(&token, token->next), token);
        cur = cur->next = new_node_unary(ND_EXPR_STMT, node, token);
      } else {
        cur = cur->next = lhs;
      }

    }

    Node *n = new_node(ND_BLOCK, token);
    n->body = head.next;
    expect(&token, token, ";");
    *rest = token;
    return n;
}

// declarator ::= "*"* ( "(" declarator ")" | ident ) type-suffix
static Type *declarator(Token **rest, Token *token, Type *ty) {
    while (consume(&token, token, "*")) {
      ty = pointer_to(ty);
    }

    if (equal(token, "(")) {
      Token *start = token;
      token = token->next;
      
      Type dummy = {};
      declarator(&token, token, &dummy);
      expect(&token, token, ")");

      /*
        nest内のtypeは、nest外のsuffixのtypeを指すため先読みしておく
        進めたtoken(rest)は使わない
      */
      ty = type_suffix(rest, token, ty);

      /*
        tokenは、"("の次のtoken
      */
      return declarator(&token, start->next, ty);
    }

    if (token->kind != TK_IDENT) {
      error_at(token->loc, "%s", "expect TK_IDENT");
    }

    ty = type_suffix(rest, token->next, ty);

    //ident取得用
    ty->token = token;

    return ty;
}

/*
  type-suffix ::= "(" func-params ")"
                | "[" num "]" type-suffix
                | ε
*/
static Type *type_suffix(Token **rest, Token *token, Type *ty) {
  if (equal(token, "(")) {
    return func_params(rest, token->next, ty);
  }

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

// func-params ::= "(" (declspec declarator ("," declspec declarator)*)? ")"
static Type *func_params(Token **rest, Token *token, Type *ty) {
  Type head = {};
  Type *cur = &head;

  int i = 0;
  while (!equal(token, ")")) {
    if (i > 0) {
      expect(&token, token, ",");
    }
    i++;

    Type *basety = declspec(&token, token, NULL);
    Type *ty2 = declarator(&token, token, basety);
    cur = cur->next = ty2;
  }

  /*
    TYPE_FUNC
    ty->params: Type型でparamごとにnextでのリスト
    ローカル変数生成は、function内でtoken->nameから取得しlocalsに追加する
  */

  ty = ty_func(ty);
  ty->params = head.next;
  expect(&token, token, ")");
  *rest = token;

  return ty;
}

/*
  stmt = expr? ";"
       | "{" stmt* "}"
       | typedef
       | declaration
       | "return" expr ";"
       | "if" "(" expr ")" stmt ("else" stmt)?
       | "while" "(" expr ")" stmt
       | "for" "(" expr? ";" expr? ";" expr? ";"  ")" stmt
       | expr-stmt
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
  if (consume(&token, token, ";")) {
    return new_node(ND_BLOCK, token);
  }

  Node *n = new_node_unary(ND_EXPR_STMT, expr(&token, token), token);
  expect(&token, token, ";");
  *rest = token;
  return n;
}

// expr = assign ("," expr)*
static Node *expr(Token **rest, Token *token) {
  Node *n = assign(&token, token);

  if (consume(&token, token, ",")) {
    return new_node_binary(ND_COMMA, n, expr(rest, token), token);
  }

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
  if (!lhs->ty->base && !rhs->ty->base) {
    n = new_node_binary(ND_ADD, lhs, rhs, token);
    return n;
  }

  // pointer + pointer
  if (lhs->ty->base && rhs->ty->base) {
    error_at(token->loc, "%s", "invalid operand");
  }

  // num + pointer to pointer + num
  if (!lhs->ty->base && rhs->ty->base) {
    Node *tmp = lhs;
    lhs = rhs;
    rhs = tmp;
  }
  
  // pointer + num * ty->size
  // int -> 4byte
  n = new_node_binary(ND_ADD, lhs, new_node_binary(ND_MUL, rhs, new_long(lhs->ty->base->size, token), token), token);

  return n;
}

static Node *new_sub(Node *lhs, Node *rhs, Token *token) {
  add_type(lhs);
  add_type(rhs);

  Node *n;

  // num - num
  if (!lhs->ty->base && !rhs->ty->base) {
    n = new_node_binary(ND_SUB, lhs, rhs, token);
    return n;
  }

  // pointer - num * ty->size 
  // int -> 4byte
  if (lhs->ty->base && !rhs->ty->base) {
    n = new_node_binary(ND_SUB, lhs, new_node_binary(ND_MUL, rhs, new_long(lhs->ty->size, token), token), token);
    return n;
  }

  // pointer - pointer, return int elements between pointer and pointer
  if (lhs->ty->base && rhs->ty->base) {
    n = new_node_binary(ND_DIV, new_node_binary(ND_SUB, lhs, rhs, token), new_node_num(lhs->ty->base->size, token), token);
    return n;
  }

  error_at(token->loc, "%s", "invalid operand");
}

// add = mul ("+" new_add | "-" new_sub)*
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

// mul = cast ("*" cast | "/" cast)*
static Node *mul(Token **rest, Token *token) {
  Node *n = cast(&token, token);

  for (;;) {
    if (consume(&token, token, "*")) {
      n = new_node_binary(ND_MUL, n, cast(&token, token), token);
    } else if (consume(&token, token, "/")) {
      n = new_node_binary(ND_DIV, n, cast(&token, token), token);
    } else {
      *rest = token;
      return n;
    }
  }

  *rest = token;
  return n;
}

// cast := "(" typename ")" cast | unary
static Node *cast(Token **rest, Token *token) {
  if (equal(token, "(") && is_type(token->next)) {
    Token *start = token;
    token = token->next;

    Type *ty = typename(&token, token);
    expect(&token, token, ")");

    Node *nc = cast(&token, token);

    *rest = token;
    return new_cast(nc, ty, start);
  }

  return unary(rest, token);
}

// abstract-declarator = "*"* ("(" abstract-declarator ")")? type-suffix
static Type *abstract_declarator(Type **rest, Token *token, Type *ty) {
  while (consume(&token, token, "*")) {
    ty = pointer_to(ty);
  }
  
  if (equal(token, "(")) {
    Token *start = token;
    token = token->next;
    
    Type dummy = {};
    abstract_declarator(&token, token, &dummy);
    expect(&token, token, ")");

    ty = type_suffix(rest, token, ty);

    return abstract_declarator(&token, start->next, ty);
  }

  return type_suffix(rest, token, ty);
}

// typename := declspec abstract-declarator
static Type *typename(Token **rest , Token *token) {
  Type *basety = declspec(&token, token, NULL);
  return abstract_declarator(rest, token, basety);
}

/*
  unary = "sizeof" "(" typename ")"
        | ("sizeof" | "+" | "-" | "*" | "&") cast
        | postfix
*/
static Node *unary(Token **rest, Token *token) {
  Node *n;

  if (equal(token, "sizeof") && equal(token->next, "(") && is_type(token->next->next)) {
    Token *start = token;
    token = token->next->next;

    Type *ty = typename(&token, token);
    n = new_node_num(ty->size, start);
    expect(&token, token, ")");

    *rest = token;
    return n;
  }

  if (equal(token, "sizeof")) {
    token = token->next;
    n = cast(&token, token);
    add_type(n);

    *rest = token;
    return new_node_num(n->ty->size, token);
  }

  if (consume(&token, token, "+")) {
    n = cast(&token, token);
    *rest = token;
    return n;
  }

  if (consume(&token, token, "-")) {
    n = new_node_binary(ND_NEG, cast(&token, token), NULL, token);
    *rest = token;
    return n;
  }

  if (consume(&token, token, "*")) {
    n = new_node(ND_DEREF, token);
    n->lhs = cast(&token, token);

    *rest = token;
    return n;
  }

  if (consume(&token, token, "&")) {
    n = new_node(ND_ADDR, token);
    n->lhs = cast(&token, token);

    *rest = token;
    return n;
  }

  n = postfix(&token, token);
  *rest = token;
  return n;
}

static Node *struct_ref(Token *token, Node *lhs) {
  // token is "."
  add_type(lhs);

  if (lhs->ty->kind != TY_STRUCT && lhs->ty->kind != TY_UNION) {
    error_at(token->loc, "%s", "not struct");
  }

  Node *n = new_node_unary(ND_MEMBER, lhs, token);
  n->member = find_member(lhs->ty, token->next);

  return n;
}

// postfix = primary ("[" expr "]" | "." ident | "->" ident)*
static Node *postfix(Token **rest, Token *token) {
  Node *n = primary(&token, token);

  //array
  for (;;) {
    // array
    if (equal(token, "[")) {
      token = token->next;

      Node *ex = expr(&token, token);

      // a[3] -> *(a+3) -> *(a + 3 * ty->size)
      n = new_node_binary(ND_DEREF, new_add(n, ex, token), NULL, token);

      expect(&token, token, "]");
      continue;
    }

    //struct member
    if (equal(token, ".")) {
      n = struct_ref(token, n);
      token = token->next->next;

      continue;
    }

    //struct member
    // a->b is (*a).b
    if (equal(token, "->")) {
      n = struct_ref(token, new_node_unary(ND_DEREF, n, token));
      token = token->next->next;

      continue;
    }

    *rest = token;
    return n;
  }
}

static Node *funcall(Token **rest, Token *token) {
  Token *start = token;

  VarScope *vs = find_var(token);
  if (!vs) {
    error_at(token->loc, "%s", "this function not definded");
  }
  if (!vs->var || vs->var->ty->kind != TY_FUNC) {
    error_at(token->loc, "%s", "not function");
  }

  Type *ty = vs->var->ty->return_type;

  Node head = {};
  Node *cur = &head;
  
  // funtionname (arg, ...);
  token = token->next;
  expect(&token, token, "(");

  while (!equal(token, ")")) {
    if (cur != &head)
      expect(&token, token, ",");
    cur = cur->next = assign(&token, token);
    add_type(cur);
  }

  expect(&token, token, ")");

  Node *n = new_node(ND_FUNC, token);
  n->ty = ty;
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
      // add_type()でtypeを設定せずにここでエラーにしている
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

    //var
    Node *n = new_node(ND_VAR, token);
    VarScope *vs = find_var(token);
    if (!vs || !vs->var) {
      error_at(token->loc, "%s", "variable not definded");
    }

    n->var = vs->var;
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
