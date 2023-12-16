#include "9cc.h"

Type *new_type(TypeKind kind, int size, int align) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = kind;
  ty->size = size;
  ty->align = align;
  return ty;
}

Type *ty_void() {
  return new_type(TY_VOID, 1, 1);
}

Type *ty_bool() {
  return new_type(TY_BOOL, 1, 1);
}

Type *ty_char() {
  return new_type(TY_CHAR, 1, 1);
}

Type *ty_short() {
  return new_type(TY_SHORT, 2, 2);
}

Type *ty_int() {
  return new_type(TY_INT, 4, 4);
}

Type *ty_long() {
  return new_type(TY_LONG, 8, 8);
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
  ty->return_ty = base;

  return ty;
}

static Type *get_common_type(Type *ty1, Type *ty2) {
  // ty1 is pointer
  if (ty1->base) {
    return pointer_to(ty1->base);
  }

  if (ty1->size == 8 || ty2->size == 8) {
    return ty_long();
  }

  return ty_int();
}

static void usual_arith_conv(Node **lhs, Node **rhs) {
  Type *ty = get_common_type((*lhs)->ty, (*rhs)->ty);

  *lhs = new_cast(*lhs, ty, (*lhs)->token);
  *rhs = new_cast(*rhs, ty, (*rhs)->token);
}

bool is_integer(Type *ty) {
  TypeKind kind = ty->kind;
  return kind == TY_BOOL || kind == TY_CHAR || kind == TY_SHORT || kind == TY_INT || kind == TY_LONG;
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
  case ND_NUM:
    n->ty = (n->val == (int)n->val) ? ty_int() : ty_long();
    return;
  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
    usual_arith_conv(&n->lhs, &n->rhs);
    n->ty = n->lhs->ty;
    return;
  case ND_NEG: {
    Type *ty = get_common_type(ty_int(), n->lhs->ty);
    n->lhs = new_cast(n->lhs, ty, n->lhs->token);
    n->ty = ty;
    return;
  }
  case ND_ASSIGN:
    if (n->lhs->ty->kind == TY_ARRAY) {
      error("%s", "left variable is array");
    }

    if (n->lhs->ty->kind != TY_STRUCT) {
      n->rhs = new_cast(n->rhs, n->lhs->ty, n->rhs->token);
    }

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
    usual_arith_conv(&n->lhs, &n->rhs);
    n->ty = ty_int();
    return;
  case ND_FUNC:
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

    if (n->lhs->ty->base->kind == TY_VOID) {
      error("%s", "dereferencing a void pointer");
    }

    n->ty = n->lhs->ty->base;
    return;
  case ND_STMT_EXPR:
    if (n->body) {
      /*
        bodyの最後がND_EXPR_STMTであることを確認し、そのlhsの型を割り当てる
        そのためbodyの最後は、「expr ";"」
        下記のようなパターンは許可しない
          + ({})
          + ({return 0;})
      */
      Node *stmt = n->body;
      while (stmt->next) {
        stmt = stmt->next;
      }

      if (stmt->kind == ND_EXPR_STMT) {
        n->ty = stmt->lhs->ty;
        return;
      }
    }

    error("%s", "statement expression returning void is not supported");
    return;
  }

}
