/*  very simple error handling functions */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "error.h"

/* print "ERROR: " followed by error msg "fmt", a printf style format string */
void
error(char *fmt, ...)
{
	va_list ap;

	(void) fflush(stdout);
	fprintf(stderr, "ERROR: ");

	va_start(ap, fmt);
	(void) vfprintf(stderr, fmt, ap);
	va_end(ap);

	(void) putc('\n', stderr);
}

/* call error and die */
void
fatal(char *fmt, ...)
{
	va_list ap;

	(void) fflush(stdout);
	fprintf(stderr, "ERROR: ");

	va_start(ap, fmt);
	(void) vfprintf(stderr, fmt, ap);
	va_end(ap);

	(void) putc('\n', stderr);
	exit(EXIT_FAILURE);
}

void *
emalloc(size_t size)
{
	register void *p = malloc(size);

	if (p == 0)
		fatal("out of memory");
	return p;
}

void *
erealloc(void *ptr, size_t size)
{
	if (0 == ptr)
		return emalloc(size);
	if (0 == size) {
		free(ptr);
		return 0;
	}
	ptr = realloc(ptr, size);
	if (0 == ptr)
		fatal("out of memory");

	return ptr;
}

FILE *
efopen(const char *path, const char *mode)
{
	FILE *p = fopen(path, mode);
	if (p == 0) {
		perror("efopen");
		error("unable to open file: %s", path);
	}
	return p;
}
