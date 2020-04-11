#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "parser.h"

typedef struct ASTIndex ASTIndex;

struct ASTIndex {
    AST super;
    AST *expr;
    AST *index;
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTIndex *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("index", out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(ast->expr, out, indent);
    json_comma(out, indent);
    json_label("index", out);
    json_AST(ast->index, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, UNUSED TypeCheckState *state, UNUSED Type **typeptr) {
    // TODO: array indexing
    ASTIndex *ast = this;
    print_code_error(stderr,
        ast->super.loc,
        "index type checker not implemented");
    return 1;
}

static char *
codeGen(UNUSED void *this, UNUSED FILE *out, UNUSED CodeGenState *state) {
    return safe_strdup("/* INDEX NOT IMPLEMENTED */");
}

static void
delete(void *this) {
    ASTIndex *ast = this;
    delete_AST(ast->expr);
    delete_AST(ast->index);
    free(this);
}

AST *
new_ASTIndex(YYLTYPE loc, AST *expr, AST *index) {
    ASTIndex *node = NULL;

    node = safe_malloc(sizeof(*node));
    *node = (ASTIndex){
        { json, getType, codeGen, delete, loc, NULL }, expr, index
    };
    return (AST *)node;
}
