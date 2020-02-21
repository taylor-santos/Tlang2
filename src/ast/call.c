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
    Vector *args;     // Vector<AST*>
    // NULL until type checker is executed:
    Vector *argTypes; // Vector<Type*>, types don't need to be deleted
    Type *type;
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
    int status = 0;
    if (ast->expr->getType(ast->expr, state, &funcType)) {
        return 1;
    }
    if (TYPE_FUNC != funcType->type) {
        char *typeName = funcType->toString(funcType);
        print_code_error(stderr,
            ast->super.loc,
            "function call attempted on non-function \"%s\" expression",
            typeName);
        free(typeName);
        return 1;
    }
    size_t ngiven = Vector_size(ast->args);
    ast->argTypes = new_Vector(ngiven);
    for (size_t j = 0; j < ngiven; j++) {
        AST *arg = Vector_get(ast->args, j);
        Type *givenType = NULL;
        if (arg->getType(arg, state, &givenType)) {
            status = 1;
            continue;
        }
        Vector_append(ast->argTypes, givenType);
    }
    if (status) {
        return 1;
    }
    int found = 0;
    for (struct FuncType *func = (struct FuncType *)funcType;
        NULL != func;
        func = func->next) {
        // TODO: generic functions
        size_t nargs = Vector_size(func->args);
        if (nargs != ngiven) {
            continue;
        }
        int valid = 1;
        for (size_t j = 0; j < nargs; j++) {
            Type *givenType = Vector_get(ast->argTypes, j),
                *expectType = Vector_get(func->args, j);
            if (givenType->compare(givenType, expectType, state)) {
                valid = 0;
                break;
            }
        }
        if (!valid) {
            continue;
        }
        if (found) {
            print_code_error(stderr,
                ast->super.loc,
                "overloaded function call is ambiguous");
            status = 1;
            continue;
        }
        found = 1;
        *typeptr = ast->type = func->ret_type->copy(func->ret_type);
    }
    if (0 == found) {
        dstring str = dstring("no matching function call with argument type");
        if (ngiven > 1) {
            append_char(&str, 's');
        }
        char *sep = " (";
        for (size_t i = 0; i < ngiven; i++) {
            Type *givenType = Vector_get(ast->argTypes, i);
            char *typeName = givenType->toString(givenType);
            append_vstr(&str, "%s%s", sep, typeName);
            free(typeName);
            sep = ", ";
        }
        append_str(&str, ")");
        print_code_error(stderr, ast->super.loc, str.str);
        free(str.str);
        status = 1;
    }
    return status;
}

static void
delete(void *this) {
    ASTCall *ast = this;
    delete_AST(ast->expr);
    delete_Vector(ast->args, (VEC_DELETE_FUNC)delete_AST);
    if (NULL != ast->type) {
        delete_type(ast->type);
        delete_Vector(ast->argTypes, NULL);
    }
    free(this);
}

AST *
new_ASTCall(YYLTYPE loc, AST *expr, Vector *args) {
    ASTCall *call = NULL;

    call = safe_malloc(sizeof(*call));
    *call = (ASTCall){
        { json, getType, delete, loc }, expr, args, NULL, NULL
    };
    return (AST *)call;
}
