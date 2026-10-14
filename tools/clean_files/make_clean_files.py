#!/usr/bin/env python3
"""Make the clean files: the free release of CM 01/02 with the official 3.9.68 patch applied.

Usage:
    python3 tools/clean_files/make_clean_files.py ISO PATCH_EXE OUT_DIR [--manifest FILE]
    python3 tools/clean_files/make_clean_files.py --check OUT_DIR [--manifest FILE]

ISO is the free release (CM0102.iso). PATCH_EXE is the official patch (cm0102_3968.exe).
OUT_DIR must not exist. The tool stops if a hash of an input is wrong.
With --check, the tool compares OUT_DIR with the manifest and changes nothing.

Manifest format (tools/clean_files/manifest.tsv): lines starting with # are comments; each other
line is "sha256<TAB>size<TAB>path", with the path relative to OUT_DIR and "/" between folders.

Needs bsdtar (macOS has it) and unshield (Homebrew).

How the facts were found (2026-10-14, static analysis with the REA tools, no CrossOver run):
- update.dat: FUN_00403240 of the patch's setup.exe (the "-pack" path) writes 46 sizes, then the
  files. FUN_00403430 reads them back in the same order. The names and folders are the table at
  0x410070. The install loop at 0x40272a deletes each old file before it writes the new one.
- The patch asks "Delete the quick start files (recommended) ?" (FUN_00402ff0), so the clean files
  have no qstart folder.
- The folders of the cabinet's file groups come from the components in data1.hdr.
"""
import argparse
import hashlib
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_MANIFEST = os.path.join(HERE, "manifest.tsv")

ISO_SIZE = 303628288
ISO_SHA256 = "853134bbfc9e774424e62f9694ad7fe9dd7361c8bab93033f82fc906b31a5c53"
PATCH_SIZE = 23458816
PATCH_SHA256 = "b3f2b7d2ace6e86d0974e5ad68ae7a87a1e8fc3494c90d00cc3758392dcf24f1"

# File groups of data1.cab and the folder each one installs to. The folders come from the
# components in data1.hdr (<TARGETDIR>\Data and so on). Groups with no files (Exe, Help - *,
# Pictures Stadiums) are not listed. The quick start groups (~QS *) are left out, because the
# patch calls them out of date and recommends to delete them.
CAB_GROUPS = {
    "CM3 Data": "Data",
    "SI Data": "Data",
    "Tactics": "Data",
    "Pictures Std": "Pictures",
    "Pictures cfg": "Pictures",
    "Picture Tools": "",
    "Hall Of Fame": "",
    "History": "History",
    "Sounds": "Sounds",
    "Editor": "Editor",
}
CAB_FILE_COUNT = 779  # all groups, quick starts included

# The file table of the patch's setup.exe (46 entries at 0x410070: name, folder). update.dat is
# 46 little-endian 32-bit sizes in this order, then the files one after the other.
PATCH_FILES = [
    ("", "cm0102.exe"), ("", "cm0102_GDI.exe"),
    ("Data", "dan.lng"), ("Data", "eng.lng"), ("Data", "esp.lng"), ("Data", "fra.lng"),
    ("Data", "ita.lng"), ("Data", "nor.lng"), ("Data", "por.lng"), ("Data", "swe.lng"),
    ("Data", "hol.lng"), ("Data", "ger.lng"), ("Data", "language.ldb"),
    ("Data", "player_setup.cfg"), ("Data", "weather.cfg"), ("", "changes.txt"),
    ("Data", "city.dat"), ("Data", "club.dat"), ("Data", "club_comp.dat"),
    ("Data", "club_comp_history.dat"), ("Data", "colour.dat"), ("Data", "common_names.dat"),
    ("Data", "continent.dat"), ("Data", "first_names.dat"), ("Data", "index.dat"),
    ("Data", "nat_club.dat"), ("Data", "nation.dat"), ("Data", "nation_comp.dat"),
    ("Data", "nation_comp_history.dat"), ("Data", "officials.dat"), ("Data", "second_names.dat"),
    ("Data", "stadium.dat"), ("Data", "staff.dat"), ("Data", "staff_comp.dat"),
    ("Data", "staff_comp_history.dat"), ("Data", "staff_history.dat"), ("", "hall_of_fame.bin"),
    ("Editor", "cm0102ed.CNT"), ("Editor", "cm0102ed.exe"), ("Editor", "cm0102ed.exe.manifest"),
    ("Editor", "CM0102ED.GID"), ("Editor", "Cm0102ed.hlp"), ("Editor", "faq.txt"),
    ("Editor", "WhatsNew.txt"), ("Editor", "ZipDll.dll"), ("Editor", "ZipSFX.bin"),
]


def fail(msg):
    sys.exit("make_clean_files: " + msg)


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def check_input(path, size, digest, what):
    if not os.path.isfile(path):
        fail(f"{what} not found: {path}")
    if os.path.getsize(path) != size or sha256(path) != digest:
        fail(f"{what} has the wrong size or SHA-256: {path}")


def run(cmd, cwd):
    r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    if r.returncode != 0:
        fail(f"{cmd[0]} failed: {r.stderr.strip() or r.stdout.strip()}")
    return r.stdout


