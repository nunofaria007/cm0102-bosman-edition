#!/usr/bin/env python3
"""Message box check (0.4.0 slice 10): the box of 0x5e8290 is centred on the screen in every mode.

Usage: python3 tests/smoke/message_box_run.py <cm0102_recomp> <game folder> [--dir <fixtures folder>]
    [--mode classic|widescreen|hd]...

Each mode (default: all three, at the same time) loads tests/fixtures start.sav. The load shows the
"CD Not Required" box. The run dumps the screen, clicks Ok at the centred place and waits for the
first news screen.

The box keeps its classic size in every mode (its text is not scaled). Classic draws it at y 242..357,
with its blue title bar at y 244..264, so the box is centred on 600 rows. In widescreen and HD mode the
whole box moves down by (screen height - 600) / 2 rows. The checks:
    - the blue title bar is centred across the screen (as before)
    - the title bar starts at y 244 + (screen height - 600) / 2
    - a click on Ok at its centred place closes the box: the game reaches the first news screen
"""
import concurrent.futures
import os
import shutil
import sys
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "fixtures"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fixtures  # noqa: E402
import smoke_run  # noqa: E402

TITLE_BGR = (132, 0, 0)     # the blue of the title bar in a dump (0xaebe04)
TITLE_Y = 244               # the first row of the title bar in classic mode
TITLE_ROWS = 21             # its rows (244..264)
OK = (399, 330)             # the Ok button in classic mode; x is the centre of the screen
MODES = ["classic", "widescreen", "hd"]


def title_bar(path):
    """The rect x1, y1, x2, y2 of the title bar: the columns that hold exactly TITLE_ROWS title blue pixels
    one above the other, from the same row. The title text breaks some columns. Lines of the screen behind
    the box have the same blue, but they are one pixel thick or much longer. None when fewer than 100
    columns match."""
    w, h, bpp, rows = smoke_run.read_bmp(path)
    blue = [[tuple(row[x * bpp:x * bpp + 3]) == TITLE_BGR for x in range(w)] for row in rows]
    starts = {}
    for x in range(w):
        y = 0
        while y < h:
            if not blue[y][x]:
                y += 1
                continue
            top = y
            while y < h and blue[y][x]:
                y += 1
            if y - top == TITLE_ROWS:
                starts.setdefault(top, []).append(x)
    if not starts:
        return None
    top, xs = max(starts.items(), key=lambda kv: len(kv[1]))
    if len(xs) < 100:
        return None
    return (min(xs), top, max(xs), top + TITLE_ROWS - 1)


def play(program, game, fix_dir, mode, work):
    w, h = smoke_run.SIZES[mode]
    dy = (h - 600) // 2
    ok = f"{OK[0] * w // 800} {OK[1] + dy}"
    out = os.path.join(work, mode)
    user = os.path.join(out, "user")
    os.makedirs(user)
    shutil.copyfile(os.path.join(fix_dir, "start.sav"), os.path.join(user, "start.sav"))
    fixtures.write_settings(user, mode, [])
    load = fixtures.LOAD_STEPS[:fixtures.LOAD_STEPS.index("wait box") + 1]
    steps = smoke_run.scaled_steps(mode, out, load) + [
        f"1500 dump {out}/box.bmp", f"2000 move {ok}", f"2500 click {ok}", "wait first-news", "1000 quit"]
    r, _ = fixtures.run(program, game, mode, user, steps, {}, 300)
    return r, os.path.join(out, "box.bmp")


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    program, game = os.path.abspath(argv[1]), os.path.abspath(argv[2])
    fix_dir = argv[argv.index("--dir") + 1] if "--dir" in argv else fixtures.DEFAULT_DIR
    modes = [argv[i + 1] for i, a in enumerate(argv) if a == "--mode"] or MODES
    bad = 0
    with tempfile.TemporaryDirectory(prefix="cm0102-box-") as work:
        with concurrent.futures.ThreadPoolExecutor(len(modes)) as pool:
            jobs = {m: pool.submit(play, program, game, fix_dir, m, work) for m in modes}
            for mode in modes:
                r, dump = jobs[mode].result()
                w, h = smoke_run.SIZES[mode]
                want_y = TITLE_Y + (h - 600) // 2
                bar = title_bar(dump) if os.path.exists(dump) else None
                checks = [
                    (bar is not None, "the title bar is in the dump"),
                    (bar is not None and abs(bar[0] + bar[2] + 1 - w) <= 1,
                     f"the title bar is centred across ({bar and (bar[0], bar[2])}, screen {w})"),
                    (bar is not None and bar[1] == want_y, f"the title bar starts at y {want_y} ({bar and bar[1]})"),
                    (r.returncode == 0, f"Ok at the centred place closes the box (code {r.returncode})"),
                ]
                for good, text in checks:
                    print(f"{'ok   ' if good else 'FAIL '} {mode}: {text}")
                    bad += not good
                if r.returncode != 0:
                    print(r.stderr[-1500:])
    print("PASS" if not bad else "FAIL")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
