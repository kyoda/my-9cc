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

assert 0 0
assert 42 42
assert 13 '7 +   6'
assert 7 '8 - 1'
assert 21 '5+20-4'
assert 41 " 12 + 34 - 5"
assert 42 " 6 * 7"
assert 9 " 27 / 3"
assert 9 "3 * (4 - 1)"
assert 6 "((3 * (4 * (1) / (21-19))))"
assert 8 "-2+5*-8+50"
assert 88 "-2+(5*+8+50)"
assert 1 "3 == 3"
assert 1 "3 != 2"
assert 1 "3 > 2"
assert 1 "3 >= 3"
assert 0 "3 < 2"
assert 1 "3 <= 3"
assert 0 "2 + 3 * 2 / 2 <= 3 == 0 < 1"

echo OK
