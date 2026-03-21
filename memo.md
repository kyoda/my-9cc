# 🧠 コンパイラ実装に役立つ用語リファレンス

開発中によく忘れる・混乱しやすい単語を記録しています。
x86-64 ABI、アセンブリ記法、型の変換命令など。


+ 自己参照構造体  
+ トークナイザ  
+ 構文木 (syntax tree)  
+ 抽象構文木 (abstract syntax tree, AST)  
+ 生成規則 (production rule)  
+ チョムスキーの生成文法  
+ BNF (Backus-Naur form)  
+ EBNF (Extended BNF)  
+ 終端記号 (terminal symbol)  
+ 非終端記号 (nonterminal symbol)  
+ 文脈自由文法 (context free grammer)  
+ 具象構文木 (concrete syntax tree)  
+ 再帰下降構文解析  
+ 演算子 (Operator)  
+ 被演算子 (Operand)  
+ デリファレンス演算子（dereference operator）
+ ニーモニック（mnemonic）
+ 型指定子（type specifier）-> int, unsigned, etc..
+ 型修飾子（type qualifier）-> const, restrict, etc..
+ ストレージクラス指定子（storage-class specifier） -> static, extern, typedef, etc..
+ 自己参照構造体（Self-referential Structure）
+ 汎用レジスタ（General-Purpose Registers, GPR）

| 分類                                        | キーワード                                                                                                                                     | 役割               | 備考                                |
| ----------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------- | ---------------- | --------------------------------- |
| **型指定子 (type specifier)**                 | `void`, `char`, `short`, `int`, `long`, `float`, `double`, `_Bool`, `_Complex`, `signed`, `unsigned`, `struct`, `union`, `enum`, typedef名 | **型そのものを決める**    | 複数組み合わせ可能（例: `unsigned long int`） |
| **型修飾子 (type qualifier)**                 | `const`, `restrict`, `volatile`, `_Atomic`                                                                                                | **型の性質を修飾する**    | 型に付随する制約や最適化ヒント                   |
| **ストレージクラス指定子 (storage-class specifier)** | `typedef`, `extern`, `static`, `_Thread_local`, `auto`, `register`                                                                        | **記憶域やリンケージを指定** | `typedef`だけ意味が特殊                  |




## 生成文法

BNFの一部を記載しています。

