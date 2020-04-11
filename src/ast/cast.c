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
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTCast *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("cast", out, indent);
    json_comma(out, indent);
    json_label("type", out);
    json_type(ast->super.type, out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(ast->expr, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, TypeCheckState *state, Type **typeptr) {
    ASTCast *ast = this;
    char *msg;
    if (ast->super.type->verify(ast->super.type, state, &msg)) {
        print_code_error(stderr, ast->super.type->loc, msg);
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
    if (Map_get(class->fieldTypes, fieldName, strlen(fieldName), &fieldType)) {
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
        if (!func->ret_type->compare(func->ret_type, ast->super.type, state)) {
            // Found right cast
            *typeptr = ast->super.type;
            return 0;
        }
    }
    char *typeName = exprType->toString(exprType);
    char *castName = ast->super.type->toString(ast->super.type);
    print_code_error(stderr,
        ast->expr->loc,
        "expression with type \"%s\" does not implement a cast to type \"%s\"",
        typeName,
        castName);
    free(typeName);
    free(castName);
    return 1;
}

static char *
codeGen(void *this, FILE *out, CodeGenState *state) {
    ASTCast *ast = this;
    char *code = ast->expr->codeGen(ast->expr, out, state);
    fprintf(out, "%*s", state->indent * 4, "");
    char *tmpName = safe_asprintf("temp%d", state->tempCount);
    state->tempCount++;
    char *typeName = ast->expr->type->codeGen(ast->expr->type, tmpName);
    fprintf(out, "%s = %s;\n", typeName, code);
    char *castType = ast->super.type->codeGen(ast->super.type, NULL);
    fprintf(out, "%*s", state->indent * 4, "");
    fprintf(out,
        "closure temp%d = { %s->cast_%s, (void*[]){%s} };\n",
        state->tempCount,
        tmpName,
        castType,
        tmpName);
    char *ret = safe_asprintf("CALL(temp%d, NULL)", state->tempCount);
    state->tempCount++;
    return ret;
}

static void
delete(void *this) {
    ASTCast *ast = this;
    delete_AST(ast->expr);
    delete_type(ast->super.type);
    free(this);
}

AST *
new_ASTCast(struct YYLTYPE loc, AST *expr, Type *type) {
    ASTCast *cast = NULL;

    cast = safe_malloc(sizeof(*cast));
    *cast = (ASTCast){
        {
            json,
            getType,
            codeGen,
            delete,
            loc,
            type
        },
        expr
    };
    return (AST *)cast;
}
