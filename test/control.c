#include "test.h"

int main() {
  ASSERT(0, ({ int a; if (5) a =0; a;}));
  ASSERT(4, ({ int flag = 0; int a; if (flag) a =0; else a = 4; a; }));
  ASSERT(0, ({ int flag = 9; int a; if (flag) a =0; else a = 4; a;}));
  ASSERT(3, ({ int i; i=0; while (i<3) {i = i + 1;} i; }));
  ASSERT(3, ({ int a = 0; int i; for (i=0; i<3; i=i+1) {a = a + i;} a; }));
  ASSERT(0, ({ int a=0; for (;;) {a;} }));

  printf("OK\n");
  return 0;
}