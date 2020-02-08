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
    Type *type = NULL;
    if (Map_get(state->symbols, this->name, strlen(this->name), &type)) {
        print_code_error(stderr,
            this->loc,
            "unknown variable \"%s\"",
            this->name);
        return 1;
    } else if (!isInit(type)) {
        print_code_error(stderr,
            this->loc,
            "variable \"%s\" used before initialization",
            this->name);
        return 1;
    }
    *typeptr = type;
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
