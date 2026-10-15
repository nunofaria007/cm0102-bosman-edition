#!/usr/bin/env python3
"""Smoke run: the recompiled game plays the start screen, a new game and some days without a person.

Usage:
    python3 tests/smoke/smoke_run.py <cm0102_recomp> <game folder> <reference folder> [--mode classic|widescreen] [--record] [--tolerance n]
    python3 tests/smoke/smoke_run.py <cm0102_recomp> <game folder> --same-save [--mode classic|widescreen] [--days n]
    python3 tests/smoke/smoke_run.py <cm0102_recomp> <game folder> --match [--days n]

--tolerance n: a colour channel that differs by n or less counts as equal. The Linux build differs from the
Mac reference pictures by 1 in some channels (the compilers round floating point differently), so its
smoke run uses --tolerance 1.

Same-save check (--same-save): the run plays the same new game five times, with -seed 1, the test
clock (RECOMP_FIXED_CLOCK=1) and the same steps to "Go on Holiday". The runtime saves the game n game
days (default 14) after the first news screen (RECOMP_SAVE_AFTER_DAYS) and stops. The runs:
    normal   every option at its default, faster_days = off
    again    the same as normal: the runs are repeatable
    faster   faster_days = on: faster days changes no game decision
    silent   sound = off: sound changes no game decision
    off      every switch option off: no option changes a game decision
Each save must be byte for byte the same as the save of "normal". A failed check names the parts of
the save file that differ. The runs play at the same time, about two minutes in all. A later option
that must not change the game adds a run to SAME_SAVE_RUNS.

Match check (--match): the same as the same-save check, but each run loads the saved test game
prematch.sav (tests/fixtures/fixtures.py) and plays the friendly at Basel on Friday 20.7.01 to the end.
The steps answer the Henry bid (Reject), go to the match, set the commentary speed to Very Fast and
kick off. At half time they restart the match. After the match, Continue Game and one
Space go on to Saturday 21.7.01 AM, where the runtime saves the game, 5 game days (default) after
the load. Later days are not reached: the Bergkamp rumour of 21.7.01 needs an answer. The runs are
in MATCH_RUNS and play at the same time, each in about two minutes. The mode is widescreen only,
because the option match_columns draws only in widescreen.

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

The runs use the SDL dummy audio driver: the sounds take their real time, without noise. A click ends
the intro and its sounds early, so the settings screen shows by 8 s.

The run sets Background Changes to Off on the settings screen, so screens 2 to 4 repeat. The days
pass on holiday, because Continue Game stops at random events (an offer for a player) that need an
answer. The news of the days is random, so screen 5 is only checked for a new date. In 30 s the
game passes about two weeks, with more than five days in every run so far.
"""
import concurrent.futures
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import time

# Positions for classic mode, 800x600. A "wait <state>" step holds the later steps until the game
# reaches the state (src/recomp_rt/win/waits.c). The times count in ms from the window opening, and
# after a wait from the moment the state was reached. A wait that does not end in 120 s ends the run
# with code 3 and names the state the game was in.
STEPS = [
    "1000 click 400 300",                   # the intro: a click ends the logos and their sounds (INTRO_CLICK)
    "8000 dump {d}/1-settings.bmp",
    "8200 click 522 272",                   # settings: Background Changes Off
    "8500 click 662 524",                   # settings: Ok
    "wait setup",
    "2000 dump {d}/2-setup.bmp",
    "2500 click 277 177",                   # Start New Game
    "wait leagues",
    "2000 dump {d}/3-leagues.bmp",
    "2500 click 705 572",                   # Next (England is selected)
    "wait box",
    "1000 move 399 330 | 638 328",
    "2000 click 399 330 | 638 328",         # "CD Not Required": Ok (the box is not scaled in widescreen)
    "wait typing",
    "1000 type Test",
    "3000 click 560 310",
    "4000 type Smoke",
    "7000 click 705 572",                   # Enter Name: Next
    "wait screen:0x821900",
    "1000 click 705 572",                   # Select Nationality: Next
    "wait screen:0x822260",
    "1000 click 155 163",                   # Select Team: Arsenal
    "wait screen:0x454900",
    "1000 click 722 14",                    # Take Control
    "wait screen:0x822aa0",
    "1000 click 705 572",                   # Please Confirm: Yes
    "wait screen:0x81ff10",
    "1000 click 705 572",                   # Manager Status: Finish
    "wait first-news",
    "2000 dump {d}/4-news.bmp",
    "2500 click 45 165",                    # manager menu (the manager's name)
    "4500 click 150 427 | 199 474",         # Go on Holiday (popup items keep their size in widescreen)
    "7000 click 705 572",                   # Please Confirm: Yes
    "37000 dump {d}/5-holiday.bmp",
    "38000 quit",
]
DATE_BOX = (0, 15, 90, 55)  # x0, y0, x1, y1 of the day and date in the left bar
SCREENS = [("1-settings", 0.20), ("2-setup", 0.0), ("3-leagues", 0.0), ("4-news", 0.0), ("5-holiday", None)]


