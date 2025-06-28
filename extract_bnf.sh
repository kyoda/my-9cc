#!/bin/bash

# 使用方法:
# ./extract_bnf.sh source.c > bnf_list.txt

awk '
BEGIN {
  in_block = 0
  bnf = ""
}

/\/\*/ {
  in_block = 1
  bnf = ""
}

in_block {
  bnf = bnf $0 "\n"
}

/\*\// {
  in_block = 0
  if (bnf ~ /::=/) {
    # コメント記号と余計な空白・アスタリスクを除去
    gsub(/\/\*|\*\//, "", bnf)
    gsub(/\n[ \t]*\*/, "\n", bnf)  # 行頭の *
    gsub(/^[ \t]+/, "", bnf)       # 各行先頭の空白
    gsub(/[ \t]+$/, "", bnf)       # 各行末尾の空白
    printf "%s", bnf               # 空行は出力しない
  }
}
' "$1"
