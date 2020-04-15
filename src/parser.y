%{
    #include "parser.h"
    #include "scanner.h"
    #include "util.h"
    #define YYERROR_VERBOSE

    // Uncomment the following line to enable Bison debug tracing
    // int yydebug = 1;

    void
    yyerror(YYLTYPE *locp,
        AST **root,
        const char *filename,
        yyscan_t scanner,
        const char *msg);
%}

%code provides{
    #define YY_DECL int yylex (YYSTYPE *yylval_param, \
        YYLTYPE *yylloc_param, \
        UNUSED const char *filename, \
        yyscan_t yyscanner)
    YY_DECL;
}

%code requires{
    #include "ast.h"
    #include "vector.h"
    #include "sparse_vector.h"
    #include "util.h"
    #include "types.h"
    #include "safe.h"
    #include "dynamic_string.h"

    # define YYLLOC_DEFAULT(Cur, Rhs, N)                            \
        do {                                                        \
          if (N)                                                    \
            {                                                       \
              (Cur).first_line   = YYRHSLOC(Rhs, 1).first_line;     \
              (Cur).first_column = YYRHSLOC(Rhs, 1).first_column;   \
              (Cur).last_line    = YYRHSLOC(Rhs, N).last_line;      \
              (Cur).last_column  = YYRHSLOC(Rhs, N).last_column;    \
            }                                                       \
          else                                                      \
            {                                                       \
              (Cur).first_line   = (Cur).last_line   =              \
                YYRHSLOC(Rhs, 0).last_line;                         \
              (Cur).first_column = (Cur).last_column =              \
                YYRHSLOC(Rhs, 0).last_column;                       \
            }                                                       \
          (Cur).filename = YYRHSLOC(Rhs, 1).filename;               \
        } while (0)

    #ifndef YY_TYPEDEF_YY_SCANNER_T
    #define YY_TYPEDEF_YY_SCANNER_T
    typedef void* yyscan_t;
    #endif
}

%define api.pure full
%define parse.error verbose
%define parse.trace
%locations
%parse-param { AST **root }
%param { const char *filename } { yyscan_t scanner }

%initial-action {
    @$ = (YYLTYPE) {
        1, 1, 1, 1, filename
    };
}

%union {
    AST *ast;
    Vector *vec;
    struct Field *field;
    struct ClassBody class;
    struct Case *switchCase;
    SparseVector *svec;
    char *str;
    dstring dstr;
    long long int int_lit;
    double double_lit;
    Type *type;
    Qualifiers qualifier;
}

%token             T_CLASS      "class"
                   T_FUNC       "func"
                   T_IMPL       "impl"
                   T_NEW        "new"
                   T_RETURN     "return"
                   T_NONE       "none"
                   T_CONST      "const"
                   T_FRIEND     "friend"
                   T_TRUE       "true"
                   T_FALSE      "false"
                   T_MAYBE      "maybe"
                   T_IF         "if"
                   T_ELSE       "else"
                   T_WHILE      "while"
                   T_DO         "do"
                   T_IS         "is"
                   T_NOT        "not"
                   T_SWITCH     "switch"
                   T_CASE       "case"
                   T_DEFAULT    "default"
                   T_OPERATOR   "operator"
                   T_REF        "ref"
                   T_ARROW      "=>"
                   T_MUL_ASSIGN "*="
                   T_DIV_ASSIGN "/="
                   T_MOD_ASSIGN "%="
                   T_ADD_ASSIGN "+="
                   T_SUB_ASSIGN "-="
                   T_OR         "||"
                   T_AND        "&&"
                   T_EQ         "=="
                   T_NE         "!="
                   T_LE         "<="
                   T_GE         ">="
                   T_INC        "++"
                   T_DEC        "--"
                   T_ERROR      "lexing error"
                   END 0        "end of file"
%token<str>        T_IDENT      "identifier"
%token<dstr>       T_STRING     "string literal"
%token<int_lit>    T_INT        "int literal"
                   T_RANGE      ".."
                   T_INDEX      "index operator"
%token<double_lit> T_DOUBLE     "double literal"

%type<ast>  File Statement Definition Expression Return OptExpression Func
            Init PrimaryExpr PostfixExpr UnaryExpr OpExpr TypeStmt Impl If While
            Switch Do CastExpr
%type<vec>  OptStatements Statements IdentList OptInherits Inherits OptNamedArgs
            NamedArgs OptArgsOptNamed ArgsOptNamed DefVars
            Constructor OptArguments Arguments OptElse OptCases Cases OptDefault
