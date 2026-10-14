#!/usr/bin/env python3
"""Smoke run: the recompiled game plays the start screen, a new game and some days without a person.

Usage:
    python3 tests/smoke/smoke_run.py <cm0102_recomp> <game folder> <reference folder> [--mode classic|widescreen] [--record]

The run uses a new empty save folder (RECOMP_USER_DIR is a temporary folder), so the game first shows its
settings. Before the start it writes settings.txt with the mode (default: classic). It saves five
screens. With --record it writes them to <reference folder>/<mode>. Without it, it compares them with
the reference pictures there. The reference pictures stay outside git, because they show the
original's screens.

Widescreen mode draws at 1280x800 and the runtime scales every area by 1280/800 across and 800/600
down, so the run scales each click and the date box by the same factors.

Screens and checks:
    1 settings   the first-start settings screen: up to 20% of pixels differ (random background)
    2 setup      the Setup Game start screen: exact match
    3 leagues    Select League(s): exact match
    4 news       the first news screen of the new game, Saturday 14.7.01 AM: exact match
    5 holiday    30 s after "Go on Holiday": the date box differs from screen 4, so days passed

The run sets Background Changes to Off on the settings screen, so screens 2 to 4 repeat. The days
pass on holiday, because Continue Game stops at random events (an offer for a player) that need an
answer. The news of the days is random, so screen 5 is only checked for a new date. In 30 s the
game passes about two weeks, with more than five days in every run so far.
"""
import os
import shutil
import struct
import subprocess
import sys
import tempfile

# Positions for classic mode, 800x600. Times in ms from the window opening. The database load after
# Select League(s) takes about 20 s, so the steps after it wait with a margin.
STEPS = [
    "8000 dump {d}/1-settings.bmp",
    "8200 click 522 272",                   # settings: Background Changes Off
    "8500 click 662 524",                   # settings: Ok
    "11000 dump {d}/2-setup.bmp",
    "11500 click 277 177",                  # Start New Game
    "14000 dump {d}/3-leagues.bmp",
    "14500 click 705 572",                  # Next (England is selected)
    "34000 move 399 330",
    "35000 click 399 330",                  # "CD Not Required": Ok
    "46000 type Test",
    "48000 click 560 310",
    "49000 type Smoke",
    "52000 click 705 572",                  # Enter Name: Next
    "56000 click 705 572",                  # Select Nationality: Next
    "60000 click 155 163",                  # Select Team: Arsenal
    "63000 click 722 14",                   # Take Control
    "67000 click 705 572",                  # Please Confirm: Yes
    "72000 click 705 572",                  # Manager Status: Finish
    "95000 dump {d}/4-news.bmp",
    "95500 click 45 165",                   # manager menu (the manager's name)
    "97500 click 150 427",                  # Go on Holiday
    "100000 click 705 572",                 # Please Confirm: Yes
    "130000 dump {d}/5-holiday.bmp",
    "131000 quit",
]
DATE_BOX = (0, 15, 90, 55)  # x0, y0, x1, y1 of the day and date in the left bar
SCREENS = [("1-settings", 0.20), ("2-setup", 0.0), ("3-leagues", 0.0), ("4-news", 0.0), ("5-holiday", None)]


SIZES = {"classic": (800, 600), "widescreen": (1280, 800)}


