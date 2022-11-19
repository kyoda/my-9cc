#include "9cc.h"

Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *t = calloc(1, sizeof(Token));
  t->kind = kind;
  t->str = str;
  t->len = len;
  cur->next = t;
  return t;
}

int punct1(char p) {
  if (
    ('a' <= p && p <= 'z') || 
    ('A' <= p && p <= 'Z') || 
    '_' == p
  ) {
    return true;
  }
  
  return false;
}

int punct2(char p) {
  if (punct1(p) || ('0' <= p && p <= '9')) {
    return true;
  }
  
  return false;
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

    if (strchr("+-*/()<>=;", *p)) {
      cur = new_token(TK_RESERVED, cur, p, 1);
      p++;
      continue;
    }
    
    if (punct1(*p)) {
      start = p;
      p++;

      while (punct2(*p)) {
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

