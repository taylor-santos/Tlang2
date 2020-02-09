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
    void (*json)(const ASTClass *this, FILE *out, int indent);
    int (*getType)(ASTClass *this,
        UNUSED TypeCheckState *state,
        Type **typeptr);
    void (*delete)(ASTClass *this);
    struct YYLTYPE loc;
    Vector *generics; // Vector<char*>
    Vector *supers;   // Vector<Type*>
    Vector *cons;     // Vector<Vector<Type*>>
    Vector *fields;   // Vector<Field*>
    Type *type;       // NULL until type checker is executed.
};

static void
json_constructor(Vector *cons, FILE *out, int indent) {
    json_vector(cons, (JSON_MAP_TYPE)json_type, out, indent);
}

static void
json(const ASTClass *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("class", out, indent);
    json_comma(out, indent);
    json_label("generics", out);
    json_vector(this->generics, (JSON_MAP_TYPE)json_string, out, indent);
    json_comma(out, indent);
    json_label("supers", out);
    json_vector(this->supers, (JSON_MAP_TYPE)json_type, out, indent);
    json_comma(out, indent);
    json_label("constructors", out);
    json_vector(this->cons, (JSON_MAP_TYPE)json_constructor, out, indent);
    json_comma(out, indent);
    json_label("fields", out);
    json_vector(this->fields, (JSON_MAP_TYPE)json_field, out, indent);
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
getType(ASTClass *this, UNUSED TypeCheckState *state, UNUSED Type **typeptr) {
    size_t ngen, nsupers, ncons, nfields;
    Vector *constructors;
    Map *fields;
    int status = 0;

    ngen = Vector_size(this->generics);
    nsupers = Vector_size(this->supers);
    ncons = Vector_size(this->cons);
    nfields = Vector_size(this->fields);
    if (ngen > 0) {
        print_warning("class generics not implemented\n");
    }
    if (nsupers > 0) {
        print_warning("class inheritance not yet implemented\n");
    }
    for (size_t i = 0; i < ncons; i++) {
        Vector *con = Vector_get(this->cons, i);
        size_t nargs = Vector_size(con);
        for (size_t j = 0; j < nargs; j++) {
            Type *argType = Vector_get(con, j);
            char *msg;
            if (TypeVerify(argType, state, &msg)) {
                print_code_error(stderr, typeLoc(argType), msg);
                free(msg);
                status = 1;
            }
        }
    }
    constructors = copy_Vector(this->cons, (VEC_COPY_FUNC)copy_cons);
    sort_Vector(constructors, (VEC_COMPARATOR)cons_compare);
    fields = Map();
    for (size_t i = 0; i < nfields; i++) {
        struct Field *field = Vector_get(this->fields, i);
        char *msg;
        if (TypeVerify(field->type, state, &msg)) {
            print_code_error(stderr, typeLoc(field->type), msg);
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
                    this->loc,
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
    *typeptr = this->type =
        ClassType(&this->loc, Vector(), Vector(), constructors, fields);
    return 0;
}

static void
delete(ASTClass *this) {
    delete_Vector(this->generics, free);
    delete_Vector(this->supers, (VEC_DELETE_FUNC)delete_type);
    delete_Vector(this->fields, (VEC_DELETE_FUNC)delete_field);
    delete_Vector(this->cons, (VEC_DELETE_FUNC)delete_cons);
    if (NULL != this->type) {
        delete_type(this->type);
    }
    free(this);
}

AST *
new_ASTClass(struct YYLTYPE *loc,
    Vector *generics,
    Vector *inherits,
    struct ClassBody *body) {
    ASTClass *class = NULL;

    class = safe_malloc(sizeof(*class));
    *class = (ASTClass){
        json,
        getType,
        delete,
        *loc,
        generics,
        inherits,
        body->constructors,
        body->fields,
        NULL
    };
    free(body);
    return (AST *)class;
}
