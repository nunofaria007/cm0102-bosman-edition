#!/bin/bash
# Builds a real SDL2 dynamic library for one architecture, for the Mac app.
#
# Usage: tools/mac/build_sdl2.sh <SDL2 source folder> <install prefix> <arm64|x86_64>
#
# Why: the Homebrew SDL2 is sdl2-compat on SDL3. It is arm64 only and needs macOS 26, so the app cannot
# use it for a universal download that runs on older Macs. Build SDL2 2.30.x or newer from the official
# source release once per architecture, then give the prefixes to make_app.sh:
#   tools/mac/build_sdl2.sh ~/src/SDL2-2.30.x ~/sdl2-arm64 arm64
#   tools/mac/build_sdl2.sh ~/src/SDL2-2.30.x ~/sdl2-x86_64 x86_64
#   tools/mac/make_app.sh --game <clean files> --sdl2-arm64 ~/sdl2-arm64 --sdl2-x86_64 ~/sdl2-x86_64
# The libraries get the minimum macOS version 11.0, which is what the app declares.
set -euo pipefail
SRC="${1:?SDL2 source folder}"; PREFIX="${2:?install prefix}"; ARCH="${3:?arm64 or x86_64}"
[ -f "$SRC/CMakeLists.txt" ] || { echo "build_sdl2.sh: $SRC has no CMakeLists.txt" >&2; exit 2; }
BUILD="$(mktemp -d "${TMPDIR:-/tmp}/sdl2-build.XXXXXX")"
cmake -S "$SRC" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="$ARCH" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST=OFF
cmake --build "$BUILD" -j
cmake --install "$BUILD"
echo "SDL2 for $ARCH is in $PREFIX. The build folder $BUILD can be moved to the Trash."
