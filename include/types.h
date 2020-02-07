#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>

typedef struct Type Type;
struct Type;
struct Vector;
struct SparseVector;
struct AST;
struct Map;

typedef enum Qualifiers {
    Q_CONST = 0x1, Q_FRIEND = 0x2
} Qualifiers;

typedef struct TypeCheckState {
    struct Map *symbols; // Map<char*, Type*>
} TypeCheckState;

void
json_type(const Type *type, FILE *out, int indent);

void
json_qualifier(const Qualifiers *value, FILE *out, int indent);

Type *
copy_type(const Type *type);

void
delete_type(Type *type);

void
Type_setQualifiers(Type *type, struct Vector *qualifiers);

#define FuncType(gen, args, ret) new_FuncType(gen, args, ret)
Type *
new_FuncType(struct Vector *generics, struct Vector *args, Type *ret_type);

#define ClassType(name, gen) new_ClassType(name, gen)
Type *
new_ClassType(char *name, struct Vector *generics);

#define ExprType(expr, gen) new_ExprType(expr, gen)
Type *
new_ExprType(struct AST *expr, struct Vector *generics);

#define TupleType(types) new_TupleType(types)
Type *
new_TupleType(struct SparseVector *types);

#define NamedType(name, type) new_NamedType(name, type)
Type *
new_NamedType(char *name, Type *type);

#endif