SIZES = {"classic": (800, 600), "widescreen": (1280, 800)}


def scaled_steps(mode, shots, steps=STEPS):
    """The steps with their x y positions scaled from 800x600 to the mode's screen size."""
    w, h = SIZES[mode]
    out = []
    for step in steps:
        step, _, wide = step.partition("|")  # "| x y": the widescreen position, where it does not scale
        parts = step.format(d=shots).split()
        if wide and mode == "widescreen":
            parts[2:4] = wide.split()
        elif parts[0] != "wait" and parts[1] in ("click", "move", "down", "up", "rclick"):
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


def differing_share(a, b, box=None, tol=0):
    wa, ha, pa, ra = read_bmp(a)
    wb, hb, pb, rb = read_bmp(b)
    if (wa, ha, pa) != (wb, hb, pb):
        return 1.0
    x0, y0, x1, y1 = box or (0, 0, wa, ha)
    diff = 0
    for y in range(y0, y1):
        x, z = ra[y][x0 * pa:x1 * pa], rb[y][x0 * pa:x1 * pa]
        if x != z:
            diff += sum(1 for i in range(0, len(x), pa)
                        if any(abs(c - d) > tol for c, d in zip(x[i:i + pa], z[i:i + pa])))
    return diff / ((x1 - x0) * (y1 - y0))


# The runs of the same-save check: a name and the option lines of settings.txt (after the mode).
SAME_SAVE_RUNS = [
    ("normal", {"faster_days": "off"}),
    ("again", {"faster_days": "off"}),
    ("faster", {"faster_days": "on"}),
    ("silent", {"faster_days": "off", "sound": "off"}),
    ("off", {"space_to_continue": "off", "shortcuts": "off", "sharp_picture": "off", "sound": "off",
             "clickable_names": "off", "attribute_colours": "off", "faster_days": "off",
             "match_columns": "off", "condition_colours": "off"}),
]

# The match check (--match): the steps after the load of prematch.sav (fixtures.LOAD_STEPS), positions
# for 800x600 as in STEPS. The times after a wait count from the moment it was reached. Each Space has
# a wait after it, so a busy machine does not lose one. Screens: news 0x7881e0, bid negotiation
# 0x8e9760, reject confirm 0x8efda0, fixtures and results 0x717db0, squad 0x454900, "Proceed to the
# match?" 0x7260f0, match 0x718e90 (RECOMP_SCREEN_LOG=1 prints them). A match event that stops the
# match, for example a red card that opens the tactics screen 0x899ea0, needs more steps: Ok, then
# Resume (label:Resume) at the same place as Kick Off.
MATCH_SAVE = "prematch.sav"
MATCH_STEPS = [
    "1500 press space",                     # news of 16.7.01: the next item
    "3500 press space",                     # the Henry bid: it blocks Continue Game
    "5000 click 172 230",                   # Respond
    "wait screen:0x8e9760",
    "1000 click 502 573",                   # Reject
    "wait screen:0x8efda0",
    "1000 click 705 573",                   # Yes
    "wait news",
    "500 press space",                      # Continue Game: fixtures of 18.7.01
    "wait day:18.7.01",
    "500 press space",                      # results of 18.7.01, the same screen
    "1500 move 400 30",
    "wait continue",
    "500 press space",                      # fixtures of 20.7.01
    "wait day:20.7.01",
    "500 press space",
    "wait screen:0x454900",
    "500 press space",
    "wait screen:0x7260f0",
    "1000 click 705 573",                   # Proceed to the match? Yes
    "wait match",
    "1000 click 45 254",                    # Commentary Speed
    "2500 click 120 242 | 192 323",         # Very Fast (widescreen; the classic position is not checked)
    "4000 click 45 120",                    # Kick Off Match
    "wait label:Re-start 600000",           # half time
    "1000 click 45 120",                    # Re-start Match
    "wait label:Continue 600000",           # full time: Continue Game on the match screen
    "1000 click 45 120",                    # Continue Game: results of 20.7.01
    "wait continue",
    "500 press space",                      # the day goes on to 21.7.01, where the runtime saves
    "600000 quit",
]
# The runs of the match check, as SAME_SAVE_RUNS. Each pair of an option must give the save of normal.
MATCH_RUNS = [
    ("normal", {}),
    ("again", {}),
    ("columns-on", {"match_columns": "on"}),
    ("columns-off", {"match_columns": "off"}),
    ("colours-on", {"attribute_colours": "on"}),
    ("colours-off", {"attribute_colours": "off"}),
]
MATCH_DAYS = 5      # 16.7.01 to 21.7.01

