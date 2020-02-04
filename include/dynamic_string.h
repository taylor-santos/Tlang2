#ifndef DYNAMIC_STRING_H
#define DYNAMIC_STRING_H
#define dstring(str) new_dstring(str)

typedef struct dstring dstring;
struct dstring {
    char *str;
    size_t size;
};

dstring *
new_dstring(const char *str);

void
append_char(dstring *str, char c);

void
append_str(dstring *str, const char *s);

void
delete_dstring(dstring *str);

#endif
