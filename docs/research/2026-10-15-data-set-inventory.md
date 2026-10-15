# Data set inventory (#39, slice 10)

Slice 10 of `docs/specs/2026-10-15-features-0.3.0-spec.md`. Date 2026-10-15.

## Answers

1. The 3.9.68 original runs a new game on the 3.9.60 data. The game reaches the first news screen (Saturday 14.7.01 AM) and passes days on holiday, to 21.8.01 in classic mode and 22.8.01 in widescreen. Neither run shows an error. #39 stays in 0.3.0.
2. The 3.9.60 data set is 20 files of 50,120,803 bytes (48 MB). With `gzip -9` the files are 8,829,206 bytes in all. A download grows by about that amount.
3. The official 3.9.65 patch holds full database files: all 20 `*.dat` files, complete, not differences.

## 1. Versions

There was no "3.9.60 patch". 3.9.60 is the version on the free release. The ISO's cm0102.exe (24 Sep 2001) holds the string "3.9.60".

The official updates:
- 3.9.62, October 2001: "latest player and team updates (including Bassett at Leicester)", a language pack and game fixes, about 18 MB (archived SI page, Wayback 20011013022740, downloads_official_cm0102a.shtml). It was only on ftp://ftp.sigames.com/official/cm0102v3962.exe. No archived copy was found.
- 3.9.65: "game and data updates", file cm0102v3965.exe, 19,395,072 bytes, server date 7 Dec 2001 (Wayback 20030430004731). Also on the Eidos FTP mirror (ftp.eidos.co.uk/pub/uk/cm0102/patch/, the same digest). The patch is too early for the January transfer window, so a "winter transfers" claim is probably wrong.
- 3.9.67: "Updated player and team data to 02/03", start date unchanged (changes.txt in the clean files).
- 3.9.68: cm0102_3968.exe, 23,458,816 bytes, server date 22 Nov 2002 (Wayback 20030430004639).

## 2. The 3.9.60 data set and the clean files

- Every `Data/*.dat` of the clean files differs from the ISO copy. club.dat holds 10,580 clubs in 3.9.60 and 13,214 in 3.9.68. staff.dat holds 132,722 people in 3.9.60 and 169,342 in 3.9.68.
- The last year in club_comp_history.dat is 2001 in 3.9.60 and 2002 in 3.9.68.
- index.dat has the same table version numbers (1 or 2) in both sets. The record sizes are the same (club 581 bytes, staff 157 bytes).
- CM0102Loader (github.com/nckstwrt/CM0102Loader) runs a clean 3.9.68 with another Data folder.
- The free release has no licence text. No archived terms were found. Eidos made the game free in December 2008 (Wikipedia).

### What the data set holds

The data set is the database files only: the 20 `*.dat` files that the 3.9.68 patch replaces (city.dat to staff_history.dat), from group "CM3 Data" of the ISO's data1.cab. The game reads every other Data file (languages, `*.cfg`, pictures, tactics) from the clean files. The 3.9.68 patch also replaces `*.lng`, language.ldb, player_setup.cfg and weather.cfg. Those stay at 3.9.68, because they belong to the 3.9.68 game, not to the database. Inference: the 3.9.68 game with its own text files is the safer pair. The run below supports it.

Make it with:

```
python3 tools/clean_files/make_clean_files.py --data-3960 /Users/max/Downloads/Archive/CM0102.iso /Users/max/Projects/cm0102/data-3960
```

The tool checks the ISO hash, writes only to the new folder, and compares the files with `tools/clean_files/manifest-3960.tsv`. The clean files and `manifest.tsv` do not change (`--check` on clean-3968 passes: 760 files).

## 3. The test switch

`RECOMP_DATA_DIR=<folder>` (`src/recomp_rt/win/files.c`, `data_dir_swap`). When the game opens `Data/<name>` of the game folder and `<folder>/<name>` exists, the runtime reads that file. Every other file and every write is unchanged. Without the variable, nothing changes. Slice 11 can use the same path rule for the `database` option.

## 4. The run on 3.9.60 data

The test binary is the test-speed branch (commit 43d4dcb, wait steps) with this slice's change. The steps are the smoke steps of `tests/smoke/smoke_run.py` with three more screen dumps and a longer holiday (60 s). Classic and widescreen ran at the same time (1 min 44 s). Both ended with code 0.

- Select Team shows the 2001/02 leagues: Blackburn, Bolton and Fulham in the Premier Division, Birmingham in Division 1. So the game reads the 3.9.60 data.
- Arsenal squad before Take Control: Seaman, Taylor, Dixon, Luzhny, Campbell, Keown, Adams, Lauren, Cole, Vieira, Grimandi, Inamoto, Van Bronckhorst (injured), Stepanovs, Upson, Tavlaridis, Volz, Grondin and others.
- First news screen: Saturday 14.7.01 AM. News: "2001/02 Premier Division fixture list decided", "Board want the title", "Arsenal appoint Test Smoke as manager". So Arsenal's manager is the player's manager, as on the 3.9.68 data.
- Holiday: the date moves to 21.8.01 (classic) and 22.8.01 (widescreen). The game plays the Inter-Toto Cup, the European Champions Cup qualifiers and the English League Cup.
- Errors: none. The screen log (`RECOMP_SCREEN_LOG=1`) has no error, failure or crash line.

