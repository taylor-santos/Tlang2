#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "parser.h"

typedef struct ASTCall ASTCall;

struct ASTCall {
    AST super;
    AST *expr;
    Vector *args; // Vector<AST*>
    Type *type;   // NULL until type checker is executed.
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTCall *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("call", out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(ast->expr, out, indent);
    json_comma(out, indent);
    json_label("args", out);
    json_vector(ast->args, (JSON_VALUE_FUNC)json_AST, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, TypeCheckState *state, UNUSED Type **typeptr) {
    ASTCall *ast = this;
    Type *funcType = NULL;
    if (ast->expr->getType(ast->expr, state, &funcType)) {
        return 1;
    }
    if (TYPE_FUNC != typeOf(funcType)) {
        char *typeName = typeToString(funcType);
        print_code_error(stderr,
            ast->super.loc,
            "function call attempted on non-function \"%s\" expression",
            typeName);
        free(typeName);
        return 1;
    }
    const struct FuncType *func = getTypeData(funcType);
    size_t ngen = Vector_size(func->generics);
    if (ngen > 0) {
        print_code_error(stderr,
            ast->super.loc,
            "generic function call type checker not implemented");
        return 1;
    }
    size_t nargs = Vector_size(func->args);
    size_t ngiven = Vector_size(ast->args);
    if (nargs != ngiven) {
        print_code_error(stderr,
            ast->super.loc,
            "%d argument%s given to a function that expects %d argument%s",
            ngiven,
            ngiven == 1
                ? ""
                : "s",
            nargs,
            nargs == 1
                ? ""
                : "s");
        return 1;
    }
    int status = 0;
    for (size_t i = 0; i < nargs; i++) {
        AST *arg = Vector_get(ast->args, i);
        Type *givenType = NULL;
        if (arg->getType(arg, state, &givenType)) {
            status = 1;
        } else {
            Type *expectType = Vector_get(func->args, i);
            if (TypeCompare(givenType, expectType, state)) {
                char *expectName = typeToString(expectType);
                char *givenName = typeToString(givenType);
                print_code_error(stderr,
                    arg->loc,
                    "incompatible argument type: expected \"%s\" but got "
                    "\"%s\"",
                    expectName,
                    givenName);
                free(expectName);
                free(givenName);
                status = 1;
            }
        }
    }
    if (status) {
        return 1;
    }
    *typeptr = ast->type = copy_type(func->ret_type);
    return 0;
}

static void
delete(void *this) {
    ASTCall *ast = this;
    delete_AST(ast->expr);
    delete_Vector(ast->args, (VEC_DELETE_FUNC)delete_AST);
    if (NULL != ast->type) {
        delete_type(ast->type);
    }
    free(this);
}

AST *
new_ASTCall(YYLTYPE loc, AST *expr, Vector *args) {
    ASTCall *call = NULL;

    call = safe_malloc(sizeof(*call));
    *call = (ASTCall){
        { json, getType, delete, loc }, expr, args, NULL
    };
    return (AST *)call;
}
