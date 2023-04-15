#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

typedef enum {
  TK_IDENT, // Identifiers
  TK_PUNCT, // Punctuators
  TK_KEYWORD, // if, while, etc..
  TK_NUM,
  TK_EOF,
} TokenKind;

typedef struct Token {
  TokenKind kind;
  struct Token *next;
  int val;
  char *loc;
  int len;
} Token;

typedef enum {
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
  ND_NEG,
  ND_ASSIGN,
  ND_ADDR,
  ND_DEREF,
  ND_EQ,
  ND_NEQ,
  ND_LT,
  ND_LE,
  ND_FUNC,
  ND_LVAR,
  ND_NUM,
  ND_RETURN,
  ND_IF,
  ND_WHILE,
  ND_FOR,
  ND_BLOCK
} NodeKind;

typedef enum {
  TY_INT,
  TY_PTR
} TypeKind;

typedef struct Type {
  TypeKind *kind;
  struct Type *next; // Pointer
} Type;

// Local Variable
typedef struct LVar {
  struct LVar *next;
  char *name;
  Type *ty;
  int len;
  int offset;
} LVar;

typedef struct Node {
  NodeKind kind;
  struct Node *lhs;
  struct Node *rhs;

  //if or for or while
  struct Node *cond;
  struct Node *init;
  struct Node *inc;
  struct Node *then;
  struct Node *els;

  //block
  struct Node *body; 
  struct Node *next;

  //function
  char *funcname;
  struct Node *args;

  int val;
  LVar *var; // ND_LVAR

  Type *ty; // int or pointer
} Node;


// Function
typedef struct Function {
  struct Function *next;
  char *name;
  LVar *params;
  Node *body;
  LVar *locals;
  int stack_size;
} Function;

Token *token;
Node *code[100];

int equal(Token *t, char *key);
Token *skip(Token *t, char *op);
void print_token(Token *t);
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void codegen(Function *prog);
void gen_main();
void gen(Node *n);
Token *tokenize();
Function *parse(Token *token);

