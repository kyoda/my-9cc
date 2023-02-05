#!/bin/bash

cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int add(int a, int b) { return a + b; }
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

assert 0 "main() { return 0; }"
assert 16 "main() { return 8 * (4  / 2) +   7 -   6-1; }"
assert 6 "main() {return ((3 * (4 * (1) / (21-19))));}"
assert 8 "main() { return -2+5*-8+50; }"
assert 88 "main() { return -2+(5*+8+50); }"
assert 1 "main() { return 3 == 3; }"
assert 1 "main() { return 3 != 2; }"
assert 1 "main() { return 3 > 2; }"
assert 1 "main() { return 3 >= 3; }"
assert 0 "main() { return 3 < 2; }"
assert 1 "main() { return 3 <= 3; }"
assert 0 "main() { return 2 + 3 * 2 / 2 <= 3 == 0 < 1; }"
assert 2 "main() { return a = 2; }"
assert 10 "main() { a = 1; b = 4 * 3; return b / a - 2; }"
assert 25 "main() { c=d=5;return c*d; }"
assert 2 "main() { return a=b=c=d=2;}"
assert 30 "main() { _foo1=5;_bar2=6;return _foo1*_bar2; }"
assert 2 "main() { a=1; b=2;return  a*b; a=3*8; return 5; }"
assert 0 "main() { if (5) a =0; return a;}"
assert 4 "main() { flag = 0; if (flag) a =0; else a = 4; return a; }"
assert 0 "main() { flag = 9; if (flag) a =0; else a = 4; return a;}"
assert 3 "main() { i=0; while (i<3) {i = i + 1;} return i; }"
assert 3 "main() { a = 0; for (i=0; i<3; i=i+1) {a = a + i;} return a; }"
assert 0 "main() { a = 0; for (;;) {return a;} }"
assert 3 "main() { return ret3(); }"
assert 3 "main() { return add(1, 2); }"

echo OK
