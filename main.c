#include "9cc.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "no args\n");
    exit(1);
  }

  user_input = argv[1];
  token = tokenize();

  Node *n = expr();

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  gen(n);

  printf("  pop rax\n");
  printf("  ret\n");

  return 0;
}

