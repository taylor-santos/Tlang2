#ifndef SAFE_H
#define SAFE_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

void *SAFE_PTR;
int SAFE_SIZE;

#define safe_malloc(size) (SAFE_PTR = malloc((size)),                       \
    NULL == SAFE_PTR                                                        \
    ? (fprintf(stderr, "%s:%d: ", __FILE__, __LINE__),                      \
        RED(stderr),                                                        \
        fprintf(stderr, "error: "),                                         \
        RESET(stderr),                                                      \
        fprintf(stderr, "in function %s(): ", __func__),                    \
        perror("malloc"),                                                   \
        exit(EXIT_FAILURE),                                                 \
        NULL)                                                               \
    : SAFE_PTR)

#define safe_calloc(count, size) (SAFE_PTR = calloc((count), (size)),       \
    NULL == SAFE_PTR                                                        \
    ? (fprintf(stderr, "%s:%d: ", __FILE__, __LINE__),                      \
        RED(stderr),                                                        \
        fprintf(stderr, "error: "),                                         \
        RESET(stderr),                                                      \
        fprintf(stderr, "in function %s(): ", __func__),                    \
        perror("calloc"),                                                   \
        exit(EXIT_FAILURE),                                                 \
        NULL)                                                               \
    : SAFE_PTR)

#define safe_realloc(ptr, new_size) (SAFE_PTR = realloc((ptr), (new_size)), \
    NULL == SAFE_PTR                                                        \
    ? (fprintf(stderr, "%s:%d: ", __FILE__, __LINE__),                      \
        RED(stderr),                                                        \
        fprintf(stderr, "error: "),                                         \
        RESET(stderr),                                                      \
        fprintf(stderr, "in function %s(): ", __func__),                    \
        perror("realloc"),                                                  \
        exit(EXIT_FAILURE),                                                 \
        NULL)                                                               \
    : SAFE_PTR)

#define safe_strdup(str) (SAFE_PTR = strdup((str)),                         \
    NULL == SAFE_PTR                                                        \
    ? (fprintf(stderr, "%s:%d: ", __FILE__, __LINE__),                      \
        RED(stderr),                                                        \
        fprintf(stderr, "error: "),                                         \
        RESET(stderr),                                                      \
        fprintf(stderr, "in function %s(): ", __func__),                    \
        perror("strdup"),                                                   \
        exit(EXIT_FAILURE),                                                 \
        NULL)                                                               \
    : SAFE_PTR)

#define safe_asprintf(fmt, ...) (                                           \
    SAFE_SIZE = snprintf(NULL, 0, (fmt), __VA_ARGS__),                      \
    SAFE_PTR = malloc(SAFE_SIZE + 1),                                       \
    NULL == SAFE_PTR                                                        \
    ? (fprintf(stderr, "%s:%d: ", __FILE__, __LINE__),                      \
        RED(stderr),                                                        \
        fprintf(stderr, "error: "),                                         \
        RESET(stderr),                                                      \
        fprintf(stderr, "in function %s(): ", __func__),                    \
        perror("malloc"),                                                   \
        exit(EXIT_FAILURE),                                                 \
        NULL)                                                               \
    : (sprintf(SAFE_PTR, (fmt), __VA_ARGS__), SAFE_PTR))

char *
safe_strdup_func(const char *str);

#endif
