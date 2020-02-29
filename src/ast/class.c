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
    Vector *ctors;     // Vector<Vector<Type*>>
    Vector *fields;   // Vector<Field*>
    Type *type;       // NULL until type checker is executed.
};

static void
json_ctor(Vector *ctor, FILE *out, int indent) {
    json_vector(ctor, (JSON_VALUE_FUNC)json_type, out, indent);
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
    json_vector(ast->ctors, (JSON_VALUE_FUNC)json_ctor, out, indent);
    json_comma(out, indent);
    json_label("fields", out);
    json_vector(ast->fields, (JSON_VALUE_FUNC)json_field, out, indent);
    json_end(out, &indent);
}

static void *
copy_ctor(const Vector *ctor) {
    return copy_Vector(ctor, (VEC_COPY_FUNC)copy_type);
}

void
delete_ctor(Vector *ctor) {
    delete_Vector(ctor, (VEC_DELETE_FUNC)delete_type);
}

static int
ctors_sort(const Vector **ctor1, const Vector **ctor2) {
    return (int)Vector_size(*ctor1) - (int)Vector_size(*ctor2);
}

static int
getType(void *this, TypeCheckState *state, Type **typeptr) {
    ASTClass *ast = this;
    size_t ngen, nsupers, nctors, nfields;
    Vector *ctors;
    Map *fields;
    int status = 0;

    ngen = Vector_size(ast->generics);
    nsupers = Vector_size(ast->supers);
    nctors = Vector_size(ast->ctors);
    nfields = Vector_size(ast->fields);
    if (ngen > 0) {
        // TODO: generic classes
        print_warning("class generics not implemented\n");
    }
    if (nsupers > 0) {
        // TODO: class inheritance
        print_warning("class inheritance not yet implemented\n");
    }
    for (size_t i = 0; i < nctors; i++) {
        Vector *ctor = Vector_get(ast->ctors, i);
        size_t nargs = Vector_size(ctor);
        for (size_t j = 0; j < nargs; j++) {
            Type *argType = Vector_get(ctor, j);
            char *msg;
            if (argType->verify(argType, state, &msg)) {
                print_code_error(stderr, argType->loc, msg);
                free(msg);
                status = 1;
            }
        }
    }
    ctors = copy_Vector(ast->ctors, (VEC_COPY_FUNC)copy_ctor);
    sort_Vector(ctors, (VEC_COMPARATOR)ctors_sort);
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
        delete_Vector(ctors, (VEC_DELETE_FUNC)delete_ctor);
        delete_Map(fields, (MAP_DELETE_FUNC)delete_type);
        return 1;
    }
    *typeptr = ast->type =
        ClassType(ast->super.loc, Vector(), Vector(), ctors, fields);
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
    delete_Vector(ast->ctors, (VEC_DELETE_FUNC)delete_ctor);
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
        }, generics, inherits, body->ctors, body->fields, NULL
    };
    free(body);
    return (AST *)class;
}
