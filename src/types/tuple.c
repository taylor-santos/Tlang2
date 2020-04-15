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
    if (0 != this->super.qualifiers) {
        json_comma(out, indent);
        json_label("qualifiers", out);
        json_qualifier(this->super.qualifiers, out, indent);
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
    const struct TupleType *type1 = type, *type2 = otherType;
    ull count1 = SparseVector_count(type1->types),
        count2 = SparseVector_count(type2->types);
    size_t s1 = SparseVector_size(type1->types),
        s2 = SparseVector_size(type2->types);
    if (count1 != count2) {
        return 1;
    }
    if (count1 == 0) {
        return 0;
    }
    Type *currType1 = NULL, *currType2 = NULL;
    ull currCount1 = 0, currCount2 = 0;
    size_t index1 = 0, index2 = 0;
    do {
        if (currCount1 < currCount2) {
            currCount2 -= currCount1;
            SparseVector_get(type1->types, index1++, &currType1, &currCount1);
        } else if (currCount2 < currCount1) {
            currCount1 -= currCount2;
            SparseVector_get(type2->types, index2++, &currType2, &currCount2);
        } else {
            SparseVector_get(type1->types, index1++, &currType1, &currCount1);
            SparseVector_get(type2->types, index2++, &currType2, &currCount2);
        }
        if (currType1->compare(currType1, currType2, state)) {
            return 1;
        }
    } while (index1 < s1 && index2 < s2);

    return 0;
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
    SparseVector_reduce(tuple->types,
        (SVEC_COMPARE_FUNC)TypeCompare,
        state,
        (SVEC_DELETE_FUNC)delete_type);
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
        vappend_str(&str, "%s%s", sep, s);
        free(s);
        if (c > 1) {
            vappend_str(&str, "..%" PRId64, c);
        }
        sep = ", ";
    }
    append_str(&str, ")");
    return str.str;
}

static char *
codeGen(UNUSED const void *this, UNUSED const char *name) {
    return NULL;
}

static void
delete(void *type) {
    struct TupleType *this = type;
    if (!this->super.isCopy) {
        delete_SparseVector(this->types, (VEC_DELETE_FUNC)delete_type);
    }
    free(this);
}

static Type *
copy(const void *type) {
    const struct TupleType *this = type;
    struct TupleType *type_copy = safe_malloc(sizeof(*type_copy));
    *type_copy = (struct TupleType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            codeGen,
            delete,
            TYPE_TUPLE,
            this->super.qualifiers,
            this->super.init,
            1,
            this->super.isRef,
            this->super.loc
        },
        this->types
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
            codeGen,
            delete,
            TYPE_TUPLE,
            0,
            0,
            0,
            0,
            loc
        },
        types
    };
    return (Type *)type;
}
