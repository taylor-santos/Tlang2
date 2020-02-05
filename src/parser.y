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
    #include "util.h"
    #include "types.h"
    #include "safe.h"
    #include "dynamic_string.h"

    #ifndef YY_TYPEDEF_YY_SCANNER_T
    #define YY_TYPEDEF_YY_SCANNER_T
    typedef void* yyscan_t;
    #endif
}

%define api.pure full
%define parse.error verbose
%define parse.lac full
%define parse.trace
%locations
%parse-param { AST **root }
%param { const char *filename } { yyscan_t scanner }

%union {
    AST *ast;
    Vector *vec;
    char *str;
    dstring *dstr;
    long long int int_lit;
    double double_lit;
    Type *type;
    Qualifiers *qualifier;
}

%token             T_CLASS      "class"
                   T_FUNC       "func"
                   T_NEW        "new"
                   T_RETURN     "return"
                   T_CONST      "const"
                   T_FRIEND     "friend"
                   T_TRUE       "true"
                   T_FALSE      "false"
                   T_DEF        ":="
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
                   T_RANGE      ".."
                   T_ERROR      "lexing error"
                   END 0        "end of file"
%token<str>        T_IDENT      "identifier"
%token<dstr>       T_STRING     "string literal"
%token<int_lit>    T_INT        "int literal"
%token<double_lit> T_DOUBLE     "double literal"

%type<ast>  File Statement Definition Expression Class NamedType Return
            OptExpression Func TypeOptNamed Init PrimaryExpr PostfixExpr
            UnaryExpr OpExpr TypeStmt
%type<vec>  OptStatements Statements OptGenerics IdentList OptInherits Inherits
            OptNamedTypes NamedTypes OptNamedArgs NamedArgs OptArgsOptNamed
            ArgsOptNamed Qualifiers Types Tuple
%type<type> Type TypeDef FuncDef TupleType
%type<qualifier> Qualifier

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
        *root = ASTProgram(&@$, $1);
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
  : Definition ';'
  | TypeStmt ';'
  | Expression ';'
  | Return ';'

Definition
  : Expression T_DEF Expression {
        $$ = ASTDefinition(&@$, $1, $3);
    }

TypeStmt
  : T_IDENT ':' Type {
        $$ = ASTTypeStmt(&@$, $1, $3);
    }

Return
  : T_RETURN OptExpression {
        $$ = ASTReturn(&@$, $2);
    }

OptExpression
  : %empty {
        $$ = NULL;
    }
  | Expression

PrimaryExpr
  : T_IDENT {
        $$ = ASTVariable(&@$, $1);
    }
  | T_INT {
        $$ = ASTInt(&@$, $1);
    }
  | T_DOUBLE {
        $$ = ASTDouble(&@$, $1);
    }
  | T_STRING {
        $$ = ASTString(&@$, $1);
    }
  | T_TRUE {
        $$ = ASTBool(&@$, 1);
    }
  | T_FALSE {
        $$ = ASTBool(&@$, 0);
    }
  | Class
  | Func
  | Init
  | '(' Expression ')' {
        $$ = $2;
    }

PostfixExpr
  : PrimaryExpr
  | PostfixExpr '.' T_IDENT {
        $$ = ASTMember(&@$, $1, $3);
    }
  | PostfixExpr '(' OptExpression ')' {
        $$ = ASTCall(&@$, $1, $3);
    }
  | PostfixExpr '[' Expression ']' {
        $$ = ASTIndex(&@$, $1, $3);
    }

UnaryExpr
  : PostfixExpr
  | T_INC PostfixExpr {
        Vector *args = init_Vector($2);
        char *name = safe_strdup("++");
        AST *func = ASTVariable(&@$, name);
        $$ = ASTCall(&@$, func, ASTTuple(&@$, args));
    }
  | T_DEC PostfixExpr {
        Vector *args = init_Vector($2);
        char *name = safe_strdup("--");
        AST *func = ASTVariable(&@$, name);
        $$ = ASTCall(&@$, func, ASTTuple(&@$, args));
     }
  | '-' PostfixExpr {
        Vector *args = init_Vector($2);
        char *name = safe_strdup("-");
        AST *func = ASTVariable(&@$, name);
        $$ = ASTCall(&@$, func, ASTTuple(&@$, args));
    }
  | '!' PostfixExpr {
        Vector *args = init_Vector($2);
        char *name = safe_strdup("!");
        AST *func = ASTVariable(&@$, name);
        $$ = ASTCall(&@$, func, ASTTuple(&@$, args));
    }

