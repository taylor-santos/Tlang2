#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "vector.h"
#include "json.h"

typedef struct ASTProgram ASTProgram;

struct ASTProgram {
    void (*json)(const ASTProgram *this, FILE *out, int indent);
    void (*delete)(ASTProgram *this);
    Vector *stmts; // Vector<AST*>
};

static void
json(const ASTProgram *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("program", out, indent);
    json_comma(out, indent);
    json_label("statements", out);
    json_list(this->stmts, (JSON_MAP_TYPE)json_AST, out, indent);
    json_end(out, &indent);
}

static void
delete(ASTProgram *this) {
    delete_Vector(this->stmts, (VEC_DELETE_TYPE)delete_AST);
    free(this);
}

AST *
new_ASTProgram(Vector *stmts) {
    ASTProgram *program = NULL;

    program = safe_malloc(sizeof(*program));
    *program = (ASTProgram){
        json, delete, stmts
    };
    return (AST *)program;
}
