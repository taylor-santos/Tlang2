#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "types.h"
#include "vector.h"
#include "parser.h"
#include "map.h"

typedef struct ASTTypeStmt ASTTypeStmt;

struct ASTTypeStmt {
    AST super;
    Vector *vars; // Vector<char*>
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTTypeStmt *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("type declaration", out, indent);
    json_comma(out, indent);
    json_label("vars", out);
    json_vector(ast->vars, (JSON_VALUE_FUNC)json_string, out, indent);
    json_comma(out, indent);
    json_label("type", out);
    json_type(ast->super.type, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, TypeCheckState *state, UNUSED Type **typeptr) {
    ASTTypeStmt *ast = this;
    int status = 0;
    size_t nvars;
    char *msg;

    if (ast->super.type->verify(ast->super.type, state, &msg)) {
        print_code_error(stderr, ast->super.type->loc, msg);
        free(msg);
        return 1;
    }
    nvars = Vector_size(ast->vars);
    for (size_t i = 0; i < nvars; i++) {
        char *name = Vector_get(ast->vars, i);
        size_t len = strlen(name);
        Type *type_copy = copy_type(ast->super.type);
        if (AddSymbol(state->symbols, name, len, type_copy, state, &msg)) {
            print_code_error(stderr, ast->super.loc, msg);
            free(msg);
            status = 1;
        }
    }
    return status;
}

static char *
codeGen(UNUSED void *this, UNUSED FILE *out, UNUSED CodeGenState *state) {
    return safe_strdup("/* TYPE STMT NOT IMPLEMENTED */");
}

static void
delete(void *this) {
    ASTTypeStmt *ast = this;
    delete_Vector(ast->vars, free);
    delete_type(ast->super.type);
    free(this);
}

AST *
new_ASTTypeStmt(YYLTYPE loc, Vector *vars, Type *type) {
    ASTTypeStmt *named_type = NULL;

    named_type = safe_malloc(sizeof(*named_type));
    *named_type = (ASTTypeStmt){
        {
            json,
            getType,
            codeGen,
            delete,
            loc,
            type
        },
        vars
    };
    return (AST *)named_type;
}
