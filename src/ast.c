#include <ast.h>
#include "ast.h"
#include "json.h"
#include "parser.h"

typedef struct ASTData ASTData;

struct Case *
new_TypeCase(char *name, struct Type *type, struct Vector *stmts) {
    struct Case *ret;

    ret = safe_malloc(sizeof(*ret));
    *ret = (struct Case){
        CASE_TYPE, .type = { name, type }, stmts
    };
    return ret;
}

struct Case *
new_ExprCase(AST *expr, struct Vector *stmts) {
    struct Case *ret;

    ret = safe_malloc(sizeof(*ret));
    *ret = (struct Case){
        CASE_EXPR, .expr = expr, stmts
    };
    return ret;
}

inline void
json_AST(const AST *this, FILE *out, int indent) {
    this->json(this, out, indent);
}

void
json_field(const struct Field *field, FILE *out, int indent) {
    json_start(out, &indent);
    json_label("names", out);
    json_vector(field->names, (JSON_MAP_TYPE)json_string, out, indent);
    json_comma(out, indent);
    json_label("type", out);
    json_type(field->type, out, indent);
    json_end(out, &indent);
}

void
json_case(const struct Case *c, FILE *out, int indent) {
    json_start(out, &indent);
    switch (c->caseType) {
        case CASE_EXPR:
            json_label("expr", out);
            json_AST(c->expr, out, indent);
            break;
        case CASE_TYPE:
            json_label("name", out);
            json_string(c->type.name, out, indent);
            json_comma(out, indent);
            json_label("type", out);
            json_type(c->type.type, out, indent);
            break;
    }
    json_comma(out, indent);
    json_label("statements", out);
    json_vector(c->stmts, (JSON_MAP_TYPE)json_AST, out, indent);
    json_end(out, &indent);
}

void
delete_field(struct Field *field) {
    delete_Vector(field->names, free);
    delete_type(field->type);
    free(field);
}

void
delete_case(struct Case *c) {
    switch (c->caseType) {
        case CASE_EXPR:
            delete_AST(c->expr);
            break;
        case CASE_TYPE:
            free(c->type.name);
            delete_type(c->type.type);
            break;
    }
    delete_Vector(c->stmts, (VEC_DELETE_FUNC)delete_AST);
    free(c);
}

inline void
delete_AST(AST *this) {
    ((AST *)this)->delete(this);
}
