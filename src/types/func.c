#include "types.h"
#include "json.h"
#include "safe.h"
#include "vector.h"

static void
json(const void *type, FILE *out, int indent) {
    const struct FuncType *this = type;
    json_start(out, &indent);
    json_label("type", out);
    json_string("function", out, indent);
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
    json_label("args", out);
    json_vector(this->args, (JSON_VALUE_FUNC)json_type, out, indent);
    json_comma(out, indent);
    json_label("return type", out);
    json_type(this->ret_type, out, indent);
    json_end(out, &indent);
}

static int
compare(const void *type, const void *otherType, const TypeCheckState *state) {
    const struct FuncType *func1 = type, *func2 = otherType;
    size_t ngens1 = Vector_size(func1->generics),
        ngens2 = Vector_size(func2->generics);
    if (ngens1 > 0 || ngens2 > 0) {
        print_ICE("compare() not implemented for generic functions\n");
        return 1;
    }
    size_t nargs1 = Vector_size(func1->args),
        nargs2 = Vector_size(func2->args);
    if (nargs1 != nargs2) {
        return 1;
    }
    /*
     *      func(A) => B   is a   func(C) => D
     * iff
     *      C   is a   A
     * and
     *      B   is a   D
     */
    for (size_t i = 0; i < nargs1; i++) {
        Type *arg1 = Vector_get(func1->args, i),
            *arg2 = Vector_get(func2->args, i);
        if (arg2->compare(arg2, arg1, state)) {
            return 1;
        }
    }
    return func1->ret_type->compare(func1->ret_type, func2->ret_type, state);
}

static int
verify(void *type, const TypeCheckState *state, char **msg) {
    struct FuncType *func = type;
    size_t nargs = Vector_size(func->args);
    for (size_t i = 0; i < nargs; i++) {
        Type *argType = Vector_get(func->args, i);
        if (argType->verify(argType, state, msg)) {
            return 1;
        }
    }
    return func->ret_type->verify(func->ret_type, state, msg);
}

static char *
toString(UNUSED const void *type) {
    return safe_strdup("function");
}

static void
delete(void *type) {
    struct FuncType *this = type;
    if (NULL != this->super.qualifiers) {
        delete_Vector(this->super.qualifiers, free);
    }
    if (!this->super.isCopy) {
        delete_Vector(this->generics, free);
        delete_Vector(this->args, (VEC_DELETE_FUNC)delete_type);
        delete_type(this->ret_type);
    }
    free(this);
}

static Type *
copy(const void *type) {
    const struct FuncType *this = type;
    struct FuncType *type_copy = safe_malloc(sizeof(*type_copy));
    Vector *qualifiers = NULL;
    if (NULL != this->super.qualifiers) {
        qualifiers = copy_Vector(this->super.qualifiers,
            (VEC_COPY_FUNC)copy_Qualifiers);
    }
    *type_copy = (struct FuncType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            delete,
            TYPE_FUNC,
            qualifiers,
            this->super.init,
            1,
            this->super.loc
        }, this->generics, this->args, this->ret_type
    };
    return (Type *)type_copy;
}

Type *
new_FuncType(YYLTYPE loc, Vector *generics, Vector *args, Type *ret_type) {
    struct FuncType *type;

    type = safe_malloc(sizeof(*type));
    *type = (struct FuncType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            delete,
            TYPE_FUNC,
            NULL,
            0,
            0,
            loc
        }, generics, args, ret_type
    };
    return (Type *)type;
}
