#include <stdlib.h>
#include "types.h"
#include "safe.h"
#include "vector.h"
#include "sparse_vector.h"
#include "json.h"
#include "ast.h"

typedef enum Types {
    FUNC, CLASS, EXPR, TUPLE, NAMED
} Types;

struct FuncType {
    Vector *generics; // Vector<char*>
    Vector *args;     // Vector<Type*>
    Type *ret_type;   // NULLable
};

struct ClassType {
    char *name;
    Vector *generics; // Vector<char*>
};

struct ExprType {
    AST *expr;
    Vector *generics; // Vector<char*>
    int ownsAST;
};

struct TupleType {
    SparseVector *types; // Vector<Type*>
};

struct NamedType {
    char *name;
    Type *type;
};

struct Type {
    Types type;
    Vector *qualifiers; // Vector<Qualifiers*>
    union {
        struct FuncType func;
        struct ClassType class;
        struct ExprType expr;
        struct TupleType tuple;
        struct NamedType named;
    };
};

static Qualifiers *
copy_Qualifiers(const Qualifiers *q) {
    Qualifiers *new_q;

    if (NULL == (new_q = malloc(sizeof(*new_q)))) {
        print_ICE("");
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    *new_q = *q;
    return new_q;
}

void
json_type(const Type *type, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("type", out);
    switch (type->type) {
        case FUNC:
            json_string("function", out, indent);
            break;
        case CLASS:
            json_string("class", out, indent);
            break;
        case EXPR:
            json_string("expression", out, indent);
            break;
        case TUPLE:
            json_string("tuple", out, indent);
            break;
        case NAMED:
            json_string("named", out, indent);
            break;
    }
    if (NULL != type->qualifiers) {
        json_comma(out, indent);
        json_label("qualifiers", out);
        json_vector(type->qualifiers,
            (JSON_MAP_TYPE)json_qualifier,
            out,
            indent);
    }
    switch (type->type) {
        case FUNC:
            json_comma(out, indent);
            json_label("generics", out);
            json_vector(type->func.generics,
                (JSON_MAP_TYPE)json_string,
                out,
                indent);
            json_comma(out, indent);
            json_label("args", out);
            json_vector(type->func.args,
                (JSON_MAP_TYPE)json_type,
                out,
                indent);
            if (NULL != type->func.ret_type) {
                json_comma(out, indent);
                json_label("return type", out);
                json_type(type->func.ret_type, out, indent);
            }
            break;
        case CLASS:
            json_comma(out, indent);
            json_label("name", out);
            json_string(type->class.name, out, indent);
            json_comma(out, indent);
            json_label("generics", out);
            json_vector(type->class.generics,
                (JSON_MAP_TYPE)json_string,
                out,
                indent);
            break;
        case EXPR:
            json_comma(out, indent);
            json_label("expr", out);
            json_AST(type->expr.expr, out, indent);
            json_comma(out, indent);
            json_label("generics", out);
            json_vector(type->expr.generics,
                (JSON_MAP_TYPE)json_string,
                out,
                indent);
            break;
        case TUPLE:
            json_comma(out, indent);
            json_label("types", out);
            json_sparse_vector(type->tuple.types,
                (JSON_MAP_TYPE)json_type,
                out,
                indent);
            break;
        case NAMED:
            json_comma(out, indent);
            json_label("name", out);
            json_string(type->named.name, out, indent);
            json_comma(out, indent);
            json_label("type", out);
            json_type(type->named.type, out, indent);
            break;
    }
    json_end(out, &indent);
}

void
json_qualifier(const Qualifiers *value, FILE *out, int indent) {
    switch (*value) {
        case Q_CONST:
            json_string("const", out, indent);
            break;
        case Q_FRIEND:
            json_string("friend", out, indent);
            break;
    }
}

struct FuncType
copy_FuncType(struct FuncType func) {
    Vector *generics;
    Vector *args;
    Type *ret_type = NULL;

    generics = copy_Vector(func.generics, (VEC_COPY_FUNC)safe_strdup_func);
    args = copy_Vector(func.args, (VEC_COPY_FUNC)copy_type);
    ret_type = NULL;
    if (NULL != func.ret_type) {
        ret_type = copy_type(func.ret_type);
    }
    return (struct FuncType){
        generics, args, ret_type
    };
}

struct ClassType
copy_ClassType(struct ClassType class) {
    char *name;
    Vector *generics;

    name = safe_strdup(class.name);
    generics = copy_Vector(class.generics, (VEC_COPY_FUNC)safe_strdup_func);
    return (struct ClassType){
        name, generics
    };
}

struct ExprType
copy_ExprType(struct ExprType expr) {
    Vector *generics;
    generics = copy_Vector(expr.generics, (VEC_COPY_FUNC)safe_strdup_func);
    return (struct ExprType){
        expr.expr, generics, 0
    };
}

struct TupleType
copy_TupleType(struct TupleType tuple) {
    SparseVector *types;

    types = copy_SparseVector(tuple.types, (SVEC_COPY_FUNC)copy_type);
    return (struct TupleType){
        types
    };
}

struct NamedType
copy_NamedType(struct NamedType named) {
    char *name;
    Type *type;

    name = safe_strdup(named.name);
    type = copy_type(named.type);
    return (struct NamedType){
        name, type
    };
}

Type *
copy_type(const Type *type) {
    Type *new_type;
    Vector *qualifiers = NULL;

    if (NULL == (new_type = malloc(sizeof(*new_type)))) {
        print_ICE("");
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    if (NULL != type->qualifiers) {
        qualifiers =
            copy_Vector(type->qualifiers, (VEC_COPY_FUNC)copy_Qualifiers);
    }
    switch (type->type) {
        case FUNC:
            *new_type = (Type){
                FUNC, qualifiers, .func=copy_FuncType(type->func)
            };
            break;
        case CLASS:
            *new_type = (Type){
                CLASS, qualifiers, .class=copy_ClassType(type->class)
            };
            break;
        case EXPR:
            *new_type = (Type){
                EXPR, qualifiers, .expr=copy_ExprType(type->expr)
            };
            break;
        case TUPLE:
            *new_type = (Type){
                TUPLE, qualifiers, .tuple=copy_TupleType(type->tuple)
            };
            break;
        case NAMED:
            *new_type = (Type){
                NAMED, qualifiers, .named=copy_NamedType(type->named)
            };
            break;
    }
    return new_type;
}

void
delete_type(Type *type) {
    switch (type->type) {
        case FUNC:
            delete_Vector(type->func.generics, free);
            delete_Vector(type->func.args, (VEC_DELETE_FUNC)delete_type);
            if (NULL != type->func.ret_type) {
                delete_type(type->func.ret_type);
            }
            break;
        case CLASS:
            free(type->class.name);
            delete_Vector(type->class.generics, free);
            break;
        case EXPR:
            if (type->expr.ownsAST) {
                delete_AST(type->expr.expr);
            }
            delete_Vector(type->expr.generics, free);
            break;
        case TUPLE:
            delete_SparseVector(type->tuple.types,
                (VEC_DELETE_FUNC)delete_type);
            break;
        case NAMED:
            free(type->named.name);
            delete_type(type->named.type);
            break;
    }
    if (NULL != type->qualifiers) {
        delete_Vector(type->qualifiers, free);
    }
    free(type);
}

void
Type_setQualifiers(Type *type, Vector *qualifiers) {
    type->qualifiers = qualifiers;
}

Type *
new_FuncType(Vector *generics, Vector *args, Type *ret_type) {
    Type *t;

    t = safe_malloc(sizeof(*t));
    *t = (Type){
        FUNC, NULL, .func = {
            generics, args, ret_type
        }
    };
    return t;
}

Type *
new_ClassType(char *name, Vector *generics) {
    Type *t;

    t = safe_malloc(sizeof(*t));
    *t = (Type){
        CLASS, NULL, .class = {
            name, generics
        }
    };
    return t;
}

Type *
new_ExprType(AST *expr, Vector *generics) {
    Type *t;

    t = safe_malloc(sizeof(*t));
    *t = (Type){
        EXPR, NULL, .expr = {
            expr, generics, 1
        }
    };
    return t;
}

Type *
new_TupleType(SparseVector *types) {
    Type *t;

    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TUPLE, NULL, .tuple = {
            types
        }
    };
    return t;
}

Type *
new_NamedType(char *name, Type *type) {
    Type *t;

    t = safe_malloc(sizeof(*t));
    *t = (Type){
        NAMED, NULL, .named = {
            name, type
        }
    };
    return t;
}
