#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTDefinition ASTDefinition;

struct ASTDefinition {
    void (*json)(const ASTDefinition *this, FILE *out, int indent);
    void (*delete)(ASTDefinition *this);
    struct YYLTYPE loc;
    AST *vars;
    AST *expr;
};

static void
json(const ASTDefinition *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("definition", out, indent);
    json_comma(out, indent);
    json_label("variables", out);
    json_AST(this->vars, out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(this->expr, out, indent);
    json_end(out, &indent);
}

static void
delete(ASTDefinition *this) {
    delete_AST(this->vars);
    delete_AST(this->expr);
    free(this);
}

AST *
new_ASTDefinition(struct YYLTYPE *loc, AST *vars, AST *expr) {
    ASTDefinition *definition = NULL;

    definition = safe_malloc(sizeof(*definition));
    *definition = (ASTDefinition){
        json, delete, *loc, vars, expr
    };
    return (AST *)definition;
}