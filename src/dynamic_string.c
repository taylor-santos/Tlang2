#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include "safe.h"
#include "dynamic_string.h"

dstring
new_dstring(const char *str) {
    size_t size;

    size = strlen(str) + 1;
    return (dstring){
        safe_strdup(str), size
    };
}

void
append_char(dstring *str, char c) {
    str->str = safe_realloc(str->str, str->size + 1);
    str->str[str->size - 1] = c;
    str->str[str->size++] = '\0';
}

void
append_str(dstring *str, const char *s) {
    size_t size;

    size = strlen(s);
    str->str = safe_realloc(str->str, str->size + size);
    strcpy(str->str + str->size - 1, s);
    str->size += size;
}

void
vappend_str(dstring *str, const char *fmt, ...) {
    size_t size;
    va_list args;

    va_start(args, fmt);
    size = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    str->str = safe_realloc(str->str, str->size + size);
    va_start(args, fmt);
    vsprintf(str->str + str->size - 1, fmt, args);
    va_end(args);
    str->size += size;
}

void
delete_dstring(dstring str) {
    free(str.str);
}
