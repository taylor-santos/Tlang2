#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"

typedef struct ASTVariable ASTVariable;

struct ASTVariable {
    void (*json)(const ASTVariable *this, FILE *out, int indent);
    void (*delete)(ASTVariable *this);
    char *name;
};

static void
json(const ASTVariable *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("variable", out, indent);
    json_comma(out, indent);
    json_label("name", out);
    json_string(this->name, out, indent);
    json_end(out, &indent);
}

static void
delete(ASTVariable *this) {
    free(this->name);
    free(this);
}

AST *
new_ASTVariable(char *name) {
    ASTVariable *variable = NULL;

    variable = safe_malloc(sizeof(*variable));
    *variable = (ASTVariable){
        json, delete, name
    };
    return (AST *)variable;
}
