/*
 * lib.c: the C library doom.c needs, written in the same C subset as doom.c.
 *
 * For a compiler with no preprocessor, no system headers and no C library:
 * compile this file followed by doom.c as one translation unit
 * (cat lib.c doom.c) and skip every #include line. The declarations below
 * stand in for the headers doom.c names. Only va_list, va_start, va_arg and
 * va_end are left to the compiler.
 *
 * Assumes char 1, short 2, int/long/pointers 4 bytes. No floating point,
 * 64-bit integers, bitfields, goto or macros, and no large stack frames.
 *
 * Together with the seven DG_* functions that doom.c declares, the host
 * implements just the four functions below. The only file DOOM has to be
 * able to read is its IWAD; nothing can be written.
 */
void host_exit(int status);          /* stop the program; does not return */
void host_print(const char *text);   /* append text to the log */
int host_size(const char *name);     /* size of a file in bytes, -1 if absent */
/* Copy up to count bytes starting at offset; return how many were copied. */
int host_read(const char *name, void *buffer, int offset, int count);

#include <stdarg.h>

/* ---- stddef.h, stdint.h, stdbool.h, limits.h, stdio.h constants ---- */

typedef unsigned int size_t;
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef int intptr_t;
typedef int bool;
enum {
    NULL = 0, false = 0, true = 1,
    SEEK_SET = 0, SEEK_CUR = 1, SEEK_END = 2,
    SHRT_MAX = 32767, INT_MAX = 2147483647, INT_MIN = -2147483647 - 1
};

/* ---- string.h ---- */

void *memcpy(void *dst, const void *src, size_t n)
{
    char *d = dst;
    const char *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
    char *d = dst;
    const char *s = src;
    if (d < s) return memcpy(dst, src, n);
    while (n--) d[n] = s[n];
    return dst;
}

void *memset(void *dst, int c, size_t n)
{
    char *d = dst;
    while (n--) *d++ = c;
    return dst;
}

size_t strlen(const char *s)
{
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

int strncmp(const char *a, const char *b, size_t n)
{
    while (n--) {
        int d = *(const unsigned char *) a - *(const unsigned char *) b;
        if (d || !*a) return d;
        a++;
        b++;
    }
    return 0;
}

int strcmp(const char *a, const char *b)
{
    return strncmp(a, b, (size_t) -1);
}

char *strncpy(char *dst, const char *src, size_t n)
{
    size_t i = 0;
    while (i < n && src[i]) {
        dst[i] = src[i];
        i++;
    }
    while (i < n) dst[i++] = 0;
    return dst;
}

char *strchr(const char *s, int c)
{
    for (;; s++) {
        if (*s == (char) c) return (char *) s;
        if (!*s) return NULL;
    }
}

char *strrchr(const char *s, int c)
{
    char *last = NULL;
    for (;; s++) {
        if (*s == (char) c) last = (char *) s;
        if (!*s) return last;
    }
}

char *strstr(const char *s, const char *needle)
{
    size_t n = strlen(needle);
    for (;; s++) {
        if (!strncmp(s, needle, n)) return (char *) s;
        if (!*s) return NULL;
    }
}

/* ---- ctype.h ---- */

int isspace(int c)
{
    return c == ' ' || (c >= '\t' && c <= '\r');
}

int toupper(int c)
{
    return c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c;
}

int tolower(int c)
{
    return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
}

/* ---- stdlib.h ---- */

/*
 * Bump allocation out of a fixed 32MiB array: a size word, then the block.
 * free() does nothing. DOOM takes a 6MiB zone and the screen buffer at
 * startup and manages the zone itself, so little else is ever allocated.
 */
static unsigned int lib_heap[8 * 1024 * 1024];
static size_t lib_heap_used; /* in words */

void *malloc(size_t size)
{
    size_t words = size / 4 + 2;
    unsigned int *p = lib_heap + lib_heap_used;
    if (words > sizeof(lib_heap) / 4 - lib_heap_used) return NULL;
    lib_heap_used += words;
    *p = size;
    return p + 1;
}

void *calloc(size_t count, size_t size)
{
    void *p = malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}

void *realloc(void *old, size_t size)
{
    void *p = malloc(size);
    if (p && old) {
        size_t n = ((unsigned int *) old)[-1];
        memcpy(p, old, n < size ? n : size);
    }
    return p;
}

void free(void *p)
{
}

char *strdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

int abs(int n)
{
    return n < 0 ? -n : n;
}

void exit(int status)
{
    host_exit(status);
}

int system(const char *command)
{
    return -1;
}

/* ---- stdio.h: formatted output ---- */

static int lib_put(char *buf, size_t size, int n, int c)
{
    if ((size_t) n + 1 < size) buf[n] = c;
    return n + 1;
}

/* %d %i %u %x %X %o %p %c %s %%, with the - and 0 flags, width and precision. */
int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    int n = 0;
    while (*fmt) {
        int left = 0, zero = 0, width = 0, precision = -1, length = 0, zeros = 0, i;
        unsigned int value, base = 10;
        char digits[12];
        char *text = digits + 12;
        const char *prefix = "", *alphabet = "0123456789abcdef";
        char c = *fmt++;
        if (c != '%') {
            n = lib_put(buf, size, n, c);
            continue;
        }
        for (; *fmt == '-' || *fmt == '0'; fmt++) {
            if (*fmt == '-') left = 1;
            else zero = 1;
        }
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + *fmt++ - '0';
        if (*fmt == '.') {
            fmt++;
            precision = 0;
            while (*fmt >= '0' && *fmt <= '9') precision = precision * 10 + *fmt++ - '0';
        }
        while (*fmt == 'l' || *fmt == 'h' || *fmt == 'z') fmt++;
        c = *fmt;
        if (!c) break;
        fmt++;
        if (c == '%') {
            n = lib_put(buf, size, n, c);
            continue;
        }
        if (c == 's') {
            text = va_arg(ap, char *);
            if (!text) text = "(null)";
            length = strlen(text);
            if (precision >= 0 && length > precision) length = precision;
        } else if (c == 'c') {
            *--text = va_arg(ap, int);
            length = 1;
        } else {
            value = va_arg(ap, unsigned int);
            if ((c == 'd' || c == 'i') && (int) value < 0) {
                prefix = "-";
                value = -value;
            }
            if (c == 'x' || c == 'X' || c == 'p') base = 16;
            if (c == 'o') base = 8;
            if (c == 'X') alphabet = "0123456789ABCDEF";
            if (c == 'p') prefix = "0x";
            do {
                *--text = alphabet[value % base];
                value /= base;
                length++;
            } while (value);
            if (precision >= 0) zeros = precision - length;
            else if (zero && !left) zeros = width - length - strlen(prefix);
            if (zeros < 0) zeros = 0;
        }
        width -= strlen(prefix) + zeros + length;
        if (!left)
            for (; width > 0; width--) n = lib_put(buf, size, n, ' ');
        while (*prefix) n = lib_put(buf, size, n, *prefix++);
        for (; zeros > 0; zeros--) n = lib_put(buf, size, n, '0');
        for (i = 0; i < length; i++) n = lib_put(buf, size, n, text[i]);
        for (; width > 0; width--) n = lib_put(buf, size, n, ' ');
    }
    if (size) buf[(size_t) n < size ? (size_t) n : size - 1] = 0;
    return n;
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return n;
}

