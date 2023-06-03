#include "9cc.h"

int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);
  
  Token *token = tokenize(argv[1]);
  Obj *prog = parse(token);
  codegen(prog);

  //debug
  //print_token(token);

  return 0;
}

