"""The start screen: pics.cfg, the start screen, the "Please Confirm" screen and their input functions."""
import os
import struct
import tempfile

from cases_helpers import (OFFSET_WIDGET_AREA, OFFSET_WIDGET_ID, OFFSET_WIDGET_STYLE, Start, _hex,
                           _s16, short_text)
from cases_stack import FUN_PUSH, FUN_SET_SLOT, FUN_SET_TITLE, OFFSET_RECORD, STACK

FUN_PICS_LOAD = 0x41B3D0
FUN_START_BUILD, FUN_START_INPUT = 0x81BAB0, 0x81BD70
FUN_CONFIRM_PUSH = 0x591840      # cdecl (title, heading, text, buttons, text colour, colour, code)
FUN_CONFIRM_BUILD, FUN_CONFIRM_INPUT = 0x591B70, 0x591DC0

# Library and system calls that the test answers itself.
CRT_TIME = 0x946EBB
CRT_FGETC = 0x9469E0
CRT_REALLOC = 0x945667
CRT_EXIT = 0x945934
FUN_FILE_EXISTS = 0x90C120       # _access of quick_start.cfg and auto_setup.cfg
FUN_SHUT_DOWN = (0x5CC7D0, 0x5A8F60)
FUN_RESTART = 0x68DBC0           # starts a new copy of the program and ends this one

G_AFTER_POP = 0xB77C84
G_INJECTED_ID = 0xDD6BDE
G_DIALOG_CODE, G_DIALOG_BUTTON = 0xDD6BE0, 0xDD6BE4
G_KEY_CHAR = 0xB67A24
G_PICS = 0x989860                # 11 sections of {name, s16 count, entries}
PICS_SECTIONS = 11
PICS_NAMES = ["nations", "clubs", "stadiums", "comps", "awards", "staff", "training", "boardroom",
              "trophy", "transfer", "si"]

BUILDS = {FUN_START_BUILD: 1, FUN_CONFIRM_BUILD: 2, 0: 0}
INPUTS = {FUN_START_INPUT: 1, FUN_CONFIRM_INPUT: 2, 0: 0}
SCREEN_POP = -9


