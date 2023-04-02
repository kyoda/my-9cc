#include "9cc.h"

static Function *current_fn;
static void gen_stmt(Node *n);
static void gen_expr(Node *n);
static char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static int count() {
  static int i = 1;
  return i++;
}

void gen_lval(Node *n) {
  if (n->kind != ND_LVAR) {
    fprintf(stderr, "no ND_LVAR \n");
    exit(1);
  }

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", n->var->offset);
}


void gen_expr(Node *n) {

  switch (n->kind) {
  case ND_NUM:
    printf("  mov rax, %d\n", n->val);    

    return;
  case ND_LVAR:
    gen_lval(n);

    printf("  mov rax, [rax]\n");

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
    gen_lval(n->lhs);
    printf("  push rax\n");
    gen_expr(n->rhs);
    printf("  push rax\n");

    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  mov rax, rdi\n");

    return;
  case ND_DEREF:
    gen_expr(n->lhs);

    printf("  mov rax, [rax]\n");

    return;
  case ND_ADDR:
    gen_lval(n->lhs);

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

void align_stack_size(Function *prog) {
  int offset;
  for (Function *fn = prog; fn; fn = fn->next) {
    offset = 0;
    for (LVar *var = fn->locals; var; var = var->next) {
      offset += 8;
      var->offset = offset;
    }

    fn->stack_size = offset;
  }
}

void codegen(Function *prog) {
  align_stack_size(prog);

  printf(".intel_syntax noprefix\n");

  for (Function *fn = prog; fn; fn = fn->next) {
    current_fn = fn;
    printf(".global %s\n", fn->name);
    printf("%s:\n", fn->name);

    //prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", fn->stack_size);

    for (LVar *var = fn->params; var; var = var->next) {
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

