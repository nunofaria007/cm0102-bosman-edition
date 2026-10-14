"""Makes the app icons from the player's own cm0102.exe, so that no icon goes into git.

Usage:
    python tools/icon/make_icons.py <cm0102.exe> <output folder>

Writes to the output folder:
    cm0102.ico    the original's icon group 102 (the ball, 32x32 in 16 and 2 colours), unchanged
    cm0102.png    the 16-colour ball at 32x32 with transparency
    cm0102.icns   the Mac icon: the ball scaled up by whole pixels, so its edges stay hard (macOS only)

Only the Python standard library is used, plus iconutil on macOS for the .icns.
"""
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib

RT_ICON, RT_GROUP_ICON = 3, 14
BALL_GROUP = 102


def pe_resources(exe):
    """Returns {(type, name): bytes} for the first language of each resource."""
    data = open(exe, 'rb').read()
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    if data[pe:pe + 4] != b'PE\0\0':
        raise SystemExit(f'{exe} is not a PE file')
    n_sections, opt_size = struct.unpack_from('<H', data, pe + 6)[0], struct.unpack_from('<H', data, pe + 20)[0]
    opt = pe + 24
    rsrc_rva = struct.unpack_from('<I', data, opt + 96 + 2 * 8)[0]
    sections = []
    for i in range(n_sections):
        s = opt + opt_size + 40 * i
        vsize, va, rsize, raw = struct.unpack_from('<IIII', data, s + 8)
        sections.append((va, max(vsize, rsize), raw))

    def at(rva):
        for va, size, raw in sections:
            if va <= rva < va + size:
                return rva - va + raw
        raise SystemExit(f'RVA {rva:#x} is outside every section')

    base = at(rsrc_rva)

    def entries(off):
        named, ids = struct.unpack_from('<HH', data, base + off + 12)
        for i in range(named + ids):
            name, target = struct.unpack_from('<II', data, base + off + 16 + 8 * i)
            yield name, target

    out = {}
    for rtype, t in entries(0):
        for name, n in entries(t & 0x7fffffff):
            lang = next(entries(n & 0x7fffffff))[1]
            rva, size = struct.unpack_from('<II', data, base + lang)
            out[(rtype, name)] = data[at(rva):at(rva) + size]
    return out


def group(res, gid):
    g = res[(RT_GROUP_ICON, gid)]
    count = struct.unpack_from('<H', g, 4)[0]
    items = []
    for i in range(count):
        w, h, colours, _, planes, bits, size, iid = struct.unpack_from('<BBBBHHIH', g, 6 + 14 * i)
        items.append(dict(w=w or 256, h=h or 256, colours=colours, planes=planes, bits=bits, image=res[(RT_ICON, iid)]))
    return items


def write_ico(items, path):
    head = struct.pack('<HHH', 0, 1, len(items))
    offset = 6 + 16 * len(items)
    dirs, body = b'', b''
    for it in items:
        dirs += struct.pack('<BBBBHHII', it['w'] % 256, it['h'] % 256, it['colours'], 0, it['planes'], it['bits'],
                            len(it['image']), offset + len(body))
        body += it['image']
    open(path, 'wb').write(head + dirs + body)


def decode(it):
    """Decodes a paletted icon image into rows of RGBA tuples."""
    img = it['image']
    hsize, w, h2, _, bits = struct.unpack_from('<IiiHH', img, 0)
    h = h2 // 2
    colours = 1 << bits
    pal = [struct.unpack_from('<BBBB', img, hsize + 4 * i) for i in range(colours)]
    xor = hsize + 4 * colours
    stride = ((w * bits + 31) // 32) * 4
    mstride = ((w + 31) // 32) * 4
    mask = xor + stride * h
    rows = []
    for y in range(h):
        src = h - 1 - y
        row = []
        for x in range(w):
            bit = x * bits
            byte = img[xor + src * stride + bit // 8]
            idx = (byte >> (8 - bits - bit % 8)) & (colours - 1)
            b, g, r, _ = pal[idx]
            clear = (img[mask + src * mstride + x // 8] >> (7 - x % 8)) & 1
            row.append((r, g, b, 0 if clear else 255))
        rows.append(row)
    return rows


def scale(rows, size):
    """Scales by nearest pixel, which keeps the hard edges of the 2001 icon."""
    h, w = len(rows), len(rows[0])
    return [[rows[y * h // size][x * w // size] for x in range(size)] for y in range(size)]


def write_png(rows, path):
    h, w = len(rows), len(rows[0])
    raw = b''.join(b'\0' + bytes(c for px in row for c in px) for row in rows)

    def chunk(kind, body):
        return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body) & 0xffffffff)

    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0))
                           + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    exe, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    items = group(pe_resources(exe), BALL_GROUP)
    write_ico(items, os.path.join(out, 'cm0102.ico'))
    ball = decode(max(items, key=lambda it: it['bits']))
    write_png(ball, os.path.join(out, 'cm0102.png'))
    if sys.platform == 'darwin' and shutil.which('iconutil'):
        with tempfile.TemporaryDirectory() as tmp:
            iconset = os.path.join(tmp, 'cm0102.iconset')
            os.mkdir(iconset)
            for pt in (16, 32, 128, 256, 512):
                write_png(scale(ball, pt), os.path.join(iconset, f'icon_{pt}x{pt}.png'))
                write_png(scale(ball, pt * 2), os.path.join(iconset, f'icon_{pt}x{pt}@2x.png'))
            subprocess.run(['iconutil', '-c', 'icns', iconset, '-o', os.path.join(out, 'cm0102.icns')], check=True)
    print(f'icons written to {out}')


if __name__ == '__main__':
    main()
