"""Write Gothic compiled textures (-C.TEX, "ZTEX"): DXT1 with a mipmap chain, or DXT3 (with
alpha, single level) as Gothic's own HUD textures use, or one uncompressed level (write_raw_tex).

Header (36 bytes): "ZTEX", version 0, format (10 = DXT1, 12 = DXT3, 6 = ARGB_4444, ...: ZenGin's
zTRnd_TextureFormat), width, height, mip count,
reference width, reference height, average colour (ARGB). Mipmaps follow smallest first,
the same layout as the game's own textures (e.g. HUM_HEAD_V0_C0-C.TEX).
"""
import struct

FORMAT_DXT1 = 10
FORMAT_DXT3 = 12


def downsample(w, h, rgba):
    nw, nh = max(w // 2, 1), max(h // 2, 1)
    out = bytearray(nw * nh * 4)
    for y in range(nh):
        for x in range(nw):
            for c in range(4):
                s = 0
                for dy in (0, 1):
                    for dx in (0, 1):
                        s += rgba[((min(2 * y + dy, h - 1)) * w + min(2 * x + dx, w - 1)) * 4 + c]
                out[(y * nw + x) * 4 + c] = s // 4
    return nw, nh, bytes(out)


def _565(r, g, b):
    return ((r * 31 + 127) // 255) << 11 | ((g * 63 + 127) // 255) << 5 | ((b * 31 + 127) // 255)


def _unpack565(v):
    return ((v >> 11) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31)


def encode_dxt1(w, h, rgba, alpha=False):
    """DXT1 blocks; with alpha=True, DXT3 blocks (explicit 4-bit alpha before each colour block,
    colour endpoints fitted to the visible pixels only)."""
    out = bytearray()
    for by in range(0, h, 4):
        for bx in range(0, w, 4):
            texels = [rgba[((min(by + i // 4, h - 1)) * w + min(bx + i % 4, w - 1)) * 4:][:4] for i in range(16)]
            px = [t[:3] for t in texels]
            fit = px
            if alpha:
                bits = 0
                for i, t in enumerate(texels):
                    bits |= (t[3] * 15 + 127) // 255 << (4 * i)
                out += struct.pack("<Q", bits)
                fit = [t[:3] for t in texels if t[3] > 16] or px
            # endpoints: extremes along the block's dominant colour axis (bounding-box diagonal)
            lo = [min(p[c] for p in fit) for c in range(3)]
            hi = [max(p[c] for p in fit) for c in range(3)]
            axis = [hi[c] - lo[c] for c in range(3)]
            proj = [sum(p[c] * axis[c] for c in range(3)) for p in fit]
            pmax, pmin = fit[proj.index(max(proj))], fit[proj.index(min(proj))]
            c0, c1 = _565(*pmax), _565(*pmin)
            if c0 < c1:
                c0, c1 = c1, c0
            if c0 == c1:
                out += struct.pack("<HHI", c0, c1, 0)
                continue
            e0, e1 = _unpack565(c0), _unpack565(c1)
            palette = [e0, e1, tuple((2 * a + b) // 3 for a, b in zip(e0, e1)), tuple((a + 2 * b) // 3 for a, b in zip(e0, e1))]
            bits = 0
            for i, p in enumerate(px):
                best = min(range(4), key=lambda k: sum((p[c] - palette[k][c]) ** 2 for c in range(3)))
                bits |= best << (2 * i)
            out += struct.pack("<HHI", c0, c1, bits)
    return bytes(out)


def _numpy():
    try:
        import numpy
        return numpy
    except ImportError:
        return None


def _downsample_numpy(np, w, h, rgba):
    a = np.frombuffer(rgba, np.uint8).reshape(h, w, 4).astype(np.uint32)
    nw, nh = max(w // 2, 1), max(h // 2, 1)
    a = a[:nh * 2, :nw * 2] if h > 1 and w > 1 else a
    s = a[0::2, 0::2] + a[1::2, 0::2] + a[0::2, 1::2] + a[1::2, 1::2]
    return nw, nh, (s // 4).astype(np.uint8).tobytes()


def _encode_dxt1_numpy(np, w, h, rgba):
    """encode_dxt1 without alpha, a block row at a time with numpy (same endpoints and palette)."""
    a = np.frombuffer(rgba, np.uint8).reshape(h, w, 4)[:, :, :3].astype(np.int32)
    a = np.pad(a, ((0, -h % 4), (0, -w % 4), (0, 0)), mode="edge")
    bh, bw = a.shape[0] // 4, a.shape[1] // 4
    blocks = a.reshape(bh, 4, bw, 4, 3).transpose(0, 2, 1, 3, 4).reshape(-1, 16, 3)
    out = np.zeros(len(blocks), dtype=[("c0", "<u2"), ("c1", "<u2"), ("bits", "<u4")])
    shifts = (2 * np.arange(16)).astype(np.uint64)
    for s in range(0, len(blocks), 16384):
        b = blocks[s:s + 16384]
        axis = b.max(1) - b.min(1)
        proj = (b * axis[:, None, :]).sum(2)
        rows = np.arange(len(b))
        pmax, pmin = b[rows, proj.argmax(1)], b[rows, proj.argmin(1)]
        to565 = lambda p: ((p[:, 0] * 31 + 127) // 255) << 11 | ((p[:, 1] * 63 + 127) // 255) << 5 | ((p[:, 2] * 31 + 127) // 255)
        c0, c1 = to565(pmax), to565(pmin)
        c0, c1 = np.maximum(c0, c1), np.minimum(c0, c1)
        unpack = lambda v: np.stack([(v >> 11) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31], 1)
        e0, e1 = unpack(c0), unpack(c1)
        palette = np.stack([e0, e1, (2 * e0 + e1) // 3, (e0 + 2 * e1) // 3], 1)
        index = ((b[:, :, None, :] - palette[:, None, :, :]) ** 2).sum(3).argmin(2).astype(np.uint64)
        bits = (index << shifts).sum(1)
        bits[c0 == c1] = 0
        out["c0"][s:s + len(b)], out["c1"][s:s + len(b)], out["bits"][s:s + len(b)] = c0, c1, bits
    return out.tobytes()


def _encode_dxt3_numpy(np, w, h, rgba):
    """DXT3 blocks: explicit 4-bit alpha, then the colour block as _encode_dxt1_numpy fits it."""
    a = np.frombuffer(rgba, np.uint8).reshape(h, w, 4)[:, :, 3].astype(np.uint64)
    a = np.pad(a, ((0, -h % 4), (0, -w % 4)), mode="edge")
    bh, bw = a.shape[0] // 4, a.shape[1] // 4
    a4 = (a.reshape(bh, 4, bw, 4).transpose(0, 2, 1, 3).reshape(-1, 16) * 15 + 127) // 255
    alpha_bits = (a4 << (4 * np.arange(16, dtype=np.uint64))).sum(1).astype("<u8")
    colour = np.frombuffer(_encode_dxt1_numpy(np, w, h, rgba), dtype="<u8")
    out = np.empty((len(colour), 2), dtype="<u8")
    out[:, 0], out[:, 1] = alpha_bits, colour
    return out.tobytes()


def _keep_coverage(np, rgba, coverage, ref):
    """Scale a mip level's alpha so the share of texels at or above `ref` stays `coverage`: the
    plain average thins alpha-tested hair out to nothing a few levels down."""
    a = np.frombuffer(rgba, np.uint8).reshape(-1, 4).copy()
    alpha = a[:, 3].astype(np.float32)
    lo, hi = 0.25, 8.0
    for _ in range(20):
        mid = (lo + hi) / 2
        if (np.minimum(alpha * mid, 255) >= ref).mean() < coverage:
            lo = mid
        else:
            hi = mid
    a[:, 3] = np.minimum(alpha * hi, 255).astype(np.uint8)
    return a.tobytes()


def write_alpha_tex(path, w, h, rgba, ref=128):
    """DXT3 with a mip chain (model textures with alpha, such as hair cards), each level keeping
    the share of texels a test against `ref` lets through. Needs numpy."""
    np = _numpy()
    if np is None:
        raise RuntimeError("write_alpha_tex needs numpy (run with: uv run --with numpy)")
    levels = [(w, h, rgba)]
    coverage = (np.frombuffer(rgba, np.uint8)[3::4] >= ref).mean()
    while min(levels[-1][0], levels[-1][1]) > 8:
        nw, nh, data = _downsample_numpy(np, *levels[-1])
        levels.append((nw, nh, _keep_coverage(np, data, coverage, ref)))
    avg = np.frombuffer(rgba, np.uint8).reshape(-1, 4).mean(0).astype(int)
    argb = int(avg[3]) << 24 | int(avg[0]) << 16 | int(avg[1]) << 8 | int(avg[2])
    header = b"ZTEX" + struct.pack("<8I", 0, FORMAT_DXT3, w, h, len(levels), w, h, argb)
    with open(path, "wb") as f:
        f.write(header + b"".join(_encode_dxt3_numpy(np, *lv) for lv in reversed(levels)))


def write_tex(path, w, h, rgba, alpha=False):
    """DXT1 with mipmaps (model textures), or with alpha=True one DXT3 level (HUD textures)."""
    np = None if alpha else _numpy()
    if np is not None:  # large textures (the radar map) in seconds instead of minutes
        levels = [(w, h, rgba)]
        while min(levels[-1][0], levels[-1][1]) > 8:
            levels.append(_downsample_numpy(np, *levels[-1]))
        avg = np.frombuffer(rgba, np.uint8).reshape(-1, 4).mean(0).astype(int)
        argb = 0xFF << 24 | int(avg[0]) << 16 | int(avg[1]) << 8 | int(avg[2])
        header = b"ZTEX" + struct.pack("<8I", 0, FORMAT_DXT1, w, h, len(levels), w, h, argb)
        with open(path, "wb") as f:
            f.write(header + b"".join(_encode_dxt1_numpy(np, *lv) for lv in reversed(levels)))
        return
    levels = [(w, h, rgba)]
    while not alpha and min(levels[-1][0], levels[-1][1]) > 8:
        levels.append(downsample(*levels[-1]))
    n = w * h
    avg = [sum(rgba[c::4]) // n for c in range(4)]
    argb = (avg[3] if alpha else 0xFF) << 24 | avg[0] << 16 | avg[1] << 8 | avg[2]
    header = b"ZTEX" + struct.pack("<8I", 0, FORMAT_DXT3 if alpha else FORMAT_DXT1, w, h, len(levels), w, h, argb)
    with open(path, "wb") as f:
        f.write(header + b"".join(encode_dxt1(*lv, alpha=alpha) for lv in reversed(levels)))


# Uncompressed formats, their pixels as ZenGin's own table has them (zCRenderer::GetTexFormatInfo,
# the same in GothicMod.exe 1.08k and Gothic2.exe 2.6: bit positions in a little-endian pixel).
FORMAT_ARGB_8888 = 0  # 4 bytes: A, R, G, B
FORMAT_BGRA_8888 = 3  # 4 bytes: B, G, R, A (Direct3D's A8R8G8B8)
FORMAT_ARGB_4444 = 6  # a little-endian 16-bit word: A in bits 12-15, R 8-11, G 4-7, B 0-3


def write_raw_tex(path, w, h, rgba, fmt=FORMAT_ARGB_4444):
    """One uncompressed level from RGBA bytes: no DXT blocks, for sharp UI art and small text.

    Gothic's Direct3D 7 renderer (zCRnd_D3D::SupportsTextureFormat) takes ARGB_4444, ARGB_1555 and
    RGB_565 as they are, DXT1 and DXT3 where the card has them, and none of the 32- or 24-bit
    formats: a texture in one is converted when it loads (zCTexture::CalcNextBestTexFormat: 0 -> 1 ->
    2 -> 3 -> ARGB_1555, one bit of alpha). So ARGB_4444 is the format for art with soft edges: four
    bits per channel and the same 16 alpha levels as DXT3 (Gothic's raindrop, SKYRAINDROP-C.TEX, is
    one). The channels are rounded to the nearest level; dither before for smooth gradients.
    The 32-bit formats are written as the table has them, for renderers that take them. Needs numpy."""
    np = _numpy()
    if np is None:
        raise RuntimeError("write_raw_tex needs numpy (run with: uv run --with numpy)")
    a = np.frombuffer(rgba, np.uint8).reshape(-1, 4)
    if fmt == FORMAT_ARGB_4444:
        q = (a.astype(np.uint16) * 15 + 127) // 255
        data = (q[:, 3] << 12 | q[:, 0] << 8 | q[:, 1] << 4 | q[:, 2]).astype("<u2").tobytes()
    elif fmt == FORMAT_ARGB_8888:
        data = np.ascontiguousarray(a[:, [3, 0, 1, 2]]).tobytes()
    elif fmt == FORMAT_BGRA_8888:
        data = np.ascontiguousarray(a[:, [2, 1, 0, 3]]).tobytes()
    else:
        raise ValueError(f"write_raw_tex: format {fmt} is not an uncompressed one it writes")
    avg = a.mean(0).astype(int)
    argb = int(avg[3]) << 24 | int(avg[0]) << 16 | int(avg[1]) << 8 | int(avg[2])
    header = b"ZTEX" + struct.pack("<8I", 0, fmt, w, h, 1, w, h, argb)
    with open(path, "wb") as f:
        f.write(header + data)
