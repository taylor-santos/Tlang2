#include <math.h>
#include "json.h"
#include "util.h"
#include "vector.h"

static void
print_indent(FILE *out, int indent) {
    fprintf(out, "%*s", indent * 2, "");
}

void
json_start(FILE *out, int *indent) {
    fprintf(out, "{\n");
    (*indent)++;
    print_indent(out, *indent);
}

void
json_end(FILE *out, int *indent) {
    fprintf(out, "\n");
    (*indent)--;
    print_indent(out, *indent);
    fprintf(out, "}");
}

void
json_label(const char *label, FILE *out) {
    fprintf(out, "\"%s\": ", label);
}

void
json_string(const char *value, FILE *out, UNUSED int indent) {
    fprintf(out, "\"%s\"", (char *)value);
}

void
json_int(long long int value, FILE *out, UNUSED int indent) {
    fprintf(out, "%lld", value);
}

void
json_double(double value, FILE *out, UNUSED int indent) {
    switch (fpclassify(value)) {
        case FP_INFINITE:
            if (0 > value) {
                fprintf(out, "\"-inf\"");
            } else {
                fprintf(out, "\"inf\"");
            }
            return;
        case FP_NAN:
            fprintf(out, "\"NaN\"");
            return;
        case FP_ZERO:
            fprintf(out, "0.0");
            return;
        default:
            fprintf(out, "%lf", value);
            return;
    }
}

void
json_comma(FILE *out, int indent) {
    fprintf(out, ",\n");
    print_indent(out, indent);
}

void
json_list(const Vector *list,
    void (*map)(const void *, FILE *, int),
    FILE *out,
    int indent) {
    size_t n = Vector_size(list);
    fprintf(out, "[");
    if (n > 0) {
        char *sep = "\n";
        for (size_t i = 0; i < n; i++) {
            fprintf(out, "%s", sep);
            sep = ",\n";
            print_indent(out, indent + 1);
            void *element = NULL;
            Vector_get(list, i, &element);
            map(element, out, indent + 1);
        }
        fprintf(out, "\n");
        print_indent(out, indent);
    }
    fprintf(out, "]");
}
