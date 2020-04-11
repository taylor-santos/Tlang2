#include "types.h"
#include "json.h"
#include "safe.h"
#include "vector.h"

static void
json(const void *type, FILE *out, int indent) {
    const struct NoneType *this = type;
    json_start(out, &indent);
    json_label("type", out);
    json_string("none", out, indent);
    if (NULL != this->super.qualifiers) {
        json_comma(out, indent);
        json_label("qualifiers", out);
        json_vector(this->super.qualifiers,
            (JSON_VALUE_FUNC)json_qualifier,
            out,
            indent);
    }
    json_end(out, &indent);
}

static int
compare(UNUSED const void *type,
    const void *otherType,
    UNUSED const TypeCheckState *state) {
    const Type *other = otherType;
    if (TYPE_NONE != other->type) {
        return 1;
    }
    return 0;
}

static int
verify(UNUSED void *type,
    UNUSED const struct TypeCheckState *state,
    UNUSED char **msg) {
    return 0;
}

static char *
toString(UNUSED const void *type) {
    return safe_strdup("none");
}

static char *
codeGen(UNUSED const void *this, UNUSED const char *name) {
    return NULL;
}

static void
delete(void *type) {
    struct NoneType *this = type;
    if (NULL != this->super.qualifiers) {
        delete_Vector(this->super.qualifiers, free);
    }
    free(this);
}

static Type *
copy(const void *type) {
    const struct NoneType *this = type;
    struct NoneType *type_copy = safe_malloc(sizeof(*type_copy));
    Vector *qualifiers = NULL;
    if (NULL != this->super.qualifiers) {
        qualifiers = copy_Vector(this->super.qualifiers,
            (VEC_COPY_FUNC)copy_Qualifiers);
    }
    *type_copy = (struct NoneType){
        {
            json, copy, compare, verify, toString, codeGen, delete, TYPE_NONE, qualifiers, this->super.init, 1, this->super.loc
        }
    };
    return (Type *)type_copy;
}

Type *
new_NoneType(YYLTYPE loc) {
    struct NoneType *type;

    type = safe_malloc(sizeof(*type));
    *type = (struct NoneType){
        {
            json, copy, compare, verify, toString, codeGen, delete, TYPE_NONE, NULL, 0, 0, loc
        }
    };
    return (Type *)type;
}
