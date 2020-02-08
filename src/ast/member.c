#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"
#include "map.h"

typedef struct ASTMember ASTMember;

struct ASTMember {
    void (*json)(const ASTMember *this, FILE *out, int indent);
    int (*getType)(ASTMember *this,
        UNUSED TypeCheckState *state,
        Type **typeptr);
    void (*delete)(ASTMember *this);
    struct YYLTYPE loc;
    AST *expr;
    char *name;
    Type *type; // NULL until type checker is executed.
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

static int
getType(ASTMember *this, UNUSED TypeCheckState *state, UNUSED Type **typeptr) {
    Type *exprType = NULL;
    if (getType_AST(this->expr, state, &exprType)) {
        return 1;
    }
    char *msg;
    if (TypeVerify(exprType, state, &msg)) {
        print_code_error(stderr, this->loc, msg);
        free(msg);
        return 1;
    }
    if (TYPE_OBJECT != typeOf(exprType)) {
        char *typeName = typeToString(exprType);
        print_code_error(stderr,
            this->loc,
            "member access operator used on non-object type \"%s\"",
            typeName);
        free(typeName);
        return 1;
    }
    const struct ObjectType *object = getTypeData(exprType);
    Type *classType = NULL;
    Map_get(state->symbols, object->name, strlen(object->name), &classType);
    const struct ClassType *class = getTypeData(classType);
    Type *fieldType;
    if (Map_get(class->fields, this->name, strlen(this->name), &fieldType)) {
        char *typeName = typeToString(exprType);
        print_code_error(stderr,
            this->loc,
            "object with type \"%s\" doesn't have a member \"%s\"",
            typeName,
            this->name);
        free(typeName);
        return 1;
    }
    *typeptr = this->type = copy_type(fieldType);
    return 0;
}

static void
delete(ASTMember *this) {
    delete_AST(this->expr);
    free(this->name);
    if (NULL != this->type) {
        delete_type(this->type);
    }
    free(this);
}

AST *
new_ASTMember(struct YYLTYPE *loc, AST *expr, char *name) {
    ASTMember *member = NULL;

    member = safe_malloc(sizeof(*member));
    *member = (ASTMember){
        json, getType, delete, *loc, expr, name, NULL
    };
    return (AST *)member;
}
