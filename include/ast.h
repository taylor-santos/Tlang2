#ifndef AST_H
#define AST_H

#include <stdio.h>

typedef struct AST AST;

struct AST;
struct Vector;
struct Type;
struct dstring;
struct YYLTYPE;

void
json_AST(const AST *this, FILE *out, int indent);

void
error_AST(const AST *this, const char *msg, FILE *out);

void
delete_AST(AST *this);

#define ASTProgram(loc, stmts) \
    new_ASTProgram(loc, stmts)
AST *
new_ASTProgram(struct YYLTYPE *loc, struct Vector *stmts);

#define ASTDefinition(loc, vars, expr) \
    new_ASTDefinition(loc, vars, expr)
AST *
new_ASTDefinition(struct YYLTYPE *loc, AST *vars, AST *expr);

#define ASTVariable(loc, name) \
    new_ASTVariable(loc, name)
AST *
new_ASTVariable(struct YYLTYPE *loc, char *name);

#define ASTMember(loc, expr, name) \
    new_ASTMember(loc, expr, name)
AST *
new_ASTMember(struct YYLTYPE *loc, AST *expr, char *name);

#define ASTCall(loc, expr, args) \
    new_ASTCall(loc, expr, args)
AST *
new_ASTCall(struct YYLTYPE *loc, AST *expr, AST *args);

#define ASTIndex(loc, expr, index) \
    new_ASTIndex(loc, expr, index)
AST *
new_ASTIndex(struct YYLTYPE *loc, AST *expr, AST *index);

#define ASTClass(loc, gen, inherit, members) \
    new_ASTClass(loc, gen, inherit, members)
AST *
new_ASTClass(struct YYLTYPE *loc,
    struct Vector *generics,
    struct Vector *inherits,
    struct Vector *members);

#define ASTReturn(loc, expr) \
    new_ASTReturn(loc, expr)
AST *
new_ASTReturn(struct YYLTYPE *loc, AST *expr);

#define ASTFunc(loc, gen, args, ret, stmts) \
    new_ASTFunc(loc, gen, args, ret, stmts)
AST *
new_ASTFunc(struct YYLTYPE *loc,
    struct Vector *generics,
    struct Vector *args,
    struct Type *ret_type,
    struct Vector *stmts);

#define ASTNamedType(loc, name, type) \
    new_ASTNamedType(loc, name, type)
AST *
new_ASTNamedType(struct YYLTYPE *loc, char *name, struct Type *type);

#define ASTTypeStmt(loc, var, type) \
    new_ASTTypeStmt(loc, var, type)
AST *
new_ASTTypeStmt(struct YYLTYPE *loc, char *var, struct Type *type);

#define ASTInit(loc, name, gen, args) \
    new_ASTInit(loc, name, gen, args)
AST *
new_ASTInit(struct YYLTYPE *loc,
    char *name,
    struct Vector *generics,
    AST *args);

#define ASTTuple(loc, exprs) \
    new_ASTTuple(loc, exprs)
AST *
new_ASTTuple(struct YYLTYPE *loc, struct Vector *exprs);

#define ASTInt(loc, val) \
    new_ASTInt(loc, val)
AST *
new_ASTInt(struct YYLTYPE *loc, long long int val);

#define ASTDouble(loc, val) \
    new_ASTDouble(loc, val)
AST *
new_ASTDouble(struct YYLTYPE *loc, double val);

#define ASTString(loc, str) \
    new_ASTString(loc, str)
AST *
new_ASTString(struct YYLTYPE *loc, struct dstring *str);

#define ASTBool(loc, val) \
    new_ASTBool(loc, val)
AST *
new_ASTBool(struct YYLTYPE *loc, int val);

#endif