class Screens(Start):
    """The screen stack with the screens of the start screen, in both programs."""

    def __init__(self, ctx):
        super().__init__(ctx)
        original = ctx.original
        self._patch(original)
        for address, value in ((G_AFTER_POP, 0), (G_DIALOG_CODE, 0), (G_DIALOG_BUTTON, 0), (G_KEY_CHAR, 0)):
            original.write_u32(address, value)
        original.write(G_INJECTED_ID, b"\0\0")
        ctx.port.send("st_set_dialog", 0, 0)
        ctx.port.send("st_after_pop", 0)

    @staticmethod
    def _patch(original):
        if getattr(original, "start_ready", False):
            return
        original.start_ready = True
        original.events = []
        original.sizes = {}

        def time_now(pointer):
            if pointer:
                original.write_u32(pointer, 1000000)
            return 1000000

        def realloc(block, size):
            new = original.alloc(size)
            if block:
                original.write(new, original.read(block, min(size, original.sizes.get(block, size))))
            original.sizes[new] = size
            return new

        original._replace(CRT_TIME, 1, time_now)
        original._replace(CRT_FGETC, 1, lambda stream: Screens._fgetc(original, stream))
        original._replace(CRT_REALLOC, 2, realloc)
        original._replace(FUN_FILE_EXISTS, 1, lambda name: 0)
        for address in FUN_SHUT_DOWN:
            original._replace(address, 0, lambda: 0)
        original._replace(CRT_EXIT, 1, lambda code: original.events.append("exit") or 0)
        original._replace(FUN_RESTART, 0, lambda: original.events.append("restart") or 0)

    @staticmethod
    def _fgetc(original, stream):
        """fgetc on a file in text mode: CR LF becomes LF, and 0x1a ends the file."""
        handle = original.files[stream]
        flag = struct.unpack("<I", original.read(stream + 0xC, 4))[0]
        byte = b"" if flag & 0x10 else handle.read(1)
        if byte == b"\r":
            following = handle.read(1)
            if following == b"\n":
                return 0x0A
            if following:
                handle.seek(-1, os.SEEK_CUR)
        if byte in (b"", b"\x1a"):
            original.write_u32(stream + 0xC, flag | 0x10)
            return 0xFFFFFFFF
        return byte[0]

    def set_title(self, title):
        """The title of the current screen and the answer of FUN_007fee80, in both programs."""
        original = self.ctx.original
        pointer = self.text(title)
        self.call_stack(FUN_SET_TITLE, pointer)
        original.title = pointer
        self.ctx.port.send("st_title", _hex(title))

    def set_player(self, player):
        self.ctx.original.write(STACK + OFFSET_RECORD, struct.pack("<h", player))
        self.ctx.port.send("st_player", player)

    def push_start(self):
        self.call_stack(FUN_PUSH, FUN_START_BUILD, FUN_START_INPUT, 0, 0, 0)
        self.ctx.port.send("st_push_start")

    def push_confirm(self, title, heading, text, buttons, text_colour, colour, code):
        self.ctx.original.call(FUN_CONFIRM_PUSH, self.text(title), self.text(heading), self.text(text), buttons,
                               text_colour, colour, code)
        self.ctx.port.send("st_confirm_push", _hex(title), _hex(heading), _hex(text), buttons, text_colour,
                           colour, code)

    def current_build(self):
        record = STACK + OFFSET_RECORD
        current = struct.unpack("<I", self.ctx.original.read(record + 0x0E, 4))[0]
        return struct.unpack("<I", self.ctx.original.read(current, 4))[0] if current else 0

    def build(self, title):
        """Empties the screen manager and builds the current screen in both programs."""
        self.scene.reset(1)
        self.set_title(title)
        self.ctx.original.call(self.current_build())
        self.ctx.port.send("st_build")

    def input(self, case, clicked):
        """Calls the input function of the current screen in both programs. Returns the result."""
        original = self.ctx.original
        current = struct.unpack("<I", original.read(STACK + OFFSET_RECORD + 0x0E, 4))[0]
        function = struct.unpack("<I", original.read(current + 0x08, 4))[0]
        original.events = []
        result = original.call(function, clicked & 0xFFFFFFFF)
        self.ctx.port.send("st_input", clicked)
        port_result = int(self.ctx.port.reply()[0])
        if "exit" in original.events:
            # The original ends the program in the input function. The port returns SCREEN_QUIT.
            self.ctx.expect_equal(f"{case}: result after exit(0)", -7, port_result, source="wanted")
        elif "restart" in original.events:
            # The original starts a new copy of itself. The port starts the stack again.
            self.ctx.expect_equal(f"{case}: result after the restart", -4, port_result, source="wanted")
        else:
            self.ctx.expect_equal(f"{case}: input result", result, port_result)
        return result

    def set_after_pop(self, value):
        self.ctx.original.write_u32(G_AFTER_POP, value)
        self.ctx.port.send("st_after_pop", value)

    def pop(self):
        """The pop of the main loop: frees the last screen and makes the one before it current."""
        original = self.ctx.original
        record = STACK + OFFSET_RECORD
        first, last, current, flagged, count = struct.unpack("<IIIhh", original.read(record + 0x06, 16))
        prev = struct.unpack("<I", original.read(last + 0x1F4, 4))[0]
        flag = struct.unpack("<I", original.read(last + 0x10, 4))[0]
        original.write_u32(prev + 0x1F8, 0)
        original.write(record + 0x0A, struct.pack("<IIhh", prev, prev, flagged - (1 if flag else 0), count - 1))
        self.ctx.port.send("st_pop")

    def back(self):
        """The result -2 of the main loop: the screen before the current one becomes current."""
        original = self.ctx.original
        record = STACK + OFFSET_RECORD
        current = struct.unpack("<I", original.read(record + 0x0E, 4))[0]
        original.write_u32(record + 0x0E, struct.unpack("<I", original.read(current + 0x1F4, 4))[0])
        self.ctx.port.send("st_back")

    def expect_same_dialog(self, case):
        wanted = list(struct.unpack("<ii", self.ctx.original.read(G_DIALOG_CODE, 8)))
        self.ctx.port.send("st_dialog")
        self.ctx.expect_equal(f"{case}: dialog code and button", wanted, [int(w) for w in self.ctx.port.reply()])

    def expect_same_stack(self, case):
        ctx, original = self.ctx, self.ctx.original
        record = STACK + OFFSET_RECORD
        first, last, current, flagged, count, rebuild, away, done = struct.unpack(
            "<IIIhhiii", original.read(record + 0x06, 0x1C))
        nodes = []
        node = first
        while node and len(nodes) < 50:
            nodes.append(node)
            node = struct.unpack("<I", original.read(node + 0x1F8, 4))[0]
        position = lambda pointer: nodes.index(pointer) if pointer in nodes else (-1 if not pointer else -2)
        wanted = [flagged, count, rebuild, away, done, position(first), position(last), position(current),
                  len(nodes)]
        ctx.port.send("st_dump")
        ctx.expect_equal(f"{case}: record", wanted, [int(w) for w in ctx.port.reply()])
        for index, node in enumerate(nodes):
            build, _, input_, _, flag = struct.unpack("<5I", original.read(node, 20))
            slots = struct.unpack("<120I", original.read(node + 0x14, 480))
            title = original.read(node + 0x1FC, 260).split(b"\0")[0]
            words = [str(BUILDS.get(build, 9)), str(INPUTS.get(input_, 9)), str(flag), "-" + title.hex()]
            for slot in range(60):
                value, owned = slots[slot * 2], slots[slot * 2 + 1]
                words.append("s-" + original.read_c_string(value).encode("latin-1").hex() if owned else str(value))
            ctx.expect_equal(f"{case}: screen {index}", words, [w.decode() for w in ctx.port.reply()])


