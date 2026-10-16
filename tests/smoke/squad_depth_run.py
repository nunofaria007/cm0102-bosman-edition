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
    recomp: squad depth row club <club> tactic <i> pos <position> slots <s>,... xi <best XI staff of each slot>,...
        players <staff>,<picker value>,<fit>,<cannot play 0/1>,<number on the screen> ...
a line for the dark panel behind the table, in 800x600 numbers and RGB565:
    recomp: squad depth panel <x1> <y1> <x2> <y2> colour <c> table <x1> <y1> <x2> <y2>
and one line when the tab is drawn: recomp: squad depth drawn club <club> tactic <i> rows <n>.
Best XI writes its line (best_xi_run.py) for each draw of the eleven.

Checks:
    - three draws of the tab (own club on the current tactic and on another, the other club)
    - one row for each position (Q161): every slot in exactly one row, no position in two rows
    - a row of k slots holds k + 2 players (fewer only when the squad has fewer), each player once
    - the first k available players of a row are best XI's players in its slots, from the Best XI tab of the
      same club and tactic; an unavailable player above one of them has a higher value than each of them
    - the players after them are in the order of their values
    - each number on the screen is the picker's score rounded, 1 to 20 (Q156), each fit 1 to 20
    - a player appears in more than one row somewhere
    - a dark panel (each colour channel below a quarter) lies behind the whole table (Q162)
    - no hidden attribute of another club's player feeds a value (hidden_check.py): each value is one that the
      visible key attributes of the player allow; a player with no score shows "-" (0)
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
import hidden_check  # noqa: E402

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

ROW = re.compile(r"recomp: squad depth row club (0x[0-9a-f]+) tactic (-?\d+) pos (\S+) slots ([0-9,]+) "
                 r"xi ([-0-9,]+) players([-0-9, ]*)")
PANEL = re.compile(r"recomp: squad depth panel (-?\d+) (-?\d+) (-?\d+) (-?\d+) colour (0x[0-9a-f]+) "
                   r"table (-?\d+) (-?\d+) (-?\d+) (-?\d+)")
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
    """The draws of the tab in log order, without repeats: {club, tactic, count, rows: [(pos, slots, xi, players)]}."""
    out, rows = [], []
    for line in log.splitlines():
        m = ROW.search(line)
        if m:
            players = [tuple(int(v) for v in p.split(",")) for p in m.group(6).split()]
            rows.append((m.group(3), [int(v) for v in m.group(4).split(",")], [int(v) for v in m.group(5).split(",")],
                         players))
            continue
        m = DRAWN.search(line)
        if m:
            t = {"club": m.group(1), "tactic": int(m.group(2)), "count": int(m.group(3)), "rows": rows}
            if not out or out[-1] != t:
                out.append(t)
            rows = []
    return out


def best_xi_after(log, club, tactic):
    """The first Best XI draw of club and tactic in the log."""
    for d in best_xi_run.draws(log):
        if d["club"] == club and d["tactic"] == tactic:
            return d
    return None


def check_row(tag, row, xi):
    pos, slots, picks, players = row
    problems = []
    k = len(slots)
    staffs = [p[0] for p in players]
    if not 1 <= len(players) <= k + 2 or len(staffs) != len(set(staffs)):
        return [f"{tag} {pos}: {len(players)} players for {k} slots, or a player twice: {staffs}"]
    if xi and picks != [xi["staff"][s] for s in slots]:
        problems.append(f"{tag} {pos}: the row's best XI players {picks} are not Best XI's")
    for staff, value, fit, out, shown in players:
        if shown != best_xi_run.shown_score(value):
            problems.append(f"{tag} {pos}: number {shown} is not the score of value {value}")
        if not 1 <= fit <= 20:
            problems.append(f"{tag} {pos}: fit {fit}")
    want = [s for s in picks if s >= 0]
    avail = [p for p in players if not p[3]]
    if sorted(p[0] for p in avail[:len(want)]) != sorted(want):
        problems.append(f"{tag} {pos}: the first available players {[p[0] for p in avail[:len(want)]]} "
                        f"are not Best XI's {want}")
        return problems
    last = players.index(avail[len(want) - 1]) if want else -1
    top = max((p[1] for p in avail[:len(want)]), default=-1)
    for p in players[:last]:
        if p[3] and p[1] <= top:
            problems.append(f"{tag} {pos}: unavailable {p[0]} above a pick with a value not higher than {top}")
    rest = [p[1] for p in players[last + 1:]]
    if rest != sorted(rest, reverse=True):
        problems.append(f"{tag} {pos}: the players after the picks are not in value order {rest}")
    return problems


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
    problems += hidden_check.check_flags(name, log)
    twice, full = False, 0
    for x in t:
        tag = f"{name} club {x['club']} tactic {x['tactic']}"
        slots = sorted(s for r in x["rows"] for s in r[1])
        positions = [r[0] for r in x["rows"]]
        if slots != list(range(11)) or len(positions) != len(set(positions)) or x["count"] != len(x["rows"]):
            problems.append(f"{tag}: rows {positions} hold the slots {slots}, expected each slot once, "
                            f"each position once")
            continue
        xi = best_xi_after(log, x["club"], x["tactic"])
        if not xi:
            problems.append(f"{tag}: no Best XI draw of the same club and tactic")
        seen = {}
        lines = hidden_check.attribute_lines(log)
        for r in x["rows"]:
            problems += check_row(tag, r, xi)
            for staff, value, fit, _out, _shown in r[3] if xi else []:
                values = hidden_check.latest(lines, staff, len(log))
                masks = [int(xi["masks"][s], 16) for s in r[1]]
                if values and not any(value in hidden_check.expected_scores(m, fit, values) for m in masks):
                    problems.append(f"{tag} {r[0]}: staff {staff} has value {value}, the visible attributes allow "
                                    f"{sorted(set().union(*(hidden_check.expected_scores(m, fit, values) for m in masks)))}")
            full += len(r[3]) == len(r[1]) + 2
            for p in r[3]:
                seen.setdefault(p[0], set()).add(r[0])
        if not any(len(r[1]) > 1 for r in x["rows"]):
            problems.append(f"{tag}: no position with more than one slot")
        twice |= any(len(v) > 1 for v in seen.values())
    if not twice:
        problems.append(f"{name}: no player in more than one row")
    if not full:
        problems.append(f"{name}: no row with k + 2 players")
    panels = PANEL.findall(log)
    if not panels:
        problems.append(f"{name}: no panel line")
    for px1, py1, px2, py2, colour, tx1, ty1, tx2, ty2 in panels:
        c = int(colour, 16)
        if (c >> 11) >= 8 or ((c >> 5) & 0x3f) >= 16 or (c & 0x1f) >= 8:
            problems.append(f"{name}: the panel colour {colour} is not dark")
        if int(px1) > int(tx1) or int(py1) > int(ty1) or int(px2) < int(tx2) or int(py2) < int(ty2):
            problems.append(f"{name}: the panel {px1} {py1} {px2} {py2} does not cover the table")
    out = sum(p[3] for x in t for r in x["rows"] for p in r[3])
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
