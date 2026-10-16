#!/usr/bin/env python3
"""Same-save checks of the 0.4.0 screens: a feature must not change the game (spec 2026-10-15-features-0.4.0).

Usage:
    python3 tests/smoke/same_save_views.py <cm0102_recomp> <game folder> [--check compare|print]
        [--days n] [--dir <fixtures folder>]

Each check plays the same game from start.sav (tests/fixtures) in runs at the same time, goes on holiday
for n game days (default 2) and compares the saved games byte for byte (smoke_run.compare_runs).

    compare   The original's "Compare two chosen players" screen (Seaman and Shaaban, set with the prompts of
              the profile's Action drop-down) with squad_views on and off, in classic and in widescreen mode.
              The squad_views screen must make the same calls that change the game (the training manager's
              call, the random numbers). Its widgets sit in other memory blocks, so the five parts of the
              save that depend on the memory layout (LAYOUT_PARTS, Q152) may differ; no other part may.
    print     The squad printed from the squad screen's Print drop-down as a PDF (the default) and as a File
              (Print To in Game Settings), with the same clicks otherwise. The PDF message box must not
              change the game (Q152): the texts of the box are in runtime memory, not on the game's heap.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "fixtures"))
sys.path.insert(0, HERE)
import fixtures  # noqa: E402
import smoke_run  # noqa: E402

# The parts of a saved game that depend on where the game's memory blocks sit (Q152): any change of memory
# layout, even opening one of the original's own screens, changes them after a few days. Research of slices 3
# and 7 (docs/research/2026-10-15-attribute-groups.md) found these five and no others.
LAYOUT_PARTS = ("fifa_rankings.dat", "general.dat", "national_teams.dat", "staff history.tmp", "transfer.dat")

# Positions for 800x600; "| x y | x y" gives the widescreen and HD positions where the game does not scale.
HOLIDAY = [
    "2500 click 45 165",                    # the manager's menu
    "4500 click 150 427 | 199 474 | 270 541",  # Go on Holiday
    "7000 click 705 572",                   # Please Confirm: Yes
]

COMPARE = [
    "2500 click 45 165",                    # the manager's menu
    "4000 click 150 156 | 199 203 | 270 270",  # Arsenal Squad
    "wait screen:0x454900",
    "1500 click 210 234",                   # David Seaman
    "wait screen:0x870700",
    "1500 click 722 14",                    # Action
    "3000 move 700 267 | 1170 275 | 1800 287",
    "3500 move 695 267 | 1166 275 | 1795 287",  # Compare Players
    "4500 move 530 269 | 1001 277 | 1630 289",
    "5000 click 530 269 | 1001 277 | 1630 289",  # Set as 1st player in comparison
    "wait screen:0x8f4210",
    "1500 click 704 572",                   # Yes
    "wait screen:0x870700",
    "1500 click 358 573",                   # Back
    "wait screen:0x454900",
    "1500 click 545 234",                   # Rami Shaaban
    "wait screen:0x870700",
    "1500 click 722 14",                    # Action
    "3000 move 700 267 | 1170 275 | 1800 287",
    "3500 move 695 267 | 1166 275 | 1795 287",  # Compare Players
    "4500 move 530 290 | 1001 298 | 1630 310",
    "5000 click 530 290 | 1001 298 | 1630 310",  # Set as 2nd player in comparison
    "wait screen:0x8f4210",
    "1500 click 704 572",                   # Yes
    "wait screen:0x870700",
    "1500 click 722 14",                    # Action
    "3000 move 700 267 | 1170 275 | 1800 287",
    "3500 move 695 267 | 1166 275 | 1795 287",  # Compare Players
    "4500 move 520 310 | 1001 318 | 1630 330",
    "5000 click 520 310 | 1001 318 | 1630 330",  # Compare two chosen players
    "wait screen:0x876520",
    "3000 click 358 573",                   # Back
    "wait screen:0x870700",
]

# Print To is three buttons of Game Settings: PDF at 444 474, File at 522 474, Both at 601 474.
OPTIONS = {"pdf": ["click 45 345", "click 148 397", "click 444 474", "click 662 524"],
           "file": ["click 45 345", "click 148 397", "click 522 474", "click 662 524"]}
PRINT_SQUAD = ["click 45 165", "click 148 156", "click 750 25", "click 700 49"]   # menu, squad, Print, Squad


def print_steps(choice, times=1, boxes=None):
    """The steps of a print run: Print To, then the squad printed times times, then a holiday. The times
    count from the last wait of the load, each after the one before. boxes: how many PDF message boxes show
    (RECOMP_TEST_NO_PDF_BOX=boxes+1); a box that does not show gets no click. None: all."""
    steps, t = [], 2000

    def add(action, gap=1000):
        nonlocal t
        t += gap
        steps.append(f"{t} {action}")

    for a in OPTIONS[choice]:               # the same clicks: only the Print To button differs
        add(a)
    for n in range(times):
        for a in PRINT_SQUAD if n == 0 else PRINT_SQUAD[2:]:     # the squad screen is open for the later prints
            add(a)
        if choice == "pdf":
            if boxes is None or n < boxes:
                add("click 399 330", 3000)  # the message with the path: Ok
            else:
                t += 3000
        else:
            add(f"type squadtxt{n}", 4000)  # the Save Printout dialog: a new name each time, then Save
            add("click 705 573", 3000)
    add("click 45 165", 4000)               # holiday
    add("click 150 427", 2500)
    add("click 705 572", 3000)
    return steps


def run_compare(program, game, days, folder):
    steps = smoke_run.without_intro_click(fixtures.LOAD_STEPS) + COMPARE + HOLIDAY + ["400000 quit"]
    failed = 0
    for mode in ("classic", "widescreen"):
        print(f"--- comparison screen, {mode}")
        runs = [("on", {"squad_views": "on"}), ("off", {"squad_views": "off"}), ("on again", {"squad_views": "on"})]
        failed += smoke_run.compare_runs(program, game, mode, runs, steps, days,
                                         os.path.join(folder, "start.sav"), allowed=LAYOUT_PARTS)
    return failed


def run_print(program, game, days, folder):
    """The message box of a PDF print must leave the game as it found it (Q152): the texts of the box are in
    runtime memory, not on the game's heap. The saved game of a PDF print with the second message box equals
    that of one without it (RECOMP_TEST_NO_PDF_BOX=2), after two and three prints.

    The first box of a run is not compared: the game's box code (0x5e8290) keeps a buffer for the pixels
    under the box (0x5e3dd0) that it makes when it first shows, and since 0.4.0 the "CD Not Required" box,
    which used to make it at the load, no longer shows. So the first box moves the game's memory blocks like
    opening a screen (Q152) and changes the five parts that depend on the layout (fifa_rankings, general,
    national_teams, staff history, transfer). Nor is a PDF run compared with a run that does not print, or
    with a File run (the Save Printout dialog is the original's code and changes much more)."""
    base = smoke_run.without_intro_click(fixtures.LOAD_STEPS)
    save = os.path.join(folder, "start.sav")
    failed = 0
    for times in (2, 3):
        runs = [(f"pdf x{times}", {"_steps": base + print_steps("pdf", times) + ["400000 quit"]}),
                (f"pdf x{times}, no box after the first",
                 {"_steps": base + print_steps("pdf", times, 1) + ["400000 quit"],
                  "_env": {"RECOMP_TEST_NO_PDF_BOX": "2"}}),
                (f"pdf x{times} again", {"_steps": base + print_steps("pdf", times) + ["400000 quit"]})]
        print(f"--- the squad printed {times} times as PDF, with and without the later message boxes, classic")
        failed += smoke_run.compare_runs(program, game, "classic", runs, base, days, save)
    return failed


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    program, game = os.path.abspath(argv[1]), os.path.abspath(argv[2])
    days = int(argv[argv.index("--days") + 1]) if "--days" in argv else 2
    folder = argv[argv.index("--dir") + 1] if "--dir" in argv else fixtures.DEFAULT_DIR
    which = argv[argv.index("--check") + 1] if "--check" in argv else "all"
    failed = 0
    if which in ("all", "compare"):
        failed += run_compare(program, game, days, folder)
    if which in ("all", "print"):
        failed += run_print(program, game, days, folder)
    print("same_save_views: " + ("FAIL" if failed else "all checks passed"))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
