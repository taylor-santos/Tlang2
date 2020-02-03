#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"

typedef struct ASTReturn ASTReturn;

struct ASTReturn {
    void (*json)(const ASTReturn *this, FILE *out, int indent);
    void (*delete)(ASTReturn *this);
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

static void
delete(ASTReturn *this) {
    delete_AST(this->expr);
    free(this);
}

AST *
new_ASTReturn(AST *expr) {
    ASTReturn *ret = NULL;

    ret = safe_malloc(sizeof(*ret));
    *ret = (ASTReturn){
        json, delete, expr
    };
    return (AST *)ret;
}
