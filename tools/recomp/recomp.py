"""Static recompiler for cm0102.exe: x86 to C.

Usage:
    python tools/recomp/recomp.py --exe <cm0102.exe> --out <build folder> [--entries 0x528540,...]
        [--functions <file of entry addresses>] [--all] [--no-follow] [--compile <driver.c>]

--entries     the functions to start from. Their direct callees and tail jumps follow unless --no-follow.
--functions   the known function entries: one hex address per line, or the JSON that REA list_procedures
              writes (its normalized_result). A jump to one of them is a tail call. The indirect call
              table holds every translated function.
--all         translate every function of --functions.
--compile     compile the generated C with the runtime (src/recomp_rt) and this driver into <out>/program.

The generated C is a translation of the original. It goes into the build folder and is not committed.
"""
import argparse
import collections
import concurrent.futures
import json
import os
import re
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.setrecursionlimit(50000)
from analysis import Analyzer  # noqa: E402
from pe import Image  # noqa: E402
from translate import FunctionWriter  # noqa: E402

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
RUNTIME = os.path.join(ROOT, "src", "recomp_rt")
GROUP = 64          # functions per C file


def runtime_symbols():
    """The functions and imports that the runtime replaces, from its RT_REPLACE and RT_IMPORT lines."""
    replaced, imports = {}, set()
    for name in os.listdir(RUNTIME):
        if not name.endswith(".c"):
            continue
        text = open(os.path.join(RUNTIME, name)).read()
        for m in re.finditer(r"^RT_REPLACE\(([0-9a-f]{8}),\s*(\w+)\)", text, re.M):
            replaced[int(m.group(1), 16)] = m.group(2)
        for m in re.finditer(r"^RT_IMPORT\((\w+)\)", text, re.M):
            imports.add(m.group(1))
    return replaced, imports


def read_functions(path):
    text = open(path).read()
    if text.lstrip().startswith("{"):
        rows = json.loads(text)["normalized_result"]
        return sorted({int(r["address"], 16) for r in rows if r["address"].startswith("0x")})
    return [int(x, 16) for x in text.split()]


