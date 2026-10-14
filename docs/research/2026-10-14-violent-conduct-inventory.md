# Violent conduct inventory (players who push or hit the referee)

Date: 2026-10-14. Original: `clean-3968/cm0102.exe` 3.9.68. Research only.

## Answer in short

There is no hidden mode, setting, cheat or command-line flag. Referee assaults, fights and long bans are part of the normal match engine of 3.9.68. They are always on. They are rare because each one needs two or three random rolls in a row, and the first roll depends on a per-match "anger" value of the player. A player who pushes or hits the referee gets a straight red card and an automatic ban of 90 or 180 days. I found no gate that the shipped game switches off.

## 1. Web sources

- FM Scout, "My FM Flashback" thread, 2010, https://www.fmscout.com/q-1753-My-FM-Flashback.html. A poster recalls "AND ROBERT BAKKER PUNCHES THE REFEREE" and a 13 match ban. The poster says FM 2007, not CM 01/02.
- Lutris game page, https://lutris.net/games/championship-manager-0102/. CM 01/02 feature list: the FA fines a club "if their players are involved in a brawl".
- StrategyWiki, https://strategywiki.org/wiki/Championship_Manager:_Season_01/02. No mention of referee assaults.
- SI community thread 292028 "Can a player punch a referee", https://community.sports-interactive.com/forums/topic/292028-can-a-player-punch-a-referee/. I could not read it. The site returned HTTP 403 and a Cloudflare check, and the Internet Archive was offline. Its game version and content: unknown.
- I found no source for a hidden mode, registry value, editor flag or patch for this in CM 01/02, CM 3 or CM 4.

Inference: Max remembers the normal 01/02 behaviour below. The 13 match ban in FM 2007 is a different game.

## 2. Text and data

### Match commentary (Data/events_*.cfg, plain text, one file per language)

Format: `= id, priority, ...` then `G` (live text) and `R` (report text) lines. All 10 language files have these events.

| cfg id | Name in file | Example text (eng) |
|---|---|---|
| 408 | EVENT_PLAYER_ARGUES_WITH_REFEREE | "<p1> is furious with <rf> !" |
| 409 | EVENT_TEAM_ARGUES_WITH_REFEREE | players surround the referee |
| 410 | EVENT_PLAYER_PUSHES_REFEREE | "<p1c> HAS PUSHED THE REFEREE !!!" |
| 411 | EVENT_PLAYER_STRIKES_REFEREE | "<p1c> PUNCHES THE REFEREE !!!" |
| 417 | EVENT_PLAYER_PUNCHES_OPPONENT | right hook, left hook |
| 418 | EVENT_PLAYER_PUSHES_OPPONENT | shoves |
| 419 | EVENT_PLAYER_HEADBUTTS_OPPONENT | headbutts |
| 420 | EVENT_PLAYER_KICKS_OPPONENT | kicks out |
| 421 | EVENT_PLAYER_ELBOWS_OPPONENT | elbows |
| 422 | EVENT_PLAYER_KICKS_BALL_AWAY | the fallback when a referee assault roll fails |
| 456 | EVENT_PLAYER_SPITS_AT_OPPONENT | |
| 457 | EVENT_REFEREE_CALLS_PLAYER_OVER | |
| 458 | EVENT_TICKING_OFF | |
| 459 | EVENT_PLAYER_SQUARES_UP_TO_OPPONENT | |
| 460 | EVENT_BIG_PUNCH_UP | "It's a full on brawl !" |

The code uses internal id = cfg id + 8000. Evidence: `FUN_006d6f70` (post a match event) accepts ids 8000 to 0x21e4 and indexes the 18-byte event table at `0xb68788` with `id - 8000`. The loader `FUN_006d9e50` reads the file for the language (`0x6d9f9e` references "events_eng.cfg" at `0xa15c3c`).

### News and ban text (in the exe, not in Data/*.lng)

