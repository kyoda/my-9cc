#include <stdio.h>
#include <stdlib.h>

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

char *user_input;
Token *token;

Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *t = calloc(1, sizeof(Token));
  t->kind = kind;
  t->str = str;
  cur->next = t;
  return t;
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

    if (*p == '+' || *p == '-') {
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

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "no args\n");
    exit(1);
  }

  user_input = argv[1];
  token = tokenize();

  printf(".intel_sytax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  printf("  mov rax %d\n", token->val);
  token = token->next;

  while(token->kind != TK_EOF) {

    if (token->kind == TK_RESERVED && token->str[0] == '+') {
      token = token->next;
      printf("  add rax %d\n", token->val);
      token = token->next;
    }

    if (token->kind == TK_RESERVED && token->str[0] == '-') {
      token = token->next;
      printf("  sub rax %d\n", token->val);
      token = token->next;
    }

  }

  printf("  ret\n");


  return 0;

}