def _pics_folder(ctx, folder):
    """Picture files in the folder, "stadium" and "old", and a pics.cfg that names them and others."""
    for sub in ("stadium", "old"):
        os.mkdir(os.path.join(folder, sub))
    files = []
    for number in range(12):
        name = f"pic{number}.RGN"
        where = ctx.rng.choice(["", "", "stadium", "old"])
        if ctx.rng.random() < 0.25:
            name = "_" + name
        with open(os.path.join(folder, where, name), "wb") as picture:
            picture.write(b"x")
        files.append(name)
    def picture():
        roll = ctx.rng.random()
        if roll < 0.6:
            name = ctx.rng.choice(files)
            return name.lstrip("_") if ctx.rng.random() < 0.5 else name.lower()
        return ctx.rng.choice(["missing.rgn", "_missing.rgn", "pic 1.rgn", "pic1.rgn"])
    lines = []
    for _ in range(ctx.rng.randint(3, 25)):
        roll = ctx.rng.random()
        if roll < 0.2:
            section = ctx.rng.choice(PICS_NAMES + ["unknown", "NATIONS", "Clubs"])
            lines.append("[" + section + ctx.rng.choice(["]", "]", "", "] # note"]))
        elif roll < 0.3:
            lines.append(ctx.rng.choice(["", "# a comment", "   ", "\t# NOTE: {} denotes", "no colon here"]))
        else:
            names = []
            for _ in range(ctx.rng.randint(0, 5)):
                name = picture()
                if ctx.rng.random() < 0.2:
                    name = "{" + name + "}"
                names.append(name)
            separator = ctx.rng.choice([", ", ",", " , "])
            lines.append(ctx.rng.choice(["England", "Misc", "a b"]) + ctx.rng.choice([": ", ":"]) +
                         separator.join(names) + ctx.rng.choice(["", "", " # end", "{x}", ","]))
    text = ""
    for line in lines:
        text += ctx.rng.choice(["", "", " ", "\t"]) + line + ctx.rng.choice(["\r\n", "\r\n", "\n", "\r\r\n"])
    if ctx.rng.random() < 0.3:
        text = text.rstrip("\r\n")
    with open(os.path.join(folder, "pics.cfg"), "wb") as config:
        config.write(text.encode("latin-1"))


