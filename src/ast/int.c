#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTInt ASTInt;

struct ASTInt {
    AST super;
    long long int val;
    Type *type;
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTInt *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("int", out, indent);
    json_comma(out, indent);
    json_label("val", out);
    json_int(ast->val, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, TypeCheckState *state, Type **typeptr) {
    ASTInt *ast = this;
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
    ASTInt *ast = this;
    delete_type(ast->type);
    free(this);
}

AST *
new_ASTInt(YYLTYPE loc, long long int val) {
    ASTInt *node = NULL;
    Type *type;

    type = new_ObjectType(loc, safe_strdup("int"), Vector());
    setInit(type, 1);
    node = safe_malloc(sizeof(*node));
    *node = (ASTInt){
        { json, getType, delete, loc }, val, type
    };
    return (AST *)node;
}