- Ban news templates "has been banned from {scope} matches for <%d> days ... after his misconduct" (string table after `discipline.cpp` at `0x9b8f18`).
- Ban reasons list: "Unprofessional Behaviour", "Misconduct", "Red card", "Invalid passport", "Use of an illegal substance".
- Long ban labels "<5 months> <World> ban" and scopes "domestic", "continental", "world-wide".
- Manager fines (`Fine.cpp`, "fine.dat"): reason "Violent Behaviour" and the replies "accepted ... his fine for violent conduct". The manager chooses this fine. The engine does not create it.
- "Players protest" and FA club fine for "continuing disciplinary problems".

## 3. Code that creates the events

### Anger check `FUN_007164b0(level, target)` (match player, thiscall)

- Reads float `this+0x1a2` (an "anger" value, 0 to 100) and squares it (`0x7164ca` to `0x7164db`, constant 2.0 at `0x968950`). Call it A2.
- Threshold t = 8 if `target` equals `this+0x1a6` (the player he is angry with), else 1.
- Level 1: `rand(110 - anger) < t` (`0x7164fa`).
- Level 2: `rand(400000 / A2) < t` (`0x716512`, constant `0x96d860`).
- Level 3: `rand(2000000 / A2) < t` (`0x716531`, `0x96aff8`).
- Level 4: `rand(4000000 / A2) < t` (`0x71653d`, `0x96d858`).
- Level 5: `rand(8000000 / A2) < t` (`0x716563`, `0x96d850`).
- `FUN_0090cfc0(n)` is the game's random number. Inference: it returns 0 to n-1.
- The anger value is raised and clamped to 100.0 in many places, for example `0x6f28e5`, `0x6f4b0e`, `0x716757`, `0x7169d5`. Inference: it rises when the player commits fouls, gets fouled or gets into fights. The exact rules: unknown.

### Reaction to the referee `FUN_00716e10(param)` (match player, thiscall)

Called after a foul decision, for example at `0x6f1dd9`, `0x6f679e`, `0x6f8390`, `0x6f8ce5` (17 call sites).

1. Level 5 passes: roll `rand(20 - this+0x75) == 0` (20.0 at `0x96afb0`) and `this+0x75 >= 5.0` (`0x9688a0`). Both pass: post 0x20db (411, punches the referee) at `0x716e7a`, severity 5. Else post 0x20e6 (422, kicks the ball away), severity 3.
2. Else level 4 passes: same two rolls. Pass: post 0x20da (410, pushes the referee) at `0x716ee9`, severity 4.
3. Else level 2: severity 3 (kicks ball away). Else level 1: post 0x20d8 (408, argues), severity 1 or 2.
4. The severity goes to the referee decision `FUN_006cfd40` -> `FUN_006eab60`. Severity 4 and 5 add 100 to the referee roll. The result is a straight red card (out code 4, event 184 EVENT_SENT_OFF) with offence code 12 in match-player byte `+0x25` or `+0x26`. Codes for the others: severity 1 or 6 -> 5, 2 -> 8, 3 -> 1.

Field `this+0x75`: set at `0x6edc37` to player record byte `+0x1c` times 0.5. Then `0x6ee829` subtracts (50 - non-player `+0x5b` - non-player `+0x59`) times 0.1. Inference: `+0x1c` is Aggression (1 to 20) in the community player.dat layout. I did not verify the names of `+0x1c`, `+0x59` and `+0x5b`. So only players with a high value of this attribute (about 10 or more after the change) can push or hit the referee.

### Fights between players `FUN_00716630(depth)`

