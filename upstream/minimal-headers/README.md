# What doom.c needs from each system header

`doom.c` includes ISO C headers by name and expects the compiler to provide
them. These files list, per header, the only declarations DOOM actually
uses (from `nm -u` on the compiled file plus the types and macros it
names). They are a specification for a compiler's built-in headers, not a
libc: every function here is left for the host to implement, or taken from
`lib.c` at the top of the repository, which replaces these headers too. Everything
non-standard DOOM needs (video, keys, clock, sleep, mkdir) is the `DG_*`
host interface declared at the top of `doom.c` itself.

Check that they are complete with:

    clang --target=i386-unknown-linux-gnu -std=c99 -nostdinc -Iminimal-headers \
          -isystem "$(clang -print-resource-dir)/include" -fsyntax-only doom.c

Assumed sizes: `char` 1, `short` 2, `int`/`long`/pointers 4, natural
alignment. `size_t` is `unsigned long` here to match `~/git/c-compiler`;
`unsigned int` is the same 4 bytes.

`stdarg.h` is deliberately not written out: `va_list`, `va_start`,
`va_arg` and `va_end` are whatever the compiler makes them. Every variadic
argument DOOM passes is 4 bytes (`int`, `unsigned`, or a pointer).
