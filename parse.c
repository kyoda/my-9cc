#include "9cc.h"

static void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s\n", pos, " ");
  fprintf(stderr, "^ ");
  fprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

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
    lvar->offset = l->offset + 8;
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
    if (var->len == t->len && strncmp(t->str, var->name, var->len) == 0) {
      return var;
    }
  }
  return NULL;
}

bool consume(char *op) {
  if (token->kind == TK_RESERVED && 
      token->len == strlen(op) &&
      strncmp(token->str, op, token->len) == 0) {
    token = token->next;
    return true;
  }

  return false;
}

bool expect(char *op) {
  if (token->kind == TK_RESERVED && 
      token->len == strlen(op) &&
      strncmp(token->str, op, token->len) == 0) {
    token = token->next;
  } else {
    error_at(token->str, "no op");
    exit(1);
  }
}

int expect_num() {
  if (token->kind != TK_NUM) {
    fprintf(stderr, "no num\n");
    exit(1);
  }

  int v = token->val;
  token = token->next;

  return v;
}

bool expect_op(char op) {
  if (strchr("+-*/()", op)) {
    token = token->next;
    return true;
  }

  return false;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

void program() {
  int i = 0;
  while (!at_eof()) {
    code[i] = stmt();
    i++;
  }
  code[i] = NULL;
}

Node *stmt() {
  Node *n;

  switch (token->kind) {
  case TK_RETURN:
    n = calloc(1, sizeof(Node));
    n->kind = ND_RETURN;
    token = token->next;
    n->lhs = expr();
    break;
  case TK_IF:
    n = calloc(1, sizeof(Node));
    n->kind = ND_IF;
    token = token->next;
    expect("(");
    n->cond = expr();
    expect(")");
    n->then = stmt();

    if (token->kind == TK_ELSE) {
      token = token->next;
      n->els = stmt();
    }

    return n;
  case TK_FOR:
    n = expr();
    break;
  case TK_WHILE:
    n = expr();
    break;
  default:
    n = expr();
    break;
  }

  expect(";");
  return n;
}

Node *expr() {
  return assign();
}

Node *assign() {
  Node *n = equality();
  if (consume("=")) {
    n = new_node(ND_ASSIGN, n, assign());
  }

  return n;
}

Node *equality() {
  Node *n = relational();

  for (;;) {
    if (consume("==")) {
      n = new_node(ND_EQ, n, relational());
    } else if (consume("!=")) {
      n = new_node(ND_NEQ, n, relational());
    } else {
      return n;
    }
  }

  return n;
}

Node *relational() {
  Node *n = add();

  for (;;) {
    if (consume("<")) {
      n = new_node(ND_LT, n, add());
    } else if (consume(">")) {
      n = new_node(ND_LT, add(), n);
    } else if (consume("<=")) {
      n = new_node(ND_LE, n, add());
    } else if (consume(">=")) {
      n = new_node(ND_LE, add(), n);
    } else {
      return n;
    }
  }

  return n;
}

Node *add() {
  Node *n = mul();

  for (;;) {
    if (consume("+")) {
      n = new_node(ND_ADD, n, mul());
    } else if (consume("-")) {
      n = new_node(ND_SUB, n, mul());
    } else {
      return n;
    }
  }

  return n;
}

Node *mul() {
  Node *n = unary();

  for (;;) {
    if (consume("*")) {
      n = new_node(ND_MUL, n, unary());
    } else if (consume("/")) {
      n = new_node(ND_DIV, n, unary());
    } else {
      return n;
    }
  }

  return n;
}

Node *unary() {
  if (consume("+")) {
    return primary();
  }

  if (consume("-")) {
    return new_node(ND_SUB, new_node_num(0), primary());
  }

  return primary();
}

Node *primary() {

  if (consume("(")) {
    Node *n = expr();
    expect(")");
    return n;
  }

  if (token->kind == TK_NUM) {
    int v = token->val;
    token = token->next;

    return new_node_num(v);
  } else if (token->kind == TK_IDENT) {
    Node *n = calloc(1, sizeof(Node));
    n->kind = ND_LVAR;

    LVar *lvar = find_lvar(token);
    if (lvar) {
      n->offset = lvar->offset;
    } else {
      lvar = new_locals(locals);
      lvar->name = token->str;
      lvar->len = token->len;
      n->offset = lvar->offset;
      locals = lvar;
    }

    token = token->next;

    return n;
  } else {
    fprintf(stderr, "no num or no ident \n");
    exit(1);
  }

}

