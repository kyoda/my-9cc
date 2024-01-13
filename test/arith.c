#include "test.h"

int main() {
  ASSERT(0, 0);
  ASSERT(-1, -3 + 2);
  ASSERT(16, 8 * (4  / 2) +   7 -   6-1);
  ASSERT(6, ((3 * (4 * (1) / (21-19)))));
  ASSERT(8, -2+5*-8+50);
  ASSERT(2, 5-3*1);
  ASSERT(5, 2 - -3);
  ASSERT(3,  0- -3);
  ASSERT(3,  - - +3);
  ASSERT(88, -2+(5*+8+50));
  ASSERT(1, 3 == 3);
  ASSERT(1, 3 != 2);
  ASSERT(1, 3 > 2);
  ASSERT(1, 3 >= 3);
  ASSERT(0, 3 < 2);
  ASSERT(1, 3 <= 3);
  ASSERT(0, 2 + 3 * 2 / 2 <= 3 == 0 < 1);

  ASSERT(5, ({ int a = 2; a += 3; }));
  ASSERT(5, ({ int a = 2; a += 3; a; }));
  ASSERT(5, ({ int a = 2; a += 3; a; }));
  ASSERT(0, ({ int a = 2; a -= 2; }));
  ASSERT(0, ({ int a = 2; a -= 2; a; }));
  ASSERT(1, ({ int a = 2; a -= a == 2; a; }));
  ASSERT(16, ({ int a = 8; a *= 2; }));
  ASSERT(16, ({ int a = 8; a *= 2; a; }));
  ASSERT(2, ({ int a = 2; a *= a == 2; a; }));
  ASSERT(4, ({ int a = 8; a /= 2; }));
  ASSERT(4, ({ int a = 8; a /= 2; a; }));
  ASSERT(2, ({ int a = 2; a /= a == 2; a; }));

  ASSERT(3, ({ int a = 2; ++a; }));
  ASSERT(3, ({ int a[2]; a[0] = 1; a[1] = 3; int *p = a; *(++p); }));
  ASSERT(3, ({ int a[2]; a[0] = 1; a[1] = 3; int *p = a; *(++p); }));
  ASSERT(2, ({ int a[2]; a[0] = 1; a[1] = 3; int *p = a; ++*p; }));
  ASSERT(1, ({ int a = 2; --a; }));
  ASSERT(1, ({ int a[2]; a[0] = 1; a[1] = 3; int *p = a + 1; *(--p); }));
  ASSERT(2, ({ int a[2]; a[0] = 1; a[1] = 3; int *p = a + 1; --*p; }));

  printf("OK\n");
  return 0;
}