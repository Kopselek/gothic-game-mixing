"""Read The Witcher 3's texture.cache and decode its block-compressed textures to RGBA.

A .xbm resource in the bundles only describes a texture; its pixels live in
content\\content0\\texture.cache under the same depot path.

texture.cache layout (dingdio's Witcher3-Blender-Tools TextureCache.py; 4.04 writes version 7):
  footer, the last 32 bytes: uint64 crc, uint32 used pages, entry count, string table size,
          mip table entry count, magic "HCXT", version
  before it: entries (52 bytes each), before them the NUL-separated names, before them the mip table
  entry: uint32 hash, int32 name offset, uint32 page (x 4096), packed size, size, alignment,
         int16 width, height, mip count, slices, int32 mip table index, int32 separate mips
         (low 16 bits), int64 timestamp, uint8 format, uint8, uint8 cube, uint8
  data at page * 4096: blocks of {uint32 packed size, uint32 size, uint8 mip, zlib data}: the
         first holds mip 0, the next ones the following mips (in 4.04 the last block holds all
         the smallest); together they are every mip in order
  formats: 0x07 BC1, 0x0D BC2, 0x08 BC3, 0x0E BC4, 0x0F BC5, 0x0A BC7, 0x00/0xFD RGBA8

BC1, BC2 and BC3 decode here (pure Python, a 1024x1024 BC1 mip takes about a second).

  python w3texture.py "<Witcher 3 folder>" <depot path .xbm> <out.png> [--mip N]
"""
import argparse
import os
import struct
import sys
import zlib

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))

MAGIC = 0x54584348  # "HCXT"
ENTRY = struct.Struct("<IiIIIIhhhhiiqBBBB")
FORMATS = {0x07: "BC1", 0x0D: "BC2", 0x08: "BC3", 0x0E: "BC4", 0x0F: "BC5", 0x0A: "BC7", 0x00: "RGBA8",
           0xFD: "RGBA8"}
BLOCK_BYTES = {"BC1": 8, "BC4": 8, "BC2": 16, "BC3": 16, "BC5": 16, "BC7": 16}


class CachedTexture:
    __slots__ = ("name", "page", "packed", "size", "width", "height", "mips", "blocks", "format")

    def __init__(self, name, row):
        self.name = name
        (_hash, _noff, self.page, self.packed, self.size, _align, self.width, self.height, self.mips,
         _slices, _mip_index, separate, _time, fmt, _t2, _cube, _u) = row
        self.blocks = 1 + (separate & 0xFFFF)
        self.format = FORMATS.get(fmt, hex(fmt))


