#include "test.h"

int main() {
  ASSERT(0, ({ int a; if (5) a =0; a;}));
  ASSERT(4, ({ int flag = 0; int a; if (flag) a =0; else a = 4; a; }));
  ASSERT(0, ({ int flag = 9; int a; if (flag) a =0; else a = 4; a;}));
  ASSERT(3, ({ int i; i=0; while (i<3) {i = i + 1;} i; }));
  ASSERT(3, ({ int a = 0; int i; for (i=0; i<3; i=i+1) {a = a + i;} a; }));
  ASSERT(4, ({ int i; for (i=0; i<3; i=i+1) i = i + 1; i;}));

  ASSERT(0, ({ int i = 0; { int i = 4; } i; }));
  ASSERT(7, ({ int i = 7; int j = 3; for (int i=0; i<3; i=i+1) {j = i + 1;} i; }));

  ASSERT(2, ({ 1; 2; 2; }));
  ASSERT(2, ({;; ; 2;}));

  ASSERT(0, 0||0);
  ASSERT(1, 0||1);
  ASSERT(1, 1||0);
  ASSERT(1, 1||1);
  ASSERT(1, 3||1);
  ASSERT(1, 3||8);
  ASSERT(1, -1||8);
  ASSERT(1, (3-3)||8);
  ASSERT(1, (3-3)||0||3);

  ASSERT(0, 0&&0);
  ASSERT(0, 0&&1);
  ASSERT(0, 1&&0);
  ASSERT(1, 1&&1);
  ASSERT(1, 3&&1);
  ASSERT(1, 3&&8);
  ASSERT(1, -1&&8);
  ASSERT(0, (3-3)&&8);
  ASSERT(0, (3-2)&&8&&(2-2));

  ASSERT(3, ({int i = 0; goto a; a: ++i; b: ++i; c: ++i; i;}));
  ASSERT(2, ({int i = 0; goto e; d: ++i; e: ++i; f: ++i; i;}));
  ASSERT(1, ({int i = 0; goto i; g: ++i; h: ++i; i: ++i; i;}));

  ASSERT(0, ({ typedef int l; goto l; l:; 0;}));

  ASSERT(3, ({ int i = 0; for (;i<10;i++) { if (i == 3) break; } i; }));
  ASSERT(3, ({ int i = 0; for (;i<10;i++) { for (;;) break; if (i == 3) break; } i; }));
  ASSERT(3, ({ int i = 0; while (i<10) { if (++i == 3) break; } i; }));
  ASSERT(3, ({ int i = 0; while (1) { while (1) break; if (++i == 3) break; } i; }));

  ASSERT(10, ({ int i = 0, j = 0; for (;i<10;i++) { if (i > 3) continue; ++j;} i; }));
  ASSERT(4, ({ int i = 0, j = 0; for (;i<10;i++) { if (i > 3) continue; ++j;} j; }));
  ASSERT(0, ({ int i = 0, j = 0; for (;!i;i++) {for(;j<10;j++) continue; if (i < 10) break; } i; }));
  ASSERT(10, ({ int i = 0, j = 0; for (;!i;i++) {for(;j<10;j++) continue; if (i < 10) break; } j; }));
  ASSERT(11, ({ int i = 0, j = 0; while (i++<10) { if (i > 3) continue; ++j;} i; }));
  ASSERT(3, ({ int i = 0, j = 0; while (i++<10) { if (i > 3) continue; ++j;} j; }));
  ASSERT(1, ({ int i = 0, j = 0; while (!i) {for(;j<10;j++) continue; if (i++ < 10) break; } i; }));
  ASSERT(10, ({ int i = 0, j = 0; while (!i) {for(;j<10;j++) continue; if (i++ < 10) break; } j; }));
  ASSERT(10, ({ int i = 0, j = 0; while (!i) {for(;j<10;j++) continue; if (i++ < 10) break; } j; }));

  ASSERT(2, ({ int i = 0, j = 1; switch (j) { case 0: i = 1; break; case 1: i = 2; break; default: i = 3; break; } i; }));
  ASSERT(7, ({ int i = 0; switch (2+3) { case 0: i = 1; break; case 1: i = 2; break; default: i = 7; break; } i; }));
  ASSERT(3, ({ int i = 0; switch (3) { case 0: i = 1; case 1: i = 2; default: i = 3; } i; }));
  ASSERT(1, ({ int i = 0; switch (-1) { case 0xffffffff: i = 1; break; default: i = 5; } i; }));

  ASSERT(0, ({ int i = 0; switch (0) ; i; }));
  ASSERT(0, ({ int i = 0; switch (1) break; i; }));
  ASSERT(0, ({ int i = 0; switch (1) case 0:; i; }));

  ASSERT(2, ({ int i = 0; for (;i<({1; 5; 2;});i++) { if (i == 3) break; } i; }));

  ASSERT(3, (1, 2, 3));

  /* error
  ASSERT(2, ({ int i=0,j=3; (i=2, j) = 4; i;})); //gcc error
  ASSERT(4, ({ int i=0,j=3; (i=2, j) = 4; j;})); //gcc error
  //ASSERT(0, ({ int a=0; for (;;) {a;} }));
  ASSERT(0, ({ }));
  ASSERT(0, ({ int a[3]; int *p; p = &a;}));
  ASSERT(0, ({ int a[3]; int b[3]; b = a;}));
  ASSERT(0, ({ void a; }));
  //assert(2, ({ {{};; ; 2; }{}}));
  //assert(0, ({; }));
  ASSERT(2, ({return 2;}));
  ASSERT(4, ({ int i = 0; int i = 4; i; })); //compile error

  //ASSERT(0, ({ int i = 0; switch (0) int j = 3; i; })); //gcc error
  //ASSERT(0, ({ int i = 0; switch (1) case 0: break; i; }));
  //ASSERT(0, ({ int i = 0; switch (1) case 0: i = 3; break; i; }));
  ASSERT(0, ({ int i = 0; switch (1) case i: break; i; })); //gcc error
  */

  printf("OK\n");
  return 0;
}