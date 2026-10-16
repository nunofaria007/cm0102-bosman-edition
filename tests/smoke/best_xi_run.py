#!/usr/bin/env python3
"""Best XI check (spec 2026-10-15-features-0.4.0, slice 4): the Best XI tab of the Team views screen.

Usage:
    python3 tests/smoke/best_xi_run.py <cm0102_recomp> <game folder> <out folder> [--dir <fixtures folder>]
        [--mode classic|widescreen|hd]...

One run from start.sav (Arsenal, 14.7.01) for each mode (classic, widescreen and hd when no --mode is given),
at the same time. The tour: own squad > Assistant > Team views (Best XI) > Formation > 3-5-2 >
Back > Assistant > Pick best team > Assistant > Team views > Back; Premier Division table > Liverpool >
Squad > Assistant > Team views. Screen dumps of each step in <out>/<mode>.

The runtime writes a line for each draw of the eleven (RECOMP_VIEWS_LOG=1):
    recomp: best xi club <club> tactic <i> "<name>" masks <11 hex> staff <11 staff indices> fit <11>
        selection <11 staff indices of the club's stored selection>
a line for the Formation list: recomp: best xi tactics <n> current <i>,
and two lines for each draw (Q154, Q155), in 800x600 numbers:
    recomp: best xi pitch <x1> <y1> <x2> <y2> boxes <x1> <x2> ... (left and right edge of each name box)
    recomp: best xi bench subs <staff of the pick's sub slots> list <staff> fit <fit> shown <rows>

Checks:
    - the first draw is on the current tactic (tactic 0) and has 11 players, each fit 1 to 20
    - Formation lists two or more tactics; the second draw is another tactic with other masks, and the
      stored selection is the same before and after (view only)
    - after "Pick best team" the stored selection equals the eleven of the first draw (same day)
    - the other club's draw has 11 players of another club
    - no draw holds a player twice
    - every name box lies inside the pitch (Q154)
    - the list on the right starts with the pick's subs, holds no player of the eleven and no player twice,
      shows every player, and each fit is 1 to 20 (Q155)
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
    "5000 dump {d}/1-best-xi.bmp",
    "5500 click 712 25",                    # Formation
    "7000 dump {d}/2-formation-list.bmp",
    "8000 click 720 174 | 1168 189 | 1800 201",  # 3-5-2, the seventh item
    "10000 dump {d}/3-other-formation.bmp",
    "11000 click 358 573",                  # Back
    "wait screen:0x454900",
    "1500 click 592 135",                   # Assistant
    "3000 click 540 159 | 928 205 | 1460 274",  # Pick best team
    "5000 click 592 135",                   # Assistant
    "6500 click 540 241 | 928 288 | 1454 357",  # Team views
    "8500 dump {d}/4-after-pick.bmp",
    "9500 click 358 573",                   # Back
    "wait screen:0x454900",
    "1500 click 584 527",                   # Premier Division
    "wait screen:0x48fcd0",
    "1500 click 215 462",                   # Liverpool
    "3500 click 169 97",                    # Squad
    "5500 click 592 135",                   # Assistant
    "7000 click 540 159 | 928 205 | 1460 274",  # Team views, the only item
    "9000 dump {d}/5-other-club.bmp",
]

LINE = re.compile(r'recomp: best xi club (0x[0-9a-f]+) tactic (\d+) "([^"]*)" masks ([0-9a-f ]+) staff '
                  r'([-0-9 ]+) fit ([-0-9 ]+) selection ([-0-9 ]+)')


def run(program, game, folder, mode, out):
    os.makedirs(out, exist_ok=True)
    work = tempfile.mkdtemp(prefix="cm0102-bestxi-")
    user = os.path.join(work, "user")
    os.makedirs(user)
    shutil.copyfile(os.path.join(folder, "start.sav"), os.path.join(user, "start.sav"))
    fixtures.write_settings(user, mode, [])
    steps = smoke_run.scaled_steps(mode, out, fixtures.LOAD_STEPS) + smoke_run.scaled_steps(mode, out, TOUR)
    steps.append("12000 quit")          # after the last step of the last block
    r, took = fixtures.run(program, game, mode, user, steps, {"RECOMP_SCREEN_LOG": "1", "RECOMP_VIEWS_LOG": "1"},
                           1200)
    with open(os.path.join(out, "stderr.txt"), "w") as f:
        f.write(r.stderr)
    for f in sorted(os.listdir(out)):
        if f.endswith(".bmp") and shutil.which("sips"):
            subprocess.run(["sips", "-s", "format", "png", os.path.join(out, f), "--out",
                            os.path.join(out, f[:-4] + ".png")], capture_output=True)
    return r.returncode, r.stderr


def draws(log):
    """The draws of the eleven in log order, without repeats of the same draw."""
    out = []
    for m in LINE.finditer(log):
        d = {"club": m.group(1), "tactic": int(m.group(2)), "name": m.group(3), "masks": m.group(4).split(),
             "staff": [int(x) for x in m.group(5).split()], "fit": [int(x) for x in m.group(6).split()],
             "selection": [int(x) for x in m.group(7).split()]}
        if not out or out[-1] != d:
            out.append(d)
    return out


PITCH = re.compile(r"recomp: best xi pitch (-?\d+) (-?\d+) (-?\d+) (-?\d+) boxes ([-0-9 ]+)")
BENCH = re.compile(r"recomp: best xi bench subs ([-0-9 ]*) list ([-0-9 ]*) fit ([-0-9 ]*) shown (\d+)")


def check_layout(name, log):
    """Problems of the pitch boxes and the list on the right."""
    problems = []
    pitches, benches = PITCH.findall(log), BENCH.findall(log)
    if not pitches or not benches:
        return [f"{name}: no pitch or list line in the log"]
    for x1, y1, x2, y2, boxes in pitches:
        e = [int(v) for v in boxes.split()]
        for left, right in zip(e[0::2], e[1::2]):
            if left < int(x1) or right > int(x2):
                problems.append(f"{name}: a name box {left}..{right} sticks out of the pitch {x1}..{x2}")
                break
    elevens = [set(d["staff"]) for d in draws(log)]
    for subs, staff, fit, shown in benches:
        subs, staff, fit = ([int(v) for v in t.split()] for t in (subs, staff, fit))
        if staff[:len(subs)] != subs:
            problems.append(f"{name}: the list starts {staff[:len(subs)]}, not with the subs {subs}")
        if len(staff) != len(set(staff)):
            problems.append(f"{name}: a player twice in the list {staff}")
        if not any(not (set(staff) & e) for e in elevens):
            problems.append(f"{name}: the list holds a player of the eleven")
        if int(shown) != len(staff) or not staff:
            problems.append(f"{name}: the list shows {shown} of {len(staff)} players")
        if any(not 1 <= f <= 20 for f in fit):
            problems.append(f"{name}: a fit outside 1 to 20 in the list {fit}")
    return problems


def check(name, code, log):
    problems = check_layout(name, log)
    if code != 0:
        problems.append(f"{name}: exit code {code}")
    d = draws(log)
    for x in d:
        players = [s for s in x["staff"] if s >= 0]
        if len(players) != len(set(players)):
            problems.append(f"{name}: a player twice in {x}")
        if len(x["staff"]) != 11 or len(players) != 11:
            problems.append(f"{name}: not 11 players in {x}")
        if any(not 1 <= f <= 20 for f in x["fit"]):
            problems.append(f"{name}: a fit outside 1 to 20 in {x}")
    if len(d) < 4:
        return problems + [f"{name}: {len(d)} draws of the eleven, expected 4: {d}"]
    first, other, after, club2 = d[0], d[1], d[2], d[-1]
    lists = re.findall(r"recomp: best xi tactics (\d+) current (\d+)", log)
    if not lists or int(lists[0][0]) < 2:
        problems.append(f"{name}: the Formation list is {lists}, expected 2 or more tactics")
    if first["tactic"] != 0:
        problems.append(f"{name}: the first draw is on tactic {first['tactic']}, expected the current one (0)")
    if other["club"] != first["club"] or other["tactic"] == first["tactic"] or other["masks"] == first["masks"]:
        problems.append(f"{name}: the change of formation did not draw another formation: {other}")
    if other["selection"] != first["selection"]:
        problems.append(f"{name}: the change of formation changed the stored selection")
    if after["club"] != first["club"] or after["selection"] != first["staff"]:
        problems.append(f"{name}: after Pick best team the selection is {after['selection']}, "
                        f"best XI was {first['staff']}")
    if after["staff"] != first["staff"]:
        problems.append(f"{name}: best XI changed after Pick best team: {after['staff']} against {first['staff']}")
    if club2["club"] == first["club"]:
        problems.append(f"{name}: the last draw is not of another club")
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
    print("best xi: " + ("FAIL" if problems else "all checks passed") + f" (dumps in {out})")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
