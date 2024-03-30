#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

bool is_debug = false;

static inline void panic(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);

  exit(EXIT_FAILURE);
}

static inline void debug(const char *format, ...) {
  if (!is_debug)
    return;

  va_list args;
  va_start(args, format);
  vfprintf(stdout, format, args);
  va_end(args);
}

static inline void warn(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}

static inline void info(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stdout, format, args);
  va_end(args);
}
