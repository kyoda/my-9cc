#!/bin/bash

cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
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

assert 0 "{ return 0; }"
assert 16 "{ return 8 * (4  / 2) +   7 -   6-1; }"
assert 6 "{return ((3 * (4 * (1) / (21-19))));}"
assert 8 "{ return -2+5*-8+50; }"
assert 88 "{ return -2+(5*+8+50); }"
assert 1 "{ return 3 == 3; }"
assert 1 "{ return 3 != 2; }"
assert 1 "{ return 3 > 2; }"
assert 1 "{ return 3 >= 3; }"
assert 0 "{ return 3 < 2; }"
assert 1 "{ return 3 <= 3; }"
assert 0 "{ return 2 + 3 * 2 / 2 <= 3 == 0 < 1; }"
assert 2 "{ return a = 2; }"
assert 10 "{ a = 1; b = 4 * 3; return b / a - 2; }"
assert 25 "{ c=d=5;return c*d; }"
assert 2 "{ return a=b=c=d=2;}"
assert 30 "{ _foo1=5;_bar2=6;return _foo1*_bar2; }"
assert 2 "{ a=1; b=2;return  a*b; a=3*8; return 5; }"
assert 0 "{ if (5) a =0; return a;}"
assert 4 "{ flag = 0; if (flag) a =0; else a = 4; return a; }"
assert 0 "{ flag = 9; if (flag) a =0; else a = 4; return a;}"
assert 3 "{ i=0; while (i<3) {i = i + 1;} return i; }"
assert 3 "{ a = 0; for (i=0; i<3; i=i+1) {a = a + i;} return a; }"
assert 0 "{ a = 0; for (;;) {return a;} }"
assert 3 "{ return ret3(); }"
assert 3 "{ return ret3(1, 2); }"

echo OK
