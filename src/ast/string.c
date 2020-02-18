#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "dynamic_string.h"
#include "parser.h"

typedef struct ASTString ASTString;

struct ASTString {
    AST super;
    dstring str;
    Type *type;
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTString *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("string", out, indent);
    json_comma(out, indent);
    json_label("val", out);
    json_dstring(&ast->str, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, UNUSED TypeCheckState *state, Type **typeptr) {
    ASTString *ast = this;
    char *msg;
    if (ast->type->verify(ast->type, state, &msg)) {
        print_code_error(stderr, ast->super.loc, msg);
        free(msg);
        return 1;
    }
    *typeptr = ast->type;
    return 0;
}

static void
delete(void *this) {
    ASTString *ast = this;
    delete_dstring(ast->str);
    delete_type(ast->type);
    free(this);
}

AST *
new_ASTString(YYLTYPE loc, dstring str) {
    ASTString *node = NULL;
    Type *type;

    type = ObjectType(loc, safe_strdup("string"), Vector());
    type->init = 1;
    node = safe_malloc(sizeof(*node));
    *node = (ASTString){
        { json, getType, delete, loc }, str, type
    };
    return (AST *)node;
}
