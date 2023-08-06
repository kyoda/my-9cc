#!/bin/bash

check () {
    if [ $? -eq 0 ]; then
        echo "testing $1: passed"
    else
        echo "testing $1: failed"
        exit 1
    fi
}

tmp=$(mktemp -d /tmp/9cc-test-XXXXXXXXXX)
trap 'rm -rf $tmp' INT TERM HUP EXIT
echo > $tmp/empty.c


# -o
rm -f $tmp/tmp.s
./9cc -o $tmp/tmp.s $tmp/empty.c
[ -f $tmp/tmp.s ]
check "-o"

# -h
./9cc -h 2>&1 | grep -q '9cc'
check "-h"

# --help
./9cc --help 2>&1 | grep -q '9cc'
check "--help"