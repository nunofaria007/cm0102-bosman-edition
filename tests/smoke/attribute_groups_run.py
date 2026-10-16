#!/usr/bin/env python3
"""Attribute groups check (#43): the player profile with attribute_groups on and off, from start.sav.

Usage:
    python3 tests/smoke/attribute_groups_run.py <cm0102_recomp> <game folder> [--mode classic|widescreen]
        [--shots <folder>]

Two runs in parallel, one with the option on and one off. Each loads start.sav (tests/fixtures), opens the
profile of Seaman (a goalkeeper) and of Vieira from the Arsenal squad and dumps both, then goes on holiday
and saves after 2 game days. The check passes when the two saves are the same and the dumps of the two
runs differ (the option draws). The dumps go to the shots folder as on-gk.png, off-gk.png, on-outfield.png
and off-outfield.png, for the screen check by eye: headings, the goalkeeper layout and the attribute
colours.
"""
import concurrent.futures
import os
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
}
DAYS = 2


def run(program, game, mode, value, work):
    out = os.path.join(work, value)
    steps = os.path.join(work, value + ".steps")
    with open(steps, "w") as f:
        f.write("\n".join(STEPS[mode]) + "\n")
    r = subprocess.run([sys.executable, FIXTURES, "play", program, game, "start.sav", steps, out, "--mode", mode,
                        "--set", f"attribute_groups={value}", "--days", str(DAYS)], capture_output=True, text=True)
    return r, out


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    program, game = os.path.abspath(argv[1]), os.path.abspath(argv[2])
    mode = argv[argv.index("--mode") + 1] if "--mode" in argv else "classic"
    shots = argv[argv.index("--shots") + 1] if "--shots" in argv else None
    if mode not in STEPS:
        print(f"unknown mode {mode}: use classic or widescreen")
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
            print(f"FAIL  the saves differ in: {', '.join(differ) or 'the file header'}")
            failed += 1
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
