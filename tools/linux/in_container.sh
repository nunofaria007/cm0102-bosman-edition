#!/bin/bash
# Runs inside the linux/amd64 container of make_appimage.sh. Mounts: /src (repo, read-only), /game (clean
# files, read-only), /out (output), /manifest.tsv (the manifest to check against), /smoke-ref (reference pictures, only with --smoke).
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq --no-install-recommends build-essential cmake ninja-build python3 python3-venv \
    python3-pip libsdl2-dev wget ca-certificates file squashfs-tools zstd >/dev/null
python3 -m venv /venv
/venv/bin/pip install -q capstone

# 1. Build. The source is read-only, so the build folder is in the container.
cmake -S /src -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCM0102_EXE=/game/cm0102.exe \
    -DRECOMP_PYTHON=/venv/bin/python >/dev/null
cmake --build /build --target cm0102_recomp

# 2. The AppDir: program, SDL2, clean files, launcher, desktop file, icon.
A=/work/CM-01-02.AppDir
mkdir -p "$A/usr/bin" "$A/usr/lib" "$A/usr/share/game"
cp /build/cm0102_recomp "$A/usr/bin/"
strip "$A/usr/bin/cm0102_recomp"
ldd "$A/usr/bin/cm0102_recomp" | awk '/libSDL2/ { print $3 }' | xargs -r -I{} cp -L {} "$A/usr/lib/"
cp -r /game/. "$A/usr/share/game/"
cat > "$A/AppRun" <<'RUN'
#!/bin/sh
HERE="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$HERE/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec "$HERE/usr/bin/cm0102_recomp" --game "$HERE/usr/share/game" "$@"
RUN
chmod +x "$A/AppRun"
cat > "$A/cm0102.desktop" <<'DESK'
[Desktop Entry]
Type=Application
Name=CM 01/02 Bosman Edition
Exec=AppRun
Icon=cm0102
Categories=Game;
DESK
# A plain placeholder icon (the original's icon is not in git).
python3 - "$A/cm0102.png" <<'PY'
import struct, sys, zlib
w = 64
raw = b"".join(b"\0" + bytes([30, 90, 50]) * w for _ in range(w))
def ch(t, d): return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d))
open(sys.argv[1], "wb").write(b"\x89PNG\r\n\x1a\n" + ch(b"IHDR", struct.pack(">IIBBBBB", w, w, 8, 2, 0, 0, 0))
                              + ch(b"IDAT", zlib.compress(raw)) + ch(b"IEND", b""))
PY

# 3. The AppImage: the type 2 runtime (pinned by hash), then the squashfs of the AppDir. appimagetool is
# not used, because it does not run under the Rosetta of the container.
cd /work
wget -q -O runtime https://github.com/AppImage/type2-runtime/releases/download/20251108/runtime-x86_64
echo "2fca8b443c92510f1483a883f60061ad09b46b978b2631c807cd873a47ec260d  runtime" | sha256sum -c -
NAME="CM-01-02-Bosman-Edition-$VERSION-linux-x86_64.AppImage"
mksquashfs "$A" /work/app.sqfs -root-owned -noappend -comp zstd -quiet
cat runtime app.sqfs > "/out/$NAME"
chmod +x "/out/$NAME"

# 4. Check: x86-64 ELF, and every game file matches the manifest, none missing or extra.
file "/out/$NAME" | grep -q "x86-64" || { echo "check: not an x86-64 AppImage" >&2; exit 1; }
rm -rf /work/x && mkdir -p /work/x && unsquashfs -q -d /work/x/squashfs-root -o "$(stat -c %s /work/runtime)" "/out/$NAME" >/dev/null
python3 -I - /manifest.tsv /work/x/squashfs-root/usr/share/game <<'PY'
import hashlib, os, sys
want = {}
for l in open(sys.argv[1]):
    if l.startswith("#") or not l.strip(): continue
    h, n, p = l.rstrip("\n").split("\t", 2)
    want[p] = (h, int(n))
have = {}
for d, _, fs in os.walk(sys.argv[2]):
    for f in fs:
        full = os.path.join(d, f)
        have[os.path.relpath(full, sys.argv[2])] = full
bad = [p for p in want if p not in have]
bad += [p for p in have if p not in want]
bad += [p for p in want if p in have and (os.path.getsize(have[p]) != want[p][1]
        or hashlib.sha256(open(have[p], "rb").read()).hexdigest() != want[p][0])]
if bad:
    print("check: files differ from the manifest:", bad[:10]); sys.exit(1)
print("check: %d game files match the manifest" % len(want))
PY
ls -la "/out/$NAME"

# 5. Smoke run with the SDL dummy drivers (only with --smoke).
if [ -n "${SMOKE:-}" ]; then
    python3 -I /src/tests/smoke/smoke_run.py /work/x/squashfs-root/usr/bin/cm0102_recomp /game /smoke-ref --mode classic --tolerance 1
fi
