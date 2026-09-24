#include <stdio.h>
extern int later;
extern int arr[4];
int *p = &later;
int *q = arr;
int later = 7;
int arr[4];
int main(void) { printf("%d %d\n", *p, (int)(q == arr)); return 0; }
