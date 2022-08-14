#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

typedef enum {
  TK_RESERVED,
  TK_NUM,
  TK_EOF,
} TokenKind;

typedef struct Token {
  TokenKind kind;
  struct Token *next;
  int val;
  char *str;
} Token;

typedef enum {
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
  ND_NUM,
} NodeKind;

typedef struct Node {
  NodeKind kind;
  struct Node *lhs;
  struct Node *rhs;
  int val;
} Node;

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

char *user_input;
Token *token;

Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *t = calloc(1, sizeof(Token));
  t->kind = kind;
  t->str = str;
  cur->next = t;
  return t;
}

bool consume(char op) {
  if (token->kind != TK_RESERVED) {
    fprintf(stderr, "no op\n");
    exit(1);
  }

  if (token->str[0] == op) {
    token = token->next;
    return true;
  }

  return false;
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

Node *expr();
Node *mul();
Node *primary();

Node *expr() {
  Node *n = mul();

  for (;;) {
    if (consume('+')) {
      n = new_node(ND_ADD, n, mul());
    } else if (consume('-')) {
      n = new_node(ND_SUB, n, mul());
    } else {
      return n;
    }
  }

  return n;
}

Node *mul() {
  Node *n = primary();

  for (;;) {
    if (consume('*')) {
      n = new_node(ND_MUL, n, primary());
    } else if (consume('/')) {
      n = new_node(ND_DIV, n, primary());
    } else {
      return n;
    }
  }

  return n;
}

Node *primary() {
  Node *n;

  if (expect_num()) {
    n = new_node_num(expect_num());
  } else if (expect_op('(')) {
    n = expr();
  } else {
    fprintf(stderr, "fail tokenize\n");
  }

  return n;
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

    if (strchr("+-*/()", *p)) {
      cur = new_token(TK_RESERVED, cur, p);
      p++;
      continue;
    }
    
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    fprintf(stderr, "can't tokenize\n");
    exit(1);
  }

  new_token(TK_EOF, cur, NULL);
  return head.next;

}

bool at_eof() {
  return token->kind == TK_EOF;
}

void gen() {

  printf("  mov rax, %d\n", expect_num());

  while(!at_eof()) {
    if (consume('+')) {
      printf("  add rax, %d\n", expect_num());
    } else if (consume('-')) {
      printf("  sub rax, %d\n", expect_num());
    }
  }

}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "no args\n");
    exit(1);
  }

  user_input = argv[1];
  token = tokenize();
  Node *n = expr();

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  gen();

  //printf("  pop rax\n");
  printf("  ret\n");

  return 0;
}
