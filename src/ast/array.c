#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "parser.h"
#include "map.h"

typedef struct ASTArray ASTArray;

struct Vector;

struct ASTArray {
    void (*json)(const ASTArray *this, FILE *out, int indent);
    int (*getType)(ASTArray *this,
        UNUSED TypeCheckState *state,
        Type **typeptr);
    void (*delete)(ASTArray *this);
    struct YYLTYPE loc;
    Type *array_type;
    long long int index;
    Type *type; // NULL until type checker is executed.
};

static void
json(const ASTArray *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("array", out, indent);
    json_comma(out, indent);
    json_label("type", out);
    json_type(this->array_type, out, indent);
    json_comma(out, indent);
    json_label("index", out);
    json_int(this->index, out, indent);
    json_end(out, &indent);
}

static int
getType(ASTArray *this, TypeCheckState *state, Type **typeptr) {
    char *msg;
    if (TypeVerify(this->array_type, state, &msg)) {
        print_code_error(stderr, typeLoc(this->array_type), msg);
        return 1;
    }
    Type *type_copy = copy_type(this->array_type);
    *typeptr = this->type = ArrayType(&this->loc, type_copy);
    return 0;
}

static void
delete(ASTArray *this) {
    delete_type(this->array_type);
    if (NULL != this->type) {
        delete_type(this->type);
    }
    free(this);
}

AST *
new_ASTArray(struct YYLTYPE *loc, Type *array_type, long long int index) {
    ASTArray *array = NULL;

    array = safe_malloc(sizeof(*array));
    *array = (ASTArray){
        json, getType, delete, *loc, array_type, index, NULL
    };
    return (AST *)array;
}
