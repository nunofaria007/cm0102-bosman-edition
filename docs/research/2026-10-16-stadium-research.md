# Stadium expansion and new stadium: research for issue #55

Date: 2026-10-16. Exe: `clean-3968/cm0102.exe` (3.9.68). Tags: **[code]** read from the decompile or disassembly, **[data]** read from `stadium.dat`, **[inf]** inference, **[check]** from memory, not confirmed by a source.

## 1. Later games

Sources found are forum posts. I found no Sports Interactive page with the numbers.

- FM requests sit under the board or Club Vision screen: "expand the stadium" and "build a new stadium". [FM Scout q-2864](https://www.fmscout.com/q-2864-Board-Request.html), [FM Scout q-22271](https://www.fmscout.com/q-22271-Unable-to-Expand--Build-new-staidum.html).
- What the board weighs: finances first (an SI reply, quoted in the Steam FM2020 thread), then attendance near capacity, season tickets, board confidence. [Steam FM2020](https://steamcommunity.com/app/1100600/discussions/0/2673382867121666546), [Steam FM2019](https://steamcommunity.com/app/872790/discussions/0/1627412171709023266), [Steam FM2024](https://steamcommunity.com/app/2252570/discussions/0/4703539060902775305).
- A hidden per-stadium expansion limit exists (editor field). Some clubs can never expand. Same threads.
- Council planning refusal is a possible outcome. Same threads.
- After a refusal the option disappears for a time. [FM Scout q-22271](https://www.fmscout.com/q-22271-Unable-to-Expand--Build-new-staidum.html).
- New stadium: about once per 20 years (FM17 claim). The cost is paid with a loan, the old ground is sold, and a sponsor pays a share. Same threads and [FM Scout q-17137](https://www.fmscout.com/q-17137-Building-A-New-Stadium.html).
- A guide summary (FM Inside, second-hand) says: old ground at least 25 years old, sold out, money in the bank, board trust, and about EUR 400m for 50,000 seats. **[check]**: I could not read the original.
- Promotion can force an expansion or rebuild. Expansion can cut capacity (a ground went from about 1,000 to about 600 after a promotion notice). [Stadium Logic](https://fmprojects.substack.com/p/stadium-logic).
- CM 03/04: the 4.1.3 patch fixed "expansions that did not restore seating or terracing lost during work". So 03/04 cut capacity during the work. [Worthplaying patch notes](https://worthplaying.com/article/2003/12/22/news/14384-championship-manager-03-04-update-patch-fix-available-now/).
- **[check]** From memory: the work runs in the close season, a new stadium takes about 2 to 3 seasons, the board names it, AI clubs follow real plans from the database, and FM 2005 to 2008 had only "improve facilities". Version-by-version changes: I don't know.

## 2. What CM 01/02 3.9.68 already has

### Stadium record (78 bytes, `DAT_00ae23b0`, 8,917 records) [data][code]

| Offset | Field | Evidence |
|---|---|---|
| 0x3c | capacity | Highbury 38,420. Read by gate and board code |
| 0x40 | seated capacity | Huish Park 9,200 / 5,212 seated |
| 0x44 | expansion capacity | Old Trafford 100,000, Anfield 50,000. **0 for 7,269 stadiums, Highbury included** |
| 0x48 | nearby stadium | 906 set |
| 0x4c | retractable roof | 45 set. Text "with retractable roof" |
| 0x4d | under-soil heating | 197 set |

Club: +0x69 stadium, +0x6d owns it (0 = rented, Brighton and Wimbledon), +0x6e second stadium, +0x73 attendance figure [inf: average], +0x7f training facilities 1..20, +0x80 reputation (short).

### The player's request already exists [code]

- Board Confidence screen (0x6b2130, opened by 0x6b2080) has "Board Request" and "FA Request" buttons for a club manager. Handler 0x6b2370: "Board Request" opens "Meet with Board" (0x425390 / 0x4253f0).
- Six options (0x6a8770): 1 Expand the Stadium, 2 youth, 3 transfer funds, 4 wages, 5 time, 6 win bonus.
- Outcome code at 0x6a8810, stadium case 0x6a8a37. Reaction news 0x1786 (text 0x6a9470 and 0x6adb70):
  - Requests counted in a per-club board record (`DAT_00b67a44`, 0x49 bytes per club). Over a limit of 2 + chairman attribute/5 (or 3): "too many requests" and board confidence -250.
  - Board confidence (record +6) under 3,500: refused.
  - Manager reputation under 5,000, or under club reputation - 1,500: "inexperienced manager".
  - Flag 0x40 already set: "plans already afoot".
  - Rented ground: "a commercial decision".
  - Capacity over 75,000: refused.
  - Money: balance at least GBP 2.5m (capacity up to 15,000), 5m (up to 25,000), 7.5m (more). Else "no money".
  - Attendance over 0.85 x capacity (constant 0x969520), then a chairman dice roll: agreed and flag 0x40 is set. Else refused.
- Agreement changes nothing at once. The work happens at the season end (below).

### Season-end stadium routine, all clubs [code]

`FUN_0059cfa0`, called through 0x59cdd0 from the daily `FUN_005d5ae0` [inf: on each nation's season-end date]. For a club in a league:

- Rented and balance over GBP 35m: buys the ground for 20m.
- Under-soil heating: balance over 17.5m, reputation over 7,500, capacity over 25,000: 1.5m.
- Roof: balance over 75m, reputation over 9,250, capacity over 57,500, training 20: 7.5m.
- Expansion when balance is at least 5.25m (human club: balance + reputation x 1,000), capacity at most 125,000, and one of these:
  - flag 0x40 (the player's agreed request),
  - league average attendance over 0.92 x capacity,
  - capacity under 2 x reputation and reputation over 4,250.
- Seats added in one step: at most 16,000, at most expansion capacity - capacity, at most max(expansion capacity / 3, 5,000), rounded to 500, at least 1,000. Cost ((n/1000 + 1) x 6,000 + n) x 125: about GBP 875 per seat, 4.4m for 5,000. Lower-level clubs add standing at about GBP 100 per place.
- If nothing can be added and capacity is under 30,000: expansion capacity +1,500.
- Otherwise it may convert standing to seats (cost (n/1000 + 1) x 50,000 + 75 per seat). The code also adds n to capacity here, which looks like a bug.
- Player's flag without money: news "expansion plans shelved" or "council blocks planning permission" (0x59fae0, news 0x1788).
- News 0x177d (text 0x5a0050): "enlarge the stadium by n seats", "standing area", "install n seats", buy, sell, heating.
- Fixed moves: Brighton to Falmer and Everton to Kings Dock, from 16.5.2004 (0x59b9c0).
- Other events: health and safety closures (0x177a), promotion refused for a small ground (0x48ee40).

So the AI already expands every season. Arsenal (expansion capacity 0, capacity over 30,000) can never expand, even after the board agrees.

### Screens

- Club information (0x460590): Stadium, capacity with seated, roof and heating, Facilities, "(Rented Stadium)".
- Board Confidence (0x6b2130), Meet with Board (0x4253f0).
- News items above.

## 3. Proposed design

Q152: the AI routine stays unchanged. Only the player's own request changes state.

1. **Expand.** Keep the original request and rules. One change: when the board agrees and the expansion capacity is at most the capacity, set the expansion capacity to capacity x 1.25 (at least +5,000). The original season-end routine then adds the seats with its own costs.
2. **New stadium.** Add option 7 "Build a new stadium". Board rules:
   - the club owns its ground,
   - attendance over 0.95 x capacity,
   - capacity is at most 15,000 below the expansion capacity, or the expansion capacity is 0,
   - reputation at least 5,000, manager reputation at least 5,000, board confidence at least 3,500,
   - balance at least 25 % of the cost,
   - once per 10 seasons.
3. **Cost and time.** GBP 1,000 per seat up to 30,000 seats, 2,000 above. The club pays in 8 quarterly parts over 2 seasons. The new ground opens at the start of the third season. The size is 1.5 x or 2 x the old capacity (the player chooses), all seated, expansion capacity x 1.25.
4. **State.** Reuse the club's stadium record: rename it and set capacity, seated and expansion. Store the build in the save. The original sets nothing else.
5. **AI.** No change. AI clubs keep the original routine and never build new grounds.

## Open decisions for Max

1. Fix the expansion-capacity dead end only on the player's agreed request, or for all clubs? All clubs changes the AI (Q152).
2. Seats at the season end (original), or after a build time with a capacity cut?
3. The numbers for the new stadium rules, cost and build time.
4. Name: the player types it, or "<club> Stadium"?
5. Ground shares: other clubs that use the same record move too. New record, or block the request?
6. Pay at once, or in parts? I don't know if the finance block has a loan field.
7. Show the expansion capacity on the club information screen?
