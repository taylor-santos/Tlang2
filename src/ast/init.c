#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "parser.h"

typedef struct ASTInit ASTInit;

struct Vector;

struct ASTInit {
    void (*json)(const ASTInit *this, FILE *out, int indent);
    void (*delete)(ASTInit *this);
    struct YYLTYPE loc;
    char *name;
    Vector *generics; // Vector<char*>
    AST *args;        // NULLable
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
    if (NULL != this->args) {
        json_comma(out, indent);
        json_label("args", out);
        json_AST(this->args, out, indent);
    }
    json_end(out, &indent);
}

static void
delete(ASTInit *this) {
    free(this->name);
    delete_Vector(this->generics, free);
    if (NULL != this->args) {
        delete_AST(this->args);
    }
    free(this);
}

AST *
new_ASTInit(struct YYLTYPE *loc, char *name, Vector *generics, AST *args) {
    ASTInit *init = NULL;

    init = safe_malloc(sizeof(*init));
    *init = (ASTInit){
        json, delete, *loc, name, generics, args
    };
    return (AST *)init;
}
