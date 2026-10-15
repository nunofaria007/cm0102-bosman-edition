#!/usr/bin/env python3
"""Save backups test (option save_backups): saves over one file keep the newest save and three backups.

Usage:
    python3 tests/smoke/save_backups_run.py <cm0102_recomp> <game folder> [--dir <fixtures folder>]

The save folder is "Saves CM Zoë" (spaces and a non-ASCII letter) in a temporary folder, set with the option
save_folder. It starts with prematch.sav (tests/fixtures/fixtures.py) as samesave.sav. Each run loads that
save and saves it again over the same file (RECOMP_SAVE_AFTER_DAYS=0, the game's own save code). Checks:
    - on: after four runs the folder holds samesave.sav and backups/samesave.1.sav to .3.sav, and the
      oldest copy (prematch.sav) is gone
    - off: after two runs the folder holds samesave.sav and no backups folder
The two chains run at the same time. All runs use RECOMP_USER_DIR, so the player's own folders stay unchanged.
"""
import concurrent.futures
import filecmp
import os
import shutil
import sys
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "fixtures"))
import fixtures  # noqa: E402
import smoke_run  # noqa: E402

NAME = smoke_run.SAVE_NAME          # samesave.sav, the name that RECOMP_SAVE_AFTER_DAYS writes


def chain(program, game, work, label, option, runs, prematch):
    saves = os.path.join(work, label, "Saves CM Zoë")
    os.makedirs(saves)
    shutil.copyfile(prematch, os.path.join(saves, NAME))
    for i in range(runs):
        user = os.path.join(work, label, f"user{i}")
        os.makedirs(user)
        fixtures.write_settings(user, "classic", [("save_folder", saves), ("save_backups", option)])
        steps = smoke_run.scaled_steps("classic", user, fixtures.LOAD_STEPS) + ["1800000 quit"]
        r, took = fixtures.run(program, game, "classic", user, steps, {"RECOMP_SAVE_AFTER_DAYS": "0"}, 900)
        if r.returncode != 0 or "recomp: saved" not in r.stderr:
            return saves, f"run {i + 1} failed (code {r.returncode}): {r.stderr[-1500:]}"
    return saves, None


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    program, game = os.path.abspath(argv[1]), os.path.abspath(argv[2])
    folder = argv[argv.index("--dir") + 1] if "--dir" in argv else fixtures.DEFAULT_DIR
    prematch = os.path.join(folder, "prematch.sav")
    failed = 0

    def check(ok, text):
        nonlocal failed
        print(("ok    " if ok else "FAIL  ") + text)
        failed += not ok

    with tempfile.TemporaryDirectory(prefix="cm0102-backups-") as work:
        with concurrent.futures.ThreadPoolExecutor(2) as pool:
            on = pool.submit(chain, program, game, work, "on", "on", 4, prematch)
            off = pool.submit(chain, program, game, work, "off", "off", 2, prematch)
            (saves_on, err_on), (saves_off, err_off) = on.result(), off.result()
        check(err_on is None, f"four saves with the option on: {err_on or 'all ran'}")
        backups = os.path.join(saves_on, "backups")
        listed = sorted(os.listdir(backups)) if os.path.isdir(backups) else []
        want = [f"samesave.{i}.sav" for i in (1, 2, 3)]
        check(sorted(n for n in os.listdir(saves_on) if n.endswith(".sav")) == [NAME],
              f"the save folder holds the newest save {NAME}")
        check(listed == want, f"backups/ holds {listed}")
        check(all(os.path.getsize(os.path.join(backups, n)) > 0 for n in listed), "no backup is empty")
        check(not any(filecmp.cmp(prematch, os.path.join(backups, n), shallow=False) for n in listed),
              "the oldest copy (the first save, prematch.sav) is gone")
        check(err_off is None, f"two saves with the option off: {err_off or 'all ran'}")
        check(os.path.exists(os.path.join(saves_off, NAME)), f"the save folder holds {NAME}")
        check(not os.path.exists(os.path.join(saves_off, "backups")), "off: no backups folder")
    print("PASS" if not failed else "FAIL")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
