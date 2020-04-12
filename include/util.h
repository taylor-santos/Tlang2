#ifndef UTIL_H
#define UTIL_H
#define LANG "tlang"
#ifdef _WIN32
#include <windows.h>

extern HANDLE handle;
extern WORD saved_attributes;
#define RED(FILE)    SetConsoleTextAttribute(handle, \
    FOREGROUND_RED)
#define PURPLE(FILE) SetConsoleTextAttribute(handle, \
    FOREGROUND_RED | FOREGROUND_BLUE)
#define WHITE(FILE)  SetConsoleTextAttribute(handle, \
    FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define RESET(FILE)  SetConsoleTextAttribute(handle, saved_attributes)
#else
#define RED(FILE)    fprintf(FILE, "\033[1m\033[31m")
#define PURPLE(FILE) fprintf(FILE, "\033[1m\033[35m")
#define WHITE(FILE)  fprintf(FILE, "\033[1m\033[37m")
#define RESET(FILE)  fprintf(FILE, "\033[0m")
#endif
#define UNUSED __attribute__ ((unused))
#define print_code_error(out, loc, msg, ...) { \
    fprintf(out, "%s:%d:\n", __FILE__, __LINE__); \
    print_code_error_func(out, loc, msg, __VA_ARGS__); \
}

#include <stdio.h>

struct YYLTYPE;

int
print_error(const char *fmt, ...);
int
print_warning(const char *fmt, ...);
int
print_ICE(const char *fmt, ...);
void
print_code_error_func(FILE *out, struct YYLTYPE loc, const char *msg, ...);
void
strident(const char *str, char *buf);

#endif
