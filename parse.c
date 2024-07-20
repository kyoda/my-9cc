#include "9cc.h"

static Obj *locals;
static Obj *globals;
static Obj *current_fn;
static Scope *scope = &(Scope){};

static Node *gotos;
static Node *labels;
// current goto jump target for break
static char *break_label;
static char *continue_label;

static Node *current_switch;

typedef struct Initializer Initializer;
struct Initializer {
  Initializer *next;
  Type *ty;
  Token *token;
  Node *expr; // initialization expression
  Initializer **children; // array or struct initializer
  bool is_flexible;
};

/*
  配列を初期化する際に、要素ごとにnew_add()をしassign()するが、
  そのnew_add()のindexを保持している
*/
typedef struct InitDesignator InitDesignator;
struct InitDesignator {
  InitDesignator *next;
  int idx;
  Member *member;
  Obj *var;
};

static Type *declspec(Token **rest, Token *token, VarAttr *attr);
static Node *lvar_initializer(Token **rest, Token *token, Obj *var);
static void initializer(Token **rest, Token *token, Initializer *init);
static Node *declaration(Token **rest, Token *token);
static Type *declarator(Token **rest, Token *token, Type *ty);
static Type *type_suffix(Token **rest, Token *token, Type *ty);
static Type *func_params(Token **rest, Token *token, Type *ty);
static Node *stmt(Token **rest, Token *token);
static Node *expr_stmt(Token **rest, Token *token);
static Node *expr(Token **rest, Token *token);
static Node *assign(Token **rest, Token *token);
static int64_t const_expr(Token **rest, Token *token);
static Node *to_assign(Node *lhs, Node *rhs, Token *token);
static Node *conditional(Token **rest, Token *token);
static Node *logicalor(Token **rest, Token *token);
static Node *logicaland(Token **rest, Token *token);
static Node *bitor(Token **rest, Token *token);
static Node *bitxor(Token **rest, Token *token);
static Node *bitand(Token **rest, Token *token);
static Node *equality(Token **rest, Token *token);
static Node *new_add(Node *lhs, Node *rhs, Token *token);
static Node *new_sub(Node *lhs, Node *rhs, Token *token);
static Node *relational(Token **rest, Token *token);
static Node *shift(Token **rest, Token *token);
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

