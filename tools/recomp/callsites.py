"""Calls of the original whose return address the runtime tests, so that a wrap acts on one call site only.

Each entry names a call instruction and the function it calls. The translation checks that the
instruction at the address is that call, and the build stops if it is not. table.c gets a constant
`rt_site_<name>` with the return address (the address of the call plus 5), for the runtime to compare
with the return address on the stack. The evidence for each address is in
docs/research/2026-10-14-columns-and-colours-inventory.md and
docs/research/2026-10-15-squad-dropdown-inventory.md, docs/research/2026-10-15-commentary-speed-inventory.md,
and for setup_menu in
docs/research/2026-10-13-screen-stack-inventory.md section 6a.
"""

# name: (address of the call, called function)
CALL_SITES = {
    # Attribute colours (#19): the label create 0x804100 of an attribute number.
    "profile_value": (0x87AC15, 0x804100),       # player profile, every value cell (0x87a510)
    "squad_attribute": (0x45C7CA, 0x804100),     # squad list, View Attributes (0x456900)
    # Attribute groups (#43): the attribute area and the name label of each cell of the player profile
    # 0x87a510 (docs/research/2026-10-15-attribute-groups.md).
    "profile_area": (0x87A705, 0x55FBA0),
    "profile_name": (0x87AA09, 0x804100),
    # Test saves (testrun.c): the write of 0x1cc stack bytes that the save 0x52dad0 never sets, into
    # index.dat (docs/research/2026-10-15-attribute-groups.md).
    "index_pad": (0x53142B, 0x932E10),
    # Match columns (#23): the player list area of the tactics screen 0x899ea0, 9 columns.
    "tactics_area_heading": (0x89C894, 0x55FBA0),  # with a heading row
    "tactics_area": (0x89C93C, 0x55FBA0),          # without a heading row
    # Match columns (#23): the status text of a row in the Condition and Match Rating views.
    "tactics_status": (0x89E01A, 0x8AADB0),
    # Match columns (#23): the Player Ratings area of the match screen 0x722ed0, 16 columns.
    "ratings_area": (0x722FA3, 0x55FBA0),
    # Data set choice (#39): the menu builder call of the Setup Game build 0x81bab0, with 9 items.
    "setup_menu": (0x81BCC5, 0x5ED300),
    # Assistant drop-down (#12): the Clear Squad button of the squad list 0x456900.
    "squad_clear": (0x458AD0, 0x8040A0),
    # Holiday until next match (#30): the Go on Holiday item create of the left bar build 0x75db00
    # (docs/research/2026-10-15-holiday-next-match-inventory.md, 4a).
    "holiday_item": (0x75F1D4, 0x55F990),
    # Faster commentary (#31): the Very Fast item create of the speed popup on the match screen 0x718e90,
    # and the speed getter call whose result the build copies into the match (esi) at 0x719c4e.
    "speed_very_fast": (0x719861, 0x55F990),
    "speed_copy": (0x719C49, 0x5FCFF0),
}

SITES = {addr: (name, target) for name, (addr, target) in CALL_SITES.items()}
