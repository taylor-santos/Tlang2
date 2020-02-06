#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "types.h"
#include "parser.h"

typedef struct ASTClass ASTClass;

struct ASTClass {
    void (*json)(const ASTClass *this, FILE *out, int indent);
    int (*getType)(const ASTClass *this, Type **typeptr);
    void (*delete)(ASTClass *this);
    struct YYLTYPE loc;
    Vector *generics; // Vector<char*>
    Vector *inherits; // Vector<Type*>
    Vector *members;  // Vector<AST*> TODO TypeDef
};

static void
json(const ASTClass *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("class", out, indent);
    json_comma(out, indent);
    json_label("generics", out);
    json_vector(this->generics, (JSON_MAP_TYPE)json_string, out, indent);
    json_comma(out, indent);
    json_label("inherits", out);
    json_vector(this->inherits, (JSON_MAP_TYPE)json_type, out, indent);
    json_comma(out, indent);
    json_label("members", out);
    json_vector(this->members, (JSON_MAP_TYPE)json_AST, out, indent);
    json_end(out, &indent);
}

static int
getType(const ASTClass *this, UNUSED Type **typeptr) {
    print_code_error(&this->loc, "class type checker not implemented", stderr);
    return 1;
}

static void
delete(ASTClass *this) {
    delete_Vector(this->generics, free);
    delete_Vector(this->inherits, (VEC_DELETE_TYPE)delete_type);
    delete_Vector(this->members, (VEC_DELETE_TYPE)delete_AST);
    free(this);
}

AST *
new_ASTClass(struct YYLTYPE *loc,
    Vector *generics,
    Vector *inherits,
    Vector *members) {
    ASTClass *class = NULL;

    class = safe_malloc(sizeof(*class));
    *class = (ASTClass){
        json, getType, delete, *loc, generics, inherits, members
    };
    return (AST *)class;
}
