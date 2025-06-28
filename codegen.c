#include "9cc.h"

static Obj *current_fn;
static FILE *out;
static int depth;
static int current_line;
static void gen_stmt(Node *n);
static void gen_expr(Node *n);
static char *argreg64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static char *argreg32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
static char *argreg16[] = {"di", "si", "dx", "cx", "r8w", "r9w"};
static char *argreg8[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};

static void println(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(out, fmt, ap);
  va_end(ap);
  fprintf(out, "\n");
}

static int count() {
  static int i = 1;
  return i++;
}

static void push(void) {
  println("  push rax");
  depth++;
}

static void pop(char *arg) {
  println("  pop %s", arg);
  depth--;
}

static void load(Type *ty) {
  if (ty->kind == TY_ARRAY || ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
    return;
  }

  switch(ty->size) {
  case 1:
    println("  movsx eax, BYTE PTR [rax]");
    break;
  case 2:
    println("  movsx eax, WORD PTR [rax]");
    break;
  case 4:
    println("  movsxd rax, DWORD PTR [rax]");
    break;
  default:
    println("  mov rax, [rax]");
    break;
  } 

}

static void store(Type *ty) {
  pop("rdi"); //rhs
  pop("rax"); //lhs

  if (ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
    for (int i = 0; i < ty->size; i++) {
      println("  mov r8b, [rdi + %d]", i);
      println("  mov [rax + %d], r8b", i);
    }

    println("  mov rax, rdi");
    return;
  }


  switch (ty->size) {
  case 1:
    println("  mov [rax], dil");
    break;
  case 2: 
    println("  mov [rax], di");
    break;
  case 4: 
    println("  mov [rax], edi");
    break;
  default:
    println("  mov [rax], rdi");
    break;
  }

  println("  mov rax, rdi");
}

static void gen_addr(Node *n) {
  switch(n->kind) {
  case ND_VAR:
    if (n->var->is_local) {
      // local variable
      println("  lea rax, [rbp - %d]", n->var->offset);
    } else {
      // global variable
      println("  lea rax, %s", n->var->name);
      //println("  lea rax, %s[rip]", n->var->name);
      //println("  mov rax, offset %s", n->var->name);
    }
    return;
  case ND_MEMBER:
    gen_addr(n->lhs);
    println("  add rax, %d", n->member->offset);
    return;
  case ND_DEREF:
    gen_expr(n->lhs);
    return;
  case ND_COMMA:
    gen_expr(n->lhs);
    gen_addr(n->rhs);
    return; 
  }

  error("expected a variable");
}

enum {I8, I16, I32, I64};
static int getTypeId(Type *ty) {
  switch (ty->kind) {
  case TY_CHAR:
    return I8;
  case TY_SHORT:
    return I16;
  case TY_INT:
    return I32;
  default:
    return I64;
  }
}

static char i32i8[] = "movsbl eax, al";
static char i32i16[] = "movswl eax, ax";
static char i64i32[] = "movsxd rax, eax";

/*
  + 同じbit数同士は、cast不要
  + 下位bitから64bitに拡張する場合は32bitレジスタを直接64bitへ
    - movsxdで符号拡張
    - 自動で上位bitは0埋めされる
  + 上記以外の下位bitから64bitに拡張する場合は、何もしない
  + 上位bitから下位bitに縮小する場合は、32bitレジスタへ
  + 64bitから32bitへは、何もしない
*/
static char *cast_table[4][4] = {
  {NULL, NULL, NULL, i64i32}, //from I8
  {i32i8, NULL, NULL, i64i32}, //from I16
  {i32i8, i32i16, NULL, i64i32}, //from I32
  {i32i8, i32i16, NULL, NULL}    //from I64
};

static void cast(Type *from, Type *to) {
  if (to->kind == TY_VOID) {
    return;
  }

  if (to->kind == TY_BOOL) {
    if (is_integer(from) && from->size <= 4) {
      println("  cmp eax, 0");
    } else {
      println("  cmp rax, 0");
    }

    println("  setne al");
    println("  movzb eax, al");
    return;
  }

  int t1 = getTypeId(from);
  int t2 = getTypeId(to);
  if (cast_table[t1][t2]) {
    println("  %s", cast_table[t1][t2]);
  }
}

