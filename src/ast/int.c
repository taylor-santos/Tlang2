#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"

typedef struct ASTInt ASTInt;

struct ASTInt {
    void (*json)(const ASTInt *this, FILE *out, int indent);
    void (*delete)(ASTInt *this);
    long long int val;
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

static void
delete(ASTInt *this) {
    free(this);
}

AST *
new_ASTInt(long long int val) {
    ASTInt *node = NULL;

    node = safe_malloc(sizeof(*node));
    *node = (ASTInt){
        json, delete, val
    };
    return (AST *)node;
}
