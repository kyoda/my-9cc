#include "test.h"

int main() {
  ASSERT(4, ({ struct {int a;} x; sizeof(x);}));
  ASSERT(0, ({ struct {} x; 0;}));
  ASSERT(0, ({ struct {int a;} x; 0;}));
  ASSERT(0, ({ struct {int a, b;} x; 0;}));
  ASSERT(0, ({ struct {int a, *b; char *c;} x; 0;}));
  ASSERT(1, ({ struct {int a;} x; x.a = 1; x.a; }));
  ASSERT(1, ({ struct {int a, b;} x; x.a = 1; x.b = 2; x.a;}));
  ASSERT(2, ({ struct {int a, b;} x; x.a = 1; x.b = 2; x.b;}));
  ASSERT(3, ({ struct {int a, b; char c;} x; x.a = 1; x.b = 2; x.c = 3; x.c;}));

  ASSERT(1, ({ struct {char a, b;} x[3]; char *p = x; p[0] = 1; p[1] = 2; x[0].a;}));
  ASSERT(2, ({ struct {char a, b;} x[3]; char *p = x; p[0] = 1; p[1] = 2; x[0].b;}));
  ASSERT(3, ({ struct {char a, b;} x[3]; char *p = x; p[2] = 3; p[3] = 4; x[1].a;}));
  ASSERT(4, ({ struct {char a, b;} x[3]; char *p = x; p[2] = 3; p[3] = 4; x[1].b;}));

  ASSERT(7, ({ struct {char a[3], b[5];} x; char *p = &x; x.a[0] = 7; p[0];}));
  ASSERT(9, ({ struct {char a[3], b[5];} x; char *p = &x; x.b[1] = 9; p[4];}));
  ASSERT(3, ({ struct { struct { int b; } a; } x; x.a.b = 3; x.a.b;}));

  ASSERT(0, ({ struct {} x; sizeof x;}));
  ASSERT(4, ({ struct {int a;} x; sizeof(x);}));
  ASSERT(8, ({ struct {int a, b;} x; sizeof(x);}));
  ASSERT(16, ({ struct {int a, *b;} x; sizeof(x);}));
  ASSERT(24, ({ struct {int a, *b; char c;} x; sizeof(x);}));
  ASSERT(12, ({ struct { int a[3]; } x; sizeof(x);}));
  ASSERT(16, ({ struct { int a; } x[4]; sizeof(x);}));
  ASSERT(48, ({ struct { int a[3]; } x[4]; sizeof(x);}));
  ASSERT(8, ({ struct {int a; char b;} x; sizeof(x);}));
  ASSERT(16, ({ struct {int *a; char b;} x; sizeof(x);}));

  ASSERT(16, ({ struct t {int *a; char b;}; struct t y; sizeof(y);}));
  ASSERT(16, ({ struct t {int *a; char b;} x; struct t y; sizeof(y);}));
  ASSERT(16, ({ struct t {int *a; char b;} x; struct t y; sizeof(x);}));
  ASSERT(3, ({ struct t {int *a; char b;} x; struct t y; y.a = 3; y.a;}));
  ASSERT(3, ({ struct t {int *a;}; int t = 1; struct t y; y.a = 3; y.a;}));
  ASSERT(12, ({ struct t {int a[3];}; {struct t {int a;};}; struct t y; sizeof(y);}));

  ASSERT(3, ({ struct t {int a;} x; struct t *y = &x; x.a = 3; y->a;}));
  ASSERT(3, ({ struct t {int a;} x; struct t *y = &x; y->a = 3; x.a;}));

  ASSERT(16, ({ struct {long a; char b;} x; sizeof(x);}));
  ASSERT(4, ({ struct {short a; char b;} x; sizeof(x);}));

  ASSERT(0, ({ struct stag; 0;}));
  ASSERT(8, ({ struct stag *st; sizeof(st);}));
  ASSERT(4, ({ struct stag *st; struct stag { int x; }; sizeof(struct stag);}));
  ASSERT(3, ({ struct stag { struct stag *p; int a; } b; struct stag c; c.a = 3; b.p = &c; b.p->a;}));
  ASSERT(4, ({ typedef struct stag mytype; struct stag { int a; }; sizeof(struct stag);})); //stagは不完全のため上書き
  ASSERT(4, ({ typedef struct stag mytype; struct stag { int a; }; sizeof(mytype);}));
  ASSERT(4, ({ typedef struct T T; struct T { int a; }; sizeof(T);}));

  /* error
    ASSERT(3, ({ struct t {int a;} x; int *y = x; x.a = 3; *y;})); //gcc error
    ASSERT(4, ({ struct { int a; int a; } x = { 4, 3 }; x.a;})); //gcc error
    ASSERT(-1, ({ typedef struct stag val; sizeof(val);})); //gcc error
  */

  printf("OK\n");
  return 0;
}