#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTDouble ASTDouble;

struct ASTDouble {
    void (*json)(const ASTDouble *this, FILE *out, int indent);
    int (*getType)(ASTDouble *this,
        UNUSED TypeCheckState *state,
        Type **typeptr);
    void (*delete)(ASTDouble *this);
    struct YYLTYPE loc;
    double val;
    Type *type;
};

static void
json(const ASTDouble *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("double", out, indent);
    json_comma(out, indent);
    json_label("val", out);
    json_double(this->val, out, indent);
    json_end(out, &indent);
}

static int
getType(ASTDouble *this, UNUSED TypeCheckState *state, UNUSED Type **typeptr) {
    *typeptr = this->type;
    return 0;
}

static void
delete(ASTDouble *this) {
    delete_type(this->type);
    free(this);
}

AST *
new_ASTDouble(struct YYLTYPE *loc, double val) {
    ASTDouble *node = NULL;
    Type *type;

    type = new_ObjectType(loc, safe_strdup("double"), Vector());
    node = safe_malloc(sizeof(*node));
    *node = (ASTDouble){
        json, getType, delete, *loc, val, type
    };
    return (AST *)node;
}
