#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTImpl ASTImpl;

struct ASTImpl {
    AST super;
    char *name;
    Vector *generics; // Vector<char*>
    Vector *stmts;    // Vector<AST*>
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTImpl *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("impl", out, indent);
    json_comma(out, indent);
    json_label("name", out);
    json_string(ast->name, out, indent);
    json_comma(out, indent);
    json_label("generics", out);
    json_vector(ast->generics, (JSON_VALUE_FUNC)json_string, out, indent);
    json_comma(out, indent);
    json_label("statements", out);
    json_vector(ast->stmts, (JSON_VALUE_FUNC)json_AST, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, UNUSED TypeCheckState *state, UNUSED Type **typeptr) {
    // TODO: class implementation
    ASTImpl *ast = this;
    print_code_error(stderr,
        ast->super.loc,
        "impl type checker not implemented");
    return 1;
}

static void
delete(void *this) {
    ASTImpl *ast = this;
    free(ast->name);
    delete_Vector(ast->generics, free);
    delete_Vector(ast->stmts, (VEC_DELETE_FUNC)delete_AST);
    free(this);
}

AST *
new_ASTImpl(YYLTYPE loc, char *name, Vector *generics, Vector *stmts) {
    ASTImpl *impl = NULL;

    impl = safe_malloc(sizeof(*impl));
    *impl = (ASTImpl){
        { json, getType, delete, loc }, name, generics, stmts
    };
    return (AST *)impl;
}
