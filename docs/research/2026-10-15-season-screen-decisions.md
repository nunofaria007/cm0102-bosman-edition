# Season screen (0.5.0): decisions so far

Settled with Max on 2026-10-15. The full grilling and the spec come before the build of 0.5.0.

- **Base (Q143, Q144):** 0.4.0 slice 12 adds custom data sets: the Database choice lists folders in `Databases` in the save folder. The start year stays July 2001 in 0.4.0.
- **Where (Q145):** a "Choose Season" button on the start screen opens a list of seasons from 2001/02 to 2025/26. Setup Game shows the chosen season and keeps the Database choice.
- **Start year (Q146):** a chosen season starts the game in July of that season. Research first: what Nick's patcher and Tapani's patch change in the original.
- **Data (Q147):** Max asked the CM 01/02 Update Team (champman0102.co.uk, director Dermotron) and they gave permission. Max, 2026-10-15: "the databases are free to use". Hosting or download details are decided in the 0.5.0 grilling.
- **One line per season (Q148):** the newest database of each season; other versions (October/April, ODB/SIM style) under "More versions".
- **Release (Q149):** the season screen is the headline of 0.5.0.
- **Credit:** the screen and the site credit the Update Team for every database.
- **Layout (Max, 2026-10-15):** "A3 + both" in `docs/research/mockups/2026-10-15-season-picker.html` (the third screen): big tiles for all 25 seasons under coloured era headings (the originals in red, the 2000s in amber, the 2010s in green, the 2020s in blue), tiles tinted in their era colour with a coloured top edge, a status tag on each tile (Official, Installed, Download), a star on seasons with saved games, and a strip below with the chosen season, its database, the start date and "More versions".
- **Nick's patcher (Q163, Q164, 2026-10-16):** the start year change and the name updates by year go into the season screen, with Nick's CM0102Patcher source (github.com/nckstwrt/CM0102Patcher, no licence file: facts and addresses only, no copied code) as the research map. The crash fixes, the restricted-player marker, the position in the tactics view and the faster screens go into 0.5.0 as small slices.
- **Season rules (Q165, Max):** the gold standard is Dav's restructure patch: each chosen season has the rules and league structures of that year. To keep it manageable, the rules change at known change points from 2001 to 2026 (for example the Intertoto Cup ends, the Conference League starts), not every year. Research: a full list of those change points and what the game can model (`docs/research/2026-10-16-season-change-points.md`). These rule changes apply only to seasons the player chooses, so the official 2001/02 and 2002/03 games stay as they are.
- **Eras (Q166):** five eras (original 2001/02 to 2002/03; 2003/04 to 2008/09; 2009/10 to 2019/20; 2020/21 to 2023/24; 2024/25 onward), plus a table for each season for club counts and names. Full table: `docs/research/2026-10-16-season-change-points.md`.
- **Release order (Q167, Max):** the small feature updates come first. The big data and league updates (eras, league sizes, rules, European cup formats, new competitions) are a release of their own.
- **Permission (Q168, Max, 2026-10-16):** Max asked Dav (dm-carmo) and Nick (nckstwrt) and decided: "it is fine to use as it is public shared and it's a free game". Public pages (site, README, issues) do not name them (Max, Q172).
- **New competitions (Q169, Max):** build the competitions that the game has no code for, such as the Nations League and the 48-team World Cup.
- **First-season special cases (Q170):** the original's special cases tied to 2001 move with the start year, as in Nick's patcher.
- **Season screen release (Q171):** the Choose Season screen ships with the big data and league release, so each season arrives with its own rules.
- **Release order (Q172):** 0.5.0 the small features (crash fixes, restricted-player marker, position in the tactics view, faster screens); 0.6.0 easy online play (#45); 0.7.0 the seasons release (Choose Season screen, eras, league structures, rules, European formats, new competitions).
