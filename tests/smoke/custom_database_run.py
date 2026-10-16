#!/usr/bin/env python3
"""Custom data set test (0.4.0 slice 12): a new game loads a folder of Databases in the save folder.

Usage:
    python3 tests/smoke/custom_database_run.py <cm0102_recomp> <game folder> <data-3960 folder> <reference folder>
        [--update-team <folder of an Update Team database>]

Each run uses a new user folder (RECOMP_USER_DIR) with settings.txt: display = classic and
database = folder:<name>. The Databases folder holds copies of data, so the player's folders stay unchanged.
    plain     the folder "Copia 3.9.68" is a copy of the clean 3.9.68 Data (every file, as a player copies a database): the news screen equals the
              reference picture <reference>/classic/4-news.bmp
    own       the folder "Zoë 3.9.60" (a space and a non-ASCII letter) is a copy of the 3.9.60 data set:
              Select League(s) and the news screen equal <reference>/classic-3.9.60, so the game reads the folder
    missing   the folder in the option does not exist: the game reads its own Data (the 3.9.68 pictures), and
              stderr names the folder
    --update-team   a folder that Max made from an Update Team database (copy it to
              test-fixtures/databases/May 2026): the run reaches the first news screen and passes days
Every run also checks the date box moves on after "Go on Holiday".
The reference pictures are those of tests/smoke/smoke_run.py (outside git).
"""
import concurrent.futures
import os
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import smoke_run  # noqa: E402

DATA_FILES = None


def copy_data(src, dest):
    os.makedirs(dest)
    for name in sorted(os.listdir(src)):
        if os.path.isfile(os.path.join(src, name)):
            shutil.copyfile(os.path.join(src, name), os.path.join(dest, name))


def run(program, game, name, folder, source, ref_dir):
    """Plays a new game with database = folder:<folder>. Returns a list of failure texts."""
    out = []
    with tempfile.TemporaryDirectory(prefix="cm0102-customdb-") as work:
        shots, home, user = (os.path.join(work, d) for d in ("shots", "home", "user"))
        for d in (shots, home, user):
            os.mkdir(d)
        if source:
            copy_data(source, os.path.join(user, "Databases", folder))
        with open(os.path.join(user, "settings.txt"), "w") as f:
            f.write(f"display = classic\ndatabase = folder:{folder}\n")
        env = dict(os.environ, HOME=home, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy", RECOMP_NO_BOX="1",
                   RECOMP_USER_DIR=user, RECOMP_SCRIPT="; ".join(smoke_run.scaled_steps("classic", shots)))
        r = subprocess.run([program, "--game", game], env=env, capture_output=True, text=True, timeout=600)
        if r.returncode != 0:
            return [f"{name}: the game ended with code {r.returncode}: {r.stderr[-500:]}"]
        if ref_dir:
            for shot in ("3-leagues", "4-news"):
                share = smoke_run.differing_share(os.path.join(shots, shot + ".bmp"), os.path.join(ref_dir, shot + ".bmp"))
                print(f"{'ok   ' if share == 0 else 'FAIL '} {name} {shot}: {share:.1%} of pixels differ from {ref_dir}")
                if share:
                    out.append(f"{name}: {shot} differs")
        moved = smoke_run.differing_share(os.path.join(shots, "5-holiday.bmp"), os.path.join(shots, "4-news.bmp"),
                                          smoke_run.scaled_box("classic"))
        print(f"{'ok   ' if moved > 0.02 else 'FAIL '} {name} holiday: {moved:.0%} of the date box changed")
        if moved <= 0.02:
            out.append(f"{name}: no days passed on holiday")
        if name == "missing":
            ok = "no usable folder" in r.stderr
            print(f"{'ok   ' if ok else 'FAIL '} missing: stderr names the folder")
            if not ok:
                out.append("missing: no message on stderr")
    return out


def main(argv):
    if len(argv) < 5:
        print(__doc__)
        return 2
    program, game, d3960, ref = (os.path.abspath(a) for a in argv[1:5])
    clean = os.path.join(game, "Data")
    jobs = [("plain", "Copia 3.9.68", clean, os.path.join(ref, "classic")),
            ("own", "Zoë 3.9.60", d3960, os.path.join(ref, "classic-3.9.60")),
            ("missing", "Gone", None, os.path.join(ref, "classic"))]
    if "--update-team" in argv:
        folder = os.path.abspath(argv[argv.index("--update-team") + 1])
        jobs.append(("update-team", os.path.basename(folder), folder, None))
    failures = []
    with concurrent.futures.ThreadPoolExecutor(len(jobs)) as pool:
        for res in pool.map(lambda j: run(program, game, *j[:3], j[3]), jobs):
            failures += res
    print("PASS" if not failures else "FAIL: " + "; ".join(failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
