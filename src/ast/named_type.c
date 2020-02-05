#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "types.h"
#include "parser.h"

typedef struct ASTNamedType ASTNamedType;

struct ASTNamedType {
    void (*json)(const ASTNamedType *this, FILE *out, int indent);
    void (*delete)(ASTNamedType *this);
    struct YYLTYPE loc;
    char *name;
    Type *type;
};

static void
json(const ASTNamedType *this, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("node", out);
    json_string("named type", out, indent);
    json_comma(out, indent);
    json_label("name", out);
    json_string(this->name, out, indent);
    json_comma(out, indent);
    json_label("type", out);
    json_type(this->type, out, indent);
    json_end(out, &indent);
}

static void
delete(ASTNamedType *this) {
    free(this->name);
    delete_type(this->type);
    free(this);
}

AST *
new_ASTNamedType(struct YYLTYPE *loc, char *name, Type *type) {
    ASTNamedType *named_type = NULL;

    named_type = safe_malloc(sizeof(*named_type));
    *named_type = (ASTNamedType){
        json, delete, *loc, name, type
    };
    return (AST *)named_type;
}
