#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "vector.h"
#include "json.h"
#include "parser.h"

typedef struct ASTProgram ASTProgram;

struct ASTProgram {
    void (*json)(const ASTProgram *this, FILE *out, int indent);
    int (*getType)(const ASTProgram *this, Type **typeptr);
    void (*delete)(ASTProgram *this);
    struct YYLTYPE loc;
    Vector *stmts; // Vector<AST*>
};

static void
json(const ASTProgram *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("program", out, indent);
    json_comma(out, indent);
    json_label("statements", out);
    json_vector(this->stmts, (JSON_MAP_TYPE)json_AST, out, indent);
    json_end(out, &indent);
}

static int
getType(const ASTProgram *this, UNUSED Type **typeptr) {
    print_code_error(&this->loc,
        "program type checker not implemented",
        stderr);
    return 1;
}

static void
delete(ASTProgram *this) {
    delete_Vector(this->stmts, (VEC_DELETE_TYPE)delete_AST);
    free(this);
}

AST *
new_ASTProgram(struct YYLTYPE *loc, Vector *stmts) {
    ASTProgram *program = NULL;

    program = safe_malloc(sizeof(*program));
    *program = (ASTProgram){
        json, getType, delete, *loc, stmts
    };
    return (AST *)program;
}
