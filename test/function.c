#include "test.h"
int ret3() { return 3; }
int add2(int a, int b) { return a + b; }
int sub2(int a, int b) { return a - b; }
int add6(int a, int b, int c, int d, int e, int f) { return a + b + c + d + e + f; }
//void alloc3(int **p, int a, int b, int c) { int *i = malloc(sizeof(int) * 3); i[0] = a, i[1] = b; i[2] = c; *p = i; };
//void alloc3add2(int **p, int a, int b, int c) { int *i = malloc(sizeof(int) * 3); i[0] = a, i[1] = b; i[2] = c; *p = i + 2; };

int main() {
  ASSERT(3, ret3());
  ASSERT(2, sub2(5, 3));
  ASSERT(8, add2(5, 3));
  ASSERT(66, add6(1, 2, add6(3, 4, 5, 6, 7, 11), 8, 9, 10));
  //ASSERT(10, add3(5, 3, 2););

  printf("OK\n");
  return 0;
}