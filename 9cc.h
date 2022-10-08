#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

typedef enum {
  TK_RESERVED,
  TK_IDENT,
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
  ND_ASSIGN,
  ND_EQ,
  ND_NEQ,
  ND_LT,
  ND_LE,
  ND_LVAR,
  ND_NUM
} NodeKind;

typedef struct Node {
  NodeKind kind;
  struct Node *lhs;
  struct Node *rhs;
  int val;
  int offset;
} Node;

char *user_input;
Token *token;
Node *code[100];

void program();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

void gen(Node *n);
Token *tokenize();

