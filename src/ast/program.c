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
    Map *compare;    // Map<Type**, Map<Type**, int>>
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTProgram *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("program", out, indent);
    json_comma(out, indent);
    json_label("statements", out);
    json_vector(ast->stmts, (JSON_VALUE_FUNC)json_AST, out, indent);
    json_end(out, &indent);
}

struct Builtin {
    char *name;
    unsigned int toBool: 1;
} builtins[] = {
    { "int",    1 },
    { "bool",   1 },
    { "double", 1 },
    { "string", 0 },
};

static TypeCheckState
addBuiltins(Map *symbols, Vector *classes, Map *compare) {
    TypeCheckState state = {
        symbols, NULL, classes, { NULL }, compare, NULL, NULL
    };
    YYLTYPE loc = {
        0
    };
    for (size_t i = 0; i < sizeof(builtins) / sizeof(*builtins); i++) {
        struct Builtin builtin = builtins[i];
        Type *type = ClassType(loc, Vector(), Vector(), Vector(), Map());
        Map_put(state.symbols, builtin.name, strlen(builtin.name), type, NULL);
        struct ClassType *class = (struct ClassType *)type;
        state.builtins[i] = class;
        Vector_append(state.classes, class);
        Map_put(compare, &class, sizeof(class), Map(), NULL);
    }
    // Iterate through builtins again to add their fields. Some fields rely
    // on pre-existing builtins for argument and return types, so the
    // original types need to be fully initialized first.
    for (size_t i = 0; i < sizeof(builtins) / sizeof(*builtins); i++) {
        struct Builtin builtin = builtins[i];
        const struct ClassType *class = state.builtins[i];
        if (builtin.toBool) {
            Type *retType = ObjectType(loc, safe_strdup("bool"), Vector());
            retType->verify(retType, &state, NULL);
            Type *fieldType = FuncType(loc, Vector(), Vector(), retType);
            char *fieldName = "=>";
            Map_put(class->fields,
                fieldName,
                strlen(fieldName),
                fieldType,
                NULL);
        }
        {
            Type *retType =
                ObjectType(loc, safe_strdup(builtin.name), Vector());
            Type *argType =
                ObjectType(loc, safe_strdup(builtin.name), Vector());
            retType->verify(retType, &state, NULL);
            argType->verify(argType, &state, NULL);
            Vector *args = init_Vector(argType);
            Type *fieldType = FuncType(loc, Vector(), args, retType);
            char *fieldName = "==";
            Map_put(class->fields,
                fieldName,
                strlen(fieldName),
                fieldType,
                NULL);
        }
    }
    return state;
}

static void
json_empty(UNUSED const void *value, FILE *out, UNUSED int indent) {
    json_string("", out, indent);
}

static void
json_typePtr(const Type **typePtr, UNUSED size_t size, FILE *out) {
    char *str = (*typePtr)->toString(*typePtr);
    json_label(str, out);
    free(str);
}

static void
json_compare(const Map *compare, FILE *out, int indent) {
    json_Map(compare,
        (JSON_KEY_FUNC)json_typePtr,
        (JSON_VALUE_FUNC)json_empty,
        out,
        indent);
}

static int
getType(void *this, UNUSED TypeCheckState *state, UNUSED Type **typeptr) {
    ASTProgram *ast = this;
    size_t n;
    int status = 0;

    TypeCheckState
        new_state = addBuiltins(ast->symbols, ast->classes, ast->compare);
    n = Vector_size(ast->stmts);
    for (size_t i = 0; i < n; i++) {
        AST *stmt = Vector_get(ast->stmts, i);
        Type *type;
        status = stmt->getType(stmt, &new_state, &type) || status;
    }
    if (!status) {
        fprintf(stdout, "Symbol Table:\n");
        json_Map(ast->symbols,
            (JSON_KEY_FUNC)json_nlabel,
            (JSON_VALUE_FUNC)json_type,
            stdout,
            0);
        fprintf(stdout, "\n");
        fprintf(stdout, "Class Hierarchy:\n");
        json_Map(ast->compare,
            (JSON_KEY_FUNC)json_typePtr,
            (JSON_VALUE_FUNC)json_compare,
            stdout,
            0);
    }
    return status;
}

static void
delete_compare(Map *compare) {
    delete_Map(compare, NULL);
}

static void
delete(void *this) {
    ASTProgram *ast = this;
    delete_Vector(ast->stmts, (VEC_DELETE_FUNC)delete_AST);
    delete_Map(ast->symbols, (MAP_DELETE_FUNC)delete_type);
    delete_Map(ast->compare, (MAP_DELETE_FUNC)delete_compare);
    delete_Vector(ast->classes, (VEC_DELETE_FUNC)NULL);
    free(this);
}

AST *
new_ASTProgram(YYLTYPE loc, Vector *stmts) {
    ASTProgram *program = NULL;
    Map *symbols, *compare;
    Vector *classes;

    program = safe_malloc(sizeof(*program));
    symbols = Map();
    classes = Vector();
    compare = Map();
    *program = (ASTProgram){
        { json, getType, delete, loc }, stmts, symbols, classes, compare
    };
    return (AST *)program;
}
