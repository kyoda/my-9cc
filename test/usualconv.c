#include "test.h"

int main() {
  ASSERT(70000, ({ int a = 60000; short b = 10000; a + b; }));
  ASSERT((long)-3, 2 + (long)-5);
  ASSERT(0, 2 < -5);
  ASSERT(0, 2 < (long)-5);
  ASSERT(1, 2 >= (long)-5);
  ASSERT(1, 2 == (long)2);
  ASSERT(-1, ({ long a = -1; a; }));
  ASSERT(-1, 1024*1024*1024*4 - 1);
  ASSERT(0, 4294967296); //2**32
  ASSERT(2147483647, 2147483647); //2**31 - 1
  ASSERT(-2147483648, 2147483647 + 1);
  ASSERT(-2147483648, 2147483647 + (long)1);
  ASSERT(2147483648, (long)2147483647 + 1);
  ASSERT(0, 2147483647 + 2147483647 + 2);

  printf("OK\n");
  return 0;
}
