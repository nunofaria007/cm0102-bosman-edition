#!/usr/bin/env python3
"""Remember the window (option remember_window): resize, quit, start again, check the size.

Usage: python3 tests/window/test_window.py build/cm0102_recomp /Users/max/Projects/cm0102/clean-3968
Each run uses a new temporary save folder."""
import os
import re
import subprocess
import sys
import tempfile


def run(program, game, user, steps):
    env = dict(os.environ, RECOMP_USER_DIR=user, RECOMP_SCRIPT="; ".join(steps))
    r = subprocess.run([program, "--game", game], env=env, capture_output=True, text=True, timeout=120)
    return r.stderr


def opened(err):
    m = re.search(r"window opened (-?\d+) (-?\d+) (\d+) (\d+) (\d)", err)
    return tuple(int(v) for v in m.groups()) if m else None


def main(argv):
    program, game = os.path.abspath(argv[1]), os.path.abspath(argv[2])
    failures = []

    def check(name, ok):
        print(("ok   " if ok else "FAIL ") + name)
        if not ok:
            failures.append(name)

    with tempfile.TemporaryDirectory() as user:
        run(program, game, user, ["3000 resize 1000 700", "4000 quit"])
        saved = open(os.path.join(user, "window.txt")).read().split()
        check("quit stores the resized size", saved[2:4] == ["1000", "700"])
        o = opened(run(program, game, user, ["3000 quit"]))
        check("next start opens at the stored size", o is not None and o[2:4] == (1000, 700))

        with open(os.path.join(user, "window.txt"), "w") as f:
            f.write("99999 99999 1000 700 0\n")
        o = opened(run(program, game, user, ["3000 quit"]))
        check("a position off every screen opens at 1280x800", o is not None and o[2:4] == (1280, 800))

        with open(os.path.join(user, "window.txt"), "w") as f:
            f.write("100 100 1000 700 0\n")
        sp = os.path.join(user, "settings.txt")
        text = open(sp).read()
        check("settings file has remember_window = on", "\nremember_window = on\n" in text)
        open(sp, "w").write(text.replace("\nremember_window = on\n", "\nremember_window = off\n"))
        o = opened(run(program, game, user, ["3000 quit"]))
        check("remember_window = off ignores the file", o is not None and o[2:4] == (1280, 800))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
