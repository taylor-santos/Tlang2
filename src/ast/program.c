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
    Map *symbols;  // Map<char*, Type*>
    Map *classes;  // Map<char*, AST*> (see struct TypeCheckState)
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

static const char *num_builtins[] = {
    "int", "double"
};

static const char *num_operators[] = {
    "+", "-", "*", "/", "%"
};

static void
addBuiltins(ASTProgram *this) {
    for (unsigned long i = 0;
        i < sizeof(num_builtins) / sizeof(*num_builtins);
        i++) {
        YYLTYPE loc = {
            0
        };
        Map *fields = Map();
        for (unsigned long j = 0;
            j < sizeof(num_operators) / sizeof(*num_operators);
            j++) {
            Vector *args = init_Vector(ObjectType(&loc,
                safe_strdup(num_builtins[i]),
                Vector()));
            Type *type = FuncType(&loc,
                Vector(),
                args,
                ObjectType(&loc, safe_strdup(num_builtins[i]), Vector()));
            Map_put(fields,
                num_operators[j],
                strlen(num_operators[j]),
                type,
                NULL);
        }
        Type *type =
            ClassType(&loc, Vector(), Vector(), init_Vector(Vector()), fields);
        Map_put(this->symbols,
            num_builtins[i],
            strlen(num_builtins[i]),
            type,
            NULL);
    }
}

static int
getType(ASTProgram *this,
    UNUSED TypeCheckState *state,
    UNUSED Type **typeptr) {
    size_t n;
    int status = 0;
    TypeCheckState new_state = {
        this->symbols, this->classes
    };

    addBuiltins(this);
    n = Vector_size(this->stmts);
    for (size_t i = 0; i < n; i++) {
        AST *stmt = Vector_get(this->stmts, i);
        Type *type;
        status = getType_AST(stmt, &new_state, &type) || status;
    }
    if (!status) {
        fprintf(stdout, "Symbol Table:\n");
        json_Map(this->symbols, (JSON_MAP_TYPE)json_type, stdout, 0);
        fprintf(stdout, "\n");
    }
    return status;
}

static void
delete(ASTProgram *this) {
    delete_Vector(this->stmts, (VEC_DELETE_FUNC)delete_AST);
    delete_Map(this->symbols, (MAP_DELETE_FUNC)delete_type);
    delete_Map(this->classes, NULL);
    free(this);
}

AST *
new_ASTProgram(struct YYLTYPE *loc, Vector *stmts) {
    ASTProgram *program = NULL;
    Map *symbols, *classes;

    program = safe_malloc(sizeof(*program));
    symbols = Map();
    classes = Map();
    *program = (ASTProgram){
        json, getType, delete, *loc, stmts, symbols, classes
    };
    return (AST *)program;
}
