#include "test.h"

int main() {
  ASSERT(1, ({ enum { one = 9-8 }; one;}));
  ASSERT(1, ({ int i = 0; switch (5) { case 3+2: i++; }; i;}));
  ASSERT(1, ({ char x[(2+1-1*4/2)%5]; sizeof(x); }));
  ASSERT(1, ({ char x[0 | 1]; sizeof(x); }));
  ASSERT(1, ({ char x[0 ^ 1]; sizeof(x); }));
  ASSERT(0, ({ char x[2 & 1]; sizeof(x); }));
  ASSERT(2, ({ char x[-(-2)]; sizeof(x); }));
  ASSERT(1, ({ char x[2?1:0]; sizeof(x); }));
  ASSERT(1, ({ char x[3 && 1]; sizeof(x); }));
  ASSERT(1, ({ char x[3 || 1]; sizeof(x); }));
  ASSERT(1, ({ char x[(3, 1)]; sizeof(x); }));
  ASSERT(0, ({ char x[!1]; sizeof(x); }));
  ASSERT(0, ({ char x[~-1]; sizeof(x); }));
  ASSERT(2, ({ char x[1<<1]; sizeof(x); }));
  ASSERT(1, ({ char x[2>>1]; sizeof(x); }));
  ASSERT(0, ({ char x[2==1]; sizeof(x); }));
  ASSERT(1, ({ char x[2!=1]; sizeof(x); }));
  ASSERT(0, ({ char x[2<1]; sizeof(x); }));
  ASSERT(0, ({ char x[2<=1]; sizeof(x); }));
  ASSERT(15, ({ char x[(char)0xffffff0f]; sizeof(x); }));
  ASSERT(0x100f, ({ char x[(short)0xffff100f]; sizeof(x); }));
  ASSERT(1, ({ char x[(int)0xffffffff+2]; sizeof(x); }));
  ASSERT(2, ({ char x[(int)0+2]; sizeof(x); }));
  ASSERT(3, ({ char x[(int*)16-(int*)4]; sizeof(x); }));



  /* error
  // ポインタの足し算, 0 + 2 * intのサイズ
  ASSERT(8, ({ char x[(int*)0+2]; sizeof(x); })); //gcc error
  ASSERT(12, ({ char x[(int*)16-1]; sizeof(x); })); //gcc error
  ASSERT(3, ({ int a = 3; char x[a]; sizeof(x); })); //gcc ok

  */

  printf("OK\n");
  return 0;
}
