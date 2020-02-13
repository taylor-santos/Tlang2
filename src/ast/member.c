#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"
#include "map.h"

typedef struct ASTMember ASTMember;

struct ASTMember {
    AST super;
    AST *expr;
    char *name;
    Type *type; // NULL until type checker is executed.
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTMember *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("member", out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(ast->expr, out, indent);
    json_comma(out, indent);
    json_label("name", out);
    json_string(ast->name, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, UNUSED TypeCheckState *state, UNUSED Type **typeptr) {
    ASTMember *ast = this;
    Type *exprType = NULL;
    if (ast->expr->getType(ast->expr, state, &exprType)) {
        return 1;
    }
    char *msg;
    if (TypeVerify(exprType, state, &msg)) {
        print_code_error(stderr, ast->super.loc, msg);
        free(msg);
        return 1;
    }
    if (TYPE_OBJECT != typeOf(exprType)) {
        char *typeName = typeToString(exprType);
        print_code_error(stderr,
            ast->super.loc,
            "member access operator used on non-object type \"%s\"",
            typeName);
        free(typeName);
        return 1;
    }
    const struct ObjectType *object = getTypeData(exprType);
    const struct ClassType *class = object->class;
    Type *fieldType;
    if (Map_get(class->fields, ast->name, strlen(ast->name), &fieldType)) {
        char *typeName = typeToString(exprType);
        print_code_error(stderr,
            ast->super.loc,
            "object with type \"%s\" doesn't have a member \"%s\"",
            typeName,
            ast->name);
        free(typeName);
        return 1;
    }
    *typeptr = ast->type = copy_type(fieldType);
    return 0;
}

static void
delete(void *this) {
    ASTMember *ast = this;
    delete_AST(ast->expr);
    free(ast->name);
    if (NULL != ast->type) {
        delete_type(ast->type);
    }
    free(this);
}

AST *
new_ASTMember(YYLTYPE loc, AST *expr, char *name) {
    ASTMember *member = NULL;

    member = safe_malloc(sizeof(*member));
    *member = (ASTMember){
        { json, getType, delete, loc }, expr, name, NULL
    };
    return (AST *)member;
}
