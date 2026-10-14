#!/bin/bash
# Builds both downloads of one release, checks them and, with --publish, uploads a GitHub release.
#
# Usage:
#   tools/release/make_release.sh --game <clean files> --sdl2-mac <folder> --sdl2-mingw <folder> --site-assets <folder> [--publish]
#
#   --game         the clean files (tools/clean_files/make_clean_files.py). The script checks them
#                  against tools/clean_files/manifest.tsv.
#   --sdl2-mac     the folder with mac-arm64 and mac-x86_64 SDL2 builds (tools/mac/build_sdl2.sh)
#   --sdl2-mingw   the SDL2 mingw development release (SDL2-2.x.y)
#   --site-assets  the site's pictures (logo, screenshots). They go into site-assets.zip on the release,
#                  where the Pages workflow fetches them, so git never holds them.
#   --publish      tag v<version>, push the tag and upload the release. Without it the script only builds
#                  and checks, and prints what it would upload.
#
# The version is the one in CMakeLists.txt. CHANGELOG.md must hold a "## <version>" section, which
# becomes the release notes. Versions below 1.0.0 go up as pre-releases.
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
GH_REPO="maxevans60/cm0102-re-signed-edition"
GAME=""; SDL_MAC=""; SDL_MINGW=""; SITE=""; PUBLISH=0
while [ $# -gt 0 ]; do
    case "$1" in
        --game) GAME="$2"; shift 2 ;;
        --sdl2-mac) SDL_MAC="$2"; shift 2 ;;
        --sdl2-mingw) SDL_MINGW="$2"; shift 2 ;;
        --site-assets) SITE="$2"; shift 2 ;;
        --publish) PUBLISH=1; shift ;;
        *) echo "make_release.sh: unknown option $1" >&2; exit 2 ;;
    esac
done
for v in GAME SDL_MAC SDL_MINGW SITE; do
    [ -n "${!v}" ] || { echo "make_release.sh: give --$(echo $v | tr 'A-Z_' 'a-z-' | sed 's/sdl-/sdl2-/')." >&2; exit 2; }
done
[ -f "$GAME/cm0102.exe" ] || { echo "make_release.sh: $GAME has no cm0102.exe." >&2; exit 2; }

VERSION="$(sed -n 's/^project([^)]*VERSION \([0-9][0-9.]*\).*/\1/p' "$REPO/CMakeLists.txt" | head -n 1)"
TAG="v$VERSION"
NOTES="$(awk -v v="## $VERSION" 'index($0, v) == 1 { on = 1; next } /^## / && on { exit } on' "$REPO/CHANGELOG.md")"
[ -n "$NOTES" ] || { echo "make_release.sh: CHANGELOG.md has no section for $VERSION." >&2; exit 2; }
if [ -n "$(git -C "$REPO" status --porcelain --untracked-files=no)" ]; then
    echo "make_release.sh: commit your changes first, so that the tag names the code that was built." >&2; exit 2
fi

OUT="$REPO/build/release-$VERSION"
mkdir -p "$OUT"
MANIFEST="$REPO/tools/clean_files/manifest.tsv"
CHECK_ARGS=(--manifest "$MANIFEST")

echo "== Mac"
"$REPO/tools/mac/make_app.sh" --game "$GAME" --sdl2-arm64 "$SDL_MAC/mac-arm64" --sdl2-x86_64 "$SDL_MAC/mac-x86_64" \
    --out "$REPO/build/mac"
DMG="$REPO/build/mac/CM-01-02-Re-Signed-Edition-$VERSION-mac.dmg"
python3 -I "$REPO/tools/mac/check_package.py" "$DMG" ${CHECK_ARGS[@]+"${CHECK_ARGS[@]}"}
cp "$DMG" "$OUT/"

echo "== Windows"
cmake -B "$REPO/build-win" -DCMAKE_TOOLCHAIN_FILE="$REPO/packaging/windows/mingw-w64-x86_64.cmake" \
    -DSDL2_DIR="$SDL_MINGW/x86_64-w64-mingw32/lib/cmake/SDL2" -DCM0102_EXE="$GAME/cm0102.exe" \
    -DCM0102_GAME_DIR="$GAME" >/dev/null
cmake --build "$REPO/build-win" -j --target windows_installer
SETUP="$REPO/build-win/CM0102-Re-Signed-Edition-$VERSION-Setup.exe"
cp "$SETUP" "$OUT/"

echo "== Site pictures"
( cd "$SITE" && zip -q -r -X "$OUT/site-assets.zip" . -x '.*' )

( cd "$OUT" && shasum -a 256 "$(basename "$DMG")" "$(basename "$SETUP")" site-assets.zip > SHA256SUMS )
printf '%s\n' "$NOTES" > "$OUT/notes.md"
echo "== Built in $OUT"
ls -la "$OUT"

if [ "$PUBLISH" != 1 ]; then
    echo "Dry run. Run again with --publish to tag $TAG and upload these files to $GH_REPO."
    exit 0
fi

PRE=()
case "$VERSION" in 0.*) PRE=(--prerelease) ;; esac
git -C "$REPO" tag -a "$TAG" -m "CM 01/02 Re-Signed Edition $VERSION"
git -C "$REPO" push origin "$TAG"
gh release create "$TAG" --repo "$GH_REPO" ${PRE[@]+"${PRE[@]}"} --title "CM 01/02 Re-Signed Edition $VERSION" \
    --notes-file "$OUT/notes.md" "$OUT/$(basename "$DMG")" "$OUT/$(basename "$SETUP")" "$OUT/site-assets.zip" "$OUT/SHA256SUMS"
echo "Published $TAG."
