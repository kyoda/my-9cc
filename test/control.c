#include "test.h"

int main() {
  ASSERT(0, ({ int a; if (5) a =0; a;}));
  ASSERT(4, ({ int flag = 0; int a; if (flag) a =0; else a = 4; a; }));
  ASSERT(0, ({ int flag = 9; int a; if (flag) a =0; else a = 4; a;}));
  ASSERT(3, ({ int i; i=0; while (i<3) {i = i + 1;} i; }));
  ASSERT(3, ({ int a = 0; int i; for (i=0; i<3; i=i+1) {a = a + i;} a; }));
  ASSERT(4, ({ int i; for (i=0; i<3; i=i+1) i = i + 1; i;}));

  ASSERT(0, ({ int i = 0; { int i = 4; } i; }));
  ASSERT(7, ({ int i = 7; int j = 3; for (int i=0; i<3; i=i+1) {j = i + 1;} i; }));

  ASSERT(2, ({ 1; 2; 2; }));
  ASSERT(2, ({;; ; 2;}));

  ASSERT(0, 0||0);
  ASSERT(1, 0||1);
  ASSERT(1, 1||0);
  ASSERT(1, 1||1);
  ASSERT(1, 3||1);
  ASSERT(1, 3||8);
  ASSERT(1, -1||8);
  ASSERT(1, (3-3)||8);
  ASSERT(1, (3-3)||0||3);

  ASSERT(0, 0&&0);
  ASSERT(0, 0&&1);
  ASSERT(0, 1&&0);
  ASSERT(1, 1&&1);
  ASSERT(1, 3&&1);
  ASSERT(1, 3&&8);
  ASSERT(1, -1&&8);
  ASSERT(0, (3-3)&&8);
  ASSERT(0, (3-2)&&8&&(2-2));

  ASSERT(3, ({int i = 0; goto i; i: ++i; j: ++i; k: ++i; i;}));
  ASSERT(2, ({int i = 0; goto j; i: ++i; j: ++i; k: ++i; i;}));
  ASSERT(1, ({int i = 0; goto k; i: ++i; j: ++i; k: ++i; i;}));

  ASSERT(0, ({ typedef int l; goto l; l:; 0;}));

  /* error
  //ASSERT(0, ({ int a=0; for (;;) {a;} }));
  //ASSERT(2, ({ int i=0,j=3; (i=2, j) = 4; i;}));
  //ASSERT(4, ({ int i=0,j=3; (i=2, j) = 4; j;}));
  ASSERT(0, ({ }));
  ASSERT(0, ({ int a[3]; int *p; p = &a;}));
  ASSERT(0, ({ int a[3]; int b[3]; b = a;}));
  ASSERT(0, ({ void a; }));
  //assert(2, ({ {{};; ; 2; }{}}));
  //assert(0, ({; }));
  ASSERT(2, ({return 2;}));
  ASSERT(4, ({ int i = 0; int i = 4; i; })); //compile error
  */

  printf("OK\n");
  return 0;
}