"""Decode The Sims 4's images (from s4package.py's packages) to RGBA pictures.

Formats, found by their first bytes whatever the resource type:
  PNG    (type 0x2F7D0004, some thumbnails): a plain PNG file. The tuning names its icons with
         0x2F7D0004 keys, but the game's files hold them as _IMG (below) with the same instance;
         the PNGs met in 1.128 are the magalog's pictures
  JFIF   thumbnails (0x3C1AF1F2, 0x5B282D45, ...): a JPEG, its alpha a greyscale PNG kept in an
         "ALFA" segment near the start (uint32 big-endian length after the tag, then the PNG)
  DDS    (0x00B2D882 "_IMG"): a 128-byte DDS header ("DDS ", height at 12, width at 16, mip
         count at 28, the pixel format's fourCC at 84, bit count and masks at 88..107), then the
         mips, largest first. FourCCs: DXT1, DXT3, DXT5; DST1 and DST5, The Sims 4's shuffled
         DXT1/DXT5 (s4pi DSTResource.cs): the same blocks with each block's parts gathered into
         separate runs over all mips together:
           DST1  every block's two colours (4 bytes), then every block's indices (4)
           DST5  every block's alpha pair (2 bytes), then colour pair (4), then alpha indices
                 (6), then colour indices (4)
         ATI1/ATI2 (BC4/BC5) as grey / red-green, and uncompressed 32-, 24- and 8-bit pixels by
         their masks. The UI's atlases are two ATI2 planes (decode_ui_atlas, see s4gfx.py)
  RLE    (0x3453CF95 RLE2, 0xBA856C78 RLES; s4pi RLEResource.cs): "DXT5" "RLE2"|"RLES",
         uint16 width, height, mip count, 0; per mip uint32 command offset, colour-pair offset,
         colour-index offset, alpha-pair offset, alpha-index offset (RLES: and a sixth run of 16
         bytes per op-1 block, a second DXT5 block not read here); then the data. Commands are
         uint16, count = c >> 2, op = c & 3:
           0  count transparent blocks, 1  count blocks from the four runs (2 + 6 + 4 + 4 bytes),
           2  count opaque blocks: colour from the runs, alpha 255 (RLE2) or from the runs (RLES)
         a mip's commands end where the next mip's begin (the last at the colour-pair offset of
         mip 0); the blocks are DXT5's. (The run lengths match these counts in the game's files.)
  LRLE   (0x2BC04EDF): not decoded (CAS textures only; no UI image uses it)

DXT decoding uses numpy, a whole picture at once.

  python s4image.py <image file> <out.png>
  python s4image.py <package> <key TTTTTTTT:GGGGGGGG:IIIIIIIIIIIIIIII> <out.png>

The output is the user's game data: keep it outside the repository (refused inside it).
"""
import argparse
import io
import os
import struct
import sys

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
import numpy as np
from PIL import Image

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))  # mashups/sims/tools -> the repo
BLOCK_BYTES = {"DXT1": 8, "DST1": 8, "DXT3": 16, "DXT5": 16, "DST5": 16, "ATI1": 8, "BC4U": 8, "ATI2": 16,
               "BC5U": 16}


def kind(data):
    """What an image resource holds: 'png', 'jpg', 'dds', 'rle', 'lrle' or None."""
    if data[:8] == b"\x89PNG\r\n\x1a\n":
        return "png"
    if data[:2] == b"\xff\xd8":
        return "jpg"
    if data[:4] == b"DDS ":
        return "dds"
    if data[:4] == b"DXT5" and data[4:8] in (b"RLE2", b"RLES"):
        return "rle"
    if data[:4] == b"LRLE":
        return "lrle"
    return None


def size(data):
    """(width, height) from an image resource's header without decoding it, or None."""
    k = kind(data)
    if k == "png" and len(data) >= 24:
        return struct.unpack_from(">II", data, 16)
    if k == "dds" and len(data) >= 20:
        h, w = struct.unpack_from("<II", data, 12)
        return w, h
    if k == "rle" and len(data) >= 12:
        return struct.unpack_from("<HH", data, 8)
    if k == "lrle" and len(data) >= 12:
        return struct.unpack_from("<HH", data, 8)
    if k == "jpg":
        try:
            return Image.open(io.BytesIO(data)).size
        except OSError:
            return None
    return None


def decode(data, mip=0):
    """An image resource as a PIL RGBA image (mip `mip` of a DDS or RLE)."""
    k = kind(data)
    if k == "png":
        return Image.open(io.BytesIO(data)).convert("RGBA")
    if k == "jpg":
        return decode_jfif(data)
    if k == "dds":
        return decode_dds(data, mip)
    if k == "rle":
        return decode_rle(data, mip)
    if k == "lrle":
        raise NotImplementedError("LRLE images are not decoded")
    raise ValueError(f"not an image The Sims 4 uses (starts {data[:8]!r})")