static void gen_expr(Node *n) {
  if (n->token->line != current_line) {
    current_line = n->token->line;
    println("  .loc 1 %d", n->token->line);
  }

  switch (n->kind) {
  case ND_NUM:
    println("  mov rax, %ld", n->val);

    return;
  case ND_NEG:
    gen_expr(n->lhs);
    println("  neg rax");

    return;
  case ND_VAR:
  case ND_MEMBER:
    gen_addr(n);
    load(n->ty);

    return;
  case ND_NULL_EXPR:
    return;
  case ND_STMT_EXPR: {
    //expresion to statement
    for (Node *nb = n->body; nb; nb = nb->next) {
      gen_stmt(nb);
    }

    return;
  }
  case ND_FUNC: {
    int nargs = 0;

    for (Node *arg = n->args; arg; arg = arg->next) {
      gen_expr(arg);
      push();
      nargs++;
    }

    for (int i = nargs - 1; i >= 0; i--) {
      pop(argreg64[i]);
    }

    println("  mov rax, 0");

    if (depth % 2 == 0) {
      println("  call %s", n->funcname);
    } else {
      println("  sub rsp, 8");
      println("  call %s", n->funcname);
      println("  add rsp, 8");
    }

    return;
  }
  case ND_CAST:
    gen_expr(n->lhs);
    // cast(from, to);
    cast(n->lhs->ty, n->ty);
    return;
  case ND_ASSIGN:
    gen_addr(n->lhs);
    push();
    gen_expr(n->rhs);
    push();

    store(n->ty);

    return;
  case ND_MEMZERO:
    println("  mov rcx, %d", n->var->ty->size); //size分repしstosbを繰り返す
    println("  lea rdi, [rbp - %d]", n->var->offset); //offsetの場所から書き込む
    println("  mov al, 0"); //alの値をrdiにコピーする
    println("  cld"); //dfを0にし、repをインクリメントする設定にする
    println("  rep stosb"); //rdiの位置にalの値を書き込み、rdiをインクリメントし、それをrcx回繰り返す

    return;
  case ND_COND: {
    int c = count();

    gen_expr(n->cond);
    println("  cmp rax, 0");
    println("  je .Lelse%03d", c);
    gen_expr(n->then);
    println("  jmp .Lend%03d", c);

    println(".Lelse%03d:", c);
    gen_expr(n->els);
    println(".Lend%03d:", c);

    return;
  }
  case ND_LOGICALOR: {
    int c = count();
    // 左辺の評価 0でなければ、右辺を見ずにtrueとして終了
    gen_expr(n->lhs);
    println("  cmp rax, 0");
    println("  jne .Ltrue.%d", c);

    gen_expr(n->rhs);
    println("  cmp rax, 0");
    println("  jne .Ltrue.%d", c);
    println("  mov rax, 0");
    println("  jmp .Lend.%d", c);
    println(".Ltrue.%d:", c);
    println("  mov rax, 1");
    println(".Lend.%d:", c);
    return;
  }
  case ND_LOGICALAND: {
    int c = count();
    // 左辺の評価 0であれば、右辺を見ずにfalseとして終了
    gen_expr(n->lhs);
    println("  cmp rax, 0");
    println("  je .Lfalse.%d", c);

    gen_expr(n->rhs);
    println("  cmp rax, 0");
    println("  je .Lfalse.%d", c);
    println("  mov rax, 1");
    println("  jmp .Lend.%d", c);
    println(".Lfalse.%d:", c);
    println("  mov rax, 0");
    println(".Lend.%d:", c);
    return;
  }
  case ND_COMMA:
    gen_expr(n->lhs);
    gen_expr(n->rhs);

    return;
  case ND_DEREF:
    gen_expr(n->lhs);
    load(n->ty);

    return;
  case ND_ADDR:
    gen_addr(n->lhs);

    return;
  case ND_NOT:
    gen_expr(n->lhs);
    println("  cmp rax, 0");

    println("  sete al");
    println("  movzb rax, al");

    return;
  case ND_BITNOT:
    gen_expr(n->lhs);
    println("  not rax");

    return;
  default:
    break;
  }

  gen_expr(n->lhs);
  push();
  gen_expr(n->rhs);
  push();

  pop("rdi");
  pop("rax");

  // パフォーマンスの最適化？？32bit計算時の一貫性？？
  char *ax, *di;
  if (n->lhs->ty->kind == TY_LONG || n->lhs->ty->base) {
    ax = "rax";
    di = "rdi";
  } else {
    ax = "eax";
    di = "edi";
  }

  switch(n->kind) {
  // レジスタでシフト量を指定するためにはCLレジスタしか使えない
  // RCX, ECXレジスタは、シフト演算時には使わない
  case ND_SHL:
    println("  mov rcx, rdi");
    println("  shl %s, cl", ax);
    break;
  case ND_SHR:
    println("  mov rcx, rdi");
    println("  sar %s, cl", ax);
    break;
  case ND_ADD:
    println("  add %s, %s", ax, di);
    break;
  case ND_SUB:
    println("  sub %s, %s", ax, di);
    break;
  case ND_MUL:
    println("  imul %s, %s", ax, di);
    break;
  case ND_DIV:
  case ND_MOD:
    if (n->lhs->ty->size == 8) {
      println("  cqo");
    } else {
      println("  cdq");
    }

    println("  idiv %s", di);

    if (n->kind == ND_MOD) {
      println("  mov rax, rdx");
    }

    break;
  case ND_OR:
    println("  or %s, %s", ax, di);
    break;
  case ND_XOR:
    println("  xor %s, %s", ax, di);
    break;
  case ND_AND:
    println("  and %s, %s", ax, di);
    break;
  case ND_EQ:
  case ND_NEQ:
  case ND_LT:
  case ND_LE:
    println("  cmp %s, %s", ax, di);

    if (n->kind == ND_EQ) {
      println("  sete al");
    } else if (n->kind == ND_NEQ) {
      println("  setne al");
    } else if (n->kind == ND_LT) {
      println("  setl al");
    } else if (n->kind == ND_LE) {
      println("  setle al");
    }

    println("  movzb %s, al", ax);
    break;
  default:
    break;
  }

}

