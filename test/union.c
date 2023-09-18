#include "test.h"

int main() {
  ASSERT(4, ({ union {int a; char b[3];} y; sizeof(y);}));
  ASSERT(8, ({ union t {int a; char b[5];} x; union t y; sizeof(y);}));
  ASSERT(256, ({ union {int a; char b[2];} y; y.b[0] = 0; y.b[1] = 1; y.a;}));
  ASSERT(1, ({ union {int a; char b[4];} y; y.a = 300; y.b[1];}));

  ASSERT(3, ({ union t {int a;} x; int *y = x; x.a = 3; *y;}));

  printf("OK\n");
  return 0;
}