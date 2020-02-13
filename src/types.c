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
    unsigned char copy : 1;
    YYLTYPE loc;
    union {
        struct FuncType *func;
        struct ClassType *class;
        struct ObjectType *object;
        struct ExprType *expr;
        struct TupleType *tuple;
        struct SpreadType *spread;
        struct NamedType *named;
        struct ArrayType *array;
        struct MaybeType *maybe;
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
            json_vector(type->func->generics,
                (JSON_MAP_TYPE)json_string,
                out,
                indent);
            json_comma(out, indent);
            json_label("args", out);
            json_vector(type->func->args,
                (JSON_MAP_TYPE)json_type,
                out,
                indent);
            json_comma(out, indent);
            json_label("return type", out);
            json_type(type->func->ret_type, out, indent);
            break;
        case TYPE_CLASS:
            json_comma(out, indent);
            json_label("generics", out);
            json_vector(type->class->generics,
                (JSON_MAP_TYPE)json_string,
                out,
                indent);
            json_comma(out, indent);
            json_label("supers", out);
            json_vector(type->class->supers,
                (JSON_MAP_TYPE)json_string,
                out,
                indent);
            json_comma(out, indent);
            json_label("constructors", out);
            json_vector(type->class->constructors,
                (JSON_MAP_TYPE)json_constructor,
                out,
                indent);
            json_comma(out, indent);
            json_label("fields", out);
            json_Map(type->class->fields,
                (JSON_MAP_TYPE)json_type,
                out,
                indent);
            break;
        case TYPE_OBJECT:
            json_comma(out, indent);
            json_label("name", out);
            json_string(type->object->name, out, indent);
            json_comma(out, indent);
            json_label("generics", out);
            json_vector(type->object->generics,
                (JSON_MAP_TYPE)json_string,
                out,
                indent);
            break;
        case TYPE_EXPR:
            json_comma(out, indent);
            json_label("expr", out);
            json_AST(type->expr->expr, out, indent);
            json_comma(out, indent);
            json_label("generics", out);
            json_vector(type->expr->generics,
                (JSON_MAP_TYPE)json_string,
                out,
                indent);
            break;
        case TYPE_TUPLE:
            json_comma(out, indent);
            json_label("types", out);
            json_sparse_vector(type->tuple->types,
                (JSON_MAP_TYPE)json_type,
                out,
                indent);
            break;
        case TYPE_SPREAD:
            json_comma(out, indent);
            json_label("types", out);
            json_sparse_vector(type->spread->types,
                (JSON_MAP_TYPE)json_type,
                out,
                indent);
            break;
        case TYPE_NAMED:
            json_comma(out, indent);
            json_label("name", out);
            json_string(type->named->name, out, indent);
            json_comma(out, indent);
            json_label("type", out);
            json_type(type->named->type, out, indent);
            break;
        case TYPE_NONE:
            break;
        case TYPE_ARRAY:
            json_comma(out, indent);
            json_label("type", out);
            json_type(type->array->type, out, indent);
            break;
        case TYPE_MAYBE:
            json_comma(out, indent);
            json_label("type", out);
            json_type(type->maybe->type, out, indent);
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
    *new_type = *type;
    new_type->qualifiers = qualifiers;
    new_type->copy = 1;
    return new_type;
}

static void
delete_cons(Vector *cons) {
    delete_Vector(cons, (VEC_DELETE_FUNC)delete_type);
}

void
delete_type(Type *type) {
    if (0 == type->copy) {
        switch (type->type) {
            case TYPE_FUNC:
                delete_Vector(type->func->generics, free);
                delete_Vector(type->func->args, (VEC_DELETE_FUNC)delete_type);
                delete_type(type->func->ret_type);
                free(type->func);
                break;
            case TYPE_CLASS:
                // Delete class definitions from program's "classes" list
                break;
            case TYPE_OBJECT:
                free(type->object->name);
                delete_Vector(type->object->generics, free);
                free(type->object);
                break;
            case TYPE_EXPR:
                if (type->expr->ownsAST) {
                    delete_AST(type->expr->expr);
                }
                delete_Vector(type->expr->generics, free);
                free(type->expr);
                break;
            case TYPE_TUPLE:
                delete_SparseVector(type->tuple->types,
                    (VEC_DELETE_FUNC)delete_type);
                free(type->tuple);
                break;
            case TYPE_SPREAD:
                delete_SparseVector(type->spread->types,
                    (VEC_DELETE_FUNC)delete_type);
                free(type->spread);
                break;
            case TYPE_NAMED:
                free(type->named->name);
                delete_type(type->named->type);
                free(type->named);
                break;
            case TYPE_NONE:
                break;
            case TYPE_ARRAY:
                delete_type(type->array->type);
                free(type->array);
                break;
            case TYPE_MAYBE:
                delete_type(type->maybe->type);
                free(type->maybe);
                break;
        }
    }
    if (NULL != type->qualifiers) {
        delete_Vector(type->qualifiers, free);
    }
    free(type);
}

