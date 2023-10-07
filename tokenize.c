#include "9cc.h"
char *user_input;
char *infile;

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error_at(char *loc, char *fmt, ...) {
  char *start = loc;
  while (user_input < start && start[-1] != '\n') {
    start--;
  }

  char *end = loc;
  while (*end != '\n') {
    end++;
  }

  int line_num = 1;
  for(char *p = user_input; p < start; p++) {
    if (*p == '\n') {
      line_num++;
    }
  }

  va_list ap;
  va_start(ap, fmt);

  int indent = fprintf(stderr, "%s:%d: ", infile, line_num);
  fprintf(stderr, "%.*s\n", end - start, start);

  int pos = loc - start + indent;
  fprintf(stderr, "%*s", pos, " ");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

static Token *new_token(TokenKind kind, Token *cur, char *loc , int len) {
  Token *t = calloc(1, sizeof(Token));
  t->kind = kind;
  t->loc = loc;
  t->len = len;
  cur->next = t;
  return t;
}

static int is_ident1(char p) {
  return (
    ('a' <= p && p <= 'z') || 
    ('A' <= p && p <= 'Z') || 
    '_' == p
  );
}

static int is_ident2(char p) {
  return (is_ident1(p) || ('0' <= p && p <= '9'));
}

int equal(Token *t, char *key) {
  return strncmp(t->loc, key, t->len) == 0 && key[t->len] == '\0';
}

Token *skip(Token *t, char *op) {
  if (!equal(t, op))
    error_at(t->loc, "expected %s", op);
  return t->next;
}

static int keyword_len(char *p) {
  char *key[] = {"return", "if", "else", "for", "while", 
                "void", "char", "short", "int", "long", "sizeof",
                "struct", "union", "typedef"};
  int key_len;
  for (int i = 0; i < sizeof(key) / sizeof(*key); i++) {
    key_len = strlen(key[i]);
    if (strncmp(key[i], p, key_len) == 0 && ! is_ident2(p[key_len])) {
      return key_len;
    }
  }
  return 0;
}

static char *string_literal_end(char *p) {
  while (*p != '"') {
    if (*p == '\n' || *p == '\0') {
      error_at(p, "unclosed string literal");
    }

    // skip escaped double qoute
    if (*p == '\\') {
      p++;
    }

    p++;
  }

  return p;
}

static int from_hex(char *p) {
  if ('0' <= *p && *p <= '9') {
    return *p - '0';
  } 

  if ('a' <= *p && *p <= 'f') {
    return *p - 'a' + 10;
  }

  if ('A' <= *p && *p <= 'F') {
    return *p - 'A' + 10;
  }

  error_at(p, "invalid hex escape sequence");
}

static int read_escaped_char(char **new_pos, char *p) {
  // hex escape sequence
  if (*p == 'x') {
    p++;
    if (!isxdigit(*p)) {
      error_at(p, "invalid hex escape sequence");
    }

    int c = 0;
    for (; isxdigit(*p); p++) {
      c = c * 16 + from_hex(p);
    }

    *new_pos = p;
    return c;
  }

  // octal escape sequence
  if ('0' <= *p && *p <= '7') {
    int c = *p++ - '0';
    if ('0' <= *p && *p <= '7') {
      c = c*8 + (*p++ - '0');
      if ('0' <= *p && *p <= '7') {
        c = c*8 + (*p++ - '0');
      }
    }

    *new_pos = p;
    return c;
  }

  switch (*p) {
    case 'a':
      return '\a';
    case 'b':
      return '\b';
    case 't':
      return '\t';
    case 'n':
      return '\n';
    case 'v':
      return '\v';
    case 'f':
      return '\f';
    case 'r':
      return '\r';
    case '"':
      return '\"';
    default:
      return *p;
  }
}

static Token *read_string_literal(char *start) {
  char *end = string_literal_end(start + 1);
  char *buf = calloc(1, end - start);

  int len = 0;
  for (char *p = start + 1; p < end; p++) {
    if (*p == '\\') {
      buf[len++] = read_escaped_char(&p, ++p);
    } else {
      buf[len++] = *p;
    }
  }

  Token *t = calloc(1, sizeof(Token));
  t->kind = TK_STR;
  t->loc = start;
  t->len = end + 1 - start;
  t->ty = ty_array(ty_char(), len + 1);
  t->str = buf;

  return t;
}

static void add_lines(Token *token) {
  int line_num = 1;
  char *p = user_input;

  while (*p) {
    if (p == token->loc) {
      token->line = line_num;
      token = token->next;
    }

    if (*p == '\n') {
      line_num++;
    }

    p++;
  }
}

Token *tokenize(char* p, char *file) {
  user_input = p;
  infile = file;

  Token head;
  head.next = NULL;
  Token *cur = &head;

  char *start;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    //skip line comments
    if (strncmp(p, "//", 2) == 0) {
      p += 2;
      while (*p != '\n') {
        p++;
      }
      continue;
    }

    //skip block comments
    if (strncmp(p, "/*", 2) == 0) {
      char *q = strstr(p + 2, "*/");
      if (!q) {
        error_at(p, "unclosed block comment");
      }
      p = q + 2;
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
        strncmp(">=", p, 2) == 0 ||
        strncmp("->", p, 2) == 0
    ) {
      cur = new_token(TK_PUNCT, cur, p, 2);
      p += 2;
      continue;
    }

    if (strchr("+-*/()<>=;{},&[].", *p)) {
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

    //string literal
    if (*p == '"') {
      cur = cur->next = read_string_literal(p);
      p += cur->len;
      continue;
    }

    error_at(p, "%s", "can't tokenize");
  }

  new_token(TK_EOF, cur, NULL, 0);
  add_lines(head.next);
  return head.next;

}
