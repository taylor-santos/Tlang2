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
AddComparison(const struct ClassType *type, TypeCheckState *state) {
    if (Map_contains(state->compare, &type, sizeof(type))) {
        return;
    }
    Map *newCompare = Map();
    Iterator *it = Map_iterator(state->compare);
    while (it->hasNext(it)) {
        MapIterData next = it->next(it);
        Map *m = next.value;
        struct ClassType **typePtr = next.key;
        if (!compare_ClassType(*typePtr, type, state)) {
            Map_put(m, &type, sizeof(type), NULL, NULL);
        }
        if (!compare_ClassType(type, *typePtr, state)) {
            Map_put(newCompare, next.key, next.len, NULL, NULL);
        }
    }
    it->delete(it);
    Map_put(state->compare, &type, sizeof(type), newCompare, NULL);
}

int
AddSymbol(const char *symbol,
    size_t len,
    Type *type,
    const TypeCheckState *state,
    char **msg) {
    Type *prev_type = NULL;
    if (Map_get(state->symbols, symbol, len, &prev_type)) {
        Type *type_copy = type->copy(type);
        Map_put(state->symbols, symbol, len, type_copy, NULL);
        return 0;
    }
    if (TYPE_FUNC == prev_type->type && TYPE_FUNC == type->type) {
        struct FuncType *func1 = (struct FuncType *)prev_type,
            *func2 = (struct FuncType *)type->copy(type);
        while (NULL != func1->next) {
            func1 = func1->next;
        }
        func1->next = func2;
        return 0;
    }
    if (type->compare(type, prev_type, state)) {
        char *oldName = prev_type->toString(prev_type),
            *newName = type->toString(type);
        *msg = safe_asprintf(
            "redefinition of variable \"%.*s\" from type \"%s\" to type "
            "\"%s\"",
            (int)len,
            symbol,
            oldName,
            newName);
        free(oldName);
        free(newName);
        return 1;
    }
    if (1 == type->init) {
        prev_type->init = 1;
        if (NULL != state->newInitSymbols) {
            Map_put(state->newInitSymbols, symbol, len, NULL, NULL);
        }
    }
    return 0;
}
