#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"

typedef struct ASTInit ASTInit;

struct Vector;

struct ASTInit {
    void (*json)(const ASTInit *this, FILE *out, int indent);
    void (*delete)(ASTInit *this);
    char *name;
    Vector *generics; // Vector<char*>
    Vector *args;     // Vector<AST*>
};

static void
json(const ASTInit *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("init", out, indent);
    json_comma(out, indent);
    json_label("name", out);
    json_string(this->name, out, indent);
    json_comma(out, indent);
    json_label("generics", out);
    json_list(this->generics, (JSON_MAP_TYPE)json_string, out, indent);
    json_comma(out, indent);
    json_label("args", out);
    json_list(this->args, (JSON_MAP_TYPE)json_AST, out, indent);
    json_end(out, &indent);
}

static void
delete(ASTInit *this) {
    free(this->name);
    delete_Vector(this->generics, free);
    delete_Vector(this->args, (VEC_DELETE_TYPE)delete_AST);
    free(this);
}

AST *
new_ASTInit(char *name, Vector *generics, Vector *args) {
    ASTInit *init = NULL;

    init = safe_malloc(sizeof(*init));
    *init = (ASTInit){
        json, delete, name, generics, args
    };
    return (AST *)init;
}