void
delete_ClassType(struct ClassType *class) {
    delete_Vector(class->generics, free);
    delete_Vector(class->supers, free);
    delete_Vector(class->constructors, (VEC_DELETE_FUNC)delete_cons);
    delete_Map(class->fields, (MAP_DELETE_FUNC)delete_type);
    free(class);
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

    if (type1->class == NULL || type2->class == NULL) {
        print_ICE("Unverified object type\n");
        exit(EXIT_FAILURE);
    }
    if (type1->class == type2->class) {
        return 0;
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
            return FuncTypeCompare(type1->func, type2->func, state);
        case TYPE_CLASS:
            print_ICE("TypeCompare not implemented for classes\n");
            return 1;
        case TYPE_OBJECT:
            return ObjectTypeCompare(type1->object, type2->object, state);
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
            return TypeCompare(type1->array->type, type2->array->type, state);
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
    object->class = classType->class;
    return 0;
}

int
TypeVerify(Type *type, const TypeCheckState *state, char **msg) {
    switch (type->type) {
        case TYPE_FUNC:
            return FuncTypeVerify(type->func, state, msg);
        case TYPE_CLASS:
            return ClassTypeVerify(type->class, state, msg);
        case TYPE_OBJECT:
            return ObjectTypeVerify(type->object, state, msg);
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
            return TypeVerify(type->array->type, state, msg);
        case TYPE_MAYBE:
            return TypeVerify(type->maybe->type, state, msg);
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
            return type->func;
        case TYPE_CLASS:
            return type->class;
        case TYPE_OBJECT:
            return type->object;
        case TYPE_EXPR:
            return type->expr;
        case TYPE_TUPLE:
            return type->tuple;
        case TYPE_SPREAD:
            return type->spread;
        case TYPE_NAMED:
            return type->named;
        case TYPE_NONE:
            print_ICE("called getTypeData() on None type\n");
            exit(EXIT_FAILURE);
        case TYPE_ARRAY:
            return type->array;
        case TYPE_MAYBE:
            return type->maybe;
    }
    return NULL;
}

char *
ClassTypeToString(const struct ClassType *class) {
    dstring str = dstring("class{");
    Iterator *it = Map_iterator(class->fields);
    char *sep = "";
    while (it->hasNext(it)) {
        MapIterData field = it->next(it);
        char *fieldType = typeToString(field.value);
        append_vstr(&str, "%s%.*s:%s", sep, field.len, field.key, fieldType);
        free(fieldType);
        sep = ",";
    }
    it->delete(it);
    append_str(&str, "}");
    return str.str;
}

char *
typeToString(const Type *type) {
    char *typeName, *name;
    switch (type->type) {
        case TYPE_FUNC:
            return safe_strdup("function");
        case TYPE_CLASS:
            return ClassTypeToString(type->class);
        case TYPE_OBJECT:
            if (type->object->name != NULL) {
                return safe_asprintf("%s instance", type->object->name);
            }
            typeName = ClassTypeToString(type->object->class);
            name = safe_asprintf("%s instance", typeName);
            free(typeName);
            return name;
        case TYPE_EXPR:
            return safe_strdup("expression");
        case TYPE_TUPLE:
            return safe_strdup("tuple");
        case TYPE_SPREAD:
            return safe_strdup("spread");
        case TYPE_NAMED:
            return typeToString(type->named->type);
        case TYPE_NONE:
            return safe_strdup("none");
        case TYPE_ARRAY:
            typeName = typeToString(type->array->type);
            name = safe_asprintf("array of %s", typeName);
            free(typeName);
            return name;
        case TYPE_MAYBE:
            typeName = typeToString(type->array->type);
            name = safe_asprintf("maybe %s", typeName);
            free(typeName);
            return name;
    }
    return NULL;
}

static int
ObjectTypeIntersection(const struct ObjectType *type1,
    const struct ObjectType *type2,
    const TypeCheckState *state,
    YYLTYPE loc,
    Type **typeptr) {
    size_t ngens1 = Vector_size(type1->generics),
        ngens2 = Vector_size(type2->generics);
    if (0 != ngens1 || 0 != ngens2) {
        print_ICE("TypeIntersection not implemented for generic objects\n");
        return 1;
    }
    Type *objectType = ObjectType(loc, NULL, Vector());
    setInit(objectType, 1);
    struct ClassType *class = malloc(sizeof(*class));
    Map *fields = Map();
    Iterator *it = Map_iterator(type1->class->fields);
    while (it->hasNext(it)) {
        MapIterData field = it->next(it);
        Type *field1 = field.value, *field2 = NULL;
        if (!Map_get(type2->class->fields, field.key, field.len, &field2)) {
            Type *newField;
            if (!TypeIntersection(field1, field2, state, loc, &newField)) {
                Map_put(fields, field.key, field.len, newField, NULL);
            }
        }
    }
    it->delete(it);
    *class = (struct ClassType){
        Vector(), Vector(), Vector(), fields
    };
    objectType->object->class = class;
    Vector_append(state->classes, class);
    *typeptr = objectType;
    return 0;
}

int
TypeIntersection(const Type *type1,
    const Type *type2,
    const TypeCheckState *state,
    YYLTYPE loc,
    Type **typeptr) {
    if (type1->type != type2->type) {
        return 1;
    }
    if (0 == TypeCompare(type1, type2, state)) {
        // type1 IS-A type2, so type2 is the intersection of the two.
        *typeptr = copy_type(type2);
        return 0;
    }
    if (0 == TypeCompare(type2, type1, state)) {
        // type2 IS-A type1, so type1 is the intersection of the two.
        *typeptr = copy_type(type1);
        return 0;
    }
    // Neither type is a subtype of the other, build a new type to represent
    // the intersection.
    switch (type1->type) {
        case TYPE_FUNC:
            print_ICE("TypeIntersection not implemented for function type\n");
            return 1;
        case TYPE_CLASS:
            print_ICE("TypeIntersection not implemented for classes\n");
            return 1;
        case TYPE_OBJECT:
            return ObjectTypeIntersection(type1->object,
                type2->object,
                state,
                loc,
                typeptr);
        case TYPE_EXPR:
            print_ICE("TypeIntersection not implemented for expressions\n");
            return 1;
        case TYPE_TUPLE:
            print_ICE("TypeIntersection not implemented for tuples\n");
            return 1;
        case TYPE_SPREAD:
            print_ICE("TypeIntersection not implemented for spread tuples\n");
            return 1;
        case TYPE_NAMED:
            print_ICE("TypeIntersection not implemented for named types\n");
            return 1;
        case TYPE_NONE:
            print_ICE("TypeIntersection not implemented for none type\n");
            return 1;
        case TYPE_ARRAY:
            print_ICE("TypeIntersection not implemented for array type\n");
            return 1;
        case TYPE_MAYBE:
            print_ICE("TypeIntersection not implemented for maybe type\n");
            return 1;
    }
    return 1;
}

Type *
new_FuncType(YYLTYPE loc, Vector *generics, Vector *args, Type *ret_type) {
    Type *t;
    struct FuncType *func;

    func = safe_malloc(sizeof(*func));
    *func = (struct FuncType){
        generics, args, ret_type
    };
    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TYPE_FUNC, NULL, 0, 0, loc, .func=func
    };
    return t;
}