%type<svec> Types Tuple
%type<type> Type TypeDef FuncDef ClassDef TypeOptNamed
%type<class> Fields OptFields
%type<switchCase> Case
%type<field> Field NamedArg Operator
%type<qualifier> Qualifier Qualifiers
%type<str> Binop Postop

%left T_AND T_OR
%left T_EQ T_NE
%left '<' '>' T_LE T_GE
%left '+' '-'
%left '*' '/' '%'

%start File

%%

File
  : OptStatements
    {
        *root = ASTProgram(@$, $1);
    }

OptStatements
  : %empty {
        $$ = Vector();
    }
  | Statements

Statements
  : Statement {
        $$ = init_Vector($1);
    }
  | Statements Statement {
        $$ = Vector_append($1, $2);
    }

Statement
  : Definition
  | If
  | While
  | Do
  | Switch
  | Impl
  | TypeStmt ';'
  | Expression ';'
  | Return ';'

Definition
  : DefVars '=' Expression ';' {
        $$ = ASTDefinition(@$, $1, $3);
    }

DefVars
  : T_IDENT {
        $$ = init_Vector($1);
    }
  | '_' {
        $$ = init_Vector(NULL);
    }
  | DefVars ',' T_IDENT {
        $$ = Vector_append($1, $3);
    }
  | DefVars ',' '_' {
        $$ = Vector_append($1, NULL);
    }

If
  : T_IF Expression '{' OptStatements '}' OptElse {
        $$ = ASTIf(@$, $2, $4, $6);
    }

OptElse
  : %empty {
        $$ = Vector();
    }
  | T_ELSE If {
        $$ = init_Vector($2);
    }
  | T_ELSE '{' OptStatements '}' {
        $$ = $3;
    }

While
  : T_WHILE Expression '{' OptStatements '}' {
        $$ = ASTWhile(@$, $2, $4);
    }

Do
  : T_DO '{' OptStatements '}' T_WHILE Expression ';' {
        $$ = ASTDo(@$, $6, $3);
    }

Switch
  : T_SWITCH Expression '{' OptCases OptDefault '}' {
        $$ = ASTSwitch(@$, $2, $4, $5);
    }

OptCases
  : %empty {
        $$ = Vector();
    }
  | Cases

Cases
  : Case {
        $$ = init_Vector($1);
    }
  | Cases Case {
        $$ = Vector_append($1, $2);
    }

Case
  : T_CASE Expression '{' OptStatements '}' {
        $$ = ExprCase($2, $4);
    }
  | T_CASE T_IDENT T_IS Type '{' OptStatements '}' {
        $$ = TypeCase($2, $4, $6);
    }

OptDefault
  : %empty {
        $$ = NULL;
    }
  | T_DEFAULT '{' OptStatements '}' {
        $$ = $3;
    }

TypeStmt
  : DefVars ':' Type {
        $$ = ASTTypeStmt(@$, $1, $3);
    }

Return
  : T_RETURN OptExpression {
        $$ = ASTReturn(@$, $2);
    }

OptExpression
  : %empty {
        $$ = NULL;
    }
  | Expression

PrimaryExpr
  : T_IDENT {
        $$ = ASTVariable(@$, $1);
    }
  | T_INT {
        $$ = ASTInt(@$, $1);
    }
  | T_DOUBLE {
        $$ = ASTDouble(@$, $1);
    }
  | T_STRING {
        $$ = ASTString(@$, $1);
    }
  | T_TRUE {
        $$ = ASTBool(@$, 1);
    }
  | T_FALSE {
        $$ = ASTBool(@$, 0);
    }
  | '(' Expression ')' {
        $$ = $2;
    }
  | '(' Tuple ')' {
        $$ = ASTTuple(@$, $2);
    }
  | Init
  | Func

PostfixExpr
  : PrimaryExpr
  | PostfixExpr '.' T_IDENT {
        $$ = ASTMember(@$, $1, $3);
    }
  | PostfixExpr '(' OptArguments ')' {
        $$ = ASTCall(@$, $1, $3);
    }
  | PostfixExpr T_INDEX {
        $$ = ASTConstIndex(@$, $1, $2);
    }
  | PostfixExpr '[' Expression ']' {
        $$ = ASTIndex(@$, $1, $3);
    }

