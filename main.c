#include "9cc.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "no args\n");
    exit(1);
  }

  user_input = argv[1];
  token = tokenize();
  program();

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  int i = 0;
  while (code[i] != NULL) {
    gen(code[i]);
    i++;
  }

  printf("  pop rax\n");
  printf("  ret\n");

  return 0;
}

