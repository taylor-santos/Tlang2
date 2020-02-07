#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTBool ASTBool;

struct ASTBool {
    void (*json)(const ASTBool *this, FILE *out, int indent);
    int (*getType)(ASTBool *this,
        UNUSED TypeCheckState *state,
        Type **typeptr);
    void (*delete)(ASTBool *this);
    struct YYLTYPE loc;
    int val;
};

static void
json(const ASTBool *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("bool", out, indent);
    json_comma(out, indent);
    json_label("val", out);
    if (this->val) {
        json_string("true", out, indent);
    } else {
        json_string("false", out, indent);
    }
    json_end(out, &indent);
}

static int
getType(ASTBool *this, UNUSED TypeCheckState *state, UNUSED Type **typeptr) {
    print_code_error(stderr, this->loc, "bool type checker not implemented");
    return 1;
}

static void
delete(ASTBool *this) {
    free(this);
}

AST *
new_ASTBool(struct YYLTYPE *loc, int val) {
    ASTBool *node = NULL;

    node = safe_malloc(sizeof(*node));
    *node = (ASTBool){
        json, getType, delete, *loc, val
    };
    return (AST *)node;
}
