#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"

typedef struct ASTMember ASTMember;

struct ASTMember {
    void (*json)(const ASTMember *this, FILE *out, int indent);
    void (*delete)(ASTMember *this);
    struct YYLTYPE loc;
    AST *expr;
    char *name;
};

static void
json(const ASTMember *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("member", out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(this->expr, out, indent);
    json_comma(out, indent);
    json_label("name", out);
    json_string(this->name, out, indent);
    json_end(out, &indent);
}

static void
delete(ASTMember *this) {
    delete_AST(this->expr);
    free(this->name);
    free(this);
}

AST *
new_ASTMember(struct YYLTYPE *loc, AST *expr, char *name) {
    ASTMember *member = NULL;

    member = safe_malloc(sizeof(*member));
    *member = (ASTMember){
        json, delete, *loc, expr, name
    };
    return (AST *)member;
}
