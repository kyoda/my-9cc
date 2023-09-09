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
  if (ty->kind == TY_ARRAY) {
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

static void gen_addr(Node *n) {
  switch(n->kind) {
  case ND_VAR:
    if (n->var->is_local) {
      println("  lea rax, [rbp - %d]", n->var->offset);
    } else {
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

static void gen_expr(Node *n) {
  println("  .loc 1 %d", n->token->line);

  switch (n->kind) {
  case ND_NUM:
    println("  mov rax, %d", n->val);

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
  case ND_ASSIGN:
    gen_addr(n->lhs);
    println("  push rax");
    gen_expr(n->rhs);
    println("  push rax");

    println("  pop rdi");
    println("  pop rax");

    switch (n->ty->size) {
    case 1:
      println("  mov [rax], dil");
      break;
    case 2: 
      println("  mov [rax], di");
      break;
    case 4: 
      println("  mov [rax], edi");
      break;
    case 8:
      println("  mov [rax], rdi");
      break;
    default:
      error("invalid size");
    }

    println("  mov rax, rdi");

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

  switch(n->kind) {
  case ND_ADD:
     println("  add rax, rdi");
     break;
  case ND_SUB:
     println("  sub rax, rdi");
     break;
  case ND_MUL:
     println("  imul rax, rdi");
     break;
  case ND_DIV:
     println("  cqo");
     println("  idiv rdi");
     break;
  case ND_EQ:
     println("  cmp rax, rdi");
     println("  sete al");
     println("  movzb rax, al");
     break;
  case ND_NEQ:
     println("  cmp rax, rdi");
     println("  setne al");
     println("  movzb rax, al");
     break;
  case ND_LT:
     println("  cmp rax, rdi");
     println("  setl al");
     println("  movzb rax, al");
     break;
  case ND_LE:
     println("  cmp rax, rdi");
     println("  setle al");
     println("  movzb rax, al");
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

static void align_stack_size(Obj *prog) {
  int offset;
  for (Obj *fn = prog; fn; fn = fn->next) {
    offset = 0;
    /*
      stack_sizeは、functionのparamsも含む 
    */
    for (Obj *var = fn->locals; var; var = var->next) {
      offset += var->ty->size;
      //offset = align_to(offset, var->ty->align);
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

