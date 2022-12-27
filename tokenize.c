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

int equal(Token *t, char *key) {
  return strncmp(t->loc, key, t->len) == 0 && key[t->len] == '\0';
}

int keyword_len(char *p) {
  char *key[] = {"return", "if", "else", "for", "while"};
  int key_len;
  for (int i = 0; i < sizeof(key) / sizeof(*key); i++) {
    key_len = strlen(key[i]);
    if (strncmp(key[i], p, key_len) == 0 && ! is_ident2(p[key_len])) {
      return key_len;
    }
  }
  return 0;
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

    //KEYWORDS
    int kl = keyword_len(p);
    if (kl) {
      cur = new_token(TK_KEYWORD, cur, p, kl);
      p += kl;
      continue;
    }

    //PUNCTUATORS
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

    if (strchr("+-*/()<>=;{}", *p)) {
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

