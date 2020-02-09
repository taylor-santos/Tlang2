#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "parser.h"
#include "map.h"

typedef struct ASTInit ASTInit;

struct Vector;

struct ASTInit {
    void (*json)(const ASTInit *this, FILE *out, int indent);
    int (*getType)(ASTInit *this,
        UNUSED TypeCheckState *state,
        Type **typeptr);
    void (*delete)(ASTInit *this);
    struct YYLTYPE loc;
    char *name;
    Vector *generics; // Vector<char*>
    Vector *args;     // Vector<AST*>
    Vector *argTypes; // NULL until type checker is executed.
    Type *type;       // NULL until type checker is executed.
};

static void
json(const ASTInit *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("init", out, indent);
    json_comma(out, indent);
    json_label("name", out);
    json_string(this->name, out, indent);
    json_comma(out, indent);
    json_label("generics", out);
    json_vector(this->generics, (JSON_MAP_TYPE)json_string, out, indent);
    json_comma(out, indent);
    json_label("args", out);
    json_vector(this->args, (JSON_MAP_TYPE)json_AST, out, indent);
    json_end(out, &indent);
}

static int
getType(ASTInit *this, TypeCheckState *state, Type **typeptr) {
    // ClassTypeVerify() assumes this fully checks the validity of the class.
    Type *classType = NULL;
    size_t len = strlen(this->name), ngen;

    if (Map_get(state->symbols, this->name, len, &classType)) {
        print_code_error(stderr,
            this->loc,
            "unrecognized type name \"%s\"",
            this->name);
        return 1;
    }
    if (typeOf(classType) != TYPE_CLASS) {
        char *typeName = typeToString(classType);
        print_code_error(stderr,
            this->loc,
            "\"%s\" has non-class type \"%s\"",
            this->name,
            typeName);
        free(typeName);
        return 1;
    }
    ngen = Vector_size(this->generics);
    if (ngen != 0) {
        print_code_error(stderr,
            this->loc,
            "init with generics not implemented");
        return 1;
    }
    size_t ngiven = Vector_size(this->args);
    this->argTypes = Vector();
    for (size_t i = 0; i < ngiven; i++) {
        AST *arg = Vector_get(this->args, i);
        Type *argType = NULL;
        if (getType_AST(arg, state, &argType)) {
            return 1;
        }
        Type *type_copy = copy_type(argType);
        Vector_append(this->argTypes, type_copy);
    }

    const struct ClassType *class = getTypeData(classType);
    size_t ncons = Vector_size(class->constructors);
    if (ncons == 0 && ngiven == 0) {
        // Implicit default constructor
        *typeptr = this->type =
            ObjectType(&this->loc, safe_strdup(this->name), Vector());
        char *msg;
        if (TypeVerify(this->type, state, &msg)) {
            print_code_error(stderr, typeLoc(this->type), msg);
            free(msg);
            return 1;
        }
        return 0;
    }
    for (size_t i = 0; i < ncons; i++) {
        Vector *con = Vector_get(class->constructors, i);
        size_t nargs = Vector_size(con);
        if (nargs == ngiven) {
            int valid = 1;
            for (size_t j = 0; j < nargs; j++) {
                Type *givenType = Vector_get(this->argTypes, j);
                Type *argType = Vector_get(con, j);
                if (TypeCompare(givenType, argType, state)) {
                    valid = 0;
                    break;
                }
            }
            if (valid) {
                *typeptr = this->type =
                    ObjectType(&this->loc, safe_strdup(this->name), Vector());
                return 0;
            }
        }
        if (nargs > ngiven) {
            break;
        }
    }
    print_code_error(stderr,
        this->loc,
        "invalid arguments given for \"%s\" constructor",
        this->name);
    return 1;
}

static void
delete(ASTInit *this) {
    free(this->name);
    delete_Vector(this->generics, free);
    delete_Vector(this->args, (VEC_DELETE_FUNC)delete_AST);
    if (NULL != this->argTypes) {
        delete_Vector(this->argTypes, (VEC_DELETE_FUNC)delete_type);
    }
    if (NULL != this->type) {
        delete_type(this->type);
    }
    free(this);
}

AST *
new_ASTInit(struct YYLTYPE *loc, char *name, Vector *generics, Vector *args) {
    ASTInit *init = NULL;

    init = safe_malloc(sizeof(*init));
    *init = (ASTInit){
        json, getType, delete, *loc, name, generics, args, NULL, NULL
    };
    return (AST *)init;
}
