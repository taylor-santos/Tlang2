#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "types.h"
#include "parser.h"

typedef struct ASTNamedType ASTNamedType;

struct ASTNamedType {
    AST super;
    Vector *names; // Vector<char*> (can be empty for unnamed arguments)
    Type *type;
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTNamedType *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("named type", out, indent);
    json_comma(out, indent);
    json_label("names", out);
    json_vector(ast->names, (JSON_VALUE_FUNC)json_string, out, indent);
    json_comma(out, indent);
    json_label("type", out);
    json_type(ast->type, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, UNUSED TypeCheckState *state, UNUSED Type **typeptr) {
    ASTNamedType *ast = this;
    print_code_error(stderr,
        ast->super.loc,
        "named_type type checker not implemented");
    return 1;
}

static void
delete(void *this) {
    ASTNamedType *ast = this;
    delete_Vector(ast->names, (VEC_DELETE_FUNC)free);
    delete_type(ast->type);
    free(this);
}

AST *
new_ASTNamedType(YYLTYPE loc, Vector *names, Type *type) {
    ASTNamedType *named_type = NULL;

    named_type = safe_malloc(sizeof(*named_type));
    *named_type = (ASTNamedType){
        { json, getType, delete, loc }, names, type
    };
    return (AST *)named_type;
}
