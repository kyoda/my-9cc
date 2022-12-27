#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 "0;"
assert 42 "42;"
assert 13 "7 +   6;"
assert 7 "8 - 1;"
assert 21 '5+20-4;'
assert 41 " 12 + 34 - 5;"
assert 42 " 6 * 7;"
assert 9 " 27 / 3;"
assert 9 "3 * (4 - 1);"
assert 6 "((3 * (4 * (1) / (21-19))));"
assert 8 "-2+5*-8+50;"
assert 88 "-2+(5*+8+50);"
assert 1 "3 == 3;"
assert 1 "3 != 2;"
assert 1 "3 > 2;"
assert 1 "3 >= 3;"
assert 0 "3 < 2;"
assert 1 "3 <= 3;"
assert 0 "2 + 3 * 2 / 2 <= 3 == 0 < 1;"
assert 2 "a = 2;"
assert 10 "a = 1; b = 4 * 3; b / a - 2;"
assert 25 "c=d=5;c*d;"
assert 2 "a=b=c=d=2;"
assert 30 "_foo1=5;_bar2=6;_foo1*_bar2;"
assert 2 "a=1; b=2;return  a*b; a=3*8; return 5;"
assert 0 "if (5) a =0;"
assert 4 "flag = 0; if (flag) a =0; else b = 4;"
assert 0 "flag = 9; if (flag) a =0; else b = 4;"
assert 3 "i=0; while (i<3) {i = i + 1; return i;}"
#assert 3 "a = 0; for (i=0; i<3; i=i+1) a = i;"


echo OK
