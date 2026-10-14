"""Compares the recompiled database load with the Unicorn run of the original, byte for byte.

Usage:
    python tests/recomp/test_load.py <cm0102.exe> <data folder> <recompiled load_driver program> [<work folder>]

Build the program first:
    python tools/recomp/recomp.py --exe <cm0102.exe> --out build/recomp/load \\
        --functions build/recomp/functions.txt --entries 0x528540 --compile tests/recomp/load_driver.c

Both runs start from the same memory: the PE image, the import address table pointing at the same stub
addresses, a bump heap at 0x70000000 and the stack at 0x20000000. After the load the test compares the
whole image (code, .rdata, .data and .bss with every table pointer and count), the whole heap (every table),
the stack and the page of fs:[0]. It then names each table of the load with its rows.
"""
import os
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from reference import HEAP_BASE, STACK_BASE, STACK_TOP, run_load  # noqa: E402

# Table pointer, count global, row size in memory (docs/research/2026-10-14-game-creation-inventory.md 2b).
TABLES = [
    ("club and nat club", 0xAE23B4, 0xAE235C, 581), ("colour", 0xAE23EC, 0xAE2394, 58),
    ("continent", 0xAE23A4, 0xAE234C, 198), ("nation", 0xAE23A8, 0xAE2350, 290),
    ("officials", 0xAE23E8, 0xAE2390, 43), ("stadium", 0xAE23B0, 0xAE2358, 78),
    ("staff", 0xAE23BC, 0xAE2364, 110), ("non-player", 0xAE23C0, 0xAE2368, 68),
    ("player", 0xAE23C4, 0xAE236C, 70), ("staff comp", 0xAE23E0, 0xAE2388, 101),
    ("city", 0xAE23AC, 0xAE2354, 56), ("club comp", 0xAE23D0, 0xAE2378, 107),
    ("first names", 0xAE23F0, 0xAE2398, 60), ("second names", 0xAE23F4, 0xAE239C, 60),
    ("common names", 0xAE23F8, 0xAE23A0, 60), ("staff history", 0xAE23CC, 0xAE2374, 17),
    ("staff comp history", 0xAE23E4, 0xAE238C, 58), ("comp history", 0xAE23D4, 0xAE237C, 26),
    ("preferences", 0xAE23C8, 0xAE2370, 52),
]


def first_difference(a, b):
    if len(a) != len(b):
        return min(len(a), len(b))
    lo, hi = 0, len(a)
    # Find the first differing byte by halving, then count the differing bytes.
    while hi - lo > 4096:
        mid = (lo + hi) // 2
        if a[lo:mid] != b[lo:mid]:
            hi = mid
        else:
            lo = mid
    for i in range(lo, hi):
        if a[i] != b[i]:
            return i
    return None


def main(argv):
    if len(argv) < 4:
        raise SystemExit(__doc__)
    exe, data, program = argv[1], argv[2].rstrip("/"), argv[3]
    work = argv[4] if len(argv) > 4 else tempfile.mkdtemp(prefix="recomp-load-")
    os.makedirs(work, exist_ok=True)

    t = time.time()
    out = subprocess.run([program, exe, data, work], capture_output=True, text=True)
    wall = time.time() - t
    if out.returncode:
        raise SystemExit(f"the recompiled load failed:\n{out.stderr}")
    words = out.stdout.split()
    result, heap_top, seconds = int(words[1]), int(words[3], 16), float(words[5])
    print(f"recompiled: returned {result}, heap top {heap_top:#x}, load {seconds:.3f} s (process {wall:.2f} s)")

    ref = run_load(exe, data)
    print(f"unicorn:    returned {ref.result}, heap top {ref.heap_top:#x}, load {ref.seconds:.2f} s")

    failed = 0
    if (result, heap_top) != (ref.result, ref.heap_top):
        print("FAIL  return value or heap size differ")
        failed += 1
    image = ref.image
    regions = [
        ("image", image.base, image.base + image.size, "image.bin"),
        ("heap", HEAP_BASE, ref.heap_top, "heap.bin"),
        ("stack", STACK_BASE, STACK_TOP, "stack.bin"),
        ("fs page", 0, 0x1000, "page0.bin"),
    ]
    for name, start, end, path in regions:
        mine = open(os.path.join(work, path), "rb").read()
        theirs = ref.read(start, end - start)
        if name == "image":
            # The Unicorn run writes a ret over the entry of each replaced function; put the bytes back.
            theirs = bytearray(theirs)
            for addr in ref.replacements():
                theirs[addr - start:addr - start + 3] = image.read(addr, 3)
            theirs = bytes(theirs)
        at = first_difference(theirs, mine)
        if at is None:
            print(f"ok    {name}: {end - start:,} bytes the same")
            continue
        failed += 1
        count = sum(1 for x, y in zip(theirs, mine) if x != y)
        print(f"FAIL  {name}: {count:,} bytes differ, first at {start + at:#x}: "
              f"original {theirs[at:at + 16].hex()} recompiled {mine[at:at + 16].hex()}")

    heap = open(os.path.join(work, "heap.bin"), "rb").read()
    for name, ptr, count_at, size in TABLES:
        base, rows = ref.u32(ptr), ref.u32(count_at)
        if not HEAP_BASE <= base < ref.heap_top:
            print(f"      table {name}: pointer {base:#x} is not in the heap")
            continue
        off = base - HEAP_BASE
        same = heap[off:off + rows * size] == ref.read(base, rows * size)
        print(f"{'ok   ' if same else 'FAIL '} table {name}: {rows:,} rows of {size} bytes at {base:#x}")
        failed += not same
    print("PASS" if not failed else f"FAIL ({failed})")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
