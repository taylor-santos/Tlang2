#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTDefinition ASTDefinition;

struct ASTDefinition {
    void (*json)(const ASTDefinition *this, FILE *out, int indent);
    int (*getType)(ASTDefinition *this,
        UNUSED TypeCheckState *state,
        Type **typeptr);
    void (*delete)(ASTDefinition *this);
    struct YYLTYPE loc;
    Vector *vars;  // Vector<char*>
    AST *expr;
};

static void
json(const ASTDefinition *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("definition", out, indent);
    json_comma(out, indent);
    json_label("variables", out);
    json_vector(this->vars, (JSON_MAP_TYPE)json_string, out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(this->expr, out, indent);
    json_end(out, &indent);
}

static int
getType(ASTDefinition *this,
    UNUSED TypeCheckState *state,
    UNUSED Type **typeptr) {
    Type *expr_type = NULL;
    int status;

    status = getType_AST(this->expr, state, &expr_type);
    //TODO: compare expr to vars and fill symbol table
    return status;
}

static void
delete(ASTDefinition *this) {
    delete_Vector(this->vars, free);
    delete_AST(this->expr);
    free(this);
}

AST *
new_ASTDefinition(struct YYLTYPE *loc, Vector *vars, AST *expr) {
    ASTDefinition *definition = NULL;

    definition = safe_malloc(sizeof(*definition));
    *definition = (ASTDefinition){
        json, getType, delete, *loc, vars, expr
    };
    return (AST *)definition;
}
