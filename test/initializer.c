#include "test.h"

int x;
char gchar1 = 1;
short gshort2 = 2;
int gint3 = 3;
long glong4 = 4;
int gint[3] = {5, 6, 7};
char gchar[] = "Hello World";

int main() {
  ASSERT(0, 0);
  ASSERT(0, ({ char a[0]; 0; }));
  ASSERT(0, ({ int a[0] = {}; 0; }));
  //ASSERT(0, ({ int a[] = {}; 0; })); gcc ok
  ASSERT(1, ({ int a[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}; a[0][0]; }));
  ASSERT(2, ({ int a[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}; a[0][1]; }));
  ASSERT(3, ({ int a[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}; a[0][2]; }));

  ASSERT(4, ({ int a[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}; a[1][0]; }));
  ASSERT(5, ({ int a[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}; a[1][1]; }));
  ASSERT(6, ({ int a[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}; a[1][2]; }));

  ASSERT(7, ({ int a[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}; a[2][0]; }));
  ASSERT(8, ({ int a[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}; a[2][1]; }));
  ASSERT(9, ({ int a[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}; a[2][2]; }));

  ASSERT(0, ({ int a[2] = {}; a[0]; }));
  ASSERT(0, ({ int a[2] = {1}; a[1]; }));
  ASSERT(0, ({ char a[3] = {}; a[0]; }));
  ASSERT(0, ({ char a[3] = {}; a[1]; }));
  ASSERT(0, ({ char a[3] = {}; a[2]; }));
  ASSERT(3, ({ int a[3][3] = {{1, 2, 3}, {4, 5, 6}}; a[0][2]; }));
  ASSERT(0, ({ int a[3][3] = {{1, 2, 3}, {4, 5, 6}}; a[2][2]; }));

  //過剰な要素はスキップ
  ASSERT(1, ({ char a[1] = {1, 2, 3}; a[0]; }));
  ASSERT(5, ({ int b = 5; char a[1] = {1, 2, b = 3, 4}; b; }));
  ASSERT(5, ({ int b = 5; char a[1] = {1, 2, ++b, 4}; b; }));
  ASSERT(6, ({ x = 6; char a[1] = {1, 2, x = 3, 4}; x; }));
  ASSERT(3, ({ int b = 5; char a[1] = {b = 3, 4}; a[0]; }));
  ASSERT(3, ({ int b = 5; char a[1] = {b = 3, 4}; b; }));

  ASSERT('a', ({ char a[4] = "abc"; a[0]; }));
  ASSERT('b', ({ char a[4] = "abc"; a[1]; }));
  ASSERT('c', ({ char a[4] = "abc"; a[2]; }));
  ASSERT(0, ({ char a[4] = "abc"; a[3]; }));
  ASSERT('b', ({ char a[4][4] = {"abc", "def"}; a[0][1]; }));
  ASSERT('e', ({ char a[4][4] = {"abc", "def"}; a[1][1]; }));

  ASSERT(2, ({ int a[] = {1, 2}; a[1]; }));
  ASSERT(8, ({ int a[] = {1, 2}; sizeof(a); }));
  ASSERT(3, ({ char a[] = {1, 2, 3}; sizeof(a); }));
  ASSERT(5, ({ char a[] = "abcd"; sizeof(a); }));
  ASSERT(8, ({ char *a = "abcd"; sizeof(a); }));
  ASSERT(12, ({ char a[][6] = {"abc", "defgh"}; sizeof(a); }));
  ASSERT(6, ({ typedef char T[]; T a = "hello"; sizeof(a); }));

  ASSERT(0, ({ struct {int a; int b; int c; } x = {0, 1, 2}; x.a; }));
  ASSERT(1, ({ struct {int a; int b; int c; } x = {0, 1, 2}; x.b; }));
  ASSERT(2, ({ struct {int a; int b; int c; } x = {0, 1, 2}; x.c; }));
  ASSERT(3, ({ struct {int a; int b; int c; } x = {3}; x.a; }));
  ASSERT(0, ({ struct {int a; int b; int c; } x = {3}; x.b; }));
  ASSERT(0, ({ struct {int a; int b; int c; } x = {3}; x.c; }));
  ASSERT(1, ({ struct {int a; int b; } x[2] = {{1, 2}, {3, 4}}; x[0].a; }));
  ASSERT(2, ({ struct {int a; int b; } x[2] = {{1, 2}, {3, 4}}; x[0].b; }));
  ASSERT(3, ({ struct {int a; int b; } x[2] = {{1, 2}, {3, 4}}; x[1].a; }));
  ASSERT(4, ({ struct {int a; int b; } x[2] = {{1, 2}, {3, 4}}; x[1].b; }));
  ASSERT(0, ({ struct {int a; int b; } x[2] = {{3, 4}}; x[1].b; }));
  ASSERT(4, ({ struct {int a; int b; } x[2] = {{3, 4}}; x[0].b; }));
  ASSERT(0, ({ struct {int a; int b; } x[2] = {}; x[0].a; }));
  ASSERT(0, ({ struct {int a; int b; } x[2] = {}; x[0].b; }));
  ASSERT(4, ({ typedef struct {int a, b, c, d, e, f; } T; T x = {1, 2, 3, 4, 5, 6}; x.d; }));
  ASSERT(0, ({ typedef struct {int a, b, c, d, e, f; } T; T x = {1, 2, 3, 4, 5}; T y; T z; z = y = x; z.f;}));
  ASSERT(0, ({ typedef struct {int a, b, c, d, e, f; } T; T x = {1, 2, 3, 4, 5}; T y; T z = y = x; z.f;}));

  ASSERT(0x56, ({ union {int a; char b[5]; } x = { 0x123456 }; x.b[0]; }));
  ASSERT(0x34, ({ union {int a; char b[5]; } x = { 0x123456 }; x.b[1]; }));
  ASSERT(0x12, ({ union {int a; char b[5]; } x = { 0x123456 }; x.b[2]; }));
  ASSERT(0, ({ union {int a; char b[5]; } x = { 0x123456 }; x.b[3]; }));
  ASSERT(0, ({ union {int a; char b[5]; } x = { 0x123456 }; x.b[4]; }));
  ASSERT(0x123456, ({ union {int a; char b[5]; } x = { 0x123456 }; x.a; }));
  ASSERT(0x56, ({ union {int a; char b[5]; } x = { 0x123456, 0x12345678 }; x.b[0]; }));
  ASSERT(0x0, ({ union {int a; char b[4]; } x = { 0x123456, 0x12345678 }; x.b[3]; }));
  ASSERT(0x78563412, ({ union { struct { char a, b, c, d; } e; int f; } x = { { 0x12, 0x34, 0x56, 0x78 } }; x.f; }));
  ASSERT(0x56, ({ typedef union {int a; char b[5]; } T; T x = { 0x123456 }; T y = x; y.b[0]; }));

  ASSERT(1, gchar1);
  ASSERT(2, gshort2);
  ASSERT(3, gint3);
  ASSERT(4, glong4);
  ASSERT(5, gint[0]);
  ASSERT(6, gint[1]);
  ASSERT(7, gint[2]);
  ASSERT('l', gchar[2]);
  ASSERT('W', gchar[6]);

  printf("OK\n");
  return 0;
}
