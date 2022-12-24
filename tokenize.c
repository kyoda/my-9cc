#include "9cc.h"

Token *new_token(TokenKind kind, Token *cur, char *loc , int len) {
  Token *t = calloc(1, sizeof(Token));
  t->kind = kind;
  t->loc = loc;
  t->len = len;
  cur->next = t;
  return t;
}

int is_ident1(char p) {
  return (
    ('a' <= p && p <= 'z') || 
    ('A' <= p && p <= 'Z') || 
    '_' == p
  );
}

int is_ident2(char p) {
  return (is_ident1(p) || ('0' <= p && p <= '9'));
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

    if (strncmp("return", p, 6) == 0 && ! is_ident2(p[6])) {
      cur = new_token(TK_RETURN, cur, p, 6);
      p += 6;
      continue;
    }

    if (strncmp("if", p, 2) == 0 && ! is_ident2(p[2])) {
      cur = new_token(TK_IF, cur, p, 2);
      p += 2;
      continue;
    }

    if (strncmp("else", p, 4) == 0 && ! is_ident2(p[4])) {
      cur = new_token(TK_ELSE, cur, p, 4);
      p += 4;
      continue;
    }

    if (strncmp("for", p, 3) == 0 && ! is_ident2(p[3])) {
      cur = new_token(TK_RETURN, cur, p, 3);
      p += 3;
      continue;
    }

    if (strncmp("while", p, 5) == 0 && ! is_ident2(p[5])) {
      cur = new_token(TK_RETURN, cur, p, 5);
      p += 5;
      continue;
    }

    if (
        strncmp("==", p, 2) == 0 || 
        strncmp("!=", p, 2) == 0 || 
        strncmp("<=", p, 2) == 0 ||
        strncmp(">=", p, 2) == 0
    ) {
      cur = new_token(TK_PUNCT, cur, p, 2);
      p += 2;
      continue;
    }

    if (strchr("+-*/()<>=;", *p)) {
      cur = new_token(TK_PUNCT, cur, p, 1);
      p++;
      continue;
    }
    
    if (is_ident1(*p)) {
      start = p;
      p++;

      while (is_ident2(*p)) {
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