```
  string-initializer ::= string-literal


  array-initializer1 ::= "{" initializer ("," initializer)* "}"


  array-initializer2 ::= initializer ("," initializer)*


  struct-initializer1 ::= "{" initializer ("," initializer)* "}"


  struct-initializer2 ::= initializer ("," initializer)*


  union-initializer ::= "{" initializer ("," dumy-initializer)* "}"
                      | initializer


  initializer ::= string-initializer
                | array-initializer
                | struct-initializer
                | union-initializer
                | assign


  function ::= declspec declarator ( stmt? | ";")


  parse_typedef ::= declarator ";"


  program ::= (declspec (parse_typedef | function | global_variable))*


  struct-member ::= (declspec declarator ("," declarator)* ";")*


  struct-union-decl ::= "struct" ident? ("{" struct-member "}")?


  struct-decl ::= struct-union-decl


  union-decl ::= struct-union-decl


  enum-specifier ::= "enum" ident? "{" enum-list? "}"
                   | "enum" ident ("{" enum-list? "}")?
  enmu-list ::= ident ("=" num)? ("," ident ("=" num)?)* 


  declspec ::= ("_Bool" | "void" | "char" | "short" | "int" | "long"
             | "struct-decl" | "union-decl" | "enum-specifier" | "_Alignas"
             | "typedef" | "static" | "extern" | typedef-name)+


  declaration ::= (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"


  declarator ::= "*"* ( "(" declarator ")" | ident ) type-suffix


  type-suffix ::= "(" func-params ")"
                | "[" num? "]" type-suffix
                | ε


  func-params ::= "(" "void" | (declspec declarator ("," declspec declarator)*)? ")"


  compound-stmt ::= declspec (parse_typedef | function | extern | global_variable)*


  stmt ::= compound-stmt
         | "return" expr? ";"
         | "if" "(" expr ")" stmt ("else" stmt)?
         | "while" "(" expr ")" stmt
         | "do" stmt "while" "(" expr ")" ";"
         | "for" "(" expr? ";" expr? ";" expr? ";"  ")" stmt
         | "switch" "(" expr ")" stmt
         | "case" num ":" stmt
         | "default" ":" stmt
         | "goto" ident ";"
         | "break" ";"
         | "continue" ";"
         | ident ":" stmt
         | expr-stmt


  expr-stmt ::= ";" | expr ";"


  expr ::= assign ("," expr)*


  assign ::= bitor (assign-op assign)?
  assign-op ::= "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "|=" | "^=" | "&="


  conditional ::= logicalor ("?" expr ":" conditional)?


  logicalor ::= logicaland ("||" logicaland)*


  logicaland ::= bitor ("&&" bitor)*


  bitor ::= bitxor ("|" bitxor)*


  bitxor ::= bitand ("^" bitand)*


  bitand ::= equality ("&" equality)*


  equality ::= relational ("==" relational | "!=" relational)*


  relational ::= shift ("<" shift | "<=" shift | ">" shift | ">=" shift)*


  shift ::= add ("<<" add | ">>" add)*


  add ::= mul ("+" new_add | "-" new_sub)*


  mul ::= cast ("*" cast | "/" cast | "%" cast)*


  cast ::= "(" typename ")" cast
         | unary


  abstract-declarator ::= "*"* ("(" abstract-declarator ")")? type-suffix


  typename ::= declspec abstract-declarator


  unary ::= "sizeof" "(" typename ")"
          | "sizeof" cast
          | "_Alignof" "(" typename ")"
          | "_Alignof" cast
          | ("+" | "-" | "*" | "&" | "!") cast
          | ("++" | "--") unary
          | postfix


  postfix ::= "(" type-name ")" "{" initializer-list "}"
            | primary ("[" expr "]" | "." ident | "->" ident | "++" | "--")*


  primary ::= "(" "{" stmt+ "}" ")"
            | "(" expr ")"
            | ident ( "(" assign "," ")" )?
            | str
            | num
```


| 記号       | 意味           | 備考             |
|------------|----------------|------------------|
| `num`      | 数字           | 0〜9の連続       |
| `ident`    | 変数、関数名    | 英字＋数字など   |
| `declspec` | 型             | int, char など   |
| `str`      | 文字列リテラル   | `"abc"` など   



## 🗂 スタック操作に関するアセンブリ命令メモ

スタックは **LIFO（Last-In First-Out）** 構造で、関数呼び出し時の戻り先アドレスやローカル変数などの一時的な値を保持するのに使われます。以下は、スタック操作に関する基本的な命令と、その内部的な動作に相当するアセンブリ命令の対応です。

---

## 🗂 スタック操作に関するアセンブリ命令まとめ

スタックは LIFO（Last-In First-Out）構造で、関数呼び出しや戻りアドレス、一時的なレジスタ退避などに使用されます。以下は代表的なスタック関連命令と、それに等価なアセンブリ操作をまとめた表です。

| 命令       | 意味                                         | 等価なアセンブリ操作                                                                 |
|------------|----------------------------------------------|----------------------------------------------------------------------------------------|
| `push rax` | RAXの値をスタックに積む                      | `sub rsp, 8`<br>`mov [rsp], rax`<br>※ `rsp -= 8` のあと `[rsp] ← rax`                 |
| `pop rax`  | スタックトップの値をRAXに取り出す            | `mov rax, [rsp]`<br>`add rsp, 8`<br>※ `[rsp] → rax` のあと `rsp += 8`                |
| `call foo` | 戻りアドレスを積んで関数fooにジャンプ       | `sub rsp, 8`<br>`mov [rsp], return_address`<br>`jmp foo`<br>※ 戻り先を積んでジャンプ |
| `ret`      | スタックトップのアドレスにジャンプ（復帰）  | `mov rip, [rsp]`<br>`add rsp, 8`<br>`jmp rip`<br>※ `[rsp] → rip` のあと `rsp += 8`   |