UnaryExpr
  : PostfixExpr
  | T_INC PostfixExpr {
        Vector *args = init_Vector($2);
        char *name = safe_strdup("++");
        AST *func = ASTVariable(@$, name);
        $$ = ASTCall(@$, func, args);
    }
  | T_DEC PostfixExpr {
        Vector *args = init_Vector($2);
        char *name = safe_strdup("--");
        AST *func = ASTVariable(@$, name);
        $$ = ASTCall(@$, func, args);
    }
  | '-' PostfixExpr {
        Vector *args = init_Vector($2);
        char *name = safe_strdup("-");
        AST *func = ASTVariable(@$, name);
        $$ = ASTCall(@$, func, args);
    }
  | '!' PostfixExpr {
        Vector *args = init_Vector($2);
        char *name = safe_strdup("!");
        AST *func = ASTVariable(@$, name);
        $$ = ASTCall(@$, func, args);
    }
  | '*' PostfixExpr {
        $$ = ASTSpread(@$, $2);
    }

CastExpr
  : UnaryExpr
  | CastExpr T_ARROW Type {
        $$ = ASTCast(@$, $1, $3);
    }

OpExpr
  : CastExpr
  | OpExpr '*' OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("*");
        AST *method = ASTMember(@$, $1, name);
        $$ = ASTCall(@$, method, args);
    }
  | OpExpr '/' OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("/");
        AST *method = ASTMember(@$, $1, name);
        $$ = ASTCall(@$, method, args);
    }
  | OpExpr '%' OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("%");
        AST *method = ASTMember(@$, $1, name);
        $$ = ASTCall(@$, method, args);
    }
  | OpExpr '+' OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("+");
        AST *method = ASTMember(@$, $1, name);
        $$ = ASTCall(@$, method, args);
    }
  | OpExpr '-' OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("-");
        AST *method = ASTMember(@$, $1, name);
        $$ = ASTCall(@$, method, args);
    }
  | OpExpr '<' OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("<");
        AST *method = ASTMember(@$, $1, name);
        $$ = ASTCall(@$, method, args);
    }
  | OpExpr '>' OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup(">");
        AST *method = ASTMember(@$, $1, name);
        $$ = ASTCall(@$, method, args);
    }
  | OpExpr T_LE OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("<=");
        AST *method = ASTMember(@$, $1, name);
        $$ = ASTCall(@$, method, args);
    }
  | OpExpr T_GE OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup(">=");
        AST *method = ASTMember(@$, $1, name);
        $$ = ASTCall(@$, method, args);
    }
  | OpExpr T_EQ OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("==");
        AST *method = ASTMember(@$, $1, name);
        $$ = ASTCall(@$, method, args);
    }
  | OpExpr T_NE OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("!=");
        AST *method = ASTMember(@$, $1, name);
        $$ = ASTCall(@$, method, args);
    }
  | OpExpr T_AND OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("&&");
        AST *method = ASTMember(@$, $1, name);
        $$ = ASTCall(@$, method, args);
    }
  | OpExpr T_OR OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("||");
        AST *method = ASTMember(@$, $1, name);
        $$ = ASTCall(@$, method, args);
    }

Tuple
  : OpExpr T_RANGE {
        $$ = init_SparseVector($1, $2);
    }
  | OpExpr ',' OpExpr {
        $$ = SparseVector_append(init_SparseVector($1, 1), $3, 1);
    }
  | OpExpr ',' OpExpr T_RANGE {
        $$ = SparseVector_append(init_SparseVector($1, 1), $3, $4);
    }
  | Tuple ',' OpExpr {
        $$ = SparseVector_append($1, $3, 1);
    }
  | Tuple ',' OpExpr T_RANGE {
        $$ = SparseVector_append($1, $3, $4);
    }

Expression
  : OpExpr
  | PostfixExpr T_MUL_ASSIGN Expression {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("*=");
        AST *method = ASTMember(@$, $1, name);
        $$ = ASTCall(@$, method, args);
    }
  | PostfixExpr T_DIV_ASSIGN Expression {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("/=");
        AST *method = ASTMember(@$, $1, name);
        $$ = ASTCall(@$, method, args);
    }
  | PostfixExpr T_MOD_ASSIGN Expression {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("%=");
        AST *method = ASTMember(@$, $1, name);
        $$ = ASTCall(@$, method, args);
    }
  | PostfixExpr T_ADD_ASSIGN Expression {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("+=");
        AST *method = ASTMember(@$, $1, name);
        $$ = ASTCall(@$, method, args);
    }
  | PostfixExpr T_SUB_ASSIGN Expression {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("-=");
        AST *method = ASTMember(@$, $1, name);
        $$ = ASTCall(@$, method, args);
    }

