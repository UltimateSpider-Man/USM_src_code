#pragma once

#include <cstdarg>
#include <cstdio>
#include <iostream>

void __log(const char *file, int line, const char *format, ...);

#define sp_log(fmt, ...)                    \
    {                                       \
        __log(__FILE__,                     \
              __LINE__,                     \
              fmt,                          \
              ##__VA_ARGS__);               \
    }

inline void __log(const char *file, int line, const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("[%s:%d] ", file, line);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}
