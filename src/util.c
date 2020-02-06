#include "util.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

int
print_error(const char *fmt, ...) {
    va_list args;
    int n;

    WHITE(stderr);
    n = fprintf(stderr, LANG ": ");
    RED(stderr);
    n += fprintf(stderr, "error: ");
    RESET(stderr);
    va_start(args, fmt);
    n += vfprintf(stderr, fmt, args);
    va_end(args);
    return n;
}

int
print_ICE(const char *fmt, ...) {
    va_list args;
    int n;

    WHITE(stderr);
    n = fprintf(stderr, LANG ": ");
    RED(stderr);
    n += fprintf(stderr, "internal compiler error: ");
    RESET(stderr);
    va_start(args, fmt);
    n += vfprintf(stderr, fmt, args);
    va_end(args);
    return n;
}

void
print_code_error(const YYLTYPE *loc, const char *msg, FILE *out) {
    FILE *in;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int lineno = 1, w, maxw;

    fprintf(out,
        "%s:%d:%d-%d:%d: ",
        loc->filename,
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column);
    RED(out);
    fprintf(out, "error: ");
    RESET(out);
    fprintf(out, "%s:\n", msg);
    maxw = snprintf(NULL, 0, "%d", loc->last_line) + 1;
    if (NULL == (in = fopen(loc->filename, "r"))) {
        perror(loc->filename);
        return;
    }
    while (-1 != (read = getline(&line, &len, in)) &&
        lineno <= loc->last_line) {
        if (lineno >= loc->first_line) {
            w = snprintf(NULL, 0, "%d", lineno);
            int min_col = lineno == loc->first_line
                ? loc->first_column - 1
                : 0;
            int max_col = lineno == loc->last_line
                ? loc->last_column - 1
                : (int)read - 2;

            fprintf(out, "%*d | ", maxw - w, lineno);
            if (min_col > 1) {
                fprintf(out, "%.*s", min_col, line);
            }
            RED(out);
            fprintf(out, "%.*s", max_col - min_col, line + min_col);
            RESET(out);
            fprintf(out, "%s", line + max_col);

            if (line[read - 1] != '\n') {
                fprintf(out, "\n");
            }
            fprintf(out, "%*s | ", maxw - 1, "");
            int i = 0;
            for (; i < min_col; i++) {
                fprintf(out,
                    "%c",
                    line[i] == '\t'
                        ? '\t'
                        : ' ');
            }
            for (; i < max_col && (line[i] == '\t' || line[i] == ' '); i++) {
                fprintf(out,
                    "%c",
                    line[i] == '\t'
                        ? '\t'
                        : ' ');
            }
            RED(out);
            for (; i < max_col; i++) {
                fprintf(out,
                    "%c",
                    line[i] == '\t'
                        ? '\t'
                        : '~');
            }
            RESET(out);
            fprintf(out, "\n");
        }
        lineno++;
    }
    free(line);
}
