#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

typedef struct Type Type;
typedef struct Token Token;
typedef struct Node Node;
typedef struct Obj Obj;
typedef struct Member Member;

typedef enum {
  TK_IDENT, // Identifiers
  TK_PUNCT, // Punctuators
  TK_KEYWORD, // if, while, etc..
  TK_STR,
  TK_NUM,
  TK_EOF,
} TokenKind;

struct Token {
  TokenKind kind;
  Token *next;
  int val;
  char *loc;
  int len;
  int line; //for .loc directive
  // str
  Type *ty;
  char *str;
};

typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NEG, // unary -
  ND_ASSIGN, // =
  ND_COMMA, // ,
  ND_ADDR, // &
  ND_DEREF, // unary *
  ND_EQ, // ==
  ND_NEQ, // !=
  ND_LT, // <
  ND_LE, // <=
  ND_FUNC, // function
  ND_VAR, // variable
  ND_NUM, // integer 
  ND_RETURN, // return
  ND_IF, // if
  ND_WHILE, // while
  ND_FOR, // for
  ND_EXPR_STMT,
  ND_STMT_EXPR,
  ND_BLOCK, // { ... }
  ND_MEMBER // struct member
} NodeKind;

struct Node {
  NodeKind kind;
  Node *lhs;
  Node *rhs;

  //if or for or while
  Node *cond;
  Node *init;
  Node *inc;
  Node *then;
  Node *els;

  //block
  Node *body; 
  Node *next;

  //function
  char *funcname;
  Node *args;

  int val;
  Obj *var; // ND_VAR

  Token *token; // for error message
  Type *ty; // int or pointer

  Member *member; // struct member
};

typedef enum {
  TY_CHAR,
  TY_INT,
  TY_PTR,
  TY_ARRAY,
  TY_FUNC,
  TY_STRUCT
} TypeKind;

struct Type {
  TypeKind *kind;
  int size; //sizeof
  int align; //stacksize
  Type *base; // pointer, array
  int array_len; // array length

  // function
  Type *params;
  Type *next; //params

  Token *token; // declaration
  Member *members; // struct member
};

// function and variable
struct Obj {
  Obj *next;
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
  Obj *params;
  Node *body;
  Obj *locals;
  int stack_size;

};

struct Member {
  Member *next;
  char *name;
  int offset;
  Type *ty;
  Token *token;
};

typedef struct Scope Scope;
typedef struct VarScope VarScope;

struct Scope {
  Scope *next;
  VarScope *vars;
};

struct VarScope {
  char *name;
  Obj *var;
  VarScope *next;
};