SAVE_NAME = "samesave.sav"
INTRO_CLICK = "1000 click 400 300"
INTRO_WAIT = 20000  # ms


def save_parts(path):
    """The parts of a saved game: {name: (stored bytes, size, check word)}. The file starts with three
    words (version, flags, count) and a table of count rows of 268 bytes: offset, size, a name of 256
    bytes and a check word. The stored bytes of a part run from its offset to the next offset."""
    data = open(path, "rb").read()
    count = struct.unpack_from("<I", data, 8)[0]
    rows = []
    for i in range(count):
        o = 12 + i * 268
        offset, size = struct.unpack_from("<II", data, o)
        name = data[o + 8:o + 264].split(b"\0")[0].decode("latin-1")
        rows.append((offset, name, size, struct.unpack_from("<I", data, o + 264)[0]))
    rows.sort()
    parts = {}
    for i, (offset, name, size, check) in enumerate(rows):
        end = rows[i + 1][0] if i + 1 < len(rows) else len(data)
        parts[name] = (data[offset:end], size, check)
    return parts


def without_intro_click(steps):
    """The steps without the click that ends the intro, and the steps before the first wait 20 s later.

    With sound the intro takes about 19 s, without it 5.5 s, but both runs load and free the same
    sounds. A click would skip the second sound in one run only, and the memory of the two games would
    differ. The steps before the first wait count from the window opening, so only they move."""
    steps = [s for s in steps if s != INTRO_CLICK]
    first_wait = next(i for i, s in enumerate(steps) if s.startswith("wait "))
    return [f"{int(s.split(' ', 1)[0]) + INTRO_WAIT} {s.split(' ', 1)[1]}" for s in steps[:first_wait]] + \
        steps[first_wait:]


