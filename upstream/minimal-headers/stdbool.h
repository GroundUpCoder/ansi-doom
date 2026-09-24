#ifndef STDBOOL_H
#define STDBOOL_H
/* DOOM's boolean is int; SDL3's API uses bool. */
#define bool int
#define true 1
#define false 0
#endif
