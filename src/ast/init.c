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
    AST super;
    char *name;
    Vector *generics; // Vector<char*>
    Vector *args;     // Vector<AST*>
    Vector *argTypes; // NULL until type checker is executed.
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTInit *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("init", out, indent);
    json_comma(out, indent);
    json_label("name", out);
    json_string(ast->name, out, indent);
    json_comma(out, indent);
    json_label("generics", out);
    json_vector(ast->generics, (JSON_VALUE_FUNC)json_string, out, indent);
    json_comma(out, indent);
    json_label("args", out);
    json_vector(ast->args, (JSON_VALUE_FUNC)json_AST, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, TypeCheckState *state, Type **typeptr) {
    // ClassTypeVerify() assumes this fully checks the validity of the class.
    ASTInit *ast = this;
    Type *classType = NULL;
    size_t ngen, len = strlen(ast->name);

    if (Map_get(state->symbols, ast->name, len, &classType)) {
        print_code_error(stderr,
            ast->super.loc,
            "unrecognized type name \"%s\"",
            ast->name);
        return 1;
    }
    if (TYPE_CLASS != classType->type) {
        char *typeName = classType->toString(classType);
        print_code_error(stderr,
            ast->super.loc,
            "\"%s\" has non-class type \"%s\"",
            ast->name,
            typeName);
        free(typeName);
        return 1;
    }
    ngen = Vector_size(ast->generics);
    if (ngen != 0) {
        // TODO: generic classes
        print_code_error(stderr,
            ast->super.loc,
            "%s",
            "init with generics not implemented");
        return 1;
    }
    size_t ngiven = Vector_size(ast->args);
    ast->argTypes = Vector();
    for (size_t i = 0; i < ngiven; i++) {
        AST *arg = Vector_get(ast->args, i);
        Type *argType = NULL;
        if (arg->getType(arg, state, &argType)) {
            return 1;
        }
        Type *type_copy = copy_type(argType);
        Vector_append(ast->argTypes, type_copy);
    }

    const struct ClassType *class = (const struct ClassType *)classType;
    size_t nctors = Vector_size(class->ctors);
    if (nctors == 0 && ngiven == 0) {
        // Implicit default constructor
        *typeptr = ast->super.type =
            ObjectType(ast->super.loc, safe_strdup(ast->name), Vector());
        char *msg;
        if (ast->super.type->verify(ast->super.type, state, &msg)) {
            print_code_error(stderr, ast->super.type->loc, "%s", msg);
            free(msg);
            return 1;
        }
        ast->super.type->init = 1;
        return 0;
    }
    for (size_t i = 0; i < nctors; i++) {
        Vector *ctor = Vector_get(class->ctors, i);
        size_t nargs = Vector_size(ctor);
        if (nargs == ngiven) {
            int valid = 1;
            for (size_t j = 0; j < nargs; j++) {
                Type *givenType = Vector_get(ast->argTypes, j);
                Type *argType = Vector_get(ctor, j);
                if (givenType->compare(givenType, argType, state)) {
                    valid = 0;
                    break;
                }
            }
            if (valid) {
                *typeptr = ast->super.type = ObjectType(ast->super.loc,
                    safe_strdup(ast->name),
                    Vector());
                char *msg;
                if (ast->super.type->verify(ast->super.type, state, &msg)) {
                    print_code_error(stderr, ast->super.type->loc, "%s", msg);
                    free(msg);
                    return 1;
                }
                ast->super.type->init = 1;
                return 0;
            }
        }
        if (nargs > ngiven) {
            break;
        }
    }
    print_code_error(stderr,
        ast->super.loc,
        "invalid arguments given for \"%s\" constructor",
        ast->name);
    return 1;
}

static char *
codeGen(void *this, UNUSED FILE *out, UNUSED CodeGenState *state) {
    ASTInit *ast = this;
    return safe_asprintf("CALL(var_%s, 0)", ast->name);
}

static void
delete(void *this) {
    ASTInit *ast = this;
    free(ast->name);
    delete_Vector(ast->generics, free);
    delete_Vector(ast->args, (VEC_DELETE_FUNC)delete_AST);
    if (NULL != ast->argTypes) {
        delete_Vector(ast->argTypes, (VEC_DELETE_FUNC)delete_type);
    }
    if (NULL != ast->super.type) {
        delete_type(ast->super.type);
    }
    free(this);
}

AST *
new_ASTInit(YYLTYPE loc, char *name, Vector *generics, Vector *args) {
    ASTInit *init = NULL;

    init = safe_malloc(sizeof(*init));
    *init = (ASTInit){
        {
            json,
            getType,
            codeGen,
            delete,
            loc,
            NULL
        },
        name,
        generics,
        args,
        NULL
    };
    return (AST *)init;
}
