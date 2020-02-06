#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "types.h"
#include "parser.h"

typedef struct ASTFunc ASTFunc;

struct ASTFunc {
    void (*json)(const ASTFunc *this, FILE *out, int indent);
    void (*delete)(ASTFunc *this);
    struct YYLTYPE loc;
    Vector *generics; // Vector<char*>
    Vector *args;     // Vector<AST*> TODO TypeDef
    Type *ret_type;
    Vector *stmts;    // Vector<AST*>
};

static void
json(const ASTFunc *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("func", out, indent);
    json_comma(out, indent);
    json_label("generics", out);
    json_vector(this->generics, (JSON_MAP_TYPE)json_string, out, indent);
    json_comma(out, indent);
    json_label("args", out);
    json_vector(this->args, (JSON_MAP_TYPE)json_AST, out, indent);
    json_comma(out, indent);
    json_label("ret_type", out);
    json_type(this->ret_type, out, indent);
    json_comma(out, indent);
    json_label("stmts", out);
    json_vector(this->stmts, (JSON_MAP_TYPE)json_AST, out, indent);
    json_end(out, &indent);
}

static void
delete(ASTFunc *this) {
    delete_Vector(this->generics, free);
    delete_Vector(this->args, (VEC_DELETE_TYPE)delete_AST);
    delete_type(this->ret_type);
    delete_Vector(this->stmts, (VEC_DELETE_TYPE)delete_AST);
    free(this);
}

AST *
new_ASTFunc(struct YYLTYPE *loc,
    Vector *generics,
    Vector *args,
    Type *ret_type,
    Vector *stmts) {
    ASTFunc *func = NULL;

    func = safe_malloc(sizeof(*func));
    *func = (ASTFunc){
        json, delete, *loc, generics, args, ret_type, stmts
    };
    return (AST *)func;
}
