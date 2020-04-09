#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"
#include "map.h"

typedef struct ASTDo ASTDo;

struct ASTDo {
    AST super;
    AST *cond;
    Vector *stmts;  // Vector<AST*>
    Type *type;     // NULL until type checker is executed.
    Map *symbols;   // NULL until type checker is executed.
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTDo *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("do", out, indent);
    json_comma(out, indent);
    json_label("cond", out);
    json_AST(ast->cond, out, indent);
    json_comma(out, indent);
    json_label("stmts", out);
    json_vector(ast->stmts, (JSON_VALUE_FUNC)json_AST, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, TypeCheckState *state, UNUSED Type **typeptr) {
    ASTDo *ast = this;
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
    Map *prevInit = state->newInitSymbols;
    Type *prevRet = state->retType;
    state->retType = NULL;
    state->newInitSymbols = Map();
    size_t nstmts = Vector_size(ast->stmts);
    if (nstmts > 0) {
        state->symbols = ast->symbols =
            copy_Map(state->symbols, (MAP_COPY_FUNC)copy_type);
    }
    for (size_t i = 0; i < nstmts; i++) {
        AST *stmt = Vector_get(ast->stmts, i);
        Type *type;
        if (stmt->getType(stmt, state, &type)) {
            status = 1;
        }
    }
    Iterator *it = Map_iterator(state->newInitSymbols);
    while (it->hasNext(it)) {
        MapIterData symbol = it->next(it);
        Type *type = NULL;
        if (!Map_get(prevSymbols, symbol.key, symbol.len, &type)) {
            type->init = 1;
            if (NULL != prevInit) {
                Map_put(prevInit, symbol.key, symbol.len, NULL, NULL);
            }
        }
    }
    it->delete(it);
    delete_Map(state->newInitSymbols, NULL);
    if (NULL != state->retType) {
        prevRet = state->retType;
    }
    state->symbols = prevSymbols;
    state->newInitSymbols = prevInit;
    state->retType = prevRet;
    return status;
}

static char *
codeGen(UNUSED void *this, UNUSED TypeCheckState *state) {
    return safe_strdup("/* NOT IMPLEMENTED */");
}

static void
delete(void *this) {
    ASTDo *ast = this;
    delete_AST(ast->cond);
    delete_Vector(ast->stmts, (VEC_DELETE_FUNC)delete_AST);
    if (NULL != ast->type) {
        delete_type(ast->type);
    }
    if (NULL != ast->symbols) {
        delete_Map(ast->symbols, (MAP_DELETE_FUNC)delete_type);
    }
    free(this);
}

AST *
new_ASTDo(YYLTYPE loc, AST *cond, struct Vector *stmts) {
    ASTDo *node = NULL;

    node = safe_malloc(sizeof(*node));
    *node = (ASTDo){
        { json, getType, codeGen, delete, loc }, cond, stmts, NULL, NULL
    };
    return (AST *)node;
}
