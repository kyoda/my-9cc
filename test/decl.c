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

  /* error
  ASSERT(4, ({ int int a; sizeof(a); }));
  */

  printf("OK\n");
  return 0;
}