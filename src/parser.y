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
    struct ClassBody *class;
    SparseVector *svec;
    char *str;
    dstring dstr;
    long long int int_lit;
    double double_lit;
    Type *type;
    Qualifiers *qualifier;
}

%token             T_CLASS      "class"
                   T_FUNC       "func"
                   T_IMPL       "impl"
                   T_NEW        "new"
                   T_RETURN     "return"
                   T_CONST      "const"
                   T_FRIEND     "friend"
                   T_TRUE       "true"
                   T_FALSE      "false"
                   T_MAYBE      "maybe"
                   T_IF         "if"
                   T_ELSE       "else"
                   T_IS         "is"
                   T_NOT        "not"
                   T_SWITCH     "switch"
                   T_CASE       "case"
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
                   T_ERROR      "lexing error"
                   END 0        "end of file"
%token<str>        T_IDENT      "identifier"
%token<dstr>       T_STRING     "string literal"
%token<int_lit>    T_INT        "int literal"
                   T_RANGE      ".."
                   T_INDEX      "index operator"
%token<double_lit> T_DOUBLE     "double literal"

%type<ast>  File Statement Definition Expression Class Return OptExpression Func
            Init PrimaryExpr PostfixExpr UnaryExpr OpExpr TypeStmt Impl If
%type<vec>  OptStatements Statements OptGenerics IdentList OptInherits Inherits
            OptNamedArgs NamedArgs OptArgsOptNamed ArgsOptNamed Qualifiers Tuple
            DefVars Constructor OptArguments Arguments OptElse
%type<svec> Types
%type<type> Type TypeDef FuncDef TypeOptNamed
%type<class> Fields OptFields
%type<field> Field
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
  | Impl
  | TypeStmt ';'
  | Expression ';'
  | Return ';'

Definition
  : DefVars T_DEF Expression ';' {
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
  : T_IF '(' Expression ')' '{' OptStatements '}' OptElse {
        $$ = ASTIf(@$, $3, $6, $8);
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
  | Class

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

OpExpr
  : UnaryExpr
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
  : OpExpr ',' OpExpr {
        $$ = Vector_append(init_Vector($1), $3);
    }
  | Tuple ',' OpExpr {
        $$ = Vector_append($1, $3);
    }

Expression
  : OpExpr
  | UnaryExpr '=' Expression
  | UnaryExpr T_MUL_ASSIGN Expression
  | UnaryExpr T_DIV_ASSIGN Expression
  | UnaryExpr T_MOD_ASSIGN Expression
  | UnaryExpr T_ADD_ASSIGN Expression
  | UnaryExpr T_SUB_ASSIGN Expression

Class
  : T_CLASS OptGenerics OptInherits '{' OptFields '}' {
        $$ = ASTClass(@$, $2, $3, $5);
    }

OptFields
  : %empty {
        $$ = safe_malloc(sizeof(*$$));
        $$->fields = Vector();
        $$->constructors = Vector();
    }
  | Fields

Fields
  : Field ';' {
        $$ = safe_malloc(sizeof(*$$));
        $$->fields = init_Vector($1);
        $$->constructors = Vector();
    }
  | Constructor ';' {
        $$ = safe_malloc(sizeof(*$$));
        $$->fields = Vector();
        $$->constructors = init_Vector($1);
    }
  | Fields Field ';' {
        $$ = $1;
        $$->fields = Vector_append($$->fields, $2);
    }
  | Fields Constructor ';' {
        $$ = $1;
        $$->constructors = Vector_append($$->constructors, $2);
    }

Field
  : IdentList ':' Type {
        $$ = safe_malloc(sizeof(*$$));
        $$->names = $1;
        $$->type = $3;
    }

Constructor
  : T_NEW '(' OptArgsOptNamed ')' {
        $$ = $3;
    }

Impl
  : T_IMPL T_IDENT OptGenerics '{' OptStatements '}' {
        $$ = ASTImpl(@$, $2, $3, $5);
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
        setTypeQualifiers($$, $1);
    }

TypeDef
  : T_IDENT OptGenerics {
        $$ = ObjectType(@$, $1, $2);
    }
  | FuncDef
  | '[' Expression ']' OptGenerics {
        $$ = ExprType(@$, $2, $4);
    }
  | '(' Type ')' {
        $$ = $2;
    }
  | '(' Types ')' {
        $$ = TupleType(@$, $2);
    }
  |  '[' ']' Type {
        $$ = ArrayType(@$, $3);
    }
  | T_MAYBE Type {
        $$ = MaybeType(@$, $2);
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
  : Qualifier {
        $$ = init_Vector($1);
    }
  | Qualifiers Qualifier {
        $$ = Vector_append($1, $2);
    }

Qualifier
  : T_CONST {
        $$ = safe_malloc(sizeof(*$$));
        *$$ = Q_CONST;
    }
  | T_FRIEND {
        $$ = safe_malloc(sizeof(*$$));
        *$$ = Q_FRIEND;
    }

FuncDef
  : T_FUNC OptGenerics '(' OptArgsOptNamed ')' T_ARROW Type {
        $$ = FuncType(@$, $2, $4, $7);
    }
  | T_FUNC OptGenerics '(' OptArgsOptNamed ')' {
          $$ = FuncType(@$, $2, $4, NoneType(@$));
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
        $$ = NamedType(@$, $1, $3);
    }
  | Type

OptGenerics
  : %empty {
        $$ = Vector();
    }
  | '<' IdentList '>' {
        $$ = $2;
    }

Func
  : T_FUNC OptGenerics '(' OptNamedArgs ')' T_ARROW Type '{' OptStatements '}' {
        $$ = ASTFunc(@$, $2, $4, $7, $9);
    }
  | T_FUNC OptGenerics '(' OptNamedArgs ')' '{' OptStatements '}' {
        $$ = ASTFunc(@$, $2, $4, NoneType(@$), $7);
    }

OptNamedArgs
  : %empty {
        $$ = Vector();
    }
  | NamedArgs

NamedArgs
  : Field {
        $$ = init_Vector($1);
    }
  | NamedArgs ',' Field {
        $$ = Vector_append($1, $3);
    }

Init
  : T_NEW T_IDENT OptGenerics '(' OptArguments ')' {
        $$ = ASTInit(@$, $2, $3, $5);
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
        $$ = init_Vector($1);
    }
  | Arguments ',' Expression {
        $$ = Vector_append($1, $3);
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
    print_code_error(stderr, *locp, msg);
}
