#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "parser.h"

typedef struct ASTSpread ASTSpread;

struct ASTSpread {
    void (*json)(const ASTSpread *this, FILE *out, int indent);
    int (*getType)(ASTSpread *this,
        UNUSED TypeCheckState *state,
        Type **typeptr);
    void (*delete)(ASTSpread *this);
    struct YYLTYPE loc;
    AST *expr;
    Type *type; // NULL until type checker is executed.
};

static void
json(const ASTSpread *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("spread", out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(this->expr, out, indent);
    json_end(out, &indent);
}

static int
getType(ASTSpread *this, TypeCheckState *state, UNUSED Type **typeptr) {
    Type *expr_type = NULL;

    if (getType_AST(this->expr, state, &expr_type)) {
        return 1;
    }
    if (typeOf(expr_type) != TYPE_TUPLE) {
        char *typeStr = typeToString(expr_type);
        print_code_error(stderr,
            getLoc_AST(this->expr),
            "spread operator used on non-tuple %s type",
            typeStr);
        free(typeStr);
        return 1;
    }
    *typeptr = this->type = SpreadType(expr_type);
    return 0;
}

static void
delete(ASTSpread *this) {
    delete_AST(this->expr);
    if (NULL != this->type) {
        delete_type(this->type);
    }
    free(this);
}

AST *
new_ASTSpread(struct YYLTYPE *loc, AST *expr) {
    ASTSpread *node = NULL;

    node = safe_malloc(sizeof(*node));
    *node = (ASTSpread){
        json, getType, delete, *loc, expr, NULL
    };
    return (AST *)node;
}
