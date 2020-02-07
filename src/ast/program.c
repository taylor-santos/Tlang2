#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "vector.h"
#include "json.h"
#include "parser.h"
#include "map.h"

typedef struct ASTProgram ASTProgram;

struct ASTProgram {
    void (*json)(const ASTProgram *this, FILE *out, int indent);
    int (*getType)(ASTProgram *this,
        UNUSED TypeCheckState *state,
        Type **typeptr);
    void (*delete)(ASTProgram *this);
    struct YYLTYPE loc;
    Vector *stmts; // Vector<AST*>
    Map *symbols; // Map<char*, Type*>
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
getType(ASTProgram *this,
    UNUSED TypeCheckState *state,
    UNUSED Type **typeptr) {
    size_t n;
    int status = 0;
    TypeCheckState new_state = {
        this->symbols
    };

    n = Vector_size(this->stmts);
    for (size_t i = 0; i < n; i++) {
        AST *stmt = NULL;
        Vector_get(this->stmts, i, &stmt);
        Type *type;
        status = getType_AST(stmt, &new_state, &type) || status;
    }
    fprintf(stdout, "Symbol Table:\n");
    json_Map(this->symbols, (JSON_MAP_TYPE)json_type, stdout, 0);
    fprintf(stdout, "\n");
    return status;
}

static void
delete(ASTProgram *this) {
    delete_Vector(this->stmts, (VEC_DELETE_FUNC)delete_AST);
    delete_Map(this->symbols, (MAP_DELETE_FUNC)delete_type);
    free(this);
}

AST *
new_ASTProgram(struct YYLTYPE *loc, Vector *stmts) {
    ASTProgram *program = NULL;
    Map *symbols;

    program = safe_malloc(sizeof(*program));
    symbols = Map();
    *program = (ASTProgram){
        json, getType, delete, *loc, stmts, symbols
    };
    return (AST *)program;
}
