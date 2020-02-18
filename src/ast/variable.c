#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"
#include "map.h"

typedef struct ASTVariable ASTVariable;

struct ASTVariable {
    AST super;
    char *name;
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTVariable *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("variable", out, indent);
    json_comma(out, indent);
    json_label("name", out);
    json_string(ast->name, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, TypeCheckState *state, Type **typeptr) {
    ASTVariable *ast = this;
    Type *type = NULL;
    if (Map_get(state->symbols, ast->name, strlen(ast->name), &type)) {
        print_code_error(stderr,
            ast->super.loc,
            "unknown variable \"%s\"",
            ast->name);
        return 1;
    } else if (!type->init) {
        print_code_error(stderr,
            ast->super.loc,
            "variable \"%s\" used before initialization",
            ast->name);
        return 1;
    }
    *typeptr = type;
    return 0;
}

static void
delete(void *this) {
    ASTVariable *ast = this;
    free(ast->name);
    free(this);
}

AST *
new_ASTVariable(YYLTYPE loc, char *name) {
    ASTVariable *variable = NULL;

    variable = safe_malloc(sizeof(*variable));
    *variable = (ASTVariable){
        { json, getType, delete, loc }, name
    };
    return (AST *)variable;
}
