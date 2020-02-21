#include "ast.h"
#include <stdlib.h>
#include <ast.h>
#include "safe.h"
#include "json.h"
#include "vector.h"
#include "parser.h"
#include "map.h"

typedef struct ASTSwitch ASTSwitch;

struct Vector;

struct ASTSwitch {
    AST super;
    AST *expr;
    Vector *cases;       // Vector<Case*>
    Vector *def;         // NULLable Vector<AST*>
    // NULL until type checker is executed:
    Vector *symbolsList; // Vector<Map<char*, Type*>>
    Map *defSymbols;     // Map<char*, Type*>
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTSwitch *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("switch", out, indent);
    json_comma(out, indent);
    json_label("expr", out);
    json_AST(ast->expr, out, indent);
    json_comma(out, indent);
    json_label("cases", out);
    json_vector(ast->cases, (JSON_VALUE_FUNC)json_case, out, indent);
    if (NULL != ast->def) {
        json_comma(out, indent);
        json_label("default", out);
        json_vector(ast->def, (JSON_VALUE_FUNC)json_AST, out, indent);
    }
    json_end(out, &indent);
}

static int
typeCheckStmts(Vector *stmts, TypeCheckState *state) {
    int status = 0;
    size_t nstmts = Vector_size(stmts);

    for (size_t i = 0; i < nstmts; i++) {
        AST *stmt = Vector_get(stmts, i);
        Type *type;
        status = stmt->getType(stmt, state, &type) || status;
    }
    return status;
}

static int
exprCase(struct Case *c,
    UNUSED Type *switchType,
    UNUSED TypeCheckState *state) {
    Type *type;
    if (c->expr->getType(c->expr, state, &type)) {
        return 1;
    }
    if (TYPE_OBJECT != switchType->type) {
        char *switchName = switchType->toString(switchType),
            *exprName = type->toString(type);
        print_code_error(stderr,
            c->expr->loc,
            "switch expression with type \"%s\" can't be compared to case "
            "expression with type \"%s\". Expression cases only work with "
            "class instances as the switch argument",
            switchName,
            exprName);
        free(switchName);
        free(exprName);
        return 1;
    }
    const struct ObjectType *object = (const struct ObjectType *)switchType;
    const struct ClassType *class = object->class;
    const char *fieldName = "==";
    Type *fieldType;
    if (Map_get(class->fields, fieldName, strlen(fieldName), &fieldType)) {
        char *switchName = switchType->toString(switchType);
        print_code_error(stderr,
            c->expr->loc,
            "switch expression with type \"%s\" does not implement the "
            "\"==\" operator",
            switchName);
        free(switchName);
        return 1;
    }
    struct FuncType *found = NULL;
    for (struct FuncType *func = (struct FuncType *)fieldType;
        NULL != func;
        func = func->next) {
        Type *argType = Vector_get(func->args, 0);
        if (!type->compare(type, argType, state)) {
            found = func;
            continue;
        }
    }
    if (NULL == found) {
        char *caseTypeName = type->toString(type);
        print_code_error(stderr,
            c->expr->loc,
            "switch expression's \"==\" operator does not accept an argument"
            " with type \"%s\"",
            caseTypeName);
        free(caseTypeName);
        return 1;
    }
    Type *retType = found->ret_type;
    const struct ObjectType *ret = (const struct ObjectType *)retType;
    if (TYPE_OBJECT != retType->type || ret->class->super.compare(ret->class,
        state->builtins[BUILTIN_BOOL],
        state)) {
        print_code_error(stderr,
            c->expr->loc,
            "switch expression's \"==\" operator does not return a \"boolean"
            " instance\"");
        return 1;
    }
    return typeCheckStmts(c->stmts, state);
}

static int
typeCase(struct Case *c, Type *exprType, TypeCheckState *state) {
    char *msg;
    if (c->type.type->verify(c->type.type, state, &msg)) {
        print_code_error(stderr, c->expr->loc, msg);
        free(msg);
        return 1;
    }
    c->type.type->init = 1;
    if (0 != c->type.type->compare(c->type.type, exprType, state)) {
        char *exprTypeName = exprType->toString(exprType),
            *caseTypeName = c->type.type->toString(c->type.type);
        print_code_error(stderr,
            c->type.type->loc,
            "case type \"%s\" is not a sub-type of switched type \"%s\"",
            caseTypeName,
            exprTypeName);
        free(exprTypeName);
        free(caseTypeName);
        return 1;
    }
    Type *prevType = NULL, *type_copy = copy_type(c->type.type);
    Map_put(state->symbols,
        c->type.name,
        strlen(c->type.name),
        type_copy,
        &prevType);
    if (NULL != prevType) {
        delete_type(prevType);
    }
    return typeCheckStmts(c->stmts, state);
}

