# 🧠 コンパイラ実装に役立つ用語リファレンス

開発中によく忘れる・混乱しやすい単語を記録しています。
x86-64 ABI、アセンブリ記法、型の変換命令など。

| 分類 | 用語 | 意味 / 役割 | 例（単語が指す記号・対応物） | 備考 |
|------|------|------------|----------------------------|------|
| ABI / OS | System V (System Five) | UNIX系OSおよびその系統の標準仕様群 | System V ABI, System V IPC | V はローマ数字の5（Five） |
| ABI / 呼び出し規約 | ABI (Application Binary Interface) | バイナリレベルでの関数呼び出し・レジスタ使用・データ配置などの取り決め | x86-64 System V ABI: rdi,rsi,rdx,rcx,r8,r9 に引数を渡す | コンパイラ・OS・ライブラリ間の互換性を保証 |
| データ構造 | 自己参照構造体 (Self-referential Structure) | 自分自身の型へのポインタを持つ構造体 | `struct Node { struct Node *next; };` | 連結リスト・木で使用 |
| 字句解析 | トークナイザ (Tokenizer) | ソースコード文字列をトークン列に分解 | `"a + 3"` → `IDENT(a), '+', NUM(3)` | lexer |
| 構文解析 | 構文木 (Syntax Tree) | プログラム構造を木で表現したもの | `(1 + 2) * 3` の木 | 広義（CST/AST含む） |
| 構文解析 | 具象構文木 (Concrete Syntax Tree, CST) | 文法をそのまま反映した構文木 | 括弧や文法ノードも保持 | パース結果そのまま |
| 構文解析 | 抽象構文木 (Abstract Syntax Tree, AST) | 不要な文法情報を省いた構文木 | `(+ 1 (* 2 3))` | コンパイラ内部で使う |
| 文法 | 生成規則 (Production Rule) | 記号を別の記号列に展開するルール | `Expr → Expr "+" Term` | 文法の基本単位 |
| 文法 | チョムスキーの生成文法 (Chomsky Hierarchy) | 文法を計算能力で分類する体系 | Type0（制限なし）〜Type3（正規文法） | 下に行くほど制約が強い |
| 文法 | 文脈自由文法 (Context-Free Grammar, CFG) | 左辺が単一の非終端記号である文法 | `Expr → Expr "+" Term` | 多くのプログラミング言語で使用 |
| 文法 | BNF (Backus-Naur Form) | 文法（特にCFG）を記述する記法 | `<expr> ::= <expr> "+" <term>` | シンプルな記法 |
| 文法 | EBNF (Extended BNF) | BNFを拡張した記法 | `<expr> = <term> { "+" <term> }` | `{}` や `[]` が使える |
| 文法 | 終端記号 (Terminal Symbol) | それ以上展開されない記号（トークン） | `"+"`, `"if"`, `"("` | lexerの出力に対応 |
| 文法 | 非終端記号 (Nonterminal Symbol) | 他の記号列に展開される記号 | `<expr>`, `<stmt>` | ASTノードに対応 |
| 構文解析 | 再帰下降構文解析 (Recursive Descent Parsing) | 関数の再帰呼び出しで文法を処理する手法 | `parse_expr()` | LL系パーサ |
| 式 | 演算子 (Operator) | 計算や操作を表す記号 | `+`, `-`, `*`, `/`, `* (deref)`, `& (reference)` | 優先順位あり |
| 式 | 被演算子 (Operand) | 演算の対象となる値や式 | `a`, `3`, `x+1` | ASTの葉 |
| 式 | デリファレンス演算子 (Dereference Operator) | ポインタの指す先の値を取得 | `*p` | 単項演算子 |
| 式 | 参照演算子 / アドレス演算子 (Reference / Address-of Operator) | 変数のアドレス（参照）を取得 | `&x` | lvalue → pointer |
| アセンブリ | ニーモニック (Mnemonic) | 機械語命令の人間可読な表現 | `mov`, `add`, `sub` | x86_64 |
| 型 | 型指定子 (Type Specifier) | 型そのものを決定するキーワード | `int`, `char`, `struct` | 複数指定可能 |
| 型 | 型修飾子 (Type Qualifier) | 型の性質を修飾するキーワード | `const`, `volatile` | 最適化や制約 |
| 型 | ストレージクラス指定子 (Storage-Class Specifier) | 変数の寿命やリンケージを指定 | `static`, `extern` | `typedef`は別用途 |
| CPU | 汎用レジスタ (General-Purpose Register, GPR) | 汎用的に使用されるCPUレジスタ | `rax`, `rbx`, `rcx`, `rdx` | x86_64 |

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

