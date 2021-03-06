#include "types.h"
#include "json.h"
#include "safe.h"
#include "vector.h"
#include "map.h"

static void
json(const void *type, FILE *out, int indent) {
    const struct ObjectType *this = type;
    json_start(out, &indent);
    json_label("type", out);
    json_string("object", out, indent);
    if (0 != this->super.qualifiers) {
        json_comma(out, indent);
        json_label("qualifiers", out);
        json_qualifier(this->super.qualifiers, out, indent);
    }
    json_comma(out, indent);
    json_label("name", out);
    json_string(this->name, out, indent);
    json_comma(out, indent);
    json_label("generics", out);
    json_vector(this->generics, (JSON_VALUE_FUNC)json_string, out, indent);
    json_end(out, &indent);
}

static int
compare(const void *type, const void *otherType, const TypeCheckState *state) {
    const Type *other = otherType;
    if (TYPE_OBJECT != other->type) {
        return 1;
    }
    const struct ObjectType *object1 = type, *object2 = otherType;
    if (object1->class == NULL || object2->class == NULL) {
        print_ICE("Unverified object type\n");
        exit(EXIT_FAILURE);
    }
    if (object1->class->fieldTypes == object2->class->fieldTypes) {
        return 0;
    }
    const struct ClassType *class1 = object1->class, *class2 = object2->class;
    return class1->super.compare(class1, class2, state);
}

static int
verify(void *type, const TypeCheckState *state, char **msg) {
    struct ObjectType *object = type;
    char *name = object->name;
    size_t len = strlen(name);
    struct ClassType *classType = NULL;
    if (Map_get(state->symbols, name, len, &classType)) {
        if (NULL != msg) {
            *msg = safe_asprintf("unknown type \"%s\"", name);
        }
        return 1;
    }
    if (TYPE_CLASS != classType->super.type) {
        if (NULL != msg) {
            *msg = safe_asprintf("\"%s\" is not a class", name);
        }
        return 1;
    }
    object->class = classType;
    return 0;
}

static char *
toString(const void *type) {
    const struct ObjectType *this = type;
    char *typeName, *name;
    if (this->name != NULL) {
        return safe_strdup(this->name);
    }
    typeName = this->class->super.toString(this->class);
    name = safe_asprintf("%s instance", typeName);
    free(typeName);
    return name;
}

static char *
codeGen(const void *this, const char *name) {
    const struct ObjectType *type = this;
    if (type->super.isRef) {
        if (NULL != type->class->name) {
            if (NULL != name) {
                return safe_asprintf("class_%s *%s", type->class->name, name);
            } else {
                return safe_asprintf("class_%s *", type->class->name);
            }
        } else {
            return safe_strdup("/* CLASS NAME ID NOT IMPLEMENTED */");
        }
    } else {
        if (NULL != type->class->name) {
            if (NULL != name) {
                return safe_asprintf("class_%s %s", type->class->name, name);
            } else {
                return safe_asprintf("class_%s", type->class->name);
            }
        } else {
            return safe_strdup("/* CLASS NAME ID NOT IMPLEMENTED */");
        }
    }
}

static void
delete(void *type) {
    struct ObjectType *this = type;
    if (!this->super.isCopy) {
        free(this->name);
        delete_Vector(this->generics, free);
    }
    free(this);
}

static Type *
copy(const void *type) {
    const struct ObjectType *this = type;
    struct ObjectType *type_copy = safe_malloc(sizeof(*type_copy));
    *type_copy = (struct ObjectType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            codeGen,
            delete,
            TYPE_OBJECT,
            this->super.qualifiers,
            this->super.init,
            1,
            this->super.isRef,
            this->super.loc
        },
        this->name,
        this->generics,
        this->class
    };
    return (Type *)type_copy;
}

Type *
new_ObjectType(YYLTYPE loc, char *name, struct Vector *generics) {
    struct ObjectType *type;

    type = safe_malloc(sizeof(*type));
    *type = (struct ObjectType){
        {
            json,
            copy,
            compare,
            verify,
            toString,
            codeGen,
            delete,
            TYPE_OBJECT,
            0,
            0,
            0,
            0,
            loc
        },
        name,
        generics,
        NULL
    };
    return (Type *)type;
}
