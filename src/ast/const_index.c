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
    if (TYPE_TUPLE == type->type) {
        const struct TupleType *tuple = (const struct TupleType *)type;
        unsigned int n = SparseVector_count(tuple->types);
        if (ast->index < 0 || ast->index >= n) {
            print_code_error(stderr,
                ast->super.loc,
                "tuple indexed at %ld is out of range, tuple has %ld "
                "elements",
                ast->index,
                n);
            return 1;
        }
        SparseVector_at(tuple->types, ast->index, typeptr);
        return 0;
    } else if (TYPE_ARRAY == type->type) {
        const struct ArrayType *array = (const struct ArrayType *)type;
        array->type->qualifiers |= Q_MAYBE;
        *typeptr = ast->super.type = array->type;
        return 0;
    }
    char *typeName = type->toString(type);
    print_code_error(stderr,
        ast->super.loc,
        "index operator used on non-indexable object with type \"%s\"",
        typeName);
    free(typeName);
    return 1;
}

static char *
codeGen(UNUSED void *this, UNUSED FILE *out, UNUSED CodeGenState *state) {
    return safe_strdup("/* CONST INDEX NOT IMPLEMENTED */");
}

static void
delete(void *this) {
    ASTConstIndex *ast = this;
    delete_AST(ast->expr);
    free(this);
}

AST *
new_ASTConstIndex(YYLTYPE loc, AST *expr, long long int index) {
    ASTConstIndex *node = NULL;

    node = safe_malloc(sizeof(*node));
    *node = (ASTConstIndex){
        {
            json,
            getType,
            codeGen,
            delete,
            loc,
            NULL
        },
        expr,
        index
    };
    return (AST *)node;
}
