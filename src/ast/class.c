#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "types.h"
#include "parser.h"
#include "map.h"

typedef struct ASTClass ASTClass;

struct ASTClass {
    AST super;
    Vector *generics; // Vector<char*>
    Vector *supers;   // Vector<Type*>
    Vector *cons;     // Vector<Vector<Type*>>
    Vector *fields;   // Vector<Field*>
    Type *type;       // NULL until type checker is executed.
};

static void
json_constructor(Vector *cons, FILE *out, int indent) {
    json_vector(cons, (JSON_VALUE_FUNC)json_type, out, indent);
}

static void
json(const void *this, FILE *out, int indent) {
    const ASTClass *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("class", out, indent);
    json_comma(out, indent);
    json_label("generics", out);
    json_vector(ast->generics, (JSON_VALUE_FUNC)json_string, out, indent);
    json_comma(out, indent);
    json_label("supers", out);
    json_vector(ast->supers, (JSON_VALUE_FUNC)json_type, out, indent);
    json_comma(out, indent);
    json_label("constructors", out);
    json_vector(ast->cons, (JSON_VALUE_FUNC)json_constructor, out, indent);
    json_comma(out, indent);
    json_label("fields", out);
    json_vector(ast->fields, (JSON_VALUE_FUNC)json_field, out, indent);
    json_end(out, &indent);
}

static void *
copy_cons(const Vector *cons) {
    return copy_Vector(cons, (VEC_COPY_FUNC)copy_type);
}

static void
delete_cons(Vector *cons) {
    delete_Vector(cons, (VEC_DELETE_FUNC)delete_type);
}

static int
cons_compare(const Vector **cons1, const Vector **cons2) {
    return (int)Vector_size(*cons1) - (int)Vector_size(*cons2);
}

static int
getType(void *this, TypeCheckState *state, Type **typeptr) {
    ASTClass *ast = this;
    size_t ngen, nsupers, ncons, nfields;
    Vector *constructors;
    Map *fields;
    int status = 0;

    ngen = Vector_size(ast->generics);
    nsupers = Vector_size(ast->supers);
    ncons = Vector_size(ast->cons);
    nfields = Vector_size(ast->fields);
    if (ngen > 0) {
        print_warning("class generics not implemented\n");
    }
    if (nsupers > 0) {
        print_warning("class inheritance not yet implemented\n");
    }
    for (size_t i = 0; i < ncons; i++) {
        Vector *con = Vector_get(ast->cons, i);
        size_t nargs = Vector_size(con);
        for (size_t j = 0; j < nargs; j++) {
            Type *argType = Vector_get(con, j);
            char *msg;
            if (argType->verify(argType, state, &msg)) {
                print_code_error(stderr, argType->loc, msg);
                free(msg);
                status = 1;
            }
        }
    }
    constructors = copy_Vector(ast->cons, (VEC_COPY_FUNC)copy_cons);
    sort_Vector(constructors, (VEC_COMPARATOR)cons_compare);
    fields = Map();
    for (size_t i = 0; i < nfields; i++) {
        struct Field *field = Vector_get(ast->fields, i);
        char *msg;
        if (field->type->verify(field->type, state, &msg)) {
            print_code_error(stderr, field->type->loc, msg);
            free(msg);
            status = 1;
            continue;
        }
        size_t nnames = Vector_size(field->names);
        for (size_t j = 0; j < nnames; j++) {
            char *name = Vector_get(field->names, j);
            size_t len = strlen(name);
            if (!Map_get(fields, name, len, NULL)) {
                print_code_error(stderr,
                    ast->super.loc,
                    "duplicate field \"%s\"",
                    name);
                status = 1;
            } else {
                Type *type_copy = copy_type(field->type);
                Map_put(fields, name, len, type_copy, NULL);
            }
        }
    }
    if (status) {
        delete_Vector(constructors, (VEC_DELETE_FUNC)delete_cons);
        delete_Map(fields, (MAP_DELETE_FUNC)delete_type);
        return 1;
    }
    *typeptr = ast->type =
        ClassType(ast->super.loc, Vector(), Vector(), constructors, fields);
    Vector_append(state->classes, ast->type);
    AddComparison(ast->type, state);
    return 0;
}

static void
delete(void *this) {
    ASTClass *ast = this;
    delete_Vector(ast->generics, free);
    delete_Vector(ast->supers, (VEC_DELETE_FUNC)delete_type);
    delete_Vector(ast->fields, (VEC_DELETE_FUNC)delete_field);
    delete_Vector(ast->cons, (VEC_DELETE_FUNC)delete_cons);
    if (NULL != ast->type) {
        delete_type(ast->type);
    }
    free(this);
}

AST *
new_ASTClass(YYLTYPE loc,
    Vector *generics,
    Vector *inherits,
    struct ClassBody *body) {
    ASTClass *class = NULL;

    class = safe_malloc(sizeof(*class));
    *class = (ASTClass){
        {
            json, getType, delete, loc
        }, generics, inherits, body->constructors, body->fields, NULL
    };
    free(body);
    return (AST *)class;
}
