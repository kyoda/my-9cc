#include "test.h"

int main() {
  ASSERT(0, 0);
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

  printf("OK\n");
  return 0;
}