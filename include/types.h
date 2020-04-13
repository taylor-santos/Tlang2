#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include "ast.h"

struct Vector;
struct SparseVector;
struct Map;

typedef enum Types {
    TYPE_FUNC,
    TYPE_CLASS,
    TYPE_OBJECT,
    TYPE_TUPLE,
    TYPE_SPREAD,
    TYPE_NONE,
    TYPE_ARRAY,
    TYPE_MAYBE
} Types;

typedef enum Qualifiers {
    Q_CONST = 0x1, Q_FRIEND = 0x2
} Qualifiers;

Qualifiers *
copy_Qualifiers(const Qualifiers *q);

typedef struct Type Type;

struct Type {
    void (*json)(const void *this, FILE *out, int indent);
    Type *(*copy)(const void *this);
    int (*compare)(const void *this,
        const void *other,
        const struct TypeCheckState *state);
    int (*verify)(void *type, const struct TypeCheckState *state, char **msg);
    char *(*toString)(const void *this);
    char *(*codeGen)(const void *this, const char *name);
    void (*delete)(void *this);
    Types type;
    struct Vector *qualifiers; // Vector<Qualifiers*>
    unsigned char init : 1;
    unsigned char isCopy : 1;
    YYLTYPE loc;
};

struct FuncType {
    Type super;
    AST *ast;
    struct Vector *generics; // Vector<char*>
    struct Vector *args;     // Vector<Type*>
    Type *ret_type;
    struct FuncType *next;   // NULLable
};

struct ClassType {
    Type super;
    char *name; // NULLable
    struct Vector *generics; // Vector<char*>
    struct Vector *supers;   // Vector<char*>
    struct Vector *fields;   // Vector<Field*>
    struct Vector *ctors;    // Vector<Vector<Type*>>
    // NULL until verify() is executed:
    struct Map *fieldTypes;  // Map<char*, Type*>
};

struct ObjectType {
    Type super;
    char *name;
    struct Vector *generics; // Vector<char*>
    struct ClassType *class;
};

struct TupleType {
    Type super;
    struct SparseVector *types; // Vector<Type*>
};

struct SpreadType {
    Type super;
    struct SparseVector *types; // Vector<Type*>
};

struct NoneType {
    Type super;
};

struct ArrayType {
    Type super;
    Type *type;
};

struct MaybeType {
    Type super;
    Type *type;
};

enum BUILTIN_TYPE {
    BUILTIN_INT = 1 << 0,
    BUILTIN_BOOL = 1 << 1,
    BUILTIN_DOUBLE = 1 << 2,
    BUILTIN_STRING = 1 << 3
};
#define NUM_BUILTINS 4

typedef struct TypeCheckState {
    struct Map *symbols;    // Map<char*, Type*>
    // Used inside control flow statements to add newly defined symbols to
    // the outer scope if they are defined in all code paths. Defaults to
    // NULL and allocation and destruction must be handled by the control
    // flow AST node.
    struct Map *newInitSymbols; // Map<char*, NULL>
    struct Vector *classes;     // Vector<const struct ClassType*>
    struct Vector *functions;   // Vector<const struct FuncType*>
    const struct ClassType *builtins[NUM_BUILTINS];
    struct Map *compare; // Map<Type**, Map<Type**, int>>
    // NULL if not in function, otherwise return type of current function
    Type *funcType;
    // NULL initially, gets set by return statements. Used to tell if there are
    // conflicting return types or if a non-void function didn't return a
    // value on all code paths.
    Type *retType;
} TypeCheckState;

typedef struct CodeGenState {
    int indent;
    unsigned int tempCount;
    unsigned int funcCount;
    struct Map *funcIDs;      // Map<const struct FuncType*, char*>
} CodeGenState;

void
json_type(const Type *type, FILE *out, int indent);

void
json_qualifier(const Qualifiers *value, FILE *out, int indent);

void
delete_type(Type *type);

void
delete_ClassType(struct ClassType *this);

int
compare_ClassType(const struct ClassType *this,
    const struct ClassType *other,
    const struct TypeCheckState *state);

Type *
copy_type(Type *type);

int
TypeCompare(const Type *type1, const Type *type2, const TypeCheckState *state);

void
AddComparison(const struct ClassType *type, TypeCheckState *state);

/*
 * Add a symbol and its type to the state's symbol table. If there is a name
 * conflict, returns 1. Otherwise, returns 0.
 * If the symbol already exists, is a function, and the added type is also a
 * function, combines the two functions into one overloaded symbol. This
 * means f(int) and f(bool) can both refer to unique functions under the
 * same symbol "f". In this case, the passed type is freed by the AddSymbol
 * function.
 */
int
AddSymbol(struct Map *symbols,
    const char *symbol,
    size_t len,
    Type *type,
    const TypeCheckState *state,
    char **msg);

#define FuncType(loc, gen, args, ret) \
    new_FuncType(loc, gen, args, ret)
Type *
new_FuncType(YYLTYPE loc,
    struct Vector *generics,
    struct Vector *args,
    Type *ret_type);

#define ClassType(loc, gen, sup, fields, ctors) \
    new_ClassType(loc, gen, sup, fields, ctors)
Type *
new_ClassType(YYLTYPE loc,
    struct Vector *generics,
    struct Vector *supers,
    struct Vector *fields,
    struct Vector *ctors);

#define ObjectType(loc, name, gen) \
    new_ObjectType(loc, name, gen)
Type *
new_ObjectType(YYLTYPE loc, char *name, struct Vector *generics);

#define TupleType(loc, types) \
    new_TupleType(loc, types)
Type *
new_TupleType(YYLTYPE loc, struct SparseVector *types);

#define SpreadType(tuple) \
    new_SpreadType(tuple)
Type *
new_SpreadType(struct TupleType *tuple);

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
