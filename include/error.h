#ifndef ERROR_H
#define ERROR_H

/* Print "error: <line>: <fmt...>" to stderr, then exit(1). */
void error_fatal(int line, const char *fmt, ...);

/* Print a warning (does not exit). */
void error_warn(int line, const char *fmt, ...);

#endif
