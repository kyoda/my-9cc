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
  int len;
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

Node *expr();
Node *mul();
Node *unary();
Node *primary();

Node *expr() {
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

    if (strchr("+-*/()", *p)) {
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

void gen(Node *n) {

  //fprintf(stderr, "n->kind: %d\n", n->kind);

  if (n->kind == ND_NUM) {
    printf("  push %d\n", n->val);    
    return;
  }

  gen(n->lhs);
  gen(n->rhs);

  printf("  pop rdi\n");    
  printf("  pop rax\n");    

  switch(n->kind) {
  case ND_ADD:
     printf("  add rax, rdi\n");    
     break;
  case ND_SUB:
     printf("  sub rax, rdi\n");    
     break;
  case ND_MUL:
     printf("  imul rax, rdi\n");    
     break;
  case ND_DIV:
     printf("  cqo\n");    
     printf("  idiv rdi\n");    
     break;
  }

  printf("  push rax\n");    

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

  gen(n);

  printf("  pop rax\n");
  printf("  ret\n");

  return 0;
}

