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
    if (ast->super.type->verify(ast->super.type, state, &msg)) {
        print_code_error(stderr, ast->super.loc, msg);
        free(msg);
        return 1;
    }
    *typeptr = ast->super.type;
    return 0;
}

static char *
codeGen(void *this, FILE *out, CodeGenState *state) {
    ASTString *ast = this;
    char *tmp = safe_asprintf("temp%d", state->tempCount);
    state->tempCount++;
    fprintf(out, "%*s", state->indent * 4, "");
    fprintf(out, "char *%s;\n", tmp);
    fprintf(out, "%*s", state->indent * 4, "");
    fprintf(out, "if (NULL == (%s = strdup(\"%s\"))) {\n", tmp, ast->str.str);
    state->indent++;
    fprintf(out, "%*s", state->indent * 4, "");
    fprintf(out, "ERROR(\"strdup\");\n");
    state->indent--;
    fprintf(out, "%*s", state->indent * 4, "");
    fprintf(out, "}\n");
    char *ret = safe_asprintf("temp%d", state->tempCount);
    state->tempCount++;
    fprintf(out, "%*s", state->indent * 4, "");
    fprintf(out, "class_string %s = new_string(%s);\n", ret, tmp);
    free(tmp);
    return ret;
}

static void
delete(void *this) {
    ASTString *ast = this;
    delete_dstring(ast->str);
    delete_type(ast->super.type);
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
        {
            json,
            getType,
            codeGen,
            delete,
            loc,
            type
        },
        str
    };
    return (AST *)node;
}
