#ifndef STDIO_H
#define STDIO_H
#include <stddef.h>
#include <stdarg.h>
/* FILE is opaque: DOOM only passes FILE pointers around. */
typedef struct FILE FILE;
extern FILE *stdout;
extern FILE *stderr;
#define SEEK_SET 0
#define SEEK_END 2
FILE *fopen(const char *path, const char *mode);
int fclose(FILE *f);
size_t fread(void *buf, size_t size, size_t count, FILE *f);
size_t fwrite(const void *buf, size_t size, size_t count, FILE *f);
int fseek(FILE *f, long offset, int whence);
long ftell(FILE *f);
int fflush(FILE *f);
int printf(const char *fmt, ...);
int fprintf(FILE *f, const char *fmt, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vfprintf(FILE *f, const char *fmt, va_list ap);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
int sscanf(const char *s, const char *fmt, ...);
int puts(const char *s);
int putchar(int c);
int remove(const char *path);
int rename(const char *from, const char *to);
#endif
