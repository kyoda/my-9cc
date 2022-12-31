#include "9cc.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "no args\n");
    exit(1);
  }

  user_input = argv[1];
  token = tokenize();
  locals = new_locals(locals);

  //debug
  //Token *t = token;
  //while(t->kind != TK_EOF) {
  //  fprintf(stderr, "token->loc: %s\n", t->loc);
  //  fprintf(stderr, "token->kind: %d\n", t->kind);
  //  t = t->next;
  //}

  program();
  gen_main();

  return 0;
}

