#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "dynamic_string.h"

typedef struct ASTString ASTString;

struct ASTString {
    void (*json)(const ASTString *this, FILE *out, int indent);
    void (*delete)(ASTString *this);
    dstring *str;
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

static void
delete(ASTString *this) {
    delete_dstring(this->str);
    free(this);
}

AST *
new_ASTString(dstring *str) {
    ASTString *node = NULL;

    node = safe_malloc(sizeof(*node));
    *node = (ASTString){
        json, delete, str
    };
    return (AST *)node;
}
