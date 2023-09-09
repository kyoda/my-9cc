#include "test.h"

int x;
int y;
int z;
char *str;

int main() {
  //local
  ASSERT(2, ({ int a; a = 2; }));
  ASSERT(2, ({ int a = 2; a; }));
  ASSERT(3, ({ int a = 2,b=3,c; b; }));
  ASSERT(10, ({ int a = 1; int b = 4*3; b / a - 2; }));
  ASSERT(25, ({ int c; int d; c=d=5; c*d; }));
  ASSERT(2, ({ int a; int b; int c; int d; a=b=c=d=2;}));
  ASSERT(30, ({ int _foo1=5; int _bar2=6; _foo1*_bar2; }));
  //align
  /*
    例:
    int a; char b;
    locals = b --> a
    offset = 0 --> offset = 1(b->offset) --> offset = 1 + 4 --> align_to(5, 4) = 8(a->offset)
    char a; int b;
    offset = 0 --> offset = 4(b->offset) --> offset = 4 + 1 --> align_to(5, 1) = 5(a->offset)
  */
  ASSERT(7, ({ int a; char b; char *p = &a; char *q = &b; q-p; }));
  ASSERT(1, ({ char a; int b; char *p = &a; char *q = &b; q-p; }));

  //global
  ASSERT(0, ({ x; }));
  ASSERT(4, ({ sizeof x; }));
  ASSERT(8, ({ sizeof &x; }));
  ASSERT(7, ({ x = 7; x; }));
  ASSERT(12, ({ x = 3; y = 4; z = 5; x + y + z; }));
  ASSERT(8, ({ sizeof str; }));
  ASSERT(1, ({ sizeof str[0]; }));
  ASSERT(97, ({ str = "abc"; str[0];}));

  printf("OK\n");
  return 0;
}