| 記号         | 意味      | 備考           |
| ---------- | ------- | ------------ |
| `num`      | 数字      | 0〜9の連続       |
| `ident`    | 変数、関数名  | 英字＋数字など      |
| `declspec` | 型       | int, char など |
| `str`      | 文字列リテラル | `"abc"` など   |

## 🗂 スタック操作に関するアセンブリ命令メモ

スタックは **LIFO（Last-In First-Out）** 構造で、関数呼び出し時の戻り先アドレスやローカル変数などの一時的な値を保持するのに使われます。以下は、スタック操作に関する基本的な命令と、その内部的な動作に相当するアセンブリ命令の対応です。

***

## 🗂 スタック操作に関するアセンブリ命令まとめ

スタックは LIFO（Last-In First-Out）構造で、関数呼び出しや戻りアドレス、一時的なレジスタ退避などに使用されます。以下は代表的なスタック関連命令と、それに等価なアセンブリ操作をまとめた表です。

| 命令         | 意味                    | 等価なアセンブリ操作                                                      |
| ---------- | --------------------- | --------------------------------------------------------------- |
| `push rax` | RAXの値をスタックに積む         | `sub rsp, 8mov [rsp], rax`※ `rsp -= 8` のあと `[rsp] ← rax`        |
| `pop rax`  | スタックトップの値をRAXに取り出す    | `mov rax, [rsp]add rsp, 8`※ `[rsp] → rax` のあと `rsp += 8`        |
| `call foo` | 戻りアドレスを積んで関数fooにジャンプ  | `sub rsp, 8mov [rsp], return_addressjmp foo`※ 戻り先を積んでジャンプ       |
| `ret`      | スタックトップのアドレスにジャンプ（復帰） | `mov rip, [rsp]add rsp, 8jmp rip`※ `[rsp] → rip` のあと `rsp += 8` |

> 🔎 `call` の return\_address は call 命令の「次の命令のアドレス」です。

> 💡 **補足：スタックは上から下に伸びる**

x86-64 アーキテクチャでは、スタックはメモリ空間上で「上から下（高アドレスから低アドレス）」に向かって伸びます。\
これはつまり、`push` 命令を使うたびに `rsp`（スタックポインタ）が **小さくなる**ということです。

### 📉 例：

初期状態 rsp = 0x7fffffffe000

push rax 実行 rsp = 0x7fffffffdff8
↑ スタックが 8バイト「下」に伸びた

この構造により、スタックは **高アドレスから低アドレスへと「下方向」に成長**していきます。

* `push` でアドレスが減る（=下に積む）

* `pop` でアドレスが増える（=上に戻す）

したがって、「上から下に伸びる」という表現は、**アドレスの増減方向**を指しており、メモリレイアウトを理解する際に非常に重要です。

## movsx（符号付き拡張）命令：Intel記法 vs AT\&T記法

| 拡張元 → 拡張先     | Intel 記法         | AT\&T 記法            | 説明              |
| ------------- | ---------------- | ------------------- | --------------- |
| 8bit → 16bit  | `movsx ax, al`   | `movsbw %al, %ax`   | ALを符号付きでAXに拡張   |
| 8bit → 32bit  | `movsx eax, al`  | `movsbl %al, %eax`  | ALを符号付きでEAXに拡張  |
| 8bit → 64bit  | `movsx rax, al`  | `movsbq %al, %rax`  | ALを符号付きでRAXに拡張  |
| 16bit → 32bit | `movsx eax, ax`  | `movswl %ax, %eax`  | AXを符号付きでEAXに拡張  |
| 16bit → 64bit | `movsx rax, ax`  | `movswq %ax, %rax`  | AXを符号付きでRAXに拡張  |
| 32bit → 64bit | `movsx rax, eax` | `movslq %eax, %rax` | EAXを符号付きでRAXに拡張 |

