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
}
