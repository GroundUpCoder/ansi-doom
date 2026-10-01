#!/bin/bash
# Rebuilds doom.c, checks for gotos/name collisions, cast-finale timing and
# lib.c, then checks doom.c three ways:
#   1. FixedMul/FixedDiv (32-bit rewrite) against the 64-bit originals
#   2. ~/git/c-compiler: compile doom.c + dg_sdl.c, run -timedemo demo1 headless
#   3. system clang + SDL3: compile natively, run -timedemo demo1 with SDL's
#      dummy video driver
# A timedemo passes when it reports "timed 5026 gametics".
#
# Paths below are for this machine; override with environment variables.
set -euo pipefail
cd "$(dirname "$0")/.."

CC_REPO=${CC_REPO:-$HOME/git/c-compiler}
IWAD=${IWAD:-$PWD/../doom1.wad}
SDL3_INCLUDE=${SDL3_INCLUDE:-$HOME/git/small/build/native/SDL3-3.4.16/include}
SDL3_LIB=${SDL3_LIB:-$HOME/git/small/build/native/sdl/lib}

./build.sh
./build.sh --collisions | grep . && { echo "FAIL: file-local name collisions"; exit 1; }

if grep -nE '(^|[^[:alnum:]_])goto([^[:alnum:]_]|$)' ../doom.c; then
    echo "FAIL: goto in amalgamated source"
    exit 1
fi

echo "== cast-finale timing"
clang -std=c99 -O1 -Wno-deprecated-non-prototype -Wno-pointer-to-int-cast \
      -Wno-absolute-value -Wno-switch -o build/cast_test test/cast_test.c
./build/cast_test

echo "== lib.c"
# Natively: lib.c and its test are one translation unit, with no system
# headers except stdarg.h. -fno-builtin keeps clang from turning the byte
# loops of memcpy/memset into calls to themselves.
cat ../lib.c test/lib_test.c > build/lib_test.c
clang -std=c99 -O0 -ffreestanding -fno-builtin -Wall -Wno-unused \
      -Wno-non-literal-null-conversion -o build/lib_test build/lib_test.c
./build/lib_test || { echo "FAIL: lib.c"; exit 1; }
# With doom.c, as a 32-bit compiler without headers sees them: every system
# header is empty except the compiler's own stdarg.h.
mkdir -p build/lib-include
for h in ctype.h limits.h stdbool.h stddef.h stdint.h stdio.h stdlib.h string.h; do
    : > "build/lib-include/$h"
done
cat ../lib.c ../doom.c > build/lib_doom.c
clang --target=i386-unknown-linux-gnu -std=c99 -ffreestanding -fno-builtin \
      -nostdinc -Ibuild/lib-include -isystem "$(clang -print-resource-dir)/include" \
      -fsyntax-only -Wall -Werror -Wno-unused -Wno-parentheses -Wno-pointer-sign \
      -Wno-missing-braces -Wno-dangling-else -Wno-switch -Wno-empty-body \
      -Wno-comment -Wno-self-assign -Wno-deprecated-non-prototype \
      -Wno-tautological-constant-out-of-range-compare -Wno-misleading-indentation \
      -Wno-non-literal-null-conversion build/lib_doom.c \
      || { echo "FAIL: lib.c + doom.c"; exit 1; }

echo "== 1. fixed-point math"
{
    printf '#include <stdlib.h>\n#include <limits.h>\ntypedef int fixed_t;\n#define FRACBITS 16\n'
    sed -n '/^fixed_t$/,$p' src/m_fixed.c | grep -v '^#include'
} > build/fixed_impl.c
clang -O2 -o build/fixed_test test/fixed_test.c build/fixed_impl.c
./build/fixed_test

echo "== 2. reference compiler (~/git/c-compiler)"
rm -f build/doom.wasm
(cd "$CC_REPO" && node compiler.js "$OLDPWD/test/bin-sdl.json" -o "$OLDPWD/build/doom.wasm" 2>&1 | grep -v 'Circular include' || true)
[[ -f build/doom.wasm ]] || { echo "FAIL: reference compile"; exit 1; }
(cd "$CC_REPO" && node host.js "$OLDPWD/build/doom.wasm" --sdl=null -iwad "$IWAD" -timedemo demo1 2>&1 || true) | grep -E 'timed|Error' | tee build/ref-timedemo.txt
grep -q 'timed 5026 gametics' build/ref-timedemo.txt || { echo "FAIL: reference timedemo"; exit 1; }

echo "== 3. clang + SDL3"
clang -std=c99 -O1 -Wall -Wno-unused -Wno-parentheses -Wno-pointer-sign \
      -Wno-missing-braces -Wno-dangling-else -Wno-switch -Wno-empty-body \
      -Wno-comment -Wno-self-assign -Wno-deprecated-non-prototype \
      -Wno-tautological-constant-out-of-range-compare -Wno-misleading-indentation \
      -Wno-absolute-value -Wno-pointer-to-int-cast \
      -Isrc -I"$SDL3_INCLUDE" -L"$SDL3_LIB" -lSDL3 -Wl,-rpath,"$SDL3_LIB" \
      -o build/doom-native ../doom.c test/dg_sdl.c || { echo "FAIL: native compile"; exit 1; }
(cd build && SDL_VIDEO_DRIVER=dummy ./doom-native -iwad "$IWAD" -timedemo demo1 2>&1 || true) | grep -E 'timed|Error' | tee build/native-timedemo.txt
grep -q 'timed 5026 gametics' build/native-timedemo.txt || { echo "FAIL: native timedemo"; exit 1; }

echo "ALL PASS"
