"""Screen size constants of the original that become run-time values, for widescreen mode.

The original draws at 800x600 and writes the size as constants in some instructions. Each entry names
the address of one instruction, the constant it holds and the C expression that replaces it. The runtime
sets rt_screen_w and rt_screen_h (rt.c: 800x600 by default, main.c: from the settings file).

Only constants that do not go into the area or widget create functions are here: the runtime scales
those coordinates when it creates the area or widget (src/recomp_rt/win/display.c). The evidence for
each address is in docs/research/2026-10-14-widescreen-inventory.md.
"""

W = "rt_screen_w"
H = "rt_screen_h"
W1 = "(rt_screen_w - 1u)"
H1 = "(rt_screen_h - 1u)"

# address: (constant in the original, C expression)
PATCHES = {
    # Area init 0x402ac0: x2 and y2 are clamped to 799 and 599.
    0x402B7C: (799, W1), 0x402B84: (799, W1), 0x402B8D: (599, H1),
    # Popup placement 0x403760: the popup opens below its widget if it ends above y 600.
    0x40384B: (600, H), 0x403922: (600, H), 0x4039B5: (600, H),
    # Area scroll and drag limits 0x403a70.
    0x403B00: (799, W1), 0x403B64: (799, W1), 0x403BDC: (599, H1), 0x403C40: (599, H1),
    # Full-screen clear and flip before a progress bar 0x55f2c0.
    0x55F2CA: (599, H1), 0x55F2CF: (799, W1), 0x55F2E7: (599, H1), 0x55F2EC: (799, W1),
    # Screen draw 0x55f300: the plain background, the clear, the fade and the flip of the whole screen.
    0x55F38D: (599, H1), 0x55F392: (799, W1), 0x55F56B: (599, H1), 0x55F570: (799, W1),
    0x55F58D: (599, H1), 0x55F592: (799, W1), 0x55F5EB: (599, H1), 0x55F5F0: (799, W1),
    # Widget drag 0x561210: clamped to the screen.
    0x5612A0: (799, W1), 0x5612A8: (799, W1), 0x5612C9: (599, H1), 0x5612D1: (599, H1),
    # Display init calls 0x5e2620(800, 600, ...): start-up, the two settings changes, the network path.
    0x5D79ED: (600, H), 0x5D79F2: (800, W),
    0x5D8645: (600, H), 0x5D864A: (800, W), 0x5D8685: (600, H), 0x5D868A: (800, W),
    0x8AEED3: (600, H), 0x8AEED8: (800, W), 0x8AEEFB: (600, H), 0x8AEF00: (800, W),
    # Outline draw 0x5e7840: the span list starts at x 800, and the lock covers the whole screen.
    0x5E78BA: (800, W), 0x5E78E2: (800, W), 0x5E78EA: (600, H),
    # Error box 0x5e8290: centred in a screen 800 wide.
    0x5E8309: (800, W),
    # Widget init 0x5ee150: x2 and y2 are clamped to 799 and 599.
    0x5EE1F5: (799, W1), 0x5EE20A: (599, H1),
    # Tactics input 0x89f710: the nearest position to a drop, by distance in screen pixels, starts at
    # 800 and counts only if it ends below 800.
    0x8A20F8: (800, W), 0x8A21E2: (800, W), 0x8A260F: (800, W), 0x8A26F6: (800, W),
}

# Instructions whose whole translation is replaced: address: (the instruction in the original, C code).
# The circle plot 0x5e7aa0 finds the pixel of row y as y * 25 * 32 (lea, lea, shl 5): a row of 800 pixels.
# The replacement makes it y * rt_screen_w. The next instruction (add or sub) sets the flags again.
ROW = "eax = (uint32_t)((int32_t)eax / 25 * (int32_t)rt_screen_w);"
INSNS = {a: ("shl eax, 5", ROW) for a in (
    0x5E7B7D, 0x5E7BE5, 0x5E7C42, 0x5E7CB8, 0x5E7D73, 0x5E7DE2, 0x5E7E44, 0x5E7EBB)}

# Calls of the text width function 0x5e5ad0 that must get the width in screen pixels. Every other call
# (screen code that lays out widgets in classic coordinates) gets the width in classic pixels, so that
# a widget placed after a text, or sized to it, keeps its place on the text once the runtime scales
# the coordinates (src/recomp_rt/win/display.c, text_width). These calls compare the width with
# a rect on the screen, or draw:
TRUE_WIDTH = {
    0x561E60,                                   # text entry 0x5617c0: does the text fit the widget
    0x5E6F5F, 0x5E7205, 0x5E722A,               # text draw 0x5e6d80: wrap and align in the box
    0x5E82DE,                                   # error box 0x5e8290: box width (not scaled)
    0x5EE26F, 0x5EE281,                         # widget init 0x5ee150: popup width
    # comp_text 0x4aeaf0 (news text and its name buttons): left as it is for the clickable names work.
    0x4AEB54, 0x4AEDEF, 0x4AF16F, 0x4AF58B, 0x4AF66F,
}

# The attribute groups (#43) replace constants of the player profile build in the same way.
from attribute_groups import PATCHES as GROUP_PATCHES  # noqa: E402

PATCHES.update(GROUP_PATCHES)
