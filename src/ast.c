#include "ast.h"
#include "parser.h"

typedef struct ASTData ASTData;

struct AST {
    void (*json)(const AST *this, FILE *out, int indent);
    int (*getType)(const AST *this, Type **typeptr);
    void (*delete)(AST *this);
    struct YYLTYPE loc;
};

void
json_AST(const AST *this, FILE *out, int indent) {
    this->json(this, out, indent);
}

void
delete_AST(AST *this) {
    ((AST *)this)->delete(this);
}

int
getType_AST(const AST *this, struct Type **typeptr) {
    return this->getType(this, typeptr);
}
