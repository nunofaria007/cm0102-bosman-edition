"""Applies deliberate breaks to the port, one at a time, and checks that the differential test fails.

Usage:
    python tools/breaks.py <path to cm0102.exe> <break list> [name of a break ...]

A break list is a Python file in tests/diff/breaks with SUITES (the suites to run) and BREAKS
(name, file, text to find, text to put in its place). The text to find must occur once in the file.
A break that the suites do not catch shows a gap in the tests. Run from the repo root, with a
build in ./build. The script restores each file and builds again at the end.
"""
import glob
import os
import runpy
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PYTHON = os.path.expanduser("~/.venvs/cm0102/bin/python")


def run(command):
    return subprocess.run(command, cwd=REPO, capture_output=True, text=True)


def build():
    # make compares file times to the second, so a quick edit can leave a stale object file.
    for stale in glob.glob(os.path.join(REPO, "build/CMakeFiles/*/src/*/*.o")):
        os.remove(stale)
    result = run(["cmake", "--build", "build"])
    return result.returncode == 0, result.stdout + result.stderr


def main(argv):
    if len(argv) < 3:
        raise SystemExit(__doc__)
    listing = runpy.run_path(argv[2])
    wanted = argv[3:]
    missed = 0
    for name, path, old, new in listing["BREAKS"]:
        if wanted and name not in wanted:
            continue
        full = os.path.join(REPO, path)
        with open(full) as source:
            text = source.read()
        if text.count(old) != 1:
            print(f"STALE    {name}: the text occurs {text.count(old)} times in {path}")
            missed += 1
            continue
        with open(full, "w") as source:
            source.write(text.replace(old, new))
        try:
            built, log = build()
            if not built:
                print(f"NO BUILD {name}: {log[-300:]}")
                missed += 1
                continue
            result = run([PYTHON, "tests/diff/run.py", argv[1], "build/port_driver", *listing["SUITES"]])
            failed = [line for line in (result.stdout + result.stderr).splitlines() if not line.startswith("ok")]
            if failed:
                print(f"caught   {name}: {failed[0][:150]}")
            else:
                print(f"MISSED   {name}")
                missed += 1
        finally:
            with open(full, "w") as source:
                source.write(text)
    build()
    print(f"not caught: {missed}")
    return 1 if missed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
