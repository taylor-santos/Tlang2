#include "ast.h"
#include "parser.h"

typedef struct ASTData ASTData;

struct AST {
    void (*json)(const AST *this, FILE *out, int indent);
    int (*getType)(AST *this, UNUSED TypeCheckState *state, Type **typeptr);
    void (*delete)(AST *this);
    struct YYLTYPE loc;
};

inline void
json_AST(const AST *this, FILE *out, int indent) {
    this->json(this, out, indent);
}

inline int
getType_AST(AST *this, UNUSED TypeCheckState *state, struct Type **typeptr) {
    return this->getType(this, state, typeptr);
}

inline void
delete_AST(AST *this) {
    ((AST *)this)->delete(this);
}