## movzx（ゼロ拡張）命令：Intel記法 vs AT\&T記法

| 拡張元 → 拡張先     | Intel 記法        | AT\&T 記法           | 説明              |
| ------------- | --------------- | ------------------ | --------------- |
| 8bit → 16bit  | `movzx ax, al`  | `movzbw %al, %ax`  | ALをゼロ拡張してAXに拡張  |
| 8bit → 32bit  | `movzx eax, al` | `movzbl %al, %eax` | ALをゼロ拡張してEAXに拡張 |
| 8bit → 64bit  | `movzx rax, al` | `movzbq %al, %rax` | ALをゼロ拡張してRAXに拡張 |
| 16bit → 32bit | `movzx eax, ax` | `movzwl %ax, %eax` | AXをゼロ拡張してEAXに拡張 |
| 16bit → 64bit | `movzx rax, ax` | `movzwq %ax, %rax` | AXをゼロ拡張してRAXに拡張 |

### AT\&T記法の接尾辞まとめ

| 接尾辞 | 意味           |
| --- | ------------ |
| `b` | byte (8bit)  |
| `w` | word (16bit) |
| `l` | long (32bit) |
| `q` | quad (64bit) |

## サイズ一覧（Intel記法）

メモリアクセスの際にサイズがわからないため指定する

| 指定子         | サイズ   | C型の対応（x86\_64） |
| ----------- | ----- | -------------- |
| `byte ptr`  | 8bit  | char           |
| `word ptr`  | 16bit | short          |
| `dword ptr` | 32bit | int            |
| `qword ptr` | 64bit | long / pointer |

例: mov rdi, QWORD PTR \[rax + 0]

## 操作例

| 命令                          | 意味                          |
| --------------------------- | --------------------------- |
| `movsx eax, al`             | ALレジスタの値を EAX に符号付き拡張する     |
| `movsx eax, byte ptr [rax]` | RAXが指すメモリの1バイトを読み、EAXに符号付き拡 |

## cast


### 🔹 符号拡張（signed → larger）

| 元の型 | 変換先 | 命令                | 説明                  |
| --- | --- | ----------------- | ------------------- |
| i8  | i32 | `movsx eax, al`   | 8bit → 32bit        |
| i16 | i32 | `movsx eax, ax`   | 16bit → 32bit       |
| i32 | i64 | `movsxd rax, eax` | 32bit → 64bit（専用命令） |
| i8  | i64 | `movsx rax, al`   | 8bit → 64bit        |
| i16 | i64 | `movsx rax, ax`   | 16bit → 64bit       |

### 🔹 ゼロ拡張（unsigned → larger）

| 元の型 | 変換先 | 命令              | 説明             |
| --- | --- | --------------- | -------------- |
| u8  | u32 | `movzx eax, al` | 8bit → 32bit   |
| u16 | u32 | `movzx eax, ax` | 16bit → 32bit  |
| u8  | u64 | `movzx rax, al` | 8bit → 64bit   |
| u16 | u64 | `movzx rax, ax` | 16bit → 64bit  |
| u32 | u64 | `mov eax, eax`  | 32bit書き込みで上位ゼロ |

### 🔹 トランケーション（縮小）

| 元の型     | 変換先     | 命令   | 説明            |
| ------- | ------- | ---- | ------------- |
| i32/u32 | i8/u8   | （不要） | 下位8bitだけ使われる  |
| i32/u32 | i16/u16 | （不要） | 下位16bitだけ使われる |
| i64/u64 | i32/u32 | （不要） | 下位32bitだけ使われる |

### 🔹 同サイズ変換

| 変換        | 命令 | 説明       |
| --------- | -- | -------- |
| i32 ↔ u32 | なし | ビット列そのまま |
| i64 ↔ u64 | なし | ビット列そのまま |
| i8 ↔ u8   | なし | ビット列そのまま |
| i16 ↔ u16 | なし | ビット列そのまま |
