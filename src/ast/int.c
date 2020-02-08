#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTInt ASTInt;

struct ASTInt {
    void (*json)(const ASTInt *this, FILE *out, int indent);
    int (*getType)(ASTInt *this, UNUSED TypeCheckState *state, Type **typeptr);
    void (*delete)(ASTInt *this);
    struct YYLTYPE loc;
    long long int val;
    Type *type;
};

static void
json(const ASTInt *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("int", out, indent);
    json_comma(out, indent);
    json_label("val", out);
    json_int(this->val, out, indent);
    json_end(out, &indent);
}

static int
getType(ASTInt *this, UNUSED TypeCheckState *state, Type **typeptr) {
    *typeptr = this->type;
    return 0;
}

static void
delete(ASTInt *this) {
    delete_type(this->type);
    free(this);
}

AST *
new_ASTInt(struct YYLTYPE *loc, long long int val) {
    ASTInt *node = NULL;
    Type *type;

    type = new_ObjectType(loc, safe_strdup("int"), Vector());
    node = safe_malloc(sizeof(*node));
    *node = (ASTInt){
        json, getType, delete, *loc, val, type
    };
    return (AST *)node;
}
