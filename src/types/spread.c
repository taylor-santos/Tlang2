#include "types.h"
#include "json.h"
#include "safe.h"
#include "sparse_vector.h"
#include "vector.h"
#include "dynamic_string.h"

static void
json(const void *type, FILE *out, int indent) {
    const struct SpreadType *this = type;
    json_start(out, &indent);
    json_label("type", out);
    json_string("spread", out, indent);
    if (NULL != this->super.qualifiers) {
        json_comma(out, indent);
        json_label("qualifiers", out);
        json_vector(this->super.qualifiers,
            (JSON_VALUE_FUNC)json_qualifier,
            out,
            indent);
    }
    json_comma(out, indent);
    json_label("types", out);
    json_sparse_vector(this->types, (JSON_VALUE_FUNC)json_type, out, indent);
    json_end(out, &indent);
}

static int
compare(UNUSED const void *type,
    const void *otherType,
    UNUSED const TypeCheckState *state) {
    const Type *other = otherType;
    if (TYPE_SPREAD != other->type) {
        return 1;
    }
    print_ICE("TypeCompare not implemented for spreads\n");
    return 1;
}

static int
verify(void *type, const TypeCheckState *state, char **msg) {
    struct SpreadType *spread = type;
    size_t n = SparseVector_size(spread->types);
    for (size_t i = 0; i < n; i++) {
        Type *t;
        SparseVector_get(spread->types, i, &t, NULL);
        if (t->verify(t, state, msg)) {
            return 1;
        }
    }
    return 0;
}

static char *
toString(const void *type) {
    const struct SpreadType *this = type;
    dstring str = dstring("*(");
    char *sep = "";
    size_t n = SparseVector_size(this->types);
    for (size_t i = 0; i < n; i++) {
        Type *t;
        unsigned long long c;
        SparseVector_get(this->types, i, &t, &c);
        char *s = t->toString(t);
        append_vstr(&str, "%s%s", sep, s);
        free(s);
        sep = ",";
    }
    append_str(&str, ")");
    return str.str;
}

static void
delete(void *type) {
    struct SpreadType *this = type;
    if (NULL != this->super.qualifiers) {
        delete_Vector(this->super.qualifiers, free);
    }
    if (!this->super.isCopy) {
        delete_SparseVector(this->types, (VEC_DELETE_FUNC)delete_type);
    }
    free(this);
}

static Type *
copy(const void *type) {
    const struct SpreadType *this = type;
    struct SpreadType *type_copy = safe_malloc(sizeof(*type_copy));
    Vector *qualifiers = NULL;
    if (NULL != this->super.qualifiers) {
        qualifiers = copy_Vector(this->super.qualifiers,
            (VEC_COPY_FUNC)copy_Qualifiers);
    }
    *type_copy = (struct SpreadType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            delete,
            TYPE_SPREAD,
            qualifiers,
            this->super.init,
            1,
            this->super.loc
        }, this->types
    };
    return (Type *)type_copy;
}

Type *
new_SpreadType(struct TupleType *tuple) {
    struct SpreadType *type;

    type = safe_malloc(sizeof(*type));
    *type = (struct SpreadType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            delete,
            TYPE_SPREAD,
            NULL,
            0,
            0,
            tuple->super.loc
        }, tuple->types
    };
    return (Type *)type;
}
