#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "types.h"
#include "vector.h"
#include "parser.h"
#include "map.h"

typedef struct ASTTypeStmt ASTTypeStmt;

struct ASTTypeStmt {
    void (*json)(const ASTTypeStmt *this, FILE *out, int indent);
    int (*getType)(ASTTypeStmt *this,
        UNUSED TypeCheckState *state,
        Type **typeptr);
    void (*delete)(ASTTypeStmt *this);
    struct YYLTYPE loc;
    Vector *vars; // Vector<char*>
    Type *type;
};

static void
json(const ASTTypeStmt *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("type declaration", out, indent);
    json_comma(out, indent);
    json_label("vars", out);
    json_vector(this->vars, (JSON_MAP_TYPE)json_string, out, indent);
    json_comma(out, indent);
    json_label("type", out);
    json_type(this->type, out, indent);
    json_end(out, &indent);
}

static int
getType(ASTTypeStmt *this, TypeCheckState *state, UNUSED Type **typeptr) {
    int status = 0;
    size_t nvars;

    nvars = Vector_size(this->vars);
    for (size_t i = 0; i < nvars; i++) {
        char *var = NULL;
        Vector_get(this->vars, i, &var);
        size_t len = strlen(var);
        Type *prev_type = NULL;
        if (!Map_get(state->symbols, var, len, &prev_type)) {
            print_code_error(stderr,
                this->loc,
                "redefinition of variable \"%s\"",
                var);
            status = 1;
        } else {
            Type *type_copy = copy_type(this->type);
            Map_put(state->symbols, var, len, type_copy, NULL);
        }
    }
    return status;
}

static void
delete(ASTTypeStmt *this) {
    delete_Vector(this->vars, free);
    delete_type(this->type);
    free(this);
}

AST *
new_ASTTypeStmt(struct YYLTYPE *loc, Vector *vars, Type *type) {
    ASTTypeStmt *named_type = NULL;

    named_type = safe_malloc(sizeof(*named_type));
    *named_type = (ASTTypeStmt){
        json, getType, delete, *loc, vars, type
    };
    return (AST *)named_type;
}
