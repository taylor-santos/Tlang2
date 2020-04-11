#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTInt ASTInt;

struct ASTInt {
    AST super;
    long long int val;
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
    if (ast->super.type->verify(ast->super.type, state, &msg)) {
        print_code_error(stderr, ast->super.loc, msg);
        free(msg);
        return 1;
    }
    *typeptr = ast->super.type;
    return 0;
}

static char *
codeGen(void *this, UNUSED FILE *out, UNUSED CodeGenState *state) {
    ASTInt *ast = this;
    return safe_asprintf("new_int(%lld)", ast->val);
}

static void
delete(void *this) {
    ASTInt *ast = this;
    delete_type(ast->super.type);
    free(this);
}

AST *
new_ASTInt(YYLTYPE loc, long long int val) {
    ASTInt *node = NULL;
    Type *type;

    type = new_ObjectType(loc, safe_strdup("int"), Vector());
    type->init = 1;
    node = safe_malloc(sizeof(*node));
    *node = (ASTInt){
        { json, getType, codeGen, delete, loc, type }, val
    };
    return (AST *)node;
}
