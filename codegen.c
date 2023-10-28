#include "9cc.h"

static Obj *current_fn;
static FILE *out;
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

static void load(Type *ty) {
  if (ty->kind == TY_ARRAY || ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
    return;
  }

  switch(ty->size) {
  case 1:
    println("  movzx rax, BYTE PTR [rax]");
    break;
  case 2:
    println("  movsx rax, WORD PTR [rax]");
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
  println("  pop rdi"); //rhs
  println("  pop rax"); //lhs

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

  int t1 = getTypeId(from);
  int t2 = getTypeId(to);
  if (cast_table[t1][t2]) {
    println("  %s", cast_table[t1][t2]);
  }
}

static void gen_expr(Node *n) {
  println("  .loc 1 %d", n->token->line);

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
      println("  push rax");
      nargs++;
    }

    for (int i = nargs - 1; i >= 0; i--) {
      println("  pop %s", argreg64[i]);
    }

    println("  mov rax, 0");
    println("  call %s", n->funcname);

    return;
  }
  case ND_CAST:
    gen_expr(n->lhs);
    // cast(from, to);
    cast(n->lhs->ty, n->ty);
    return;
  case ND_ASSIGN:
    gen_addr(n->lhs);
    println("  push rax");
    gen_expr(n->rhs);
    println("  push rax");

    store(n->ty);

    return;
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
  default:
    break;
  }

  gen_expr(n->lhs);
  println("  push rax");
  gen_expr(n->rhs);
  println("  push rax");

  println("  pop rdi");
  println("  pop rax");

  // パフォーマンスの最適化？？32bit計算時の一貫性？？
  char *ax, *di;
  if (n->lhs->ty->kind == TY_LONG || n->lhs->ty->base) {
    ax = "rax";
    di = "rdi";
  } else {
    ax = "eax";
    di = "edi";
  }
  //println("  pop %s", di);
  //println("  pop %s", ax);

  switch(n->kind) {
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
    if (n->lhs->ty->size == 8) {
      println("  cqo");
    } else {
      println("  cdq");
    }

    println("  idiv %s", di);
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
  println("  .loc 1 %d", n->token->line);

  int c;
  switch (n->kind) {
  case ND_BLOCK:
    for (Node *nb = n->body; nb; nb = nb->next) {
      gen_stmt(nb);
    }

    return;
  case ND_EXPR_STMT:
    // statement to expression
    gen_expr(n->lhs);
    return;
  case ND_RETURN:
    gen_expr(n->lhs);
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
  case ND_WHILE:
    c = count();

    println(".Lbegin%03d:", c);
    gen_expr(n->cond);
    println("  cmp rax, 0");
    println("  je .Lend%03d", c);
    gen_stmt(n->then);
    println("  jmp .Lbegin%03d", c);
    println(".Lend%03d:", c);

    return;
  case ND_FOR:
    c = count();

    if (n->init) {
      gen_expr(n->init);
    }
    println(".Lbegin%03d:", c);
    if (n->cond) {
      gen_expr(n->cond);
    }
    println("  cmp rax, 0");
    println("  je .Lend%03d", c);
    gen_stmt(n->then);
    if (n->inc) {
      gen_expr(n->inc);
    }
    println("  jmp .Lbegin%03d", c);
    println(".Lend%03d:", c);

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
      offset = align_to(offset, var->ty->align);
      var->offset = offset;
    }

    fn->stack_size = align_to(offset, 16);
  }
}

static void emit_data(Obj *prog) {
  for (Obj *var = prog; var; var = var->next) {
    if (var->is_function) {
      continue;
    }

    println("  .data");
    println("  .global %s", var->name);
    println("%s:", var->name);
    if (var->init_data) {
      for (int i = 0; i < var->ty->size; i++) {
        println("  .byte %d", var->init_data[i]);
      }
    } else {
      println("  .zero %d", var->ty->align);
    }
  }

}

static void emit_text(Obj *prog) {
  println(".intel_syntax noprefix");

  for (Obj *fn = prog; fn; fn = fn->next) {
    if (!fn->is_function) {
      continue;
    }

    current_fn = fn;
    println("  .global %s", fn->name);
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

