#include <stdlib.h>
#include "types.h"
#include "safe.h"
#include "vector.h"
#include "sparse_vector.h"
#include "json.h"
#include "ast.h"
#include "map.h"
#include "parser.h"

struct Type {
    Types type;
    Vector *qualifiers; // Vector<Qualifiers*>
    unsigned char init : 1;
    YYLTYPE loc;
    union {
        struct FuncType func;
        struct ClassType class;
        struct ObjectType object;
        struct ExprType expr;
        struct TupleType tuple;
        struct SpreadType spread;
        struct NamedType named;
        struct ArrayType array;
        struct MaybeType maybe;
    };
};

static Qualifiers *
copy_Qualifiers(const Qualifiers *q) {
    Qualifiers *new_q;

    if (NULL == (new_q = malloc(sizeof(*new_q)))) {
        print_ICE("");
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    *new_q = *q;
    return new_q;
}

static void
json_constructor(Vector *cons, FILE *out, int indent) {
    json_vector(cons, (JSON_MAP_TYPE)json_type, out, indent);
}

void
json_type(const Type *type, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("type", out);
    switch (type->type) {
        case TYPE_FUNC:
            json_string("function", out, indent);
            break;
        case TYPE_CLASS:
            json_string("class", out, indent);
            break;
        case TYPE_OBJECT:
            json_string("object", out, indent);
            break;
        case TYPE_EXPR:
            json_string("expression", out, indent);
            break;
        case TYPE_TUPLE:
            json_string("tuple", out, indent);
            break;
        case TYPE_SPREAD:
            json_string("spread", out, indent);
            break;
        case TYPE_NAMED:
            json_string("named", out, indent);
            break;
        case TYPE_NONE:
            json_string("none", out, indent);
            break;
        case TYPE_ARRAY:
            json_string("array", out, indent);
            break;
        case TYPE_MAYBE:
            json_string("maybe", out, indent);
            break;
    }
    if (NULL != type->qualifiers) {
        json_comma(out, indent);
        json_label("qualifiers", out);
        json_vector(type->qualifiers,
            (JSON_MAP_TYPE)json_qualifier,
            out,
            indent);
    }
    switch (type->type) {
        case TYPE_FUNC:
            json_comma(out, indent);
            json_label("generics", out);
            json_vector(type->func.generics,
                (JSON_MAP_TYPE)json_string,
                out,
                indent);
            json_comma(out, indent);
            json_label("args", out);
            json_vector(type->func.args,
                (JSON_MAP_TYPE)json_type,
                out,
                indent);
            json_comma(out, indent);
            json_label("return type", out);
            json_type(type->func.ret_type, out, indent);
            break;
        case TYPE_CLASS:
            json_comma(out, indent);
            json_label("generics", out);
            json_vector(type->class.generics,
                (JSON_MAP_TYPE)json_string,
                out,
                indent);
            json_comma(out, indent);
            json_label("supers", out);
            json_vector(type->class.supers,
                (JSON_MAP_TYPE)json_string,
                out,
                indent);
            json_comma(out, indent);
            json_label("constructors", out);
            json_vector(type->class.constructors,
                (JSON_MAP_TYPE)json_constructor,
                out,
                indent);
            json_comma(out, indent);
            json_label("fields", out);
            json_Map(type->class.fields,
                (JSON_MAP_TYPE)json_type,
                out,
                indent);
            break;
        case TYPE_OBJECT:
            json_comma(out, indent);
            json_label("name", out);
            json_string(type->object.name, out, indent);
            json_comma(out, indent);
            json_label("generics", out);
            json_vector(type->object.generics,
                (JSON_MAP_TYPE)json_string,
                out,
                indent);
            break;
        case TYPE_EXPR:
            json_comma(out, indent);
            json_label("expr", out);
            json_AST(type->expr.expr, out, indent);
            json_comma(out, indent);
            json_label("generics", out);
            json_vector(type->expr.generics,
                (JSON_MAP_TYPE)json_string,
                out,
                indent);
            break;
        case TYPE_TUPLE:
            json_comma(out, indent);
            json_label("types", out);
            json_sparse_vector(type->tuple.types,
                (JSON_MAP_TYPE)json_type,
                out,
                indent);
            break;
        case TYPE_SPREAD:
            json_comma(out, indent);
            json_label("types", out);
            json_sparse_vector(type->spread.types,
                (JSON_MAP_TYPE)json_type,
                out,
                indent);
            break;
        case TYPE_NAMED:
            json_comma(out, indent);
            json_label("name", out);
            json_string(type->named.name, out, indent);
            json_comma(out, indent);
            json_label("type", out);
            json_type(type->named.type, out, indent);
            break;
        case TYPE_NONE:
            break;
        case TYPE_ARRAY:
            json_comma(out, indent);
            json_label("type", out);
            json_type(type->array.type, out, indent);
            break;
        case TYPE_MAYBE:
            json_comma(out, indent);
            json_label("type", out);
            json_type(type->maybe.type, out, indent);
            break;
    }
    json_end(out, &indent);
}

void
json_qualifier(const Qualifiers *value, FILE *out, int indent) {
    switch (*value) {
        case Q_CONST:
            json_string("const", out, indent);
            break;
        case Q_FRIEND:
            json_string("friend", out, indent);
            break;
    }
}

struct FuncType
copy_FuncType(struct FuncType func) {
    Vector *generics;
    Vector *args;
    Type *ret_type = NULL;

    generics = copy_Vector(func.generics, (VEC_COPY_FUNC)safe_strdup_func);
    args = copy_Vector(func.args, (VEC_COPY_FUNC)copy_type);
    ret_type = copy_type(func.ret_type);
    return (struct FuncType){
        generics, args, ret_type
    };
}

static Vector *
copy_constructor(const Vector *cons) {
    return copy_Vector(cons, (VEC_COPY_FUNC)copy_type);
}

struct ClassType
copy_ClassType(struct ClassType class) {
    Vector *generics, *supers, *cons;
    Map *fields;

    generics = copy_Vector(class.generics, (VEC_COPY_FUNC)safe_strdup_func);
    supers = copy_Vector(class.supers, (VEC_COPY_FUNC)safe_strdup_func);
    cons = copy_Vector(class.constructors, (VEC_COPY_FUNC)copy_constructor);
    fields = copy_Map(class.fields, (MAP_COPY_FUNC)copy_type);
    return (struct ClassType){
        generics, supers, cons, fields
    };
}

struct ObjectType
copy_ObjectType(struct ObjectType object) {
    char *name;
    Vector *generics;

    name = safe_strdup(object.name);
    generics = copy_Vector(object.generics, (VEC_COPY_FUNC)safe_strdup_func);
    return (struct ObjectType){
        name, generics, object.class
    };
}

struct ExprType
copy_ExprType(struct ExprType expr) {
    Vector *generics;
    generics = copy_Vector(expr.generics, (VEC_COPY_FUNC)safe_strdup_func);
    return (struct ExprType){
        expr.expr, generics, 0
    };
}

struct TupleType
copy_TupleType(struct TupleType tuple) {
    SparseVector *types;

    types = copy_SparseVector(tuple.types, (SVEC_COPY_FUNC)copy_type);
    return (struct TupleType){
        types
    };
}

struct SpreadType
copy_SpreadType(struct SpreadType spread) {
    SparseVector *types;

    types = copy_SparseVector(spread.types, (SVEC_COPY_FUNC)copy_type);
    return (struct SpreadType){
        types
    };
}

struct NamedType
copy_NamedType(struct NamedType named) {
    char *name;
    Type *type;

    name = safe_strdup(named.name);
    type = copy_type(named.type);
    return (struct NamedType){
        name, type
    };
}

struct ArrayType
copy_ArrayType(struct ArrayType array) {
    Type *type;

    type = copy_type(array.type);
    return (struct ArrayType){
        type
    };
}

struct MaybeType
copy_MaybeType(struct MaybeType maybe) {
    Type *type;

    type = copy_type(maybe.type);
    return (struct MaybeType){
        type
    };
}

Type *
copy_type(const Type *type) {
    Type *new_type;
    Vector *qualifiers = NULL;

    if (NULL == (new_type = malloc(sizeof(*new_type)))) {
        print_ICE("");
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    if (NULL != type->qualifiers) {
        qualifiers =
            copy_Vector(type->qualifiers, (VEC_COPY_FUNC)copy_Qualifiers);
    }
    *new_type = (Type){
        type->type, qualifiers, type->init, type->loc, { { 0 } }
    };
    switch (type->type) {
        case TYPE_FUNC:
            new_type->func = copy_FuncType(type->func);
            break;
        case TYPE_CLASS:
            new_type->class = copy_ClassType(type->class);
            break;
        case TYPE_OBJECT:
            new_type->object = copy_ObjectType(type->object);
            break;
        case TYPE_EXPR:
            new_type->expr = copy_ExprType(type->expr);
            break;
        case TYPE_TUPLE:
            new_type->tuple = copy_TupleType(type->tuple);
            break;
        case TYPE_SPREAD:
            new_type->spread = copy_SpreadType(type->spread);
            break;
        case TYPE_NAMED:
            new_type->named = copy_NamedType(type->named);
            break;
        case TYPE_NONE:
            break;
        case TYPE_ARRAY:
            new_type->array = copy_ArrayType(type->array);
            break;
        case TYPE_MAYBE:
            new_type->maybe = copy_MaybeType(type->maybe);
            break;
    }
    return new_type;
}

static void
delete_cons(Vector *cons) {
    delete_Vector(cons, (VEC_DELETE_FUNC)delete_type);
}

void
delete_type(Type *type) {
    switch (type->type) {
        case TYPE_FUNC:
            delete_Vector(type->func.generics, free);
            delete_Vector(type->func.args, (VEC_DELETE_FUNC)delete_type);
            delete_type(type->func.ret_type);
            break;
        case TYPE_CLASS:
            delete_Vector(type->class.generics, free);
            delete_Vector(type->class.supers, free);
            delete_Vector(type->class.constructors,
                (VEC_DELETE_FUNC)delete_cons);
            delete_Map(type->class.fields, (MAP_DELETE_FUNC)delete_type);
            break;
        case TYPE_OBJECT:
            free(type->object.name);
            delete_Vector(type->object.generics, free);
            break;
        case TYPE_EXPR:
            if (type->expr.ownsAST) {
                delete_AST(type->expr.expr);
            }
            delete_Vector(type->expr.generics, free);
            break;
        case TYPE_TUPLE:
            delete_SparseVector(type->tuple.types,
                (VEC_DELETE_FUNC)delete_type);
            break;
        case TYPE_SPREAD:
            delete_SparseVector(type->spread.types,
                (VEC_DELETE_FUNC)delete_type);
            break;
        case TYPE_NAMED:
            free(type->named.name);
            delete_type(type->named.type);
            break;
        case TYPE_NONE:
            break;
        case TYPE_ARRAY:
            delete_type(type->array.type);
            break;
        case TYPE_MAYBE:
            delete_type(type->maybe.type);
            break;
    }
    if (NULL != type->qualifiers) {
        delete_Vector(type->qualifiers, free);
    }
    free(type);
}

void
setTypeQualifiers(Type *type, Vector *qualifiers) {
    type->qualifiers = qualifiers;
}

int
FuncTypeCompare(const struct FuncType *type1,
    const struct FuncType *type2,
    const TypeCheckState *state) {
    size_t ngens1 = Vector_size(type1->generics),
        ngens2 = Vector_size(type2->generics);
    if (ngens1 > 0 || ngens2 > 0) {
        print_ICE("TypeCompare not implemented for generic functions\n");
        return 1;
    }
    size_t nargs1 = Vector_size(type1->args),
        nargs2 = Vector_size(type2->args);
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
        Type *arg1 = Vector_get(type1->args, i),
            *arg2 = Vector_get(type2->args, i);
        if (TypeCompare(arg2, arg1, state)) {
            return 1;
        }
    }
    return TypeCompare(type1->ret_type, type2->ret_type, state);
}

static int
ObjectTypeCompare(const struct ObjectType *type1,
    const struct ObjectType *type2,
    const TypeCheckState *state) {
    if (0 == strcmp(type1->name, type2->name)) {
        return 0;
    }
    if (type1->class == NULL || type2->class == NULL) {
        print_ICE("Unverified object type\n");
        exit(EXIT_FAILURE);
    }
    const struct ClassType *class1 = type1->class, *class2 = type2->class;
    size_t ngens1 = Vector_size(class1->generics),
        ngens2 = Vector_size(class2->generics);
    if (ngens1 > 0 || ngens2 > 0) {
        print_ICE("TypeCompare not implemented for generic objects\n");
        return 1;
    }
    Iterator *field_it = Map_iterator(class2->fields);
    while (field_it->hasNext(field_it)) {
        MapIterData field = field_it->next(field_it);
        Type *argType = NULL;
        if (Map_get(class1->fields, field.key, field.len, &argType) ||
            TypeCompare(argType, field.value, state)) {
            field_it->delete(field_it);
            return 1;
        }
    }
    field_it->delete(field_it);
    return 0;
}

int
TypeCompare(const Type *type1,
    const Type *type2,
    const TypeCheckState *state) {
    if (type1->type != type2->type) {
        return 1;
    }
    switch (type1->type) {
        case TYPE_FUNC:
            return FuncTypeCompare(&type1->func, &type2->func, state);
        case TYPE_CLASS:
            print_ICE("TypeCompare not implemented for classes\n");
            return 1;
        case TYPE_OBJECT:
            return ObjectTypeCompare(&type1->object, &type2->object, state);
        case TYPE_EXPR:
            print_ICE("TypeCompare not implemented for expressions\n");
            return 1;
        case TYPE_TUPLE:
            print_ICE("TypeCompare not implemented for tuples\n");
            return 1;
        case TYPE_SPREAD:
            print_ICE("TypeCompare not implemented for spread tuples\n");
            return 1;
        case TYPE_NAMED:
            print_ICE("TypeCompare not implemented for named types\n");
            return 1;
        case TYPE_NONE:
            print_ICE("TypeCompare not implemented for none type\n");
            return 1;
        case TYPE_ARRAY:
            return TypeCompare(type1->array.type, type2->array.type, state);
        case TYPE_MAYBE:
            print_ICE("TypeCompare not implemented for maybe type\n");
            return 1;
    }
    return 1;
}

static int
FuncTypeVerify(struct FuncType *func,
    const TypeCheckState *state,
    char **msg) {
    size_t nargs = Vector_size(func->args);
    for (size_t i = 0; i < nargs; i++) {
        Type *argType = Vector_get(func->args, i);
        if (TypeVerify(argType, state, msg)) {
            return 1;
        }
    }
    return TypeVerify(func->ret_type, state, msg);
}

static int
ClassTypeVerify(UNUSED const struct ClassType *class,
    UNUSED const TypeCheckState *state,
    UNUSED char **msg) {
    // Class types are checked on creation
    return 0;
}

static int
ObjectTypeVerify(struct ObjectType *object,
    const TypeCheckState *state,
    char **msg) {
    char *name = object->name;
    size_t len = strlen(name);
    Type *classType = NULL;
    if (Map_get(state->symbols, name, len, &classType)) {
        if (NULL != msg) {
            *msg = safe_asprintf("unknown type \"%s\"", name);
        }
        return 1;
    }
    object->class = &classType->class;
    return 0;
}

int
TypeVerify(Type *type, const TypeCheckState *state, char **msg) {
    switch (type->type) {
        case TYPE_FUNC:
            return FuncTypeVerify(&type->func, state, msg);
        case TYPE_CLASS:
            return ClassTypeVerify(&type->class, state, msg);
        case TYPE_OBJECT:
            return ObjectTypeVerify(&type->object, state, msg);
        case TYPE_EXPR:
            *msg = safe_strdup("TypeVerify not implemented for expressions");
            return 1;
        case TYPE_TUPLE:
            *msg = safe_strdup("TypeVerify not implemented for tuples");
            return 1;
        case TYPE_SPREAD:
            *msg = safe_strdup("TypeVerify not implemented for spread tuples");
            return 1;
        case TYPE_NAMED:
            *msg = safe_strdup("TypeVerify not implemented for named types");
            return 1;
        case TYPE_NONE:
            return 0;
        case TYPE_ARRAY:
            return TypeVerify(type->array.type, state, msg);
        case TYPE_MAYBE:
            return TypeVerify(type->maybe.type, state, msg);
    }
    return 1;
}

inline Types
typeOf(const Type *type) {
    return type->type;
}

YYLTYPE
typeLoc(const Type *type) {
    return type->loc;
}

inline unsigned char
isInit(const Type *type) {
    return type->init;
}

inline void
setInit(Type *type, unsigned char init) {
    type->init = init;
}

const void *
getTypeData(Type *type) {
    switch (type->type) {
        case TYPE_FUNC:
            return &type->func;
        case TYPE_CLASS:
            return &type->class;
        case TYPE_OBJECT:
            return &type->object;
        case TYPE_EXPR:
            return &type->expr;
        case TYPE_TUPLE:
            return &type->tuple;
        case TYPE_SPREAD:
            return &type->spread;
        case TYPE_NAMED:
            return &type->named;
        case TYPE_NONE:
            print_ICE("called getTypeData() on None type\n");
            exit(EXIT_FAILURE);
        case TYPE_ARRAY:
            return &type->array;
        case TYPE_MAYBE:
            return &type->maybe;
    }
    return NULL;
}

char *
typeToString(const Type *type) {
    char *typeName, *name;
    switch (type->type) {
        case TYPE_FUNC:
            return safe_strdup("function");
        case TYPE_CLASS:
            return safe_strdup("class");
        case TYPE_OBJECT:
            return safe_asprintf("%s instance", type->object.name);
        case TYPE_EXPR:
            return safe_strdup("expression");
        case TYPE_TUPLE:
            return safe_strdup("tuple");
        case TYPE_SPREAD:
            return safe_strdup("spread");
        case TYPE_NAMED:
            return typeToString(type->named.type);
        case TYPE_NONE:
            return safe_strdup("none");
        case TYPE_ARRAY:
            typeName = typeToString(type->array.type);
            name = safe_asprintf("array of %s", typeName);
            free(typeName);
            return name;
        case TYPE_MAYBE:
            typeName = typeToString(type->array.type);
            name = safe_asprintf("maybe %s", typeName);
            free(typeName);
            return name;
    }
    return NULL;
}

Type *
new_FuncType(const YYLTYPE *loc,
    Vector *generics,
    Vector *args,
    Type *ret_type) {
    Type *t;

    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TYPE_FUNC, NULL, 0, *loc, { { 0 } }
    };
    t->func = (struct FuncType){
        generics, args, ret_type
    };
    return t;
}

