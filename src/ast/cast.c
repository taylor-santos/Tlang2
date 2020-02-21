#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"
#include "map.h"

typedef struct ASTCast ASTCast;

struct Vector;

struct ASTCast {
    AST super;
    AST *expr;
    Type *cast_type;
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTCast *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("cast", out, indent);
    json_comma(out, indent);
    json_label("type", out);
    json_type(ast->cast_type, out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(ast->expr, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, TypeCheckState *state, Type **typeptr) {
    ASTCast *ast = this;
    char *msg;
    if (ast->cast_type->verify(ast->cast_type, state, &msg)) {
        print_code_error(stderr, ast->cast_type->loc, msg);
        free(msg);
        return 1;
    }

    Type *exprType;
    if (ast->expr->getType(ast->expr, state, &exprType)) {
        return 1;
    }
    if (TYPE_OBJECT != exprType->type) {
        char *typeName = exprType->toString(exprType);
        print_code_error(stderr,
            ast->expr->loc,
            "expression with type \"%s\" does not support casting",
            typeName);
        free(typeName);
        return 1;
    }
    const struct ObjectType *object = (const struct ObjectType *)exprType;
    const struct ClassType *class = object->class;
    const char *fieldName = "=>";
    Type *fieldType;
    if (Map_get(class->fields, fieldName, strlen(fieldName), &fieldType)) {
        char *typeName = exprType->toString(exprType);
        print_code_error(stderr,
            ast->expr->loc,
            "expression with type \"%s\" does not implement a cast operator",
            typeName);
        free(typeName);
        return 1;
    }
    for (struct FuncType *func = (struct FuncType *)fieldType;
        NULL != func;
        func = func->next) {
        if (!func->ret_type->compare(func->ret_type, ast->cast_type, state)) {
            // Found right cast
            *typeptr = ast->cast_type;
            return 0;
        }
    }
    char *typeName = exprType->toString(exprType);
    char *castName = ast->cast_type->toString(ast->cast_type);
    print_code_error(stderr,
        ast->expr->loc,
        "expression with type \"%s\" does not implement a cast to type \"%s\"",
        typeName,
        castName);
    free(typeName);
    free(castName);
    return 1;
}

static void
delete(void *this) {
    ASTCast *ast = this;
    delete_AST(ast->expr);
    delete_type(ast->cast_type);
    free(this);
}

AST *
new_ASTCast(struct YYLTYPE loc, AST *expr, Type *type) {
    ASTCast *cast = NULL;

    cast = safe_malloc(sizeof(*cast));
    *cast = (ASTCast){
        { json, getType, delete, loc }, expr, type
    };
    return (AST *)cast;
}
