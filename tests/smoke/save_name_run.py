#!/usr/bin/env python3
"""Save names test (#34): Save Game suggests "Club YYYY-MM-DD", and an edited name saves as typed.

Usage:
    python3 tests/smoke/save_name_run.py <cm0102_recomp> <game folder> [--shots <folder>]

The runs start from the saved test game prematch.sav (tests/fixtures/fixtures.py): Arsenal, Monday
16.7.01. Checks:
    - run 1 chooses Game Options, Save Game and Ok: the user folder holds "Arsenal 2001-07-16.sav"
    - run 2 deletes the date with Backspace (the entry adds typed text at the end) and types "Final": the user folder holds "Arsenal Final.sav"
    - run 3 has only the save of run 1 in a new user folder, chooses Load Game, the first save of the list
      and Ok, and reaches the news screen of the loaded game
Runs 1 and 2 play at the same time. With --shots the dumps of the Save Game screen and of the load list
stay in that folder (.bmp, with a .png copy).
"""
import os
import shutil
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "fixtures"))
import fixtures  # noqa: E402

SUGGESTED = "Arsenal 2001-07-16.sav"
EDITED = "Arsenal Final.sav"
# Times count from the end of the last wait step.
SAVE_GAME = ["1000 click 45 342",       # Game Options
             "3000 click 140 334"]      # Save Game


def play(program, game, user, steps):
    os.makedirs(user, exist_ok=True)
    fixtures.write_settings(user, "classic", [])
    steps = fixtures.smoke_run.scaled_steps("classic", "", steps)
    r, _ = fixtures.run(program, game, "classic", user, steps, {}, 300)
    return r


def sav_files(folder):
    return sorted(n for n in os.listdir(folder) if n.lower().endswith(".sav"))


def main(argv):
    if len(argv) not in (3, 5):
        print(__doc__)
        return 2
    program, game = os.path.abspath(argv[1]), os.path.abspath(argv[2])
    fixture = os.path.join(fixtures.DEFAULT_DIR, "prematch.sav")
    if not os.path.exists(fixture):
        print(f"no saved game {fixture}: see tests/fixtures/fixtures.py")
        return 2
    failed = []

    def check(ok, text):
        print(("ok   " if ok else "FAIL ") + text)
        if not ok:
            failed.append(text)

    with tempfile.TemporaryDirectory(prefix="cm0102-savename-") as work:
        shots = os.path.abspath(argv[4]) if len(argv) == 5 else work
        os.makedirs(shots, exist_ok=True)
        users = [os.path.join(work, n) for n in ("user1", "user2", "user3")]
        for user in users[:2]:
            os.makedirs(user)
            shutil.copyfile(fixture, os.path.join(user, "prematch.sav"))
        suggested = fixtures.LOAD_STEPS + SAVE_GAME + [f"4500 dump {shots}/save_suggested.bmp",
                                                       "5000 click 705 572", "9000 quit"]     # Ok
        edited = (fixtures.LOAD_STEPS + SAVE_GAME + [f"{4500 + 200 * i} key 8" for i in range(10)] +
                  ["7000 type Final", f"8500 dump {shots}/save_edited.bmp", "9000 click 705 572", "13000 quit"])
        with ThreadPoolExecutor(2) as pool:
            r1, r2 = pool.map(lambda a: play(program, game, *a), [(users[0], suggested), (users[1], edited)])
        check(r1.returncode == 0, f"run 1 (suggested name) ended with code {r1.returncode}")
        check(SUGGESTED in sav_files(users[0]), f"run 1 saved {sav_files(users[0])}")
        check(r2.returncode == 0, f"run 2 (edited name) ended with code {r2.returncode}")
        check(EDITED in sav_files(users[1]), f"run 2 saved {sav_files(users[1])}")

        if os.path.exists(os.path.join(users[0], SUGGESTED)):
            os.makedirs(users[2])
            shutil.copyfile(os.path.join(users[0], SUGGESTED), os.path.join(users[2], SUGGESTED))
            load = list(fixtures.LOAD_STEPS)
            load.insert(load.index("4000 click 445 257"), f"2000 dump {shots}/load_list.bmp")
            loaded = os.path.join(work, "loaded.bmp")
            r3 = play(program, game, users[2], load + [f"2000 dump {loaded}", "3000 quit"])
            check(r3.returncode == 0 and os.path.exists(loaded),
                  f"run 3 loaded {SUGGESTED} from the load list (code {r3.returncode})")
        for f in sorted(os.listdir(shots)):
            if f.endswith(".bmp") and shutil.which("sips"):
                subprocess.run(["sips", "-s", "format", "png", os.path.join(shots, f), "--out",
                                os.path.join(shots, f[:-4] + ".png")], capture_output=True)
    print("PASS" if not failed else "FAIL")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
