#include "9cc.h"

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *n = calloc(1, sizeof(Node));
  n->kind = kind;
  n->lhs = lhs;
  n->rhs = rhs;
  return n;
}

Node *new_node_num(int val) {
  Node *n = calloc(1, sizeof(Node));
  n->kind = ND_NUM;
  n->val = val;
  return n;
}

Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *t = calloc(1, sizeof(Token));
  t->kind = kind;
  t->str = str;
  t->len = len;
  cur->next = t;
  return t;
}

LVar *find_lvar(Token *t) {
  for (LVar *var = locals; var; var = var->next) {
    if (var->len == t->len && !strncmp(t->str, var->name, var->len)) {
      return var;
    }
  }
  return NULL;
}

bool consume(char *op) {
  if (token->kind == TK_RESERVED && 
      token->len == strlen(op) &&
      ! strncmp(token->str, op, token->len)) {
    token = token->next;
    return true;
  }

  return false;
}

bool consume_num() {
  if (token->kind == TK_NUM) {
    return true;
  }

  return false;
}

bool expect(char *op) {
  if (token->kind == TK_RESERVED && 
      token->len == strlen(op) &&
      ! strncmp(token->str, op, token->len)) {
    token = token->next;
  } else {
    fprintf(stderr, "no %s\n", op);
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
  Node *n = expr();
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
      lvar = calloc(1, sizeof(LVar));
      lvar->next = locals;
      lvar->name = token->str;
      lvar->len = token->len;
      lvar->offset = locals->offset + 8;
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

Token *tokenize() {
  char *p = user_input;
  Token head;
  head.next = NULL;
  Token *cur = &head;

  char *start;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (
        !strncmp("==", p, 2) || 
        !strncmp("!=", p, 2) || 
        !strncmp("<=", p, 2) ||
        !strncmp(">=", p, 2)
    ) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    if (strchr("+-*/()<>=;", *p)) {
      cur = new_token(TK_RESERVED, cur, p, 1);
      p++;
      continue;
    }
    
    if (
      ('a' <= *p && *p <= 'z') || 
      ('A' <= *p && *p <= 'Z') || 
      '_' == *p
    ) {
      start = p;
      p++;

      while (
        ('a' <= *p && *p <= 'z') || 
        ('A' <= *p && *p <= 'Z') || 
        '_' == *p ||
        ('0' <= *p && *p <= '9')
      ) {
        p++;
      }

      cur = new_token(TK_IDENT, cur, start, p - start);
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    fprintf(stderr, "can't tokenize\n");
    exit(1);
  }

  new_token(TK_EOF, cur, NULL, 0);
  return head.next;

}

