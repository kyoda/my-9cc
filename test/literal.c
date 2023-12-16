#include "test.h"

int main() {
  ASSERT(97, 'a');
  ASSERT(10, '\n');
  ASSERT(15, '\x000f');
  ASSERT(112, '\x70'); // 0111 0000
  ASSERT(-128, '\x80'); // 1000 0000
  ASSERT(-112, '\x90'); // 1001 0000

  printf("OK\n");
  return 0;
}