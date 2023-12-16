#include "test.h"

int main() {
  ASSERT(1, ({ char a; sizeof(a); }));
  ASSERT(2, ({ short a; sizeof(a); }));
  ASSERT(2, ({ short int a; sizeof(a); }));
  ASSERT(2, ({ int short a; sizeof(a); }));
  ASSERT(4, ({ int a; sizeof(a); }));
  ASSERT(8, ({ long a; sizeof(a); }));
  ASSERT(8, ({ long int a; sizeof(a); }));
  ASSERT(8, ({ int long a; sizeof(a); }));
  ASSERT(8, ({ long long a; sizeof(a); }));
  ASSERT(8, ({ long long int a; sizeof(a); }));
  ASSERT(8, ({ long int long a; sizeof(a); }));
  ASSERT(8, ({ int long long a; sizeof(a); }));

  ASSERT(1, ({ _Bool a; sizeof(a); }));
  ASSERT(0, ({ _Bool a = 0; a; }));
  ASSERT(1, ({ _Bool a = 1; a; }));
  ASSERT(1, ({ _Bool a = 2; a; }));
  ASSERT(1, ({ _Bool a = 512; a; }));
  ASSERT(1, ({ _Bool a = -2; a; }));
  ASSERT(0, ({ (_Bool)0; }));
  ASSERT(1, ({ (_Bool)1; }));
  ASSERT(1, ({ (_Bool)256; }));
  ASSERT(1, ({ (_Bool)-1; }));
  ASSERT(0, ({ (_Bool)(char)256; }));

  /* error
  ASSERT(4, ({ int int a; sizeof(a); }));
  */

  printf("OK\n");
  return 0;
}