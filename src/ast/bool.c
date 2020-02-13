#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTBool ASTBool;

struct ASTBool {
    AST super;
    int val;
    Type *type;
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTBool *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("bool", out, indent);
    json_comma(out, indent);
    json_label("val", out);
    if (ast->val) {
        json_string("true", out, indent);
    } else {
        json_string("false", out, indent);
    }
    json_end(out, &indent);
}

static int
getType(void *this, UNUSED TypeCheckState *state, UNUSED Type **typeptr) {
    ASTBool *ast = this;
    char *msg;
    if (TypeVerify(ast->type, state, &msg)) {
        print_code_error(stderr, ast->super.loc, msg);
        free(msg);
        return 1;
    }
    *typeptr = ast->type;
    return 0;
}

static void
delete(void *this) {
    ASTBool *ast = this;
    delete_type(ast->type);
    free(ast);
}

AST *
new_ASTBool(YYLTYPE loc, int val) {
    ASTBool *node = NULL;
    Type *type;

    type = new_ObjectType(loc, safe_strdup("bool"), Vector());
    setInit(type, 1);
    node = safe_malloc(sizeof(*node));
    *node = (ASTBool){
        { json, getType, delete, loc }, val, type
    };
    return (AST *)node;
}
