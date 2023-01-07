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
演算子（Operator)  
被演算子（Operand)  


# 生成文法


```
program = stmt*
stmt = expr ";" |
       "{" stmt* "}" |
       "if" "(" expr ")" stmt ("else" stmt)? |
       "while" "(" expr ")" stmt |
       "for" "(" expr? ";" expr? ";" expr? ";"  ")" stmt |
       "return" expr ";"
expr = assign
assign = equality ("=" assign)?
equality = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add = mul ("+" mul | "-" mul)*
mul = unary ("*" unary | "/" unary)*
unary = ("+" | "-")? primary
primary = num | 
          ident ( "(" ")" )? | 
          "(" expr ")"
```

