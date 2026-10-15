#!/usr/bin/env python3
"""Save folder test: a game is saved to the folder of the option save_folder and restored from it.

Usage:
    python3 tests/smoke/save_folder_run.py <cm0102_recomp> <game folder>

The folder is "Saves CM Zoë" (spaces and a non-ASCII letter) in a temporary folder. Run 1 plays a new game
(the steps of smoke_run.py), then chooses Game Options, Save Game and the name Test1. Run 2 starts with a new
empty user folder and the same option, then chooses Restore Saved Game and Test1. Checks:
    - Test1.sav is in the save folder, and no .sav file is in the user folder
    - settings.txt and game.cfg are in the user folder, and not in the save folder
    - after the restore the left bar (date and manager name) is the same as in the saved game
    - a save folder that cannot be created gives a message on stderr, and the save goes to the user folder
All runs use RECOMP_USER_DIR, so the player's own folders stay unchanged.
"""
import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import smoke_run  # noqa: E402

LEFT_BAR = (0, 0, 90, 600)
# Save Game suggests "Arsenal 2001-07-14" (#34, savename.c). Backspace deletes its 18 characters.
CLEAR_NAME = [f"{98000 + 50 * i} key 8" for i in range(18)]
# Steps of smoke_run.py up to the first news screen (95000 ms), without its dumps.
NEW_GAME = [s for s in smoke_run.scaled_steps("classic", "") if int(s.split()[0]) < 95000 and " dump " not in s]


def run(program, game, user, save_folder, steps, home):
    os.makedirs(user, exist_ok=True)
    with open(os.path.join(user, "settings.txt"), "w") as f:
        f.write(f"display = classic\nsave_folder = {save_folder}\n")
    env = dict(os.environ, HOME=home, SDL_VIDEODRIVER="dummy", RECOMP_NO_BOX="1",
               RECOMP_USER_DIR=user, RECOMP_SCRIPT="; ".join(steps))
    return subprocess.run([program, "--game", game], env=env, capture_output=True, text=True, timeout=300)


def sav_files(folder):
    return sorted(n for n in os.listdir(folder) if n.lower().endswith(".sav")) if os.path.isdir(folder) else []


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    program, game = os.path.abspath(argv[1]), os.path.abspath(argv[2])
    failed = 0

    def check(ok, text):
        nonlocal failed
        print(("ok    " if ok else "FAIL  ") + text)
        failed += not ok

    with tempfile.TemporaryDirectory(prefix="cm0102-savefolder-") as work:
        home = os.path.join(work, "home")
        os.mkdir(home)
        saves = os.path.join(work, "Saves CM Zoë")
        user1, user2 = os.path.join(work, "user1"), os.path.join(work, "user2")
        saved, loaded = os.path.join(work, "saved.bmp"), os.path.join(work, "loaded.bmp")
        save_steps = NEW_GAME + [f"95000 dump {saved}",
                                 "95500 click 45 342",          # Game Options
                                 "97500 click 140 334",         # Save Game
                                 *CLEAR_NAME,
                                 "99000 type Test1",
                                 "101000 click 705 572",        # Ok
                                 "108000 quit"]
        r = run(program, game, user1, saves, save_steps, home)
        check(r.returncode == 0, f"run 1 (new game, save) ended with code {r.returncode}")
        check(sav_files(saves) == ["Test1.sav"], f"save folder holds {sav_files(saves)}")
        check(sav_files(user1) == [], f"user folder holds no .sav file: {sav_files(user1)}")
        check(os.path.exists(os.path.join(user1, "settings.txt")) and not os.path.exists(os.path.join(saves, "settings.txt")),
              "settings.txt stays in the user folder")
        check(os.path.exists(os.path.join(user1, "game.cfg")) and not os.path.exists(os.path.join(saves, "game.cfg")),
              "game.cfg stays in the user folder")

        restore_steps = ["8200 click 522 272", "8500 click 662 524",    # settings: Background Changes Off, Ok
                         "11500 click 277 242",                         # Restore Saved Game
                         "14000 click 445 257",                         # the first file in the list
                         "16000 click 705 572",                         # Ok
                         "35000 click 399 330",                         # CD Not Required: Ok
                         f"60000 dump {loaded}", "61000 quit"]
        r = run(program, game, user2, saves, restore_steps, home)
        check(r.returncode == 0, f"run 2 (restore) ended with code {r.returncode}")
        same = os.path.exists(loaded) and smoke_run.differing_share(saved, loaded, LEFT_BAR) < 0.01
        check(same, "the restored game shows the saved game's date and manager")

        bad = os.path.join(work, "file")
        open(bad, "w").close()                                          # a file: the folder cannot be created
        user3 = os.path.join(work, "user3")
        r = run(program, game, user3, os.path.join(bad, "Saves"), NEW_GAME + [
            "95500 click 45 342", "97500 click 140 334", *CLEAR_NAME, "99000 type Test2", "101000 click 705 572", "108000 quit"], home)
        check("cannot create the folder" in r.stderr, "a save folder that cannot be created gives a message on stderr")
        check(sav_files(user3) == ["Test2.sav"], f"the save fell back to the user folder: {sav_files(user3)}")
    print("PASS" if not failed else "FAIL")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
