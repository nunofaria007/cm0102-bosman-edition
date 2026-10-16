#!/usr/bin/env python3
"""No "CD Not Required" box: a load and a new game go on without a click on Ok.

Usage:
    python3 tests/smoke/cd_box_run.py <cm0102_recomp> <game folder> [--dir <fixtures folder>]

The original shows the message box "CD Not Required" ("You may now remove the CM01/02 CD at any time")
at the end of a new game (call 0x82a98b in 0x828110) and of a load (call 0x82e400 in 0x82a9c0). The
build uses no CD, so the runtime skips the box at these two calls (cd_box in waits.c).

The runs play at the same time, with steps that never click Ok:
    load-classic     Restore Saved Game with start.sav (tests/fixtures/fixtures.py), to the first news screen
    load-widescreen  the same in widescreen
    new-game         Start New Game, England, Next, to the Enter Name screen
A run fails when a wait does not end (code 3; the box loop has no input poll, so the game stops there)
or when the log of the runtime does not name the skipped box.
"""
import concurrent.futures
import os
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "fixtures"))
import fixtures  # noqa: E402
import smoke_run  # noqa: E402

SKIPPED = "recomp: skipped the box CD Not Required"

LOAD = [s for s in fixtures.LOAD_STEPS if "box" not in s and "399 330" not in s] + ["2000 quit"]
NEW_GAME = smoke_run.STEPS[:smoke_run.STEPS.index("wait typing") + 1]
NEW_GAME = [s for s in NEW_GAME if "dump" not in s and "box" not in s and "399 330" not in s] + ["2000 quit"]


def play(program, game, folder, name, mode, steps):
    with tempfile.TemporaryDirectory(prefix="cm0102-cd-box-") as work:
        user = os.path.join(work, "user")
        os.makedirs(user)
        if name.startswith("load"):
            shutil.copyfile(os.path.join(folder, "start.sav"), os.path.join(user, "start.sav"))
        fixtures.write_settings(user, mode, [])
        r, took = fixtures.run(program, game, mode, user, smoke_run.scaled_steps(mode, work, steps), {}, 900)
    problems = []
    if r.returncode != 0:
        tail = [l for l in r.stderr.splitlines() if "wait" in l or "box" in l][-3:]
        problems.append(f"code {r.returncode}: {' / '.join(tail)}")
    if SKIPPED not in r.stderr:
        problems.append("the log does not name the skipped box")
    return name, took, problems


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    folder = argv[argv.index("--dir") + 1] if "--dir" in argv else fixtures.DEFAULT_DIR
    if not os.path.exists(os.path.join(folder, "start.sav")):
        print(f"no saved game {os.path.join(folder, 'start.sav')}: see tests/fixtures/fixtures.py")
        return 2
    program, game = os.path.abspath(argv[1]), os.path.abspath(argv[2])
    runs = [("load-classic", "classic", LOAD), ("load-widescreen", "widescreen", LOAD),
            ("new-game", "classic", NEW_GAME)]
    failed = 0
    with concurrent.futures.ThreadPoolExecutor(len(runs)) as pool:
        jobs = [pool.submit(play, program, game, folder, n, m, s) for n, m, s in runs]
        for job in jobs:
            name, took, problems = job.result()
            print(f"{'FAIL ' if problems else 'ok   '} {name} ({took:.0f} s){': ' + '; '.join(problems) if problems else ''}")
            failed += bool(problems)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
