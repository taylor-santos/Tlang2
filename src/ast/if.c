#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "parser.h"
#include "map.h"

typedef struct ASTIf ASTIf;

struct Vector;

struct ASTIf {
    AST super;
    AST *cond;
    Vector *trueStmts;  // Vector<AST*>
    Vector *falseStmts; // Vector<AST*>
    Type *type;        // NULL until type checker is executed.
    Map *trueSymbols;  // NULL until type checker is executed.
    Map *falseSymbols; // NULL until type checker is executed.
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTIf *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("if", out, indent);
    json_comma(out, indent);
    json_label("cond", out);
    json_AST(ast->cond, out, indent);
    json_comma(out, indent);
    json_label("true stmts", out);
    json_vector(ast->trueStmts, (JSON_MAP_TYPE)json_AST, out, indent);
    json_comma(out, indent);
    json_label("false stmts", out);
    json_vector(ast->falseStmts, (JSON_MAP_TYPE)json_AST, out, indent);
    json_end(out, &indent);
}

static int
getType(void *this, TypeCheckState *state, UNUSED Type **typeptr) {
    ASTIf *ast = this;
    Type *condType, *newType;
    int status = 0;

    if (ast->cond->getType(ast->cond, state, &condType)) {
        status = 1;
    } else if (typeOf(condType) != TYPE_OBJECT) {
        char *typeName = typeToString(condType);
        print_code_error(stderr,
            ast->cond->loc,
            "invalid condition type \"%s\"",
            typeName);
        free(typeName);
        status = 1;
    } else {
        const struct ObjectType *object = getTypeData(condType);
        const struct ClassType *class = object->class;
        const char *fieldName = "toBool";
        Type *fieldType;
        if (Map_get(class->fields, fieldName, strlen(fieldName), &fieldType)) {
            print_code_error(stderr,
                ast->cond->loc,
                "conditional expression type does not implement a toBool() "
                "method");
            status = 1;
        } else if (typeOf(fieldType) != TYPE_FUNC) {
            print_code_error(stderr,
                ast->cond->loc,
                "conditional expression has a toBool field, but it is not a "
                "method");
            status = 1;
        } else {
            const struct FuncType *funcType = getTypeData(fieldType);
            size_t ngens = Vector_size(funcType->generics);
            size_t nargs = Vector_size(funcType->args);
            if (ngens != 0) {
                print_code_error(stderr,
                    ast->cond->loc,
                    "conditional expression's toBool() method cannot be a "
                    "generic function");
                status = 1;
            }
            if (nargs != 0) {
                print_code_error(stderr,
                    ast->cond->loc,
                    "conditional expression's toBool() method must take zero"
                    " arguments");
                status = 1;
            }
            Type *retType = funcType->ret_type;
            const struct ObjectType *retObj = getTypeData(retType);
            if (typeOf(retType) != TYPE_OBJECT ||
                retObj->class != state->builtins[BUILTIN_BOOL]) {
                print_code_error(stderr,
                    ast->cond->loc,
                    "conditional expression's toBool() method must return a "
                    "boolean value");
                status = 1;
            }
        }
    }
    Map *prevSymbols = state->symbols;
    Map *trueNewSymbols = NULL, *falseNewSymbols = NULL;
    size_t nTrue = Vector_size(ast->trueStmts);
    size_t nFalse = Vector_size(ast->falseStmts);
    if (nTrue != 0 && nFalse != 0) {
        trueNewSymbols = Map();
        falseNewSymbols = Map();
    }

    // True Branch
    if (nTrue != 0) {
        ast->trueSymbols = copy_Map(prevSymbols, (MAP_COPY_FUNC)copy_type);
        state->symbols = ast->trueSymbols;
        state->newSymbols = trueNewSymbols;
    }
    for (size_t i = 0; i < nTrue; i++) {
        AST *stmt = Vector_get(ast->trueStmts, i);
        Type *type;
        if (stmt->getType(stmt, state, &type)) {
            status = 1;
        }
    }

    // False Branch
    if (nFalse != 0) {
        ast->falseSymbols = copy_Map(prevSymbols, (MAP_COPY_FUNC)copy_type);
        state->symbols = ast->falseSymbols;
        state->newSymbols = falseNewSymbols;
    }
    for (size_t i = 0; i < nFalse; i++) {
        AST *stmt = Vector_get(ast->falseStmts, i);
        Type *type;
        if (stmt->getType(stmt, state, &type)) {
            status = 1;
        }
    }

    state->symbols = prevSymbols;
    if (nTrue != 0 && nFalse != 0) {
        // Check for compatible symbol overlap between the two branches
        Iterator *it = Map_iterator(trueNewSymbols);
        while (it->hasNext(it)) {
            MapIterData symbol = it->next(it);
            Type *falseType, *trueType = symbol.value;
            if (!Map_get(falseNewSymbols,
                symbol.key,
                symbol.len,
                &falseType)) {
                if (!TypeIntersection(trueType,
                    falseType,
                    state,
                    ast->super.loc,
                    &newType)) {
                    Map_put(state->symbols,
                        symbol.key,
                        symbol.len,
                        newType,
                        NULL);
                }
            }
        }
        it->delete(it);
        delete_Map(trueNewSymbols, (MAP_DELETE_FUNC)delete_type);
        delete_Map(falseNewSymbols, (MAP_DELETE_FUNC)delete_type);
        state->newSymbols = NULL;
    }

    return status;
}

static void
delete(void *this) {
    ASTIf *ast = this;
    delete_AST(ast->cond);
    delete_Vector(ast->trueStmts, (VEC_DELETE_FUNC)delete_AST);
    delete_Vector(ast->falseStmts, (VEC_DELETE_FUNC)delete_AST);
    if (NULL != ast->type) {
        delete_type(ast->type);
    }
    if (NULL != ast->trueSymbols) {
        delete_Map(ast->trueSymbols, (MAP_DELETE_FUNC)delete_type);
    }
    if (NULL != ast->falseSymbols) {
        delete_Map(ast->falseSymbols, (MAP_DELETE_FUNC)delete_type);
    }
    free(this);
}

AST *
new_ASTIf(YYLTYPE loc, AST *cond, Vector *trueStmts, Vector *falseStmts) {
    ASTIf *node = NULL;

    node = safe_malloc(sizeof(*node));
    *node = (ASTIf){
        { json, getType, delete, loc },
        cond,
        trueStmts,
        falseStmts,
        NULL,
        NULL,
        NULL
    };
    return (AST *)node;
}
