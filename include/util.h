#ifndef UTIL_H
#define UTIL_H
#define LANG "tlang"
#ifdef _WIN32
#define RED     ""
#define WHITE   ""
#define RESET   ""
#define ERROR   "error: "
#else
#define RED     "\033[1m\033[31m"
#define WHITE   "\033[1m\033[37m"
#define RESET   "\033[0m"
#endif
#define UNUSED __attribute__ ((unused))

#include <stdint.h>

int
print_error(const char *fmt, ...);
int
print_ICE(const char *fmt, ...);

#endif
