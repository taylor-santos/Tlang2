#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "parser.h"
#include "map.h"

typedef struct ASTDefinition ASTDefinition;

struct ASTDefinition {
    AST super;
    Vector *vars;  // Vector<char* | NULL>: NULL indicates ignored variables
    AST *expr;
};

static void
json_underscore(const char *str, FILE *out, int indent) {
    if (str == NULL) {
        json_string("_", out, indent);
    } else {
        json_string(str, out, indent);
    }
}

static void
json(const void *this, FILE *out, int indent) {
    const ASTDefinition *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("definition", out, indent);
    json_comma(out, indent);
    json_label("variables", out);
    json_vector(ast->vars, (JSON_VALUE_FUNC)json_underscore, out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(ast->expr, out, indent);
    json_end(out, &indent);
}

static int
handle_spread(const struct SpreadType *spread,
    size_t nvars,
    ASTDefinition *ast,
    const TypeCheckState *state) {
    int status = 0;
    size_t nspread = SparseVector_count(spread->types);
    if (nvars != nspread) {
        print_code_error(stderr,
            ast->expr->loc,
            "assignment to %d variable%s from %d tuple value%s",
            nvars,
            nvars == 1
                ? ""
                : "s",
            nspread,
            nspread == 1
                ? ""
                : "s");
        return 1;
    }
    size_t sparse_size = SparseVector_size(spread->types);
    size_t var_index = 0;
    for (size_t i = 0; i < sparse_size; i++) {
        unsigned long long count;
        Type *type = NULL;
        SparseVector_get(spread->types, i, &type, &count);
        for (unsigned long long j = 0; j < count; j++) {
            char *name = Vector_get(ast->vars, var_index);
            if (NULL != name) {
                size_t len = strlen(name);
                type->init = 1;
                char *msg;
                Type *type_copy = copy_type(type);
                if (AddSymbol(state->symbols,
                    name,
                    len,
                    type_copy,
                    state,
                    &msg)) {
                    print_code_error(stderr, ast->super.loc, "%s", msg);
                    free(msg);
                    status = 1;
                }
            }
            var_index++;
        }
    }
    return status;
}

static int
getType(void *this, TypeCheckState *state, Type **typeptr) {
    ASTDefinition *ast = this;
    Type *expr_type = NULL;
    int status = 0;
    size_t nvars;

    if (ast->expr->getType(ast->expr, state, &expr_type)) {
        return 1;
    }
    if (expr_type == NULL) {
        print_code_error(stderr,
            ast->super.loc,
            "%s",
            "assigning to variable from none");
        return 1;
    }
    nvars = Vector_size(ast->vars);
    if (TYPE_SPREAD == expr_type->type) {
        return handle_spread((const struct SpreadType *)expr_type,
            nvars,
            ast,
            state);
    }
    // Right-hand expression is not a spread tuple
    for (size_t i = 0; i < nvars; i++) {
        char *name = Vector_get(ast->vars, i);
        if (name != NULL) {
            //Not an ignored variable (_)
            size_t len = strlen(name);
            expr_type->init = 1;
            char *msg;
            Type *type_copy = copy_type(expr_type);
            if (AddSymbol(state->symbols, name, len, type_copy, state, &msg)) {
                print_code_error(stderr, ast->super.loc, "%s", msg);
                free(msg);
                status = 1;
            }
        }
    }
    *typeptr = expr_type;
    return status;
}

static char *
codeGen(void *this, FILE *out, CodeGenState *state) {
    ASTDefinition *ast = this;
    char *code = ast->expr->codeGen(ast->expr, out, state);
    fprintf(out, "%*s", state->indent * 4, "");
    size_t n = Vector_size(ast->vars);
    for (size_t i = 0; i < n; i++) {
        char *var = Vector_get(ast->vars, i);
        if (NULL != var) {
            fprintf(out, "var_%s = ", var);
        }
    }
    fprintf(out, "%s;\n", code);
    free(code);
    return NULL;
}

static void
delete(void *this) {
    ASTDefinition *ast = this;
    delete_Vector(ast->vars, free);
    delete_AST(ast->expr);
    free(this);
}

AST *
new_ASTDefinition(YYLTYPE loc, Vector *vars, AST *expr) {
    ASTDefinition *definition = NULL;

    definition = safe_malloc(sizeof(*definition));
    *definition = (ASTDefinition){
        {
            json,
            getType,
            codeGen,
            delete,
            loc,
            NULL
        },
        vars,
        expr
    };
    return (AST *)definition;
}
