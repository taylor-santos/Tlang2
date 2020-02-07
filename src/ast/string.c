#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "dynamic_string.h"
#include "parser.h"

typedef struct ASTString ASTString;

struct ASTString {
    void (*json)(const ASTString *this, FILE *out, int indent);
    int (*getType)(ASTString *this,
        UNUSED TypeCheckState *state,
        Type **typeptr);
    void (*delete)(ASTString *this);
    struct YYLTYPE loc;
    dstring *str;
    Type *type;
};

static void
json(const ASTString *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("string", out, indent);
    json_comma(out, indent);
    json_label("val", out);
    json_dstring(this->str, out, indent);
    json_end(out, &indent);
}

static int
getType(ASTString *this, UNUSED TypeCheckState *state, Type **typeptr) {
    *typeptr = this->type;
    return 0;
}

static void
delete(ASTString *this) {
    delete_dstring(this->str);
    delete_type(this->type);
    free(this);
}

AST *
new_ASTString(struct YYLTYPE *loc, dstring *str) {
    ASTString *node = NULL;
    Type *type;

    type = ObjectType(safe_strdup("string"), Vector());
    node = safe_malloc(sizeof(*node));
    *node = (ASTString){
        json, getType, delete, *loc, str, type
    };
    return (AST *)node;
}
