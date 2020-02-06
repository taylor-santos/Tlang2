#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "parser.h"

typedef struct ASTTuple ASTTuple;

struct ASTTuple {
    void (*json)(const ASTTuple *this, FILE *out, int indent);
    int (*getType)(const ASTTuple *this, Type **typeptr);
    void (*delete)(ASTTuple *this);
    struct YYLTYPE loc;
    Vector *exprs; // Vector<AST*>
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
getType(const ASTTuple *this, UNUSED Type **typeptr) {
    print_code_error(&this->loc, "tuple type checker not implemented", stderr);
    return 1;
}

static void
delete(ASTTuple *this) {
    delete_Vector(this->exprs, (VEC_DELETE_TYPE)delete_AST);
    free(this);
}

AST *
new_ASTTuple(struct YYLTYPE *loc, Vector *exprs) {
    ASTTuple *tuple = NULL;

    tuple = safe_malloc(sizeof(*tuple));
    *tuple = (ASTTuple){
        json, getType, delete, *loc, exprs
    };
    return (AST *)tuple;
}
