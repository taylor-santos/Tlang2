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
    if (0 != this->super.qualifiers) {
        json_comma(out, indent);
        json_label("qualifiers", out);
        json_qualifier(this->super.qualifiers, out, indent);
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
    free(this);
}

static Type *
copy(const void *type) {
    const struct NoneType *this = type;
    struct NoneType *type_copy = safe_malloc(sizeof(*type_copy));
    *type_copy = (struct NoneType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            codeGen,
            delete,
            TYPE_NONE,
            this->super.qualifiers,
            this->super.init,
            1,
            this->super.isRef,
            this->super.loc
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
            json,
            copy,
            compare,
            verify,
            toString,
            codeGen,
            delete,
            TYPE_NONE,
            0,
            0,
            0,
            0,
            loc
        }
    };
    return (Type *)type;
}
