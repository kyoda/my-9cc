#include "9cc.h"

LVar *locals;

LVar *find_lvar(Token *t);
Node *stmt(Token **rest, Token *token);
Node *expr(Token **rest, Token *token);
Node *assign(Token **rest, Token *token);
Node *equality(Token **rest, Token *token);
Node *relational(Token **rest, Token *token);
Node *add(Token **rest, Token *token);
Node *mul(Token **rest, Token *token);
Node *unary(Token **rest, Token *token);
Node *primary(Token **rest, Token *token);

Node *new_node(NodeKind kind) {
  Node *n = calloc(1, sizeof(Node));
  n->kind = kind;
  return n;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
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

LVar *new_lvar(char *name, Type *ty) {
  LVar *lvar;
  lvar = calloc(1, sizeof(LVar));
  lvar->name = name;
  lvar->len = strlen(name);
  lvar->ty = ty;
  lvar->next = locals;
  locals = lvar;

  return lvar;
}

LVar *find_lvar(Token *t) {
  for (LVar *var = locals; var; var = var->next) {
    if (var->len == t->len && strncmp(t->loc, var->name, var->len) == 0) {
      return var;
    }
  }
  return NULL;
}

char *get_ident_name(Token *t) {
  if (t->kind != TK_IDENT) {
    fprintf(stderr, "expected an identifier\n");
    exit(1);
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

bool at_eof(Token *token) {
  return token->kind == TK_EOF;
}

Type *ty_int() {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_INT;
  ty->size = 4;
  return ty;
}

Type *pointer_to(Type *base) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_PTR;
  ty->size = 8;
  ty->next = base;

  return ty;
}

void add_type(Node *n) {
  if (!n || n->ty) {
    return;
  }

  add_type(n->lhs);
  add_type(n->rhs);
  add_type(n->init);
  add_type(n->cond);
  add_type(n->inc);
  add_type(n->then);
  add_type(n->els);

  for (Node *node = n->body; node; node = node->next) {
    add_type(node);
  }

  switch(n->kind) {
  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
  case ND_NEG:
  case ND_ASSIGN:
    n->ty = n->lhs->ty;
    return;
  case ND_EQ:
  case ND_NEQ:
  case ND_LT:
  case ND_LE:
  case ND_FUNC:
  case ND_NUM:
    n->ty = ty_int();
    return;
  case ND_LVAR:
    n->ty = n->var->ty;
    return;
  case ND_ADDR:
    n->ty = pointer_to(n->lhs->ty);
    return;
  case ND_DEREF:
    if (n->lhs->ty->kind != TY_PTR) {
      fprintf(stderr, "invalid deref\n");
      exit(1);
    }

    n->ty = n->lhs->ty->next;
    return;
  }

}

// stmt = expr? ";" |
//        "{" stmt* "}" |
//        "if" "(" expr ")" stmt ("else" stmt)? |
//        "while" "(" expr ")" stmt |
//        "for" "(" expr? ";" expr? ";" expr? ";"  ")" stmt |
//        "return" expr ";" |
//         declspec "*"* ident ("=" assign)? ";"
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

  if (equal(token, "int")) {
    token = token->next;

    Type *ty = ty_int();
    while (consume(&token, token, "*")) {
      ty = pointer_to(ty);
    }

    LVar *lvar = find_lvar(token);
    if (lvar) {
      fprintf(stderr, "definded variable\n");
      exit(1);
    }

    lvar = new_lvar(get_ident_name(token), ty);
    token = token->next;

    n = calloc(1, sizeof(Node));
    n->kind = ND_LVAR;
    n->var = lvar;

    if (consume(&token, token, "=")) {
      n = new_binary(ND_ASSIGN, n, assign(&token, token));
    }

    expect(&token, token, ";");

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

  n = expr(&token, token);
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
    n = new_binary(ND_ASSIGN, n, assign(&token, token));
  }

  *rest = token;
  return n;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality(Token **rest, Token *token) {
  Node *n = relational(&token, token);

  for (;;) {
    if (consume(&token, token, "==")) {
      n = new_binary(ND_EQ, n, relational(&token, token));
    } else if (consume(&token, token, "!=")) {
      n = new_binary(ND_NEQ, n, relational(&token, token));
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
      n = new_binary(ND_LT, n, add(&token, token));
    } else if (consume(&token, token, ">")) {
      n = new_binary(ND_LT, add(&token, token), n);
    } else if (consume(&token, token, "<=")) {
      n = new_binary(ND_LE, n, add(&token, token));
    } else if (consume(&token, token, ">=")) {
      n = new_binary(ND_LE, add(&token, token), n);
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
  if (lhs->ty->kind == TY_INT && rhs->ty->kind == TY_INT) {
    n = new_binary(ND_ADD, lhs, rhs);
    return n;
  }

  // pointer + pointer
  if (lhs->ty->kind == TY_PTR && rhs->ty->kind == TY_PTR) {
    fprintf(stderr, "invalid operand\n");
    exit(1);
  }

  // num + pointer to pointer + num
  if (lhs->ty->kind == TY_INT && rhs->ty->kind == TY_PTR) {
    Node *tmp = lhs;
    lhs = rhs;
    rhs = tmp;
  }
  
  // pointer + num * 4
  // int -> 4byte
  n = new_binary(ND_ADD, lhs, new_binary(ND_MUL, rhs, new_node_num(4)));

  return n;
}

Node *new_sub(Node *lhs, Node *rhs, Token *token) {
  add_type(lhs);
  add_type(rhs);

  Node *n;

  // num - num
  if (lhs->ty->kind == TY_INT && rhs->ty->kind == TY_INT) {
    n = new_binary(ND_SUB, lhs, rhs);
    return n;
  }

  // pointer - num * 4
  // int -> 4byte
  if (lhs->ty->kind == TY_PTR && rhs->ty->kind == TY_INT) {
    n = new_binary(ND_SUB, lhs, new_binary(ND_MUL, rhs, new_node_num(4)));
    return n;
  }

  // pointer - pointer, return int elements between pointer and pointer
  if (lhs->ty->kind == TY_PTR && rhs->ty->kind == TY_PTR) {
    n = new_binary(ND_DIV, new_binary(ND_SUB, lhs, rhs), new_node_num(4));
    return n;
  }

  fprintf(stderr, "invalid operand\n");
  exit(1);
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
      n = new_binary(ND_MUL, n, unary(&token, token));
    } else if (consume(&token, token, "/")) {
      n = new_binary(ND_DIV, n, unary(&token, token));
    } else {
      *rest = token;
      return n;
    }
  }

  *rest = token;
  return n;
}

// unary = "sizeof" unary |
//         ("+" | "-")? primary |
//         "*" unary |
//         "&" unary
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
    n = new_binary(ND_NEG, unary(&token, token), NULL);
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

  n = primary(&token, token);
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

// primary = num | 
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
  } else if (token->kind == TK_IDENT) {

    //funcall
    if (equal(token->next, "("))
      return funcall(rest, token);

    // lvar
    Node *n = calloc(1, sizeof(Node));
    n->kind = ND_LVAR;

    LVar *lvar = find_lvar(token);
    if (lvar) {
      n->var = lvar;
    } else {
      fprintf(stderr, "variable not definded\n");
      exit(1);
    }

    token = token->next;

    *rest = token;
    return n;
  } else {
    fprintf(stderr, "no num, no ident, no func \n");
    exit(1);
  }

}

