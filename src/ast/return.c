#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTReturn ASTReturn;

struct ASTReturn {
    void (*json)(const ASTReturn *this, FILE *out, int indent);
    int (*getType)(ASTReturn *this,
        UNUSED TypeCheckState *state,
        Type **typeptr);
    void (*delete)(ASTReturn *this);
    struct YYLTYPE loc;
    AST *expr;  // NULLable
    Type *type; // NULL until type checker is executed.
};

static void
json(const ASTReturn *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("return", out, indent);
    if (NULL != this->expr) {
        json_comma(out, indent);
        json_label("expr", out);
        json_AST(this->expr, out, indent);
    }
    json_end(out, &indent);
}

static int
getType(ASTReturn *this, TypeCheckState *state, Type **typeptr) {
    if (NULL == state->funcType) {
        print_code_error(stderr,
            this->loc,
            "return statement outside of function");
        return 1;
    }
    if (NULL != this->expr) {
        Type *retType = NULL;
        if (getType_AST(this->expr, state, &retType)) {
            return 1;
        }
        if (TypeCompare(retType, state->funcType, state)) {
            char *expectName = typeToString(state->funcType);
            char *givenName = typeToString(retType);
            print_code_error(stderr,
                this->loc,
                "function's return type is \"%s\" but returned value has "
                "type \"%s\"",
                expectName,
                givenName);
            free(expectName);
            free(givenName);
            return 1;
        }
        *typeptr = state->retType = this->type = copy_type(retType);
        return 0;
    }
    // Returns nothing
    if (TYPE_NONE != typeOf(state->funcType)) {
        char *expectName = typeToString(state->funcType);
        print_code_error(stderr,
            this->loc,
            "empty return statement in a function that returns \"%s\"",
            expectName);
        free(expectName);
        return 1;
    }
    return 0;
}

static void
delete(ASTReturn *this) {
    if (NULL != this->expr) {
        delete_AST(this->expr);
    }
    if (NULL != this->type) {
        delete_type(this->type);
    }
    free(this);
}

AST *
new_ASTReturn(struct YYLTYPE *loc, AST *expr) {
    ASTReturn *ret = NULL;

    ret = safe_malloc(sizeof(*ret));
    *ret = (ASTReturn){
        json, getType, delete, *loc, expr, NULL
    };
    return (AST *)ret;
}
