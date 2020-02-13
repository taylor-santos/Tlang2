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
    json_vector(ast->vars, (JSON_MAP_TYPE)json_underscore, out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(ast->expr, out, indent);
    json_end(out, &indent);
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
            "assigning to variable from none");
        return 1;
    }
    nvars = Vector_size(ast->vars);
    if (typeOf(expr_type) == TYPE_SPREAD) {
        const struct SpreadType *spread = getTypeData(expr_type);
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
                size_t len = strlen(name);
                Type *prev_type = NULL;
                if (!Map_get(state->symbols, name, len, &prev_type)) {
                    if (TypeCompare(type, prev_type, state)) {
                        char *oldTypeName = typeToString(prev_type);
                        char *newTypeName = typeToString(type);
                        print_code_error(stderr,
                            ast->super.loc,
                            "redefinition of variable \"%s\" from type "
                            "\"%s\" to type \"%s\"",
                            name,
                            oldTypeName,
                            newTypeName);
                        free(oldTypeName);
                        free(newTypeName);
                        status = 1;
                    } else {
                        setInit(prev_type, 1);
                        if (NULL != state->newInitSymbols) {
                            Map_put(state->newInitSymbols,
                                name,
                                len,
                                NULL,
                                NULL);
                        }
                    }
                } else {
                    Type *type_copy = copy_type(type);
                    setInit(type_copy, 1);
                    Map_put(state->symbols, name, len, type_copy, NULL);
                }
                var_index++;
            }
        }

        return status;
    }
    // Right-hand expression is not a spread tuple
    for (size_t i = 0; i < nvars; i++) {
        char *name = Vector_get(ast->vars, i);
        if (name != NULL) {
            //Not an ignored variable (_)
            size_t len = strlen(name);
            Type *prev_type = NULL;
            if (!Map_get(state->symbols, name, len, &prev_type)) {
                if (TypeCompare(expr_type, prev_type, state)) {
                    char *oldTypeName = typeToString(prev_type);
                    char *newTypeName = typeToString(expr_type);
                    print_code_error(stderr,
                        ast->super.loc,
                        "redefinition of variable \"%s\" from type "
                        "\"%s\" to type \"%s\"",
                        name,
                        oldTypeName,
                        newTypeName);
                    free(oldTypeName);
                    free(newTypeName);
                    status = 1;
                } else {
                    setInit(prev_type, 1);
                    if (NULL != state->newInitSymbols) {
                        Map_put(state->newInitSymbols, name, len, NULL, NULL);
                    }
                }
            } else {
                Type *type_copy = copy_type(expr_type);
                setInit(type_copy, 1);
                Map_put(state->symbols, name, len, type_copy, NULL);
            }
        }
    }
    *typeptr = expr_type;
    return status;
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
        { json, getType, delete, loc }, vars, expr
    };
    return (AST *)definition;
}
