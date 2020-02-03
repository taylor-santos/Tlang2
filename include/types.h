#ifndef TYPES_H
#define TYPES_H
#define FuncType(gen, args, ret) new_FuncType(gen, args, ret)
#define ClassType(name, gen) new_ClassType(name, gen)
#define ExprType(expr, gen) new_ExprType(expr, gen)

#include <stdio.h>

typedef struct Type Type;
struct Type;
struct Vector;
struct AST;

typedef enum Qualifiers {
    CONST = 0x1, FRIEND = 0x2
} Qualifiers;

void
json_type(const Type *type, FILE *out, int indent);

void
json_qualifier(Qualifiers *value, FILE *out, int indent);

void
delete_type(Type *type);

void
Type_setQualifiers(Type *type, struct Vector *qualifiers);

Type *
new_FuncType(struct Vector *generics, struct Vector *args, Type *ret_type);

Type *
new_ClassType(char *name, struct Vector *generics);

Type *
new_ExprType(struct AST *expr, struct Vector *generics);

#endif
