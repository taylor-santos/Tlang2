#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "parser.h"

typedef struct ASTConstIndex ASTConstIndex;

struct ASTConstIndex {
    void (*json)(const ASTConstIndex *this, FILE *out, int indent);
    int (*getType)(ASTConstIndex *this,
        UNUSED TypeCheckState *state,
        Type **typeptr);
    void (*delete)(ASTConstIndex *this);
    struct YYLTYPE loc;
    AST *expr;
    long long int index;
    Type *type; // NULL until type checker is executed.
};

static void
json(const ASTConstIndex *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("const index", out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(this->expr, out, indent);
    json_comma(out, indent);
    json_label("index", out);
    json_int(this->index, out, indent);
    json_end(out, &indent);
}

static int
getType(ASTConstIndex *this, TypeCheckState *state, Type **typeptr) {
    Type *type = NULL;
    if (getType_AST(this->expr, state, &type)) {
        return 1;
    }
    if (TYPE_ARRAY != typeOf(type)) {
        char *typeName = typeToString(type);
        print_code_error(stderr,
            this->loc,
            "index operator used on non-array object with type \"%s\"",
            typeName);
        free(typeName);
        return 1;
    }
    const struct ArrayType *array = getTypeData(type);
    *typeptr = this->type = MaybeType(&this->loc, copy_type(array->type));
    return 0;
}

static void
delete(ASTConstIndex *this) {
    delete_AST(this->expr);
    if (NULL != this->type) {
        delete_type(this->type);
    }
    free(this);
}

AST *
new_ASTConstIndex(struct YYLTYPE *loc, AST *expr, long long int index) {
    ASTConstIndex *node = NULL;

    node = safe_malloc(sizeof(*node));
    *node = (ASTConstIndex){
        json, getType, delete, *loc, expr, index, NULL
    };
    return (AST *)node;
}
