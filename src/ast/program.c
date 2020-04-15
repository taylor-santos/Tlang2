#include "ast.h"
#include <stdlib.h>
#include "safe.h"
#include "vector.h"
#include "json.h"
#include "parser.h"
#include "map.h"
#include "types.h"

typedef struct ASTProgram ASTProgram;

struct ASTProgram {
    AST super;
    Vector *stmts;     // Vector<AST*>
    Map *symbols;      // Map<char*, Type*>
    Vector *classes;   // Vector<const struct ClassType*>
    Vector *functions; // Vector<const struct FuncType*>
    Map *compare;      // Map<Type**, Map<Type**, int>>
};

static void
json(const void *this, FILE *out, int indent) {
    const ASTProgram *ast = this;
    json_start(out, &indent);
    json_label("node", out);
    json_string("program", out, indent);
    json_comma(out, indent);
    json_label("statements", out);
    json_vector(ast->stmts, (JSON_VALUE_FUNC)json_AST, out, indent);
    json_end(out, &indent);
}

enum OPTYPE {
    PLUS = 1 << 0, MINUS = 1 << 1, TIMES = 1 << 2, DIVIDE = 1 << 3
};

struct Operator {
    enum OPTYPE type;
    const char *op;
    const char *assign_op;
} operators[] = {
    {
        PLUS,
        "+",
        "+="
    },
    {
        MINUS,
        "-",
        "-="
    },
    {
        TIMES,
        "*",
        "*="
    },
    {
        DIVIDE,
        "/",
        "/="
    }
};

struct Builtin {
    enum BUILTIN_TYPE type;
    char *name;
    char *ctype;
    char *fmt;
    enum OPTYPE operators;
    enum BUILTIN_TYPE casts;
} builtins[] = {
    {
        BUILTIN_INT,
        "int",
        "int64_t",
        "%\" PRId64 \"",
        PLUS | MINUS | TIMES | DIVIDE,
        BUILTIN_INT | BUILTIN_BOOL | BUILTIN_DOUBLE | BUILTIN_STRING
    },
    {
        BUILTIN_BOOL,
        "bool",
        "unsigned char",
        "%d",
        0,
        BUILTIN_INT | BUILTIN_BOOL | BUILTIN_DOUBLE | BUILTIN_STRING
    },
    {
        BUILTIN_DOUBLE,
        "double",
        "double",
        "%f",
        PLUS | MINUS | TIMES | DIVIDE,
        BUILTIN_INT | BUILTIN_BOOL | BUILTIN_DOUBLE | BUILTIN_STRING
    },
    {
        BUILTIN_STRING,
        "string",
        "char*",
        "%s",
        PLUS,
        BUILTIN_STRING
    },
};

