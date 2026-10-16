"""Constants of the player profile build 0x87a510 that become run-time values, for the attribute groups (#43).

The build fills a table of 36 cells of 0x65 bytes at 0xddf570 (a name of 100 bytes, then a value byte),
sorts the first 31 by name, adds five rows and lays the cells out in three columns. The runtime
(src/recomp_rt/win/groups.c) puts the cells in group order in a table of its own, with heading rows.
widescreen.py adds these entries to its PATCHES, so the translation checks each constant in the same way.
Evidence: docs/research/2026-10-15-attribute-groups.md.
"""

# address: (constant in the original, C expression)
PATCHES = {
    # The count of cells that the build shares out over the three columns (mov edi, 0x24). At this
    # point the table is full: the runtime builds its own table and gives its count.
    0x87A81E: (0x24, "rt_groups_cells(esp)"),
    # The value byte of the first cell, where the cell loop starts (mov [esp+0x14], 0xddf5d4).
    0x87A85B: (0xDDF5D4, "rt_groups_table()"),
    # The end of the cell loop (cmp eax, 0x24).
    0x87ACB5: (0x24, "rt_groups_count()"),
}
