#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "map.h"
#include "types.h"
#include "parser.h"

typedef struct ASTFunc ASTFunc;

struct ASTFunc {
    void (*json)(const ASTFunc *this, FILE *out, int indent);
    int (*getType)(ASTFunc *this,
        UNUSED TypeCheckState *state,
        Type **typeptr);
    void (*delete)(ASTFunc *this);
    struct YYLTYPE loc;
    Vector *generics; // Vector<char*>
    Vector *args;     // Vector<Field*>
    Type *ret_type;
    Vector *stmts;    // Vector<AST*>
    Type *type;       // NULL until type checker is executed.
    Map *symbols;     // NULL until type checker is executed.
};

static void
json(const ASTFunc *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("func", out, indent);
    json_comma(out, indent);
    json_label("generics", out);
    json_vector(this->generics, (JSON_MAP_TYPE)json_string, out, indent);
    json_comma(out, indent);
    json_label("args", out);
    json_vector(this->args, (JSON_MAP_TYPE)json_field, out, indent);
    json_comma(out, indent);
    json_label("ret_type", out);
    json_type(this->ret_type, out, indent);
    json_comma(out, indent);
    json_label("statements", out);
    json_vector(this->stmts, (JSON_MAP_TYPE)json_AST, out, indent);
    json_end(out, &indent);
}

static int
getType(ASTFunc *this, TypeCheckState *state, Type **typeptr) {
    int status = 0;
    char *msg;

    size_t ngen = Vector_size(this->generics);
    if (ngen > 0) {
        print_code_error(stderr,
            this->loc,
            "generic func type checker not implemented");
        return 1;
    }
    Vector *args = Vector();
    this->symbols = copy_Map(state->symbols, (MAP_COPY_FUNC)copy_type);
    size_t nargs = Vector_size(this->args);
    for (size_t i = 0; i < nargs; i++) {
        struct Field *arg = Vector_get(this->args, i);
        if (TypeVerify(arg->type, state, &msg)) {
            print_code_error(stderr, this->loc, msg);
            free(msg);
            status = 1;
        } else {
            Type *type_copy = copy_type(arg->type);
            Vector_append(args, type_copy);
            size_t nnames = Vector_size(arg->names);
            for (size_t j = 0; j < nnames; j++) {
                char *name = Vector_get(arg->names, j);
                size_t len = strlen(name);
                type_copy = copy_type(arg->type);
                setInit(type_copy, 1);
                Type *prev_type = NULL;
                Map_put(this->symbols, name, len, type_copy, &prev_type);
                if (NULL != prev_type) {
                    delete_type(prev_type);
                }
            }
        }
    }
    if (TypeVerify(this->ret_type, state, &msg)) {
        print_code_error(stderr, typeLoc(this->ret_type), msg);
        free(msg);
        status = 1;
    }
    if (status) {
        delete_Vector(args, (VEC_DELETE_FUNC)delete_type);
        return 1;
    }
    Type *prevFuncType = state->funcType;
    Type *prevRetType = state->retType;
    Map *prevSymbols = state->symbols;
    state->retType = NULL;
    state->funcType = this->ret_type;
    state->symbols = this->symbols;
    size_t nstmts = Vector_size(this->stmts);
    for (size_t i = 0; i < nstmts; i++) {
        AST *stmt = Vector_get(this->stmts, i);
        Type *type;
        if (getType_AST(stmt, state, &type)) {
            status = 1;
        }
    }
    if (TYPE_NONE != typeOf(this->ret_type) && NULL == state->retType) {
        char *typeName = typeToString(this->ret_type);
        print_code_error(stderr,
            typeLoc(this->ret_type),
            "function's return type is \"%s\" but not all code paths return a "
            "value",
            typeName);
        free(typeName);
        status = 1;
    }
    state->funcType = prevFuncType;
    state->retType = prevRetType;
    state->symbols = prevSymbols;
    if (status) {
        delete_Vector(args, (VEC_DELETE_FUNC)delete_type);
        return 1;
    }
    Type *ret_type = copy_type(this->ret_type);
    *typeptr = this->type = FuncType(&this->loc, Vector(), args, ret_type);
    return 0;
}

static void
delete(ASTFunc *this) {
    delete_Vector(this->generics, free);
    delete_Vector(this->args, (VEC_DELETE_FUNC)delete_field);
    delete_type(this->ret_type);
    delete_Vector(this->stmts, (VEC_DELETE_FUNC)delete_AST);
    if (NULL != this->type) {
        delete_type(this->type);
    }
    if (NULL != this->symbols) {
        delete_Map(this->symbols, (MAP_DELETE_FUNC)delete_type);
    }
    free(this);
}

AST *
new_ASTFunc(struct YYLTYPE *loc,
    Vector *generics,
    Vector *args,
    Type *ret_type,
    Vector *stmts) {
    ASTFunc *func = NULL;

    func = safe_malloc(sizeof(*func));
    *func = (ASTFunc){
        json,
        getType,
        delete,
        *loc,
        generics,
        args,
        ret_type,
        stmts,
        NULL,
        NULL
    };
    return (AST *)func;
}
