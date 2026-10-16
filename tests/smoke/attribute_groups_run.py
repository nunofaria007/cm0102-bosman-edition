#!/usr/bin/env python3
"""Attribute groups check (#43): the player profile with attribute_groups on and off, from start.sav.

Usage:
    python3 tests/smoke/attribute_groups_run.py <cm0102_recomp> <game folder> [--mode classic|widescreen|hd]
        [--shots <folder>]

Two runs in parallel, one with the option on and one off. Each loads start.sav (tests/fixtures), opens the
profile of Seaman (a goalkeeper) and of Vieira from the Arsenal squad and dumps both, then goes on holiday
and saves after 2 game days. The check passes when the two saves are the same and the dumps of the two
runs differ (the option draws). In classic mode the saves may differ (ACCEPTED_DIFFERENCE). The dumps go to the shots folder as on-gk.png, off-gk.png, on-outfield.png
and off-outfield.png, for the screen check by eye: headings, the goalkeeper layout and the attribute
colours.

The runtime also logs the text and colour of each value label of the profile (RECOMP_COLOUR_LOG=1,
columns.c). With the option on, every value that is a whole number 1 to 20 must have the attribute colour of
that number (1 to 5 red, 6 to 10 amber, 11 to 15 light green, 16 to 20 bright green, as RGB565), and the two
profiles must show at least 40 such values together.
"""
import concurrent.futures
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
FIXTURES = os.path.join(HERE, "..", "fixtures", "fixtures.py")
sys.path.insert(0, HERE)
import smoke_run  # noqa: E402

# Steps after the load, positions for each mode: manager menu, Arsenal Squad, Seaman, Back, Vieira,
# then manager menu, Go on Holiday, Yes.
STEPS = {
    "classic": ["1000 click 45 165", "2500 click 150 156", "6000 click 210 234", "9000 dump {d}/gk.bmp",
                "9500 click 358 573", "12000 click 210 369", "15000 dump {d}/outfield.bmp",
                "17000 click 45 165", "20500 click 150 427", "23500 click 705 572"],
    "widescreen": ["1000 click 72 220", "2500 click 199 203", "6000 click 336 312", "9000 dump {d}/gk.bmp",
                   "9500 click 573 764", "12000 click 336 492", "15000 dump {d}/outfield.bmp",
                   "17000 click 72 220", "20500 click 199 474", "23500 click 1128 763"],
    "hd": ["1000 click 108 297", "2500 click 271 271", "6000 click 460 414", "9000 dump {d}/gk.bmp",
           "9500 click 860 1030", "12000 click 460 664", "15000 dump {d}/outfield.bmp",
           "17000 click 108 297", "20500 click 270 542", "23500 click 1690 1030"],
}
# The attribute colours as the runtime paints them (columns.c COLOUR_*), written again here.
COLOUR = [(5, 0xFA8A), (10, 0xFD84), (15, 0xA70C), (20, 0x37E6)]
VALUE = re.compile(r'recomp: profile value "([^"]*)" colour (0x[0-9a-f]+)')
DAYS = 2
# Modes where the saves with the option on and off may differ. In classic the option gives the profile's
# attribute area 13 rows in place of 12, which changes the memory layout and so later days of the game.
# The spec accepts that (Q152, Q153). In widescreen and HD the 13 rows leave the game the same: the saves
# must be identical.
ACCEPTED_DIFFERENCE = {"classic": "accepted: 13 rows in place of 12, Q152 and Q153"}


def run(program, game, mode, value, work):
    out = os.path.join(work, value)
    steps = os.path.join(work, value + ".steps")
    with open(steps, "w") as f:
        f.write("\n".join(STEPS[mode]) + "\n")
    r = subprocess.run([sys.executable, FIXTURES, "play", program, game, "start.sav", steps, out, "--mode", mode,
                        "--set", f"attribute_groups={value}", "--days", str(DAYS)], capture_output=True, text=True,
                       env=dict(os.environ, RECOMP_COLOUR_LOG="1"))
    return r, out


def check_colours(folder):
    """Problems of the attribute colours of the profiles in the log of a run with the option on. The log
    holds the goalkeeper's profile and then the outfield player's (the steps of STEPS)."""
    log = open(os.path.join(folder, "stderr.txt"), errors="replace").read()
    values = [(m.group(1).strip(), int(m.group(2), 16)) for m in VALUE.finditer(log)]
    numbers = [(int(t), c) for t, c in values if t.isdigit() and 1 <= int(t) <= 20]
    problems = []
    for v, c in numbers:
        want = next(colour for top, colour in COLOUR if v <= top)
        if c != want:
            problems.append(f"the value {v} has the colour {c:#x}, expected {want:#x}")
    if len(numbers) < 40:
        problems.append(f"only {len(numbers)} attribute values in the log of two profiles, expected 40 or more")
    return problems


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    program, game = os.path.abspath(argv[1]), os.path.abspath(argv[2])
    mode = argv[argv.index("--mode") + 1] if "--mode" in argv else "classic"
    shots = argv[argv.index("--shots") + 1] if "--shots" in argv else None
    if mode not in STEPS:
        print(f"unknown mode {mode}: use classic, widescreen or hd")
        return 2
    with tempfile.TemporaryDirectory(prefix="cm0102-groups-") as work:
        with concurrent.futures.ThreadPoolExecutor(2) as pool:
            jobs = {v: pool.submit(run, program, game, mode, v, work) for v in ("on", "off")}
            results = {v: j.result() for v, j in jobs.items()}
        failed = 0
        for v, (r, out) in results.items():
            if r.returncode != 0:
                print(r.stdout[-1500:])
                print(f"FAIL  {v}: the run failed")
                failed += 1
        if failed:
            print("FAIL")
            return 1
        on, off = results["on"][1], results["off"][1]
        read = lambda p: open(p, "rb").read()  # noqa: E731
        if read(os.path.join(on, "samesave.sav")) == read(os.path.join(off, "samesave.sav")):
            print(f"ok    the saves after {DAYS} game days are the same")
        else:
            a = smoke_run.save_parts(os.path.join(on, "samesave.sav"))
            b = smoke_run.save_parts(os.path.join(off, "samesave.sav"))
            differ = sorted(k for k in set(a) | set(b) if a.get(k, (b"",))[0] != b.get(k, (b"",))[0])
            parts = ', '.join(differ) or 'the file header'
            if mode in ACCEPTED_DIFFERENCE:
                print(f"ok    the saves differ in: {parts} ({ACCEPTED_DIFFERENCE[mode]})")
            else:
                print(f"FAIL  the saves differ in: {parts}")
                failed += 1
        problems = check_colours(on)
        for p in problems:
            print(f"FAIL  colours: {p}")
        if problems:
            failed += 1
        else:
            print("ok    the attribute colours match the values on the grouped profiles")
        for name in ("gk", "outfield"):
            if read(os.path.join(on, name + ".bmp")) != read(os.path.join(off, name + ".bmp")):
                print(f"ok    {name}: the profile differs with the option on")
            else:
                print(f"FAIL  {name}: the profile is the same with the option on")
                failed += 1
            if shots:
                os.makedirs(shots, exist_ok=True)
                for v, folder in (("on", on), ("off", off)):
                    png = os.path.join(folder, name + ".png")
                    if os.path.exists(png):
                        shutil.copyfile(png, os.path.join(shots, f"{mode}-{v}-{name}.png"))
    print("PASS" if not failed else "FAIL")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
