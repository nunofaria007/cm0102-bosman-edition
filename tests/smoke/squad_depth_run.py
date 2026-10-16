#!/usr/bin/env python3
"""Squad depth check (spec 2026-10-15-features-0.4.0, slice 5): the Squad depth tab of the Team views screen.

Usage:
    python3 tests/smoke/squad_depth_run.py <cm0102_recomp> <game folder> <out folder> [--dir <fixtures folder>]
        [--save start.sav] [--mode classic|widescreen|hd]...

One run from the save (default start.sav, Arsenal 14.7.01) for each mode (classic, widescreen and hd when no
--mode is given), at the same time. The tour: own squad > Assistant > Team views (Best XI) > Squad depth >
Formation > 3-5-2 > Best XI > Back; Premier Division table > Liverpool > Squad > Assistant > Team views >
Squad depth > Best XI. Screen dumps of each step in <out>/<mode>.

The runtime writes a line for each row of the tab (RECOMP_VIEWS_LOG=1):
    recomp: squad depth row club <club> tactic <i> slot <s> mask <hex> xi <staff of best XI in the slot>
        players <staff>,<picker value>,<fit>,<cannot play 0/1>,<number on the screen> ...
and one line when the tab is drawn: recomp: squad depth drawn club <club> tactic <i> rows <n>.
Best XI writes its line (best_xi_run.py) for each draw of the eleven.

Checks:
    - three draws of the tab (own club on the current tactic and on another, the other club), each with
      11 rows, one for each slot, and 1 to 3 players in each row
    - each row's first available player is best XI's player in that slot, from the Best XI tab of the same
      club and tactic (no Best XI line in between that differs)
    - the players after the first available one are in the order of their values; an unavailable player
      above best XI's player has a higher value than him
    - each number on the screen is the picker's score rounded, 1 to 20 (Q156), each fit 1 to 20
    - a player appears in more than one row somewhere (a check that rows do not share out the squad)
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
import best_xi_run  # noqa: E402

TOUR = [
    "2500 click 45 165",                    # the manager's menu
    "4000 click 150 156 | 199 203 | 270 270",  # Arsenal Squad
    "wait screen:0x454900",
    "1500 click 592 135",                   # Assistant
    "3000 click 540 241 | 928 288 | 1454 357",  # Team views
    "5000 dump {d}/1-best-xi.bmp",
    "5500 click 445 97",                    # Squad depth
    "7500 dump {d}/2-squad-depth.bmp",
    "8000 click 712 25",                    # Formation
    "9500 click 720 174 | 1168 189 | 1800 201",  # 3-5-2, the seventh item
    "11500 dump {d}/3-other-formation.bmp",
    "12000 click 215 97",                   # Best XI
    "14000 dump {d}/4-best-xi-3-5-2.bmp",
    "15000 click 358 573",                  # Back
    "wait screen:0x454900",
    "1500 click 584 527",                   # Premier Division
    "wait screen:0x48fcd0",
    "1500 click 215 462",                   # Liverpool
    "3500 click 169 97",                    # Squad
    "5500 click 592 135",                   # Assistant
    "7000 click 540 159 | 928 205 | 1460 274",  # Team views, the only item
    "9000 click 445 97",                    # Squad depth
    "11000 dump {d}/5-other-club.bmp",
    "12000 click 215 97",                   # Best XI
    "14000 dump {d}/6-other-club-best-xi.bmp",
]

ROW = re.compile(r"recomp: squad depth row club (0x[0-9a-f]+) tactic (-?\d+) slot (\d+) mask ([0-9a-f]+) "
                 r"xi (-?\d+) players([-0-9, ]*)")
DRAWN = re.compile(r"recomp: squad depth drawn club (0x[0-9a-f]+) tactic (-?\d+) rows (\d+)")


def run(program, game, folder, save, mode, out):
    os.makedirs(out, exist_ok=True)
    work = tempfile.mkdtemp(prefix="cm0102-depth-")
    user = os.path.join(work, "user")
    os.makedirs(user)
    shutil.copyfile(os.path.join(folder, save), os.path.join(user, "start.sav"))
    fixtures.write_settings(user, mode, [])
    steps = smoke_run.scaled_steps(mode, out, fixtures.LOAD_STEPS) + smoke_run.scaled_steps(mode, out, TOUR)
    steps.append("16000 quit")
    r, took = fixtures.run(program, game, mode, user, steps, {"RECOMP_SCREEN_LOG": "1", "RECOMP_VIEWS_LOG": "1"},
                           1200)
    with open(os.path.join(out, "stderr.txt"), "w") as f:
        f.write(r.stderr)
    for f in sorted(os.listdir(out)):
        if f.endswith(".bmp") and shutil.which("sips"):
            subprocess.run(["sips", "-s", "format", "png", os.path.join(out, f), "--out",
                            os.path.join(out, f[:-4] + ".png")], capture_output=True)
    return r.returncode, r.stderr


def tables(log):
    """The draws of the tab in log order, without repeats: {club, tactic, rows: {slot: (mask, xi, players)}}."""
    out, rows = [], {}
    for line in log.splitlines():
        m = ROW.search(line)
        if m:
            players = [tuple(int(v) for v in p.split(",")) for p in m.group(6).split()]
            rows[int(m.group(3))] = (m.group(4), int(m.group(5)), players)
            continue
        m = DRAWN.search(line)
        if m:
            t = {"club": m.group(1), "tactic": int(m.group(2)), "count": int(m.group(3)), "rows": rows}
            if not out or out[-1] != t:
                out.append(t)
            rows = {}
    return out


def best_xi_after(log, club, tactic):
    """The first Best XI draw of club and tactic in the log."""
    for d in best_xi_run.draws(log):
        if d["club"] == club and d["tactic"] == tactic:
            return d
    return None


def check(name, code, log):
    problems = [] if code == 0 else [f"{name}: exit code {code}"]
    t = tables(log)
    if len(t) < 3:
        return problems + [f"{name}: {len(t)} draws of the tab, expected 3"]
    own, other, club2 = t[0], t[1], t[-1]
    if own["tactic"] != 0 or other["club"] != own["club"] or other["tactic"] == own["tactic"]:
        problems.append(f"{name}: the first two draws are not the own club on tactic 0 and another tactic")
    if club2["club"] == own["club"]:
        problems.append(f"{name}: the last draw is not of another club")
    twice = False
    for x in t:
        tag = f"{name} club {x['club']} tactic {x['tactic']}"
        if x["count"] != 11 or sorted(x["rows"]) != list(range(11)):
            problems.append(f"{tag}: rows {sorted(x['rows'])}, expected the 11 slots")
            continue
        xi = best_xi_after(log, x["club"], x["tactic"])
        if not xi:
            problems.append(f"{tag}: no Best XI draw of the same club and tactic")
        seen = {}
        for s, (mask, pick, players) in sorted(x["rows"].items()):
            if not 1 <= len(players) <= 3:
                problems.append(f"{tag} slot {s}: {len(players)} players")
                continue
            if xi and xi["masks"][s] != mask:
                problems.append(f"{tag} slot {s}: mask {mask}, Best XI has {xi['masks'][s]}")
            if xi and pick != xi["staff"][s]:
                problems.append(f"{tag} slot {s}: the row's best XI player {pick} is not Best XI's {xi['staff'][s]}")
            for staff, value, fit, out, shown in players:
                seen.setdefault(staff, set()).add(s)
                if shown != best_xi_run.shown_score(value):
                    problems.append(f"{tag} slot {s}: number {shown} is not the score of value {value}")
                if not 1 <= fit <= 20:
                    problems.append(f"{tag} slot {s}: fit {fit}")
            avail = [p for p in players if not p[3]]
            if xi and xi["staff"][s] >= 0 and (not avail or avail[0][0] != xi["staff"][s]):
                problems.append(f"{tag} slot {s}: the first available player is "
                                f"{avail[0][0] if avail else None}, Best XI has {xi['staff'][s]}")
            first = players.index(avail[0]) if avail else len(players)
            for p in players[:first]:
                if avail and p[1] <= avail[0][1]:
                    problems.append(f"{tag} slot {s}: unavailable {p[0]} above the pick with a value not higher")
            rest = [p[1] for p in players[first + 1:]]
            if rest != sorted(rest, reverse=True):
                problems.append(f"{tag} slot {s}: the players after the pick are not in value order {rest}")
        twice |= any(len(v) > 1 for v in seen.values())
    if not twice:
        problems.append(f"{name}: no player in more than one row")
    out = sum(p[3] for x in t for _, _, ps in x["rows"].values() for p in ps)
    print(f"{name}: {len(t)} draws, {out} entries of players who cannot play")
    return problems


def main(argv):
    args = argv[1:]
    folder, save, modes = fixtures.DEFAULT_DIR, "start.sav", []
    while any(f in args for f in ("--dir", "--mode", "--save")):
        flag = next(f for f in ("--dir", "--mode", "--save") if f in args)
        i = args.index(flag)
        if flag == "--dir":
            folder = args[i + 1]
        elif flag == "--save":
            save = args[i + 1]
        else:
            modes.append(args[i + 1])
        del args[i:i + 2]
    if len(args) != 3:
        print(__doc__)
        return 2
    modes = modes or ["classic", "widescreen", "hd"]
    program, game, out = (os.path.abspath(a) for a in args)
    with ThreadPoolExecutor(len(modes)) as pool:
        futures = {m: pool.submit(run, program, game, folder, save, m, os.path.join(out, m)) for m in modes}
        results = {m: f.result() for m, f in futures.items()}
    problems = []
    for m in modes:
        problems += check(m, *results[m])
    for p in problems:
        print("FAIL:", p)
    print("squad depth: " + ("FAIL" if problems else "all checks passed") + f" (dumps in {out})")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
