#include "util.h"
#include <stdarg.h>
#include <stdio.h>

int
print_error(const char *fmt, ...) {
    va_list args;
    int n;

    n = fprintf(stderr, WHITE LANG ": " RED "error: " RESET);
    va_start(args, fmt);
    n += vfprintf(stderr, fmt, args);
    va_end(args);
    return n;
}

int
print_ICE(const char *fmt, ...) {
    va_list args;
    int n;

    n = fprintf(stderr, WHITE LANG ": " RED "internal compiler error: " RESET);
    va_start(args, fmt);
    n += vfprintf(stderr, fmt, args);
    va_end(args);
    return n;
}
