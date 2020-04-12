#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTDouble ASTDouble;

struct ASTDouble {
    AST super;
    double val;
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
    if (ast->super.type->verify(ast->super.type, state, &msg)) {
        print_code_error(stderr, ast->super.loc, "%s", msg);
        free(msg);
        return 1;
    }
    *typeptr = ast->super.type;
    return 0;
}

static char *
codeGen(void *this, UNUSED FILE *out, UNUSED CodeGenState *state) {
    ASTDouble *ast = this;
    return safe_asprintf("builtin_double(%f)", ast->val);
}

static void
delete(void *this) {
    ASTDouble *ast = this;
    delete_type(ast->super.type);
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
        {
            json,
            getType,
            codeGen,
            delete,
            loc,
            type
        },
        val
    };
    return (AST *)node;
}
