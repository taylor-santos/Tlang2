#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "parser.h"

typedef struct ASTTuple ASTTuple;

struct ASTTuple {
    AST super;
    SparseVector *exprs; // Vector<AST*>
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTTuple *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("tuple", out, indent);
    json_comma(out, indent);
    json_label("elements", out);
    json_sparse_vector(ast->exprs, (JSON_VALUE_FUNC)json_AST, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, UNUSED TypeCheckState *state, Type **typeptr) {
    ASTTuple *ast = this;
    int status = 0;

    ull count = SparseVector_count(ast->exprs);
    if (1 == count) {
        //Tuple has single value, flatten it:
        AST *expr;
        SparseVector_at(ast->exprs, 0, &expr);
        Type *type;
        if (expr->getType(expr, state, &type)) {
            return 1;
        }
        *typeptr = ast->super.type = copy_type(type);
        return 0;
    }
    //Tuple has multiple values:
    size_t n = SparseVector_size(ast->exprs);
    SparseVector *types = new_SparseVector(n);
    for (size_t i = 0; i < n; i++) {
        Type *type, *type_copy;
        AST *expr;
        SparseVector_get(ast->exprs, i, &expr, &count);
        if (expr->getType(expr, state, &type)) {
            status = 1;
        } else {
            type_copy = copy_type(type);
            SparseVector_append(types, type_copy, count);
        }
    }
    SparseVector_reduce(types,
        (SVEC_COMPARE_FUNC)TypeCompare,
        state,
        (SVEC_DELETE_FUNC)delete_type);
    if (0 == status) {
        *typeptr = ast->super.type = TupleType(ast->super.loc, types);
    }
    return status;
}

static char *
codeGen(UNUSED void *this, UNUSED FILE *out, UNUSED CodeGenState *state) {
    return safe_strdup("/* TUPLE NOT IMPLEMENTED */");
}

static void
delete(void *this) {
    ASTTuple *ast = this;
    delete_SparseVector(ast->exprs, (SVEC_DELETE_FUNC)delete_AST);
    if (NULL != ast->super.type) {
        delete_type(ast->super.type);
    }
    free(this);
}

AST *
new_ASTTuple(YYLTYPE loc, SparseVector *exprs) {
    ASTTuple *tuple = NULL;

    tuple = safe_malloc(sizeof(*tuple));
    *tuple = (ASTTuple){
        { json, getType, codeGen, delete, loc, NULL }, exprs
    };
    return (AST *)tuple;
}
