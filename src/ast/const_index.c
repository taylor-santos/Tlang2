#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "parser.h"

typedef struct ASTConstIndex ASTConstIndex;

struct ASTConstIndex {
    AST super;
    AST *expr;
    long long int index;
    Type *type; // NULL until type checker is executed.
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTConstIndex *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("const index", out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(ast->expr, out, indent);
    json_comma(out, indent);
    json_label("index", out);
    json_int(ast->index, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, TypeCheckState *state, Type **typeptr) {
    ASTConstIndex *ast = this;
    Type *type = NULL;
    if (ast->expr->getType(ast->expr, state, &type)) {
        return 1;
    }
    if (TYPE_ARRAY != type->type) {
        char *typeName = type->toString(type);
        print_code_error(stderr,
            ast->super.loc,
            "index operator used on non-array object with type \"%s\"",
            typeName);
        free(typeName);
        return 1;
    }
    const struct ArrayType *array = (void *)type->type;
    *typeptr = ast->type = MaybeType(ast->super.loc, copy_type(array->type));
    return 0;
}

static void
delete(void *this) {
    ASTConstIndex *ast = this;
    delete_AST(ast->expr);
    if (NULL != ast->type) {
        delete_type(ast->type);
    }
    free(this);
}

AST *
new_ASTConstIndex(YYLTYPE loc, AST *expr, long long int index) {
    ASTConstIndex *node = NULL;

    node = safe_malloc(sizeof(*node));
    *node = (ASTConstIndex){
        { json, getType, delete, loc }, expr, index, NULL
    };
    return (AST *)node;
}
