#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTReturn ASTReturn;

struct ASTReturn {
    AST super;
    AST *expr;  // NULLable
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTReturn *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("return", out, indent);
    if (NULL != ast->expr) {
        json_comma(out, indent);
        json_label("expr", out);
        json_AST(ast->expr, out, indent);
    }
    json_end(out, &indent);
}

static int
getType(void *this, TypeCheckState *state, Type **typeptr) {
    ASTReturn *ast = this;
    if (NULL == state->funcType) {
        print_code_error(stderr,
            ast->super.loc,
            "%s",
            "return statement outside of function");
        return 1;
    }
    if (NULL != ast->expr) {
        Type *retType = NULL;
        if (ast->expr->getType(ast->expr, state, &retType)) {
            return 1;
        }
        if (retType->compare(retType, state->funcType, state)) {
            char *expectName = state->funcType->toString(state->funcType);
            char *givenName = retType->toString(retType);
            print_code_error(stderr,
                ast->super.loc,
                "function's return type is \"%s\" but returned value has "
                "type \"%s\"",
                expectName,
                givenName);
            free(expectName);
            free(givenName);
            return 1;
        }
        *typeptr = state->retType = retType;
        return 0;
    }
    // Returns nothing
    if (TYPE_NONE != state->funcType->type) {
        char *expectName = state->funcType->toString(state->funcType);
        print_code_error(stderr,
            ast->super.loc,
            "empty return statement in a function that returns \"%s\"",
            expectName);
        free(expectName);
        return 1;
    }
    return 0;
}

static char *
codeGen(void *this, FILE *out, CodeGenState *state) {
    ASTReturn *ast = this;
    if (NULL == ast->expr) {
        fprintf(out, "%*s", state->indent * 4, "");
        fprintf(out, "return NULL;\n");
    } else {
        char *code = ast->expr->codeGen(ast->expr, out, state);

        fprintf(out, "%*s", state->indent * 4, "");
        fprintf(out, "return %s;\n", code);
        free(code);
    }
    return NULL;
}

static void
delete(void *this) {
    ASTReturn *ast = this;
    if (NULL != ast->expr) {
        delete_AST(ast->expr);
    }
    free(this);
}

AST *
new_ASTReturn(YYLTYPE loc, AST *expr) {
    ASTReturn *ret = NULL;

    ret = safe_malloc(sizeof(*ret));
    *ret = (ASTReturn){
        {
            json,
            getType,
            codeGen,
            delete,
            loc,
            NULL
        },
        expr
    };
    return (AST *)ret;
}