Type *
new_ClassType(YYLTYPE loc,
    struct Vector *generics,
    struct Vector *supers,
    struct Vector *cons,
    struct Map *fields) {
    Type *t;
    struct ClassType *class;

    class = safe_malloc(sizeof(*class));
    *class = (struct ClassType){
        generics, supers, cons, fields
    };
    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TYPE_CLASS, NULL, 0, 0, loc, .class=class
    };
    return t;
}

Type *
new_ObjectType(YYLTYPE loc, char *name, Vector *generics) {
    Type *t;
    struct ObjectType *object;

    object = safe_malloc(sizeof(*object));
    *object = (struct ObjectType){
        name, generics, NULL
    };
    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TYPE_OBJECT, NULL, 0, 0, loc, .object=object
    };
    return t;
}

Type *
new_ExprType(YYLTYPE loc, AST *expr, Vector *generics) {
    Type *t;
    struct ExprType *exprType;

    exprType = safe_malloc(sizeof(*exprType));
    *exprType = (struct ExprType){
        expr, generics, 1
    };
    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TYPE_EXPR, NULL, 0, 0, loc, .expr=exprType
    };
    return t;
}

Type *
new_TupleType(YYLTYPE loc, SparseVector *types) {
    Type *t;
    struct TupleType *tuple;

    tuple = safe_malloc(sizeof(*tuple));
    *tuple = (struct TupleType){
        types
    };
    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TYPE_TUPLE, NULL, 0, 0, loc, .tuple=tuple
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
    t->spread->types = t->tuple->types;
    return t;
}

Type *
new_NamedType(YYLTYPE loc, char *name, Type *type) {
    Type *t;
    struct NamedType *named;

    named = safe_malloc(sizeof(*named));
    *named = (struct NamedType){
        name, type
    };
    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TYPE_NAMED, NULL, 0, 0, loc, .named=named
    };
    return t;
}

Type *
new_NoneType(YYLTYPE loc) {
    Type *t;

    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TYPE_NONE, NULL, 0, 0, loc, { NULL }
    };
    return t;
}

Type *
new_ArrayType(YYLTYPE loc, Type *type) {
    Type *t;
    struct ArrayType *array;

    array = safe_malloc(sizeof(*array));
    *array = (struct ArrayType){
        type
    };
    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TYPE_ARRAY, NULL, 0, 0, loc, .array=array
    };
    return t;
}

Type *
new_MaybeType(YYLTYPE loc, Type *type) {
    Type *t;
    struct MaybeType *maybe;

    maybe = safe_malloc(sizeof(*maybe));
    *maybe = (struct MaybeType){
        type
    };
    t = safe_malloc(sizeof(*t));
    *t = (Type){
        TYPE_MAYBE, NULL, 0, 0, loc, .maybe=maybe
    };
    return t;
}
