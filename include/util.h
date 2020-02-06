#ifndef UTIL_H
#define UTIL_H
#define LANG "tlang"
#ifdef _WIN32
#include <windows.h>

extern HANDLE handle;
extern WORD saved_attributes;
#define RED(FILE)   SetConsoleTextAttribute(handle, 12)
#define WHITE(FILE) SetConsoleTextAttribute(handle, 15)
#define RESET(FILE) SetConsoleTextAttribute(handle, saved_attributes)
#else
#define RED(FILE)   fprintf(FILE, "\033[1m\033[31m")
#define WHITE(FILE) fprintf(FILE, "\033[1m\033[37m")
#define RESET(FILE) fprintf(FILE, "\033[0m")
#endif
#define UNUSED __attribute__ ((unused))

#include <stdio.h>

struct YYLTYPE;

int
print_error(const char *fmt, ...);
int
print_ICE(const char *fmt, ...);
void
print_code_error(const struct YYLTYPE *loc, const char *msg, FILE *out);

#endif