def _original_pics(original):
    lines = []
    for section in range(PICS_SECTIONS):
        _, count, entries = struct.unpack("<IhI", original.read(G_PICS + section * 10, 10))
        words = [str(count), str(int(entries != 0))]
        for index in range(count):
            key, names, names2, list1, list2 = struct.unpack("<IhhII", original.read(entries + index * 16, 16))
            words += ["k-" + original.read_c_string(key).encode("latin-1").hex(), str(names), str(names2)]
            for pointer, number, mark in ((list1, names, "a"), (list2, names2, "b")):
                for item in range(number):
                    text = original.read_c_string(struct.unpack("<I", original.read(pointer + item * 4, 4))[0])
                    words.append(mark + "-" + text.encode("latin-1").hex())
        lines.append(words)
    return lines


def test_pics(ctx):
    original = ctx.original
    for case in range(30):
        Screens(ctx)
        with tempfile.TemporaryDirectory(dir="/tmp") as folder:
            _pics_folder(ctx, folder)
            for section in range(PICS_SECTIONS):
                original.write(G_PICS + section * 10 + 4, bytes(6))
            saved, original.pictures_dir = original.pictures_dir, folder
            original.call(FUN_PICS_LOAD)
            original.pictures_dir = saved
            ctx.port.send("st_pics_load", folder)
            ctx.port.send("st_pics")
            for section, wanted in enumerate(_original_pics(original)):
                ctx.expect_equal(f"pics {case} section {section}", wanted,
                                 [w.decode() for w in ctx.port.reply()])
    return case + 1


def _background(ctx):
    folder = ctx.original.pictures_dir
    names = sorted(name for name in os.listdir(folder) if name.lower().endswith(".rgn"))
    return os.path.join(folder, ctx.rng.choice(names)).encode()


def _prefix(ctx, start):
    """Screens under the start screen: none, another start screen, or a confirm screen."""
    roll = ctx.rng.random()
    if roll < 0.2:
        start.push_start()
    elif roll < 0.3:
        start.push_confirm(b"T", b"H", b"Q", 1, 0xE71C, 0xF800, 0xB)


def _menu_area(start):
    """The area of the menu buttons, which have style 0x22 or 0x822."""
    for index in range(start.widget_count()):
        if start.widget(index, OFFSET_WIDGET_STYLE, "<I") in (0x22, 0x822):
            return start.widget(index, OFFSET_WIDGET_AREA, "<h")
    return None


def test_start_build(ctx):
    for case in range(12):
        start = Screens(ctx)
        ctx.sync_framebuffer(case)
        start.set_player(ctx.rng.choice([0, 0, 0, 1]))
        _prefix(ctx, start)
        start.push_start()
        if ctx.rng.random() < 0.2:
            start.call_stack(FUN_SET_SLOT, 0, 1, 0)
            ctx.port.send("st_slot0", 1)
        title = _background(ctx)
        start.build(title)
        name = f"start build {case}"
        start.expect_same_stack(name)
        start.expect_same_screen(name, _menu_area(start))
        start.expect_same_draw(name, title)
    return case + 1


def test_confirm_build(ctx):
    for case in range(12):
        start = Screens(ctx)
        ctx.sync_framebuffer(case)
        start.push_start()
        text = ctx.rng.choice([b"Exit the game ?", b"Restart the game ?", short_text(ctx)])
        start.push_confirm(b"Championship Manager 2001/02", ctx.rng.choice([b"Please Confirm", short_text(ctx)]),
                           text, ctx.rng.choice([0, 1, 1, 2]), ctx.colour(), ctx.colour(),
                           ctx.rng.choice([0xB, 0x3D, ctx.rng.randint(0, 99)]))
        title = _background(ctx)
        start.build(title)
        name = f"confirm build {case}"
        start.expect_same_stack(name)
        start.expect_same_screen(name)
        start.expect_same_draw(name, title)
        # The answer buttons, "Ok" among them, and the side bar.
        answers = [i for i in range(start.widget_count()) if _s16(start.widget(i, OFFSET_WIDGET_ID, "<i")) in (1, 2, 3)]
        clicked = ctx.rng.choice(answers) if answers and ctx.rng.random() < 0.7 else _click(ctx, start)
        start.input(f"{name}: click {clicked}", clicked)
        start.expect_same_dialog(name)
    return case + 1


