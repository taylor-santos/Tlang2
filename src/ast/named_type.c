#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "types.h"
#include "parser.h"

typedef struct ASTNamedType ASTNamedType;

struct ASTNamedType {
    void (*json)(const ASTNamedType *this, FILE *out, int indent);
    int (*getType)(const ASTNamedType *this, Type **typeptr);
    void (*delete)(ASTNamedType *this);
    struct YYLTYPE loc;
    Vector *names; // Vector<char*> (can be empty for unnamed arguments)
    Type *type;
};

static void
json(const ASTNamedType *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("named type", out, indent);
    json_comma(out, indent);
    json_label("names", out);
    json_vector(this->names, (JSON_MAP_TYPE)json_string, out, indent);
    json_comma(out, indent);
    json_label("type", out);
    json_type(this->type, out, indent);
    json_end(out, &indent);
}

static int
getType(const ASTNamedType *this, UNUSED Type **typeptr) {
    print_code_error(&this->loc,
        "named_type type checker not implemented",
        stderr);
    return 1;
}

static void
delete(ASTNamedType *this) {
    delete_Vector(this->names, (VEC_DELETE_TYPE)free);
    delete_type(this->type);
    free(this);
}

AST *
new_ASTNamedType(struct YYLTYPE *loc, Vector *names, Type *type) {
    ASTNamedType *named_type = NULL;

    named_type = safe_malloc(sizeof(*named_type));
    *named_type = (ASTNamedType){
        json, getType, delete, *loc, names, type
    };
    return (AST *)named_type;
}
