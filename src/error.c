#include "error.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

void error_fatal(int line, const char *fmt, ...) {
    va_list ap;
    if (line > 0)
        fprintf(stderr, "error (line %d): ", line);
    else
        fprintf(stderr, "error: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_warn(int line, const char *fmt, ...) {
    va_list ap;
    if (line > 0)
        fprintf(stderr, "warning (line %d): ", line);
    else
        fprintf(stderr, "warning: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}