OpExpr
  : UnaryExpr
  | OpExpr '*' OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("*");
        AST *method = ASTMember(&@$, $1, name);
        $$ = ASTCall(&@$, method, ASTTuple(&@$, args));
    }
  | OpExpr '/' OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("/");
        AST *method = ASTMember(&@$, $1, name);
        $$ = ASTCall(&@$, method, ASTTuple(&@$, args));
    }
  | OpExpr '%' OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("%");
        AST *method = ASTMember(&@$, $1, name);
        $$ = ASTCall(&@$, method, ASTTuple(&@$, args));
    }
  | OpExpr '+' OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("+");
        AST *method = ASTMember(&@$, $1, name);
        $$ = ASTCall(&@$, method, ASTTuple(&@$, args));
    }
  | OpExpr '-' OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("-");
        AST *method = ASTMember(&@$, $1, name);
        $$ = ASTCall(&@$, method, ASTTuple(&@$, args));
    }
  | OpExpr '<' OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("<");
        AST *method = ASTMember(&@$, $1, name);
        $$ = ASTCall(&@$, method, ASTTuple(&@$, args));
    }
  | OpExpr '>' OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup(">");
        AST *method = ASTMember(&@$, $1, name);
        $$ = ASTCall(&@$, method, ASTTuple(&@$, args));
    }
  | OpExpr T_LE OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("<=");
        AST *method = ASTMember(&@$, $1, name);
        $$ = ASTCall(&@$, method, ASTTuple(&@$, args));
    }
  | OpExpr T_GE OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup(">=");
        AST *method = ASTMember(&@$, $1, name);
        $$ = ASTCall(&@$, method, ASTTuple(&@$, args));
    }
  | OpExpr T_EQ OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("==");
        AST *method = ASTMember(&@$, $1, name);
        $$ = ASTCall(&@$, method, ASTTuple(&@$, args));
    }
  | OpExpr T_NE OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("!=");
        AST *method = ASTMember(&@$, $1, name);
        $$ = ASTCall(&@$, method, ASTTuple(&@$, args));
    }
  | OpExpr T_AND OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("&&");
        AST *method = ASTMember(&@$, $1, name);
        $$ = ASTCall(&@$, method, ASTTuple(&@$, args));
    }
  | OpExpr T_OR OpExpr {
        Vector *args = init_Vector($3);
        char *name = safe_strdup("||");
        AST *method = ASTMember(&@$, $1, name);
        $$ = ASTCall(&@$, method, ASTTuple(&@$, args));
    }

Tuple
  : OpExpr {
        $$ = init_Vector($1);
    }
  | Tuple ',' OpExpr {
        $$ = Vector_append($1, $3);
    }

Expression
  : Tuple {
        $$ = ASTTuple(&@$, $1);
    }
  | UnaryExpr '=' Expression
  | UnaryExpr T_MUL_ASSIGN Expression
  | UnaryExpr T_DIV_ASSIGN Expression
  | UnaryExpr T_MOD_ASSIGN Expression
  | UnaryExpr T_ADD_ASSIGN Expression
  | UnaryExpr T_SUB_ASSIGN Expression

Class
  : T_CLASS OptGenerics OptInherits '{' OptNamedTypes '}' {
        $$ = ASTClass(&@$, $2, $3, $5);
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

OptNamedTypes
  : %empty {
        $$ = Vector();
    }
  | NamedTypes

NamedTypes
  : NamedType ';' {
        $$ = init_Vector($1);
    }
  | NamedTypes NamedType ';' {
        $$ = Vector_append($1, $2);
    }

NamedType
  : T_IDENT ':' Type {
        $$ = ASTNamedType(&@$, $1, $3);
    }

Type
  : TypeDef
  | Qualifiers TypeDef {
        $$ = $2;
        Type_setQualifiers($$, $1);
    }

TypeDef
  : T_IDENT OptGenerics {
        $$ = ClassType($1, $2);
    }
  | FuncDef
  | '[' Expression ']' OptGenerics {
        $$ = ExprType($2, $4);
    }
  | TupleType

TupleType
  : '(' Types ')' {
        $$ = TupleType($2);
    }

Types
  : Type {
        $$ = init_Vector($1);
    }
  | Types ',' Type {
        $$ = Vector_append($1, $3);
  }

Qualifiers
  : Qualifier {
        $$ = init_Vector($1);
    }
  | Qualifiers Qualifier {
        $$ = Vector_append($1, $2);
    }

Qualifier
  : T_CONST {
        $$ = safe_malloc(sizeof(*$$));
        *$$ = CONST;
    }
  | T_FRIEND {
        $$ = safe_malloc(sizeof(*$$));
        *$$ = FRIEND;
    }

FuncDef
  : T_FUNC OptGenerics '(' OptArgsOptNamed ')' T_ARROW Type {
        $$ = FuncType($2, $4, $7);
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
  : NamedType
  | Type {
        $$ = ASTNamedType(&@$, safe_strdup(""), $1);
    }

OptGenerics
  : %empty {
        $$ = Vector();
    }
  | '<' IdentList '>' {
        $$ = $2;
    }

Func
  : T_FUNC OptGenerics '(' OptNamedArgs ')' T_ARROW Type '{' OptStatements '}' {
        $$ = ASTFunc(&@$, $2, $4, $7, $9);
    }

OptNamedArgs
  : %empty {
        $$ = Vector();
    }
  | NamedArgs

NamedArgs
  : NamedType {
        $$ = init_Vector($1);
    }
  | NamedArgs ',' NamedType {
        $$ = Vector_append($1, $3);
    }

Init
  : T_NEW T_IDENT OptGenerics '(' OptExpression ')' {
        $$ = ASTInit(&@$, $2, $3, $5);
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
    const char *filename,
    UNUSED yyscan_t scanner,
    const char *msg
) {
    fprintf(stderr,
        "%s:%d:%d: " RED "error: " RESET "%s",
        filename,
        locp->first_line,
        locp->first_column,
        msg
    );
}
