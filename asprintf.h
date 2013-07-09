/*
 * This code is licensed under BSD-MIT By Rusty Russell, see also:
 * https://github.com/rustyrussell/ccan/
 *
 * Tiny modifications by Ahmed Samy 2013.
 */

#ifndef asprintf
#ifndef _ASPRINTF_H
#define _ASPRINTF_H

#include <stdarg.h>

extern int asprintf(char **strp,  const char *fmt, ...)
			__attribute__((__format__(__printf__, 2, 3)));
extern int vasprintf(char **strp, const char *fmt, va_list va);

#endif  /* _ASPRINTF_H */
#endif