def install_cab(work, out):
    """Extract the free release's InstallShield cabinets and lay the files out as setup does."""
    iso_dir = os.path.join(work, "iso")
    os.mkdir(iso_dir)
    run(["bsdtar", "-xf", ARGS.iso, "data1.cab", "data1.hdr"], iso_dir)
    count = 0
    for group, folder in CAB_GROUPS.items():
        gdir = tempfile.mkdtemp(dir=work)
        run(["unshield", "-g", group, "-j", "-d", gdir, "x", os.path.join(iso_dir, "data1.cab")], iso_dir)
        dest = os.path.join(out, folder)
        os.makedirs(dest, exist_ok=True)
        # unshield puts the files in a folder named after the group; each group is flat.
        for d, _, files in os.walk(gdir):
            for name in sorted(files):
                src, dst = os.path.join(d, name), os.path.join(dest, name)
                if os.path.exists(dst):
                    # Pictures Std and Pictures cfg both hold the same pics.cfg.
                    if sha256(src) != sha256(dst):
                        fail(f"two groups give different files for {folder}/{name}")
                    continue
                os.replace(src, dst)
                count += 1
    if count == 0:
        fail("no files came out of data1.cab")
    return count


def apply_patch(work, out):
    """Do what the patch's setup.exe does: write each file of update.dat over the old one."""
    patch_dir = os.path.join(work, "patch")
    os.mkdir(patch_dir)
    run(["bsdtar", "-xf", ARGS.patch, "update.dat"], patch_dir)
    with open(os.path.join(patch_dir, "update.dat"), "rb") as f:
        sizes = struct.unpack("<46I", f.read(46 * 4))
        if 46 * 4 + sum(sizes) != os.path.getsize(f.name):
            fail("update.dat does not have the expected format")
        for (folder, name), size in zip(PATCH_FILES, sizes):
            dest_dir = os.path.join(out, folder)
            os.makedirs(dest_dir, exist_ok=True)
            # setup.exe deletes the old file, then writes the new one with its own spelling.
            for old in os.listdir(dest_dir):
                if old.lower() == name.lower():
                    os.remove(os.path.join(dest_dir, old))
            with open(os.path.join(dest_dir, name), "wb") as g:
                g.write(f.read(size))


def exe_version(path):
    """The version text that the game shows. The exe has no version resource, only the string
    "3.9.xx" in its data. Returns it if there is exactly one, else None."""
    data = open(path, "rb").read()
    found = set(re.findall(rb"\x003\.9\.(\d\d)\x00", data))
    return f"3.9.{found.pop().decode()}" if len(found) == 1 else None


def listing(root):
    rows = []
    for d, dirs, files in os.walk(root):
        dirs.sort()
        for name in files:
            p = os.path.join(d, name)
            rows.append((os.path.relpath(p, root), os.path.getsize(p), sha256(p)))
    return sorted(rows, key=lambda r: r[0].lower())


def write_manifest(rows, path):
    with open(path, "w") as f:
        f.write("# Clean files: CM0102.iso with cm0102_3968.exe applied. Made by make_clean_files.py.\n")
        f.write("# sha256\tsize\tpath\n")
        for rel, size, digest in rows:
            f.write(f"{digest}\t{size}\t{rel}\n")


def read_manifest(path):
    rows = []
    for line in open(path):
        if line.startswith("#") or not line.strip():
            continue
        digest, size, rel = line.rstrip("\n").split("\t", 2)
        rows.append((rel, int(size), digest))
    return rows


def check(out, manifest):
    want = read_manifest(manifest)
    have = listing(out)
    if have == want:
        print(f"ok: {len(have)} files match {manifest}")
        return 0
    wd, hd = {r[0]: r for r in want}, {r[0]: r for r in have}
    for rel in sorted(set(wd) | set(hd)):
        if wd.get(rel) != hd.get(rel):
            print("differs:", rel)
    return 1


def main():
    global ARGS
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--check", metavar="OUT_DIR")
    ap.add_argument("--manifest", default=DEFAULT_MANIFEST)
    ap.add_argument("iso", nargs="?")
    ap.add_argument("patch", nargs="?")
    ap.add_argument("out", nargs="?")
    ARGS = ap.parse_args()
    if ARGS.check:
        return check(ARGS.check, ARGS.manifest)
    if not (ARGS.iso and ARGS.patch and ARGS.out):
        ap.error("give ISO, PATCH_EXE and OUT_DIR, or --check OUT_DIR")
    for tool in ("bsdtar", "unshield"):
        if not shutil.which(tool):
            fail(f"{tool} not found (brew install {tool})")
    out = os.path.abspath(ARGS.out)
    if os.path.exists(out):
        fail(f"{out} exists; give a new folder")
    check_input(ARGS.iso, ISO_SIZE, ISO_SHA256, "free release")
    check_input(ARGS.patch, PATCH_SIZE, PATCH_SHA256, "official patch")

    parent = os.path.dirname(out)
    os.makedirs(parent, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=parent, prefix=".clean-files-") as work:
        stage = os.path.join(work, "out")
        os.mkdir(stage)
        n = install_cab(work, stage)
        apply_patch(work, stage)
        version = exe_version(os.path.join(stage, "cm0102.exe"))
        if version != "3.9.68":
            fail(f"the patched cm0102.exe reports {version}, not 3.9.68")
        os.replace(stage, out)
    rows = listing(out)
    write_manifest(rows, ARGS.manifest)
    print(f"{n} files from data1.cab, {len(PATCH_FILES)} from update.dat; {len(rows)} files in {out}")
    print(f"cm0102.exe version {version}; manifest {ARGS.manifest}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