ClassDef
  : T_CLASS OptInherits '{' OptFields '}' {
        $$ = ClassType(@$, Vector(), $2, $4.fields, $4.ctors);
    }
  | '<' IdentList '>' T_CLASS OptInherits '{' OptFields '}' {
        $$ = ClassType(@$, $2, $5, $7.fields, $7.ctors);
    }

OptFields
  : %empty {
        $$.fields = Vector();
        $$.ctors = Vector();
    }
  | Fields

Fields
  : Field ';' {
        $$.fields = init_Vector($1);
        $$.ctors = Vector();
    }
  | Constructor ';' {
        $$.fields = Vector();
        $$.ctors = init_Vector($1);
    }
  | Operator ';' {
        $$.fields = init_Vector($1);
        $$.ctors = Vector();
    }
  | Fields Field ';' {
        $$ = $1;
        Vector_append($$.fields, $2);
    }
  | Fields Constructor ';' {
        $$ = $1;
        Vector_append($$.ctors, $2);
    }
  | Fields Operator ';' {
        $$ = $1;
        Vector_append($$.fields, $2);
    }

Field
  : IdentList ':' Type {
        $$ = safe_malloc(sizeof(*$$));
        $$->names = $1;
        $$->type = $3;
    }

Operator
  : T_OPERATOR Binop '(' Type ')' T_ARROW Type {
        $$ = safe_malloc(sizeof(*$$));
        $$->names = init_Vector($2);
        $$->type = FuncType(@$, Vector(), init_Vector($4), $7);
    }
  | T_OPERATOR Binop '(' T_IDENT ':' Type ')' T_ARROW Type {
        $$ = safe_malloc(sizeof(*$$));
        $$->names = init_Vector($2);
        $$->type = FuncType(@$, Vector(), init_Vector($6), $9);
    }
  | T_OPERATOR Postop T_ARROW Type {
        $$ = safe_malloc(sizeof(*$$));
        $$->names = init_Vector($2);
        $$->type = FuncType(@$, Vector(), Vector(), $4);
    }
  | T_OPERATOR T_ARROW Type {
        $$ = safe_malloc(sizeof(*$$));
        $$->names = init_Vector(safe_strdup("=>"));
        $$->type = FuncType(@$, Vector(), Vector(), $3);
    }

Binop
  : '*' {
        $$ = safe_strdup("*");
    }
  | '/' {
        $$ = safe_strdup("/");
    }
  | '%' {
        $$ = safe_strdup("%");
    }
  | '+' {
        $$ = safe_strdup("+");
    }
  | '-' {
        $$ = safe_strdup("-");
    }
  | '<' {
        $$ = safe_strdup("<");
    }
  | '>' {
        $$ = safe_strdup(">");
    }
  | T_MUL_ASSIGN {
        $$ = safe_strdup("*=");
    }
  | T_DIV_ASSIGN {
        $$ = safe_strdup("/=");
    }
  | T_MOD_ASSIGN {
        $$ = safe_strdup("%=");
    }
  | T_ADD_ASSIGN {
        $$ = safe_strdup("+=");
    }
  | T_SUB_ASSIGN {
        $$ = safe_strdup("-=");
    }
  | T_OR {
        $$ = safe_strdup("||");
    }
  | T_AND {
        $$ = safe_strdup("&&");
    }
  | T_EQ {
        $$ = safe_strdup("==");
    }
  | T_NE {
        $$ = safe_strdup("!=");
    }
  | T_LE {
        $$ = safe_strdup("<=");
    }
  | T_GE {
        $$ = safe_strdup(">=");
    }

Postop
  : T_INC {
        $$ = safe_strdup("++");
    }
  | T_DEC {
        $$ = safe_strdup("++");
    }

Constructor
  : T_NEW '(' OptArgsOptNamed ')' {
        $$ = $3;
    }

Impl
  : T_IMPL T_IDENT '{' OptStatements '}' {
        $$ = ASTImpl(@$, $2, Vector(), $4);
    }
  | T_IMPL '<' IdentList '>' T_IDENT '{' OptStatements '}' {
        $$ = ASTImpl(@$, $5, $3, $7);
    }

OptInherits
  : %empty {
        $$ = Vector();
    }
  | ':' Inherits {
        $$ = $2;
    }

Inherits
  : Type {
        $$ = init_Vector($1);
    }
  | Inherits ',' Type {
        $$ = Vector_append($1, $3);
    }

Type
  : TypeDef
  | Qualifiers TypeDef {
        $$ = $2;
        $$->qualifiers = $1;
    }

