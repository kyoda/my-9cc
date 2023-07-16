#include "9cc.h"

int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);
  
  Token *token = tokenize_file(argv[1]);
  Obj *prog = parse(token);
  codegen(prog);

  return 0;
}

