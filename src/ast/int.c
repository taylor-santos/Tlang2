#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTInt ASTInt;

struct ASTInt {
    void (*json)(const ASTInt *this, FILE *out, int indent);
    int (*getType)(const ASTInt *this, Type **typeptr);
    void (*delete)(ASTInt *this);
    struct YYLTYPE loc;
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

static int
getType(const ASTInt *this, UNUSED Type **typeptr) {
    print_code_error(&this->loc, "bool type checker not implemented", stderr);
    return 1;
}

static void
delete(ASTInt *this) {
    free(this);
}

AST *
new_ASTInt(struct YYLTYPE *loc, long long int val) {
    ASTInt *node = NULL;

    node = safe_malloc(sizeof(*node));
    *node = (ASTInt){
        json, getType, delete, *loc, val
    };
    return (AST *)node;
}
