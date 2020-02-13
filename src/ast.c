#include "ast.h"
#include "json.h"
#include "parser.h"

typedef struct ASTData ASTData;

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
delete_field(struct Field *field) {
    delete_Vector(field->names, free);
    delete_type(field->type);
    free(field);
}

inline void
delete_AST(AST *this) {
    ((AST *)this)->delete(this);
}