static void gen_stmt(Node *n) {
  if (n->token->line != current_line) {
    current_line = n->token->line;
    println("  .loc 1 %d", n->token->line);
  }

  int c;
  switch (n->kind) {
  case ND_BLOCK:
    for (Node *nb = n->body; nb; nb = nb->next) {
      gen_stmt(nb);
    }

    return;
  case ND_GOTO:
    println("  jmp %s", n->unique_label);
    return;
  case ND_LABEL:
    println("%s:", n->unique_label);
    gen_stmt(n->lhs);
    return;
  case ND_EXPR_STMT:
    // statement to expression
    gen_expr(n->lhs);
    return;
  case ND_RETURN:
    if (n->lhs) {
      gen_expr(n->lhs);
    }
    println("  jmp .Lreturn.%s", current_fn->name);

    return;
  case ND_IF:
    c = count();

    gen_expr(n->cond);
    println("  cmp rax, 0");
    println("  je .Lelse%03d", c);
    gen_stmt(n->then);
    println("  jmp .Lend%03d", c);

    println(".Lelse%03d:", c);
    if (n->els) {
      gen_stmt(n->els);
    }
    println(".Lend%03d:", c);

    return;
  case ND_FOR:
    c = count();

    if (n->init) {
      gen_stmt(n->init);
    }
    println(".Lbegin%03d:", c);

    if (n->cond) {
      gen_expr(n->cond);
    }
    println("  cmp rax, 0");
    println("  je %s", n->break_label);
    gen_stmt(n->then);
    println("%s:", n->continue_label);
    if (n->inc) {
      gen_expr(n->inc);
    }
    println("  jmp .Lbegin%03d", c);
    println("%s:", n->break_label);

    return;
  case ND_DO:
    c = count();

    println(".Lbegin%03d:", c);
    gen_stmt(n->then);
    println("%s:", n->continue_label);

    gen_expr(n->cond);
    println("  cmp rax, 0");
    println("  jne .Lbegin%03d", c);
    println("%s:", n->break_label);

    return;
  case ND_SWITCH:
    gen_expr(n->cond);

    char *reg = n->cond->ty->size == 8 ? "rax" : "eax";
    for (Node *c = n->case_next; c; c = c->case_next) {
      println("  cmp %s, %ld", reg, c->val);
      println("  je %s", c->unique_label);
    }

    if (n->default_case) {
      println("  jmp %s", n->default_case->unique_label); 
    }

    println("  jmp %s", n->break_label);

    gen_stmt(n->then);
    println("%s:", n->break_label);

    return;
  case ND_CASE:
    println("%s:", n->unique_label);
    gen_stmt(n->lhs);

    return;
  default:
    break;
  }

  gen_expr(n);
}

/*
  n以上の最小のalignの倍数を返す 
  例:
  0, 4 -> 0
  0, 8 -> 0
  1, 8 -> 8
  11, 8 -> 16
  17, 8 -> 24
*/
int align_to(int n, int align) {
  return n % align == 0 ? n : ((n / align + 1) * align);
}

