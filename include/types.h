#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include "ast.h"

typedef struct Type Type;
struct Type;
struct Vector;
struct SparseVector;
struct Map;

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
    AST *expr;
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

enum {
    BUILTIN_INT, BUILTIN_BOOL, BUILTIN_DOUBLE, BUILTIN_STRING
} Builtins;
#define NUM_BUILTINS 4

typedef struct TypeCheckState {
    struct Map *symbols;    // Map<char*, Type*>
    // Used inside control flow statements to add newly defined symbols to
    // the outer scope if they are defined in all code paths. Defaults to
    // NULL and allocation and destruction must be handled by the control
    // flow AST node.
    struct Map *newSymbols; // Map<char*, Type*>
    struct Vector *classes; // Vector<const struct ClassType*>
    const struct ClassType *builtins[NUM_BUILTINS];
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
delete_ClassType(struct ClassType *class);

void
setTypeQualifiers(Type *type, struct Vector *qualifiers);

int
TypeCompare(const Type *type1, const Type *type2, const TypeCheckState *state);

int
TypeVerify(Type *type, const TypeCheckState *state, char **msg);

Types
typeOf(const Type *type);

YYLTYPE
typeLoc(const Type *type);

unsigned char
isInit(const Type *type);

void
setInit(Type *type, unsigned char init);

const void *
getTypeData(Type *type);

char *
typeToString(const Type *type);

int
TypeIntersection(const Type *type1,
    const Type *type2,
    const TypeCheckState *state,
    YYLTYPE loc,
    Type **typeptr);

#define FuncType(loc, gen, args, ret) \
    new_FuncType(loc, gen, args, ret)
Type *
new_FuncType(YYLTYPE loc,
    struct Vector *generics,
    struct Vector *args,
    Type *ret_type);

#define ClassType(loc, gen, sup, cons, fields) \
    new_ClassType(loc, gen, sup, cons, fields)
Type *
new_ClassType(YYLTYPE loc,
    struct Vector *generics,
    struct Vector *supers,
    struct Vector *cons,
    struct Map *fields);

#define ObjectType(loc, name, gen) \
    new_ObjectType(loc, name, gen)
Type *
new_ObjectType(YYLTYPE loc, char *name, struct Vector *generics);

#define ExprType(loc, expr, gen) \
    new_ExprType(loc, expr, gen)
Type *
new_ExprType(YYLTYPE loc, AST *expr, struct Vector *generics);

#define TupleType(loc, types) \
    new_TupleType(loc, types)
Type *
new_TupleType(YYLTYPE loc, struct SparseVector *types);

#define SpreadType(tuple) \
    new_SpreadType(tuple)
Type *
new_SpreadType(Type *tuple);

#define NamedType(loc, name, type) \
    new_NamedType(loc, name, type)
Type *
new_NamedType(YYLTYPE loc, char *name, Type *type);

#define NoneType(loc) \
    new_NoneType(loc)
Type *
new_NoneType(YYLTYPE loc);

#define ArrayType(loc, type) \
    new_ArrayType(loc, type)
Type *
new_ArrayType(YYLTYPE loc, Type *type);

#define MaybeType(loc, type) \
    new_MaybeType(loc, type)
Type *
new_MaybeType(YYLTYPE loc, Type *type);

#endif
