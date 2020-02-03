#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

void *
safe_malloc(size_t size) {
    void *ret;

    if (NULL == (ret = malloc(size))) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    return ret;
}

char *
safe_strdup(const char *str) {
    char *s;

    if (NULL == (s = strdup(str))) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    return s;
}

int
safe_asprintf(char **strp, const char *fmt, ...) {
    va_list args;
    int size;

    va_start(args, fmt);
    size = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    *strp = safe_malloc(size + 1);
    va_start(args, fmt);
    size = vsprintf(*strp, fmt, args);
    va_end(args);
    return size;
}
