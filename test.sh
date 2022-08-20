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

echo OK