def decode_jfif(data):
    img = Image.open(io.BytesIO(data)).convert("RGBA")
    at = data.find(b"ALFA", 0, 256)
    if at >= 0:
        n = struct.unpack_from(">I", data, at + 4)[0]
        try:
            alpha = Image.open(io.BytesIO(data[at + 8:at + 8 + n])).convert("L")
            if alpha.size == img.size:
                img.putalpha(alpha)
        except OSError:
            pass
    return img


# --- DDS and DST ------------------------------------------------------------------------------

def dds_header(data):
    """(width, height, mips, fourcc or None, bit count, masks (r, g, b, a), pixel format flags)."""
    if data[:4] != b"DDS " or len(data) < 128:
        raise ValueError("not a DDS file")
    h, w = struct.unpack_from("<II", data, 12)
    mips = max(1, struct.unpack_from("<I", data, 28)[0])
    pf_flags = struct.unpack_from("<I", data, 80)[0]
    fourcc = data[84:88].decode("latin-1") if pf_flags & 0x4 else None
    bits = struct.unpack_from("<I", data, 88)[0]
    masks = struct.unpack_from("<4I", data, 92)
    return w, h, mips, fourcc, bits, masks, pf_flags


def unshuffle_dst(fourcc, body):
    """DST1/DST5 data (every mip) as plain DXT1/DXT5 blocks."""
    if fourcc == "DST1":
        count = len(body) // 8
        a = np.frombuffer(body, np.uint8, count * 8)
        return np.concatenate([a[:count * 4].reshape(count, 4), a[count * 4:].reshape(count, 4)], 1).tobytes()
    if fourcc == "DST5":
        count = len(body) // 16
        a = np.frombuffer(body, np.uint8, count * 16)
        o1, o2, o3 = count * 2, count * 6, count * 12
        alpha_pair = a[:o1].reshape(count, 2)
        colour_pair = a[o1:o2].reshape(count, 4)
        alpha_index = a[o2:o3].reshape(count, 6)
        colour_index = a[o3:].reshape(count, 4)
        return np.concatenate([alpha_pair, alpha_index, colour_pair, colour_index], 1).tobytes()
    raise ValueError(f"{fourcc}: not a DST format")


