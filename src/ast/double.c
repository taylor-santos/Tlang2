#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTDouble ASTDouble;

struct ASTDouble {
    AST super;
    double val;
    Type *type;
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTDouble *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("double", out, indent);
    json_comma(out, indent);
    json_label("val", out);
    json_double(ast->val, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, UNUSED TypeCheckState *state, UNUSED Type **typeptr) {
    ASTDouble *ast = this;
    char *msg;
    if (ast->type->verify(ast->type, state, &msg)) {
        print_code_error(stderr, ast->super.loc, msg);
        free(msg);
        return 1;
    }
    *typeptr = ast->type;
    return 0;
}

static char *
codeGen(UNUSED void *this, UNUSED TypeCheckState *state) {
    return safe_strdup("/* NOT IMPLEMENTED */");
}

static void
delete(void *this) {
    ASTDouble *ast = this;
    delete_type(ast->type);
    free(this);
}

AST *
new_ASTDouble(YYLTYPE loc, double val) {
    ASTDouble *node = NULL;
    Type *type;

    type = new_ObjectType(loc, safe_strdup("double"), Vector());
    type->init = 1;
    node = safe_malloc(sizeof(*node));
    *node = (ASTDouble){
        { json, getType, codeGen, delete, loc }, val, type
    };
    return (AST *)node;
}
