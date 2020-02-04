#include <stddef.h>
#include <string.h>
#include "safe.h"
#include "dynamic_string.h"

dstring *
new_dstring(const char *str) {
    dstring *ret;
    size_t size;

    ret = safe_malloc(sizeof(*ret));
    size = strlen(str) + 1;
    *ret = (dstring){
        safe_strdup(str), size
    };
    return ret;
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
delete_dstring(dstring *str) {
    free(str->str);
    free(str);
}
