%{
    #include "parser.h"
    #include "ast.h"
    #include "safe.h"
    #include "dynamic_string.h"
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

    static dstring *str;

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
%option nodefault
%option reentrant noyywrap never-interactive nounistd
%option bison-bridge bison-locations
%option noinput
%option yylineno
%option noyywrap

%x comment string

%%

[/][/].*
[ \t\n\r]+

"/*" BEGIN(comment);
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

\" {
    str = new_dstring("");
    BEGIN(string);
}
<string>{
    \" {
        yylval->dstr = str;
        BEGIN(INITIAL);
        return T_STRING;
    }
    [\n\r] {
        yyerror(yylloc,
            NULL,
            filename,
            NULL,
            "unterminated string constant\n");
        return T_ERROR;
    }
    \\n {
        append_char(str, '\n');
    }
    \\r {
        append_char(str, '\r');
    }
    \\b {
        append_char(str, '\b');
    }
    \\t {
        append_char(str, '\t');
    }
    \\a {
        append_char(str, '\a');
    }
    \\v {
        append_char(str, '\v');
    }
    \\f {
        append_char(str, '\f');
    }
    \\\\ {
        append_char(str, '\\');
    }
    \\[0-7]{1,3} {
        unsigned int result;
        sscanf(yytext+1, "%o", &result);
        if (result > 0xff) {
            char msg[40];
            sprintf(msg, "invalid octal escape sequence: '%s'\n", yytext);
            yyerror(yylloc,
                NULL,
                filename,
                NULL,
                msg);
            return T_ERROR;
        }
        append_char(str, (char)result);
    }
    [^\\"]+ {
        append_str(str, yytext);
    }
    \\[\n\r]+ {
        append_str(str, yytext);
    }
    \\. {
        char msg[40];
        sprintf(msg, "invalid escape sequence: '%s'\n", yytext);
        yyerror(yylloc,
            NULL,
            filename,
            NULL,
            msg);
        return T_ERROR;
    }
    \\  {
        yyerror(yylloc,
            NULL,
            filename,
            NULL,
            "dangling escape character in string constant\n");
        return T_ERROR;
    }
    <<EOF>> {
        yyerror(yylloc,
            NULL,
            filename,
            NULL,
            "end of file encountered inside string constant\n");
        return T_ERROR;
    }
}

class  { return T_CLASS; }
func   { return T_FUNC; }
new    { return T_NEW; }
return { return T_RETURN; }
const  { return T_CONST; }
friend { return T_FRIEND; }
true   { return T_TRUE; }
false  { return T_FALSE; }
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
[.][.] { return T_RANGE; }
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