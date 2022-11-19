#include "9cc.h"

void gen_lval(Node *n) {
  if (n->kind != ND_LVAR) {
    fprintf(stderr, "no ND_LVAR \n");
    exit(1);
  }

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", n->offset);
  printf("  push rax\n");
}

void gen_main() {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  //prologue
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, 208\n");

  int i = 0;
  while (code[i] != NULL) {
    gen(code[i]);
    i++;
    printf("  pop rax\n");
  }

  //epilogue
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
}

void gen(Node *n) {

  //fprintf(stderr, "n->kind: %d\n", n->kind);

  switch (n->kind) {
  case ND_NUM:
    printf("  push %d\n", n->val);    

    return;
  case ND_LVAR:
    gen_lval(n);

    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");

    return;
  case ND_ASSIGN:
    gen_lval(n->lhs);
    gen(n->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return;
  }

  gen(n->lhs);
  gen(n->rhs);

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
  }

  printf("  push rax\n");    

}

