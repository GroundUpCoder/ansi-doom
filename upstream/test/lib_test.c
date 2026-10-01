/*
 * Checks lib.c with a real compiler. run-tests.sh compiles lib.c followed by
 * this file as one translation unit, so there are no system headers here:
 * the host functions sit directly on write() and _exit().
 */
long write(int fd, const void *buffer, unsigned long count);
void _exit(int status);

static const char lib_test_file[] = "0123456789";
static int lib_test_bad;

void host_exit(int status) { _exit(status); }
void host_print(const char *text) { write(1, text, strlen(text)); }
int host_size(const char *name) { return strcmp(name, "./digits") ? -1 : 10; }
int host_read(const char *name, void *buffer, int offset, int count)
{
    if (host_size(name) < 0) return -1;
    if (offset > 10) offset = 10;
    if (count > 10 - offset) count = 10 - offset;
    memcpy(buffer, lib_test_file + offset, count);
    return count;
}

static void check(int ok, const char *what)
{
    if (!ok) {
        lib_test_bad++;
        printf("FAIL: %s\n", what);
    }
}

static void format(void)
{
    char b[64];
    check(snprintf(b, 64, "%d %i %u", -42, 0, 3000000000u) == 16 &&
          !strcmp(b, "-42 0 3000000000"), "decimal");
    check(snprintf(b, 64, "%x %X %o %p", 255, 255, 8, (void *) 16) == 13 &&
          !strcmp(b, "ff FF 10 0x10"), "bases");
    snprintf(b, 64, "[%7i] [%-4d] [%02i] [%05d]", 12, 3, -4, -42);
    check(!strcmp(b, "[     12] [3   ] [-4] [-0042]"), "width");
    snprintf(b, 64, "%.2d %.3d %2.2d %04x", 3, 12, 7, 255);
    check(!strcmp(b, "03 012 07 00ff"), "precision");
    snprintf(b, 64, "%s|%.8s|%6s|%-6s|%c|%%", "doom", "doomabcdxyz", "ab", "cd", 'x');
    check(!strcmp(b, "doom|doomabcd|    ab|cd    |x|%"), "strings");
    snprintf(b, 64, "%d %ld %s", -2147483647 - 1, 7L, (char *) 0);
    check(!strcmp(b, "-2147483648 7 (null)"), "limits");
    memset(b, 'z', 64);
    check(snprintf(b, 4, "%d", 123456) == 6 && !strcmp(b, "123") && b[4] == 'z', "truncation");
    check(snprintf(b, 0, "abc") == 3 && b[0] == '1', "size zero writes nothing");
}

static void scan(void)
{
    int a = 0, b = 0;
    unsigned int u = 0;
    check(sscanf("10", " 0x%x", &a) == 0, "literal mismatch");
    check(sscanf(" 0x1F", " 0x%x", &a) == 1 && a == 31, "hex after literal");
    check(sscanf("0X1f", "%x", &a) == 1 && a == 31, "hex prefix");
    check(sscanf(" 017", " 0%o", &a) == 1 && a == 15, "octal");
    check(sscanf("  -12 34", "%d %d", &a, &b) == 2 && a == -12 && b == 34, "decimal");
    check(sscanf("017", "%i", &a) == 1 && a == 15, "%i octal");
    check(sscanf("0x20", "%i", &a) == 1 && a == 32, "%i hex");
    check(sscanf("42", "%i", &a) == 1 && a == 42, "%i decimal");
    check(sscanf("0", "%i", &a) == 1 && a == 0, "%i zero");
    check(sscanf("x", "%d", &a) == 0, "no digits");
    check(sscanf("4294967295", "%u", &u) == 1 && u == 4294967295u, "unsigned");
    check(atoi("  123abc") == 123 && atoi("-7") == -7 && atoi("abc") == 0, "atoi");
}