> 🔎 `call` の return_address は call 命令の「次の命令のアドレス」です。


> 💡 **補足：スタックは上から下に伸びる**

x86-64 アーキテクチャでは、スタックはメモリ空間上で「上から下（高アドレスから低アドレス）」に向かって伸びます。  
これはつまり、`push` 命令を使うたびに `rsp`（スタックポインタ）が **小さくなる**ということです。

### 📉 例：

初期状態 rsp = 0x7fffffffe000

push rax 実行 rsp = 0x7fffffffdff8
↑ スタックが 8バイト「下」に伸びた


この構造により、スタックは **高アドレスから低アドレスへと「下方向」に成長**していきます。

- `push` でアドレスが減る（=下に積む）
- `pop` でアドレスが増える（=上に戻す）

したがって、「上から下に伸びる」という表現は、**アドレスの増減方向**を指しており、メモリレイアウトを理解する際に非常に重要です。




## movsx（符号付き拡張）命令：Intel記法 vs AT&T記法


| 拡張元 → 拡張先     | Intel 記法         | AT\&T 記法            | 説明              |
| ------------- | ---------------- | ------------------- | --------------- |
| 8bit → 16bit  | `movsx ax, al`   | `movsbw %al, %ax`   | ALを符号付きでAXに拡張   |
| 8bit → 32bit  | `movsx eax, al`  | `movsbl %al, %eax`  | ALを符号付きでEAXに拡張  |
| 8bit → 64bit  | `movsx rax, al`  | `movsbq %al, %rax`  | ALを符号付きでRAXに拡張  |
| 16bit → 32bit | `movsx eax, ax`  | `movswl %ax, %eax`  | AXを符号付きでEAXに拡張  |
| 16bit → 64bit | `movsx rax, ax`  | `movswq %ax, %rax`  | AXを符号付きでRAXに拡張  |
| 32bit → 64bit | `movsx rax, eax` | `movslq %eax, %rax` | EAXを符号付きでRAXに拡張 |


## movzx（ゼロ拡張）命令：Intel記法 vs AT&T記法

| 拡張元 → 拡張先     | Intel 記法        | AT\&T 記法           | 説明              |
| ------------- | --------------- | ------------------ | --------------- |
| 8bit → 16bit  | `movzx ax, al`  | `movzbw %al, %ax`  | ALをゼロ拡張してAXに拡張  |
| 8bit → 32bit  | `movzx eax, al` | `movzbl %al, %eax` | ALをゼロ拡張してEAXに拡張 |
| 8bit → 64bit  | `movzx rax, al` | `movzbq %al, %rax` | ALをゼロ拡張してRAXに拡張 |
| 16bit → 32bit | `movzx eax, ax` | `movzwl %ax, %eax` | AXをゼロ拡張してEAXに拡張 |
| 16bit → 64bit | `movzx rax, ax` | `movzwq %ax, %rax` | AXをゼロ拡張してRAXに拡張 |


### AT&T記法の接尾辞まとめ

| 接尾辞 | 意味           |
| --- | ------------ |
| `b` | byte (8bit)  |
| `w` | word (16bit) |
| `l` | long (32bit) |
| `q` | quad (64bit) |

## サイズ一覧（Intel記法）

メモリアクセスの際にサイズがわからないため指定する

| 指定子         | サイズ   | C型の対応（x86_64）  |
| ----------- | ----- | -------------- |
| `byte ptr`  | 8bit  | char           |
| `word ptr`  | 16bit | short          |
| `dword ptr` | 32bit | int            |
| `qword ptr` | 64bit | long / pointer |

例: mov rdi, QWORD PTR [rax + 0]


## 操作例

| 命令                          | 意味                           |
| --------------------------- | ---------------------------- |
| `movsx eax, al`             | ALレジスタの値を EAX に符号付き拡張する      |
| `movsx eax, byte ptr [rax]` | RAXが指すメモリの1バイトを読み、EAXに符号付き拡張 |
