#ifndef JSON_H
#define JSON_H

#include <stdio.h>

typedef void (*JSON_MAP_TYPE)(const void *, FILE *, int);

struct Vector;

void
json_start(FILE *out, int *indent);

void
json_end(FILE *out, int *indent);

void
json_label(const char *label, FILE *out);

void
json_string(const char *value, FILE *out, int indent);

void
json_int(long long int value, FILE *out, int indent);

void
json_double(double value, FILE *out, int indent);

void
json_comma(FILE *out, int indent);

void
json_list(const struct Vector *list,
    void (*map)(const void *, FILE *, int),
    FILE *out,
    int indent);

#endif
