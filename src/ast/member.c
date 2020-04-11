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
    if (TYPE_OBJECT != exprType->type) {
        char *typeName = exprType->toString(exprType);
        print_code_error(stderr,
            ast->super.loc,
            "member access operator used on non-object type \"%s\"",
            typeName);
        free(typeName);
        return 1;
    }
    const struct ObjectType *object = (const struct ObjectType *)exprType;
    const struct ClassType *class = object->class;
    Type *fieldType;
    if (Map_get(class->fieldTypes, ast->name, strlen(ast->name), &fieldType)) {
        char *typeName = exprType->toString(exprType);
        print_code_error(stderr,
            ast->super.loc,
            "object with type \"%s\" doesn't have a member \"%s\"",
            typeName,
            ast->name);
        free(typeName);
        return 1;
    }
    *typeptr = ast->super.type = copy_type(fieldType);
    return 0;
}

static char *
codeGen(void *this, FILE *out, CodeGenState *state) {
    const ASTMember *ast = this;
    char *code = ast->expr->codeGen(ast->expr, out, state);
    char *tmpName = safe_asprintf("temp%d", state->tempCount);
    state->tempCount++;
    char *typeName = ast->expr->type->codeGen(ast->expr->type, tmpName);
    fprintf(out, "%*s", state->indent * 4, "");
    fprintf(out, "%s = %s;\n", typeName, code);
    free(typeName);
    free(code);

    if (ast->super.type->type == TYPE_FUNC) {
        char *tmpName2 = safe_asprintf("temp%d", state->tempCount);
        state->tempCount++;
        char fieldName[strlen(ast->name) * 2 + 1];
        strident(ast->name, fieldName);
        fprintf(out, "%*s", state->indent * 4, "");
        fprintf(out,
            "closure %s = { %s->field_%s, (void*[]){%s} };\n",
            tmpName2,
            tmpName,
            fieldName,
            tmpName);
        free(tmpName);
        return tmpName2;
    } else {
        char fieldName[strlen(ast->name) * 2 + 1];
        strident(ast->name, fieldName);
        char *ret = safe_asprintf("%s->field_%s", tmpName, fieldName);
        free(tmpName);
        return ret;
    }
}

static void
delete(void *this) {
    ASTMember *ast = this;
    delete_AST(ast->expr);
    free(ast->name);
    if (NULL != ast->super.type) {
        delete_type(ast->super.type);
    }
    free(this);
}

AST *
new_ASTMember(YYLTYPE loc, AST *expr, char *name) {
    ASTMember *member = NULL;

    member = safe_malloc(sizeof(*member));
    *member = (ASTMember){
        {
            json,
            getType,
            codeGen,
            delete,
            loc,
            NULL
        },
        expr,
        name
    };
    return (AST *)member;
}
