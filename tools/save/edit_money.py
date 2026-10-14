#!/usr/bin/env python3
"""Set the bank balance of one club in a CM 01/02 saved game, and write the result as a new .sav file.

Usage:
    python3 tools/save/edit_money.py <in.sav> <out.sav> --club "Yeovil Town" --balance 1000000 [--show]

The tool never changes the input file, and it stops if the output file exists.

Where the money is (cm0102.exe 3.9.68):
- finance.dat in the save holds one 0x167-byte record per club, in club order (read at 0x59b2a0,
  written at 0x59b3b0, count = all clubs). The bank balance is the signed 64-bit value at offset 0
  (set at 0x5961d0, changed by the income and cost functions 0x5987c0 to 0x5999a0). The record holds
  the club number at offset 8.
- The transfer funds have no field. The finance screen (0x460590) calls 0x598800, which works them
  out from the balance, the wage bill, the club reputation (club+0x80) and a per-club board amount.
  For a club with a reputation under 3500 it caps the funds at reputation x 500 (professional club)
  or reputation x 250. So the tool sets the balance only, and the game derives the funds from it.
- club.dat in the save holds the 0x245-byte club records. The name is at offset 4.
"""
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import savfile  # noqa: E402

CLUB_SIZE = 0x245
FINANCE_SIZE = 0x167


def find_club(club_dat, name):
    hits = []
    for row in range(len(club_dat) // CLUB_SIZE):
        rec = club_dat[row * CLUB_SIZE:(row + 1) * CLUB_SIZE]
        full = rec[4:0x37].split(b"\0")[0].decode("latin-1")
        short = rec[0x38:0x52].split(b"\0")[0].decode("latin-1")
        if name.lower() in (full.lower(), short.lower()):
            hits.append((row, struct.unpack_from("<i", rec, 0)[0], full))
    return hits


def main(argv):
    p = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    p.add_argument("input")
    p.add_argument("output")
    p.add_argument("--club", required=True)
    p.add_argument("--balance", type=int)
    p.add_argument("--show", action="store_true", help="print the record and do not write a file")
    a = p.parse_args(argv)

    if not a.show and os.path.exists(a.output):
        sys.exit(f"stop: {a.output} exists")
    sav = savfile.Sav(a.input)
    clubs = sav.read_member("club.dat")
    hits = find_club(clubs, a.club)
    if len(hits) != 1:
        sys.exit(f"stop: {len(hits)} clubs match {a.club!r}: {hits}")
    row, club_id, full = hits[0]
    fin = bytearray(sav.read_member("finance.dat"))
    o = club_id * FINANCE_SIZE
    if struct.unpack_from("<i", fin, o + 8)[0] != club_id:
        sys.exit("stop: the finance record does not belong to the club")
    balance = struct.unpack_from("<q", fin, o)[0]
    reputation = struct.unpack_from("<h", clubs, row * CLUB_SIZE + 0x80)[0]
    print(f"{full}: club {club_id}, balance {balance}, reputation {reputation}")
    if a.show or a.balance is None:
        return 0
    struct.pack_into("<q", fin, o, a.balance)
    sav.write_member("finance.dat", fin)
    sav.save(a.output)
    check = savfile.Sav(a.output)
    new = struct.unpack_from("<q", check.read_member("finance.dat"), o)[0]
    print(f"wrote {a.output}: balance {new}")
    return 0 if new == a.balance else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
