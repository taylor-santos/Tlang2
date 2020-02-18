#include "types.h"
#include "json.h"
#include "safe.h"
#include "sparse_vector.h"
#include "vector.h"
#include "dynamic_string.h"

static void
json(const void *type, FILE *out, int indent) {
    const struct TupleType *this = type;
    json_start(out, &indent);
    json_label("type", out);
    json_string("tuple", out, indent);
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
    if (TYPE_TUPLE != other->type) {
        return 1;
    }
    print_ICE("TypeCompare not implemented for tuples\n");
    return 1;
}

static int
verify(void *type, const struct TypeCheckState *state, char **msg) {
    struct TupleType *tuple = type;
    size_t n = SparseVector_size(tuple->types);
    for (size_t i = 0; i < n; i++) {
        Type *t;
        SparseVector_get(tuple->types, i, &t, NULL);
        if (t->verify(t, state, msg)) {
            return 1;
        }
    }
    return 0;
}

static char *
toString(const void *type) {
    const struct TupleType *this = type;
    dstring str = dstring("(");
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
    struct TupleType *this = type;
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
    const struct TupleType *this = type;
    struct TupleType *type_copy = safe_malloc(sizeof(*type_copy));
    Vector *qualifiers = NULL;
    if (NULL != this->super.qualifiers) {
        qualifiers = copy_Vector(this->super.qualifiers,
            (VEC_COPY_FUNC)copy_Qualifiers);
    }
    *type_copy = (struct TupleType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            delete,
            TYPE_TUPLE,
            qualifiers,
            this->super.init,
            1,
            this->super.loc
        }, this->types
    };
    return (Type *)type_copy;
}

Type *
new_TupleType(YYLTYPE loc, struct SparseVector *types) {
    struct TupleType *type;

    type = safe_malloc(sizeof(*type));
    *type = (struct TupleType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            delete,
            TYPE_TUPLE,
            NULL,
            0,
            0,
            loc
        }, types
    };
    return (Type *)type;
}
