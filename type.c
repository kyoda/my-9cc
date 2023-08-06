#include "9cc.h"

Type *ty_int() {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_INT;
  ty->size = 4;
  ty->align = 8;
  return ty;
}

Type *ty_char() {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_CHAR;
  ty->size = 1;
  ty->align = 8;
  return ty;
}

Type *pointer_to(Type *base) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_PTR;
  ty->size = 8;
  ty->align = 8;
  ty->next = base;

  return ty;
}

Type *ty_array(Type *base, int len) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_ARRAY;
  ty->size = base->size * len;
  ty->align = base->size * len;
  ty->next = base;
  ty->array_len = len;

  return ty;
}

void add_type(Node *n) {
  if (!n || n->ty) {
    return;
  }

  add_type(n->lhs);
  add_type(n->rhs);
  add_type(n->init);
  add_type(n->cond);
  add_type(n->inc);
  add_type(n->then);
  add_type(n->els);

  for (Node *node = n->body; node; node = node->next) {
    add_type(node);
  }

  for (Node *node = n->args; node; node = node->next) {
    add_type(node);
  }

  switch(n->kind) {
  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
  case ND_NEG:
  case ND_ASSIGN:
    n->ty = n->lhs->ty;
    return;
  case ND_COMMA:
    n->ty = n->rhs->ty;
    return;
  case ND_EQ:
  case ND_NEQ:
  case ND_LT:
  case ND_LE:
  case ND_FUNC:
  case ND_NUM:
    n->ty = ty_int();
    return;
  case ND_VAR:
    n->ty = n->var->ty;
    return;
  case ND_ADDR:
    n->ty = pointer_to(n->lhs->ty);
    return;
  case ND_DEREF:
    if (!n->lhs->ty->next) {
      error("%s", "invalid deref");
    }

    n->ty = n->lhs->ty->next;
    return;
  }

}
