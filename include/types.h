#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>

typedef struct Type Type;
struct Type;
struct Vector;
struct SparseVector;
struct AST;
struct Map;
struct YYLTYPE;

typedef enum Types {
    TYPE_FUNC,
    TYPE_CLASS,
    TYPE_OBJECT,
    TYPE_EXPR,
    TYPE_TUPLE,
    TYPE_SPREAD,
    TYPE_NAMED,
    TYPE_NONE,
    TYPE_ARRAY,
    TYPE_MAYBE
} Types;

typedef enum Qualifiers {
    Q_CONST = 0x1, Q_FRIEND = 0x2
} Qualifiers;

struct FuncType {
    struct Vector *generics; // Vector<char*>
    struct Vector *args;     // Vector<Type*>
    Type *ret_type;
};

struct ClassType {
    struct Vector *generics;     // Vector<char*>
    struct Vector *supers;       // Vector<char*>
    struct Vector *constructors; // Vector<Vector<Type*>>
    struct Map *fields;          // Map<char*, Type*>
};

struct ObjectType {
    char *name;
    struct Vector *generics; // Vector<char*>
    struct ClassType *class;
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

struct ArrayType {
    Type *type;
};

struct MaybeType {
    Type *type;
};

typedef struct TypeCheckState {
    struct Map *symbols; // Map<char*, Type*>
    // Map class names to their implementations. Each time a class name is
    // seen it gets added to the map with NULL as the AST. Each time an impl
    // is seen, add its AST to the map. At the end of type checking, check
    // that all classes have implementations.
    struct Map *classes; // Map<char*, AST*>
    // NULL if not in function, otherwise return type of current function
    Type *funcType;
    // NULL initially, gets set by return statements. Used to tell if there are
    // conflicting return types or if a non-void function didn't return a
    // value on all code paths.
    Type *retType;
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

int
TypeVerify(Type *type, const TypeCheckState *state, char **msg);

Types
typeOf(const Type *type);

struct YYLTYPE
typeLoc(const Type *type);

unsigned char
isInit(const Type *type);

void
setInit(Type *type, unsigned char init);

const void *
getTypeData(Type *type);

char *
typeToString(const Type *type);

#define FuncType(loc, gen, args, ret) \
    new_FuncType(loc, gen, args, ret)
Type *
new_FuncType(const struct YYLTYPE *loc,
    struct Vector *generics,
    struct Vector *args,
    Type *ret_type);

#define ClassType(loc, gen, sup, cons, fields) \
    new_ClassType(loc, gen, sup, cons, fields)
Type *
new_ClassType(const struct YYLTYPE *loc,
    struct Vector *generics,
    struct Vector *supers,
    struct Vector *cons,
    struct Map *fields);

#define ObjectType(loc, name, gen) \
    new_ObjectType(loc, name, gen)
Type *
new_ObjectType(const struct YYLTYPE *loc, char *name, struct Vector *generics);

#define ExprType(loc, expr, gen) \
    new_ExprType(loc, expr, gen)
Type *
new_ExprType(const struct YYLTYPE *loc,
    struct AST *expr,
    struct Vector *generics);

#define TupleType(loc, types) \
    new_TupleType(loc, types)
Type *
new_TupleType(const struct YYLTYPE *loc, struct SparseVector *types);

#define SpreadType(tuple) \
    new_SpreadType(tuple)
Type *
new_SpreadType(Type *tuple);

#define NamedType(loc, name, type) \
    new_NamedType(loc, name, type)
Type *
new_NamedType(const struct YYLTYPE *loc, char *name, Type *type);

#define NoneType(loc) \
    new_NoneType(loc)
Type *
new_NoneType(const struct YYLTYPE *loc);

#define ArrayType(loc, type) \
    new_ArrayType(loc, type)
Type *
new_ArrayType(const struct YYLTYPE *loc, Type *type);

#define MaybeType(loc, type) \
    new_MaybeType(loc, type)
Type *
new_MaybeType(const struct YYLTYPE *loc, Type *type);

#endif