static TypeCheckState
addBuiltins(Map *symbols, Vector *classes, Vector *functions, Map *compare) {
    TypeCheckState state = {
        symbols,
        NULL,
        NULL,
        NULL,
        classes,
        functions,
        {
            NULL
        },
        compare,
        NULL,
        NULL
    };
    YYLTYPE loc = {
        0
    };
    for (size_t i = 0; i < sizeof(builtins) / sizeof(*builtins); i++) {
        struct Builtin builtin = builtins[i];
        Vector *gen = Vector();
        Vector *supers = Vector();
        Vector *fields = Vector();
        Vector *ctors = Vector();
        Type *type = ClassType(loc, gen, supers, fields, ctors);
        type->init = 1;
        Map_put(state.symbols, builtin.name, strlen(builtin.name), type, NULL);
        struct ClassType *class = (struct ClassType *)type;
        class->name = safe_strdup(builtin.name);
        state.builtins[i] = class;
        Map_put(compare, &class, sizeof(class), Map(), NULL);
        char *msg;
        if (type->verify(type, &state, &msg)) {
            print_ICE(msg);
            exit(EXIT_FAILURE);
        }
    }
    // Iterate through builtins again to add their fields. Some fields rely
    // on pre-existing builtins for argument and return types, so the
    // original types need to be fully initialized first.
    for (size_t i = 0; i < sizeof(builtins) / sizeof(*builtins); i++) {
        struct Builtin builtin = builtins[i];
        const struct ClassType *class = state.builtins[i];
        for (size_t j = 0; j < sizeof(builtins) / sizeof(*builtins); j++) {
            struct Builtin cast = builtins[j];
            if (builtin.casts & cast.type) {
                Type *retType =
                    ObjectType(loc, safe_strdup(cast.name), Vector());
                retType->verify(retType, &state, NULL);
                Type *fieldType = FuncType(loc, Vector(), Vector(), retType);
                char *fieldName = "=>";
                AddSymbol(class->fieldTypes,
                    fieldName,
                    strlen(fieldName),
                    fieldType,
                    0,
                    &state,
                    NULL);
            }
        }
        for (size_t j = 0; j < sizeof(operators) / sizeof(*operators); j++) {
            if (builtin.operators & operators[j].type) {
                {
                    Type *retType =
                        ObjectType(loc, safe_strdup(builtin.name), Vector());
                    Type *argType =
                        ObjectType(loc, safe_strdup(builtin.name), Vector());
                    retType->verify(retType, &state, NULL);
                    argType->verify(argType, &state, NULL);
                    Vector *args = init_Vector(argType);
                    Type *fieldType = FuncType(loc, Vector(), args, retType);
                    //Vector_append(state.functions, fieldType);
                    Map_put(class->fieldTypes,
                        operators[j].op,
                        strlen(operators[j].op),
                        fieldType,
                        NULL);
                }
                {
                    Type *retType =
                        ObjectType(loc, safe_strdup(builtin.name), Vector());
                    Type *argType =
                        ObjectType(loc, safe_strdup(builtin.name), Vector());
                    retType->verify(retType, &state, NULL);
                    argType->verify(argType, &state, NULL);
                    Vector *args = init_Vector(argType);
                    Type *fieldType = FuncType(loc, Vector(), args, retType);
                    //Vector_append(state.functions, fieldType);
                    Map_put(class->fieldTypes,
                        operators[j].assign_op,
                        strlen(operators[j].assign_op),
                        fieldType,
                        NULL);
                }
            }
        }
        {
            Type *retType =
                ObjectType(loc, safe_strdup(builtin.name), Vector());
            Type *argType =
                ObjectType(loc, safe_strdup(builtin.name), Vector());
            retType->verify(retType, &state, NULL);
            argType->verify(argType, &state, NULL);
            Vector *args = init_Vector(argType);
            Type *fieldType = FuncType(loc, Vector(), args, retType);
            //Vector_append(state.functions, fieldType);
            char *fieldName = "==";
            Map_put(class->fieldTypes,
                fieldName,
                strlen(fieldName),
                fieldType,
                NULL);
        }
    }
    return state;
}

static void
json_typePtr(const Type **typePtr, UNUSED size_t size, FILE *out) {
    char *str = (*typePtr)->toString(*typePtr);
    json_label(str, out);
    free(str);
}

static void
json_compare(const Map *compare, FILE *out, int indent) {
    json_Map(compare,
        (JSON_KEY_FUNC)json_typePtr,
        (JSON_VALUE_FUNC)json_empty,
        out,
        indent);
}

static int
getType(void *this, UNUSED TypeCheckState *state, UNUSED Type **typeptr) {
    ASTProgram *ast = this;
    size_t n;
    int status = 0;

    TypeCheckState new_state =
        addBuiltins(ast->symbols, ast->classes, ast->functions, ast->compare);
    n = Vector_size(ast->stmts);
    for (size_t i = 0; i < n; i++) {
        AST *stmt = Vector_get(ast->stmts, i);
        Type *type;
        status = stmt->getType(stmt, &new_state, &type) || status;
    }
    if (!status) {
        fprintf(stdout, "Symbol Table:\n");
        json_Map(ast->symbols,
            (JSON_KEY_FUNC)json_nlabel,
            (JSON_VALUE_FUNC)json_type,
            stdout,
            0);
        fprintf(stdout, "\n");
        fprintf(stdout, "Class Hierarchy:\n");
        json_Map(ast->compare,
            (JSON_KEY_FUNC)json_typePtr,
            (JSON_VALUE_FUNC)json_compare,
            stdout,
            0);
        fprintf(stdout, "\n");
        fprintf(stdout, "Functions:\n");
        json_vector(ast->functions, (JSON_VALUE_FUNC)json_type, stdout, 0);
    }
    return status;
}

