#include "ast.h"
#include "parser.h"

typedef struct ASTData ASTData;

struct AST {
    void (*json)(const AST *this, FILE *out, int indent);
    void (*delete)(AST *this);
    struct YYLTYPE loc;
};

void
json_AST(const AST *this, FILE *out, int indent) {
    this->json(this, out, indent);
}

void
error_AST(const AST *this, const char *msg, FILE *out) {
    FILE *in;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int lineno = 1, w, maxw;

    fprintf(out,
        "%s:%d:%d-%d:%d: " RED "error: " RESET "%s:\n",
        this->loc.filename,
        this->loc.first_line,
        this->loc.first_column,
        this->loc.last_line,
        this->loc.last_column,
        msg);
    maxw = snprintf(NULL, 0, "%d", this->loc.last_line) + 1;
    if (NULL == (in = fopen(this->loc.filename, "r"))) {
        perror(this->loc.filename);
        return;
    }
    while (-1 != (read = getline(&line, &len, in)) &&
        lineno <= this->loc.last_line) {
        if (lineno >= this->loc.first_line) {
            w = snprintf(NULL, 0, "%d", lineno);
            int min_col = lineno == this->loc.first_line
                ? this->loc.first_column - 1
                : 0;
            int max_col = lineno == this->loc.last_line
                ? this->loc.last_column - 1
                : (int)read - 2;

            fprintf(out, "%*d | ", maxw - w, lineno);
            if (min_col > 1) {
                fprintf(out, "%.*s", min_col, line);
            }
            fprintf(out, RED);
            fprintf(out, "%.*s", max_col - min_col, line + min_col);
            fprintf(out, RESET);
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
            fprintf(out, RED);
            for (; i < max_col; i++) {
                fprintf(out,
                    "%c",
                    line[i] == '\t'
                        ? '\t'
                        : '~');
            }
            fprintf(out, RESET);
            fprintf(out, "\n");
        }
        lineno++;
    }
    free(line);
}

void
delete_AST(AST *this) {
    ((AST *)this)->delete(this);
}
