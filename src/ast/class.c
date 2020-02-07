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
    Vector *fields;   // Vector<Field*>
    Type *type;       // NULL until type checker is executed.
};

static void
json_field(struct Field *field, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("names", out);
    json_vector(field->names, (JSON_MAP_TYPE)json_string, out, indent);
    json_comma(out, indent);
    json_label("type", out);
    json_type(field->type, out, indent);
    json_end(out, &indent);
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
    json_label("fields", out);
    json_vector(this->fields, (JSON_MAP_TYPE)json_field, out, indent);
    json_end(out, &indent);
}

static int
getType(ASTClass *this, UNUSED TypeCheckState *state, UNUSED Type **typeptr) {
    size_t ngen, nsupers, nfields;
    Map *fields = Map();
    int status = 0;

    ngen = Vector_size(this->generics);
    nsupers = Vector_size(this->supers);
    nfields = Vector_size(this->fields);
    if (ngen > 0) {
        print_warning("class generics not implemented\n");
    }
    if (nsupers > 0) {
        print_warning("class inheritance not yet implemented\n");
    }
    for (size_t i = 0; i < nfields; i++) {
        struct Field *field = NULL;
        Vector_get(this->fields, i, &field);
        size_t nnames = Vector_size(field->names);
        for (size_t j = 0; j < nnames; j++) {
            char *name = NULL;
            Vector_get(field->names, j, &name);
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
        return 1;
    }
    *typeptr = this->type = ClassType(Vector(), Vector(), fields);
    return 0;
}

static void
delete_field(struct Field *field) {
    delete_Vector(field->names, free);
    delete_type(field->type);
    free(field);
}

static void
delete(ASTClass *this) {
    delete_Vector(this->generics, free);
    delete_Vector(this->supers, (VEC_DELETE_FUNC)delete_type);
    delete_Vector(this->fields, (VEC_DELETE_FUNC)delete_field);
    if (NULL != this->type) {
        delete_type(this->type);
    }
    free(this);
}

AST *
new_ASTClass(struct YYLTYPE *loc,
    Vector *generics,
    Vector *inherits,
    Vector *members) {
    ASTClass *class = NULL;

    class = safe_malloc(sizeof(*class));
    *class = (ASTClass){
        json, getType, delete, *loc, generics, inherits, members, NULL
    };
    return (AST *)class;
}
