#ifndef error_h
#define error_h

void error(char *fmt, ...);
void fatal(char *fmt, ...);

void *emalloc(size_t size);
void *erealloc(void *ptr, size_t size);
FILE *efopen(const char *path, const char *mode);

#endif
