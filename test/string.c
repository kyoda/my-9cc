#include "test.h"

int main() {
  ASSERT(97, "abc"[0]);
  ASSERT(98, ({ char *str = "abc"; str[1]; }));

  printf("OK\n");
  return 0;
}