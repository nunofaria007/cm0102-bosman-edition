#!/usr/bin/env python3
"""Comparison check (spec 2026-10-15-features-0.4.0, slice 6): the polygon on the comparison screen and the
Compare players tab of the Team views screen.

Usage:
    python3 tests/smoke/compare_run.py <cm0102_recomp> <game folder> <out folder> [--dir <fixtures folder>]
        [--mode classic|widescreen|hd]...

One run from start.sav (Arsenal, 14.7.01) for each mode (classic, widescreen and hd when no --mode is given),
at the same time. The tour:
    - own squad > Assistant > Team views > Compare players (the first two of the squad) > the first name
      box > Seaman > the second name box > (classic: More players > the box again) > Shaaban > Back;
    - Seaman's profile > Action > Compare Players > Set as 1st player > Yes > Back > Shaaban's profile >
      Set as 2nd player > Yes > Compare two chosen players (the comparison screen) > Back > the manager's
      menu > Compare two chosen players (side bar item 0x434) > the first name box (the profile).
Screen dumps of each step in <out>/<mode>.

The runtime writes a line for each build of a comparison (RECOMP_VIEWS_LOG=1), and "recomp: comparison
screen built" for each build of the original's screen:
    recomp: compare staff <s1> <s2> keepers <0|1> columns <1|2> rows <n> attr1 <31 values> attr2 <31>
        axes1 <8 tenths> axes2 <8 tenths> marks <n>

Checks:
    - every axis value is the average of its shown attributes, rounded half up to tenths (Q110, Q117), with
      the goalkeeper axes when both players are goalkeepers
    - every attribute value is 0 (hidden) to 20
    - the Compare tab opens on two outfield players of the squad, the drop-downs change both sides, and two
      goalkeepers get the goalkeeper axes
    - the prompts choose the same two goalkeepers for the comparison screen, which opens from the profile
      and from the side bar, and its name box opens the profile
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "fixtures"))
import fixtures  # noqa: E402
import smoke_run  # noqa: E402

# Positions for 800x600; "| x y | x y" gives the widescreen and HD positions where the game does not scale
# (popups).
TOUR = [
    "2500 click 45 165",                    # the manager's menu
    "4000 click 150 156 | 199 203 | 270 270",  # Arsenal Squad
    "wait screen:0x454900",
    "1500 click 592 135",                   # Assistant
    "3000 click 540 241 | 928 288 | 1454 357",  # Team views
    "5000 click 675 97",                    # Compare players: Vieira and Henry, the first two of the squad
    "7000 dump {d}/1-compare-tab.bmp",
    "8000 click 160 137 | 220 180 | 290 238",  # the first name box
    "9500 dump {d}/2-squad-list.bmp",
    "10500 click 160 327 | 220 370 | 290 428",  # David Seaman
    "12500 click 610 137 | 1080 180 | 1820 238",  # the second name box
    # Classic: More players (page 2). Widescreen and HD: Rami Shaaban on page 1.
    "14000 click 610 559 | 1140 641 | 1820 701",
    "16000 click 610 137 | 1080 180 | 1820 238",  # the second name box again
    "17500 dump {d}/3-second-list.bmp",
    # Classic: Rami Shaaban on page 2. Widescreen and HD: a click beside the list closes it.
    "18500 click 720 223 | 712 365 | 1069 521",
    "20500 dump {d}/4-two-keepers.bmp",
    "21500 click 358 573",                  # Back
    "wait screen:0x454900",
    # The original's prompts on the profile's Action drop-down > Compare Players.
    "1500 click 210 234",                   # David Seaman
    "wait screen:0x870700",
    "1500 click 722 14",                    # Action
    "3000 move 700 267 | 1170 275 | 1800 287",
    "3500 move 695 267 | 1166 275 | 1795 287",  # Compare Players
    "4500 move 530 269 | 1001 277 | 1630 289",
    "5000 click 530 269 | 1001 277 | 1630 289",  # Set as 1st player in comparison
    "wait screen:0x8f4210",
    "1500 click 704 572",                   # Yes
    "wait screen:0x870700",
    "1500 click 358 573",                   # Back
    "wait screen:0x454900",
    "1500 click 545 234",                   # Rami Shaaban
    "wait screen:0x870700",
    "1500 click 722 14",                    # Action
    "3000 move 700 267 | 1170 275 | 1800 287",
    "3500 move 695 267 | 1166 275 | 1795 287",  # Compare Players
    "4500 move 530 290 | 1001 298 | 1630 310",
    "5000 click 530 290 | 1001 298 | 1630 310",  # Set as 2nd player in comparison
    "wait screen:0x8f4210",
    "1500 click 704 572",                   # Yes
    "wait screen:0x870700",
    "1500 click 722 14",                    # Action
    "3000 move 700 267 | 1170 275 | 1800 287",
    "3500 move 695 267 | 1166 275 | 1795 287",  # Compare Players
    "4500 move 520 310 | 1001 318 | 1630 330",
    "5000 click 520 310 | 1001 318 | 1630 330",  # Compare two chosen players
    "wait screen:0x876520",
    "2000 dump {d}/5-comparison-screen.bmp",
    "3000 click 358 573",                   # Back
    "wait screen:0x870700",
    "1500 click 45 165",                    # the manager's menu
    "3000 click 150 323 | 199 370 | 270 437",  # Compare two chosen players (side bar item 0x434)
    "wait screen:0x876520",
    "2000 dump {d}/6-from-side-bar.bmp",
    "3000 click 200 88 | 220 117 | 300 158",  # the first name box: a link to the profile
    "wait screen:0x870700",
    "2000 dump {d}/7-name-link.bmp",
]

# The attributes of each axis (src/recomp_rt/win/compare_layout.c), by the getter's numbers 1 to 31.
A = {n: i + 1 for i, n in enumerate(
    "acceleration aggression agility anticipation balance bravery creativity crossing decisions determination "
    "dribbling finishing flair handling heading influence jumping long_shots marking off_the_ball pace passing "
    "positioning reflexes set_pieces stamina strength tackling teamwork technique work_rate".split())}
OUTFIELD = [["tackling", "marking", "positioning"], ["strength", "stamina", "agility", "balance"],
            ["pace", "acceleration"], ["passing", "creativity", "flair"],
            ["finishing", "off_the_ball", "long_shots"], ["technique", "dribbling", "crossing"],
            ["heading", "jumping"], ["decisions", "anticipation", "work_rate", "teamwork", "determination"]]
KEEPER = [["reflexes", "agility"], ["handling"], ["jumping", "bravery"], ["positioning", "influence"],
          ["passing", "technique"], ["pace", "acceleration"], ["strength", "stamina", "balance"],
          ["decisions", "anticipation", "work_rate", "teamwork", "determination"]]

LINE = re.compile(r"recomp: compare staff (-?\d+) (-?\d+) keepers (\d) columns (\d) rows (\d+) attr1 ([\d ]+) "
                  r"attr2 ([\d ]+) axes1 ([-\d ]+) axes2 ([-\d ]+) marks (\d+)")


def run(program, game, folder, mode, out):
    os.makedirs(out, exist_ok=True)
    work = tempfile.mkdtemp(prefix="cm0102-compare-")
    user = os.path.join(work, "user")
    os.makedirs(user)
    shutil.copyfile(os.path.join(folder, "start.sav"), os.path.join(user, "start.sav"))
    fixtures.write_settings(user, mode, [])
    steps = smoke_run.scaled_steps(mode, out, fixtures.LOAD_STEPS) + smoke_run.scaled_steps(mode, out, TOUR)
    steps.append("4000 quit")
    r, took = fixtures.run(program, game, mode, user, steps, {"RECOMP_SCREEN_LOG": "1", "RECOMP_VIEWS_LOG": "1"},
                           1200)
    with open(os.path.join(out, "stderr.txt"), "w") as f:
        f.write(r.stderr)
    for f in sorted(os.listdir(out)):
        if f.endswith(".bmp") and shutil.which("sips"):
            subprocess.run(["sips", "-s", "format", "png", os.path.join(out, f), "--out",
                            os.path.join(out, f[:-4] + ".png")], capture_output=True)
    return r.returncode, r.stderr


def builds(log):
    """The comparisons in log order, without repeats of the same build."""
    out = []
    for m in LINE.finditer(log):
        b = {"staff": (int(m.group(1)), int(m.group(2))), "keepers": int(m.group(3)), "columns": int(m.group(4)),
             "rows": int(m.group(5)), "attr": ([int(v) for v in m.group(6).split()],
                                               [int(v) for v in m.group(7).split()]),
             "axes": ([int(v) for v in m.group(8).split()], [int(v) for v in m.group(9).split()]),
             "marks": int(m.group(10))}
        if not out or out[-1] != b:
            out.append(b)
    return out


def axis_value(names, attr):
    shown = [attr[A[n] - 1] for n in names if attr[A[n] - 1]]
    if not shown:
        return -1
    return (sum(shown) * 20 + len(shown)) // (2 * len(shown))


def check_values(name, b):
    problems = []
    axes = KEEPER if b["keepers"] else OUTFIELD
    for side in (0, 1):
        attr = b["attr"][side]
        if len(attr) != 31 or any(not 0 <= v <= 20 for v in attr):
            problems.append(f"{name}: player {side + 1} has values outside 0 to 20: {attr}")
            continue
        want = [axis_value(names, attr) for names in axes]
        if b["axes"][side] != want:
            problems.append(f"{name}: player {side + 1} axes {b['axes'][side]}, expected {want}")
    return problems


def check(name, code, log):
    problems = []
    if code != 0:
        problems.append(f"{name}: exit code {code}")
    bs = builds(log)
    if not bs:
        return problems + [f"{name}: no comparison in the log"]
    for b in bs:
        problems += check_values(name, b)
    first, keepers = bs[0], bs[-1]
    if first["keepers"] or first["staff"][0] < 0 or first["staff"][1] < 0 or first["staff"][0] == first["staff"][1]:
        problems.append(f"{name}: the Compare tab did not open on two outfield players: {first['staff']}")
    if not keepers["keepers"] or first["staff"][0] in keepers["staff"] or first["staff"][1] in keepers["staff"]:
        problems.append(f"{name}: the drop-downs did not choose two goalkeepers: {keepers['staff']}")
    screens = log.count("recomp: comparison screen built")
    if screens < 2:
        problems.append(f"{name}: the comparison screen was built {screens} times, expected 2 (profile, side bar)")
    tail = log[log.find("recomp: comparison screen built") - 2000:] if screens else ""
    shown = builds(tail)
    if not shown or shown[-1]["staff"] != keepers["staff"] or not shown[-1]["keepers"]:
        problems.append(f"{name}: the comparison screen does not show the two chosen goalkeepers "
                        f"{keepers['staff']}: {[b['staff'] for b in shown]}")
    if "screen 0x870700" not in log[log.rfind("recomp: comparison screen built"):]:
        problems.append(f"{name}: the name box of the comparison screen did not open the profile")
    return problems


def main(argv):
    args = argv[1:]
    folder = fixtures.DEFAULT_DIR
    modes = []
    while "--dir" in args or "--mode" in args:
        flag = "--dir" if "--dir" in args else "--mode"
        i = args.index(flag)
        if flag == "--dir":
            folder = args[i + 1]
        else:
            modes.append(args[i + 1])
        del args[i:i + 2]
    if len(args) != 3:
        print(__doc__)
        return 2
    modes = modes or ["classic", "widescreen", "hd"]
    program, game, out = (os.path.abspath(a) for a in args)
    with ThreadPoolExecutor(len(modes)) as pool:
        futures = {m: pool.submit(run, program, game, folder, m, os.path.join(out, m)) for m in modes}
        results = {m: f.result() for m, f in futures.items()}
    problems = []
    for m in modes:
        problems += check(m, *results[m])
    for p in problems:
        print("FAIL:", p)
    print("compare: " + ("FAIL" if problems else "all checks passed") + f" (dumps in {out})")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