void create_params(Token **rest, Token *token) {
  while (!equal(token, ")")) {
    if (locals != NULL)
      expect(&token, token, ",");

    Type *ty = calloc(1, sizeof(Type));
    if (equal(token, "int")) {
      token = token->next;
      ty->kind = TY_INT;
    } else {
      fprintf(stderr, "param type not definded\n");
      exit(1);
    }

    while(consume(&token, token, "*")) {
      ty = pointer_to(ty);
    }

    locals = new_lvar(strndup(token->loc, token->len), ty);
    token = token->next;
  }

  *rest = token;
}

// program = (declspec) stmt*
Function *function (Token **rest, Token *token) {
  locals = NULL;

  Function *fn = calloc(1, sizeof(Function));


  if (equal(token, "int")) {
    token = token->next;
  } else {
    fprintf(stderr, "func not definded\n");
    exit(1);
  }

  fn->name = strndup(token->loc, token->len);
  token = token->next;

  expect(&token, token, "(");
  create_params(&token, token);
  expect(&token, token, ")");

  fn->params = locals;

  fn->body = stmt(&token, token);
  fn->locals = locals;

  *rest = token;
  return fn;
}

Function *parse(Token *token) {
  Function head = {};
  Function *cur = &head;
  while (token->kind != TK_EOF)
    cur = cur->next = function(&token, token);

  return head.next;
}

