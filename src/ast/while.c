#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"
#include "map.h"

typedef struct ASTWhile ASTWhile;

struct ASTWhile {
    AST super;
    AST *cond;
    Vector *stmts;  // Vector<AST*>
    Type *type;     // NULL until type checker is executed.
    Map *symbols;   // NULL until type checker is executed.
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTWhile *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("while", out, indent);
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
    ASTWhile *ast = this;
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
        const struct ObjectType *object = (void *)condType->type;
        const struct ClassType *class = object->class;
        const char *fieldName = "=>";
        Type *fieldType;
        if (Map_get(class->fields, fieldName, strlen(fieldName), &fieldType)) {
            char *typeName = condType->toString(condType);
            print_code_error(stderr,
                ast->cond->loc,
                "conditional expression with type \"%s\" does not implement"
                " an explicit cast to bool",
                typeName);
            free(typeName);
            status = 1;
        } else {
            const struct FuncType *funcType = (void *)fieldType->type;
            Type *retType = funcType->ret_type;
            const struct ObjectType *retObj = (void *)retType->type;
            if (TYPE_OBJECT != retType->type ||
                retObj->class != state->builtins[BUILTIN_BOOL]) {
                char *typeName = condType->toString(condType);
                print_code_error(stderr,
                    ast->cond->loc,
                    "conditional expression with type \"%s\" does not implement"
                    " an explicit cast to bool",
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
    state->newInitSymbols = NULL;
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
    state->symbols = prevSymbols;
    state->newInitSymbols = prevInit;
    state->retType = prevRet;
    return status;
}

static void
delete(void *this) {
    ASTWhile *ast = this;
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
new_ASTWhile(YYLTYPE loc, AST *cond, struct Vector *stmts) {
    ASTWhile *node = NULL;

    node = safe_malloc(sizeof(*node));
    *node = (ASTWhile){
        { json, getType, delete, loc }, cond, stmts, NULL, NULL
    };
    return (AST *)node;
}
