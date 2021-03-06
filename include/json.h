#ifndef JSON_H
#define JSON_H

#include <stdio.h>
#include <inttypes.h>

typedef void (*JSON_VALUE_FUNC)(const void *, FILE *, int);

struct Vector;
struct SparseVector;
struct dstring;

void
json_start(FILE *out, int *indent);

void
json_end(FILE *out, int *indent);

void
json_label(const char *label, FILE *out);

void
json_nlabel(const char *label, size_t len, FILE *out);

void
json_string(const char *value, FILE *out, int indent);

void
json_dstring(const struct dstring *value, FILE *out, int indent);

void
json_int(int64_t value, FILE *out, int indent);

void
json_double(double value, FILE *out, int indent);

void
json_comma(FILE *out, int indent);

void
json_empty(const void *value, FILE *out, int indent);

void
json_vector(const struct Vector *list,
    void (*map)(const void *, FILE *, int),
    FILE *out,
    int indent);

void
json_sparse_vector(const struct SparseVector *list,
    void(*map)(const void *, FILE *, int),
    FILE *out,
    int indent);

void
json_linked_list(const void *list,
    void(*json)(const void *, FILE *, int),
    const void *(*next)(const void *),
    FILE *out,
    int indent);

#endif
