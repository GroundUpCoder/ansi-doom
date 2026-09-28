# ansi-doom

DOOM in one C file. `doom.c` is the doomgeneric port of DOOM, patched to
need nothing but ISO C99 and seven host functions, and amalgamated into a
single 49k-line translation unit. It exists as a test program for small,
from-scratch C compilers: if your compiler builds `doom.c`, it runs DOOM.

- No floating point. No 64-bit integers. No bitfields. No `goto`. No sound.
- No `#define`, `#if`, or local `#include`: the only preprocessor lines are
  eight ISO C `#include`s at the top.
- Everything platform-specific is behind seven `extern` functions declared
  right after the includes.

`doom1.wad` is the shareware IWAD (episode 1), included so that everything
is in one place. See "License" below: it is id Software's data under the
shareware terms, not GPL.

## The host interface

```c
extern uint32_t *DG_ScreenBuffer;               /* 640x400 pixels, 0x00RRGGBB */
void DG_Init(void);
void DG_DrawFrame(void);                         /* show DG_ScreenBuffer */
void DG_SleepMs(uint32_t ms);
uint32_t DG_GetTicksMs(void);                    /* ms since start */
int DG_GetKey(int *pressed, unsigned char *key); /* next key event, or 0 */
void DG_SetWindowTitle(const char *title);
int DG_MakeDirectory(const char *path);          /* for savegames; may fail */
```

| Function | Contract |
|---|---|
| `DG_Init` | Called once, before the first frame. |
| `DG_DrawFrame` | Show `DG_ScreenBuffer`: 640x400 `uint32_t` pixels, `0x00RRGGBB`, row-major, allocated by DOOM with `malloc`. Called at most 35 times/s. |
| `DG_GetTicksMs` | Milliseconds since some fixed point; never decreases. |
| `DG_SleepMs` | Sleep. Called with 1 ms in wait loops. May be a no-op if the host drives `doomgeneric_Tick()` itself (see below). |
| `DG_GetKey` | Pop the next key event: write 1 (press) or 0 (release) to `*pressed` and the DOOM key code to `*key`, return 1. Return 0 if there is none. |
| `DG_SetWindowTitle` | May be a no-op. |
| `DG_MakeDirectory` | Create a directory (DOOM asks once, for savegames). Returning -1 is fine. |

`doom.c` also defines `main()`, which calls `doomgeneric_Create(argc,
argv)` and then loops on `doomgeneric_Tick()` forever. A host that cannot
block (a web page) skips `main` and calls those two itself, e.g. one
`doomgeneric_Tick()` per `requestAnimationFrame`, with `DG_SleepMs` a
no-op; DOOM paces its 35 Hz game logic from `DG_GetTicksMs` regardless.

`upstream/test/dg_sdl.c` is a complete host on SDL3 (about 200 lines).

### DOOM key codes

Letters, digits and punctuation are their lowercase ASCII codes. The rest
(`doomkeys.h`):

| Key | Code | Key | Code |
|---|---|---|---|
| Right / Left arrow | `0xae` / `0xac` | Enter / Escape / Tab | `13` / `27` / `9` |
| Up / Down arrow | `0xad` / `0xaf` | Backspace | `0x7f` |
| Fire (Ctrl) | `0xa3` | Shift | `0xb6` |
| Use (Space) | `0xa2` | Alt | `0xb8` |
| Strafe left / right | `0xa0` / `0xa1` | Equals / Minus | `0x3d` / `0x2d` |
| F1 .. F10 | `0xbb` .. `0xc4` | F11 / F12 | `0xd7` / `0xd8` |
| Pause | `0xff` | Home / End | `0xc7` / `0xcf` |
| Page up / down | `0xc9` / `0xd1` | Insert / Delete | `0xd2` / `0xd3` |
| Caps / Num / Scroll lock | `0xba` / `0xc5` / `0xc6` | Print screen | `0xd9` |

### The C library

