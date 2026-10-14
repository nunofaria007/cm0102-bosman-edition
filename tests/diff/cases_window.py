"""The window procedure of the original against input_poll of the port: keys, the wheel and the mouse."""
import struct

from cases_scroll import (G_CLICK_FRAMES, G_LEFT_CLICKED, G_LEFT_HELD, G_MOUSE_X, G_MOUSE_Y, G_RIGHT_CLICKED,
                          G_RIGHT_HELD, _count_frame, _new_screen, _u32)

FUN_WINDOW_PROC = 0x68D630       # stdcall (hwnd, message, wParam, lParam)
FUN_CURSOR_AGAIN = 0x5E81E0      # sets the cursor shape again on a mouse move
IAT_DEFAULT_PROC = 0x9671F4      # the import that the window procedure calls for other messages
STUB = 0x64000000                # a stub for that import: stdcall, 4 arguments, returns 0

WM_MOUSEMOVE, WM_LBUTTONDOWN, WM_LBUTTONUP = 0x200, 0x201, 0x202
WM_RBUTTONDOWN, WM_RBUTTONUP, WM_MOUSEWHEEL, WM_KEYDOWN = 0x204, 0x205, 0x20A, 0x100
KEYS = list(range(0x21, 0x29))   # VK_PRIOR to VK_DOWN


def _stub_windows(original):
    if getattr(original, "window_stubs", False):
        return
    original.uc.mem_map(STUB, 0x1000)
    original.write(STUB, b"\x31\xc0\xc2\x10\x00")          # xor eax, eax; ret 0x10
    original.write_u32(IAT_DEFAULT_PROC, STUB)
    original._replace(FUN_CURSOR_AGAIN, 0, lambda: 0)
    original.window_stubs = True


def _send(ctx, name, message, w_param, l_param, port_words):
    original = ctx.original
    original.call(FUN_WINDOW_PROC, 0, message, w_param & 0xFFFFFFFF, l_param & 0xFFFFFFFF)
    wanted = [_u32(ctx, address) for address in (G_MOUSE_X, G_MOUSE_Y, G_LEFT_HELD, G_RIGHT_HELD,
                                                  G_LEFT_CLICKED, G_RIGHT_CLICKED, G_CLICK_FRAMES)]
    ctx.port.send("sdl", *port_words)
    ctx.expect_equal(f"{name}: mouse x, y, held, clicked, frames", wanted, [int(w) for w in ctx.port.reply()])


def test_window(ctx):
    steps = 0
    for case in range(30):
        scene, areas, widgets = _new_screen(ctx, case)
        _stub_windows(ctx.original)
        # The poll clears the one-shot click flags. This test has no poll, so each case starts clear.
        ctx.original.write_u32(G_LEFT_CLICKED, 0)
        ctx.original.write_u32(G_RIGHT_CLICKED, 0)
        ctx.port.send("clicks", 0, 0)
        for step in range(ctx.rng.randint(5, 25)):
            name = f"window {case} step {step}"
            steps += 1
            roll = ctx.rng.random()
            x, y = ctx.rng.randint(0, 799), ctx.rng.randint(0, 599)
            if roll < 0.25:
                vk = ctx.rng.choice(KEYS)
                _send(ctx, f"{name}: key {vk:#x}", WM_KEYDOWN, vk, 0, [0, vk])
            elif roll < 0.4:
                notches = ctx.rng.choice([1, -1, 2, -2, 3])
                _send(ctx, f"{name}: wheel {notches}", WM_MOUSEWHEEL, (notches * 120) << 16, 0, [1, notches])
            elif roll < 0.6:
                _send(ctx, f"{name}: move", WM_MOUSEMOVE, 0, (y << 16) | x, [2, x, y])
            elif roll < 0.85:
                down, button = ctx.rng.choice([1, 0]), ctx.rng.choice([1, 1, 3])
                message = {(1, 1): WM_LBUTTONDOWN, (0, 1): WM_LBUTTONUP,
                           (1, 3): WM_RBUTTONDOWN, (0, 3): WM_RBUTTONUP}[down, button]
                # The port also takes the position from a button event (a tap can come with no move
                # before it). A move to the same point first makes the two programs comparable.
                _send(ctx, f"{name}: move before the button", WM_MOUSEMOVE, 0, (y << 16) | x, [2, x, y])
                _send(ctx, f"{name}: button {button} {'down' if down else 'up'}", message, 0, (y << 16) | x,
                      [3, down, button, (y << 16) | x])
            else:
                _count_frame(ctx, name)
    return steps


TESTS = [
    ("window", test_window),
]
