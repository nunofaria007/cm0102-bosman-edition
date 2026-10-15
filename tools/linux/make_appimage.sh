#!/bin/bash
# Makes the Linux download: one x86-64 AppImage with the clean files. It runs in a linux/amd64 container
# (Docker, Colima or OrbStack), so the build does not depend on the host. The same AppImage runs on
# the Steam Deck in its desktop mode.
#
# Usage:
#   tools/linux/make_appimage.sh --game <clean files> [--manifest <file>] [--out <folder>] [--smoke <reference folder>]
#
#   --game   the clean files (tools/clean_files/make_clean_files.py). The container checks the
#            AppImage against the manifest.
#   --manifest  the manifest to check against. Default: tools/clean_files/manifest.tsv. Give the
#            merged download manifest when --game holds the Data-3.9.60 and Data-3.9.65 folders.
#   --out    output folder. Default: build/linux in the repo.
#   --smoke  also run tests/smoke/smoke_run.py in the container (classic mode, SDL dummy drivers)
#            against the reference pictures in this folder.
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
GAME=""; MANIFEST=""; OUT="$REPO/build/linux"; SMOKE=""
while [ $# -gt 0 ]; do
    case "$1" in
        --game) GAME="${2:-}"; shift 2 ;;
        --manifest) MANIFEST="${2:-}"; shift 2 ;;
        --out) OUT="${2:-}"; shift 2 ;;
        --smoke) SMOKE="${2:-}"; shift 2 ;;
        *) echo "make_appimage.sh: unknown option $1" >&2; exit 2 ;;
    esac
done
[ -f "$GAME/cm0102.exe" ] && [ -d "$GAME/Data" ] || { echo "make_appimage.sh: give --game <clean files>." >&2; exit 2; }
command -v docker >/dev/null || { echo "make_appimage.sh: Docker is not installed." >&2; exit 2; }
GAME="$(cd "$GAME" && pwd)"
[ -n "$MANIFEST" ] || MANIFEST="$REPO/tools/clean_files/manifest.tsv"
[ -f "$MANIFEST" ] || { echo "make_appimage.sh: no manifest file $MANIFEST." >&2; exit 2; }
MANIFEST="$(cd "$(dirname "$MANIFEST")" && pwd)/$(basename "$MANIFEST")"
mkdir -p "$OUT"; OUT="$(cd "$OUT" && pwd)"
VERSION="$(sed -n 's/^project([^)]*VERSION \([0-9][0-9.]*\).*/\1/p' "$REPO/CMakeLists.txt" | head -n 1)"

MOUNTS=(-v "$REPO:/src:ro" -v "$GAME:/game:ro" -v "$OUT:/out" -v "$MANIFEST:/manifest.tsv:ro" -v cm0102-linux-build:/build)
[ -n "$SMOKE" ] && MOUNTS+=(-v "$(cd "$SMOKE" && pwd):/smoke-ref:ro")
docker run --rm --platform linux/amd64 "${MOUNTS[@]}" -e VERSION="$VERSION" -e SMOKE="${SMOKE:+1}" \
    ubuntu:22.04 bash /src/tools/linux/in_container.sh
echo "== $OUT/CM-01-02-Bosman-Edition-$VERSION-linux-x86_64.AppImage"
