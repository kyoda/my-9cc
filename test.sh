#!/bin/bash

cat <<EOF | gcc -xc -c -o tmp2.o -
#include <stdlib.h>
int ret3() { return 3; }
int add2(int a, int b) { return a + b; }
int sub2(int a, int b) { return a - b; }
int add6(int a, int b, int c, int d, int e, int f) { return a + b + c + d + e + f; }
void alloc2(int **p, int a, int b) { int *i = malloc(sizeof(int) * 2); i[0] = a, i[1] = b; *p = i; };
EOF

assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s || exit
  gcc -static -o tmp tmp.s tmp2.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 "int main() { return 0; }"
assert 16 "int main() { return 8 * (4  / 2) +   7 -   6-1; }"
assert 6 "int main() {return ((3 * (4 * (1) / (21-19))));}"
assert 8 "int main() { return -2+5*-8+50; }"
assert 2 "int main() { return 5-3*1; }"
assert 5 "int main() { return 2 - -3; }"
assert 3 "int main() { return  0- -3; }"
assert 3 "int main() { return  - - +3; }"
assert 88 "int main() { return -2+(5*+8+50); }"
assert 1 "int main() { return 3 == 3; }"
assert 1 "int main() { return 3 != 2; }"
assert 1 "int main() { return 3 > 2; }"
assert 1 "int main() { return 3 >= 3; }"
assert 0 "int main() { return 3 < 2; }"
assert 1 "int main() { return 3 <= 3; }"
assert 0 "int main() { return 2 + 3 * 2 / 2 <= 3 == 0 < 1; }"
assert 2 "int main() { 1; 2; return 2; }"
assert 2 "int main() { ;; ; return 2; }"
assert 2 "int main() { {{};; ; return 2; }{}}"

assert 2 "int main() { int a; return a = 2; }"
assert 2 "int main() { int a = 2; return a; }"
assert 10 "int main() { int a = 1; int b = 4*3; return b / a - 2; }"
assert 25 "int main() { int c; int d; c=d=5;return c*d; }"
assert 2 "int main() { int a; int b; int c; int d; return a=b=c=d=2;}"
assert 30 "int main() { int _foo1=5; int _bar2=6;return _foo1*_bar2; }"
assert 2 "int main() { int a=1;int b=2;return  a*b; a=3*8; return 5; }"
assert 0 "int main() { int a; if (5) a =0; return a;}"
assert 4 "int main() { int flag = 0; int a; if (flag) a =0; else a = 4; return a; }"
assert 0 "int main() { int flag = 9; int a; if (flag) a =0; else a = 4; return a;}"
assert 3 "int main() { int i; i=0; while (i<3) {i = i + 1;} return i; }"
assert 3 "int main() { int a = 0; int i; for (i=0; i<3; i=i+1) {a = a + i;} return a; }"
assert 0 "int main() { int a=0; for (;;) {return a;} }"

assert 3 "int main() { return ret3(); }"
assert 2 "int main() { return sub2(5, 3); }"
assert 8 "int main() { return add2(5, 3); }"
assert 66 "int main() { return add6(1, 2, add6(3, 4, 5, 6, 7, 11), 8, 9, 10); }"
assert 10 "int main() { return add3(5, 3, 2); } int add3(int a, int b, int c) { return a + b + c; }"
assert 30 "int main() { return mul3(5, 3, 2); } int mul3(int a, int b, int c) { return a * b * c; }"
assert 2 "int main() { return cal(5, 3, 1); } int cal(int a, int b, int c) { return a - b * c; }"
assert 8 "int main() { return cal(5, 3, 2, 8, 9, 20); } int cal(int a, int b, int c, int d, int e, int f) { return f + e / b - c - a - d; }"

assert 25 "int main() { int i = 20;int *j; j = &i; return *j + 5;}"
assert 5 "int main() { int i;int *p; p = &i; i = 5; return *p;}"
assert 5 "int main() { int i = 7;int *p = &i; *p = 5; return *p;}"
assert 5 "int main() { int x=5; return *&x; }"
assert 5 "int main() { int x=5; int *y=&x; int **z=&y; return **z; }"
assert 7 "int main() { int *p; alloc2(&p, 7, 11); return *p;}"
assert 11 "int main() { int *p; alloc2(&p, 7, 11); return *(p+1);}"


echo OK