/*
  同じスコープでの再定義はエラー
*/
static VarScope *find_var_in_current_scope(Token *t) {
  for (VarScope *vs = scope->vars; vs; vs = vs->next) {
    if (equal(t, vs->name)) {
      return vs;
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

static Initializer *new_initializer(Type *ty, bool is_flexible) {
  Initializer *init = calloc(1, sizeof(Initializer));
  init->ty = ty;

  if (ty->kind == TY_ARRAY) {
    if (is_flexible && ty->size < 0) {
      init->is_flexible = true;
      return init;
    }

    init->children = calloc(ty->array_len, sizeof(Initializer *));
    for (int i = 0; i < ty->array_len; i++) {
      init->children[i] = new_initializer(ty->base, false);
    }
    return init;
  }

  if (ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
    int len = 0;
    for (Member *mem  = ty->members; mem; mem = mem->next) {
      len++;
    }

    init->children = calloc(len, sizeof(Initializer *));

    for (Member *mem = ty->members; mem; mem = mem->next) {
      init->children[mem->idx] = new_initializer(mem->ty, false);
    }

    return init;
  }

  return init;
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
  f -> e -> d -> a -> b -> c
  params部分は、順番が逆としている
  
*/
static Obj *new_lvar(char *name, Type *ty) {
  Obj *lvar = new_var(name, ty);
  lvar->is_local = true;
  lvar->next = locals;

  return lvar;
}

static Obj *new_gvar(char *name, Type *ty) {
  VarScope *vs = find_var_by_name(name);
  if (vs) {
    error("%s: defined variable", name);
  }

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

// ND_ASSIGNの左辺の準備
static Node *init_desg_expr(InitDesignator *desg, Token *token) {
  if (desg->var) {
    return new_node_var(desg->var, token);
  }

  /*
                          ND_ASSIGN  -------> create_lvar_init()
                             |
                          |        |
    init_desg_expr() <-- ND_MEMBER  ND_EXPR
                           |
    init_desg_expr() <--  ND_DEREF
                           |
                           new_add()
                           |        |
                        ND_VAR    desg->idx
  */
  if (desg->member) {
    Node *lhs = init_desg_expr(desg->next, token);
    Node *n = new_node_unary(ND_MEMBER, lhs, token);
    n->member = desg->member;
    return n;
  }

  /*
    変数の頭からindex分進める
                              ND_ASSIGN  -------> create_lvar_init()
                              |        |
    init_desg_expr() <--  ND_DEREF    ND_EXPR
                              |
                            ND_ADD
                            |       |
    init_desg_expr() <-- ND_DEREF   desg->idx
                          |
                         ND_ADD
                          |     |
                      ND_VAR   desg->idx
  */
  Node *lhs = init_desg_expr(desg->next, token);
  Node *rhs = new_node_num(desg->idx, token);

  // postfixのarrayの場合の値代入の構文木と同じ
  return new_node_unary(ND_DEREF, new_add(lhs, rhs, token), token);
}

/*
  配列の初期化
  int a[2][3] = {{1, 2, 3}, {4, 5, 6}};
  の場合、
  a[0][0] = 1
  a[0][1] = 2
  a[0][2] = 3
  a[1][0] = 4
  a[1][1] = 5
  a[1][2] = 6
  をそのまま処理する。
  例えば、a[1][2] = 6の場合、
  init_desg_exprでa[1]を取得し、ポインタを4の位置に進める
  次のinit_desg_exprでa[1][2]を取得し、create_lvar_initでexprの結果として6を代入する

*/
static Node *create_lvar_init(Initializer *init, Type *ty, InitDesignator *desg, Token *token) {
  if (ty->kind == TY_ARRAY) {
    //空の配列（次のforループで何もしない）の場合、初期化は不要
    Node *node = new_node(ND_NULL_EXPR, token);

    for (int i = 0; i < ty->array_len; i++) {
      InitDesignator desg2 = {desg, i};
      Node *rhs = create_lvar_init(init->children[i], ty->base, &desg2, token);
      node = new_node_binary(ND_COMMA, node, rhs, token);
    }

    return node;
  }

  if (ty->kind == TY_STRUCT && !init->expr) {
    //空の配列（次のforループで何もしない）の場合、初期化は不要
    Node *node = new_node(ND_NULL_EXPR, token);

    
    for (Member *mem = ty->members; mem; mem = mem->next) {
      // 2番目の引数は、arrayでのみ使用するためここでは常に0とする。childrenにはmemberのidxを使う。
      InitDesignator desg2 = {desg, 0, mem};
      Node *rhs = create_lvar_init(init->children[mem->idx], mem->ty, &desg2, token);
      node = new_node_binary(ND_COMMA, node, rhs, token);
    }

    return node;
  }

  if (ty->kind == TY_UNION && !init->expr) {
      InitDesignator desg2 = {desg, 0, ty->members};
      return create_lvar_init(init->children[0], ty->members->ty, &desg2, token);
  }

  /*
    初期化するポインタを取得
    int a[3][2]
    a[0][0]: desg {next, 0, NULL} -> desg {next, 0, NULL} -> desg {NULL, 0, var}
    a[0][1]: desg {next, 1, NULL} -> desg {next, 0, NULL} -> desg {NULL, 0, var}
    a[1][0]: desg {next, 0, NULL} -> desg {next, 1, NULL} -> desg {NULL, 0, var}
    a[1][1]: desg {next, 1, NULL} -> desg {next, 1, NULL} -> desg {NULL, 0, var}
    a[2][0]: desg {next, 0, NULL} -> desg {next, 2, NULL} -> desg {NULL, 0, var}
    a[2][1]: desg {next, 1, NULL} -> desg {next, 2, NULL} -> desg {NULL, 0, var}

    例えばa[2][1]の場合、リストの最後のvarから遡ってnew_add()でpointerを2進め, 次にnew_add()でpointerを1進めるnodeを作成する
  */

  //exprがない場合はスキップ(0で初期化済み)
  if (!init->expr) {
    return new_node(ND_NULL_EXPR, token);
  } 

  Node *lhs = init_desg_expr(desg, token);
  Node *rhs = init->expr;
  return new_node_binary(ND_ASSIGN, lhs, rhs, token);
}

Token *skip_excess_elements(Token *token) {
  if (equal(token, "{")) {
    token = skip_excess_elements(token->next);
    expect(&token, token, "}");

    return token;
  }

  //下記のassign()はASTには反映しない。tokenを進めるのみ。
  assign(&token, token);
  return token;
}

static int count_array_elements(Token *token, Type *ty) {
  Initializer *dummy = new_initializer(ty, false);

  int i = 0;
  for (; !equal(token, "}"); i++) {
    if (i > 0) {
      expect(&token, token, ",");
    }
    initializer(&token, token, dummy);
  }

  return i;
}

/*
  string-initializer ::= string-literal
*/
static void string_initializer(Token **rest, Token *token, Initializer *init) {
  if (init->is_flexible) {
    /*
      "abc"の場合
      token->lenは、5
      token->ty->array_lenは、4
    */
    *init = *new_initializer(ty_array(init->ty->base, token->ty->array_len), false);
  }

  int len = MIN(token->len, init->ty->array_len);

  for (int i = 0; i < len; i++) {
    init->children[i]->expr = new_node_num(token->str[i], token);
  }

  *rest = token->next;
}

/*
  array-initializer ::= "{" initializer ("," initializer)* "}"
                      | assign
*/
static void array_initializer(Token **rest, Token *token, Initializer *init) {
  expect(&token, token, "{");

  if (init->is_flexible) {
    int len = count_array_elements(token, init->ty->base);
    *init = *new_initializer(ty_array(init->ty->base, len), false);
  }

  for (int i = 0; !consume(&token, token, "}"); i++) {
    if (i > 0) {
      expect(&token, token, ",");
    }

    if (i < init->ty->array_len) {
      initializer(&token, token, init->children[i]);
    } else {
      // a[1] = {1, 2, 3}のように、配列の要素数を超える初期化の場合はスキップ
      token = skip_excess_elements(token);
    }

  }

  *rest = token;
}

/*
  struct-initializer ::= "{" initializer ("," initializer)* "}"
                      | assign
*/
static void struct_initializer(Token **rest, Token *token, Initializer *init) {
  expect(&token, token, "{");

  Member *mem = init->ty->members;
  while (!consume(rest, token, "}")) {
    if (mem != init->ty->members) {
      expect(&token, token, ",");
    }

    if (mem) {
      initializer(&token, token, init->children[mem->idx]);
      mem = mem->next;
    } else {
      token = skip_excess_elements(token);
    }
  }
}

/*
  union-initializer ::= "{" initializer ("," initializer)* "}"
                      | assign
*/
static void union_initializer(Token **rest, Token *token, Initializer *init) {
  expect(&token, token, "{");

  bool first = true;
  while (!consume(rest, token, "}")) {
    if (first) {
      // unionの場合は、最初の要素のみ初期化する
      initializer(&token, token, init->children[0]);
      first = false;
    } else {
      expect(&token, token, ",");
      token = skip_excess_elements(token);
    }
  }
}

/*
  initializer ::= string-initializer
                | array-initializer
                | struct-initializer
                | union-initializer
                | assign
*/
static void initializer(Token **rest, Token *token, Initializer *init) {
  if (init->ty->kind == TY_ARRAY && token->kind == TK_STR) {
    string_initializer(rest, token, init);
    return;
  }

  if (init->ty->kind == TY_ARRAY) {
    array_initializer(rest, token, init);
    return;
  }

  if (init->ty->kind == TY_STRUCT) {
    // type struct T; T y; T x = y;
    if (!equal(token, "{")) {
      Node *expr = assign(rest, token);
      add_type(expr);
      if (expr->ty->kind == TY_STRUCT) {
        init->expr = expr;
        return;
      }
    }

    struct_initializer(rest, token, init);
    return;
  }

  if (init->ty->kind == TY_UNION) {
    // type union T; T y; T x = y;
    if (!equal(token, "{")) {
      Node *expr = assign(rest, token);
      add_type(expr);
      if (expr->ty->kind == TY_UNION) {
        init->expr = expr;
        return;
      }
    }

    union_initializer(rest, token, init);
    return;
  }

  /*
    上記のarray_initializer()内のforループで配列のそれぞれの要素に対して再帰的に初期化を行う
    配列でない場合でもそのままassignが成り立つ
  */
  init->expr = assign(rest, token);
}

//declaration内でassign時に呼び出される
static Node *lvar_initializer(Token **rest, Token *token, Obj *var) {
  Initializer *init = new_initializer(var->ty, true);
  initializer(rest, token, init);
  var->ty = init->ty;
  InitDesignator desg = {NULL, 0, NULL, var};

  //ユーザ指定の初期値を入れる前に、0で初期化
  Node *lhs = new_node(ND_MEMZERO, token);
  lhs->var = var;
  Node *rhs = create_lvar_init(init, var->ty, &desg, token);

  return new_node_binary(ND_COMMA, lhs, rhs, token);
}

static bool is_function(Token *token) {
  Type dummy = {};
  // functionかどうか先読みを行う
  Type *ty = declarator(&token, token, &dummy);

  return ty->kind == TY_FUNC;
}

static bool is_type(Token *token) {
  char *kw[] = {
    "_Bool", "void", "char", "short", "int", "long", "struct", "union", "enum", "typedef", "static"
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

/*
  labelは後から定義されるため、gotoのjmp先のラベル解決は後で行う
*/
static void resolve_goto_labels() {
  for (Node *x = gotos; x; x = x->goto_next) {
    for (Node *y = labels; y; y = y->goto_next) {
      if (strcmp(x->label, y->label) == 0) {
        x->unique_label = y->unique_label;
        break;
      }
    }

    if (!x->unique_label) {
      error_at(x->token->loc, "undefined label: %s", x->label);
    }
  }
}

// function ::= declspec declarator ( stmt? | ";")
static void *function (Token **rest, Token *token, Type *basety, VarAttr *attr) {
  Type *ty = declarator(&token, token, basety);
  Obj *fn = new_gvar(get_ident_name(ty->token), ty);
  fn->is_function = true;
  fn->is_definition = !consume(&token, token, ";");
  fn->is_static = attr->is_static;

  if (!fn->is_definition) {
    *rest = token;
    return;
  }

  current_fn = fn;
  enter_scope();

  locals = NULL;
  //このタイミングでparamsをlvarにする
  create_params(ty->params);

  fn->params = locals;

  fn->body = stmt(&token, token);
  add_type(fn->body);
  fn->locals = locals;

  leave_scope();

  resolve_goto_labels();

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
      function(&token, token, basety, &attr);
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

  int idx = 0;
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
      mem->idx = idx++;
      cur = cur->next = mem;
    }

    // ";"
    token = token->next;
  }

  ty->members = head.next;
  *rest = token;
}

// struct-union-decl ::= "struct" ident? ("{" struct-member "}")?
static Type *struct_union_decl(Token **rest, Token *token) {
  // "struct"
  token = token->next;

  Token *token_tag = NULL;
  if (token->kind == TK_IDENT) {
    //tokenだけ保持しておき、structのtypeを作成した後にtagを追加する
    token_tag = token;
    token = token->next;
  }

  /*
    struct tag x; といったtagによるstructの宣言
    struct tag; も不完全なtagとして扱い、上書き可能
  */
  if (token_tag && !equal(token, "{")) {
    *rest = token;

    Type *ty = find_tag_type(token_tag);
    if (ty) {
      return ty;
    }

    // incomplete struct type for sizeof()
    ty = ty_struct();
    ty->size = -1;
    push_new_tag(token_tag, ty);

    return ty;
  }

  expect(&token, token, "{");

  Type *ty = ty_struct();
  struct_member(&token, token, ty);
  expect(rest, token, "}");

  if (token_tag) {
    // tagがすでにあれば、typeの中身を上書きする
    for (TagScope *ts = scope->tags; ts; ts = ts->next) {
      if (equal(token_tag, ts->name)) {
        //ts->ty = ty; としてポインタを変更してしまうと、他で参照している場合に問題が発生する
        *ts->ty = *ty;
        return ts->ty;
      }
    }

    push_new_tag(token_tag, ty);
  }

  return ty;
}

// struct-decl ::= struct-union-decl
static Type *struct_decl(Token **rest, Token *token) {
  Type *ty = struct_union_decl(rest, token);
  ty->kind = TY_STRUCT;

  if (ty->size < 0) {
    return ty;
  }

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

  if (ty->size < 0) {
    return ty;
  }

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
  enum-specifier ::= "enum" ident? "{" enum-list? "}"
                  || "enum" ident ("{" enum-list? "}")?
  enmu-list ::= ident ("=" num)? ("," ident ("=" num)?)* 
*/
static Type *enum_specifier(Token **rest, Token *token) {
  Type *ty = ty_enum();
  // enum
  token = token->next;

  Token *token_tag = NULL;
  if (token->kind == TK_IDENT) {
    //tokenだけ保持しておき、enumのtypeを作成した後にtagを追加する
    token_tag = token;
    token = token->next;
  }

  // enum tag x; といったtagによるenumの宣言
  if (token_tag && !equal(token, "{")) {
    Type *ty = find_tag_type(token_tag);
    if (!ty) {
      error_at(token_tag->loc, "%s", "undefined enum tag");
    }

    if (ty->kind != TY_ENUM) {
      error_at(token_tag->loc, "%s", "not enum tag");
    }

    *rest = token;
    return ty;
  }


  expect(&token, token, "{");

  //enum-list
  int i = 0;
  int64_t val = 0;
  while (!equal(token, "}")) {
    if (i++ > 0) {
      expect(&token, token, ",");
    }

    char *name = get_ident_name(token);
    token = token->next;

    if (equal(token, "=")) {
      token = token->next;
      val = const_expr(&token, token);
    }

    VarScope *vs = push_scope(name);
    vs->enum_ty = ty;
    vs->enum_val = val++;
  }

  expect(&token, token, "}");

  if (token_tag) {
    push_new_tag(token_tag, ty);
  }

  *rest = token;
  return ty;
}

/*
  declspec ::= ("_Bool" || "void" || "char" || "short" || "int" || "long"
            || "struct-decl" || "union-decl" || "enum-specifier"
            || "typedef" || "static" || typedef-name)+
*/
static Type *declspec(Token **rest, Token *token, VarAttr *attr) {
  enum {
    VOID = 1 << 0,
    BOOL = 1 << 2,
    CHAR = 1 << 4,
    SHORT = 1 << 6,
    INT = 1 << 8,
    LONG = 1 << 10,
    OTHER = 1 << 12
  };

  Type *ty = ty_int();
  int counter = 0; 

  while (is_type(token)) {
    //"typedef" or "static"
    if (equal(token, "typedef") || equal(token, "static")) {
      if (!attr) {
        error(token->loc, "%s", "typedef or static is not allowed");
      }

      if (equal(token, "typedef")) {
        attr->is_typedef = true;
      } else if (equal(token, "static")) {
        attr->is_static = true;
      }

      if (attr->is_typedef && attr->is_static) {
        error(token->loc, "%s", "typedef and static may not be used together ");
      }

      token = token->next;
      continue;
    }
    
    //"struct" "union" "enum" typedef-name
    Type *ty2 = find_typedef(token);
    if (equal(token, "struct") || equal(token, "union") 
        || equal(token, "enum") || ty2) {
      // struct, union, enum, typedefで定義された型は、重複した定義はない
      if (counter) {
        break;
      }

      if (equal(token, "struct")) {
        ty = struct_decl(&token, token);
      } else if (equal(token, "union")) {
        ty = union_decl(&token, token);
      } else if (equal(token, "enum")) {
        ty = enum_specifier(&token, token);
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
    } else if (equal(token, "_Bool")) {
      counter += BOOL;
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
    case BOOL:
      ty = ty_bool();
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

      VarScope *vs = find_var_in_current_scope(ty->token);
      if (vs) {
        error_at(ty->token->loc, "%s", "defined variable");
      }

      Obj *lvar = new_lvar(get_ident_name(ty->token), ty);
      locals = lvar;
      Node *lhs = new_node_var(lvar, token);

      if (equal(token, "=")) {
        Node *expr = lvar_initializer(&token, token->next, lvar);
        cur = cur->next = new_node_unary(ND_EXPR_STMT, expr, token);
      } else {
        cur = cur->next = lhs;
      }

      if (lvar->ty->size < 0) {
        error_at(token->loc, "%s", "incomplete type");
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
                | "[" num? "]" type-suffix
                | ε
*/
static Type *type_suffix(Token **rest, Token *token, Type *ty) {
  if (equal(token, "(")) {
    return func_params(rest, token->next, ty);
  }

  if (equal(token, "[")) {
    token = token->next;

    //incomplete array
    if (equal(token, "]")) {
      ty = type_suffix(rest, token->next, ty);
      return ty_array(ty, -1);
    }

    int64_t size = const_expr(&token, token);
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

    // func(int a[])
    // a[] -> *a
    if (ty2->kind == TY_ARRAY) {
      Token *ty2_token = ty2->token;
      ty2 = pointer_to(ty2->base);
      ty2->token = ty2_token;
    }

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
       | "switch" "(" expr ")" stmt
       | "case" num ":" stmt
       | "default" ":" stmt
       | "goto" ident ";"
       | "break" ";"
       | "continue" ";"
       | ident ":" stmt
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

  // type (exclude label ":")
  if (is_type(token) && !equal(token->next, ":")) {
    n = declaration(&token, token);
    add_type(n);
    *rest = token;
    return n;
  }

  if (equal(token, "return")) {
    n = new_node(ND_RETURN, token);
    token = token->next;
    Node *exp = expr(&token, token);
    expect(&token, token, ";");

    add_type(exp);
    n->lhs = new_cast(exp, current_fn->ty->return_ty, token);

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
    n = new_node(ND_FOR, token);
    token = token->next;
    expect(&token, token, "(");
    n->cond = expr(&token, token);
    expect(&token, token, ")");

    //1個前のbreak, continueのlabelを保持
    char *tmp_break = break_label;
    char *tmp_continue = continue_label;
    //n->then内でbreak, continueがあった場合のlabelを設定する
    break_label = n->break_label = new_unique_name();
    continue_label = n->continue_label = new_unique_name();

    n->then = stmt(&token, token);

    //1個前のbreak, continueのlabelを復元
    break_label = tmp_break;
    continue_label = tmp_continue;

    *rest = token;
    return n;
  }

  if (equal(token, "for")) {
    n = new_node(ND_FOR, token);
    token = token->next;
    expect(&token, token, "(");

    enter_scope();

    if (is_type(token)) {
      n->init = declaration(&token, token);
    } else {
      n->init = expr_stmt(&token, token);
    }

    if(!equal(token, ";")) {
      n->cond = expr(&token, token); 
    }
    expect(&token, token, ";");

    if(!equal(token, ")")) {
      n->inc = expr(&token, token); 
    }
    expect(&token, token, ")");

    //1個前のbreak, continueのlabelを保持
    char *tmp_break = break_label;
    char *tmp_continue = continue_label;
    //n->then内でbreak, continueがあった場合のlabelを設定する
    break_label = n->break_label = new_unique_name();
    continue_label = n->continue_label = new_unique_name();

    n->then = stmt(&token, token);

    leave_scope();

    //1個前のbreak, continueのlabelを復元
    break_label = tmp_break;
    continue_label = tmp_continue;

    *rest = token;
    return n;
  }

  if (equal(token, "switch")) {
    n = new_node(ND_SWITCH, token);
    token = token->next;
    expect(&token, token, "(");
    n->cond = expr(&token, token);
    expect(&token, token, ")");

    Node *tmp_switch = current_switch;
    current_switch = n;

    char *tmp_break = break_label;
    break_label = n->break_label = new_unique_name();

    n->then = stmt(&token, token);

    current_switch = tmp_switch;
    break_label = tmp_break;

    *rest = token;
    return n;
  }

  if (equal(token, "case")) {
    if (!current_switch) {
      error_at(token->loc, "%s", "stray case");
    }

    n = new_node(ND_CASE, token);
    token = token->next;
    int64_t val = const_expr(&token, token);
    expect(&token, token, ":");

    n->unique_label = new_unique_name();
    n->lhs = stmt(&token, token);
    n->val = val;
    n->case_next = current_switch->case_next;
    current_switch->case_next = n;

    *rest = token;
    return n;
  }

  if (equal(token, "default")) {
    if (!current_switch) {
      error_at(token->loc, "%s", "stray default");
    }
    n = new_node(ND_CASE, token);
    token = token->next;
    expect(&token, token, ":");

    n->unique_label = new_unique_name();
    n->lhs = stmt(&token, token);

    current_switch->default_case = n;

    *rest = token;
    return n;
  }

  if (equal(token, "goto")) {
    n = new_node(ND_GOTO, token);
    n->label = get_ident_name(token->next);
    n->goto_next = gotos;
    gotos = n;
    token = token->next->next;
    
    expect(rest, token, ";");
    return n;
  }

  // label statement
  if (token->kind == TK_IDENT && equal(token->next, ":")) {
    n = new_node(ND_LABEL, token);
    n->label = get_ident_name(token);
    n->unique_label = new_unique_name();
    n->goto_next = labels;
    labels = n;
    token = token->next->next;
    n->lhs = stmt(rest, token);
    
    return n;
  }

  if (equal(token, "break")) {
    if (!break_label) {
      error_at(token->loc, "%s", "stray break");
    }

    n = new_node(ND_GOTO, token);
    n->unique_label = break_label;
    token = token->next;
    expect(rest, token, ";");
    return n;
  }

  if (equal(token, "continue")) {
    if (!continue_label) {
      error_at(token->loc, "%s", "stray continue");
    }

    n = new_node(ND_GOTO, token);
    n->unique_label = continue_label;
    token = token->next;
    expect(rest, token, ";");
    return n;
  }

  n = expr_stmt(&token, token);

  *rest = token;
  return n;
}

// expr-stmt = expr ";"?
static Node *expr_stmt(Token **rest, Token *token) {
  if (consume(&token, token, ";")) {
    *rest = token;
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

static int64_t eval(Node *n) {
  add_type(n);

  switch (n->kind) {
  case ND_ADD:
    return eval(n->lhs) + eval(n->rhs);
  case ND_SUB:
    return eval(n->lhs) - eval(n->rhs);
  case ND_MUL:
    return eval(n->lhs) * eval(n->rhs);
  case ND_DIV:
    return eval(n->lhs) / eval(n->rhs);
  case ND_MOD:
    return eval(n->lhs) % eval(n->rhs);
  case ND_OR:
    return eval(n->lhs) | eval(n->rhs);
  case ND_XOR:
    return eval(n->lhs) ^ eval(n->rhs);
  case ND_AND:
    return eval(n->lhs) & eval(n->rhs);
  case ND_NEG:
    return -eval(n->lhs);
  case ND_COND:
    return eval(n->cond) ? eval(n->then) : eval(n->els);
  case ND_LOGICALOR:
    return eval(n->lhs) || eval(n->rhs);
  case ND_LOGICALAND:
    return eval(n->lhs) && eval(n->rhs);
  case ND_COMMA:
    return eval(n->rhs);
  case ND_NOT:
    return !eval(n->lhs);
  case ND_BITNOT:
    return ~eval(n->lhs);
  case ND_SHL:
    return eval(n->lhs) << eval(n->rhs);
  case ND_SHR:
    return eval(n->lhs) >> eval(n->rhs);
  case ND_EQ:
    return eval(n->lhs) == eval(n->rhs);
  case ND_NEQ:
    return eval(n->lhs) != eval(n->rhs);
  case ND_LT:
    return eval(n->lhs) < eval(n->rhs);
  case ND_LE:
    return eval(n->lhs) <= eval(n->rhs);
  case ND_CAST:
    if (is_integer(n->ty)) {
      switch (n->ty->size) {
      case 1:
        return (int8_t)eval(n->lhs);
      case 2:
        return (int16_t)eval(n->lhs);
      case 4:
        return (int32_t)eval(n->lhs);
      }
    }

    return eval(n->lhs);
  case ND_NUM:
    return n->val;
  }

  error_at(n->token->loc, "%s", "not a constant expression"); 
}

/*
  値が決まる箇所の計算
  enum { two = 1+1 };, x[3+3], case 1+2: など
*/
static int64_t const_expr(Token **rest, Token *token) {
  Node *n = conditional(rest, token);
  return eval(n);
}

static Node *to_assign(Node *lhs, Node *rhs, Token *token) {
  return new_node_binary(ND_ASSIGN, lhs, rhs, token);
}

/*
 assign = bitor (assign-op assign)?
 assign-op = "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "|=" | "^=" | "&="
*/
static Node *assign(Token **rest, Token *token) {
  Node *n = conditional(&token, token);
  if (consume(&token, token, "=")) {
    return new_node_binary(ND_ASSIGN, n, assign(rest, token), token);
  }

  /*
    a += 1; -> a = a + 1;
      asign
    |       |
    n     new_add
        |         |
        n        assign
  */
  if (consume(&token, token, "+=")) {
    return to_assign(n, new_add(n, assign(rest, token), token), token);
  }

  if (consume(&token, token, "-=")) {
    return to_assign(n, new_sub(n, assign(rest, token), token), token);
  }

  if (consume(&token, token, "*=")) {
    return to_assign(n, new_node_binary(ND_MUL, n, assign(rest, token), token), token);
  }

  if (consume(&token, token, "/=")) {
    return to_assign(n, new_node_binary(ND_DIV, n, assign(rest, token), token), token);
  }

  if (consume(&token, token, "%=")) {
    return to_assign(n, new_node_binary(ND_MOD, n, assign(rest, token), token), token);
  }

  if (consume(&token, token, "|=")) {
    return to_assign(n, new_node_binary(ND_OR, n, assign(rest, token), token), token);
  }

  if (consume(&token, token, "^=")) {
    return to_assign(n, new_node_binary(ND_XOR, n, assign(rest, token), token), token);
  }

  if (consume(&token, token, "&=")) {
    return to_assign(n, new_node_binary(ND_AND, n, assign(rest, token), token), token);
  }

  if (consume(&token, token, "<<=")) {
    return to_assign(n, new_node_binary(ND_SHL, n, assign(rest, token), token), token);
  }

  if (consume(&token, token, ">>=")) {
    return to_assign(n, new_node_binary(ND_SHR, n, assign(rest, token), token), token);
  }

  *rest = token;
  return n;
}

// conditional = logicalor ("?" expr ":" conditional)?
static Node *conditional(Token **rest, Token *token) {
  Node *n = logicalor(&token, token);

  if (equal(token, "?")) {
    Node *cond = new_node(ND_COND, token);
    cond->cond = n;
    token = token->next;
    cond->then = expr(&token, token);
    expect(&token, token, ":");
    cond->els = conditional(&token, token);

    *rest = token;
    return cond;
  }

  *rest = token;
  return n;
}

// logicalor = logicaland ("||" logicaland)*
static Node *logicalor(Token **rest, Token *token) {
  Node *n = logicaland(&token, token);

  while (equal(token, "||")) {
    Token *start = token;
    n = new_node_binary(ND_LOGICALOR, n, logicaland(&token, token->next), start);
  }

  *rest = token;
  return n;
}

// logicaland = bitor ("&&" bitor)*
static Node *logicaland(Token **rest, Token *token) {
  Node *n = bitor(&token, token);

  while (equal(token, "&&")) {
    Token *start = token;
    n = new_node_binary(ND_LOGICALAND, n, bitor(&token, token->next), start);
  }

  *rest = token;
  return n;
}

// bitor = bitxor ("|" bitxor)*
static Node *bitor(Token **rest, Token *token) {
  Node *n = bitxor(&token, token);

  while(equal(token, "|")) {
    Token *start = token;
    n = new_node_binary(ND_OR, n, bitxor(&token, token->next), start);
  }

  *rest = token;
  return n;
}

// bitxor = bitand ("^" bitand)*
static Node *bitxor(Token **rest, Token *token) {
  Node *n = bitand(&token, token);

  while (equal(token, "^")) {
    Token *start = token;
    n = new_node_binary(ND_XOR, n, bitand(&token, token->next), start);
  }

  *rest = token;
  return n;
}

// bitand = equality ("&" equality)*
static Node *bitand(Token **rest, Token *token) {
  Node *n = equality(&token, token);

  while (equal(token, "&")) {
    Token *start = token;
    n = new_node_binary(ND_AND, n, equality(&token, token->next), start);
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

// relational = shift ("<" shift | "<=" shift | ">" shift | ">=" shift)*
static Node *relational(Token **rest, Token *token) {
  Node *n = shift(&token, token);

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

// shift = add ("<<" add | ">>" add)*
static Node *shift(Token **rest, Token *token) {
  Node *n = add(&token, token);

  for (;;) {
    if (consume(&token, token, "<<")) {
      n = new_node_binary(ND_SHL, n, add(&token, token), token);
    } else if (consume(&token, token, ">>")) {
      n = new_node_binary(ND_SHR, n, add(&token, token), token);
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
    n = new_node_binary(ND_SUB, lhs, new_node_binary(ND_MUL, rhs, new_long(lhs->ty->base->size, token), token), token);
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

// mul = cast ("*" cast | "/" cast | "%" cast)*
static Node *mul(Token **rest, Token *token) {
  Node *n = cast(&token, token);

  for (;;) {
    if (consume(&token, token, "*")) {
      n = new_node_binary(ND_MUL, n, cast(&token, token), token);
    } else if (consume(&token, token, "/")) {
      n = new_node_binary(ND_DIV, n, cast(&token, token), token);
    } else if (consume(&token, token, "%")) {
      n = new_node_binary(ND_MOD, n, cast(&token, token), token);
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
        | ("sizeof" | "+" | "-" | "*" | "&" | "!") cast
        | ("++" | "--") unary
        | postfix
*/
static Node *unary(Token **rest, Token *token) {
  Node *n;

  if (equal(token, "sizeof") && equal(token->next, "(") && is_type(token->next->next)) {
    Token *start = token;
    token = token->next->next;

    Type *ty = typename(&token, token);

    if (ty->kind == TY_ARRAY && ty->size < 0) {
      error_at(token->loc, "%s", "incomplete array type");
    }

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

  if (consume(&token, token, "!")) {
    n = new_node(ND_NOT, token);
    n->lhs = cast(&token, token);

    *rest = token;
    return n;
  }

  if (consume(&token, token, "~")) {
    n = new_node(ND_BITNOT, token);
    n->lhs = cast(&token, token);

    *rest = token;
    return n;
  }

  /*
    ++a -> a = a + 1
    --a -> a = a - 1
  */
  if (consume(&token, token, "++")) {
    n = unary(rest, token);
    return new_node_binary(ND_ASSIGN, n, new_add(n, new_node_num(1, token), token), token);
  }

  if (consume(&token, token, "--")) {
    n = unary(rest, token);
    return new_node_binary(ND_ASSIGN, n, new_sub(n, new_node_num(1, token), token), token);
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

// postfix = primary ("[" expr "]" | "." ident | "->" ident | "++" | "--")*
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

    // a++ -> (typeof a)((a += 1) - 1)
    if (equal(token, "++")) {
      n = to_assign(n, new_add(n, new_node_num(1, token), token), token);
      n = new_sub(n, new_node_num(1, token), token);

      add_type(n);
      n = new_cast(n, n->ty, token);

      token = token->next;
      continue;
    }

    // a-- -> (typeof a)((a -= 1) + 1)
    if (equal(token, "--")) {
      n = to_assign(n, new_sub(n, new_node_num(1, token), token), token);
      n = new_sub(n, new_node_num(-1, token), token);

      add_type(n);
      n = new_cast(n, n->ty, token);

      token = token->next;
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

  Type *ty = vs->var->ty;
  Type *param_ty = ty->params;

  Node head = {};
  Node *cur = &head;
  
  // funtionname (arg, ...);
  token = token->next;
  expect(&token, token, "(");

  while (!equal(token, ")")) {
    if (cur != &head)
      expect(&token, token, ",");

    Node *arg = assign(&token, token);
    add_type(arg);

    if (param_ty) {
      if (param_ty->kind == TY_STRUCT || param_ty->kind == TY_UNION) {
        error_at(token->loc, "%s", "struct or union is not permitted");
      }

      arg = new_cast(arg, param_ty, token);
      param_ty = param_ty->next;
    }

    cur = cur->next = arg;
  }

  expect(&token, token, ")");

  Node *n = new_node(ND_FUNC, token);
  n->func_ty = ty;
  n->ty = ty->return_ty;
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
    /*
      add_type()のND_STMT_EXPRで下記を除外

      + ({stmt+})のstmtの中でreturn
      + empty block
    */
    while (!equal(token, "}")) {
      cur->next = stmt(&token, token);
      cur = cur->next;
    }

    expect(&token, token, "}");

    leave_scope();

    Node *n = new_node(ND_STMT_EXPR, token);
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
    VarScope *vs = find_var(token);
    if (!vs || (!vs->var && !vs->enum_ty)) {
      error_at(token->loc, "%s", "variable not definded");
    }

    Node *n;
    if (vs->var) {
      n = new_node_var(vs->var, token);
    } else {
      n = new_node_num(vs->enum_val, token);
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
