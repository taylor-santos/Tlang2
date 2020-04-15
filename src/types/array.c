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
    if (0 != this->super.qualifiers) {
        json_comma(out, indent);
        json_label("qualifiers", out);
        json_qualifier(this->super.qualifiers, out, indent);
    }
    json_comma(out, indent);
    json_label("type", out);
    json_type(this->type, out, indent);
    json_end(out, &indent);
}

static int
compare(const void *type, const void *otherType, const TypeCheckState *state) {
    const Type *other = otherType;
    if (TYPE_ARRAY != other->type) {
        return 1;
    }
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

static char *
codeGen(UNUSED const void *this, UNUSED const char *name) {
    return NULL;
}

static void
delete(void *type) {
    struct ArrayType *this = type;
    if (!this->super.isCopy) {
        delete_type(this->type);
    }
    free(this);
}

static Type *
copy(const void *type) {
    const struct ArrayType *this = type;
    struct ArrayType *type_copy = safe_malloc(sizeof(*type_copy));
    *type_copy = (struct ArrayType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            codeGen,
            delete,
            TYPE_ARRAY,
            this->super.qualifiers,
            this->super.init,
            1,
            0,
            this->super.loc
        },
        this->type
    };
    return (Type *)type_copy;
}

Type *
new_ArrayType(YYLTYPE loc, Type *type) {
    struct ArrayType *array;

    array = safe_malloc(sizeof(*array));
    *array = (struct ArrayType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            codeGen,
            delete,
            TYPE_ARRAY,
            0,
            0,
            0,
            0,
            loc
        },
        type
    };
    return (Type *)array;
}
