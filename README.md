# memo

自己参照構造体  
トークナイザ  
構文木 (syntax tree)  
抽象構文木 (abstract syntax tree, AST)  
生成規則 (production rule)  
チョムスキーの生成文法  
BNF (Backus-Naur form)  
EBNF (Extended BNF)  
終端記号 (terminal symbol)  
非終端記号 (nonterminal symbol)  
文脈自由文法 (context free grammer)  
具象構文木 (concrete syntax tree)  
再帰下降構文解析  
演算子 (Operator)  
被演算子 (Operand)  
デリファレンス演算子（dereference operator）


# 生成文法


```
program = (declspec) stmt*
stmt = expr? ";" |
       "{" stmt* "}" |
       "if" "(" expr ")" stmt ("else" stmt)? |
       "while" "(" expr ")" stmt |
       "for" "(" expr? ";" expr? ";" expr? ";"  ")" stmt |
       "return" expr ";" |
        declspec "*"* ident ("=" assign)? ";"
expr = assign
assign = equality ("=" assign)?
equality = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add = mul ("+" mul | "-" mul)*
mul = unary ("*" unary | "/" unary)*
unary = ("+" | "-")? primary |
        "*" unary |
        "&" unary
primary = num | 
          ident ( "(" assign "," ")" )? | 
          "(" expr ")"
```

```
num -> 数字
ident -> 変数, 関数名
declspec -> 型
```

```
declaration = declspec ident ";"

compound-stmt = (declaration | stmt)* "}"
declspec = "int"
declarator = "*"* ident
declaration = declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
```

# stack


```
pop rax

mov rax, [rsp]
add rsp, 8
```


```
push rax

sub rsp, 8
mov [rsp], rax
```

```
call

callの次の命令のアドレスをスタックにプッシュ
callの引数として与えられたアドレスにジャンプ
```

```
ret
スタックからアドレスを１つポップ
そのアドレスにジャンプ
```

