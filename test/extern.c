#include "test.h"

extern int ext1;
extern int *ext2;

int main() {
  ASSERT(5, ext1);
  ASSERT(5, *ext2);

  extern int ext3;
  ASSERT(3, ext3);

  int ext_fn1(int x);
  ASSERT(7, ext_fn1(7));

  extern int ext_fn2(int x);
  ASSERT(6, ext_fn1(6));

  printf("OK\n");
  return 0;
}