static void strings(void)
{
    char b[16];
    const char *s = "hello world";
    check(strlen("") == 0 && strlen(s) == 11, "strlen");
    check(strcmp("a", "a") == 0 && strcmp("a", "b") < 0 && strcmp("b", "a") > 0 &&
          strcmp("a", "ab") < 0 && strcmp("\xff", "a") > 0, "strcmp");
    check(strncmp("abc", "abd", 2) == 0 && strncmp("abc", "abd", 3) < 0 &&
          strncmp("ab", "ab", 9) == 0 && strncmp("a", "b", 0) == 0, "strncmp");
    memset(b, 'z', 16);
    strncpy(b, "ab", 5);
    check(b[0] == 'a' && b[1] == 'b' && !b[2] && !b[3] && !b[4] && b[5] == 'z', "strncpy");
    check(strchr(s, 'o') == s + 4 && strchr(s, 'q') == NULL && strchr(s, 0) == s + 11, "strchr");
    check(strrchr(s, 'o') == s + 7 && strrchr(s, 'q') == NULL, "strrchr");
    check(strstr(s, "world") == s + 6 && strstr(s, "") == s && strstr(s, "worlds") == NULL,
          "strstr");
    check(toupper('a') == 'A' && toupper('1') == '1' && tolower('Z') == 'z' && isspace('\n') &&
          isspace(' ') && !isspace('a') && !isspace(0) && abs(-3) == 3 && abs(3) == 3, "ctype");
    memcpy(b, "0123456789", 11);
    memmove(b + 2, b, 5);
    check(!strcmp(b, "0101234789"), "memmove up");
    memmove(b, b + 2, 5);
    check(!strcmp(b, "0123434789"), "memmove down");
}

static void heap(void)
{
    int *p = calloc(4, sizeof(int)), *q;
    char *s = strdup("doom");
    check(p && !p[0] && !p[3] && ((unsigned long) p & 3) == 0, "calloc");
    p[2] = 7;
    q = realloc(p, 64);
    check(q && q != p && q[2] == 7 && q[0] == 0, "realloc grows");
    q = realloc(q, 12);
    check(q[2] == 7, "realloc shrinks");
    free(q);
    check(!strcmp(s, "doom"), "strdup");
    check(realloc(NULL, 8) != NULL, "realloc from null");
    check(malloc(64 * 1024 * 1024) == NULL, "too large");
    check(malloc(24 * 1024 * 1024) != NULL && malloc(24 * 1024 * 1024) == NULL, "exhaustion");
    check(malloc(16) != NULL, "small block still fits");
}

static void files(void)
{
    char b[8] = {0};
    FILE *f = fopen("./digits", "rb"), *g = fopen("./digits", "r");
    check(fopen("missing", "rb") == NULL, "missing file");
    check(fopen("./digits", "wb") == NULL, "no writing");
    check(f && g && f != g, "fopen");
    check(fread(b, 1, 4, f) == 4 && !strcmp(b, "0123") && ftell(f) == 4, "fread");
    check(fread(b, 1, 2, g) == 2 && b[0] == '0' && ftell(g) == 2, "independent cursors");
    check(fread(b, 4, 2, f) == 1 && ftell(f) == 10, "short read counts whole items");
    check(fread(b, 1, 1, f) == 0, "end of file");
    check(fseek(f, -3, SEEK_END) == 0 && fread(b, 1, 8, f) == 3 && b[0] == '7', "SEEK_END");
    check(fseek(f, 2, SEEK_SET) == 0 && fseek(f, 3, SEEK_CUR) == 0 && ftell(f) == 5, "SEEK_CUR");
    check(fseek(f, -1, SEEK_SET) == -1 && ftell(f) == 5, "negative seek");
    check(fclose(f) == 0 && fclose(g) == 0 && fflush(stdout) == 0, "fclose");
    check(fwrite(b, 1, 1, stdout) == 0 && remove("x") == -1 && rename("x", "y") == -1 &&
          system("true") == -1, "stubs");
    check(stdout != stderr, "streams");
}

int main(void)
{
    format();
    scan();
    strings();
    heap();
    files();
    puts("lib.c:");
    putchar(' ');
    fprintf(stderr, "bad=%d\n", lib_test_bad);
    return lib_test_bad != 0;
}
