#!/usr/bin/env python3
"""New-game check on every Update Team database (0.4.0 slice 12).

Usage:
    python3 tests/smoke/update_team_sweep.py <cm0102_recomp> <game folder> <folder of databases> [--jobs n]
        [--timeout s] [--out table.md]

<folder of databases> holds one folder per database (Max keeps them in test-fixtures/databases). Each run
copies one whole folder into Databases of a new user folder, sets database = folder:<name> and plays the
steps of smoke_run.py up to the first news screen and holiday. A run passes when the date box moves on
after "Go on Holiday". A run that fails or hangs names the last step it reached. At most --jobs (6) games
run at the same time. The table is a Markdown table (folder, result, seconds, failure step).
"""
import concurrent.futures
import os
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import custom_database_run as cdr  # noqa: E402
import smoke_run  # noqa: E402

SHOTS = ["1-settings", "2-setup", "3-leagues", "4-news", "5-holiday"]
AFTER = {"1-settings": "the setup screen", "2-setup": "Select League(s) or the name entry",
         "3-leagues": "the new game build (after Next on Select League(s))", "4-news": "Go on Holiday",
         "5-holiday": "the end"}


def check(program, game, base, name, timeout, extra=None):
    """Returns (passed, seconds, failure text)."""
    start = time.time()
    with tempfile.TemporaryDirectory(prefix="cm0102-sweep-") as work:
        shots, home, user = (os.path.join(work, d) for d in ("shots", "home", "user"))
        for d in (shots, home, user):
            os.mkdir(d)
        folder = os.path.join(user, "Databases", name)
        cdr.copy_data(os.path.join(base, name), folder)
        if extra:
            extra(folder)
        with open(os.path.join(user, "settings.txt"), "w") as f:
            f.write(f"display = classic\ndatabase = folder:{name}\n")
        env = dict(os.environ, HOME=home, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy", RECOMP_NO_BOX="1",
                   RECOMP_USER_DIR=user, RECOMP_SCRIPT="; ".join(smoke_run.scaled_steps("classic", shots)))
        try:
            r = subprocess.run([program, "--game", game], env=env, capture_output=True, text=True, timeout=timeout)
        except subprocess.TimeoutExpired:
            done = [s for s in SHOTS if os.path.exists(os.path.join(shots, s + ".bmp"))]
            last = done[-1] if done else "none"
            return False, time.time() - start, f"hang (no end after {timeout} s) after {last}: {AFTER.get(last, '?')}"
        took = time.time() - start
        done = [s for s in SHOTS if os.path.exists(os.path.join(shots, s + ".bmp"))]
        last = done[-1] if done else "none"
        if r.returncode != 0:
            line = next((ln for ln in r.stderr.splitlines() if "timed out" in ln), r.stderr[-120:].strip())
            return False, took, f"code {r.returncode} after {last}: {line[:110]}"
        if "5-holiday" not in done:
            return False, took, f"no holiday picture, last {last}"
        moved = smoke_run.differing_share(os.path.join(shots, "5-holiday.bmp"), os.path.join(shots, "4-news.bmp"),
                                          smoke_run.scaled_box("classic"))
        if moved <= 0.02:
            return False, took, "no days passed on holiday"
        return True, took, ""


def clean_euro(game):
    """A folder change that puts the clean euro.cfg in the folder (the mismatch case of the guard)."""
    def put(folder):
        src = os.path.join(game, "Data", "euro.cfg")
        with open(src, "rb") as a, open(os.path.join(folder, "euro.cfg"), "wb") as b:
            b.write(a.read())
    return put


def main(argv):
    if len(argv) < 4:
        print(__doc__)
        return 2
    program, game, base = (os.path.abspath(a) for a in argv[1:4])
    jobs = int(argv[argv.index("--jobs") + 1]) if "--jobs" in argv else 6
    timeout = int(argv[argv.index("--timeout") + 1]) if "--timeout" in argv else 420
    out = argv[argv.index("--out") + 1] if "--out" in argv else None
    names = sorted(n for n in os.listdir(base) if os.path.isdir(os.path.join(base, n)) and n != "zips")
    work = [(n, None) for n in names]
    if "May 2026" in names:
        work.append(("May 2026", clean_euro(game)))      # the guard case: euro.cfg that does not match
    rows = []
    with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
        futs = [(n, ex, pool.submit(check, program, game, base, n, timeout, ex)) for n, ex in work]
        for n, ex, fut in futs:
            ok, took, why = fut.result()
            label = n + (" + clean euro.cfg" if ex else "")
            rows.append((label, ok, took, why))
            print(f"{'ok  ' if ok else 'FAIL'} {label}: {took:.0f} s {why}", flush=True)
    table = ["| Folder | Result | Seconds | Failure step |", "|---|---|---|---|"]
    table += [f"| {n} | {'pass' if ok else 'FAIL'} | {took:.0f} | {why} |" for n, ok, took, why in rows]
    if out:
        with open(out, "w") as f:
            f.write("\n".join(table) + "\n")
    print("\n".join(table))
    failed = sum(not ok for _, ok, _, _ in rows)
    print(f"{len(rows) - failed} of {len(rows)} pass")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
