#ifndef SAFE_H
#define SAFE_H

#include <stddef.h>

void *
safe_malloc(size_t size);
char *
safe_strdup(const char *str);
int
safe_asprintf(char **strp, const char *fmt, ...);

#endif