Screenshots (outside git): `/Users/max/Projects/cm0102/shots/slice-10/`.

Not checked: a played match, a save and load, and the same-save check on this data. Slice 11 has these checks.

## 5. The 3.9.65 patch

Downloaded from `web.archive.org/web/20030430004731id_/http://www3.sigames.com/download/official/CM%2001-02/cm0102v3965.exe` into a new temp folder. Not run. 19,395,072 bytes, SHA-256 `3e6c1a248eaf7602abfa2e397fb0571a00ec8ac27ca03d8b91db840c2fcfcbe4`, a PE32 file that `bsdtar` opens.

Contents (bsdtar): fonts, readme_*.txt, setup.exe (6 Dec 2001) and update.dat (143,996,242 bytes, 7 Dec 2001). update.dat has the same form as the 3.9.68 one, with 47 sizes (188 bytes) then the files. The file names come from the strings of setup.exe. In order:

- cm0102.exe (holds "3.9.65"), cm0102_GDI.exe
- euro.cfg, ten events_*.cfg, player_setup.cfg
- the 20 database files city.dat to staff_history.dat
- ten *.lng files, language.ldb (57,763,282 bytes)
- entry 45: size 4,199,560, with no bytes in update.dat (the sizes add up to the file size only without it). What it is, is unknown.
- changes.txt ("Version 3.9.65 (Save game compatible)")

The database files are full files. club.dat is 6,287,001 bytes, exactly 10,821 records of 581 bytes, with the same record layout as 3.9.60. staff.dat is 30,900,029 bytes (3.9.60: 30,150,534). So 3.9.65 can be a third data set (spec Q89 to Q97). The 3.9.65 changes.txt lists only game fixes, no data notes. Not checked: whether the 3.9.68 original runs a new game on the 3.9.65 data.

## 6. Slice 11: the data set choice

Three data sets: "2001/02, original data (3.9.60)", "2001/02, December update (3.9.65)" and "2002/03, updated data (3.9.68)", the default.

- The 3.9.65 data set: `python3 tools/clean_files/make_clean_files.py --data-3965 cm0102v3965.exe /Users/max/Projects/cm0102/data-3965`. The tool checks the patch hash and takes entries 14 to 33 of update.dat. Their sizes match the 3.9.60 files in name order (index.dat 1,482 bytes in both, club.dat 6,287,001). 20 files, 51,540,017 bytes. Manifest: `tools/clean_files/manifest-3965.tsv`.
- The option `database = 3.9.68` (or `3.9.60`, `3.9.65`) chooses the folder `Data-3.9.60` or `Data-3.9.65` of the game folder, with the same rule as `RECOMP_DATA_DIR` (`src/recomp_rt/win/files.c`). `RECOMP_DATA_DIR` wins over the option.
- Setup Game: a tenth menu item, "Database: 2002/03 (3.9.68)", in the free cell after Web Sites. The runtime fills item 9 of the build's menu items and passes 10 to the menu builder (call 0x81bcc5, `tools/recomp/callsites.py`). A click moves the option to the next data set and writes it to settings.txt (`src/recomp_rt/win/database.c`).
- The release script copies both data sets into the game folder of each download and checks them against their manifests.

### Results (2026-10-15)

- Smoke runs to holiday pass on 3.9.60 and 3.9.65 in both modes. Select Team on 3.9.60 and 3.9.65 shows the 2001/02 Premier Division (Blackburn, Bolton, Fulham). The first news screen is the same picture on all three data sets.
- A save 5 game days after the first news screen differs in size by data set: 3.9.60 16,772,269 bytes, 3.9.65 16,944,272, 3.9.68 17,802,721. So the game reads each folder.
- Save and load: a 3.9.60 save loads with 3.9.68 selected, a 3.9.65 save with 3.9.60, a 3.9.68 save with 3.9.60. Each continues the holiday and saves again 3 game days later.
- On 3.9.68 only the Setup Game screen differs from the old smoke pictures, by the new button (5.0% of pixels classic, 4.8% widescreen). The new reference pictures are in `/Users/max/Projects/cm0102/smoke-ref-slice11`, with folders `<mode>-3.9.60` and `<mode>-3.9.65` for the other data sets. After the merge, they replace `smoke-ref`.
- Download size, both data sets: the Mac DMG (UDZO) grows by about 23.06 MB (a DMG of the two folders alone is 23,076,200 bytes, an empty one 16,841). The Windows installer (solid LZMA) grows by about 10.4 MB (xz -9 of both folders: 10,373,176 bytes). For each set alone: gzip -9 8,837,371 (3.9.60) and 9,076,877 (3.9.65). Not measured: a full release build.
