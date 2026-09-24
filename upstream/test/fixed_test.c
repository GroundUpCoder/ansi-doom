#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
typedef int fixed_t;
#define FRACBITS 16
fixed_t FixedMul(fixed_t a, fixed_t b);
fixed_t FixedDiv(fixed_t a, fixed_t b);
static fixed_t RefMul(fixed_t a, fixed_t b) { return ((int64_t) a * (int64_t) b) >> FRACBITS; }
static fixed_t RefDiv(fixed_t a, fixed_t b) {
    if ((abs(a) >> 14) >= abs(b)) return (a^b) < 0 ? INT_MIN : INT_MAX;
    return (fixed_t)(((int64_t) a << 16) / b);
}
static unsigned r(void) { static unsigned s = 12345; s = s * 1103515245 + 12345; return s; }
int main(void) {
    int i, bad = 0;
    for (i = 0; i < 20000000; i++) {
        int a = r(), b = r();
        switch (r() % 4) { case 0: a >>= r()%32; break; case 1: b >>= r()%32; break; case 2: a >>= r()%32; b >>= r()%32; break; }
        if (a == INT_MIN || b == INT_MIN) continue;
        if (FixedMul(a,b) != RefMul(a,b)) { if (bad++ < 5) printf("mul %d %d: %d vs %d\n", a, b, FixedMul(a,b), RefMul(a,b)); }
        if (b != 0 && FixedDiv(a,b) != RefDiv(a,b)) { if (bad++ < 5) printf("div %d %d: %d vs %d\n", a, b, FixedDiv(a,b), RefDiv(a,b)); }
    }
    printf("bad=%d\n", bad);
    return bad != 0;
}
