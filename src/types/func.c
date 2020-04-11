#include "types.h"
#include "json.h"
#include "safe.h"
#include "vector.h"
#include "dynamic_string.h"

static void
json_overload(const void *overload, FILE *out, int indent) {
    const struct FuncType *this = overload;
    json_start(out, &indent);
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

static const void *
next_overload(const void *overload) {
    const struct FuncType *this = overload;
    return this->next;
}

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
    json_label("overloads", out);
    json_linked_list(this, json_overload, next_overload, out, indent);
    json_end(out, &indent);
}

static int
compare_overload(const struct FuncType *func1,
    const struct FuncType *func2,
    const TypeCheckState *state) {
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
compare(const void *type, const void *otherType, const TypeCheckState *state) {
    const Type *other = otherType;
    if (TYPE_FUNC != other->type) {
        return 1;
    }
    for (const struct FuncType *func2 = otherType;
        NULL != func2;
        func2 = func2->next) {
        int found = 0;
        for (const struct FuncType *func1 = type;
            NULL != func1;
            func1 = func1->next) {
            if (!compare_overload(func1, func2, state)) {
                found = 1;
                break;
            }
        }
        if (!found) {
            return 1;
        }
    }
    return 0;
}

static int
verify_overload(struct FuncType *func,
    const TypeCheckState *state,
    char **msg) {
    size_t nargs = Vector_size(func->args);
    for (size_t i = 0; i < nargs; i++) {
        Type *argType = Vector_get(func->args, i);
        if (argType->verify(argType, state, msg)) {
            return 1;
        }
    }
    return func->ret_type->verify(func->ret_type, state, msg);
}

static int
verify(void *type, const TypeCheckState *state, char **msg) {
    for (struct FuncType *f = type; NULL != f; f = f->next) {
        if (verify_overload(f, state, msg)) {
            return 1;
        }
    }
    return 0;
}

static void
overload_toString(const struct FuncType *func, dstring *str) {
    append_str(str, "func(");
    char *sep = "";
    size_t n = Vector_size(func->args);
    for (size_t i = 0; i < n; i++) {
        Type *t = Vector_get(func->args, i);
        char *s = t->toString(t);
        vappend_str(str, "%s%s", sep, s);
        free(s);
        sep = ", ";
    }
    char *s = func->ret_type->toString(func->ret_type);
    vappend_str(str, ") => %s", s);
    free(s);
}

static char *
toString(const void *type) {
    dstring str = dstring("{");
    char *sep = "";
    for (const struct FuncType *func = type; NULL != func; func = func->next) {
        append_str(&str, sep);
        overload_toString(func, &str);
        sep = ", ";
    }
    append_str(&str, "}");
    return str.str;
}

static char *
codeGen(const void *this, const char *name) {
    const struct FuncType *type = this;
    char *retType = type->ret_type->codeGen(type->ret_type, NULL);
    const char *ident = name == NULL
        ? ""
        : name;
    dstring ret = dstring(retType);
    free(retType);
    vappend_str(&ret, " (*%s)(", ident);
    size_t n = Vector_size(type->args);
    char *sep = "";
    for (size_t i = 0; i < n; i++) {
        Type *arg = Vector_get(type->args, i);
        char *argName = arg->codeGen(arg, NULL);
        vappend_str(&ret, "%s%s", sep, argName);
        free(argName);
        sep = ", ";
    }
    append_str(&ret, ")");
    return ret.str;
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
    if (NULL != this->next) {
        delete(this->next);
    }
    free(this);
}

static Type *
copy(const void *type) {
    const struct FuncType *this = type;
    struct FuncType *next_copy = NULL;
    struct FuncType *type_copy = safe_malloc(sizeof(*type_copy));
    Vector *qualifiers = NULL;
    if (NULL != this->super.qualifiers) {
        qualifiers = copy_Vector(this->super.qualifiers,
            (VEC_COPY_FUNC)copy_Qualifiers);
    }
    if (NULL != this->next) {
        next_copy = (struct FuncType *)copy(this->next);
    }
    *type_copy = (struct FuncType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            codeGen,
            delete,
            TYPE_FUNC,
            qualifiers,
            this->super.init,
            1,
            this->super.loc
        }, this->generics, this->args, this->ret_type, next_copy
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
            codeGen,
            delete,
            TYPE_FUNC,
            NULL,
            0,
            0,
            loc
        }, generics, args, ret_type, NULL
    };
    return (Type *)type;
}
