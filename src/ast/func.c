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
    AST super;
    Vector *generics; // Vector<char*>
    Vector *args;     // Vector<Field*>
    Type *ret_type;
    Vector *stmts;    // Vector<AST*>
    Map *symbols;     // NULL until type checker is executed.
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTFunc *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("func", out, indent);
    json_comma(out, indent);
    json_label("generics", out);
    json_vector(ast->generics, (JSON_VALUE_FUNC)json_string, out, indent);
    json_comma(out, indent);
    json_label("args", out);
    json_vector(ast->args, (JSON_VALUE_FUNC)json_field, out, indent);
    json_comma(out, indent);
    json_label("ret_type", out);
    json_type(ast->ret_type, out, indent);
    json_comma(out, indent);
    json_label("statements", out);
    json_vector(ast->stmts, (JSON_VALUE_FUNC)json_AST, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, TypeCheckState *state, Type **typeptr) {
    ASTFunc *ast = this;
    int status = 0;
    char *msg;

    size_t ngen = Vector_size(ast->generics);
    if (ngen > 0) {
        // TODO: generic functions
        print_code_error(stderr,
            ast->super.loc,
            "generic func type checker not implemented");
        return 1;
    }
    Vector *args = Vector();
    ast->symbols = copy_Map(state->symbols, (MAP_COPY_FUNC)copy_type);
    size_t nargs = Vector_size(ast->args);
    for (size_t i = 0; i < nargs; i++) {
        struct Field *arg = Vector_get(ast->args, i);
        if (arg->type->verify(arg->type, state, &msg)) {
            print_code_error(stderr, ast->super.loc, msg);
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
                type_copy->init = 1;
                Type *prev_type = NULL;
                Map_put(ast->symbols, name, len, type_copy, &prev_type);
                if (NULL != prev_type) {
                    delete_type(prev_type);
                }
            }
        }
    }
    if (ast->ret_type->verify(ast->ret_type, state, &msg)) {
        print_code_error(stderr, ast->ret_type->loc, msg);
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
    state->funcType = ast->ret_type;
    state->symbols = ast->symbols;
    size_t nstmts = Vector_size(ast->stmts);
    for (size_t i = 0; i < nstmts; i++) {
        AST *stmt = Vector_get(ast->stmts, i);
        Type *type;
        if (stmt->getType(stmt, state, &type)) {
            status = 1;
        }
    }
    if (TYPE_NONE != ast->ret_type->type && NULL == state->retType) {
        char *typeName = ast->ret_type->toString(ast->ret_type);
        print_code_error(stderr,
            ast->ret_type->loc,
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
    Type *ret_type = copy_type(ast->ret_type);
    *typeptr = ast->super.type =
        FuncType(ast->super.loc, Vector(), args, ret_type);
    Vector_append(state->functions, ast->super.type);
    return 0;
}

static char *
codeGen(UNUSED void *this, UNUSED FILE *out, UNUSED CodeGenState *state) {
    return safe_strdup("/* FUNC NOT IMPLEMENTED */");
}

static void
delete(void *this) {
    ASTFunc *ast = this;
    delete_Vector(ast->generics, free);
    delete_Vector(ast->args, (VEC_DELETE_FUNC)delete_field);
    delete_type(ast->ret_type);
    delete_Vector(ast->stmts, (VEC_DELETE_FUNC)delete_AST);
    if (NULL != ast->super.type) {
        delete_type(ast->super.type);
    }
    if (NULL != ast->symbols) {
        delete_Map(ast->symbols, (MAP_DELETE_FUNC)delete_type);
    }
    free(this);
}

AST *
new_ASTFunc(YYLTYPE loc,
    Vector *generics,
    Vector *args,
    Type *ret_type,
    Vector *stmts) {
    ASTFunc *func = NULL;

    func = safe_malloc(sizeof(*func));
    *func = (ASTFunc){
        {
            json, getType, codeGen, delete, loc, NULL
        }, generics, args, ret_type, stmts, NULL
    };
    return (AST *)func;
}
