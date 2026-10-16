"""Checks that no hidden attribute of another club's player is shown (spec 2026-10-15-features-0.4.0, Q111).

The runtime writes, with RECOMP_VIEWS_LOG=1, for each player that best XI, squad depth or the picker looks
at for a viewer (src/recomp_rt/win/picker.c log_hidden):
    recomp: picker attributes staff <id> values <31 numbers, 0 hidden> hidden <31 flags>
where a flag of 1 is the game's own answer (0x543280) that the viewer's screens hide the attribute. The
comparison writes the same flags for its two players ("hidden1", "hidden2" in its line, compare_run.py).

The checks use only the log: a flagged attribute has the value 0, and each score that a screen shows is the
score of the key attributes that the viewer sees (from the values of the log), never more. A player whose key
attributes are all hidden has no score (-2), and the screen shows "-".
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import picker_check  # noqa: E402

ATTRIBUTES = re.compile(r"recomp: picker attributes staff (-?\d+) values ([\d ]+) hidden ([01 ]+)")
ATTRS, KEYS = picker_check.ATTRS, picker_check.KEYS
ROLES = [(0x1, "GK"), (0x2, "DC"), (0x4, "DC"), (0x8, "DMC"), (0x10, "MC"), (0x20, "AM"), (0x40, "FC")]


def attribute_lines(log):
    """[(position in the log, staff id, values, hidden flags)]."""
    out = []
    for m in ATTRIBUTES.finditer(log):
        out.append((m.start(), int(m.group(1)), [int(v) for v in m.group(2).split()],
                    [int(v) for v in m.group(3).split()]))
    return out


def latest(lines, staff, before):
    """The values of staff in the last attribute line before position before, or None."""
    found = None
    for at, s, values, _ in lines:
        if at < before and s == staff:
            found = values
    return found


def roles_of(mask):
    """The key lists that a slot mask can use: one for each position bit, the wide variants too."""
    roles = set()
    wide = bool(mask & 0x880)
    for bit, role in ROLES:
        if mask & bit:
            roles.add({"DC": "DLR" if wide and bit == 0x4 else "DC", "MC": "MLR" if wide else "MC"}.get(role, role))
    return roles or {"FC"}


def expected_scores(mask, fit, values):
    """The picker values that the visible attributes allow for a slot, one for each role that the mask can
    take: -2 (no score) when every key attribute is hidden, else sum x fit x 60 / count."""
    out = set()
    for role in roles_of(mask):
        seen = [values[ATTRS.index(k)] for k in KEYS[role] if values[ATTRS.index(k)]]
        out.add(-2 if not seen else sum(seen) * fit * 60 // len(seen))
    return out


def check_flags(name, log):
    """A flagged attribute has the value 0, and some flag is set (the masking was exercised)."""
    problems, any_hidden = [], False
    for _, staff, values, hidden in attribute_lines(log):
        if len(values) != 31 or len(hidden) != 31:
            problems.append(f"{name}: staff {staff}: {len(values)} values and {len(hidden)} flags, expected 31")
            continue
        for i, (v, h) in enumerate(zip(values, hidden)):
            any_hidden |= bool(h)
            if h and v:
                problems.append(f"{name}: staff {staff}: {ATTRS[i]} is hidden by the game but holds {v}")
    if not any_hidden:
        problems.append(f"{name}: no hidden attribute in the log: the masking was not exercised")
    return problems


def check_draw_scores(name, log, draws, masks_of=None):
    """Each score of a best XI draw (draws as best_xi_run.draws) equals a score that the visible key
    attributes of the player allow. Draw lines are matched to the attribute lines before them."""
    problems, lines, at = [], attribute_lines(log), 0
    for d in draws:
        marker = f"recomp: best xi club {d['club']} tactic {d['tactic']}"
        at = log.find(marker, at)
        if at < 0:
            continue
        for s, (staff, fit, score) in enumerate(zip(d["staff"], d["fit"], d["score"])):
            if staff < 0:
                continue
            values = latest(lines, staff, at)
            if values is None:
                continue                            # no viewer: the player's own club
            mask = int(d["masks"][s], 16)
            if score not in expected_scores(mask, fit, values):
                problems.append(f"{name}: staff {staff} in slot {s} (mask {mask:x}) has score {score}, the visible "
                                f"attributes allow {sorted(expected_scores(mask, fit, values))}")
    return problems
