%{
    #include "parser.h"
    #include "ast.h"
    #include "safe.h"
    #include <errno.h>
    #include <limits.h>

    #define YY_USER_ACTION                                               \
        yylloc->first_line = yylloc->last_line;                          \
        yylloc->first_column = yylloc->last_column;                      \
        if (yylloc->last_line == yylineno)                               \
          yylloc->last_column += yyleng;                                 \
        else {                                                           \
          yylloc->last_line = yylineno;                                  \
          yylloc->last_column = yytext + yyleng - strrchr(yytext, '\n'); \
        }
    YY_DECL;
    void yyerror(YYLTYPE *locp,
        const void **root,
        const char *filename,
        yyscan_t scanner,
        const char *msg);

    enum yytokentype parse_int(const char *str,
        long long int *val,
        YYLTYPE *yylloc,
        const char *filename) {
        char *end = NULL;
        errno = 0;
        *val = strtoll(str, &end, 10);
        if (errno != 0) {
            yyerror(yylloc,
                NULL,
                filename,
                NULL,
                "invalid int literal ");
            fprintf(stderr, "\"%s\": ", str);
            perror("");
            return T_ERROR;
        } else if (end == str) {
            yyerror(yylloc,
                NULL,
                filename,
                NULL,
                "invalid int literal ");
            fprintf(stderr, "\"%s\"\n", str);
            return T_ERROR;
        }
        return T_INT;
    }

    enum yytokentype parse_double(const char *str,
        double *val,
        YYLTYPE *yylloc,
        const char *filename) {
        char *end = NULL;
        errno = 0;
        *val = strtod(str, &end);
        if (errno != 0) {
            yyerror(yylloc,
                NULL,
                filename,
                NULL,
                "invalid double literal ");
            fprintf(stderr, "\"%s\": ", str);
            perror("");
            return T_ERROR;
        } else if (end == str) {
            yyerror(yylloc,
                NULL,
                filename,
                NULL,
                "invalid double literal ");
            fprintf(stderr, "\"%s\"\n", str);
            return T_ERROR;
        }
        return T_DOUBLE;
    }
%}

%option debug
%option warn
%option nounput
%option reentrant noyywrap never-interactive nounistd
%option bison-bridge bison-locations
%option noinput
%option yylineno
%option noyywrap

%x comment

%%

[/][/].*
"/*" {
    BEGIN(comment);
};

<comment>{
    "*/" {
        BEGIN(INITIAL);
    }
    .|[\n\r]
    <<EOF>> {
        yyerror(yylloc,
            NULL,
            filename,
            NULL,
            "end of file encountered inside comment block\n");
        return T_ERROR;
    }
}

[ \t\n\r\s]+
class  { return T_CLASS; }
func   { return T_FUNC; }
new    { return T_NEW; }
return { return T_RETURN; }
const  { return T_CONST; }
friend { return T_FRIEND; }
[:][=] { return T_DEF; }
[=][>] { return T_ARROW; }
[*][=] { return T_MUL_ASSIGN; }
[/][=] { return T_DIV_ASSIGN; }
[%][=] { return T_MOD_ASSIGN; }
[+][=] { return T_ADD_ASSIGN; }
[-][=] { return T_SUB_ASSIGN; }
[|][|] { return T_OR; }
[&][&] { return T_AND; }
[=][=] { return T_EQ; }
[!][=] { return T_NE; }
[<][=] { return T_LE; }
[>][=] { return T_GE; }
[+][+] { return T_INC; }
[-][-] { return T_DEC; }
[;{}<>:()[\]]  {
    return *yytext;
}
-?[0-9]+ {
    return parse_int(yytext, &yylval->int_lit, yylloc, filename);
}
-?[0-9]+\.[0-9]* {
    return parse_double(yytext, &yylval->double_lit, yylloc, filename);
}
-?\.[0-9]+ {
    return parse_double(yytext, &yylval->double_lit, yylloc, filename);
}
-?[0-9]+(\.[0-9]+)?(e|E)-?[0-9]+ {
    return parse_double(yytext, &yylval->double_lit, yylloc, filename);
}
-?inf {
    return parse_double(yytext, &yylval->double_lit, yylloc, filename);
}
nan {
    return parse_double(yytext, &yylval->double_lit, yylloc, filename);
}
[a-zA-Z_][a-zA-Z0-9_]* {
    yylval->str = safe_strdup(yytext);
    return T_IDENT;
}

. {
    return yytext[0];
}

%%
