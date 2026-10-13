"""Differential test: compares functions of the port with the original.

Usage:
    python tests/diff/run.py <path to cm0102.exe> <path to port_driver> [test name ...]

Needs the unicorn package. The port driver is the CMake target port_driver.
"""
import random
import subprocess
import sys

import cases_images
import cases_primitives
from original import FB_SIZE, HEIGHT, WIDTH, Original

SUITES = [cases_primitives, cases_images]


class Mismatch(Exception):
    pass


class Port:
    """Talks to the port driver: one text command per line on stdin."""

    def __init__(self, driver_path):
        self.proc = subprocess.Popen([driver_path], stdin=subprocess.PIPE, stdout=subprocess.PIPE)

    def send(self, *words):
        self.proc.stdin.write((" ".join(str(w) for w in words) + "\n").encode())
        self.proc.stdin.flush()

    def reply(self):
        line = self.proc.stdout.readline()
        if not line:
            raise RuntimeError("the port driver stopped")
        return line.split()

    def read(self, size):
        data = self.proc.stdout.read(size)
        if len(data) != size:
            raise RuntimeError("the port driver stopped")
        return data

    def framebuffer(self):
        self.send("dump")
        return self.read(FB_SIZE)

    def close(self):
        self.proc.stdin.close()
        self.proc.wait()


class Context:
    def __init__(self, original, port, seed):
        self.original = original
        self.port = port
        self.rng = random.Random(seed)

    def coord(self, limit):
        """A coordinate that is usually on screen and sometimes at or past an edge."""
        roll = self.rng.random()
        if roll < 0.15:
            return self.rng.choice([-1, 0, limit - 1, limit])
        if roll < 0.25:
            return self.rng.randint(-40, limit + 40)
        return self.rng.randint(0, limit - 1)

    def x(self):
        return self.coord(WIDTH)

    def y(self):
        return self.coord(HEIGHT)

    def origin(self, size, limit):
        """Often puts a shape of this size flush with the far screen edge, or one pixel either side."""
        if self.rng.random() < 0.3:
            return limit - size + self.rng.choice([-1, 0, 1])
        return self.coord(limit)

    def colour(self):
        return self.rng.choice([0x0000, 0xFFFF, self.rng.randint(0, 0xFFFF)])

    def sync_framebuffer(self, seed):
        """Gives both sides the same noise picture."""
        self.port.send("fill", seed)
        self.original.set_framebuffer(self.port.framebuffer())

    def expect_equal(self, case, wanted, port_value, source="original"):
        if wanted != port_value:
            raise Mismatch(f"{case}: {source} {repr(wanted)[:200]}, port {repr(port_value)[:200]}")

    def expect_same_framebuffer(self, case):
        a = self.original.framebuffer()
        b = self.port.framebuffer()
        if a == b:
            return
        wrong = [i // 2 for i in range(0, FB_SIZE, 2) if a[i:i + 2] != b[i:i + 2]]
        first = wrong[0]
        pixel = lambda fb: int.from_bytes(fb[first * 2:first * 2 + 2], "little")
        raise Mismatch(
            f"{case}: {len(wrong)} pixels differ, first at x={first % WIDTH} y={first // WIDTH}: "
            f"original {pixel(a):#06x}, port {pixel(b):#06x}")


def main(argv):
    if len(argv) < 3:
        raise SystemExit(__doc__)
    tests = [entry for suite in SUITES for entry in suite.TESTS]
    wanted = argv[3:] or [name for name, _ in tests]
    unknown = set(wanted) - {name for name, _ in tests}
    if unknown:
        raise SystemExit(f"unknown test: {', '.join(sorted(unknown))}")
    original = Original(argv[1])
    port = Port(argv[2])
    failed = 0
    for name, test in tests:
        if name not in wanted:
            continue
        original.reset_heap()
        try:
            count = test(Context(original, port, seed=name))
            print(f"ok    {name}: {count} cases")
        except Mismatch as exc:
            failed += 1
            print(f"FAIL  {name}: {exc}")
    port.close()
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
