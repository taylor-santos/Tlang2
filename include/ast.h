#ifndef AST_H
#define AST_H

#include <stdio.h>

struct Vector;
struct SparseVector;
struct Type;
struct dstring;
struct TypeCheckState;
struct CodeGenState;

#define YYLTYPE YYLTYPE
typedef struct YYLTYPE {
    int first_line;
    int first_column;
    int last_line;
    int last_column;
    const char *filename;
} YYLTYPE;

typedef struct AST {
    void (*json)(const void *this, FILE *out, int indent);
    int (*getType)(void *this,
        struct TypeCheckState *state,
        struct Type **typeptr);
    char *(*codeGen)(void *this, FILE *out, struct CodeGenState *state);
    void (*delete)(void *this);
    struct YYLTYPE loc;
    struct Type *type;
} AST;

struct Field {
    struct Vector *names;
    struct Type *type;
};

struct ClassBody {
    struct Vector *fields; // Vector<Field*>
    struct Vector *ctors;  // Vector<AST*>
};

struct Case {
    enum {
        CASE_EXPR, CASE_TYPE
    } caseType;
    union {
        AST *expr;
        struct {
            char *name;
            struct Type *type;
        } type;
    };
    struct Vector *stmts;
};

#define TypeCase(name, type, stmts) \
    new_TypeCase(name, type, stmts)
struct Case *
new_TypeCase(char *name, struct Type *type, struct Vector *stmts);

#define ExprCase(expr, stmts) \
    new_ExprCase(expr, stmts)
struct Case *
new_ExprCase(AST *expr, struct Vector *stmts);

void
json_AST(const AST *this, FILE *out, int indent);

void
json_field(const struct Field *field, FILE *out, int indent);

void
json_case(const struct Case *c, FILE *out, int indent);

void
delete_field(struct Field *field);

void
delete_ctor(struct Vector *ctor);

void
delete_case(struct Case *c);

void
delete_AST(AST *this);

void
codeGenFuncBody(void *this, FILE *out, struct CodeGenState *state);

#define TypeCheck(root) root->getType(root, NULL, NULL)

#define CodeGen(root, out) root->codeGen(root, out, NULL)

#define ASTProgram(loc, stmts) \
    new_ASTProgram(loc, stmts)
AST *
new_ASTProgram(YYLTYPE loc, struct Vector *stmts);

#define ASTDefinition(loc, vars, expr) \
    new_ASTDefinition(loc, vars, expr)
AST *
new_ASTDefinition(YYLTYPE loc, struct Vector *vars, AST *expr);

#define ASTVariable(loc, name) \
    new_ASTVariable(loc, name)
AST *
new_ASTVariable(YYLTYPE loc, char *name);

#define ASTMember(loc, expr, name) \
    new_ASTMember(loc, expr, name)
AST *
new_ASTMember(YYLTYPE loc, AST *expr, char *name);

#define ASTCall(loc, expr, args) \
    new_ASTCall(loc, expr, args)
AST *
new_ASTCall(YYLTYPE loc, AST *expr, struct Vector *args);

#define ASTIndex(loc, expr, index) \
    new_ASTIndex(loc, expr, index)
AST *
new_ASTIndex(YYLTYPE loc, AST *expr, AST *index);

#define ASTConstIndex(loc, expr, index) \
    new_ASTConstIndex(loc, expr, index)
AST *
new_ASTConstIndex(YYLTYPE loc, AST *expr, long long int index);

#define ASTImpl(loc, name, gen, stmts) new_ASTImpl(loc, name, gen, stmts)
AST *
new_ASTImpl(YYLTYPE loc,
    char *name,
    struct Vector *generics,
    struct Vector *stmts);

#define ASTReturn(loc, expr) \
    new_ASTReturn(loc, expr)
AST *
new_ASTReturn(YYLTYPE loc, AST *expr);

#define ASTFunc(loc, gen, args, ret, stmts) \
    new_ASTFunc(loc, gen, args, ret, stmts)
AST *
new_ASTFunc(YYLTYPE loc,
    struct Vector *generics,
    struct Vector *args,
    struct Type *ret_type,
    struct Vector *stmts);

#define ASTTypeStmt(loc, vars, type) \
    new_ASTTypeStmt(loc, vars, type)
AST *
new_ASTTypeStmt(YYLTYPE loc, struct Vector *vars, struct Type *type);

#define ASTInit(loc, name, gen, args) \
    new_ASTInit(loc, name, gen, args)
AST *
new_ASTInit(YYLTYPE loc,
    char *name,
    struct Vector *generics,
    struct Vector *args);

#define ASTTuple(loc, exprs) \
    new_ASTTuple(loc, exprs)
AST *
new_ASTTuple(YYLTYPE loc, struct SparseVector *exprs);

#define ASTSpread(loc, expr) \
    new_ASTSpread(loc, expr)
AST *
new_ASTSpread(YYLTYPE loc, AST *expr);

#define ASTInt(loc, val) \
    new_ASTInt(loc, val)
AST *
new_ASTInt(YYLTYPE loc, long long int val);

#define ASTDouble(loc, val) \
    new_ASTDouble(loc, val)
AST *
new_ASTDouble(YYLTYPE loc, double val);

#define ASTString(loc, str) \
    new_ASTString(loc, str)
AST *
new_ASTString(YYLTYPE loc, struct dstring str);

#define ASTBool(loc, val) \
    new_ASTBool(loc, val)
AST *
new_ASTBool(YYLTYPE loc, int val);

#define ASTArray(loc, type, index) \
    new_ASTArray(loc, type, index)
AST *
new_ASTArray(YYLTYPE loc, struct Type *array_type, long long int index);

#define ASTIf(loc, cond, true, false) \
    new_ASTIf(loc, cond, true, false)
AST *
new_ASTIf(YYLTYPE loc,
    AST *cond,
    struct Vector *trueStmts,
    struct Vector *falseStmts);

#define ASTWhile(loc, cond, stmts) \
    new_ASTWhile(loc, cond, stmts)
AST *
new_ASTWhile(YYLTYPE loc, AST *cond, struct Vector *stmts);

#define ASTDo(loc, cond, stmts) \
    new_ASTDo(loc, cond, stmts)
AST *
new_ASTDo(YYLTYPE loc, AST *cond, struct Vector *stmts);

#define ASTSwitch(loc, expr, cases, def) \
    new_ASTSwitch(loc, expr, cases, def)
AST *
new_ASTSwitch(YYLTYPE loc,
    AST *expr,
    struct Vector *cases,
    struct Vector *def);

#define ASTCast(loc, expr, type) \
    new_ASTCast(loc, expr, type)
AST *
new_ASTCast(YYLTYPE loc, AST *expr, struct Type *type);

#endif
