#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "types.h"
#include "vector.h"
#include "parser.h"

typedef struct ASTTypeStmt ASTTypeStmt;

struct ASTTypeStmt {
    void (*json)(const ASTTypeStmt *this, FILE *out, int indent);
    int (*getType)(const ASTTypeStmt *this, Type **typeptr);
    void (*delete)(ASTTypeStmt *this);
    struct YYLTYPE loc;
    AST *expr;
    Type *type;
};

static void
json(const ASTTypeStmt *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("type declaration", out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(this->expr, out, indent);
    json_comma(out, indent);
    json_label("type", out);
    json_type(this->type, out, indent);
    json_end(out, &indent);
}

static int
getType(const ASTTypeStmt *this, UNUSED Type **typeptr) {
    print_code_error(&this->loc,
        "type statement type checker not implemented",
        stderr);
    return 1;
}

static void
delete(ASTTypeStmt *this) {
    delete_AST(this->expr);
    delete_type(this->type);
    free(this);
}

AST *
new_ASTTypeStmt(struct YYLTYPE *loc, AST *expr, Type *type) {
    ASTTypeStmt *named_type = NULL;

    named_type = safe_malloc(sizeof(*named_type));
    *named_type = (ASTTypeStmt){
        json, getType, delete, *loc, expr, type
    };
    return (AST *)named_type;
}
