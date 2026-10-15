"""Write 3D Studio (.3DS) meshes that Gothic loads as static visuals (items, vobs, effects).

Put the file in _work\\DATA\\MESHES\\<any subfolder>\\NAME.3DS and reference it as "NAME.3DS"
(item VISUAL, zCVob::SetVisual). Gothic compiles it on first use to _work\\DATA\\MESHES\\
_COMPILED\\NAME.MRM; delete that .MRM to make Gothic recompile a changed source.

Conventions: centimetres; Gothic's object space is Y up, but a 3DS file is written Z up and
Gothic converts it (author it as in 3ds Max: Z up). Faces counter-clockwise seen from outside.
UV with V up (flip V of top-row-first textures). A material's texture is a .TGA name; Gothic
looks for _work\\DATA\\TEXTURES\\_COMPILED\\NAME-C.TEX (see tex.py) or a source .TGA.
"""
import struct


def chunk(cid, body):
    return struct.pack("<HI", cid, 6 + len(body)) + body


def cstr(text):
    return text.encode("latin-1") + b"\0"


def _material(name, texture):
    white = chunk(0x0011, bytes((255, 255, 255)))  # ambient / diffuse colour
    return chunk(0xAFFF, chunk(0xA000, cstr(name)) + chunk(0xA010, white) + chunk(0xA020, white)
                 + chunk(0xA200, chunk(0x0030, struct.pack("<H", 100)) + chunk(0xA300, cstr(texture))))


def write_3ds(path, name, verts, uvs, faces, materials, smooth=True):
    """One object `name`.
    verts: [(x, y, z)] cm; uvs: [(u, v)] one per vertex (V up);
    faces: [(a, b, c)] (all material 0) or [(a, b, c, material index)];
    materials: [(material name, texture file)] e.g. [("SWORD", "MY_SWORD.TGA")]."""
    faces = [f if len(f) == 4 else (f[0], f[1], f[2], 0) for f in faces]
    mats = b"".join(_material(n, t) for n, t in materials)
    mesh = chunk(0x4110, struct.pack("<H", len(verts)) + b"".join(struct.pack("<3f", *v) for v in verts))
    mesh += chunk(0x4140, struct.pack("<H", len(uvs)) + b"".join(struct.pack("<2f", *uv) for uv in uvs))
    mesh += chunk(0x4160, struct.pack("<12f", 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0))  # identity local axes
    groups = b""
    for m, (mname, _) in enumerate(materials):
        idx = [i for i, f in enumerate(faces) if f[3] == m]
        if idx:
            groups += chunk(0x4130, cstr(mname) + struct.pack("<H", len(idx)) + struct.pack(f"<{len(idx)}H", *idx))
    extra = chunk(0x4150, struct.pack(f"<{len(faces)}I", *([1] * len(faces)))) if smooth else b""
    mesh += chunk(0x4120, struct.pack("<H", len(faces)) + b"".join(struct.pack("<4H", a, b, c, 7) for a, b, c, _ in faces)
                  + groups + extra)
    obj = chunk(0x4000, cstr(name) + chunk(0x4100, mesh))
    editor = chunk(0x3D3D, chunk(0x3D3E, struct.pack("<I", 3)) + mats + chunk(0x0100, struct.pack("<f", 1.0)) + obj)
    with open(path, "wb") as f:
        f.write(chunk(0x4D4D, chunk(0x0002, struct.pack("<I", 3)) + editor))