/*
  新しい変数をリストの先頭に追加
  new --> ~~~ -> second -> first
  先頭アドレスはlocals
  align_stack_size()でoffsetをリストの最初から加算していくので、stack上は新しい変数ほどアドレスは大きい

  stack
  +------------------------------
  | rbp
  | 7fff ffff ffff ffff 
  +------------------------------
  | [rbp - new->offset]    (offset = 8 の場合)
  | 6fff ffff ffff ffff
  +------------------------------
  | ~~~
  +------------------------------
  | [rbp - (first->offset)]
  +------------------------------
  | ~~~
  +------------------------------
  | rsp
  +------------------------------
  | ~~~
  +------------------------------
  | 0x 0000 0000 0000 0000
  +------------------------------

  例:
  int exp(int a, int b, int c) {
     int d, e, f;
     return a + b + c + d + e + f;
  };

  stack_sizeは、functionのparamsも含む 
  リストは下記となる
  f -> e -> d -> c -> a -> b -> c
  params部分は、順番が逆としている
*/
static void align_stack_size(Obj *prog) {
  int offset;
  for (Obj *fn = prog; fn; fn = fn->next) {
    offset = 0;
    for (Obj *var = fn->locals; var; var = var->next) {
      offset += var->ty->size;
      /*
        例:
        int a; char b;
        locals = b --> a
        offset = 0 --> offset = 1(b->offset) --> offset = 1 + 4 --> align_to(5, 4) = 8(a->offset)
        char a; int b;
        offset = 0 --> offset = 4(b->offset) --> offset = 4 + 1 --> align_to(5, 1) = 5(a->offset)
      */
      offset = align_to(offset, var->align);
      var->offset = offset;
    }

    /*
      x86-64 ABIの要件としてfunctionのスタックフレーム境界が16byteであることを保証するために、stack_sizeを16の倍数にする
      https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf
      3.2.2 The Stack Frame
    */
    fn->stack_size = align_to(offset, 16);
  }
}

static void emit_data(Obj *prog) {
  for (Obj *var = prog; var; var = var->next) {
    if (var->is_function || !var->is_definition) {
      continue;
    }

    if (var->is_static) {
      println("  .local %s", var->name);
    } else {
      println("  .global %s", var->name);
    }

    if (var->init_data) {
      println("  .data");
      /*
        .alignディレクティブを使うことで型ごとの適切なデータ配置を行う(CPUによってはアライメントが必要)
      */
      println("  .align %d", var->align);
      println("%s:", var->name);

      Relocation *rel = var->rel;
      int pos = 0;
      while (pos < var->ty->size) {
        if (rel && rel->offset == pos) {
          // そのラベルに8byteデータ配置の場合は.quad
          println("  .quad %s%+ld", rel->label, rel->addend);
          rel = rel->next;
          pos += 8;
        } else {
          // そのラベルに1byteデータ配置の場合は.byte
          println("  .byte %d", var->init_data[pos++]);
        }
      }
      continue;
    }

    /*
      初期化されていない変数は.bssに配置
      実際の0埋めのデータが実行ファイルには含まれないため実行ファイルサイズを小さくできる
    */
    println("  .bss");
    println("  .align %d", var->align);
    println("%s:", var->name);
    println("  .zero %d", var->ty->size);
  }

}

static void emit_text(Obj *prog) {
  println(".intel_syntax noprefix");

  for (Obj *fn = prog; fn; fn = fn->next) {
    if (!fn->is_function || !fn->is_definition) {
      continue;
    }

    current_fn = fn;
    if (fn->is_static) {
      println("  .local %s", fn->name);
    } else {
      println("  .global %s", fn->name);
    }

    println("  .text ");
    println("%s:", fn->name);

    //prologue
    println("  push rbp");
    println("  mov rbp, rsp");
    println("  sub rsp, %d", fn->stack_size);

    /*
      paramsのリストは引数指定順
      func(a, b, c, d, e, f)
      a -> b -> c -> d -> e -> f
    */
    int i = 0;
    for (Obj *var = fn->params; var; var = var->next) {
      switch (var->ty->size) {
      case 1:
        println("  mov [rbp - %d], %s", var->offset, argreg8[i++]);
        break;
      case 2:
        println("  mov [rbp - %d], %s", var->offset, argreg16[i++]);
        break;
      case 4:
        println("  mov [rbp - %d], %s", var->offset, argreg32[i++]);
        break;
      default:
        println("  mov [rbp - %d], %s", var->offset, argreg64[i++]);
        break;
      }
    }

    gen_stmt(fn->body);
    assert(depth == 0);

    //epilogue
    println(".Lreturn.%s:", fn->name);
    println("  mov rsp, rbp");
    println("  pop rbp");
    println("  ret");
  }
}

void codegen(Obj *prog, FILE *outfile) {
  out = outfile;

  align_stack_size(prog);
  emit_data(prog);
  emit_text(prog);
}

