#include "types.h"
#include "json.h"
#include "safe.h"
#include "vector.h"
#include "map.h"
#include "dynamic_string.h"

static void
json_constructor(Vector *cons, FILE *out, int indent) {
    json_vector(cons, (JSON_VALUE_FUNC)json_type, out, indent);
}

static void
json(const void *type, FILE *out, int indent) {
    const struct ClassType *this = type;
    json_start(out, &indent);
    json_label("type", out);
    json_string("class", out, indent);
    if (NULL != this->super.qualifiers) {
        json_comma(out, indent);
        json_label("qualifiers", out);
        json_vector(this->super.qualifiers,
            (JSON_VALUE_FUNC)json_qualifier,
            out,
            indent);
    }
    json_comma(out, indent);
    json_label("generics", out);
    json_vector(this->generics, (JSON_VALUE_FUNC)json_string, out, indent);
    json_comma(out, indent);
    json_label("supers", out);
    json_vector(this->supers, (JSON_VALUE_FUNC)json_string, out, indent);
    json_comma(out, indent);
    json_label("constructors", out);
    json_vector(this->constructors,
        (JSON_VALUE_FUNC)json_constructor,
        out,
        indent);
    json_comma(out, indent);
    json_label("fields", out);
    json_Map(this->fields,
        (JSON_KEY_FUNC)json_nlabel,
        (JSON_VALUE_FUNC)json_type,
        out,
        indent);
    json_end(out, &indent);
}

static int
compare(const void *type, const void *otherType, const TypeCheckState *state) {
    const Type *other = otherType;
    if (TYPE_CLASS != other->type) {
        return 1;
    }
    const struct ClassType *class1 = type, *class2 = otherType;
    if (class1->fields == class2->fields) {
        // One is a copy of the other if their pointers are the same
        return 0;
    }
    Map *compare;
    if (!Map_get(state->compare, &class1, sizeof(class1), &compare)) {
        return !Map_contains(compare, &class2, sizeof(class2));
    }
    size_t ngens1 = Vector_size(class1->generics),
        ngens2 = Vector_size(class2->generics);
    if (ngens1 > 0 || ngens2 > 0) {
        // TODO: generic classes
        print_ICE("TypeCompare not implemented for generic objects\n");
        return 1;
    }
    Iterator *field_it = Map_iterator(class2->fields);
    while (field_it->hasNext(field_it)) {
        MapIterData field = field_it->next(field_it);
        Type *argType = NULL;
        if (Map_get(class1->fields, field.key, field.len, &argType) ||
            argType->compare(argType, field.value, state)) {
            field_it->delete(field_it);
            return 1;
        }
    }
    field_it->delete(field_it);
    return 0;
}

static int
verify(UNUSED void *type,
    UNUSED const TypeCheckState *state,
    UNUSED char **msg) {
    // Class types are checked on creation
    return 0;
}

static char *
toString(const void *type) {
    const struct ClassType *this = type;
    dstring str = dstring("class{");
    Iterator *it = Map_iterator(this->fields);
    char *sep = "";
    while (it->hasNext(it)) {
        MapIterData field = it->next(it);
        Type *fieldType = field.value;
        char *typeName = fieldType->toString(fieldType);
        append_vstr(&str, "%s%.*s:%s", sep, field.len, field.key, typeName);
        free(typeName);
        sep = ",";
    }
    it->delete(it);
    append_str(&str, "}");
    return str.str;
}

static void
delete_cons(Vector *cons) {
    delete_Vector(cons, (VEC_DELETE_FUNC)delete_type);
}

static void
delete(void *type) {
    struct ClassType *this = type;
    if (NULL != this->super.qualifiers) {
        delete_Vector(this->super.qualifiers, free);
    }
    if (!this->super.isCopy) {
        delete_Vector(this->generics, free);
        delete_Vector(this->supers, free);
        delete_Vector(this->constructors, (VEC_DELETE_FUNC)delete_cons);
        delete_Map(this->fields, (MAP_DELETE_FUNC)delete_type);
    }
    free(this);
}

static Type *
copy(const void *type) {
    const struct ClassType *this = type;
    struct ClassType *type_copy = safe_malloc(sizeof(*type_copy));
    Vector *qualifiers = NULL;
    if (NULL != this->super.qualifiers) {
        qualifiers = copy_Vector(this->super.qualifiers,
            (VEC_COPY_FUNC)copy_Qualifiers);
    }
    *type_copy = (struct ClassType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            delete,
            TYPE_CLASS,
            qualifiers,
            this->super.init,
            1,
            this->super.loc
        }, this->generics, this->supers, this->constructors, this->fields
    };
    return (Type *)type_copy;
}

Type *
new_ClassType(YYLTYPE loc,
    struct Vector *generics,
    struct Vector *supers,
    struct Vector *cons,
    struct Map *fields) {
    struct ClassType *type;

    type = safe_malloc(sizeof(*type));
    *type = (struct ClassType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            delete,
            TYPE_CLASS,
            NULL,
            0,
            0,
            loc
        }, generics, supers, cons, fields
    };
    return (Type *)type;
}
