#include "9cc.h"

LVar *locals;

LVar *new_locals(LVar *l);
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

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *n = calloc(1, sizeof(Node));
  n->kind = kind;
  n->lhs = lhs;
  n->rhs = rhs;
  //fprintf(stderr, "node kind: %s\n", node_kind_enum_map[kind]);
  return n;
}

Node *new_node_num(int val) {
  Node *n = calloc(1, sizeof(Node));
  n->kind = ND_NUM;
  n->val = val;
  //fprintf(stderr, "node kind: %s\n", node_kind_enum_map[ND_NUM]);
  return n;
}

LVar *new_locals(LVar *l) {
  LVar *lvar;
  lvar = calloc(1, sizeof(LVar));
  if (l) {
    lvar->next = l;
    lvar->offset = l->offset + 8; //8Bytes
  } else {
    lvar->next = NULL;
    lvar->name = NULL;
    lvar->len = 0;
    lvar->offset = 0;
  }

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

Node *stmt(Token **rest, Token *token) {
  Node *n;

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

Node *expr(Token **rest, Token *token) {
  Node *n = assign(&token, token);
  *rest = token;
  return n;
}

Node *assign(Token **rest, Token *token) {
  Node *n = equality(&token, token);
  if (consume(&token, token, "=")) {
    n = new_node(ND_ASSIGN, n, assign(&token, token));
  }

  *rest = token;
  return n;
}

Node *equality(Token **rest, Token *token) {
  Node *n = relational(&token, token);

  for (;;) {
    if (consume(&token, token, "==")) {
      n = new_node(ND_EQ, n, relational(&token, token));
    } else if (consume(&token, token, "!=")) {
      n = new_node(ND_NEQ, n, relational(&token, token));
    } else {
      *rest = token;
      return n;
    }
  }

  *rest = token;
  return n;
}

Node *relational(Token **rest, Token *token) {
  Node *n = add(&token, token);

  for (;;) {
    if (consume(&token, token, "<")) {
      n = new_node(ND_LT, n, add(&token, token));
    } else if (consume(&token, token, ">")) {
      n = new_node(ND_LT, add(&token, token), n);
    } else if (consume(&token, token, "<=")) {
      n = new_node(ND_LE, n, add(&token, token));
    } else if (consume(&token, token, ">=")) {
      n = new_node(ND_LE, add(&token, token), n);
    } else {
      *rest = token;
      return n;
    }
  }

  *rest = token;
  return n;
}

Node *add(Token **rest, Token *token) {
  Node *n = mul(&token, token);

  for (;;) {
    if (consume(&token, token, "+")) {
      n = new_node(ND_ADD, n, mul(&token, token));
    } else if (consume(&token, token, "-")) {
      n = new_node(ND_SUB, n, mul(&token, token));
    } else {
      *rest = token;
      return n;
    }
  }

  *rest = token;
  return n;
}

Node *mul(Token **rest, Token *token) {
  Node *n = unary(&token, token);

  for (;;) {
    if (consume(&token, token, "*")) {
      n = new_node(ND_MUL, n, unary(&token, token));
    } else if (consume(&token, token, "/")) {
      n = new_node(ND_DIV, n, unary(&token, token));
    } else {
      *rest = token;
      return n;
    }
  }

  *rest = token;
  return n;
}

Node *unary(Token **rest, Token *token) {
  Node *n;
  if (consume(&token, token, "+")) {
    n = primary(&token, token);
    *rest = token;
    return n;
  }

  if (consume(&token, token, "-")) {
    n = new_node(ND_SUB, new_node_num(0), primary(&token, token));
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
      n->offset = lvar->offset;
    } else {
      lvar = new_locals(locals);
      lvar->name = token->loc;
      lvar->len = token->len;
      n->offset = lvar->offset;
      locals = lvar;
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
    if (locals == NULL) {
      locals = new_locals(locals);
      locals->name = strndup(token->loc, token->len);
      locals->len = token->len;
      locals->offset = 8;
    } else {
      expect(&token, token, ",");
      locals = new_locals(locals);
      locals->name = strndup(token->loc, token->len);
      locals->len = token->len;
    }
    token = token->next;
  }

  *rest = token;
}

Function *function (Token **rest, Token *token) {
  locals = NULL;

  Function *fn = calloc(1, sizeof(Function));
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

