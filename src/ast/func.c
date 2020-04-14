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
    Map *locals;      // Map<char*, NULL>
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
            "%s",
            "generic func type checker not implemented");
        return 1;
    }
    Vector *args = Vector();
    Vector *argNames = Vector();
    ast->symbols = copy_Map(state->symbols, (MAP_COPY_FUNC)copy_type);
    ast->locals = Map();
    size_t nargs = Vector_size(ast->args);
    for (size_t i = 0; i < nargs; i++) {
        struct Field *arg = Vector_get(ast->args, i);
        if (arg->type->verify(arg->type, state, &msg)) {
            print_code_error(stderr, ast->super.loc, "%s", msg);
            free(msg);
            status = 1;
        } else {
            Type *type_copy = copy_type(arg->type);
            Vector_append(args, type_copy);
            size_t nnames = Vector_size(arg->names);
            for (size_t j = 0; j < nnames; j++) {
                char *name = Vector_get(arg->names, j);
                Vector_append(argNames, name);
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
        print_code_error(stderr, ast->ret_type->loc, "%s", msg);
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
    Map *prevNewSymbols = state->newSymbols;
    Map *prevUsedSymbols = state->usedSymbols;
    state->retType = NULL;
    state->funcType = ast->ret_type;
    state->symbols = ast->symbols;
    state->newSymbols = ast->locals;
    Map *used = state->usedSymbols = Map();
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
    state->newSymbols = prevNewSymbols;
    state->usedSymbols = prevUsedSymbols;
    if (status) {
        delete_Vector(args, (VEC_DELETE_FUNC)delete_type);
        return 1;
    }
    Iterator *it = Map_iterator(ast->locals);
    // Remove local variables and arguments from used variables, to produce a
    // list of used environment variables.
    while (it->hasNext(it)) {
        MapIterData data = it->next(it);
        Map_remove(used, data.key, data.len, NULL);
    }
    it->delete(it);
    nargs = Vector_size(argNames);
    for (size_t i = 0; i < nargs; i++) {
        char *arg = Vector_get(argNames, i);
        Map_remove(used, arg, strlen(arg), NULL);
    }
    delete_Vector(argNames, NULL);
    if (NULL != state->usedSymbols) {
        it = Map_iterator(used);
        while (it->hasNext(it)) {
            MapIterData data = it->next(it);
            Map_put(state->usedSymbols, data.key, data.len, NULL, NULL);
        }
        it->delete(it);
    }
    Type *ret_type = copy_type(ast->ret_type);
    *typeptr = ast->super.type =
        FuncType(ast->super.loc, Vector(), args, ret_type);
    struct FuncType *func = (struct FuncType *)*typeptr;
    func->ast = this;
    func->env = used;
    Vector_append(state->functions, ast->super.type);
    return 0;
}

void
codeGenFuncBody(void *this, FILE *out, struct CodeGenState *state) {
    ASTFunc *ast = this;
    Iterator *it = Map_iterator(ast->locals);
    while (it->hasNext(it)) {
        MapIterData data = it->next(it);
        char *symbol = data.key;
        size_t len = data.len;
        Type *type;
        Map_get(ast->symbols, symbol, len, &type);
        char *name = safe_asprintf("var_%.*s", (int)len, symbol);
        char *typeName = type->codeGen(type, name);
        free(name);
        fprintf(out, "%*s", state->indent * 4, "");
        fprintf(out, "%s;\n", typeName);
        free(typeName);
    }
    it->delete(it);

    struct FuncType *func = (struct FuncType *)ast->super.type;
    it = Map_iterator(func->env);
    int envID = 0;
    while (it->hasNext(it)) {
        MapIterData data = it->next(it);
        char *symbol = data.key;
        size_t len = data.len;
        Type *type;
        Map_get(ast->symbols, symbol, len, &type);
        char *name = safe_asprintf("var_%.*s", (int)len, symbol);
        char *typeName = type->codeGen(type, NULL);
        fprintf(out, "%*s", state->indent * 4, "");
        fprintf(out,
            "#define %s (*(%s*)env.env[%d])\n",
            name,
            typeName,
            envID++);
        free(name);
        free(typeName);
    }
    it->delete(it);

    size_t nargs = Vector_size(ast->args);
    int argi = 0;
    for (size_t i = 0; i < nargs; i++) {
        struct Field *arg = Vector_get(ast->args, i);
        size_t nnames = Vector_size(arg->names);
        for (size_t j = 0; j < nnames; j++) {
            char *name = Vector_get(arg->names, j);
            char *ident = safe_asprintf("var_%s", name);
            char *typeName = arg->type->codeGen(arg->type, ident);
            free(ident);
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "%s = args[%d];\n", typeName, argi++);
            free(typeName);
        }
    }
    fprintf(out, "\n");

    size_t nstmts = Vector_size(ast->stmts);
    for (size_t i = 0; i < nstmts; i++) {
        AST *stmt = Vector_get(ast->stmts, i);
        char *code = stmt->codeGen(stmt, out, state);
        free(code);
    }
    it = Map_iterator(func->env);
    while (it->hasNext(it)) {
        MapIterData data = it->next(it);
        char *symbol = data.key;
        size_t len = data.len;
        char *name = safe_asprintf("var_%.*s", (int)len, symbol);
        fprintf(out, "%*s", state->indent * 4, "");
        fprintf(out, "#undef %s\n", name);
        free(name);
    }
    it->delete(it);
}

static char *
codeGen(void *this, UNUSED FILE *out, CodeGenState *state) {
    ASTFunc *ast = this;
    struct FuncType *func = (struct FuncType *)ast->super.type;
    char *tmp = safe_asprintf("temp%d", state->tempCount);
    state->tempCount++;
    fprintf(out, "%*s", state->indent * 4, "");
    fprintf(out, "void *%s[] = {", tmp);
    Iterator *it = Map_iterator(func->env);
    char *sep = "";
    while (it->hasNext(it)) {
        MapIterData data = it->next(it);
        char *symbol = data.key;
        size_t len = data.len;
        fprintf(out, " %s&var_%.*s", sep, (int)len, symbol);
        sep = ",";
    }
    fprintf(out, " };\n");
    it->delete(it);
    char *name;
    Map_get(state->funcIDs, &func, sizeof(func), &name);
    char *ret = safe_asprintf("(closure){ %s, %s }", name, tmp);
    free(tmp);
    return ret;
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
    if (NULL != ast->locals) {
        delete_Map(ast->locals, NULL);
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
            json,
            getType,
            codeGen,
            delete,
            loc,
            NULL
        },
        generics,
        args,
        ret_type,
        stmts,
        NULL,
        NULL
    };
    return (AST *)func;
}
