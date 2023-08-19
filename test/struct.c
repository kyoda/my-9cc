#include "test.h"

int main() {
  ASSERT(0, ({ struct {} x; 0;}));
  ASSERT(0, ({ struct {int a;} x; 0;}));
  ASSERT(0, ({ struct {int a, b;} x; 0;}));
  ASSERT(0, ({ struct {int a, *b; char *c;} x; 0;}));
  ASSERT(1, ({ struct {int a, b;} x; x.a = 1; x.b = 2; x.a;}));
  ASSERT(2, ({ struct {int a, b;} x; x.a = 1; x.b = 2; x.b;}));

  printf("OK\n");
  return 0;
}