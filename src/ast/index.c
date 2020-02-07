#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "parser.h"

typedef struct ASTIndex ASTIndex;

struct ASTIndex {
    void (*json)(const ASTIndex *this, FILE *out, int indent);
    int (*getType)(ASTIndex *this,
        UNUSED TypeCheckState *state,
        Type **typeptr);
    void (*delete)(ASTIndex *this);
    struct YYLTYPE loc;
    AST *expr;
    AST *index;
};

static void
json(const ASTIndex *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("index", out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(this->expr, out, indent);
    json_comma(out, indent);
    json_label("index", out);
    json_AST(this->index, out, indent);
    json_end(out, &indent);
}

static int
getType(ASTIndex *this, UNUSED TypeCheckState *state, UNUSED Type **typeptr) {
    print_code_error(stderr, this->loc, "index type checker not implemented");
    return 1;
}

static void
delete(ASTIndex *this) {
    delete_AST(this->expr);
    delete_AST(this->index);
    free(this);
}

AST *
new_ASTIndex(struct YYLTYPE *loc, AST *expr, AST *index) {
    ASTIndex *node = NULL;

    node = safe_malloc(sizeof(*node));
    *node = (ASTIndex){
        json, getType, delete, *loc, expr, index
    };
    return (AST *)node;
}
