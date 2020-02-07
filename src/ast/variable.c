#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"
#include "map.h"

typedef struct ASTVariable ASTVariable;

struct ASTVariable {
    void (*json)(const ASTVariable *this, FILE *out, int indent);
    int (*getType)(ASTVariable *this,
        UNUSED TypeCheckState *state,
        Type **typeptr);
    void (*delete)(ASTVariable *this);
    struct YYLTYPE loc;
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

static int
getType(ASTVariable *this, TypeCheckState *state, Type **typeptr) {
    Map *symbols = state->symbols;
    return 0;
}

static void
delete(ASTVariable *this) {
    free(this->name);
    free(this);
}

AST *
new_ASTVariable(struct YYLTYPE *loc, char *name) {
    ASTVariable *variable = NULL;

    variable = safe_malloc(sizeof(*variable));
    *variable = (ASTVariable){
        json, getType, delete, *loc, name
    };
    return (AST *)variable;
}
