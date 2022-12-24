#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

typedef enum {
  TK_RESERVED,
  TK_IDENT,
  TK_NUM,
  TK_RETURN,
  TK_IF,
  TK_ELSE,
  TK_WHILE,
  TK_FOR,
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
  ND_NUM,
  ND_RETURN,
  ND_IF,
  ND_WHILE,
  ND_FOR
} NodeKind;

static const char *node_kind_enum_map[] = {
  "ND_ADD",
  "ND_SUB",
  "ND_MUL",
  "ND_DIV",
  "ND_ASSIGN",
  "ND_EQ",
  "ND_NEQ",
  "ND_LT",
  "ND_LE",
  "ND_LVAR",
  "ND_NUM",
  "ND_RETURN",
  "ND_IF",
  "ND_WHILE",
  "ND_FOR"
};

typedef struct Node {
  NodeKind kind;
  struct Node *lhs;
  struct Node *rhs;
  //if
  struct Node *cond;
  struct Node *then;
  struct Node *els;
  int val;
  int offset;
} Node;

typedef struct LVar {
  struct LVar *next;
  char *name;
  int len;
  int offset;
} LVar;

LVar *locals;
char *user_input;
Token *token;
Node *code[100];

static void error_at(char *loc, char *fmt, ...);
LVar *new_locals(LVar *l);
LVar *find_lvar(Token *t);
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

void gen_main();
void gen(Node *n);
Token *tokenize();

