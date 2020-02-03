#include <stdlib.h>
#include "types.h"
#include "safe.h"
#include "vector.h"
#include "json.h"
#include "ast.h"

typedef enum Types {
    FUNC, CLASS, EXPR
} Types;

struct FuncType {
    Vector *generics; // Vector<char*>
    Vector *args;     // Vector<AST*>
    Type *ret_type;
};

struct ClassType {
    char *name;
    Vector *generics; // Vector<char*>
};

struct ExprType {
    AST *expr;
    Vector *generics; // Vector<char*>
};

struct Type {
    Types type;
    Vector *qualifiers; // Vector<Qualifiers*>
    union {
        struct FuncType func;
        struct ClassType class;
        struct ExprType expr;
    };
};

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
    }
    if (NULL != type->qualifiers) {
        json_comma(out, indent);
        json_label("qualifiers", out);
        json_list(type->qualifiers,
            (JSON_MAP_TYPE)json_qualifier,
            out,
            indent);
    }
    switch (type->type) {
        case FUNC:
            json_comma(out, indent);
            json_label("generics", out);
            json_list(type->func.generics,
                (JSON_MAP_TYPE)json_string,
                out,
                indent);
            json_comma(out, indent);
            json_label("args", out);
            json_list(type->func.args, (JSON_MAP_TYPE)json_AST, out, indent);
            json_comma(out, indent);
            json_label("return type", out);
            json_type(type->func.ret_type, out, indent);
            break;
        case CLASS:
            json_comma(out, indent);
            json_label("name", out);
            json_string(type->class.name, out, indent);
            json_comma(out, indent);
            json_label("generics", out);
            json_list(type->class.generics,
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
            json_list(type->expr.generics,
                (JSON_MAP_TYPE)json_string,
                out,
                indent);
            break;
    }
    json_end(out, &indent);
}

void
json_qualifier(Qualifiers *value, FILE *out, int indent) {
    switch (*value) {
        case CONST:
            json_string("const", out, indent);
            break;
        case FRIEND:
            json_string("friend", out, indent);
            break;
    }
}

void
delete_type(Type *type) {
    switch (type->type) {
        case FUNC:
            delete_Vector(type->func.generics, free);
            delete_Vector(type->func.args, (VEC_DELETE_TYPE)delete_AST);
            delete_type(type->func.ret_type);
            break;
        case CLASS:
            free(type->class.name);
            delete_Vector(type->class.generics, free);
            break;
        case EXPR:
            delete_AST(type->expr.expr);
            delete_Vector(type->expr.generics, free);
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
            expr, generics
        }
    };
    return t;
}