- Main loop `0x6b9b64` to `0x6b9c8e`: during a stoppage (match `+0x8eb2` not 0, no fight running `+0x8eb4`), it tries up to 20 random players. A player who passes level 1 goes to `FUN_006d1210`, which calls `FUN_00716630`.
- For each opponent near him: level 4 -> headbutt (0x20e3, 419). Level 3 -> spit (0x2108, 456) with chance 10 in 1000 or 10 in 300 (depends on a byte equal to 8 at `+0x77`), else 4 in 10 kick (420), 5 in 10 punch (417), 1 in 10 elbow (421). Level 2 -> push (418) or square up (459).
- Each act multiplies the opponent's anger by (4.0 - 0.16 x n) where n is the byte at `opponent+0x69 -> +0x5d` (inference: Temperament). So one act can start the next one. The function calls itself (`0x716c51`) up to 5 people. With 2 or more people and `rand(5) != 0` and enough people, it posts 0x210c (460, brawl) at `0x716a5f`.
- The referee decision for fights is `FUN_006cfc60`. Its card and offence codes: unknown.

### The long ban `FUN_0055e300` (discipline.cpp, called at `0x7e8244`)

- Runs only if offence code 12 is in record `+0x2e` or `+0x2f` (`0x55e352` to `0x55e35f`).
- Clears the player's normal pending bans (`FUN_00556d00` loop).
- Makes a ban of 90 days (`0x55e3e9`, `mov word [esp+0x2a], 0x5a`). It makes 180 days (`0x55e423`, 0xb4) if `FUN_0054cdb0(game date)` is below 80. Inference: that function returns a day count, so a late-season assault gets the longer ban. Scope byte 0xff, type 3. Stored with `FUN_00556940` and `FUN_00557280`.
- Then the club may react (`FUN_00758de0` with mode 1, 2 or 3). Inference: transfer list, fine or release. Unknown.

### Gates

- No global flag, setting, difficulty value, game.cfg value or command-line option is read in `FUN_007164b0`, `FUN_00716e10`, `FUN_00716630`, `FUN_006eab60` or `FUN_0055e300`.
- The only gates are the anger value, the attribute at `+0x75`, the target bonus (t = 8) and the random rolls.
- Not checked: every caller of these functions. A gate in a caller is possible but I saw none at the call sites I read.

## 4. How often

Inference from the formulas. Not measured.

- Punch the referee: per foul decision, P = 1/(8000000/A2) x 1/(20 - x) with x = `+0x75`. At anger 100 and x = 10: 1/800 x 1/10 = 1 in 8,000. At anger 50: 1 in 32,000. Below x = 5: never.
- Push the referee: double the punch rate, and only if the punch roll failed.
- Inference: across a full season of all leagues this gives a few referee assaults per season. A human manager may see one every few seasons. A run is needed to confirm.

## 5. Recommendation for the Bosman Edition

A switch that makes these events more frequent changes the game's decisions. The 0.2.0 build brief forbids that ("No feature may change match results or the game's decisions"). Max must approve an exception before any build.

If Max approves, the option could be `violent_conduct = normal | frequent` in the settings file, default `normal`.

- Hook `FUN_007164b0` (`0x7164b0`) with `RT_WRAP`. For `frequent`, divide the level 4 and level 5 divisors (the doubles at `0x96d858` and `0x96d850`) by a factor, for example 10. Do not patch the constants in place: `0x96d850` has 2 readers and `0x96aff8` has 3. Patch in the wrapper instead.
- Optional: hook `FUN_00716e10` (`0x716e10`) to drop the `+0x75 >= 5.0` test. Do not change the constant at `0x9688a0`: many functions read it.
- The ban length is the immediates at `0x55e3e9` (90) and `0x55e423` (180). Leave them.

Risks:

- Crashes: low. The events, text, sounds, red card and ban code all exist and run in the shipped game. Higher rates will reach paths that are rare today, for example the fight recursion limit (5 people) and many red cards in one match. Test a team that falls below 7 players.
- Missing text: none expected. All 10 language files have ids 408 to 460. The ban news templates are in the exe.
- Save compatibility: no new data. Bans are normal ban records. A save made with `frequent` loads with `normal` and the reverse.
- Determinism: any change to the random calls changes every later match result. The smoke reference runs will no longer match with `frequent`. They must still match with `normal`.
