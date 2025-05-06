#include "test.h"

int main() {
  ASSERT(1, ({ _Alignof(char); }));
  ASSERT(2, ({ _Alignof(short); }));
  ASSERT(4, ({ _Alignof(int); }));
  ASSERT(8, ({ _Alignof(long); }));
  ASSERT(8, ({ _Alignof(long long); }));
  ASSERT(1, ({ _Alignof(char[4]); }));
  ASSERT(4, ({ _Alignof(int[5]); }));
  ASSERT(1, ({ _Alignof(struct {char a; char b;}[1]); }));
  ASSERT(8, ({ _Alignof(struct {char a; long b;}[3]); }));

  printf("OK\n");
  return 0;
}
