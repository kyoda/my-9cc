#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

typedef enum {
  TK_IDENT, // Identifiers
  TK_PUNCT, // Punctuators
  TK_KEYWORD, // if, while, etc..
  TK_STR,
  TK_NUM,
  TK_EOF,
} TokenKind;

typedef struct Token {
  TokenKind kind;
  struct Token *next;
  int val;
  char *loc;
  int len;
  // str
  struct Type *ty;
  char *str;
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
  ND_VAR,
  ND_NUM,
  ND_RETURN,
  ND_IF,
  ND_WHILE,
  ND_FOR,
  ND_EXPR_STMT,
  ND_STMT_EXPR,
  ND_BLOCK
} NodeKind;

typedef enum {
  TY_CHAR,
  TY_INT,
  TY_PTR,
  TY_ARRAY
} TypeKind;

typedef struct Type {
  TypeKind *kind;
  int size; //sizeof
  int align; //stacksize
  struct Type *next; // Pointer
  int array_len; // Array Length
  Token *token; // declaration
} Type;

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
  struct Obj *var; // ND_VAR

  Type *ty; // int or pointer
} Node;

// function and variable
typedef struct Obj {
  struct Obj *next;
  char *name;

  // variable
  Type *ty;
  int len;
  int offset;
  bool is_local;

  // function or global variable
  bool is_function;

  // global variable
  char *init_data;

  //function
  struct Obj *params;
  Node *body;
  struct Obj *locals;
  int stack_size;

} Obj;

typedef struct Scope {
  struct Scope *next;
  struct VarScope *vars;
} Scope;

typedef struct VarScope {
  char *name;
  Obj *var;
  struct VarScope *next;
} VarScope;

int equal(Token *t, char *key);
Token *skip(Token *t, char *op);
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void codegen(Obj *prog);
void gen_main();
void gen(Node *n);
Token *tokenize_file(char *path);
Obj *parse(Token *token);
int align_to(int n, int align);
