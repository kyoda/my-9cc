#include "test.h"

int main() {
  ASSERT(1, ({char a = 1; (char)a;}));
  ASSERT(1, ({char a = 1; (short)a;}));
  ASSERT(1, ({char a = 1; (int)a;}));
  ASSERT(1, ({char a = 1; (long)a;}));
  ASSERT(1, ({short a = 1; (char)a;}));
  ASSERT(1, ({short a = 1; (short)a;}));
  ASSERT(1, ({short a = 1; (int)a;}));
  ASSERT(1, ({short a = 1; (long)a;}));
  ASSERT(1, ({int a = 1; (char)a;}));
  ASSERT(1, ({int a = 1; (short)a;}));
  ASSERT(1, ({int a = 1; (int)a;}));
  ASSERT(1, ({int a = 1; (long)a;}));
  ASSERT(1, ({long a = 1; (char)a;}));
  ASSERT(1, ({long a = 1; (short)a;}));
  ASSERT(1, ({long a = 1; (int)a;}));
  ASSERT(1, ({long a = 1; (long)a;}));

  ASSERT(1, ({char a = 257; (char)a;}));
  ASSERT(1, ({char a = 257; (short)a;}));
  ASSERT(1, ({char a = 257; (int)a;}));
  ASSERT(1, ({char a = 257; (long)a;}));
  ASSERT(1, ({short a = 257; (char)a;}));
  ASSERT(257, ({short a = 257; (short)a;}));
  ASSERT(257, ({short a = 257; (int)a;}));
  ASSERT(257, ({short a = 257; (long)a;}));
  ASSERT(1, ({int a = 257; (char)a;}));
  ASSERT(257, ({int a = 257; (short)a;}));
  ASSERT(257, ({int a = 257; (int)a;}));
  ASSERT(257, ({int a = 257; (long)a;}));
  ASSERT(1, ({long a = 257; (char)a;}));
  ASSERT(257, ({long a = 257; (short)a;}));
  ASSERT(257, ({long a = 257; (int)a;}));
  ASSERT(257, ({long a = 257; (long)a;}));

  ASSERT(1, (char)257);
  ASSERT(1, (short)(1024*64+1));
  ASSERT(1, (int)(1024*1024*1024*4+1));
  ASSERT(1, ({ int a=1; (long)a;}));
  ASSERT(1, (long)1);
  ASSERT(0, (long)&*(int *)0);

  /* 
    int x=512 --> 00000000 00000000 00000010 00000000
    ポインタのchar型にしてデリファレンスするのでload時に1byte目のみの操作(movzx rax, BYTE PTR [rax])となっている
    (add_type()でND_DEREFはlhs->baseのタイプでそのlhsのタイプはND_CASTのタイプ)
  */
  ASSERT(513, ({ int x=512; *(char *)&x=1; x; }));
  ASSERT(5, ({ int x=5; long y=(long)&x; *(int*)y; }));

  (void)1;

  printf("OK\n");
  return 0;
}