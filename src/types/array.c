#include "types.h"
#include "json.h"
#include "safe.h"
#include "sparse_vector.h"
#include "vector.h"

static void
json(const void *type, FILE *out, int indent) {
    const struct ArrayType *this = type;
    json_start(out, &indent);
    json_label("type", out);
    json_string("array", out, indent);
    if (NULL != this->super.qualifiers) {
        json_comma(out, indent);
        json_label("qualifiers", out);
        json_vector(this->super.qualifiers,
            (JSON_VALUE_FUNC)json_qualifier,
            out,
            indent);
    }
    json_comma(out, indent);
    json_label("type", out);
    json_type(this->type, out, indent);
    json_end(out, &indent);
}

static int
compare(const void *type, const void *otherType, const TypeCheckState *state) {
    const struct ArrayType *array1 = type, *array2 = otherType;
    return array1->type->compare(array1->type, array2->type, state);
}

static int
verify(void *type, const TypeCheckState *state, char **msg) {
    struct ArrayType *array = type;
    return array->type->verify(array->type, state, msg);
}

static char *
toString(const void *type) {
    const struct ArrayType *this = type;
    char *typeName = this->type->toString(this->type);
    char *name = safe_asprintf("array of %s", typeName);
    free(typeName);
    return name;
}

static void
delete(void *type) {
    struct ArrayType *this = type;
    if (NULL != this->super.qualifiers) {
        delete_Vector(this->super.qualifiers, free);
    }
    if (!this->super.isCopy) {
        delete_type(this->type);
    }
    free(this);
}

static Type *
copy(const void *type) {
    const struct ArrayType *this = type;
    struct ArrayType *type_copy = safe_malloc(sizeof(*type_copy));
    Vector *qualifiers = NULL;
    if (NULL != this->super.qualifiers) {
        qualifiers = copy_Vector(this->super.qualifiers,
            (VEC_COPY_FUNC)copy_Qualifiers);
    }
    *type_copy = (struct ArrayType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            delete,
            TYPE_ARRAY,
            qualifiers,
            this->super.init,
            1,
            this->super.loc
        }, this->type
    };
    return (Type *)type_copy;
}

Type *
new_ArrayType(YYLTYPE loc, Type *type) {
    struct ArrayType *array;

    array = safe_malloc(sizeof(*type));
    *array = (struct ArrayType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            delete,
            TYPE_ARRAY,
            NULL,
            0,
            0,
            loc
        }, type
    };
    return (Type *)array;
}
