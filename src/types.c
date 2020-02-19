#include <stdlib.h>
#include "types.h"
#include "safe.h"
#include "vector.h"
#include "sparse_vector.h"
#include "json.h"
#include "ast.h"
#include "map.h"
#include "parser.h"

Qualifiers *
copy_Qualifiers(const Qualifiers *q) {
    Qualifiers *new_q;

    if (NULL == (new_q = malloc(sizeof(*new_q)))) {
        print_ICE("");
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    *new_q = *q;
    return new_q;
}

void
json_type(const Type *type, FILE *out, int indent) {
    type->json(type, out, indent);
}

Type *
copy_type(Type *type) {
    return type->copy(type);
}

int
TypeCompare(const Type *type1,
    const Type *type2,
    const TypeCheckState *state) {
    return type1->compare(type1, type2, state);
}

void
json_qualifier(const Qualifiers *value, FILE *out, int indent) {
    switch (*value) {
        case Q_CONST:
            json_string("const", out, indent);
            break;
        case Q_FRIEND:
            json_string("friend", out, indent);
            break;
    }
}

void
delete_type(Type *type) {
    type->delete(type);
}

void
AddComparison(const Type *type, TypeCheckState *state) {
    Map *newCompare = Map();
    Iterator *it = Map_iterator(state->compare);
    while (it->hasNext(it)) {
        MapIterData next = it->next(it);
        Map *m = next.value;
        Type **typePtr = next.key;
        if (!(*typePtr)->compare(*typePtr, type, state)) {
            Map_put(m, &type, sizeof(type), NULL, NULL);
        }
        if (!type->compare(type, *typePtr, state)) {
            Map_put(newCompare, next.key, next.len, NULL, NULL);
        }
    }
    it->delete(it);
    Map_put(state->compare, &type, sizeof(type), newCompare, NULL);
}
