#ifndef AST_H
#define AST_H

#include <stdio.h>

typedef struct AST AST;

struct AST;
struct Vector;
struct Type;

void
json_AST(const AST *this, FILE *out, int indent);

void
delete_AST(AST *this);

#define ASTProgram(stmts) new_ASTProgram(stmts)
AST *
new_ASTProgram(struct Vector *stmts);

#define ASTDefinition(ident, expr) new_ASTDefinition(ident, expr)
AST *
new_ASTDefinition(char *ident, AST *expr);

#define ASTVariable(name) new_ASTVariable(name)
AST *
new_ASTVariable(char *name);

#define ASTMember(expr, name) new_ASTMember(expr, name)
AST *
new_ASTMember(AST *expr, char *name);

#define ASTCall(expr, args) new_ASTCall(expr, args)
AST *
new_ASTCall(AST *expr, struct Vector *args);

#define ASTClass(gen, inherit, members) new_ASTClass(gen, inherit, members)
AST *
new_ASTClass(struct Vector *generics,
    struct Vector *inherits,
    struct Vector *members);

#define ASTReturn(expr) new_ASTReturn(expr)
AST *
new_ASTReturn(AST *expr);

#define ASTFunc(gen, args, ret, stmts) new_ASTFunc(gen, args, ret, stmts)
AST *
new_ASTFunc(struct Vector *generics,
    struct Vector *args,
    struct Type *ret_type,
    struct Vector *stmts);

#define ASTNamedType(name, type) new_ASTNamedType(name, type)
AST *
new_ASTNamedType(char *name, struct Type *type);

#define ASTInit(name, gen, args) new_ASTInit(name, gen, args)
AST *
new_ASTInit(char *name, struct Vector *generics, struct Vector *args);

#define ASTInt(val) new_ASTInt(val)
AST *
new_ASTInt(long long int val);

#define ASTDouble(val) new_ASTDouble(val)
AST *
new_ASTDouble(double val);

#endif
