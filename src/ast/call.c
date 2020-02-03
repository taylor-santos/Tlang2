#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"

typedef struct ASTCall ASTCall;

struct ASTCall {
    void (*json)(const ASTCall *this, FILE *out, int indent);
    void (*delete)(ASTCall *this);
    AST *expr;
    Vector *args; // Vector<AST*>
};

static void
json(const ASTCall *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("call", out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(this->expr, out, indent);
    json_comma(out, indent);
    json_label("args", out);
    json_list(this->args, (JSON_MAP_TYPE)json_AST, out, indent);
    json_end(out, &indent);
}

static void
delete(ASTCall *this) {
    delete_AST(this->expr);
    delete_Vector(this->args, (VEC_DELETE_TYPE)delete_AST);
    free(this);
}

AST *
new_ASTCall(AST *expr, Vector *args) {
    ASTCall *call = NULL;

    call = safe_malloc(sizeof(*call));
    *call = (ASTCall){
        json, delete, expr, args
    };
    return (AST *)call;
}