def decode_dds(data, mip=0):
    w, h, mips, fourcc, bits, masks, pf_flags = dds_header(data)
    body = data[128:]
    if fourcc == "DX10":
        raise NotImplementedError("DDS with a DX10 header")
    if fourcc in ("DST1", "DST5"):
        body = unshuffle_dst(fourcc, body)
        fourcc = "DXT" + fourcc[3]
    if fourcc in BLOCK_BYTES:
        bpb = BLOCK_BYTES[fourcc]
        offset = 0
        for _ in range(min(mip, mips - 1)):
            offset += ((w + 3) // 4) * ((h + 3) // 4) * bpb
            w, h = max(1, w // 2), max(1, h // 2)
        n = ((w + 3) // 4) * ((h + 3) // 4) * bpb
        if offset + n > len(body):
            raise ValueError(f"DDS {fourcc} {w}x{h}: {len(body) - offset} bytes of {n}")
        return Image.fromarray(decode_dxt(fourcc, w, h, body[offset:offset + n]), "RGBA")
    if fourcc:
        raise NotImplementedError(f"DDS {fourcc}")
    bpp = bits // 8
    if bpp not in (1, 2, 3, 4):
        raise NotImplementedError(f"DDS with {bits} bits a pixel")
    offset = 0
    for _ in range(min(mip, mips - 1)):
        offset += w * h * bpp
        w, h = max(1, w // 2), max(1, h // 2)
    raw = np.frombuffer(body, np.uint8, w * h * bpp, offset).reshape(h * w, bpp)
    value = np.zeros(h * w, np.uint32)
    for i in range(bpp):
        value |= raw[:, i].astype(np.uint32) << (8 * i)
    out = np.zeros((h * w, 4), np.uint8)
    for c, mask in enumerate(masks):
        if not mask:
            out[:, c] = 255 if c == 3 else 0
            continue
        shift = (mask & -mask).bit_length() - 1
        top = mask >> shift
        out[:, c] = ((value & mask) >> shift) * 255 // top
    if not masks[0] and not masks[1] and not masks[2] and masks[3]:  # alpha only
        out[:, :3] = 255
    elif pf_flags & 0x20000 and masks[0] and not masks[1]:  # luminance
        out[:, 1] = out[:, 2] = out[:, 0]
    return Image.fromarray(out.reshape(h, w, 4), "RGBA")


def _bc4(blocks):
    """BC4 (DXT5 alpha) blocks (n, 8) as their 16 values each (n, 16), pixel i = 4 * row + column."""
    n = len(blocks)
    a0 = blocks[:, 0].astype(np.int32)
    a1 = blocks[:, 1].astype(np.int32)
    pal = np.empty((n, 8), np.int32)
    pal[:, 0], pal[:, 1] = a0, a1
    eight = a0 > a1
    for i in range(1, 7):
        pal[:, 1 + i] = np.where(eight, ((7 - i) * a0 + i * a1) // 7, 0)
    for i in range(1, 5):
        pal[:, 1 + i] = np.where(eight, pal[:, 1 + i], ((5 - i) * a0 + i * a1) // 5)
    pal[:, 6] = np.where(eight, pal[:, 6], 0)
    pal[:, 7] = np.where(eight, pal[:, 7], 255)
    bits = np.zeros(n, np.uint64)
    for i in range(6):
        bits |= blocks[:, 2 + i].astype(np.uint64) << np.uint64(8 * i)
    sel = (bits[:, None] >> (np.arange(16, dtype=np.uint64) * 3)) & 7
    return pal[np.arange(n)[:, None], sel.astype(np.intp)].astype(np.uint8)


def _unblock(px, w, h):
    """Per-block pixels (n, 16, ...) back to an (h, w, ...) picture."""
    bw, bh = (w + 3) // 4, (h + 3) // 4
    rest = px.shape[2:]
    img = px.reshape((bh, bw, 4, 4) + rest).swapaxes(1, 2).reshape((bh * 4, bw * 4) + rest)
    return np.ascontiguousarray(img[:h, :w])


def decode_bc5(w, h, data):
    """ATI2 / BC5 blocks as two (h, w) uint8 channels."""
    n = ((w + 3) // 4) * ((h + 3) // 4)
    blocks = np.frombuffer(data, np.uint8, n * 16).reshape(n, 16)
    return _unblock(_bc4(blocks[:, :8]), w, h), _unblock(_bc4(blocks[:, 8:]), w, h)


def dds_bc5(data):
    """The two channels of an ATI2 DDS's first mip."""
    w, h, _mips, fourcc = dds_header(data)[:4]
    if fourcc not in ("ATI2", "BC5U"):
        raise ValueError(f"DDS {fourcc}, not ATI2")
    return decode_bc5(w, h, data[128:])


def decode_ui_atlas(full, half):
    """A UI atlas from its two ATI2 planes (UI.package, s4gfx.py): `full` (group 0x00064DCA) holds
    alpha and luma, `half` (0x00064DC9, half size) the chroma Co and Cg around 128."""
    a, y = dds_bc5(full)
    co, cg = dds_bc5(half)
    h, w = y.shape
    co = np.asarray(Image.fromarray(co).resize((w, h), Image.BILINEAR), np.float32) - 128
    cg = np.asarray(Image.fromarray(cg).resize((w, h), Image.BILINEAR), np.float32) - 128
    y = y.astype(np.float32)
    rgb = np.stack([y + co - cg, y + cg, y - co - cg], -1)
    out = np.empty((h, w, 4), np.uint8)
    out[..., :3] = np.clip(rgb + 0.5, 0, 255).astype(np.uint8)
    out[..., 3] = a
    return Image.fromarray(out, "RGBA")


def _colours(c0, c1, four):
    """The 4-colour palettes of DXT colour blocks: (n, 4, 4) uint8 (RGBA)."""
    def expand(c):
        r = (c >> 11) & 31
        g = (c >> 5) & 63
        b = c & 31
        return np.stack([(r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)], -1).astype(np.int32)
    p0, p1 = expand(c0.astype(np.int32)), expand(c1.astype(np.int32))
    pal = np.empty((len(c0), 4, 4), np.int32)
    pal[:, 0, :3], pal[:, 1, :3] = p0, p1
    pal[:, :, 3] = 255
    more = four | (c0 > c1)
    pal[:, 2, :3] = np.where(more[:, None], (2 * p0 + p1) // 3, (p0 + p1) // 2)
    pal[:, 3, :3] = np.where(more[:, None], (p0 + 2 * p1) // 3, 0)
    pal[:, 3, 3] = np.where(more, 255, 0)
    return pal.astype(np.uint8)


def decode_dxt(fourcc, w, h, data):
    """DXT1/DXT3/DXT5 (and ATI1/ATI2 as grey and red-green) blocks of a w x h picture as an
    (h, w, 4) uint8 array."""
    n = ((w + 3) // 4) * ((h + 3) // 4)
    if fourcc in ("ATI1", "BC4U", "ATI2", "BC5U"):
        bpb = BLOCK_BYTES[fourcc]
        blocks = np.frombuffer(data, np.uint8, n * bpb).reshape(n, bpb)
        out = np.zeros((n, 16, 4), np.uint8)
        out[:, :, 0] = _bc4(blocks[:, :8])
        out[:, :, 1] = _bc4(blocks[:, 8:]) if bpb == 16 else out[:, :, 0]
        out[:, :, 2] = 0 if bpb == 16 else out[:, :, 0]
        out[:, :, 3] = 255
        return _unblock(out, w, h)
    bpb = 8 if fourcc in ("DXT1", "DST1") else 16
    blocks = np.frombuffer(data, np.uint8, n * bpb).reshape(n, bpb)
    colour = blocks[:, bpb - 8:]
    c0 = colour[:, 0].astype(np.uint16) | colour[:, 1].astype(np.uint16) << 8
    c1 = colour[:, 2].astype(np.uint16) | colour[:, 3].astype(np.uint16) << 8
    pal = _colours(c0, c1, np.full(n, bpb == 16))
    idx = colour[:, 4:8].copy().view("<u4").reshape(n)
    shifts = np.arange(16, dtype=np.uint32) * 2
    sel = (idx[:, None] >> shifts) & 3  # (n, 16), pixel i = 4 * row + column
    px = pal[np.arange(n)[:, None], sel]  # (n, 16, 4)
    if fourcc == "DXT3":
        bits = blocks[:, :8].copy().view("<u8").reshape(n)
        a = (bits[:, None] >> (np.arange(16, dtype=np.uint64) * 4)) & 15
        px[:, :, 3] = (a * 17).astype(np.uint8)
    elif fourcc in ("DXT5", "DST5"):
        px[:, :, 3] = _bc4(blocks[:, :8])
    return _unblock(px, w, h)


# --- RLE2 / RLES ------------------------------------------------------------------------------

def decode_rle(data, mip=0):
    if data[:4] != b"DXT5" or data[4:8] not in (b"RLE2", b"RLES"):
        raise ValueError("not an RLE2/RLES image")
    rles = data[4:8] == b"RLES"
    w, h, mips, _zero = struct.unpack_from("<HHHH", data, 8)
    fields = 6 if rles else 5
    heads = [struct.unpack_from(f"<{fields}I", data, 16 + 4 * fields * i) for i in range(mips)]
    mip = min(mip, mips - 1)
    for _ in range(mip):
        w, h = max(1, w // 2), max(1, h // 2)
    cmd, colour_pair, colour_index, alpha_pair, alpha_index = heads[mip][:5]
    end = heads[mip + 1][0] if mip + 1 < mips else heads[0][1]
    n = ((w + 3) // 4) * ((h + 3) // 4)
    out = np.zeros((n, 16), np.uint8)
    out[:, 0] = out[:, 1] = 0  # transparent: alpha 0 and 0, every index 0
    commands = np.frombuffer(data, "<u2", (end - cmd) // 2, cmd)
    b = 0
    src = np.frombuffer(data, np.uint8)
    for c in commands.tolist():
        op, count = c & 3, c >> 2
        if not count:
            continue
        if b + count > n:
            raise ValueError(f"RLE: {b + count} blocks of {n}")
        if op == 0:
            pass
        elif op in (1, 2):
            if op == 1 or rles:
                out[b:b + count, 0:2] = src[alpha_pair:alpha_pair + 2 * count].reshape(count, 2)
                out[b:b + count, 2:8] = src[alpha_index:alpha_index + 6 * count].reshape(count, 6)
                alpha_pair += 2 * count
                alpha_index += 6 * count
            else:
                out[b:b + count, 0:2] = 255
                out[b:b + count, 2:8] = 0
            out[b:b + count, 8:12] = src[colour_pair:colour_pair + 4 * count].reshape(count, 4)
            out[b:b + count, 12:16] = src[colour_index:colour_index + 4 * count].reshape(count, 4)
            colour_pair += 4 * count
            colour_index += 4 * count
        else:
            raise ValueError(f"RLE: unknown command {c:#06x}")
        b += count
    return Image.fromarray(decode_dxt("DXT5", w, h, out.tobytes()), "RGBA")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("source", help="an image file, or a .package")
    parser.add_argument("rest", nargs="+", help="<out.png>, or for a package <key> <out.png>")
    parser.add_argument("--mip", type=int, default=0)
    args = parser.parse_args(argv)
    if args.source.lower().endswith(".package"):
        if len(args.rest) != 2:
            parser.error("a package needs <key> <out.png>")
        from s4package import Package, parse_key
        data = Package(args.source).read(parse_key(args.rest[0]))
        out = args.rest[1]
    else:
        with open(args.source, "rb") as f:
            data = f.read()
        out = args.rest[0]
    out = os.path.abspath(out)
    if os.path.commonpath([out, REPO]) == REPO:
        sys.exit(f"{out} is inside the repository; game data must stay outside it")
    img = decode(data, args.mip)
    img.save(out)
    print(f"{kind(data)} {img.size[0]}x{img.size[1]} -> {out}")


if __name__ == "__main__":
    main()