static char *
codeGen(void *this, FILE *out, UNUSED CodeGenState *state) {
    ASTProgram *ast = this;
    size_t n;
    CodeGenState newState = (CodeGenState){
        0,
        0,
        0,
        Map()
    };
    state = &newState;

    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <stdlib.h>\n");
    fprintf(out, "#include <string.h>\n");
    fprintf(out, "#include <inttypes.h>\n");
    fprintf(out, "\n");
    fprintf(out, "#define ERROR(msg) { \\\n");
    state->indent++;
    fprintf(out, "%*s", state->indent * 4, "");
    fprintf(out, "perror(msg); \\\n");
    fprintf(out, "%*s", state->indent * 4, "");
    fprintf(out, "exit(EXIT_FAILURE); \\\n");
    state->indent--;
    fprintf(out, "}\n");

    fprintf(out, "#define CALL(closure, args) closure.fn(closure, args)\n");
    fprintf(out, "\n");

    fprintf(out, "struct closure;\n");
    fprintf(out, "\n");

    fprintf(out, "typedef struct closure closure;\n");
    fprintf(out, "typedef void *FUNC(closure env, void **args);\n");
    fprintf(out, "\n");

    fprintf(out, "struct closure {\n");
    state->indent++;
    fprintf(out, "%*s", state->indent * 4, "");
    fprintf(out, "FUNC *fn;\n");
    fprintf(out, "%*s", state->indent * 4, "");
    fprintf(out, "void **env;\n");
    state->indent--;
    fprintf(out, "};\n");
    fprintf(out, "\n");

    n = Vector_size(ast->functions);
    if (n > 0) {
        fprintf(out, "FUNC ");
    }
    char *sep = "";
    for (size_t i = 0; i < n; i++) {
        const struct FuncType *func = Vector_get(ast->functions, i);
        char *name = safe_asprintf("func%d", state->funcCount);
        state->funcCount++;
        Map_put(state->funcIDs, &func, sizeof(func), name, NULL);
        fprintf(out, "%s%s", sep, name);
        sep = ", ";
    }
    if (n > 0) {
        fprintf(out, ";\n");
        fprintf(out, "\n");
    }

    for (size_t i = 0; i < sizeof(builtins) / sizeof(*builtins); i++) {
        struct Builtin builtin = builtins[i];
        fprintf(out, "struct class_%s *\n", builtin.name);
        fprintf(out, "builtin_%s(%s val);\n", builtin.name, builtin.ctype);
        fprintf(out, "\n");
    }
    for (size_t i = 0; i < sizeof(builtins) / sizeof(*builtins); i++) {
        struct Builtin builtin = builtins[i];
        fprintf(out, "typedef struct class_%s {\n", builtin.name);
        state->indent++;
        fprintf(out, "%*s", state->indent * 4, "");
        fprintf(out, "%s val;\n", builtin.ctype);
        size_t opCount = sizeof(operators) / sizeof(*operators);
        for (size_t j = 0; j < opCount; j++) {
            if (builtin.operators & operators[j].type) {
                char op[strlen(operators[j].op) * 2 + 1];
                strident(operators[j].op, op);
                fprintf(out, "%*s", state->indent * 4, "");
                fprintf(out,
                    "void *(*field_%s)(closure env, void **args);\n",
                    op);

                char assign_op[strlen(operators[j].assign_op) * 2 + 1];
                strident(operators[j].assign_op, assign_op);
                fprintf(out, "%*s", state->indent * 4, "");
                fprintf(out,
                    "void *(*field_%s)(closure env, void **args);\n",
                    assign_op);
            }
        }
        for (size_t j = 0; j < sizeof(builtins) / sizeof(*builtins); j++) {
            struct Builtin cast = builtins[j];
            if (builtin.casts & cast.type) {
                fprintf(out, "%*s", state->indent * 4, "");
                fprintf(out,
                    "void *(*cast_class_%s)(closure env, void **args);\n",
                    cast.name);
            }
        }
        state->indent--;
        fprintf(out, "} *class_%s;\n", builtin.name);
        fprintf(out, "\n");
        if (builtin.type == BUILTIN_STRING) {
            char assign_op[strlen("+=") * 2 + 1];
            strident("+=", assign_op);
            fprintf(out, "void *\n");
            fprintf(out,
                "class_%s_field_%s(closure env, void **args) {\n",
                builtin.name,
                assign_op);
            state->indent++;
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "class_%s this = env.env[0];\n", builtin.name);
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "class_%s other = args[0];\n", builtin.name);
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "size_t size = strlen(this->val);\n");
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out,
                "if (NULL == (this->val = realloc(this->val, size + strlen"
                "(other->val) + 1))) {\n");
            state->indent++;
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "ERROR(\"realloc\");\n");
            state->indent--;
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "}\n");
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "strcpy(this->val + size, other->val);\n");
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "return this;\n");
            state->indent--;
            fprintf(out, "}\n");
            fprintf(out, "\n");

            char op[strlen("+") * 2 + 1];
            strident("+", op);
            fprintf(out, "void *\n");
            fprintf(out,
                "class_%s_field_%s(closure env, void **args) {\n",
                builtin.name,
                op);
            state->indent++;
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "class_%s this = env.env[0];\n", builtin.name);
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "class_%s other = args[0];\n", builtin.name);
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "size_t size1 = strlen(this->val),\n");
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "       size2 = strlen(other->val);\n");
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "char *val;\n");
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "if (NULL == (val = malloc(size1 + size2 + 1))) {\n");
            state->indent++;
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "ERROR(\"malloc\");\n");
            state->indent--;
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "}\n");
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "strcpy(val, this->val);\n");
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "strcpy(val + size1, other->val);\n");
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "return builtin_string(val);\n");
            state->indent--;
            fprintf(out, "}\n");
            fprintf(out, "\n");

            fprintf(out, "void *\n");
            fprintf(out,
                "class_%s_cast_class_string(closure env, void **args) {\n",
                builtin.name);
            state->indent++;
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "class_%s this = env.env[0];\n", builtin.name);
            fprintf(out, "%*s", state->indent * 4, "");
            fprintf(out, "return builtin_string(this->val);\n");
            state->indent--;
            fprintf(out, "}\n");
            fprintf(out, "\n");
        } else {
            for (size_t j = 0; j < opCount; j++) {
                if (builtin.operators & operators[j].type) {
                    char assign_op[strlen(operators[j].assign_op) * 2 + 1];
                    strident(operators[j].assign_op, assign_op);
                    fprintf(out, "void *\n");
                    fprintf(out,
                        "class_%s_field_%s(closure env, void **args) {\n",
                        builtin.name,
                        assign_op);
                    state->indent++;
                    fprintf(out, "%*s", state->indent * 4, "");
                    fprintf(out,
                        "class_%s this = env.env[0];\n",
                        builtin.name);
                    fprintf(out, "%*s", state->indent * 4, "");
                    fprintf(out, "class_%s other = args[0];\n", builtin.name);
                    fprintf(out, "%*s", state->indent * 4, "");
                    fprintf(out,
                        "this->val %s other->val;\n",
                        operators[j].assign_op);
                    fprintf(out, "%*s", state->indent * 4, "");
                    fprintf(out, "return this;\n");
                    state->indent--;
                    fprintf(out, "}\n");
                    fprintf(out, "\n");

                    char op[strlen(operators[j].op) * 2 + 1];
                    strident(operators[j].op, op);
                    fprintf(out, "void *\n");
                    fprintf(out,
                        "class_%s_field_%s(closure env, void **args) {\n",
                        builtin.name,
                        op);
                    state->indent++;
                    fprintf(out, "%*s", state->indent * 4, "");
                    fprintf(out,
                        "class_%s this = env.env[0];\n",
                        builtin.name);
                    fprintf(out, "%*s", state->indent * 4, "");
                    fprintf(out, "class_%s other = args[0];\n", builtin.name);
                    fprintf(out, "%*s", state->indent * 4, "");
                    fprintf(out,
                        "return builtin_%s(this->val %s other->val);\n",
                        builtin.name,
                        operators[j].op);
                    state->indent--;
                    fprintf(out, "}\n");
                    fprintf(out, "\n");
                }
            }
            for (size_t j = 0; j < sizeof(builtins) / sizeof(*builtins); j++) {
                struct Builtin cast = builtins[j];
                if (builtin.casts & cast.type) {
                    if (cast.type == BUILTIN_STRING) {
                        fprintf(out, "%*s", state->indent * 4, "");
                        fprintf(out, "void *\n");
                        fprintf(out, "%*s", state->indent * 4, "");
                        fprintf(out,
                            "class_%s_cast_class_%s(closure env, void **args) "
                            "{\n",
                            builtin.name,
                            cast.name);
                        state->indent++;
                        fprintf(out, "%*s", state->indent * 4, "");
                        fprintf(out,
                            "class_%s this = env.env[0];\n",
                            builtin.name);
                        fprintf(out, "%*s", state->indent * 4, "");
                        fprintf(out,
                            "size_t size = snprintf(NULL, 0, \"%s\", this->val);"
                            "\n",
                            builtin.fmt);
                        fprintf(out, "%*s", state->indent * 4, "");
                        fprintf(out, "char *val;\n");
                        fprintf(out, "%*s", state->indent * 4, "");
                        fprintf(out,
                            "if (NULL == (val = malloc(size + 1))) {\n");
                        state->indent++;
                        fprintf(out, "%*s", state->indent * 4, "");
                        fprintf(out, "ERROR(\"malloc\");\n");
                        state->indent--;
                        fprintf(out, "%*s", state->indent * 4, "");
                        fprintf(out, "}\n");
                        fprintf(out, "%*s", state->indent * 4, "");
                        fprintf(out,
                            "sprintf(val, \"%s\", this->val);\n",
                            builtin.fmt);
                        fprintf(out, "%*s", state->indent * 4, "");
                        fprintf(out, "return builtin_string(val);\n");
                        state->indent--;
                        fprintf(out, "}\n");
                        fprintf(out, "\n");
                    } else {
                        fprintf(out, "%*s", state->indent * 4, "");
                        fprintf(out, "void *\n");
                        fprintf(out, "%*s", state->indent * 4, "");
                        fprintf(out,
                            "class_%s_cast_class_%s(closure env, void "
                            "**args) {\n",
                            builtin.name,
                            cast.name);
                        state->indent++;
                        fprintf(out, "%*s", state->indent * 4, "");
                        fprintf(out,
                            "class_%s this = env.env[0];\n",
                            builtin.name);
                        fprintf(out, "%*s", state->indent * 4, "");
                        fprintf(out,
                            "return builtin_%s((%s)this->val);\n",
                            cast.name,
                            cast.ctype);
                        state->indent--;
                        fprintf(out, "}\n");
                        fprintf(out, "\n");
                    }
                }
            }
        }
        fprintf(out, "class_%s\n", builtin.name);
        fprintf(out, "builtin_%s(%s val) {\n", builtin.name, builtin.ctype);
        state->indent++;
        fprintf(out, "%*s", state->indent * 4, "");
        fprintf(out, "class_%s ret;\n", builtin.name);
        fprintf(out, "%*s", state->indent * 4, "");
        fprintf(out, "if (NULL == (ret = malloc(sizeof(*ret)))) {\n");
        state->indent++;
        fprintf(out, "%*s", state->indent * 4, "");
        fprintf(out, "ERROR(\"malloc\");\n");
        state->indent--;
        fprintf(out, "%*s", state->indent * 4, "");
        fprintf(out, "}\n");
        fprintf(out, "%*s", state->indent * 4, "");
        fprintf(out, "*ret = (struct class_%s) {\n", builtin.name);
        state->indent++;
        fprintf(out, "%*s", state->indent * 4, "");
        fprintf(out, "val,\n");
        for (size_t j = 0; j < opCount; j++) {
            if (builtin.operators & operators[j].type) {
                char op[strlen(operators[j].op) * 2 + 1];
                strident(operators[j].op, op);
                fprintf(out, "%*s", state->indent * 4, "");
                fprintf(out, "class_%s_field_%s,\n", builtin.name, op);

                char assign_op[strlen(operators[j].assign_op) * 2 + 1];
                strident(operators[j].assign_op, assign_op);
                fprintf(out, "%*s", state->indent * 4, "");
                fprintf(out, "class_%s_field_%s,\n", builtin.name, assign_op);
            }
        }
        for (size_t j = 0; j < sizeof(builtins) / sizeof(*builtins); j++) {
            struct Builtin cast = builtins[j];
            if (builtin.casts & cast.type) {
                fprintf(out, "%*s", state->indent * 4, "");
                fprintf(out,
                    "class_%s_cast_class_%s,\n",
                    builtin.name,
                    cast.name);
            }
        }
        state->indent--;
        fprintf(out, "%*s", state->indent * 4, "");
        fprintf(out, "};\n");
        fprintf(out, "%*s", state->indent * 4, "");
        fprintf(out, "return ret;\n");
        state->indent--;
        fprintf(out, "}\n");
        fprintf(out, "\n");

        fprintf(out, "void *\n");
        fprintf(out, "new_%s(closure env, void **args) {\n", builtin.name);
        state->indent++;
        fprintf(out, "%*s", state->indent * 4, "");
        fprintf(out, "return builtin_%s(0);\n", builtin.name);
        state->indent--;
        fprintf(out, "}\n");
        fprintf(out, "\n");
    }

    n = Vector_size(ast->functions);
    for (size_t i = 0; i < n; i++) {
        const struct FuncType *func = Vector_get(ast->functions, i);
        char *name;
        Map_get(state->funcIDs, &func, sizeof(func), &name);
        fprintf(out, "void *\n%s(closure env, void **args) {\n", name);
        state->indent++;
        codeGenFuncBody(func->ast, out, &newState);
        state->indent--;
        fprintf(out, "}\n");
        fprintf(out, "\n");
    }

    fprintf(out, "int\nmain(int argc, char *argv[]) {\n");
    state->indent++;
    Iterator *it = Map_iterator(ast->symbols);
    while (it->hasNext(it)) {
        MapIterData data = it->next(it);
        Type *type = data.value;
        char
            *name = safe_asprintf("var_%.*s", (int)data.len, (char *)data.key);
        char *typeName = type->codeGen(type, name);
        free(name);
        fprintf(out, "%*s", state->indent * 4, "");
        fprintf(out, "%s;\n", typeName);
        free(typeName);
    }
    it->delete(it);
    fprintf(out, "\n");
    for (size_t i = 0; i < sizeof(builtins) / sizeof(*builtins); i++) {
        struct Builtin builtin = builtins[i];
        fprintf(out, "%*s", state->indent * 4, "");
        fprintf(out,
            "var_%s = (closure){ new_%s, NULL };\n",
            builtin.name,
            builtin.name);
    }
    n = Vector_size(ast->stmts);
    for (size_t i = 0; i < n; i++) {
        AST *stmt = Vector_get(ast->stmts, i);
        char *code = stmt->codeGen(stmt, out, &newState);
        free(code);
    }
    state->indent--;
    fprintf(out, "}\n");
    delete_Map(state->funcIDs, free);
    return NULL;
}

static void
delete_compare(Map *compare) {
    delete_Map(compare, NULL);
}

static void
delete(void *this) {
    ASTProgram *ast = this;
    delete_Vector(ast->stmts, (VEC_DELETE_FUNC)delete_AST);
    delete_Map(ast->symbols, (MAP_DELETE_FUNC)delete_type);
    delete_Map(ast->compare, (MAP_DELETE_FUNC)delete_compare);
    delete_Vector(ast->classes, (VEC_DELETE_FUNC)delete_ClassType);
    delete_Vector(ast->functions, NULL);
    free(this);
}

AST *
new_ASTProgram(YYLTYPE loc, Vector *stmts) {
    ASTProgram *program = NULL;
    Map *symbols, *compare;
    Vector *classes, *functions;

    program = safe_malloc(sizeof(*program));
    symbols = Map();
    classes = Vector();
    functions = Vector();
    compare = Map();
    *program = (ASTProgram){
        {
            json,
            getType,
            codeGen,
            delete,
            loc,
            NULL
        },
        stmts,
        symbols,
        classes,
        functions,
        compare
    };
    return (AST *)program;
}
