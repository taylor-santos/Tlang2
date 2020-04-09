#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "parser.h"
#include "map.h"

typedef struct ASTIf ASTIf;

struct Vector;

struct ASTIf {
    AST super;
    AST *cond;
    Vector *trueStmts;  // Vector<AST*>
    Vector *falseStmts; // Vector<AST*>
    // NULL until type checker is executed:
    Type *type;
    Map *trueSymbols;
    Map *falseSymbols;
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTIf *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("if", out, indent);
    json_comma(out, indent);
    json_label("cond", out);
    json_AST(ast->cond, out, indent);
    json_comma(out, indent);
    json_label("true stmts", out);
    json_vector(ast->trueStmts, (JSON_VALUE_FUNC)json_AST, out, indent);
    json_comma(out, indent);
    json_label("false stmts", out);
    json_vector(ast->falseStmts, (JSON_VALUE_FUNC)json_AST, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, TypeCheckState *state, UNUSED Type **typeptr) {
    ASTIf *ast = this;
    Type *condType;
    int status = 0;

    if (ast->cond->getType(ast->cond, state, &condType)) {
        status = 1;
    } else if (TYPE_OBJECT != condType->type) {
        char *typeName = condType->toString(condType);
        print_code_error(stderr,
            ast->cond->loc,
            "invalid condition type \"%s\"",
            typeName);
        free(typeName);
        status = 1;
    } else {
        const struct ObjectType *object = (const struct ObjectType *)condType;
        const struct ClassType *class = object->class;
        const char *fieldName = "=>";
        Type *fieldType;
        if (Map_get(class->fieldTypes,
            fieldName,
            strlen(fieldName),
            &fieldType)) {
            char *typeName = condType->toString(condType);
            print_code_error(stderr,
                ast->cond->loc,
                "conditional expression with type \"%s\" does not implement"
                " an explicit cast to bool",
                typeName);
            free(typeName);
            status = 1;
        } else {
            int found = 0;
            for (struct FuncType *func = (struct FuncType *)fieldType;
                NULL != func;
                func = func->next) {
                const struct ObjectType
                    *ret = (const struct ObjectType *)func->ret_type;
                if (TYPE_OBJECT == func->ret_type->type &&
                    !ret->class->super.compare(ret->class,
                        state->builtins[BUILTIN_BOOL],
                        state)) {
                    // Found right cast
                    found = 1;
                    break;
                }
            }
            if (!found) {
                char *typeName = condType->toString(condType);
                print_code_error(stderr,
                    ast->cond->loc,
                    "expression with type \"%s\" does not implement a cast "
                    "to type \"bool\"",
                    typeName);
                free(typeName);
                status = 1;
            }
        }
    }
    Map *prevSymbols = state->symbols;
    Map *prevNewInit = state->newInitSymbols;
    Type *prevRetType = state->retType;
    size_t nTrue = Vector_size(ast->trueStmts);
    size_t nFalse = Vector_size(ast->falseStmts);
    Map *trueNewInit = Map(), *falseNewInit = Map();

    // True Branch
    if (nTrue != 0) {
        ast->trueSymbols = copy_Map(prevSymbols, (MAP_COPY_FUNC)copy_type);
        state->symbols = ast->trueSymbols;
        state->retType = NULL;
    }
    state->newInitSymbols = trueNewInit;
    for (size_t i = 0; i < nTrue; i++) {
        AST *stmt = Vector_get(ast->trueStmts, i);
        Type *type;
        if (stmt->getType(stmt, state, &type)) {
            status = 1;
        }
    }
    Type *trueRetType = state->retType;

    // False Branch
    if (nFalse != 0) {
        ast->falseSymbols = copy_Map(prevSymbols, (MAP_COPY_FUNC)copy_type);
        state->symbols = ast->falseSymbols;
        state->retType = NULL;
    }
    state->newInitSymbols = falseNewInit;
    for (size_t i = 0; i < nFalse; i++) {
        AST *stmt = Vector_get(ast->falseStmts, i);
        Type *type;
        if (stmt->getType(stmt, state, &type)) {
            status = 1;
        }
    }
    Type *falseRetType = state->retType;

    if (NULL != prevRetType) {
        state->retType = prevRetType;
    } else if (NULL != trueRetType && NULL != falseRetType) {
        state->retType = trueRetType;
    } else {
        state->retType = NULL;
    }
    state->newInitSymbols = prevNewInit;
    state->symbols = prevSymbols;

    Iterator *it = Map_iterator(trueNewInit);
    while (it->hasNext(it)) {
        MapIterData symbol = it->next(it);
        Type *type;
        if (!Map_get(state->symbols, symbol.key, symbol.len, &type) &&
            Map_contains(falseNewInit, symbol.key, symbol.len)) {
            type->init = 1;
            if (NULL != state->newInitSymbols) {
                Map_put(state->newInitSymbols,
                    symbol.key,
                    symbol.len,
                    NULL,
                    NULL);
            }
        }
    }
    it->delete(it);
    delete_Map(trueNewInit, NULL);
    delete_Map(falseNewInit, NULL);

    return status;
}

static char *
codeGen(UNUSED void *this, UNUSED TypeCheckState *state) {
    return safe_strdup("/* NOT IMPLEMENTED */");
}

static void
delete(void *this) {
    ASTIf *ast = this;
    delete_AST(ast->cond);
    delete_Vector(ast->trueStmts, (VEC_DELETE_FUNC)delete_AST);
    delete_Vector(ast->falseStmts, (VEC_DELETE_FUNC)delete_AST);
    if (NULL != ast->type) {
        delete_type(ast->type);
    }
    if (NULL != ast->trueSymbols) {
        delete_Map(ast->trueSymbols, (MAP_DELETE_FUNC)delete_type);
    }
    if (NULL != ast->falseSymbols) {
        delete_Map(ast->falseSymbols, (MAP_DELETE_FUNC)delete_type);
    }
    free(this);
}

AST *
new_ASTIf(YYLTYPE loc, AST *cond, Vector *trueStmts, Vector *falseStmts) {
    ASTIf *node = NULL;

    node = safe_malloc(sizeof(*node));
    *node = (ASTIf){
        { json, getType, codeGen, delete, loc },
        cond,
        trueStmts,
        falseStmts,
        NULL,
        NULL,
        NULL
    };
    return (AST *)node;
}
