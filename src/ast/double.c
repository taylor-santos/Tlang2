#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTDouble ASTDouble;

struct ASTDouble {
    void (*json)(const ASTDouble *this, FILE *out, int indent);
    int (*getType)(const ASTDouble *this, Type **typeptr);
    void (*delete)(ASTDouble *this);
    struct YYLTYPE loc;
    double val;
};

static void
json(const ASTDouble *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("double", out, indent);
    json_comma(out, indent);
    json_label("val", out);
    json_double(this->val, out, indent);
    json_end(out, &indent);
}

static int
getType(const ASTDouble *this, UNUSED Type **typeptr) {
    print_code_error(&this->loc,
        "double type checker not implemented",
        stderr);
    return 1;
}

static void
delete(ASTDouble *this) {
    free(this);
}

AST *
new_ASTDouble(struct YYLTYPE *loc, double val) {
    ASTDouble *node = NULL;

    node = safe_malloc(sizeof(*node));
    *node = (ASTDouble){
        json, getType, delete, *loc, val
    };
    return (AST *)node;
}
