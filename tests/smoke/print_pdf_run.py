#!/usr/bin/env python3
"""Print to PDF check (#33): scripted runs from tests/fixtures start.sav (Arsenal, 14.7.01).

Usage: python3 tests/smoke/print_pdf_run.py <cm0102_recomp> <game folder> [--dir <fixtures folder>]

Each run prints the squad from the squad screen's Print drop-down with another Print To choice:
  pdf    the default: a PDF in Prints, the path in a message, no .txt file
  file   the original's text file through the Save Printout dialog, no PDF
  both   the PDF and the text file, and the text of the PDF equals the text file
  folder pdf in a save folder with spaces and non-ASCII letters
The text of a PDF is read back from its page streams (the writer is src/recomp_rt/win/pdf.c).
"""
import concurrent.futures
import os
import re
import sys
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "fixtures"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fixtures  # noqa: E402
import smoke_run  # noqa: E402

PRINT_SQUAD = ["click 45 165", "click 148 156", "click 750 25", "click 700 49"]   # manager menu, squad, Print, Squad
PRINT_TABLE = ["click 45 165", "click 148 156", "click 580 527", "click 750 25", "click 700 49"]
                                                # ... the Premier Division button, Print, Table
PRINT_SEQUENCE = PRINT_SQUAD + ["click 399 330", "click 580 527", "click 750 25", "click 700 49"]
                                                # the squad, Ok, then the Premier Division's table: two PDFs
PRINT_FIXTURES_TAB = ["click 45 165", "click 148 156", "click 580 527", "click 530 97", "click 750 25",
                      "click 700 49"]           # the Fixtures tab of the competition screen, Print, Table
PRINT_RESULTS_TAB = PRINT_FIXTURES_TAB[:3] + ["click 358 97"] + PRINT_FIXTURES_TAB[4:]
OPTION_CLICK = {"pdf": None, "file": "522 474", "both": "601 474"}                # Print To in Game Settings


def pdf_lines(path):
    """The text lines of a PDF made by pdf.c; a form feed line between pages."""
    data = open(path, "rb").read().decode("latin-1")
    out = []
    for n, stream in enumerate(re.findall(r"BT\n(.*?)ET\n", data, re.S)):
        if n:
            out.append("\f")
        for m in re.finditer(r"^\((.*)\) Tj T\*$", stream, re.M):
            out.append(re.sub(r"\\([()\\])", r"\1", m.group(1)))
    return out


def steps(choice, name, nav):
    s, t = [], 2000
    def add(action, gap=1000):
        nonlocal t
        t += gap
        s.append(f"{t} {action}")
    if OPTION_CLICK[choice]:
        add("click 45 345"); add("click 148 397"); add(f"click {OPTION_CLICK[choice]}"); add("click 662 524")
    for a in nav:
        add(a)
    if choice == "pdf":
        add("dump {d}/box.bmp", 3000)
        add("click 399 330", 1000)
    else:
        add("type " + name, 4000)
        add("click 705 573", 3000)
        if choice == "both":
            add("dump {d}/box.bmp", 3000)
            add("click 399 330", 1000)
    add("quit", 3000)
    return s


def run(program, game, fix_dir, choice, options=(), nav=PRINT_SQUAD):
    with tempfile.TemporaryDirectory(prefix="cm0102-print-") as work:
        user = os.path.join(work, "user")
        os.makedirs(user)
        out = os.path.join(work, "out")
        os.makedirs(out)
        shutil_copy = __import__("shutil").copyfile
        shutil_copy(os.path.join(fix_dir, "start.sav"), os.path.join(user, "start.sav"))
        fixtures.write_settings(user, "classic", [(k, v.format(w=work)) for k, v in options])
        for k, v in options:                    # the saved games of a save folder live there
            if k == "save_folder":
                os.makedirs(v.format(w=work))
                shutil_copy(os.path.join(fix_dir, "start.sav"), os.path.join(v.format(w=work), "start.sav"))
        extra = [x.format(d=out) for x in steps(choice, "squadtxt", nav)]
        all_steps = smoke_run.scaled_steps("classic", out, fixtures.LOAD_STEPS) + extra
        r, _ = fixtures.run(program, game, "classic", user, all_steps, {}, 600)
        files = []
        for base, _dirs, names in os.walk(work):
            for n in names:
                files.append(os.path.join(base, n))
        found = {"pdf": [f for f in files if f.endswith(".pdf")],
                 "txt": [f for f in files if f.endswith(".txt") and "squadtxt" in f.lower()],
                 "log": r.stderr, "code": r.returncode}
        found["lines"] = [pdf_lines(f) for f in found["pdf"]]
        found["text"] = [open(f, encoding="latin-1").read() for f in found["txt"]]
        found["pdf_rel"] = [os.path.relpath(f, work) for f in found["pdf"]]
        return found