static void
delete_init(Map *init) {
    delete_Map(init, NULL);
}

static int
getType(void *this, TypeCheckState *state, UNUSED Type **typeptr) {
    ASTSwitch *ast = this;
    Type *exprType;
    int status = 0;
    int allCasesReturn = 1;

    if (ast->expr->getType(ast->expr, state, &exprType)) {
        return 1;
    }
    size_t ncases = Vector_size(ast->cases);
    ast->symbolsList = new_Vector(ncases);
    Vector *initList = new_Vector(ncases);
    Map *prevSymbols = state->symbols;
    Map *prevNewInit = state->newInitSymbols;
    Type *prevRetType = state->retType;
    for (size_t i = 0; i < ncases; i++) {
        state->symbols = copy_Map(prevSymbols, (MAP_COPY_FUNC)copy_type);
        state->newInitSymbols = Map();
        state->retType = NULL;
        Vector_append(ast->symbolsList, state->symbols);
        Vector_append(initList, state->newInitSymbols);
        struct Case *c = Vector_get(ast->cases, i);
        switch (c->caseType) {
            case CASE_EXPR:
                if (TYPE_OBJECT != exprType->type) {
                }
                status = exprCase(c, exprType, state) || status;
                break;
            case CASE_TYPE:
                status = typeCase(c, exprType, state) || status;
                break;
        }
        if (NULL == state->retType) {
            allCasesReturn = 0;
        }
    }
    if (ast->def != NULL) {
        state->symbols = ast->defSymbols =
            copy_Map(prevSymbols, (MAP_COPY_FUNC)copy_type);
        state->newInitSymbols = Map();
        state->retType = NULL;
        status = typeCheckStmts(ast->def, state) || status;
        // Collect initialized variables from all branches
        Iterator *it = Map_iterator(state->newInitSymbols);
        while (it->hasNext(it)) {
            MapIterData symbol = it->next(it);
            Type *type;
            // Check the outer symbol table to make sure it exists first
            if (!Map_get(prevSymbols, symbol.key, symbol.len, &type)) {
                int found = 1;
                // Search each case to see if the variable is not initialized in
                // any of them
                for (size_t i = 0; i < ncases; i++) {
                    Map *caseInit = Vector_get(initList, i);
                    if (!Map_contains(caseInit, symbol.key, symbol.len)) {
                        found = 0;
                        break;
                    }
                }
                // If all cases initialize the variable, set it to be
                // initialized in the outer symbol table, then add it to the
                // outer init list.
                if (found) {
                    type->init = 1;
                    if (NULL != prevNewInit) {
                        Map_put(prevNewInit,
                            symbol.key,
                            symbol.len,
                            NULL,
                            NULL);
                    }
                }
            }
        }
        it->delete(it);
        delete_Map(state->newInitSymbols, NULL);
        // Check if all paths return a value
        if (0 != allCasesReturn && NULL != state->retType) {
            prevRetType = state->retType;
        }
    }
    delete_Vector(initList, (VEC_DELETE_FUNC)delete_init);
    state->symbols = prevSymbols;
    state->newInitSymbols = prevNewInit;
    state->retType = prevRetType;

    return status;
}

static void
delete_symbols(Map *symbols) {
    delete_Map(symbols, (MAP_DELETE_FUNC)delete_type);
}

static void
delete(void *this) {
    ASTSwitch *ast = this;
    delete_AST(ast->expr);
    delete_Vector(ast->cases, (VEC_DELETE_FUNC)delete_case);
    if (NULL != ast->def) {
        delete_Vector(ast->def, (VEC_DELETE_FUNC)delete_AST);
    }
    if (NULL != ast->symbolsList) {
        delete_Vector(ast->symbolsList, (VEC_DELETE_FUNC)delete_symbols);
    }
    if (NULL != ast->defSymbols) {
        delete_Map(ast->defSymbols, (MAP_DELETE_FUNC)delete_type);
    }
    free(this);
}

AST *
new_ASTSwitch(YYLTYPE loc,
    AST *expr,
    struct Vector *cases,
    struct Vector *def) {
    ASTSwitch *node;

    node = safe_malloc(sizeof(*node));
    *node = (ASTSwitch){
        { json, getType, delete, loc }, expr, cases, def, NULL, NULL
    };
    return (AST *)node;
}