41 functions and two variables, all standard: `abs atoi calloc exit fclose
fflush fopen fprintf fread free fseek ftell fwrite isspace malloc memcpy
memmove memset printf putchar puts realloc remove rename snprintf sscanf
strchr strcmp strdup strlen strncmp strncpy strrchr strstr system tolower
toupper vfprintf vsnprintf`, `stdout`, `stderr`.
`upstream/minimal-headers/` has the exact prototypes per header, as a spec
for a compiler's built-in headers. Semantics that matter for a minimal host:

- `fopen` must work for the IWAD (`./doom1.wad` with no arguments, or
  `-iwad path`), with `fread`, `fseek` (`SEEK_SET`/`SEEK_END`), `ftell`,
  `fclose`. Every other `fopen` (config, savegames, existence probes) may
  return `NULL`.
- `printf` and friends go to a log; `stdout`/`stderr` are only passed
  through, so any two distinct values work.
- `malloc`: a 6 MB zone at startup, the 1 MB screen buffer, and a few
  hundred small blocks. A bump allocator with a no-op `free` is enough.
- `remove`, `rename`, `fflush`, `system`, `sscanf`: stubs are fine.

## Building and running

With clang and SDL3, using the reference host:

```sh
clang -std=c99 -O2 -Iupstream/src doom.c upstream/test/dg_sdl.c -lSDL3 -o doom
./doom                     # uses ./doom1.wad
./doom -timedemo demo1     # plays the built-in demo and reports "timed 5026 gametics"
```

The `-timedemo demo1` run is deterministic (there is no floating point),
so it doubles as a correctness test for a compiler: the game logic of the
whole demo runs, and any miscompilation tends to end in `I_Error` or a
different gametic count. `upstream/test/run-tests.sh` runs it with
`~/git/c-compiler` and with clang+SDL3 (headless via `SDL_VIDEO_DRIVER=dummy`).
It also checks fixed-point arithmetic, absence of `goto` and symbol collisions,
and cast-finale timing (`upstream/test/cast_test.c`). The cast check covers
attack-stop paths that the shareware demo cannot exercise.

## What a compiler has to handle

Counts are occurrences in `doom.c`.

Present: `struct` (186), `union` (3), `enum` (60), `typedef` (195),
`static` (487, file scope and function scope), `extern` (491), `switch`
(109), `do`/`while`/`for`/`break`/`continue`, `?:` (220),
`sizeof` (219, on types and expressions), function pointers (including
tables of them and calls through unions), variadic functions (44 `...`, 9
`va_list`, standard `va_start`/`va_arg`/`va_end`), string literals with
escapes, char literals, nested brace initializers, `extern int x[];`,
multi-dimensional arrays, pointer arithmetic, casts, compound assignment,
`++`/`--`, the comma operator, declarations after statements, one `for
(int i ...)`, one octal literal, old-style declarations without a prototype
(`void A_Look();` later defined with a parameter), and `extern int x;`
followed by `int x;` in the same file.

Keywords that can be parsed and ignored: `const` (109), `register` (14),
`inline` (15, on static functions), `signed`.

Absent: `goto`, statement labels (other than `case`/`default`), `float`,
`double`, `long long`, bitfields, VLAs, designated initializers, compound literals, wide strings, `__attribute__`,
`__builtin_*`, `volatile`, inline asm.

Types: `char` 1, `short` 2, `int`/`long`/`enum`/pointer 4 (32-bit target;
64-bit works too, that is how the native build runs), natural alignment,
no packing needed. Every variadic argument is an `int`, `unsigned` or
pointer.

## What was changed against upstream

`upstream/orig/` is the untouched doomgeneric source; `upstream/src/` is
the patched one; `diff -ru upstream/orig upstream/src` shows everything.

