#include "test.h"

int main() {
  ASSERT(2, ({ int a; a = 2; }));
  ASSERT(2, ({ int a = 2; a; }));
  //ASSERT(10, ({ int a = 1; int b = 4*3; return b / a - 2; }));
  //ASSERT(25, ({ int c; int d; c=d=5;return c*d; }));
  //ASSERT(2, ({ int a; int b; int c; int d; return a=b=c=d=2;}));
  //ASSERT(30, ({ int _foo1=5; int _bar2=6;return _foo1*_bar2; }));
  //ASSERT(2, ({ int a=1;int b=2;return  a*b; a=3*8; return 5; }));
  //ASSERT(0, ({ int a; if (5) a =0; return a;}));
  //ASSERT(4, ({ int flag = 0; int a; if (flag) a =0; else a = 4; return a; }));
  //ASSERT(0, ({ int flag = 9; int a; if (flag) a =0; else a = 4; return a;}));
  //ASSERT(3, ({ int i; i=0; while (i<3) {i = i + 1;} return i; }));
  //ASSERT(3, ({ int a = 0; int i; for (i=0; i<3; i=i+1) {a = a + i;} return a; }));
  //ASSERT(0, ({ int a=0; for (;;) {return a;} }));

  printf("OK\n");
  return 0;
}