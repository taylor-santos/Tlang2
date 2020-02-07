#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "parser.h"

typedef struct ASTTuple ASTTuple;

struct ASTTuple {
    void (*json)(const ASTTuple *this, FILE *out, int indent);
    int (*getType)(ASTTuple *this,
        UNUSED TypeCheckState *state,
        Type **typeptr);
    void (*delete)(ASTTuple *this);
    struct YYLTYPE loc;
    Vector *exprs; // Vector<AST*>
    Type *type;    // NULL until type checker is executed.
};

static void
json(const ASTTuple *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("tuple", out, indent);
    json_comma(out, indent);
    json_label("elements", out);
    json_vector(this->exprs, (JSON_MAP_TYPE)json_AST, out, indent);
    json_end(out, &indent);
}

static int
getType(ASTTuple *this, UNUSED TypeCheckState *state, Type **typeptr) {
    SparseVector *types;
    size_t n;
    int status = 0;

    n = Vector_size(this->exprs);
    if (1 == n) {
        //Tuple has single value, flatten it:
        Type *type;
        AST *expr = NULL;

        Vector_get(this->exprs, 0, &expr);
        if (getType_AST(expr, state, &type)) {
            return 1;
        }
        *typeptr = this->type = copy_type(type);
        return 0;
    }
    //Tuple has multiple values:
    types = new_SparseVector(n);
    for (size_t i = 0; i < n; i++) {
        AST *expr = NULL;
        Type *type, *type_copy;

        Vector_get(this->exprs, i, &expr);
        if (getType_AST(expr, state, &type)) {
            status = 1;
        } else {
            type_copy = copy_type(type);
            SparseVector_append(types, type_copy, 1);
        }
    }
    if (0 == status) {
        *typeptr = this->type = TupleType(types);
    }
    return status;
}

static void
delete(ASTTuple *this) {
    delete_Vector(this->exprs, (VEC_DELETE_FUNC)delete_AST);
    if (NULL != this->type) {
        delete_type(this->type);
    }
    free(this);
}

AST *
new_ASTTuple(struct YYLTYPE *loc, Vector *exprs) {
    ASTTuple *tuple = NULL;

    tuple = safe_malloc(sizeof(*tuple));
    *tuple = (ASTTuple){
        json, getType, delete, *loc, exprs, NULL
    };
    return (AST *)tuple;
}
