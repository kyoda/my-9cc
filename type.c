#include "9cc.h"

Type *new_type(TypeKind kind, int size, int align) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = kind;
  ty->size = size;
  ty->align = align;
  return ty;
}

Type *ty_int() {
  return new_type(TY_INT, 4, 4);
}

Type *ty_long() {
  return new_type(TY_LONG, 8, 8);
}

Type *ty_char() {
  return new_type(TY_CHAR, 1, 1);
}

Type *pointer_to(Type *base) {
  Type *ty = new_type(TY_PTR, 8, 8);
  ty->base = base;

  return ty;
}

Type *ty_array(Type *base, int len) {
  Type *ty = new_type(TY_ARRAY, base->size * len, base->align);
  ty->base = base;
  ty->array_len = len;

  return ty;
}

Type *ty_func(Type *base) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_FUNC;
  ty->base = base;

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
  case ND_MEMBER:
    n->ty = n->member->ty;
    return;
  case ND_EQ:
  case ND_NEQ:
  case ND_LT:
  case ND_LE:
  case ND_FUNC:
  case ND_NUM:
    n->ty = ty_long();
    return;
  case ND_VAR:
    n->ty = n->var->ty;
    return;
  case ND_ADDR:
    n->ty = pointer_to(n->lhs->ty);
    return;
  case ND_DEREF:
    if (!n->lhs->ty->base) {
      error("%s", "invalid deref");
    }

    n->ty = n->lhs->ty->base;
    return;
  }

}
