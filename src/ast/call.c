#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "parser.h"

typedef struct ASTCall ASTCall;

struct ASTCall {
    void (*json)(const ASTCall *this, FILE *out, int indent);
    int (*getType)(const ASTCall *this, Type **typeptr);
    void (*delete)(ASTCall *this);
    struct YYLTYPE loc;
    AST *expr;
    AST *args; // NULLable
};

static void
json(const ASTCall *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("call", out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(this->expr, out, indent);
    if (NULL != this->args) {
        json_comma(out, indent);
        json_label("args", out);
        json_AST(this->args, out, indent);
    }
    json_end(out, &indent);
}

static int
getType(const ASTCall *this, UNUSED Type **typeptr) {
    print_code_error(&this->loc,
        "function call type checker not implemented",
        stderr);
    return 1;
}

static void
delete(ASTCall *this) {
    delete_AST(this->expr);
    if (NULL != this->args) {
        delete_AST(this->args);
    }
    free(this);
}

AST *
new_ASTCall(struct YYLTYPE *loc, AST *expr, AST *args) {
    ASTCall *call = NULL;

    call = safe_malloc(sizeof(*call));
    *call = (ASTCall){
        json, getType, delete, *loc, expr, args
    };
    return (AST *)call;
}
