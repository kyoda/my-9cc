#include "test.h"

int ret3() { return 3; }
int add2(int a, int b) { return a + b; }
int sub2(int a, int b) { return a - b; }
int add6(int a, int b, int c, int d, int e, int f) { return a + b + c + d + e + f; }
int add3(int a, int b, int c) { return a + b + c; }
int mul3(int a, int b, int c) { return a * b * c; }
int cal3(int a, int b, int c) { return a - b * c; }
int cal6(int a, int b, int c, int d, int e, int f) { return f + e / b - c - a - d; }
int sub_long(long a, long b, long c) { return a - b - c;}
int sub_short(long a, long b, long c) { return a - b - c;}
int g1;
int *g1_ptr() { return &g1; }
char int_to_char(int x) { return x; }

int main() {
  ASSERT(8, add2(5, 3));
  ASSERT(3, ret3());
  ASSERT(8, add2(5, 3));
  ASSERT(2, sub2(5, 3));
  ASSERT(66, add6(1, 2, add6(3, 4, 5, 6, 7, 11), 8, 9, 10));
  ASSERT(10, add3(5, 3, 2));
  ASSERT(30, mul3(5, 3, 2));
  ASSERT(2, cal3(5, 3, 1));
  ASSERT(8, cal6(5, 3, 2, 8, 9, 20));

  ASSERT(5, sub_long(9, 3, 1));
  ASSERT(5, sub_short(9, 3, 1));

  g1 = 1;
  ASSERT(1, *g1_ptr());
  ASSERT(0, int_to_char(256));



  printf("OK\n");
  return 0;
}