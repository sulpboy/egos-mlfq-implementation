/*
 * (C) 2025, Cornell University
 * All rights reserved.
 *
 * Description: formatted printing
 * format_to_str() converts a format into a C string:
 * e.g., converts ("%s-%d", "egos", 2000) to "egos-2000".
 * term_write() prints the converted C string to the screen.
 */

#include "egos.h"
#include "servers.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
static void ulltoa_base(unsigned long long value, char *out, int base) {
    char buf[32];
    int i = 0;

    if (value == 0) {
        buf[i++] = '0';
    } else {
        while (value) {
            int d = value % base;
            buf[i++] = (d < 10) ? ('0' + d) : ('a' + d - 10);
            value /= base;
        }
    }

    while (i--) {
        *out++ = buf[i];
    }
    *out = '\0';
}

static void lltoa_base(long long value, char *out, int base) {
    if (value < 0) {
        *out++ = '-';
        ulltoa_base((unsigned long long)(-value), out, base);
    } else {
        ulltoa_base((unsigned long long)value, out, base);
    }
}

void format_to_str(char* out, const char* fmt, va_list args) {
    for (out[0] = 0; *fmt != '\0'; fmt++) {
        if (*fmt != '%') {
            strncat(out, fmt, 1);
        } else {
            fmt++;
            if (*fmt == 'l' && *(fmt + 1) == 'l') {
                char spec = *(fmt + 2);
                if (spec == 'd') {
                    long long v = va_arg(args, long long);
                    lltoa_base(v, out + strlen(out), 10);
                } else if (spec == 'u') {
                    unsigned long long v = va_arg(args, unsigned long long);
                    ulltoa_base(v, out + strlen(out), 10);
                } else if (spec == 'x') {
                    unsigned long long v = va_arg(args, unsigned long long);
                    ulltoa_base(v, out + strlen(out), 16);
                }
                fmt += 2; /* skip second 'l' and the specifier */
            } else if (*fmt == 's') {
                strcat(out, va_arg(args, char*));
            } else if (*fmt == 'd') {
                itoa(va_arg(args, int), out + strlen(out), 10);
            } else if (*fmt == 'x') {
                itoa(va_arg(args, int), out + strlen(out), 16);
            } else if (*fmt == 'c') {
                char c = (char)va_arg(args, int);
                strncat(out, &c, 1);
            } else if (*fmt == 'u') {
                unsigned int v = va_arg(args, unsigned int);
                itoa((int)v, out + strlen(out), 10);
            } else if (*fmt == 'p') {
                unsigned long v = (unsigned long)va_arg(args, void*);
                strcat(out, "0x");
                itoa((int)v, out + strlen(out), 16);
            }
        }

    }
}

#define LOG(prefix, suffix)                                                    \
    char buf[512];                                                             \
    strcpy(buf, prefix);                                                       \
    va_list args;                                                              \
    va_start(args, format);                                                    \
    format_to_str(buf + strlen(prefix), format, args);                         \
    va_end(args);                                                              \
    strcat(buf, suffix);                                                       \
    term_write(buf, strlen(buf));

int my_printf(const char* format, ...) { LOG("", ""); }

int INFO(const char* format, ...) { LOG("[INFO] ", "\n\r") }

int FATAL(const char* format, ...) {
    LOG("\x1B[1;31m[FATAL] ", "\x1B[1;0m\n\r") /* \x1B[1;31m means red. */
    while (1);
}

int SUCCESS(const char* format, ...) {
    LOG("\x1B[1;32m[SUCCESS] ", "\x1B[1;0m\n\r") /* \x1B[1;32m means green. */
}

int CRITICAL(const char* format, ...) {
    LOG("\x1B[1;33m[CRITICAL] ", "\x1B[1;0m\n\r") /* \x1B[1;33m means yellow. */
}
