#include "asprintf.h"

#include <stdio.h>
#include <stdlib.h>

int vasprintf(char **strp, const char *fmt, va_list va)
{
	int len;
	va_list va_copy;

	va_copy(va_copy, va);
	len = vsnprintf(NULL, 0, fmt, va_copy);
	va_end(va_copy);

	/* Until version 2.0.6 glibc would return -1 on truncated output.
	 * OTOH, they had asprintf. */
	if (len < 0)
		return -1;

	*strp = malloc(len+1);
	if (!*strp)
		return -1;

	return vsprintf(*strp, fmt, va);
}

int asprintf(char **strp, const char *fmt, ...)
{
	va_list va;
	int len;

	va_start(va, fmt);
	len = vasprintf(strp, fmt, va);
	va_end(va);
	return len;
}

