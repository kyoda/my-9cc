#include "test.h"

int main() {
  ASSERT(97, 'a');
  ASSERT(10, '\n');
  ASSERT(15, '\x000f');
  ASSERT(112, '\x70'); // 0111 0000
  ASSERT(-128, '\x80'); // 1000 0000
  ASSERT(-112, '\x90'); // 1001 0000

  ASSERT(0, 0x0);
  ASSERT(0, 0X0);
  ASSERT(0, 0x000000000000000);
  ASSERT(15, 0xf);
  ASSERT(15, 0xF);
  ASSERT(2, 0b10);
  ASSERT(2, 0B10);
  ASSERT(15, 0b01111);
  ASSERT(511, 0777); // 7*8^2 + 7*8^1 + 7*8^0 = 511

  printf("OK\n");
  return 0;
}