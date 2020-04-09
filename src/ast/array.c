#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTArray ASTArray;

struct Vector;

struct ASTArray {
    AST super;
    Type *array_type;
    long long int index;
    Type *type; // NULL until type checker is executed.
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTArray *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("array", out, indent);
    json_comma(out, indent);
    json_label("type", out);
    json_type(ast->array_type, out, indent);
    json_comma(out, indent);
    json_label("index", out);
    json_int(ast->index, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, TypeCheckState *state, Type **typeptr) {
    ASTArray *ast = this;
    char *msg;
    if (ast->array_type->verify(ast->array_type, state, &msg)) {
        print_code_error(stderr, ast->array_type->loc, msg);
        return 1;
    }
    Type *type_copy = ast->array_type->copy(ast->array_type);
    *typeptr = ast->type = ArrayType(ast->super.loc, type_copy);
    return 0;
}

static char *
codeGen(UNUSED void *this, UNUSED TypeCheckState *state) {
    return safe_strdup("/* NOT IMPLEMENTED */");
}

static void
delete(void *this) {
    ASTArray *ast = this;
    delete_type(ast->array_type);
    if (NULL != ast->type) {
        delete_type(ast->type);
    }
    free(this);
}

AST *
new_ASTArray(struct YYLTYPE loc, Type *array_type, long long int index) {
    ASTArray *array = NULL;

    array = safe_malloc(sizeof(*array));
    *array = (ASTArray){
        { json, getType, codeGen, delete, loc }, array_type, index, NULL
    };
    return (AST *)array;
}
