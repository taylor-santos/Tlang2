#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>

typedef struct Type Type;
struct Type;
struct Vector;
struct SparseVector;
struct AST;
struct Map;

typedef enum Types {
    TYPE_FUNC,
    TYPE_CLASS,
    TYPE_OBJECT,
    TYPE_EXPR,
    TYPE_TUPLE,
    TYPE_SPREAD,
    TYPE_NAMED
} Types;

typedef enum Qualifiers {
    Q_CONST = 0x1, Q_FRIEND = 0x2
} Qualifiers;

struct FuncType {
    struct Vector *generics; // Vector<char*>
    struct Vector *args;     // Vector<Type*>
    Type *ret_type;          // NULLable
};

struct ClassType {
    struct Vector *generics; // Vector<char*>
    struct Vector *supers;   // Vector<char*>
    struct Map *fields;      // Map<char*, Type*>
};

struct ObjectType {
    char *name;
    struct Vector *generics; // Vector<char*>
};

struct ExprType {
    struct AST *expr;
    struct Vector *generics; // Vector<char*>
    int ownsAST;
};

struct TupleType {
    struct SparseVector *types; // Vector<Type*>
};

struct SpreadType {
    struct SparseVector *types; // Vector<Type*>
};

struct NamedType {
    char *name;
    Type *type;
};

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
setTypeQualifiers(Type *type, struct Vector *qualifiers);

int
TypeCompare(const Type *type1, const Type *type2, const TypeCheckState *state);

Types
typeOf(const Type *type);

unsigned char
isInit(const Type *type);

void
setInit(Type *type, unsigned char init);

const void *
getTypeData(Type *type);

char *
typeToString(const Type *type);

#define FuncType(gen, args, ret) new_FuncType(gen, args, ret)
Type *
new_FuncType(struct Vector *generics, struct Vector *args, Type *ret_type);

#define ClassType(gen, supers, fields) new_ClassType(gen, supers, fields)
Type *
new_ClassType(struct Vector *generics,
    struct Vector *supers,
    struct Map *fields);

#define ObjectType(name, gen) new_ObjectType(name, gen)
Type *
new_ObjectType(char *name, struct Vector *generics);

#define ExprType(expr, gen) new_ExprType(expr, gen)
Type *
new_ExprType(struct AST *expr, struct Vector *generics);

#define TupleType(types) new_TupleType(types)
Type *
new_TupleType(struct SparseVector *types);

#define NamedType(name, type) new_NamedType(name, type)
Type *
new_NamedType(char *name, Type *type);

#define SpreadType(tuple) new_SpreadType(tuple)
Type *
new_SpreadType(Type *tuple);

#endif
