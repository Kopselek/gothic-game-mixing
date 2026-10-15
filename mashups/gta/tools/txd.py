"""Read RenderWare texture dictionaries (.txd) from the PC versions of GTA San Andreas.

Textures decode to (width, height, rgba_bytes), top row first. Supports the D3D8/D3D9
native rasters SA ships: DXT1/3/5, 8888, 888, 1555, 565, 4444, LUM8 and PAL8/PAL4.
"""
import struct

from dff import STRUCT, chunks

TEXTURE_NATIVE = 0x15
HEADER = struct.Struct("<IBBH32s32sIIHHBBBB")  # 88 bytes
FMT_MASK, PAL8, PAL4 = 0x0F00, 0x2000, 0x4000
FMT_1555, FMT_565, FMT_4444, FMT_LUM8, FMT_8888, FMT_888, FMT_555 = 0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0xA00
FOURCC = {0x31545844: "DXT1", 0x33545844: "DXT3", 0x35545844: "DXT5"}


class Texture:
    def __init__(self, name, width, height, rgba, compression=None, levels=None):
        self.name, self.width, self.height, self.rgba = name, width, height, rgba
        self.compression = compression  # "DXT1"/"DXT3"/"DXT5" when stored compressed
        self.levels = levels or []       # raw mip levels (largest first) for compressed textures


def read_txd(data):
    textures = {}
    for kind, s, e in chunks(data, 12, None) if struct.unpack_from("<I", data)[0] == 0x16 else ():
        if kind != TEXTURE_NATIVE:
            continue
        st = next((cs, ce) for k, cs, ce in chunks(data, s, e) if k == STRUCT)
        tex = _read_native(data, st[0])
        textures[tex.name.lower()] = tex
    return textures


def _read_native(data, pos):
    (platform, _, _, _, name, _, raster, d3d_format, width, height,
     depth, num_levels, _, flags) = HEADER.unpack_from(data, pos)
    name = name.split(b"\0")[0].decode("latin-1")
    pos += HEADER.size
    if platform == 9:
        compression = FOURCC.get(d3d_format)
    elif platform == 8:
        compression = {1: "DXT1", 3: "DXT3", 5: "DXT5"}.get(flags)
    else:
        raise ValueError(f"texture {name}: unsupported platform {platform}")

    palette = None
    if raster & (PAL8 | PAL4):
        count = 256 if raster & PAL8 else (32 if platform == 8 else 16)
        palette = data[pos:pos + count * 4]  # RGBA
        pos += count * 4

    levels = []
    for _ in range(num_levels):
        size = struct.unpack_from("<I", data, pos)[0]
        levels.append(data[pos + 4:pos + 4 + size])
        pos += 4 + size

    top = levels[0]
    if compression:
        rgba = decode_dxt(top, width, height, compression)
    elif palette is not None:
        idx = top if raster & PAL8 else bytes(n for b in top for n in (b & 15, b >> 4))
        rgba = b"".join(palette[i * 4:i * 4 + 4] for i in idx[:width * height])
    else:
        rgba = _decode_plain(top, width, height, raster & FMT_MASK)
    return Texture(name, width, height, rgba, compression, levels if compression else None)


def _decode_plain(src, w, h, fmt):
    out = bytearray(w * h * 4)
    if fmt in (FMT_8888, FMT_888):  # stored B, G, R, A
        for i in range(w * h):
            b, g, r, a = src[i * 4:i * 4 + 4]
            out[i * 4:i * 4 + 4] = bytes((r, g, b, a if fmt == FMT_8888 else 255))
        return bytes(out)
    if fmt == FMT_LUM8:
        return b"".join(bytes((v, v, v, 255)) for v in src[:w * h])
    px = struct.unpack_from(f"<{w * h}H", src)
    for i, v in enumerate(px):
        if fmt == FMT_565:
            c = ((v >> 11) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31, 255)
        elif fmt == FMT_4444:
            c = (((v >> 8) & 15) * 17, ((v >> 4) & 15) * 17, (v & 15) * 17, (v >> 12) * 17)
        else:  # 1555 / 555
            c = (((v >> 10) & 31) * 255 // 31, ((v >> 5) & 31) * 255 // 31, (v & 31) * 255 // 31,
                 255 if fmt == FMT_555 or v >> 15 else 0)
        out[i * 4:i * 4 + 4] = bytes(c)
    return bytes(out)


def _rgb565(v):
    return ((v >> 11) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31)


def decode_dxt(src, w, h, kind):
    out = bytearray(w * h * 4)
    block = 8 if kind == "DXT1" else 16
    pos = 0
    for by in range(0, max(h, 4), 4):
        for bx in range(0, max(w, 4), 4):
            alpha = [255] * 16
            if kind == "DXT3":
                bits = int.from_bytes(src[pos:pos + 8], "little")
                alpha = [((bits >> (4 * i)) & 15) * 17 for i in range(16)]
            elif kind == "DXT5":
                a0, a1 = src[pos], src[pos + 1]
                bits = int.from_bytes(src[pos + 2:pos + 8], "little")
                table = [a0, a1] + ([((6 - i) * a0 + (1 + i) * a1) // 7 for i in range(6)] if a0 > a1
                                    else [((4 - i) * a0 + (1 + i) * a1) // 5 for i in range(4)] + [0, 255])
                alpha = [table[(bits >> (3 * i)) & 7] for i in range(16)]
            c = pos + (block - 8)
            c0, c1, bits = struct.unpack_from("<HHI", src, c)
            p0, p1 = _rgb565(c0), _rgb565(c1)
            if c0 > c1 or kind != "DXT1":
                colors = [p0 + (255,), p1 + (255,),
                          tuple((2 * a + b) // 3 for a, b in zip(p0, p1)) + (255,),
                          tuple((a + 2 * b) // 3 for a, b in zip(p0, p1)) + (255,)]
            else:
                colors = [p0 + (255,), p1 + (255,), tuple((a + b) // 2 for a, b in zip(p0, p1)) + (255,), (0, 0, 0, 0)]
            for i in range(16):
                x, y = bx + (i & 3), by + (i >> 2)
                if x < w and y < h:
                    r, g, b, a = colors[(bits >> (2 * i)) & 3]
                    o = (y * w + x) * 4
                    out[o:o + 4] = bytes((r, g, b, min(a, alpha[i])))
            pos += block
    return bytes(out)
