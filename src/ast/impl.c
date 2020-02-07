#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTImpl ASTImpl;

struct ASTImpl {
    void (*json)(const ASTImpl *this, FILE *out, int indent);
    int (*getType)(ASTImpl *this,
        UNUSED TypeCheckState *state,
        Type **typeptr);
    void (*delete)(ASTImpl *this);
    struct YYLTYPE loc;
    char *name;
    Vector *generics; // Vector<char*>
    Vector *stmts;    // Vector<AST*>
};

static void
json(const ASTImpl *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("impl", out, indent);
    json_comma(out, indent);
    json_label("name", out);
    json_string(this->name, out, indent);
    json_comma(out, indent);
    json_label("generics", out);
    json_vector(this->generics, (JSON_MAP_TYPE)json_string, out, indent);
    json_comma(out, indent);
    json_label("statements", out);
    json_vector(this->stmts, (JSON_MAP_TYPE)json_AST, out, indent);
    json_end(out, &indent);
}

static int
getType(ASTImpl *this, UNUSED TypeCheckState *state, UNUSED Type **typeptr) {
    print_code_error(stderr, this->loc, "impl type checker not implemented");
    return 1;
}

static void
delete(ASTImpl *this) {
    free(this->name);
    delete_Vector(this->generics, free);
    delete_Vector(this->stmts, (VEC_DELETE_FUNC)delete_AST);
    free(this);
}

AST *
new_ASTImpl(struct YYLTYPE *loc, char *name, Vector *generics, Vector *stmts) {
    ASTImpl *impl = NULL;

    impl = safe_malloc(sizeof(*impl));
    *impl = (ASTImpl){
        json, getType, delete, *loc, name, generics, stmts
    };
    return (AST *)impl;
}
