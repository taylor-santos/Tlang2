#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "vector.h"
#include "json.h"
#include "parser.h"
#include "map.h"

typedef struct ASTProgram ASTProgram;

struct ASTProgram {
    AST super;
    Vector *stmts;   // Vector<AST*>
    Map *symbols;    // Map<char*, Type*>
    Vector *classes; // Vector<const struct ClassType*>
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTProgram *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("program", out, indent);
    json_comma(out, indent);
    json_label("statements", out);
    json_vector(ast->stmts, (JSON_MAP_TYPE)json_AST, out, indent);
    json_end(out, &indent);
}

struct Builtin {
    char *name;
    unsigned int toBool: 1;
    unsigned int toString: 1;
} builtins[] = {
    { "int",    1, 1 },
    { "bool",   1, 0 },
    { "double", 1, 1 },
    { "string", 0, 1 },
};

static TypeCheckState
addBuiltins(Map *symbols, Vector *classes) {
    TypeCheckState state = {
        symbols, NULL, classes, { NULL }, NULL, NULL
    };
    YYLTYPE loc = {
        0
    };
    for (size_t i = 0; i < sizeof(builtins) / sizeof(*builtins); i++) {
        struct Builtin builtin = builtins[i];
        Type *type = ClassType(loc, Vector(), Vector(), Vector(), Map());
        Map_put(state.symbols, builtin.name, strlen(builtin.name), type, NULL);
        const struct ClassType *class = getTypeData(type);
        state.builtins[i] = class;
        Vector_append(state.classes, (void *)class);
    }
    // Iterate through builtins again to add their fields. Some fields rely
    // on pre-existing builtins for argument and return types, so the
    // original types need to be fully initialized first.
    for (size_t i = 0; i < sizeof(builtins) / sizeof(*builtins); i++) {
        struct Builtin builtin = builtins[i];
        const struct ClassType *class = state.builtins[i];
        if (builtin.toBool) {
            Type *retType = ObjectType(loc, safe_strdup("bool"), Vector());
            TypeVerify(retType, &state, NULL);
            Type *fieldType = FuncType(loc, Vector(), Vector(), retType);
            char *fieldName = "toBool";
            Map_put(class->fields,
                fieldName,
                strlen(fieldName),
                fieldType,
                NULL);
        }
        if (builtin.toString) {
            Type *retType = ObjectType(loc, safe_strdup("string"), Vector());
            TypeVerify(retType, &state, NULL);
            Type *fieldType = FuncType(loc, Vector(), Vector(), retType);
            char *fieldName = "toString";
            Map_put(class->fields,
                fieldName,
                strlen(fieldName),
                fieldType,
                NULL);
        }
    }
    return state;
}

static int
getType(void *this, UNUSED TypeCheckState *state, UNUSED Type **typeptr) {
    ASTProgram *ast = this;
    size_t n;
    int status = 0;

    TypeCheckState new_state = addBuiltins(ast->symbols, ast->classes);
    n = Vector_size(ast->stmts);
    for (size_t i = 0; i < n; i++) {
        AST *stmt = Vector_get(ast->stmts, i);
        Type *type;
        status = stmt->getType(stmt, &new_state, &type) || status;
    }
    if (!status) {
        fprintf(stdout, "Symbol Table:\n");
        json_Map(ast->symbols, (JSON_MAP_TYPE)json_type, stdout, 0);
        fprintf(stdout, "\n");
    }
    return status;
}

static void
delete(void *this) {
    ASTProgram *ast = this;
    // Delete classes before statements and symbols, because of ownership
    delete_Vector(ast->classes, (VEC_DELETE_FUNC)delete_ClassType);
    delete_Vector(ast->stmts, (VEC_DELETE_FUNC)delete_AST);
    delete_Map(ast->symbols, (MAP_DELETE_FUNC)delete_type);
    free(this);
}

AST *
new_ASTProgram(YYLTYPE loc, Vector *stmts) {
    ASTProgram *program = NULL;
    Map *symbols;
    Vector *classes;

    program = safe_malloc(sizeof(*program));
    symbols = Map();
    classes = Vector();
    *program = (ASTProgram){
        { json, getType, delete, loc }, stmts, symbols, classes
    };
    return (AST *)program;
}
