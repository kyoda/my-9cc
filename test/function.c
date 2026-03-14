#include "test.h"

int ret3(void) { return 3; }
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
int *g1_ptr(void) { return &g1; }
char int_to_char(int x) { return x; }
int div_long(long a, long b) { return a / b;}
int arg_char(char a) { return a; }

_Bool add_bool(_Bool a) { return a + 1; }
_Bool sub_bool(_Bool a) { return a - 1; }

static int static_func(void) { return 1; }

int param_decay(int a[]) { return a[0]; }

int counter() {
  static int i;
  static int j = 1 + 1;
  return i++ + j++;
}

void ret_none(void) {
  return;
}

int add_all(int n, ...);

# include <stdarg.h>
int sum6(int a, int b, ...) {
  va_list ap;
  va_start(ap, b);
  int c = va_arg(ap, int);
  int d = va_arg(ap, int);
  int e = va_arg(ap, int);
  int f = va_arg(ap, int);

  return  a + b + c + d + e + f;
}

int sum7(int a, int b, ...) {
  va_list ap;
  va_start(ap, b);
  int c = va_arg(ap, int);
  int d = va_arg(ap, int);
  int e = va_arg(ap, int);
  int f = va_arg(ap, int);
  int g = va_arg(ap, int);

  return  a + b + c + d + e + f + g;
}

int sum8(int a, int b, ...) {
  va_list ap;
  va_start(ap, b);
  int c = va_arg(ap, int);
  int d = va_arg(ap, int);
  int e = va_arg(ap, int);
  int f = va_arg(ap, int);
  int g = va_arg(ap, int);
  int h = va_arg(ap, int);


  return  a + b + c + d + e + f + g + h;
}

int sum10(int a, int b, ...) {
  va_list ap;
  va_start(ap, b);
  int c = va_arg(ap, int);
  int d = va_arg(ap, int);
  int e = va_arg(ap, int);
  int f = va_arg(ap, int);
  int g = va_arg(ap, int);
  int h = va_arg(ap, int);
  int i = va_arg(ap, int);
  int j = va_arg(ap, int);

  return  a + b + c + d + e + f + g + h + i + j;
}

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
  ASSERT(-5, div_long(-10, 2));
  ASSERT(1, arg_char(513));

  ASSERT(1, add_bool(3));
  ASSERT(1, add_bool(-1));
  ASSERT(1, sub_bool(0));
  ASSERT(0, sub_bool(1));
  ASSERT(0, sub_bool(5));

  ASSERT(1, static_func());

  ASSERT(3, ({ int b[2]; b[0] = 3; param_decay(b); }));

  ASSERT(2, counter());
  ASSERT(4, counter());
  ASSERT(6, counter());

  ASSERT(15, add_all(5, 1, 2, 3, 4, 5));
  ASSERT(8, add_all(5, 1, 2, 3, 4, -2));
  ASSERT(0, ({ char buf[100]; sprintf(buf, "%d, %d, %s", 1, 2, "hello"); strcmp(buf, "1, 2, hello"); }));

  ret_none();

  ASSERT(21, sum6(1, 2, 3, 4, 5, 6));
  ASSERT(28, sum7(1, 2, 3, 4, 5, 6, 7));
  ASSERT(36, sum8(1, 2, 3, 4, 5, 6, 7, 8));
  ASSERT(55, sum10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));

  printf("OK\n");
  return 0;
}