#include "test.h"

int main() {
  ASSERT(0, ({ int a; if (5) a =0; a;}));
  ASSERT(4, ({ int flag = 0; int a; if (flag) a =0; else a = 4; a; }));
  ASSERT(0, ({ int flag = 9; int a; if (flag) a =0; else a = 4; a;}));
  ASSERT(3, ({ int i; i=0; while (i<3) {i = i + 1;} i; }));
  ASSERT(3, ({ int a = 0; int i; for (i=0; i<3; i=i+1) {a = a + i;} a; }));
  ASSERT(0, ({ int a=0; for (;;) {a;} }));

  ASSERT(2, ({ int i=0,j=3; (i=2, j) = 4; i;}));
  ASSERT(4, ({ int i=0,j=3; (i=2, j) = 4; j;}));

  ASSERT(2, ({ 1; 2; 2; }));
  ASSERT(2, ({;; ; 2;}));
  ASSERT(2, ({ {{};; ; 2; }{}}));
  ASSERT(0, ({; }));

  /* error
  ASSERT(0, ({ }));
  ASSERT(0, ({ int a[3]; int *p; p = &a;}));
  ASSERT(0, ({ int a[3]; int b[3]; b = a;}));
  ASSERT(0, ({ void a; }));
  */

  printf("OK\n");
  return 0;
}