Type *
new_ClassType(const YYLTYPE *loc,
    struct Vector *generics,
    struct Vector *supers,
    struct Vector *cons,
    struct Map *fields) {
    Type *t;

    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TYPE_CLASS, NULL, 0, *loc, { { 0 } }
    };
    t->class = (struct ClassType){
        generics, supers, cons, fields
    };
    return t;
}

Type *
new_ObjectType(const YYLTYPE *loc, char *name, Vector *generics) {
    Type *t;

    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TYPE_OBJECT, NULL, 0, *loc, { { 0 } }
    };
    t->object = (struct ObjectType){
        name, generics, NULL
    };
    return t;
}

Type *
new_ExprType(const YYLTYPE *loc, AST *expr, Vector *generics) {
    Type *t;

    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TYPE_EXPR, NULL, 0, *loc, { { 0 } }
    };
    t->expr = (struct ExprType){
        expr, generics, 1
    };
    return t;
}

Type *
new_TupleType(const YYLTYPE *loc, SparseVector *types) {
    Type *t;

    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TYPE_TUPLE, NULL, 0, *loc, { { 0 } }
    };
    t->tuple = (struct TupleType){
        types
    };
    return t;
}

Type *
new_SpreadType(Type *tuple) {
    Type *t;
    if (tuple->type != TYPE_TUPLE) {
        print_ICE("non-tuple typed passed to SpreadType constructor\n");
        exit(EXIT_FAILURE);
    }
    t = copy_type(tuple);
    t->type = TYPE_SPREAD;
    t->spread.types = t->tuple.types;
    return t;
}

Type *
new_NamedType(const YYLTYPE *loc, char *name, Type *type) {
    Type *t;

    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TYPE_NAMED, NULL, 0, *loc, { { 0 } }
    };
    t->named = (struct NamedType){
        name, type
    };
    return t;
}

Type *
new_NoneType(const YYLTYPE *loc) {
    Type *t;

    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TYPE_NONE, NULL, 0, *loc, { { 0 } }
    };
    return t;
}

Type *
new_ArrayType(const YYLTYPE *loc, Type *type) {
    Type *t;

    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TYPE_ARRAY, NULL, 0, *loc, { { 0 } }
    };
    t->array = (struct ArrayType){
        type
    };
    return t;
}

Type *
new_MaybeType(const YYLTYPE *loc, Type *type) {
    Type *t;

    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TYPE_MAYBE, NULL, 0, *loc, { { 0 } }
    };
    t->maybe = (struct MaybeType){
        type
    };
    return t;
}