def scaled_steps(mode, shots):
    """The steps with their x y positions scaled from 800x600 to the mode's screen size."""
    w, h = SIZES[mode]
    out = []
    for step in STEPS:
        parts = step.format(d=shots).split()
        if parts[1] in ("click", "move", "down", "up", "rclick"):
            parts[2] = str(int(parts[2]) * w // 800)
            parts[3] = str(int(parts[3]) * h // 600)
        out.append(" ".join(parts))
    return out


def scaled_box(mode):
    w, h = SIZES[mode]
    x0, y0, x1, y1 = DATE_BOX
    return (x0 * w // 800, y0 * h // 600, x1 * w // 800, y1 * h // 600)


def read_bmp(path):
    """Pixels of an uncompressed 24- or 32-bit BMP as (width, height, bytes per pixel, rows top down)."""
    data = open(path, "rb").read()
    offset = struct.unpack_from("<I", data, 10)[0]
    w, h = struct.unpack_from("<ii", data, 18)
    bpp = struct.unpack_from("<H", data, 28)[0] // 8
    stride = (w * bpp + 3) & ~3
    rows = [data[offset + y * stride: offset + y * stride + w * bpp] for y in range(abs(h))]
    if h > 0:
        rows.reverse()
    return w, abs(h), bpp, rows


def differing_share(a, b, box=None):
    wa, ha, pa, ra = read_bmp(a)
    wb, hb, pb, rb = read_bmp(b)
    if (wa, ha, pa) != (wb, hb, pb):
        return 1.0
    x0, y0, x1, y1 = box or (0, 0, wa, ha)
    diff = 0
    for y in range(y0, y1):
        x, z = ra[y][x0 * pa:x1 * pa], rb[y][x0 * pa:x1 * pa]
        if x != z:
            diff += sum(1 for i in range(0, len(x), pa) if x[i:i + pa] != z[i:i + pa])
    return diff / ((x1 - x0) * (y1 - y0))


def main(argv):
    if len(argv) < 4:
        print(__doc__)
        return 2
    program, game, ref = os.path.abspath(argv[1]), os.path.abspath(argv[2]), os.path.abspath(argv[3])
    record = "--record" in argv[4:]
    mode = "classic"
    if "--mode" in argv[4:]:
        mode = argv[argv.index("--mode") + 1]
    if mode not in SIZES:
        print(f"unknown mode {mode}: use classic or widescreen")
        return 2
    ref = os.path.join(ref, mode)
    with tempfile.TemporaryDirectory(prefix="cm0102-smoke-") as work:
        shots = os.path.join(work, "shots")
        home = os.path.join(work, "home")
        os.mkdir(shots)
        os.mkdir(home)
        user = os.path.join(work, "user")  # RECOMP_USER_DIR: the same on Mac and Windows
        os.makedirs(user)
        with open(os.path.join(user, "settings.txt"), "w") as f:
            f.write(f"display = {mode}\n")
        env = dict(os.environ, HOME=home, SDL_VIDEODRIVER="dummy", RECOMP_NO_BOX="1",
                   RECOMP_SCRIPT="; ".join(scaled_steps(mode, shots)))
        env["RECOMP_USER_DIR"] = user
        r = subprocess.run([program, "--game", game], env=env, capture_output=True, text=True, timeout=300)
        if r.returncode != 0:
            print(r.stderr[-2000:])
            print(f"FAIL: the game ended with code {r.returncode}")
            return 1
        os.makedirs(ref, exist_ok=True)
        failed = 0
        for name, allowed in SCREENS:
            got = os.path.join(shots, name + ".bmp")
            want = os.path.join(ref, name + ".bmp")
            if not os.path.exists(got):
                print(f"FAIL  {name}: no screen dump")
                failed += 1
                continue
            if allowed is None:
                # The date must move on from the first news screen.
                moved = differing_share(got, os.path.join(shots, "4-news.bmp"), scaled_box(mode))
                ok = moved > 0.02
                print(f"{'ok   ' if ok else 'FAIL '} {name}: {moved:.0%} of the date box changed since 4-news")
            elif record:
                ok = True
                print(f"saved {want}")
            elif not os.path.exists(want):
                ok = False
                print(f"FAIL  {name}: no reference picture {want} (run with --record)")
            else:
                share = differing_share(got, want)
                ok = share <= allowed
                print(f"{'ok   ' if ok else 'FAIL '} {name}: {share:.1%} of pixels differ (allowed {allowed:.0%})")
            if record or not ok:
                shutil.copyfile(got, want if record else os.path.join(ref, name + ".fail.bmp"))
            failed += not ok
    print("PASS" if not failed else "FAIL")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