class TextureCache:
    def __init__(self, path):
        self.path = path
        with open(path, "rb") as f:
            f.seek(-32, os.SEEK_END)
            _crc, _pages, count, strings, _mips, magic, self.version = struct.unpack("<QIIIIII", f.read(32))
            if magic != MAGIC:
                raise ValueError(f"{path}: not a texture.cache")
            f.seek(-(32 + count * ENTRY.size + strings), os.SEEK_END)
            names = f.read(strings).split(b"\0")[:count]
            rows = f.read(count * ENTRY.size)
        self.textures = {}
        for i, name in enumerate(names):
            t = CachedTexture(name.decode("latin-1"), ENTRY.unpack_from(rows, i * ENTRY.size))
            self.textures[t.name.lower().replace("/", "\\")] = t

    def get(self, name):
        return self.textures.get(name.lower().replace("/", "\\"))

    def mip(self, name, level=0):
        """(width, height, format, bytes) of mip `level` of a cached texture."""
        t = self.get(name)
        if t is None:
            raise KeyError(f"{name}: not in {self.path}")
        w, h, offset = t.width, t.height, 0
        for _ in range(level):
            offset += mip_size(t.format, w, h)
            w, h = max(1, w // 2), max(1, h // 2)
        n = mip_size(t.format, w, h)
        data = bytearray()
        with open(self.path, "rb") as f:
            f.seek(t.page * 4096)
            for _ in range(t.blocks):
                packed, size, _ = struct.unpack("<IIB", f.read(9))
                if len(data) + size <= offset:  # a block before the wanted mip: skip it unpacked
                    f.seek(packed, os.SEEK_CUR)
                    data += bytes(size)
                    continue
                data += zlib.decompress(f.read(packed))
                if len(data) >= offset + n:
                    break
        if offset + n > len(data):
            raise ValueError(f"{name}: mip {level} lies beyond the texture's {len(data)} bytes")
        return w, h, t.format, bytes(data[offset:offset + n])


def mip_size(fmt, w, h):
    if fmt == "RGBA8":
        return w * h * 4
    return max(1, (w + 3) // 4) * max(1, (h + 3) // 4) * BLOCK_BYTES[fmt]


def _colors565(c0, c1, four):
    r0, g0, b0 = (c0 >> 11) * 255 // 31, ((c0 >> 5) & 63) * 255 // 63, (c0 & 31) * 255 // 31
    r1, g1, b1 = (c1 >> 11) * 255 // 31, ((c1 >> 5) & 63) * 255 // 63, (c1 & 31) * 255 // 31
    if four:
        return [(r0, g0, b0, 255), (r1, g1, b1, 255),
                ((2 * r0 + r1) // 3, (2 * g0 + g1) // 3, (2 * b0 + b1) // 3, 255),
                ((r0 + 2 * r1) // 3, (g0 + 2 * g1) // 3, (b0 + 2 * b1) // 3, 255)]
    return [(r0, g0, b0, 255), (r1, g1, b1, 255), ((r0 + r1) // 2, (g0 + g1) // 2, (b0 + b1) // 2, 255),
            (0, 0, 0, 0)]


def _alpha_bc3(block):
    a0, a1 = block[0], block[1]
    if a0 > a1:
        table = [a0, a1] + [((7 - i) * a0 + i * a1) // 7 for i in range(1, 7)]
    else:
        table = [a0, a1] + [((5 - i) * a0 + i * a1) // 5 for i in range(1, 5)] + [0, 255]
    bits = int.from_bytes(block[2:8], "little")
    return [table[(bits >> (3 * i)) & 7] for i in range(16)]


def decode(fmt, w, h, data):
    """RGBA bytes, rows top first, of a BC1/BC2/BC3 or RGBA8 mip."""
    if fmt == "RGBA8":
        return bytes(data)
    if fmt not in ("BC1", "BC2", "BC3"):
        raise NotImplementedError(f"{fmt} textures are not decoded yet")
    out = bytearray(w * h * 4)
    bw, bh = (w + 3) // 4, (h + 3) // 4
    step = BLOCK_BYTES[fmt]
    for by in range(bh):
        for bx in range(bw):
            o = (by * bw + bx) * step
            alpha = None
            if fmt == "BC2":
                bits = int.from_bytes(data[o:o + 8], "little")
                alpha = [((bits >> (4 * i)) & 15) * 17 for i in range(16)]
                o += 8
            elif fmt == "BC3":
                alpha = _alpha_bc3(data[o:o + 8])
                o += 8
            c0, c1, idx = struct.unpack_from("<HHI", data, o)
            colors = _colors565(c0, c1, c0 > c1 or fmt != "BC1")
            for py in range(4):
                y = by * 4 + py
                if y >= h:
                    break
                row = (y * w + bx * 4) * 4
                for px in range(4):
                    if bx * 4 + px >= w:
                        break
                    i = py * 4 + px
                    r, g, b, a = colors[(idx >> (2 * i)) & 3]
                    if alpha is not None:
                        a = alpha[i]
                    out[row + px * 4:row + px * 4 + 4] = bytes((r, g, b, a))
    return bytes(out)


def load_rgba(cache, name, max_size=1024):
    """(width, height, rgba) of a texture at the first mip no larger than max_size."""
    t = cache.get(name)
    if t is None:
        raise KeyError(f"{name}: not in the texture cache")
    level, w, h = 0, t.width, t.height
    while max(w, h) > max_size and level < t.mips - 1:
        level, w, h = level + 1, max(1, w // 2), max(1, h // 2)
    w, h, fmt, data = cache.mip(name, level)
    return w, h, decode(fmt, w, h, data)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("w3")
    parser.add_argument("name")
    parser.add_argument("out")
    parser.add_argument("--max-size", type=int, default=1024)
    args = parser.parse_args(argv)
    from png import write_png
    cache = TextureCache(os.path.join(args.w3, "content", "content0", "texture.cache"))
    w, h, rgba = load_rgba(cache, args.name, args.max_size)
    write_png(args.out, w, h, rgba)
    print(f"{args.name}: {w}x{h} -> {args.out}")


if __name__ == "__main__":
    sys.exit(main())
