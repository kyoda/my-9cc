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

bool consume(char *op) {
  if (token->kind == TK_RESERVED && 
      token->len == strlen(op) &&
      ! strncmp(token->str, op, token->len)) {
    token = token->next;
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
    fprintf(stderr, "no )\n");
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

Node *expr() {
  Node *n = equality();
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

  return new_node_num(expect_num());
}

Token *tokenize() {
  char *p = user_input;
  Token head;
  head.next = NULL;
  Token *cur = &head;

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

    if (strchr("+-*/()<>", *p)) {
      cur = new_token(TK_RESERVED, cur, p, 1);
      p++;
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

bool at_eof() {
  return token->kind == TK_EOF;
}