def _click(ctx, start, skip_ids=()):
    """A widget of the screen, or -1. Widgets with an id in skip_ids are left out."""
    choices = [index for index in range(start.widget_count())
               if _s16(start.widget(index, OFFSET_WIDGET_ID, "<i")) not in skip_ids]
    if not choices or ctx.rng.random() < 0.15:
        return -1
    return ctx.rng.choice(choices)


def test_input(ctx):
    steps = 0
    for case in range(80):
        start = Screens(ctx)
        name = f"input {case}"
        _prefix(ctx, start)
        start.push_start()
        start.build(_background(ctx))
        start.expect_same_stack(f"{name}: start")
        # The original acts on the menu items 1 to 9 with screens that the port does not have.
        clicked = _click(ctx, start, skip_ids=range(1, 10))
        roll = ctx.rng.random()
        if roll < 0.7:
            # Mostly the side bar buttons that open the confirm screen, and the buttons with id -2.
            wanted = (0x402, 0x42F) if roll < 0.5 else (-2,)
            buttons = [i for i in range(start.widget_count())
                       if _s16(start.widget(i, OFFSET_WIDGET_ID, "<i")) in wanted]
            clicked = ctx.rng.choice(buttons) if buttons else clicked
        steps += 1
        result = start.input(f"{name}: click {clicked}", clicked)
        start.expect_same_dialog(name)
        start.expect_same_stack(f"{name}: after the click")
        if result == -2:
            # Back to the start screen before, which now has one after it, then forward with ">>>".
            start.back()
            start.build(_background(ctx))
            forward = [i for i in range(start.widget_count()) if _s16(start.widget(i, OFFSET_WIDGET_ID, "<i")) == -3]
            steps += 1
            start.input(f"{name}: forward", ctx.rng.choice(forward))
            continue
        if start.current_build() != FUN_CONFIRM_BUILD:
            continue
        start.build(_background(ctx))
        start.expect_same_screen(f"{name}: confirm")
        clicked = _click(ctx, start)
        answers = [i for i in range(start.widget_count()) if _s16(start.widget(i, OFFSET_WIDGET_ID, "<i")) in (1, 2, 3)]
        if answers and ctx.rng.random() < 0.6:
            clicked = ctx.rng.choice(answers)
        steps += 1
        result = start.input(f"{name}: confirm click {clicked}", clicked)
        start.expect_same_dialog(f"{name}: confirm")
        start.expect_same_stack(f"{name}: after the confirm click")
        if result != SCREEN_POP:
            continue
        start.pop()
        start.set_after_pop(1)
        steps += 1
        start.input(f"{name}: after the pop", -1)
        start.set_after_pop(0)
        start.expect_same_dialog(f"{name}: after the pop")
        if not ctx.original.events:
            start.expect_same_stack(f"{name}: after the pop")
        elif "restart" in ctx.original.events:
            # The port starts again with the start screen alone, to be built.
            ctx.port.send("st_dump")
            ctx.expect_equal(f"{name}: record after the restart", ["0", "1", "1", "0", "0", "0", "0", "0", "1"],
                             [w.decode() for w in ctx.port.reply()], source="wanted")
            ctx.expect_equal(f"{name}: screen after the restart", ["1", "1", "0", "-5c"],
                             [w.decode() for w in ctx.port.reply()][:4], source="wanted")
    return steps


TESTS = [
    ("start_pics", test_pics),
    ("start_build", test_start_build),
    ("start_confirm_build", test_confirm_build),
    ("start_input", test_input),
]
