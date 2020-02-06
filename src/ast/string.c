#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "dynamic_string.h"
#include "parser.h"

typedef struct ASTString ASTString;

struct ASTString {
    void (*json)(const ASTString *this, FILE *out, int indent);
    int (*getType)(const ASTString *this, Type **typeptr);
    void (*delete)(ASTString *this);
    struct YYLTYPE loc;
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

static int
getType(const ASTString *this, UNUSED Type **typeptr) {
    print_code_error(&this->loc,
        "string type checker not implemented",
        stderr);
    return 1;
}

static void
delete(ASTString *this) {
    delete_dstring(this->str);
    free(this);
}

AST *
new_ASTString(struct YYLTYPE *loc, dstring *str) {
    ASTString *node = NULL;

    node = safe_malloc(sizeof(*node));
    *node = (ASTString){
        json, getType, delete, *loc, str
    };
    return (AST *)node;
}
