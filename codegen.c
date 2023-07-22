#include "9cc.h"

static Obj *current_fn;
static void gen_stmt(Node *n);
static void gen_expr(Node *n);
static char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static int count() {
  static int i = 1;
  return i++;
}

void load(Type *ty) {
  if (ty->kind == TY_ARRAY) {
    return;
  }

  if (ty->size == 1) {
    printf("  mov al, [rax]\n");
  } else {
    printf("  mov rax, [rax]\n");
  }
}

void gen_addr(Node *n) {
  switch(n->kind) {
  case ND_VAR:
    if (n->var->is_local) {
      printf("  lea rax, [rbp - %d]\n", n->var->offset);
    } else {
      printf("  lea rax, %s\n", n->var->name);
      //printf("  lea rax, %s[rip]\n", n->var->name);
      //printf("  mov rax, offset %s\n", n->var->name);
    }
    return;
  case ND_DEREF:
    gen_expr(n->lhs);
    return;
  }

  error("expected a variable");
}

void gen_expr(Node *n) {

  switch (n->kind) {
  case ND_NUM:
    printf("  mov rax, %d\n", n->val);    

    return;
  case ND_NEG:
    gen_expr(n->lhs);
    printf("  neg rax\n");    

    return;
  case ND_VAR:
    gen_addr(n);
    load(n->var->ty);

    return;
  case ND_FUNC: {
    int nargs = 0;

    for (Node *arg = n->args; arg; arg = arg->next) {
      gen_expr(arg);
      printf("  push rax\n");
      nargs++;
    }

    for (int i = nargs - 1; i >= 0; i--) {
      printf("  pop %s\n", argreg[i]);
    }

    printf("  mov rax, 0\n");
    printf("  call %s\n", n->funcname);

    return;
  }
  case ND_ASSIGN:
    gen_addr(n->lhs);
    printf("  push rax\n");
    gen_expr(n->rhs);
    printf("  push rax\n");

    printf("  pop rdi\n");
    printf("  pop rax\n");

    if (n->ty->size == 1) {
      printf("  mov [rax], dil\n");
    } else {
      printf("  mov [rax], rdi\n");
    }

    printf("  mov rax, rdi\n");

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
  printf("  push rax\n");    
  gen_expr(n->rhs);
  printf("  push rax\n");    

  printf("  pop rdi\n");    
  printf("  pop rax\n");    

  switch(n->kind) {
  case ND_ADD:
     printf("  add rax, rdi\n");    
     break;
  case ND_SUB:
     printf("  sub rax, rdi\n");    
     break;
  case ND_MUL:
     printf("  imul rax, rdi\n");    
     break;
  case ND_DIV:
     printf("  cqo\n");    
     printf("  idiv rdi\n");    
     break;
  case ND_EQ:
     printf("  cmp rax, rdi\n");    
     printf("  sete al\n");    
     printf("  movzb rax, al\n");    
     break;
  case ND_NEQ:
     printf("  cmp rax, rdi\n");    
     printf("  setne al\n");    
     printf("  movzb rax, al\n");    
     break;
  case ND_LT:
     printf("  cmp rax, rdi\n");    
     printf("  setl al\n");    
     printf("  movzb rax, al\n");    
     break;
  case ND_LE:
     printf("  cmp rax, rdi\n");    
     printf("  setle al\n");    
     printf("  movzb rax, al\n");    
     break;
  default:
    break;
  }

}


static void gen_stmt(Node *n) {
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
    printf("  jmp .Lreturn.%s\n", current_fn->name);    

    return;
  case ND_IF:
    c = count();

    gen_expr(n->cond);
    printf("  cmp rax, 0\n");    
    printf("  je .Lelse%03d\n", c);    
    gen_stmt(n->then);
    printf("  jmp .Lend%03d\n", c);    

    printf(".Lelse%03d:\n", c);
    if (n->els) {
      gen_stmt(n->els);
    }
    printf(".Lend%03d:\n", c);

    return;
  case ND_WHILE:
    c = count();

    printf(".Lbegin%03d:\n", c);
    gen_expr(n->cond);
    printf("  cmp rax, 0\n");    
    printf("  je .Lend%03d\n", c);    
    gen_stmt(n->then);
    printf("  jmp .Lbegin%03d\n", c);    
    printf(".Lend%03d:\n", c);

    return;
  case ND_FOR:
    c = count();

    if (n->init) {
      gen_expr(n->init);
    }
    printf(".Lbegin%03d:\n", c);
    if (n->cond) {
      gen_expr(n->cond);
    }
    printf("  cmp rax, 0\n");    
    printf("  je .Lend%03d\n", c);    
    gen_stmt(n->then);
    if (n->inc) {
      gen_expr(n->inc);
    }
    printf("  jmp .Lbegin%03d\n", c);    
    printf(".Lend%03d:\n", c);

    return;
  default:
    break;
  }

  gen_expr(n);
}

int align_to(int n, int align) {
  return (n / align + 1) * align;
}

void align_stack_size(Obj *prog) {
  int offset;
  for (Obj *fn = prog; fn; fn = fn->next) {
    offset = 0;
    for (Obj *var = fn->locals; var; var = var->next) {
      offset += var->ty->align;
    }

    fn->stack_size = align_to(offset, 16);

    for (Obj *var = fn->locals; var; var = var->next) {
      var->offset = offset;
      offset -= var->ty->align;
    }

  }
}

void emit_data(Obj *prog) {
  for (Obj *var = prog; var; var = var->next) {
    if (var->is_function) {
      continue;
    }

    printf("  .data\n");
    printf("  .global %s\n", var->name);
    printf("%s:\n", var->name);
    if (var->init_data) {
      for (int i = 0; i < var->ty->size; i++) {
        printf("  .byte %d\n", var->init_data[i]);
      }
    } else {
      printf("  .zero %d\n", var->ty->align);
    }
  }

}

void emit_text(Obj *prog) {
  printf(".intel_syntax noprefix\n");

  for (Obj *fn = prog; fn; fn = fn->next) {
    if (!fn->is_function) {
      continue;
    }

    current_fn = fn;
    printf("  .global %s\n", fn->name);
    printf("  .text \n");
    printf("%s:\n", fn->name);

    //prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", fn->stack_size);

    for (Obj *var = fn->params; var; var = var->next) {
      printf("  mov [rbp - %d], %s\n", var->offset, argreg[var->offset / 8 - 1]);
    }
    
    gen_stmt(fn->body);

    //epilogue
    printf(".Lreturn.%s:\n", fn->name);
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
  }
}

void codegen(Obj *prog) {
  align_stack_size(prog);
  emit_data(prog);
  emit_text(prog);

}