| Where | Change | Why |
|---|---|---|
| `m_fixed.c` | `FixedMul`/`FixedDiv` rewritten with 32-bit unsigned math (16x16 split, shift-subtract division). Bit-exact with the `int64_t` originals (`upstream/test/fixed_test.c`, 20M random pairs). | no 64-bit ints |
| `i_video.c/h`, `v_video.c`, `m_config.c/h`, `i_sound.c`, `g_game.c` | `mouse_acceleration` and `libsamplerate_scale` become `int`; `M_GetFloatVariable` and the float config cases removed; timedemo fps printed as an int. | no floats (that was every real use; the rest were `#if 0`) |
| `i_video.h` | `struct color` bitfields (`uint32_t r:8` ...) become `uint8_t` fields, same layout. | no bitfields |
| `doomtype.h` | `boolean` is `int` over `<stdbool.h>` instead of `enum { false, true, undef }` (`undef` was unused). | `<stdbool.h>`'s `true`/`false` macros break the enum in one translation unit |
| `doomtype.h`, `m_misc.c`, 16 call sites | `strcasecmp`/`strncasecmp` (POSIX) replaced by DOOM's own `M_StrCaseCmp`/`M_StrNCaseCmp`. | ISO C only |
| `m_misc.c`, `doomgeneric.h`, `m_config.c` | `M_MakeDirectory` calls the new host function `DG_MakeDirectory` instead of `mkdir`; `M_FileExists` no longer consults `errno`. | no `<sys/stat.h>`, no `<errno.h>` |
| `doomfeatures.h` | `#undef FEATURE_SOUND`; `dg_sound.c` and the OPL emulator dropped. | no sound |
| `main.c` | The SDL platform code moved out to `upstream/test/dg_sdl.c`; `main` is `doomgeneric_Create` + `for (;;) doomgeneric_Tick();`. | the host owns the platform |
| `wi_stuff.c`, `am_map.c` | `anim_t`/`anims`/`load_callback_t` -> `wi_*`; `plr` -> `am_plr`. | file-local names that clash in one translation unit |
| `f_finale.c`, `p_enemy.c`, `p_map.c`, `r_bsp.c` | All 23 `goto`s in eight functions replaced with branches, early returns, a bounded loop, and one small helper. | no labels or `goto` support needed in the compiler |

The control-flow rewrite preserves the existing behavior:

- `F_CastTicker` uses `F_StopCastAttack` plus early returns at both former
  jumps; stopping an attack still leaves `casttics` unchanged.
- `A_Look` tracks whether a sound target was accepted before searching for
  players; `A_Chase` guards missile attacks with short-circuit conditions.
- `PTR_SlideTraverse` returns early for passable lines; `P_SlideMove` makes
  at most two slide attempts before the original stairstep fallback.
- `PTR_ShootTraverse` tracks whether a line blocks the shot, checking the
  floor before the ceiling and preserving missing-back-sector handling.
- `R_ClipSolidWallSegment` breaks out of its range scan before shared
  compaction; `R_AddLine` selects solid/pass clipping with early returns.

Checks retain their original evaluation order, including calls that update
state or consume random numbers. `upstream/orig/` retains the original
labels and jumps for comparison.

Unions, the blocking screen wipe, config files, savegames, demo playback
and command-line parsing are all still there.

## Regenerating doom.c

`doom.c` is generated; do not edit it by hand. `upstream/build.sh` runs
clang's preprocessor over the sources in `upstream/sources.txt` with empty
stand-ins for the system headers (so DOOM's macros expand but `stdio.h`
etc. do not), prepends `upstream/header.txt`, and syntax-checks the result.
The source order in `sources.txt` is dependency-driven: a file that takes
the address of a global comes after the file that defines it.
`./build.sh --collisions` lists file-local names that would clash (must be
empty).

## License

The code (`doom.c` and everything under `upstream/`) is GPL v2, like DOOM
and doomgeneric; see `LICENSE`. Copyright (C) 1993-1996 Id Software, Inc.;
(C) 2005-2014 Simon Howard (Chocolate Doom); (C) 2015 ozkl and
contributors (doomgeneric).

`doom1.wad` is not covered by the GPL. It is the data file of the DOOM
shareware release (v1.9, SHA-1
`5b2e249b9c5133ec987b3ea77596381dc0d6bc1d`), copyright (C) 1993 Id
Software, Inc., which id distributed for free redistribution under its
shareware terms (unmodified, not for sale). It is included here as a
convenience on that basis, as Debian's `doom-wad-shareware` package and
many DOOM source ports do.
