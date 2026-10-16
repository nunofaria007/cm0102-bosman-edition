# Update Team databases and euro.cfg (slice 12)

Date 2026-10-16. Slice 12 of `docs/specs/2026-10-15-features-0.4.0-spec.md`.

## Result

A new game loads each of the 49 database folders in `test-fixtures/databases`, except EmptyDatabase
(it holds no clubs, so no league can start). Each run copies the whole folder into `Databases` of a new
user folder and plays a new game to the first news screen and on holiday. The test is
`tests/smoke/update_team_sweep.py`. It ran 6 games at a time. The table is below.

## The hang on May 2026 (fixed)

- The game hung when a database was used with an euro.cfg that does not match it. My first test copied only the
  `.dat` files, so May 2026 met the clean euro.cfg. The whole May 2026 folder, with its own euro.cfg, never hung.
- Cause (traced from the native sample and the disassembly): `0x9059e0` reads euro.cfg (`CM3_DATA`, mode "r").
  It is called from `0x904040` only when `[this+0x439]` is 2001. The file has `*Nation` header lines, each
  followed by club names. At `0x905a45` the loop tests the line in the buffer `0xde1f64` for a leading `*`. If
  it has none, `0x905a4c` jumps to `0x905c21`. There `test eax, eax` / `jne 0x905a45` tests the same line again
  and nothing reads a new line. Three error paths read one line and jump to `0x905c21`: a header names a nation
  that is not in the list at `this+4` (`0x905a8c`), the matched nation pointer is null (`0x905ab1`), and the
  nation has no club that passes the filter (`0x905af3`). The next line is a club name, so the loop never ends.
  The original 3.9.68 has this loop too. The official data never reaches it.
- Fix: `tools/recomp/widescreen.py` (INSNS, address `0x905a4c`, checked against the text "jne 0x905c21").
  When the line is not a header, the code reads the next line first (call `0x90cba0` with the file in edi and the
  buffer `0xde1f64`) and jumps to `0x905c21` with its result. The loop then ends at the next header or at the end
  of the file. A club list of a skipped header is skipped, so the seeding for that nation stays unchanged.
- Official data: same-save (14 game days) and the match check give saves that are byte for byte the same with
  and without the fix (a build of the same branch without it).

## What the patchers do

From the public pages that I could read (the CM0102Patcher readme on GitHub): v1.07 "Fix for Euro.cfg when
updating Holland->Netherlands" and v1.18 "Auto-fix Euros Crash when setting year to 2019". Both point at euro.cfg
nation names that no longer match the database. I found no change list that names `0x904040` or `0x9059e0`.
The Update Team says its unpatched databases are for use with Nick's patcher or Tapani's patch. Unknown: what
those patchers change in euro.cfg.

## Results

| Folder | Result | Seconds | Failure step |
|---|---|---|---|
| April 2010 SIM | pass | 71 |  |
| April 2011 SIM | pass | 71 |  |
| April 2015 | pass | 73 |  |
| April 2016 | pass | 73 |  |
| April 2017 | pass | 72 |  |
| April 2021 | pass | 74 |  |
| April 2022 | pass | 75 |  |
| April 2023 | pass | 75 |  |
| April 2024 | pass | 75 |  |
| Autumn 2013 | pass | 72 |  |
| December 2008 | pass | 71 |  |
| December 2010 SIM | pass | 73 |  |
| December 2025 | pass | 76 |  |
| EmptyDatabase | FAIL | 16 | code 70 after 3-leagues: 0 
recomp: script wait typing (at most 120000 ms)
recomp: free of 0x70592c30, which is not a heap block (from  |
| February 2008 | pass | 72 |  |
| February 2010 ODB | pass | 71 |  |
| February 2011 ODB | pass | 71 |  |
| June 2010 | pass | 70 |  |
| March 2009 | pass | 74 |  |
| March 2012 ODB | pass | 72 |  |
| March 2012 SIM | pass | 71 |  |
| March 2013 | pass | 72 |  |
| March 2014 | pass | 72 |  |
| March 2018 | pass | 73 |  |
| March 2019 | pass | 73 |  |
| March 2020 | pass | 73 |  |
| May 2026 | pass | 75 |  |
| November 2007 | pass | 71 |  |
| November 2020 | pass | 100 |  |
| October 2003 Sawczuk | pass | 73 |  |
| October 2014 | pass | 72 |  |
| October 2015 | pass | 72 |  |
| October 2016 | pass | 72 |  |
| October 2017 | pass | 72 |  |
| October 2018 | pass | 73 |  |
| October 2019 | pass | 124 |  |
| October 2021 | pass | 74 |  |
| October 2022 | pass | 79 |  |
| October 2023 | pass | 78 |  |
| October 2024 | pass | 79 |  |
| Original 3.9.60 Swedish_fixed | pass | 75 |  |
| Original 3.9.65 Swedish_fixed | pass | 82 |  |
| Original 3.9.68 | pass | 74 |  |
| September 2005 Helvete | pass | 73 |  |
| September 2008 | pass | 76 |  |
| September 2009 | pass | 75 |  |
| September 2010 ODB | pass | 72 |  |
| September 2011 ODB | pass | 72 |  |
| September 2012 | pass | 77 |  |
| May 2026 + clean euro.cfg | pass | 83 |  |

EmptyDatabase fails after Select League(s): the game stops with code 70 when it builds the new game, because the
database has no clubs. The row "May 2026 + clean euro.cfg" is the mismatch case that hung before the fix.
