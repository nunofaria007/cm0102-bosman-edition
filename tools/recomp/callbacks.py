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

CALLBACK_FILES = ("picker.c", "assistant.c", "commentary.c", "holiday_next.c", "print_pdf.c")      # in src/recomp_rt/win

# address: (bytes of the first eight, name)
CALLBACKS = {
    0x45CBE0: ("5355568b74241085", "can the player play the fixture (squad screen test)"),
    0x5431E0: ("8b44240481ec0002", "shown attribute of a player (staff, index 1 to 31)"),
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
    0x896A90: ("81ec000200005355", "store a selection slot"),
    0x896FE0: ("81ec00020000538b", "clear a selection"),
    0x897760: ("8b44240481ec0002", "store the captain"),
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
