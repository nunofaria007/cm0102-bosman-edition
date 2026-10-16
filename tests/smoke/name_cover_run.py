#!/usr/bin/env python3
"""Covered names (Q150): a clickable name under a box that the game paints over it gets no underline.

Usage:
    python3 tests/smoke/name_cover_run.py <cm0102_recomp> <game folder> <out folder> [--dir <fixtures folder>]

Two runs from start.sav (tests/fixtures/fixtures.py), classic, at the same time. Both put the mouse on a
news-text link (Arsenal) and on a squad row (Seaman) and dump the screen each time. The runtime logs
"name line in dump" for each dump that shows an underline (RECOMP_SCREEN_LOG=1).
    plain    no box: both dumps show the line
    covered  RECOMP_TEST_COVER paints a red box over the squad row after each draw, as the goal box of the
             match screen does: the news dump shows the line, the squad dump does not
"""
import os
import re
import shutil
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "fixtures"))
import fixtures  # noqa: E402
import smoke_run  # noqa: E402

STEPS = [
    "2000 move 150 303",                    # Arsenal in the news text
    "3000 dump {d}/news.bmp",
    "3500 click 45 165",                    # the manager's menu
    "5000 click 150 156",                   # Arsenal Squad
    "wait screen:0x454900",
    "1500 move 200 234",                    # Seaman, D
    "2500 dump {d}/squad.bmp",
    "3500 quit",
]
COVER = "150 228 300 245"                   # over the Seaman row, 800x600


def run(program, game, folder, out, cover):
    os.makedirs(out, exist_ok=True)
    work = tempfile.mkdtemp(prefix="cm0102-cover-")
    user = os.path.join(work, "user")
    os.makedirs(user)
    shutil.copyfile(os.path.join(folder, "start.sav"), os.path.join(user, "start.sav"))
    fixtures.write_settings(user, "classic", [])
    steps = smoke_run.scaled_steps("classic", out, fixtures.LOAD_STEPS) + [s.format(d=out) for s in STEPS]
    env = {"RECOMP_SCREEN_LOG": "1"}
    if cover:
        env["RECOMP_TEST_COVER"] = COVER
    r, _ = fixtures.run(program, game, "classic", user, steps, env, 900)
    with open(os.path.join(out, "stderr.txt"), "w") as f:
        f.write(r.stderr)
    return r.returncode, r.stderr


def lines_per_dump(log):
    """{dump file name: whether a line followed it}."""
    result, last = {}, None
    for line in log.splitlines():
        m = re.search(r"script \d+ dump 0 0 (\S+)", line)
        if m:
            last = os.path.basename(m.group(1))
            result[last] = False
        elif "name line in dump" in line and last:
            result[last] = True
    return result


def main(argv):
    args = argv[1:]
    folder = fixtures.DEFAULT_DIR
    if "--dir" in args:
        i = args.index("--dir")
        folder = args[i + 1]
        del args[i:i + 2]
    if len(args) != 3:
        print(__doc__)
        return 2
    program, game, out = (os.path.abspath(a) for a in args)
    with ThreadPoolExecutor(2) as pool:
        plain = pool.submit(run, program, game, folder, os.path.join(out, "plain"), False)
        cover = pool.submit(run, program, game, folder, os.path.join(out, "covered"), True)
        results = {"plain": plain.result(), "covered": cover.result()}
    want = {"plain": {"news.bmp": True, "squad.bmp": True}, "covered": {"news.bmp": True, "squad.bmp": False}}
    problems = []
    for name, (code, log) in results.items():
        got = lines_per_dump(log)
        if code != 0:
            problems.append(f"{name}: exit code {code}")
        for dump, line in want[name].items():
            if got.get(dump) != line:
                problems.append(f"{name}: {dump} {'has no' if line else 'has a'} line")
    for p in problems:
        print("FAIL:", p)
    print("name cover: " + ("FAIL" if problems else "all checks passed") + f" (dumps in {out})")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
