#ifndef SAFE_H
#define SAFE_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"

void *SAFE_PTR;

#define safe_malloc(size) (SAFE_PTR = malloc(size),                         \
    NULL == SAFE_PTR                                                        \
    ? (fprintf(stderr,                                                      \
            "%s:%d: " RED "error: " RESET "in function %s(): ",             \
            __FILE__,                                                       \
            __LINE__,                                                       \
            __func__),                                                      \
        perror("malloc"),                                                   \
        exit(EXIT_FAILURE),                                                 \
        NULL)                                                               \
    : SAFE_PTR)

#define safe_realloc(ptr, new_size) (SAFE_PTR = realloc(ptr, new_size),     \
    NULL == SAFE_PTR                                                        \
    ? (fprintf(stderr,                                                      \
            "%s:%d: " RED "error: " RESET "in function %s(): ",             \
            __FILE__,                                                       \
            __LINE__,                                                       \
            __func__),                                                      \
        perror("realloc"),                                                  \
        exit(EXIT_FAILURE),                                                 \
        NULL)                                                               \
    : SAFE_PTR)

#define safe_strdup(str) (SAFE_PTR = strdup(str),                           \
    NULL == SAFE_PTR                                                        \
    ? (fprintf(stderr,                                                      \
            "%s:%d: " RED "error: " RESET "in function %s(): ",             \
            __FILE__,                                                       \
            __LINE__,                                                       \
            __func__),                                                      \
        perror("strdup"),                                                   \
        exit(EXIT_FAILURE),                                                 \
        NULL)                                                               \
    : SAFE_PTR)

#endif
