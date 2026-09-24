#include <stdio.h>
extern int later;
void set(int *p) { *p = 9; }
void bind(void) { set(&later); }
int later = 7;
int main(void) { bind(); printf("%d\n", later); return 0; }
