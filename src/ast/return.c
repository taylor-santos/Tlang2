#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTReturn ASTReturn;

struct ASTReturn {
    void (*json)(const ASTReturn *this, FILE *out, int indent);
    int (*getType)(const ASTReturn *this, Type **typeptr);
    void (*delete)(ASTReturn *this);
    struct YYLTYPE loc;
    AST *expr;
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
getType(const ASTReturn *this, UNUSED Type **typeptr) {
    print_code_error(&this->loc,
        "return type checker not implemented",
        stderr);
    return 1;
}

static void
delete(ASTReturn *this) {
    delete_AST(this->expr);
    free(this);
}

AST *
new_ASTReturn(struct YYLTYPE *loc, AST *expr) {
    ASTReturn *ret = NULL;

    ret = safe_malloc(sizeof(*ret));
    *ret = (ASTReturn){
        json, getType, delete, *loc, expr
    };
    return (AST *)ret;
}
