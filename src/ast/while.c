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
    json_vector(ast->stmts, (JSON_MAP_TYPE)json_AST, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, TypeCheckState *state, UNUSED Type **typeptr) {
    ASTWhile *ast = this;
    Type *condType;
    int status = 0;

    if (ast->cond->getType(ast->cond, state, &condType)) {
        status = 1;
    } else if (typeOf(condType) != TYPE_OBJECT) {
        char *typeName = typeToString(condType);
        print_code_error(stderr,
            ast->cond->loc,
            "invalid condition type \"%s\"",
            typeName);
        free(typeName);
        status = 1;
    } else {
        const struct ObjectType *object = getTypeData(condType);
        const struct ClassType *class = object->class;
        const char *fieldName = "toBool";
        Type *fieldType;
        if (Map_get(class->fields, fieldName, strlen(fieldName), &fieldType)) {
            print_code_error(stderr,
                ast->cond->loc,
                "conditional expression type does not implement a toBool() "
                "method");
            status = 1;
        } else if (typeOf(fieldType) != TYPE_FUNC) {
            print_code_error(stderr,
                ast->cond->loc,
                "conditional expression has a toBool field, but it is not a "
                "method");
            status = 1;
        } else {
            const struct FuncType *funcType = getTypeData(fieldType);
            size_t ngens = Vector_size(funcType->generics);
            size_t nargs = Vector_size(funcType->args);
            if (ngens != 0) {
                print_code_error(stderr,
                    ast->cond->loc,
                    "conditional expression's toBool() method cannot be a "
                    "generic function");
                status = 1;
            }
            if (nargs != 0) {
                print_code_error(stderr,
                    ast->cond->loc,
                    "conditional expression's toBool() method must take zero"
                    " arguments");
                status = 1;
            }
            Type *retType = funcType->ret_type;
            const struct ObjectType *retObj = getTypeData(retType);
            if (typeOf(retType) != TYPE_OBJECT ||
                retObj->class != state->builtins[BUILTIN_BOOL]) {
                print_code_error(stderr,
                    ast->cond->loc,
                    "conditional expression's toBool() method must return a "
                    "boolean value");
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
