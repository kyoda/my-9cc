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
  ASSERT(0, ({ int a = 8; a %= 2; }));
  ASSERT(0, ({ int a = 8; a %= 2; a; }));
  ASSERT(0, ({ int a = 8; a %= 2 == 2; a; }));

  ASSERT(0, ({ int a = 0; a |= 0; a; }));
  ASSERT(1, ({ int a = 0; a |= 1; a; }));
  ASSERT(1, ({ int a = 1; a |= 0; a; }));
  ASSERT(1, ({ int a = 1; a |= 1; a; }));

  ASSERT(0, ({ int a = 0; a ^= 0; a; }));
  ASSERT(1, ({ int a = 0; a ^= 1; a; }));
  ASSERT(1, ({ int a = 1; a ^= 0; a; }));
  ASSERT(0, ({ int a = 1; a ^= 1; a; }));

  ASSERT(0, ({ int a = 0; a &= 0; a; }));
  ASSERT(0, ({ int a = 0; a &= 1; a; }));
  ASSERT(0, ({ int a = 1; a &= 0; a; }));
  ASSERT(1, ({ int a = 1; a &= 1; a; }));

  ASSERT(3, ({ int a = 2; ++a; }));
  ASSERT(3, ({ int a[2]; a[0] = 1; a[1] = 3; int *p = a; *(++p); }));
  ASSERT(3, ({ int a[2]; a[0] = 1; a[1] = 3; int *p = a; *(++p); }));
  ASSERT(2, ({ int a[2]; a[0] = 1; a[1] = 3; int *p = a; ++*p; }));
  ASSERT(1, ({ int a = 2; --a; }));
  ASSERT(1, ({ int a[2]; a[0] = 1; a[1] = 3; int *p = a + 1; *(--p); }));
  ASSERT(2, ({ int a[2]; a[0] = 1; a[1] = 3; int *p = a + 1; --*p; }));

  ASSERT(1, ({ int a = 1; a++; }));
  ASSERT(2, ({ int a = 1; a++; a;}));
  ASSERT(1, ({ int a = 1; a--; }));
  ASSERT(0, ({ int a = 1; a--; a;}));

  ASSERT(1, !0);
  ASSERT(0, !1);
  ASSERT(0, !2);
  ASSERT(0, !-3);
  ASSERT(0, !(char)4);
  ASSERT(0, !(long)5);
  ASSERT(4, sizeof(!-7));
  ASSERT(4, sizeof(!(char)-8));
  ASSERT(4, sizeof(!(long)9));

  ASSERT(-1, ~0);
  ASSERT(0, ~-1);

  ASSERT(0, 4 % 4);
  ASSERT(1, 4 % 3); // 3 * 1 + 1
  ASSERT(1, 4 % -3); // -3 * -1 + 1
  ASSERT(-1, 4 / -3); // -3 * -1 + 1
  ASSERT(-3, -3 % 5); // 5 * 0 + -3
  ASSERT(-1, -8 / 5); // 5 * -1 + -3
  ASSERT(-3, -8 % 5); // 5 * -1 + -3

  ASSERT(0, 0|0);
  ASSERT(1, 0|1);
  ASSERT(1, 1|1);
  ASSERT(1, 1|0);
  ASSERT(3, 0|3);
  ASSERT(7, 0|7);
  ASSERT(3, 1|3);
  ASSERT(7, 5|2); // 101 | 010
  ASSERT(-1, -1|10); // 11111111111111111111111111111111 ^ 00000000000000000000000000001010
  ASSERT(-1, 0b11111111111111111111111111111111|0b00000000000000000000000000001010);
  ASSERT(-1, 0xffffffff|00000000012);

  ASSERT(0, 0^0);
  ASSERT(1, 0^1);
  ASSERT(1, 1^0);
  ASSERT(0, 1^1);
  ASSERT(2, 1^3);
  ASSERT(0, 7^7);
  ASSERT(-11, -1^10); // 11111111111111111111111111111111 ^ 00000000000000000000000000001010
  ASSERT(-11, 0b11111111111111111111111111111111^0b00000000000000000000000000001010);
  ASSERT(-11, 0xffffffff^00000000012);

  ASSERT(0, 0&0);
  ASSERT(0, 0&1);
  ASSERT(0, 1&0);
  ASSERT(1, 1&1);
  ASSERT(0, 5&2); // 101 & 010
  ASSERT(10, -1&10); // 11111111111111111111111111111111 ^ 00000000000000000000000000001010
  ASSERT(10, 0b11111111111111111111111111111010&0b00000000000000000000000000001010);
  ASSERT(10, 0xffffffff&00000000012);

  ASSERT(1, 1<<0);
  ASSERT(2, 1<<1);
  ASSERT(-2, -1<<1);
  ASSERT(2, 2>>0);
  ASSERT(1, 2>>1);
  ASSERT(-1, -1>>1);
  ASSERT(2, ({ int a = 1; a <<= 1; a; }));
  ASSERT(-2, ({ int a = -1; a <<= 1; a; }));
  ASSERT(1, ({ int a = 2; a >>= 1; a; }));
  ASSERT(-1, ({ int a = -1; a >>= 1; a; }));

  ASSERT(1, 0 ? 3 : 1);
  ASSERT(3, 1 ? 3 : 1);
  ASSERT(-1, 1?-1:1);
  ASSERT(4, sizeof(-2 ? 3 : -1));
  ASSERT(8, sizeof(0 ? (long)3 : 1));
  ASSERT(8, sizeof(1 ? (long)3 : 1));
  1 ? 0 : (void)-2;

  printf("OK\n");
  return 0;
}