def squeeze(lines):
    return [re.sub(r"\s+$", "", l) for l in lines if l.strip() and l != "\f"]


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    program, game = os.path.abspath(argv[1]), os.path.abspath(argv[2])
    fix_dir = argv[argv.index("--dir") + 1] if "--dir" in argv else fixtures.DEFAULT_DIR
    runs = {"pdf": ("pdf", ()), "file": ("file", ()), "both": ("both", ()),
            "folder": ("pdf", (("save_folder", "{w}/Sav Ünï ✓ folder"),)),
            "table": ("pdf", ()), "table_both": ("both", ()), "sequence": ("pdf", ()),
            "fixtures": ("pdf", ()), "results": ("pdf", ())}
    navs = {"table": PRINT_TABLE, "table_both": PRINT_TABLE, "sequence": PRINT_SEQUENCE,
            "fixtures": PRINT_FIXTURES_TAB, "results": PRINT_RESULTS_TAB}
    with concurrent.futures.ThreadPoolExecutor(4) as ex:
        res = {k: f for k, f in ((k, ex.submit(run, program, game, fix_dir, c, o, navs.get(k, PRINT_SQUAD))) for k, (c, o) in runs.items())}
        res = {k: f.result() for k, f in res.items()}
    bad = []
    def check(cond, text):
        print(("ok   " if cond else "FAIL ") + text)
        if not cond:
            bad.append(text)
    r = res["pdf"]
    check(r["code"] == 0, "pdf: the run ends with code 0")
    check(len(r["pdf"]) == 1 and r["pdf_rel"][0].endswith("user/Prints/Squad - Arsenal 2001-07-14.pdf"),
          f"pdf: one PDF in Prints with the expected name ({r['pdf_rel']})")
    check(not r["txt"], "pdf: no .txt file")
    check("recomp: printout PDF:" in r["log"], "pdf: the path is shown (message log line)")
    check(bool(r["lines"]) and any("Seaman" in l for l in r["lines"][0]) and any("Squad" in l for l in r["lines"][0]),
          "pdf: the PDF holds the squad text")
    r = res["file"]
    check(len(r["txt"]) == 1 and not r["pdf"], "file: only the .txt file")
    r = res["both"]
    check(len(r["txt"]) == 1 and len(r["pdf"]) == 1, "both: the .txt file and the PDF")
    if r["txt"] and r["pdf"]:
        check(squeeze(r["text"][0].split("\n")) == squeeze(r["lines"][0]), "both: the text of the PDF equals the .txt printout")
    r = res["folder"]
    check(len(r["pdf"]) == 1 and "Sav Ünï ✓ folder/Prints/Squad - Arsenal" in r["pdf_rel"][0],
          f"folder: the PDF is in Prints of a save folder with spaces and non-ASCII letters ({r['pdf_rel']})")
    r = res["table"]
    check(len(r["pdf"]) == 1 and r["pdf_rel"][0].endswith("user/Prints/Table - English Premier Division 2001-07-14.pdf"),
          f"table: one PDF in Prints with the expected name ({r['pdf_rel']})")
    check(bool(r["lines"]) and any("Pld" in l for l in r["lines"][0]) and any("Arsenal" in l for l in r["lines"][0]),
          "table: the PDF holds the league table text")
    r = res["table_both"]
    check(len(r["txt"]) == 1 and len(r["pdf"]) == 1, "table both: the .txt file and the PDF")
    if r["txt"] and r["pdf"]:
        check(squeeze(r["text"][0].split("\n")) == squeeze(r["lines"][0]), "table both: the text of the PDF equals the .txt printout")
    # A print takes its name from its own writer, never from an earlier print.
    r = res["sequence"]
    names = sorted(os.path.basename(f) for f in r["pdf_rel"])
    check(names == ["Squad - Arsenal 2001-07-14.pdf", "Table - English Premier Division 2001-07-14.pdf"],
          f"sequence: a squad print and then a table print have their own names ({names})")
    for tab in ("fixtures", "results"):
        r = res[tab]
        check(len(r["pdf"]) == 1 and r["pdf_rel"][0].endswith("Prints/Table - English Premier Division 2001-07-14.pdf"),
              f"{tab}: the Print drop-down of the {tab} tab gives a PDF named by the competition ({r['pdf_rel']})")
    print("FAILED" if bad else "all ok")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
