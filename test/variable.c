#include "test.h"

int x;
int y;
int z;
char *str;

int main() {
  //local
  ASSERT(2, ({ int a; a = 2; }));
  ASSERT(2, ({ int a = 2; a; }));
  ASSERT(10, ({ int a = 1; int b = 4*3; b / a - 2; }));
  ASSERT(25, ({ int c; int d; c=d=5; c*d; }));
  ASSERT(2, ({ int a; int b; int c; int d; a=b=c=d=2;}));
  ASSERT(30, ({ int _foo1=5; int _bar2=6; _foo1*_bar2; }));

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