TypeDef
  : T_IDENT {
        $$ = ObjectType(@$, $1, Vector());
    }
  | '<' IdentList '>' T_IDENT {
        $$ = ObjectType(@$, $4, $2);
    }
  | T_NONE {
        $$ = NoneType(@$);
    }
  | FuncDef
  | ClassDef
  | '(' Type ')' {
        $$ = $2;
    }
  | '(' Types ')' {
        $$ = TupleType(@$, $2);
    }
  |  '[' ']' Type {
        $$ = ArrayType(@$, $3);
    }

Types
  : Type T_RANGE {
        $$ = init_SparseVector($1, $2);
    }
  | Type ',' Type {
        $$ = SparseVector_append(init_SparseVector($1, 1), $3, 1);
    }
  | Type ',' Type T_RANGE {
        $$ = SparseVector_append(init_SparseVector($1, 1), $3, $4);
    }
  | Types ',' Type {
        $$ = SparseVector_append($1, $3, 1);
    }
  | Types ',' Type T_RANGE {
        $$ = SparseVector_append($1, $3, $4);
    }

Qualifiers
  : Qualifier
  | Qualifiers Qualifier {
        $$ = $1 | $2;
    }

Qualifier
  : T_CONST {
        $$ = Q_CONST;
    }
  | T_FRIEND {
        $$ = Q_FRIEND;
    }
  | T_MAYBE {
        $$ = Q_MAYBE;
    }

FuncDef
  : '<' IdentList '>' T_FUNC '(' OptArgsOptNamed ')' T_ARROW Type {
        $$ = FuncType(@$, $2, $6, $9);
    }
  | T_FUNC '(' OptArgsOptNamed ')' T_ARROW Type {
        $$ = FuncType(@$, Vector(), $3, $6);
    }

OptArgsOptNamed
  : %empty {
        $$ = Vector();
    }
  | ArgsOptNamed

ArgsOptNamed
  : TypeOptNamed {
        $$ = init_Vector($1);
    }
  | ArgsOptNamed ',' TypeOptNamed {
        $$ = Vector_append($1, $3);
    }

TypeOptNamed
  : T_IDENT ':' Type {
        $$ = $3;
        free($1);
    }
  | T_IDENT ':' T_REF Type {
        $$ = $4;
        $$->isRef = 1;
    }
  | Type
  | T_REF Type {
        $$ = $2;
        $$->isRef = 1;
    }

Func
  : '<' IdentList '>' T_FUNC '(' OptNamedArgs ')' T_ARROW Type '{' OptStatements '}' {
        $$ = ASTFunc(@$, $2, $6, $9, $11);
    }
  | T_FUNC '(' OptNamedArgs ')' T_ARROW Type '{' OptStatements '}' {
        $$ = ASTFunc(@$, Vector(), $3, $6, $8);
    }

OptNamedArgs
  : %empty {
        $$ = Vector();
    }
  | NamedArgs

NamedArgs
  : NamedArg {
        $$ = init_Vector($1);
    }
  | NamedArgs ',' NamedArg {
        $$ = Vector_append($1, $3);
    }

NamedArg
  : IdentList ':' Type {
        $$ = safe_malloc(sizeof(*$$));
        $$->names = $1;
        $$->type = $3;
    }
  | IdentList ':' T_REF Type {
        $$ = safe_malloc(sizeof(*$$));
        $$->names = $1;
        $$->type = $4;
        $$->type->isRef = 1;
    }

Init
  : T_NEW T_IDENT '(' OptArguments ')' {
        $$ = ASTInit(@$, $2, Vector(), $4);
    }
  | T_NEW '<' IdentList '>' T_IDENT '(' OptArguments ')' {
        $$ = ASTInit(@$, $5, $3, $7);
    }
  | T_NEW Type T_INDEX {
        $$ = ASTArray(@$, $2, $3);
    }

OptArguments
  : %empty {
        $$ = Vector();
    }
  | Arguments

Arguments
  : Expression {
        $$ = init_Vector(Argument($1, 0));
    }
  | T_REF Expression {
        $$ = init_Vector(Argument($2, 1));
    }
  | Arguments ',' Expression {
        $$ = Vector_append($1, Argument($3, 0));
    }
  | Arguments ',' T_REF Expression {
        $$ = Vector_append($1, Argument($4, 1));
    }

IdentList
  : T_IDENT {
        $$ = init_Vector($1);
    }
  | IdentList ',' T_IDENT {
        $$ = Vector_append($1, $3);
    }

%%

void yyerror(YYLTYPE *locp,
    UNUSED AST **root,
    UNUSED const char *filename,
    UNUSED yyscan_t scanner,
    const char *msg
) {
    print_code_error(stderr, *locp, "%s", msg);
}
