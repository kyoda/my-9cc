#include "9cc.h"
char *user_input;
char *filename;

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

  int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
  fprintf(stderr, "%.*s\n", end - start, start);

  int pos = loc - start + indent;
  fprintf(stderr, "%*s", pos, " ");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

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

Token *skip(Token *t, char *op) {
  if (!equal(t, op))
    error_at(t->loc, "expected %s", op);
  return t->next;
}

int keyword_len(char *p) {
  char *key[] = {"return", "if", "else", "for", "while", "char", "int", "sizeof"};
  int key_len;
  for (int i = 0; i < sizeof(key) / sizeof(*key); i++) {
    key_len = strlen(key[i]);
    if (strncmp(key[i], p, key_len) == 0 && ! is_ident2(p[key_len])) {
      return key_len;
    }
  }
  return 0;
}

char *string_literal_end(char *p) {
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

Token *read_string_literal(char *start) {
  char *end = string_literal_end(start + 1);
  char *buf = calloc(1, end - start);

  int len = 0;
  for (char *p = start + 1; p < end; p++) {
    if (*p == '\\') {
      buf[len++] = read_escaped_char(++p);
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

int read_escaped_char(char *p) {
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

char *read_file(char *path) {
  filename = path;
  FILE *fp;
  if (strcmp(path, "-") == 0) {
    fp = stdin;
  } else {
    fp = fopen(path, "r");
    if (!fp) {
      error("cannot open %s: %s", path, strerror(errno));
    }
  }

  char *buf;
  size_t buflen;
  FILE *out = open_memstream(&buf, &buflen);

  for (;;) {
    char line[4096];
    int n = fread(line, 1, sizeof(line), fp);
    if (n == 0) {
      break;
    }
    fwrite(line, 1, n, out);
  }

  if (fp != stdin) {
    fclose(fp);
  }

  fflush(out);

  // add newline to end of file
  if (buflen == 0 || buf[buflen - 1] != '\n') {
    fputc('\n', out);
  }
  fputc('\0', out);

  fclose(out);

  return buf;
}

Token *tokenize(char* p) {
  user_input = p;
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
        strncmp(">=", p, 2) == 0
    ) {
      cur = new_token(TK_PUNCT, cur, p, 2);
      p += 2;
      continue;
    }

    if (strchr("+-*/()<>=;{},&[]", *p)) {
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
  return head.next;

}

Token *tokenize_file(char *path) {
  return tokenize(read_file(path));
}