def compare_runs(program, game, mode, runs, steps, days, save=None, timeout=600):
    """Plays the runs at the same time, each with its option lines, and compares each saved game with
    the save of the first run. save: a saved game to copy into each user folder first."""
    def play(name, options, work):
        run = os.path.join(work, name)
        user, home, shots = (os.path.join(run, d) for d in ("user", "home", "shots"))
        for d in (user, home, shots):
            os.makedirs(d)
        if save:
            shutil.copyfile(save, os.path.join(user, os.path.basename(save)))
        with open(os.path.join(user, "settings.txt"), "w") as f:
            f.write(f"display = {mode}\n")
            for key, value in options.items():
                f.write(f"{key} = {value}\n")
        env = dict(os.environ, HOME=home, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy", RECOMP_NO_BOX="1",
                   RECOMP_USER_DIR=user,
                   RECOMP_FIXED_CLOCK="1", RECOMP_SAVE_AFTER_DAYS=str(days),
                   RECOMP_SCRIPT="; ".join(scaled_steps(mode, shots, steps)))
        start = time.time()
        r = subprocess.run([program, "--game", game, "-seed", "1"], env=env, capture_output=True, text=True,
                           timeout=timeout)
        return r, os.path.join(user, SAVE_NAME), time.time() - start

    # The runs play at the same time. The test clock and the wait steps make each run the same on a
    # busy machine.
    saves = {}
    with tempfile.TemporaryDirectory(prefix="cm0102-same-save-") as work:
        with concurrent.futures.ThreadPoolExecutor(len(runs)) as pool:
            jobs = [(name, pool.submit(play, name, options, work)) for name, options in runs]
            failed_runs = 0
            for name, job in jobs:
                r, path, took = job.result()
                if r.returncode != 0 or not os.path.exists(path):
                    print(r.stderr[-2000:])
                    print(f"FAIL  {name}: no saved game (code {r.returncode}) after {took:.0f} s")
                    failed_runs += 1
                    continue
                print(f"run   {name}: saved after {days} game days in {took:.0f} s")
                saves[name] = path
            if failed_runs:
                print("FAIL")
                return 1
        failed = 0
        first = runs[0][0]
        want = open(saves[first], "rb").read()
        for name, _ in runs[1:]:
            if open(saves[name], "rb").read() == want:
                print(f"ok    {name}: the save is the same as {first}")
                continue
            failed += 1
            a, b = save_parts(saves[first]), save_parts(saves[name])
            differ = sorted(k for k in set(a) | set(b) if a.get(k) != b.get(k))
            print(f"FAIL  {name}: the save differs from {first} in: {', '.join(differ) or 'the file header'}")
    print("PASS" if not failed else "FAIL")
    return 1 if failed else 0


def same_save(program, game, mode, days):
    # No click ends the intro (without_intro_click).
    steps = [s for s in STEPS if "5-holiday" not in s and "quit" not in s]
    steps = without_intro_click(steps) + ["400000 quit"]
    return compare_runs(program, game, mode, SAME_SAVE_RUNS, steps, days)


def match(program, game, days):
    sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "fixtures"))
    import fixtures  # noqa: E402  (fixtures.py imports this file)
    save = os.path.join(fixtures.DEFAULT_DIR, MATCH_SAVE)
    if not os.path.exists(save):
        print(f"FAIL: no saved test game {save} (tests/fixtures/fixtures.py says where it comes from)")
        return 1
    # No click ends the intro (without_intro_click): with the click, the runs on a busy machine played
    # different matches.
    steps = without_intro_click(fixtures.LOAD_STEPS) + MATCH_STEPS
    return compare_runs(program, game, "widescreen", MATCH_RUNS, steps, days, save, timeout=900)


def main(argv):
    if len(argv) < 4:
        print(__doc__)
        return 2
    mode = "classic"
    if "--mode" in argv[3:]:
        mode = argv[argv.index("--mode") + 1]
    if mode not in SIZES:
        print(f"unknown mode {mode}: use classic or widescreen")
        return 2
    if "--match" in argv[3:]:
        days = int(argv[argv.index("--days") + 1]) if "--days" in argv[3:] else MATCH_DAYS
        return match(os.path.abspath(argv[1]), os.path.abspath(argv[2]), days)
    if "--same-save" in argv[3:]:
        days = int(argv[argv.index("--days") + 1]) if "--days" in argv[3:] else 14
        return same_save(os.path.abspath(argv[1]), os.path.abspath(argv[2]), mode, days)
    program, game, ref = os.path.abspath(argv[1]), os.path.abspath(argv[2]), os.path.abspath(argv[3])
    record = "--record" in argv[4:]
    tol = int(argv[argv.index("--tolerance") + 1]) if "--tolerance" in argv[4:] else 0
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
        env = dict(os.environ, HOME=home, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy", RECOMP_NO_BOX="1",
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
                share = differing_share(got, want, tol=tol)
                ok = share <= allowed
                print(f"{'ok   ' if ok else 'FAIL '} {name}: {share:.1%} of pixels differ (allowed {allowed:.0%})")
            if record or not ok:
                try:
                    shutil.copyfile(got, want if record else os.path.join(ref, name + ".fail.bmp"))
                except OSError:
                    print(f"      cannot write {name}.fail.bmp (read-only reference folder)")
            failed += not ok
    print("PASS" if not failed else "FAIL")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
