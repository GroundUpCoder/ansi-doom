#!/bin/bash
# Builds ../doom.c: all sources in sources.txt amalgamated into one C file.
#
# clang's preprocessor is run over a unity file that includes every source,
# with empty files standing in for the system headers. That expands DOOM's own headers and macros and resolves its
# platform #ifdefs, but leaves NULL, va_start, INT_MAX, size_t, FILE, the
# SDL API etc. untouched. The real #include lines are then put at the top.
#
# Usage: ./build.sh               -> writes doom.c, syntax-checks it with
#                                    clang against the real headers
#        ./build.sh --collisions  -> list file-local names that clash
#                                    across files (must be empty)
set -euo pipefail
cd "$(dirname "$0")"

# -U__GNUC__ so that PACKEDATTR (__attribute__((packed))) expands to nothing.
PPFLAGS=(-std=c99 -nostdinc -Ibuild/empty-include -Isrc -U__GNUC__
         -Wno-everything -Werror=macro-redefined)

# The system headers doom.c includes (header.txt puts the real lines at the
# top of doom.c). Empty files of the same names are created under
# build/empty-include so that the preprocessor finds them but expands them
# to nothing.
SYSTEM_HEADERS=(assert.h ctype.h errno.h fcntl.h inttypes.h limits.h math.h
                stdarg.h stdbool.h stddef.h stdint.h stdio.h stdlib.h string.h
                strings.h unistd.h sys/types.h sys/stat.h SDL3/SDL.h)

SRCS=()
while read -r f; do SRCS+=("$f"); done < sources.txt

mkdir -p build build/empty-include/sys build/empty-include/SDL3
for h in "${SYSTEM_HEADERS[@]}"; do : > "build/empty-include/$h"; done

# Real system headers, for the checks.
CHECKFLAGS=(-std=c99 -Wall -Werror=implicit-function-declaration -Werror=excess-initializers -Wno-unused -Wno-parentheses -Wno-pointer-sign
            -Wno-tautological-constant-out-of-range-compare -Wno-missing-braces
            -Wno-dangling-else -Wno-switch -Wno-empty-body -Wno-comment
            -Wno-unknown-pragmas -Wno-misleading-indentation
            -Wno-self-assign -Wno-deprecated-non-prototype)

if [[ "${1:-}" == "--collisions" ]]; then
    # Compile each file on its own, list its file-local (static) symbols,
    # then report any name that is static in one file and defined in another.
    for f in "${SRCS[@]}"; do
        clang "${CHECKFLAGS[@]}" -Isrc -c "src/$f" -o "build/${f%.c}.o"
        # macOS nm: names carry a leading '_', string literals are l_.str.N
        nm "build/${f%.c}.o" | awk -v f="$f" '
            $3 ~ /^l/ { next }
            { sub(/^_/, "", $3) }
            $2 ~ /^[tdbrs]$/ {print $3, "static", f}
            $2 ~ /^[TDBRS]$/ {print $3, "global", f}'
    done | sort | awk '
        { names[$1] = names[$1] " " $2 ":" $3; count[$1]++; if ($2 == "static") st[$1]++ }
        END { for (n in names) if (count[n] > 1 && st[n] > 0) print n ":" names[n] }
    ' | sort
    exit 0
fi

{
    for f in "${SRCS[@]}"; do
        echo "#include \"$f\""
    done
} > build/unity.c

{
    cat header.txt
    # -C keeps comments; -P drops line markers. Blank-line runs are squeezed.
    # In the unity file, a sized tentative definition reserves an array immediately.
    # Keep extern in the original headers for separate-file compilation.
    clang "${PPFLAGS[@]}" -E -P -C build/unity.c | cat -s |
        sed -E 's/^extern ([^;(]*\[[^;]*);$/\1;/'
} > ../doom.c

clang "${CHECKFLAGS[@]}" -fsyntax-only ../doom.c
echo "doom.c: $(wc -l < ../doom.c) lines, $(wc -c < ../doom.c) bytes"
