#include "test.h"
//void alloc3(int **p, int a, int b, int c) { int *i = malloc(sizeof(int) * 3); i[0] = a, i[1] = b; i[2] = c; *p = i; };
//void alloc3add2(int **p, int a, int b, int c) { int *i = malloc(sizeof(int) * 3); i[0] = a, i[1] = b; i[2] = c; *p = i + 2; };

int main() {
  ASSERT(25, ({ int i = 20;int *j; j = &i; *j + 5;}));
  ASSERT(5, ({ int i;int *p; p = &i; i = 5; *p;}));
  ASSERT(5, ({ int i = 7;int *p = &i; *p = 5; *p;}));
  ASSERT(5, ({ int x=5; *&x; }));
  ASSERT(5, ({ int x=5; int *y=&x; int **z=&y; **z; }));
  //ASSERT(7, ({ int *p; alloc3(&p, 7, 11, 3); *p;}));
  //ASSERT(11, ({ int *p; alloc3(&p, 7, 11, 3); *(1+p);}));
  //ASSERT(3, ({ int *p; alloc3(&p, 7, 11, 3); *(p+2);}));
  //ASSERT(11, ({ int *p; alloc3add2(&p, 7, 11, 3); *(p-1);}));
  //ASSERT(2, ({ int *p; alloc3(&p, 7, 11, 3); (p+2)-p;}));
  ASSERT(8, ({ sizeof 8;}));
  ASSERT(8, ({ int *p; sizeof p;}));
  ASSERT(4, ({ int *p; sizeof *p;}));
  ASSERT(10, ({ sizeof sizeof sizeof (8+3-2) + 2;}));
  ASSERT(0, ({ int a[3]; 0;}));
  ASSERT(99, ({ 2["abcd"]; }));
  ASSERT(3, ({ int x[2]; int *y=x; *y=3; *x; }));
  ASSERT(5, ({ int a[3]; *(a+1) = 5; *(a+1);}));
  ASSERT(15, ({ int a[3]; *(a+1) = 5; 3 * *(a+1);}));
  ASSERT(6, ({ int a[8]; a[0] = 6; a[1] = 6; a[2] = 6; a[3] = 6; int b = 7; int c = 17; a[1];}));
  ASSERT(4, ({ int a[3]; a[0] = 5; *(a+1) = 4; *(a+2) = 3; a[1];}));
  ASSERT(4, ({ int a[3]; a[0] = 5; *(a+1) = 4; *(a+2) = 3; a[1];}));
  ASSERT(5, ({ int a[3]; a[2] = 5; a[2];}));
  ASSERT(5, ({ int a[3]; a[2] = 5; 2[a];}));
  ASSERT(7, ({ int a[2][3]; a[0][2] = 7; a[0][2];}));
  ASSERT(7, ({ int a[2][3]; int *b = a; b[2] = 7; a[0][2];}));
  ASSERT(5, ({ int a[2][3]; a[1][0] = 5; a[1][0];}));
  ASSERT(48, ({ int x[3][4]; sizeof(x); }));
  ASSERT(8, ({ int x; sizeof(&x); }));
  ASSERT(5, ({ int x[3][4]; sizeof **x + 1; }));
  ASSERT(20, ({ int x[3][5]; sizeof *x; }));
  ASSERT(4, ({ int x[3][4]; sizeof(**x + 1); }));

  printf("OK\n");
  return 0;
}