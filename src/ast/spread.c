#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "parser.h"

typedef struct ASTSpread ASTSpread;

struct ASTSpread {
    AST super;
    AST *expr;
    Type *type; // NULL until type checker is executed.
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTSpread *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("spread", out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(ast->expr, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, TypeCheckState *state, UNUSED Type **typeptr) {
    ASTSpread *ast = this;
    Type *expr_type = NULL;

    if (ast->expr->getType(ast->expr, state, &expr_type)) {
        return 1;
    }
    if (TYPE_TUPLE != expr_type->type) {
        char *typeStr = expr_type->toString(expr_type);
        print_code_error(stderr,
            ast->expr->loc,
            "spread operator used on non-tuple %s type",
            typeStr);
        free(typeStr);
        return 1;
    }
    *typeptr = ast->type = SpreadType((struct TupleType *)expr_type);
    return 0;
}

static char *
codeGen(UNUSED void *this, UNUSED TypeCheckState *state) {
    return safe_strdup("/* NOT IMPLEMENTED */");
}

static void
delete(void *this) {
    ASTSpread *ast = this;
    delete_AST(ast->expr);
    if (NULL != ast->type) {
        delete_type(ast->type);
    }
    free(this);
}

AST *
new_ASTSpread(YYLTYPE loc, AST *expr) {
    ASTSpread *node = NULL;

    node = safe_malloc(sizeof(*node));
    *node = (ASTSpread){
        { json, getType, codeGen, delete, loc }, expr, NULL
    };
    return (AST *)node;
}
