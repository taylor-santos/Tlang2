#include "types.h"
#include "json.h"
#include "safe.h"
#include "sparse_vector.h"
#include "vector.h"

static void
json(const void *type, FILE *out, int indent) {
    const struct MaybeType *this = type;
    json_start(out, &indent);
    json_label("type", out);
    json_string("maybe", out, indent);
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
    const Type *other = otherType;
    if (TYPE_MAYBE != other->type) {
        return 1;
    }
    const struct MaybeType *maybe1 = type, *maybe2 = otherType;
    return maybe1->type->compare(maybe1->type, maybe2->type, state);
}

static int
verify(void *type, const struct TypeCheckState *state, char **msg) {
    struct MaybeType *maybe = type;
    return maybe->type->verify(maybe->type, state, msg);
}

static char *
toString(const void *type) {
    const struct MaybeType *this = type;
    char *typeName = this->type->toString(this->type);
    char *name = safe_asprintf("maybe of %s", typeName);
    free(typeName);
    return name;
}

static char *
codeGen(UNUSED const void *this, UNUSED const char *name) {
    return NULL;
}

static void
delete(void *type) {
    struct MaybeType *this = type;
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
    const struct MaybeType *this = type;
    struct MaybeType *type_copy = safe_malloc(sizeof(*type_copy));
    Vector *qualifiers = NULL;
    if (NULL != this->super.qualifiers) {
        qualifiers = copy_Vector(this->super.qualifiers,
            (VEC_COPY_FUNC)copy_Qualifiers);
    }
    *type_copy = (struct MaybeType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            codeGen,
            delete,
            TYPE_MAYBE,
            qualifiers,
            this->super.init,
            1,
            this->super.loc
        }, this->type
    };
    return (Type *)type_copy;
}

Type *
new_MaybeType(YYLTYPE loc, Type *type) {
    struct MaybeType *maybe;

    maybe = safe_malloc(sizeof(*type));
    *maybe = (struct MaybeType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            codeGen,
            delete,
            TYPE_MAYBE,
            NULL,
            0,
            0,
            loc
        }, type
    };
    return (Type *)maybe;
}