def fname(addr):
    return "f_%08x" % addr


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--entries", default="")
    ap.add_argument("--functions")
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--no-follow", action="store_true")
    ap.add_argument("--compile")
    ap.add_argument("--opt", default="-O1")
    ap.add_argument("--jobs", type=int, default=os.cpu_count())
    args = ap.parse_args(argv)

    t_start = time.time()
    image = Image(args.exe)
    known = read_functions(args.functions) if args.functions else []
    entries = [int(x, 16) for x in args.entries.split(",") if x]
    if args.all:
        entries += known
    replaced, imported = runtime_symbols()
    an = Analyzer(image, set(known) | set(entries))

    funcs = {}
    work = list(dict.fromkeys(entries))
    while work:
        e = work.pop()
        if e in funcs or e in replaced:
            continue
        if not image.is_code(e):
            print(f"skipping {e:#x}: not in .text", file=sys.stderr)
            continue
        f = an.function(e)
        funcs[e] = f
        if not args.no_follow:
            work.extend(t for t in (f.calls | f.tails) if t not in funcs)
    t_analysis = time.time() - t_start

    # Translate
    t0 = time.time()
    gen = os.path.join(args.out, "gen")
    os.makedirs(gen, exist_ok=True)
    stats = collections.Counter()
    unhandled = collections.Counter()
    unhandled_at = []
    features = collections.Counter()
    mnemonics = collections.Counter()
    referenced = set()

    def names(addr):
        referenced.add(addr)
        return fname(addr)

    bodies = {}
    for e in sorted(funcs):
        f = funcs[e]
        w = FunctionWriter(f, image, names)
        bodies[e] = w.write()
        stats["functions"] += 1
        stats["instructions"] += len(f.insns)
        stats["handled"] += w.handled
        stats["decode_stops"] += len(f.bad)
        stats["jump_tables"] += len(f.tables)
        for insn in f.insns.values():
            mnemonics[insn.mnemonic] += 1
        for a, text, why in w.unhandled:
            unhandled[text.split()[0]] += 1
            unhandled_at.append((a, text, why))
        for x in w.features:
            features[x] += 1

    # Functions that are called but neither translated nor replaced (with --no-follow).
    missing = sorted(a for a in referenced if a not in funcs and a not in replaced)
    files = []
    order = sorted(bodies)
    for g in range(0, len(order), GROUP):
        chunk = order[g:g + GROUP]
        path = os.path.join(gen, "fn_%06x.c" % chunk[0])
        with open(path, "w") as out:
            out.write('#include "rt.h"\n#include "functions.h"\n\n')
            for e in chunk:
                out.write(bodies[e])
                out.write("\n")
        files.append(path)

    with open(os.path.join(gen, "functions.h"), "w") as out:
        out.write("/* Generated by tools/recomp. */\n")
        for a in sorted(set(funcs) | set(referenced) | set(replaced)):
            out.write(f"void {fname(a)}(void);\n")
        stubs = sorted(set(n for _, n, _ in image.imports.values()))
        for n in stubs:
            out.write(f"void imp_{n}(void);\n")

    # The table of addresses, the import address table and stubs for missing pieces.
    with open(os.path.join(gen, "table.c"), "w") as out:
        out.write('#include "rt.h"\n#include "rt_host.h"\n#include "functions.h"\n\n')
        for a in missing:
            out.write(f"void {fname(a)}(void) {{ rt_fatal(\"function %#x was not recompiled\", 0x{a:x}u); }}\n")
        for n in stubs:
            if n not in imported:
                out.write(f"void imp_{n}(void) {{ rt_missing_import(\"{n}\"); }}\n")
        rows = {}
        for a in set(funcs) | set(replaced) | set(missing):
            rows[a] = (fname(a), "")
        for iat, (dll, n, stub) in image.imports.items():
            rows[stub] = (f"imp_{n}", n)
        out.write("\nconst RtEntry rt_table[] = {\n")
        for a in sorted(rows):
            fn, n = rows[a]
            out.write(f"    {{0x{a:x}u, {fn}, \"{n or fn}\"}},\n")
        out.write("};\n")
        out.write(f"const uint32_t rt_table_size = {len(rows)};\n\n")
        out.write("const RtIat rt_iat[] = {\n")
        for iat, (dll, n, stub) in sorted(image.imports.items()):
            out.write(f"    {{0x{iat:x}u, 0x{stub:x}u}},\n")
        out.write("};\n")
        out.write(f"const uint32_t rt_iat_size = {len(image.imports)};\n")
    files.append(os.path.join(gen, "table.c"))
    t_translate = time.time() - t0

    report = {
        "functions": stats["functions"],
        "instructions": stats["instructions"],
        "handled": stats["handled"],
        "handled_share": stats["handled"] / max(1, stats["instructions"]),
        "mnemonics": len(mnemonics),
        "unhandled": dict(unhandled.most_common()),
        "unhandled_at": [("%#x" % a, t, why) for a, t, why in unhandled_at[:200]],
        "decode_stops": stats["decode_stops"],
        "jump_tables": stats["jump_tables"],
        "features": dict(features.most_common()),
        "missing_functions": ["%#x" % a for a in missing],
        "c_bytes": sum(os.path.getsize(p) for p in files),
        "seconds_analysis": round(t_analysis, 2),
        "seconds_translate": round(t_translate, 2),
    }
    unhandled_mn = {m for m in mnemonics if any(t.split()[0] == m.split()[0] for t in unhandled)}
    report["mnemonics_handled"] = len(mnemonics) - len(unhandled_mn)
    report["mnemonics_unhandled"] = sorted(unhandled_mn)

    if args.compile:
        report.update(compile_all(args, files))
    json.dump(report, open(os.path.join(args.out, "report.json"), "w"), indent=1)
    short = {k: v for k, v in report.items() if k not in ("unhandled_at", "missing_functions")}
    print(json.dumps(short, indent=1))
    return 0


def compile_all(args, files):
    obj = os.path.join(args.out, "obj")
    os.makedirs(obj, exist_ok=True)
    gen = os.path.join(args.out, "gen")
    flags = [args.opt, "-std=c11", "-fno-strict-aliasing", "-fwrapv", "-w", "-I", RUNTIME, "-I", gen]
    runtime = [os.path.join(RUNTIME, n) for n in sorted(os.listdir(RUNTIME)) if n.endswith(".c")]
    sources = files + runtime + [os.path.abspath(args.compile)]

    def one(src):
        o = os.path.join(obj, os.path.basename(src)[:-2] + ".o")
        t = time.time()
        r = subprocess.run(["clang", "-c", src, "-o", o] + flags, capture_output=True, text=True)
        if r.returncode:
            raise SystemExit(f"clang failed on {src}:\n{r.stderr[:4000]}")
        return o, time.time() - t

    t0 = time.time()
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as ex:
        results = list(ex.map(one, sources))
    t_compile = time.time() - t0
    program = os.path.join(args.out, "program")
    t1 = time.time()
    r = subprocess.run(["clang", "-o", program] + [o for o, _ in results] + ["-lm"], capture_output=True, text=True)
    if r.returncode:
        raise SystemExit("link failed:\n" + r.stderr[:4000])
    return {"seconds_compile_wall": round(t_compile, 2), "seconds_compile_cpu": round(sum(s for _, s in results), 2),
            "seconds_link": round(time.time() - t1, 2), "jobs": args.jobs, "opt": args.opt}


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
