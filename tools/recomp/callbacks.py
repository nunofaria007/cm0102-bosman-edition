"""Functions of the original that the runtime calls by address with rt_callback.

RT_WRAP and RT_REPLACE stop the build when their function is missing. A plain rt_callback call has no
such check, so a wrong address would only fail at run time. This list closes the gap in two ways:

- Each address below must be a translated function that starts with the bytes written here (its first
  eight bytes in cm0102.exe 3.9.68).
- Each address of code that a file in CALLBACK_FILES writes in C code (not in a comment) must be in this
  list. So a new call needs a line here, with the bytes from the original.
"""
import os
import re

CALLBACK_FILES = ("picker.c", "assistant.c", "commentary.c", "holiday_next.c", "views.c", "print_pdf.c", "best_xi.c",
                  "tactics.c", "groups.c", "compare.c", "compare_screen.c", "squad_depth.c", "names.c", "pdf.c", "dbfolders.c")
                                # in src/recomp_rt/win

# address: (bytes of the first eight, name)
CALLBACKS = {
    0x45CBE0: ("5355568b74241085", "can the player play the fixture (squad screen test)"),
    0x5431E0: ("8b44240481ec0002", "shown attribute of a player (staff, index 1 to 31)"),
    0x543280: ("8b4424045685c00f", "is an attribute shown to a manager (manager, staff, index)"),
    0x54C5B0: ("83ec10a06924ae00", "date plus days"),
    0x55F990: ("81ec00020000568b", "popup item create"),
    0x55FBA0: ("a10c29ae008b1510", "popup area create"),
    0x5ACBE0: ("81ec240200008b84", "fixture search"),
    0x5E8290: ("83ec6c8b0d307ab6", "message box (title, text, flags)"),
    0x5FCFF0: ("8b44240481ec0002", "commentary speed of a manager"),
    0x5FD2F0: ("81ec00020000568b", "set the holiday flag"),
    0x5FED50: ("8b44240481ec0002", "selection saved at the start of a holiday"),
    0x6B2D60: ("6a006a006a016840", "push the Go on holiday confirm"),
    0x5FBD00: ("8b44240481ec0002", "tactic of a manager"),
    0x62C400: ("81ec08020000568b", "injury state"),
    0x62C590: ("568b742408578bf9", "condition"),
    0x68D040: ("568bf18b460285c0", "list free"),
    0x68D1B0: ("8b510a85d274098b", "list first"),
    0x6E1CF0: ("558bec83e4f883ec", "captain value"),
    0x7757D0: ("6aff68f0de950064", "international duty"),
    0x7FF010: ("0fbf812a38000081", "screen node slot"),
    0x8040A0: ("8b4424048b54242c", "cell button create"),
    0x41C250: ("5657b95044b700e8", "club background picture"),
    0x53B640: ("66a1f0bdae0081ec", "club colours"),
    0x542100: ("8b44241081ec0002", "club name"),
    0x5ED780: ("64a1000000006aff", "tab bar"),
    0x5EDCC0: ("81ecd4070000566a", "Back and Next bar"),
    0x7FEC60: ("0fbf812a38000081", "screen stack: cannot go back"),
    0x7FEBB0: ("0fbf812a38000081", "screen stack: at the end"),
    0x7FE600: ("81ec08020000538b", "screen push"),
    0x7FF270: ("81ec00020000538b", "set a screen node slot"),
    0x804030: ("8b4424308b54242c", "button with its own rectangle"),
    0x803F30: ("8b44240c8b542408", "screen heading"),
    0x75DB00: ("81ec640300000fbf", "side bar build"),
    0x7615D0: ("81eca4020000538b", "side bar input"),
    0x7643D0: ("83ec5866a17a7cb7", "game actions input (Back, profiles)"),
    0x896A90: ("81ec000200005355", "store a selection slot"),
    0x896FE0: ("81ec00020000538b", "clear a selection"),
    0x897760: ("8b44240481ec0002", "store the captain"),
    0x5428A0: ("8a4c240481ec0002", "name of an attribute (1 to 31, text, size, 0)"),
    0x53EE50: ("b8a4110000e8b667", "positions text of a player (data, text, size, 1)"),
    0x543440: ("81ec000200005355", "shown value of an attribute, masked (viewer, staff, attribute)"),
    0x543540: ("8b54240481ec0402", "shown value of an attribute (staff, data, attribute)"),
    0x5E5AD0: ("b804200000e836fb", "width of a text in screen pixels (font, text)"),
    0x7F7920: ("0fbf057a7cb70081", "player of the comparison (scout manager, 1 or 2)"),
    0x8B1750: ("81ec340400005355", "training manager: train a player"),
    0x41C790: ("5657b95044b700e8", "staff picture as the screen's picture"),
    0x53DBA0: ("81ec000200005355", "full name of a staff record"),
}


def check(image, funcs, win_dir):
    """The problems of the list, as strings. funcs: the translated function addresses."""
    problems = []
    for addr, (head, name) in sorted(CALLBACKS.items()):
        if addr not in funcs:
            problems.append("%#x (%s) is not a translated function" % (addr, name))
        elif image.read(addr, 8).hex() != head:
            problems.append("%#x (%s) starts with %s, not %s" % (addr, name, image.read(addr, 8).hex(), head))
    for fname in CALLBACK_FILES:
        text = open(os.path.join(win_dir, fname)).read()
        text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
        text = re.sub(r"//[^\n]*", "", text)
        for lit in sorted(set(re.findall(r"\b0x[0-9a-fA-F]{5,8}(?=[uUlL]*\b)", text))):
            v = int(lit, 16)
            if image.is_code(v) and v not in CALLBACKS:
                problems.append("%s uses code address %#x that is not in tools/recomp/callbacks.py" % (fname, v))
    return problems