/* ---- stdio.h: streams ---- */

/* Files are read through the host by name; a FILE only remembers a position. */
typedef struct FILE {
    char *name;
    long position;
} FILE;

/* stdout and stderr are only ever passed to fprintf, which prints to the log. */
static FILE lib_stdout, lib_stderr;
FILE *stdout = &lib_stdout, *stderr = &lib_stderr;

static char lib_text[4096];

int vfprintf(FILE *f, const char *fmt, va_list ap)
{
    int n = vsnprintf(lib_text, sizeof(lib_text), fmt, ap);
    host_print(lib_text);
    return n;
}

int fprintf(FILE *f, const char *fmt, ...)
{
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vfprintf(f, fmt, ap);
    va_end(ap);
    return n;
}

int printf(const char *fmt, ...)
{
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vfprintf(stdout, fmt, ap);
    va_end(ap);
    return n;
}

int puts(const char *s)
{
    return printf("%s\n", s);
}

int putchar(int c)
{
    printf("%c", c);
    return c;
}

static int lib_digit(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    c = tolower(c);
    return c >= 'a' && c <= 'f' ? c - 'a' + 10 : 99;
}

/* Whitespace, literal characters and the integer conversions %d %i %u %x %o. */
int sscanf(const char *s, const char *fmt, ...)
{
    va_list ap;
    int count = 0;
    va_start(ap, fmt);
    while (*fmt) {
        unsigned int value = 0, base = 10;
        int negative = 0, digits = 0;
        if (isspace(*fmt)) {
            while (isspace(*s)) s++;
            fmt++;
            continue;
        }
        if (*fmt != '%') {
            if (*s != *fmt) break;
            s++;
            fmt++;
            continue;
        }
        fmt++;
        if (*fmt == 'x') base = 16;
        if (*fmt == 'o') base = 8;
        if (*fmt == 'i') base = 0;
        fmt++;
        while (isspace(*s)) s++;
        if (*s == '-' || *s == '+') negative = *s++ == '-';
        if ((base == 0 || base == 16) && s[0] == '0' && tolower(s[1]) == 'x' &&
            lib_digit(s[2]) < 16) {
            s += 2;
            base = 16;
        }
        if (base == 0) base = *s == '0' ? 8 : 10;
        for (; (unsigned int) lib_digit(*s) < base; s++) {
            value = value * base + lib_digit(*s);
            digits++;
        }
        if (!digits) break;
        *va_arg(ap, int *) = negative ? -value : value;
        count++;
    }
    va_end(ap);
    return count;
}

int atoi(const char *s)
{
    int n = 0;
    sscanf(s, "%d", &n);
    return n;
}

FILE *fopen(const char *name, const char *mode)
{
    FILE *f;
    if (mode[0] != 'r' || host_size(name) < 0) return NULL;
    f = malloc(sizeof(FILE));
    f->name = strdup(name);
    f->position = 0;
    return f;
}

int fclose(FILE *f)
{
    return 0;
}

size_t fread(void *buffer, size_t size, size_t count, FILE *f)
{
    int n = host_read(f->name, buffer, f->position, size * count);
    if (n <= 0 || !size) return 0;
    f->position += n;
    return n / size;
}

size_t fwrite(const void *buffer, size_t size, size_t count, FILE *f)
{
    return 0;
}

int fseek(FILE *f, long offset, int whence)
{
    if (whence == SEEK_CUR) offset += f->position;
    if (whence == SEEK_END) offset += host_size(f->name);
    if (offset < 0) return -1;
    f->position = offset;
    return 0;
}

long ftell(FILE *f)
{
    return f->position;
}

int fflush(FILE *f)
{
    return 0;
}

int remove(const char *name)
{
    return -1;
}

int rename(const char *from, const char *to)
{
    return -1;
}
