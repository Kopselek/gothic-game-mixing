"""The ground of the CJMovement plugin's GTA San Andreas radar for Gothic 1's world.

Two styles (--style):
  gothic  (default) Gothic's own map of the colony, Map_World.tga (the world map item's page),
          its torn parchment laid on a dark ground. It covers the world as Gothic's map document
          does when it places the player's arrow (oCViewDocumentMap::UpdatePosition): the page's
          middle is the world's origin and the page spans the size of the world's BSP root box.
  gta     a map drawn in the colours of GTA's radar from the world mesh, see below.

The gta style: the world mesh of WORLD.ZEN (worlds.VDF) is drawn from above: every polygon facing up in the
colour class of its material (Gothic keeps each material's texture average colour and material
group in the mesh), mapped onto the colours of GTA's own radar tiles (gta3.img radarNN.txd:
grass, dirt, sand, concrete, water), lit from the north-west so hills and cliffs read. Higher
polygons are drawn over lower ones, so roofs cover floors and the ground covers caves.

Writes _work\\DATA\\TEXTURES\\_COMPILED\\CJ_RADAR-C.TEX and the world rectangle it covers to
Gothic.ini [CJMOVEMENT] radarX0, radarZ0, radarX1, radarZ1 (Gothic world x and z, cm; north = +z
is the top of the map).

usage: python make_cj_radar.py [--gothic DIR] [--style gothic|gta] [--size 2048] [--preview FILE.png]
Needs Pillow (pip install pillow).
"""
import argparse
import colorsys
import math
import os
import re
import struct

from PIL import Image, ImageDraw

from tex import write_tex
from vdf import VdfReader

DEFAULT_GOTHIC = r"C:\Program Files (x86)\Steam\steamapps\common\Gothic"
MAT_WATER, MAT_SNOW, MAT_WOOD, MAT_STONE = 5, 6, 3, 2
# GTA's radar tile colours (most common colours of radar00-143, rounded)
WATER = (114, 140, 172)
GRASS_LIGHT, GRASS_DARK = (112, 136, 56), (56, 104, 40)
DIRT, SAND = (112, 96, 80), (200, 176, 104)
CONCRETE, ROCK = (152, 152, 152), (110, 108, 104)
SNOW = (232, 232, 232)
LIGHT = (-0.45, 0.8, 0.4)  # from the north-west and above (x, y up, z north)


def read_world(gothic_dir):
    """(materials [(name, group, (r, g, b))], vertices [(x, y, z)], polygons [(material, normal, [vertex])])
    of WORLD.ZEN's world mesh (zCMesh chunks, Gothic 1 layout)."""
    vdf = VdfReader(os.path.join(gothic_dir, "Data", "worlds.VDF"))
    data = vdf.read(next(k for k in vdf.files if k.upper().endswith("/WORLD.ZEN")))
    o = data.index(b"[MeshAndBsp % 0 0]") + len(b"[MeshAndBsp % 0 0]") + 8  # after bsp version, size
    chunks = {}
    while True:
        cid, length = struct.unpack_from("<HI", data, o)
        chunks[cid] = (o + 6, length)
        o += 6 + length
        if cid == 0xB060:
            break

    start, _ = chunks[0xB020]  # materials: an embedded binary zCArchiver
    o = data.index(b"END\n\n", data.index(b"objects", start)) + 5
    count = struct.unpack_from("<I", data, o)[0]
    o += 4
    materials = []
    for _ in range(count):
        o = data.index(b"\0", o) + 1  # list name
        size = struct.unpack_from("<I", data, o)[0]
        payload = o + 4
        p = payload + 2 + 4  # version, object index
        p = data.index(b"\0", p) + 1  # object name "%"
        p = data.index(b"\0", p) + 1  # class "zCMaterial"
        end = data.index(b"\0", p)
        name = data[p:end].decode("latin-1")
        group = data[end + 1]
        b, g, r = data[end + 2], data[end + 3], data[end + 4]
        materials.append((name, group, (r, g, b)))
        o = payload + size

    start, _ = chunks[0xB030]
    n = struct.unpack_from("<I", data, start)[0]
    vertices = list(struct.iter_unpack("<3f", data[start + 4:start + 4 + 12 * n]))

    start, length = chunks[0xB050]
    n = struct.unpack_from("<I", data, start)[0]
    o = start + 4
    polygons = []
    for _ in range(n):
        material, _, _, nx, ny, nz, flags, count = struct.unpack_from("<hhffffIB", data, o)
        o += 25
        index = [struct.unpack_from("<H", data, o + 6 * i)[0] for i in range(count)]
        o += 6 * count
        if flags & 3:  # portal polygons: invisible
            continue
        polygons.append((material, (nx, ny, nz), index))
    assert o == start + length, "polygon chunk layout"
    return materials, vertices, polygons


# Gothic's material names say what the ground is (German and English parts): checked in order.
NAME_CLASSES = [
    (("UNDERWATER", "WASSER", "WATER", "WATST", "WFALL"), WATER),
    (("STRAND", "SAND", "BEACH"), SAND),
    (("SWAMP", "SUMPF", "GRASSDA", "FOREST", "WALD", "MOOS", "MOSS"), GRASS_DARK),
    (("GRASS", "WIESE", "FIELD", "FELD"), GRASS_LIGHT),
    (("TILE", "PFLASTER", "STREET", "PATH", "WEG", "PLATZ", "FLOOR", "BODEN", "WALL", "MAUER", "STONE", "STEIN"), CONCRETE),
    (("MOUNTAIN", "ROCK", "FELS", "CLIFF", "BERG"), ROCK),
    (("WOOD", "HOLZ", "BAUM", "PLANK", "BRETT", "ROOF", "DACH"), DIRT),
]


def radar_colour(name, group, colour):
    """The GTA radar colour class of a Gothic material: by its name, else by its group and its
    texture's average colour (Gothic's grass is olive, so olive counts as grass)."""
    if group == MAT_WATER:
        return WATER
    if group == MAT_SNOW:
        return SNOW
    upper = name.upper()
    for words, colour_class in NAME_CLASSES:
        if any(w in upper for w in words):
            return colour_class
    r, g, b = colour
    h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
    if s < 0.18:  # grey: built stone, rock
        return CONCRETE if v > 0.4 else ROCK
    if 0.11 < h < 0.45 and s > 0.3:  # olive to green
        return GRASS_LIGHT if v > 0.33 else GRASS_DARK
    return SAND if v > 0.55 and h < 0.17 else DIRT


def build(gothic_dir, size, preview=None):
    materials, vertices, polygons = read_world(gothic_dir)
    classes = [radar_colour(*m) for m in materials]
    up = [(m, n, idx) for m, n, idx in polygons  # floors and slopes, not walls or portals
          if n[1] > 0.2 and "PORTAL" not in materials[m][0].upper()]
    used = {i for _, _, idx in up for i in idx}
    xs = [vertices[i][0] for i in used]
    zs = [vertices[i][2] for i in used]
    x0, x1, z0, z1 = min(xs), max(xs), min(zs), max(zs)
    span = max(x1 - x0, z1 - z0) * 1.02
    cx, cz = (x0 + x1) / 2, (z0 + z1) / 2
    x0, x1, z0, z1 = cx - span / 2, cx + span / 2, cz - span / 2, cz + span / 2
    scale = size / span

    image = Image.new("RGB", (size, size), WATER)
    draw = ImageDraw.Draw(image)
    light = [c / math.sqrt(sum(l * l for l in LIGHT)) for c in LIGHT]
    # painter's order: lowest first, so the highest surface stays on top
    up.sort(key=lambda p: sum(vertices[i][1] for i in p[2]) / len(p[2]))
    for material, normal, index in up:
        base = classes[material]
        shade = 1.0 if base == WATER else 0.82 + 0.3 * (sum(n * l for n, l in zip(normal, light)) - 0.55)
        colour = tuple(max(0, min(255, round(c * shade))) for c in base)
        points = [((vertices[i][0] - x0) * scale, (z1 - vertices[i][2]) * scale) for i in index]
        draw.polygon(points, fill=colour)
    if preview:
        image.save(preview)
    out = os.path.join(gothic_dir, "_work", "DATA", "TEXTURES", "_COMPILED", "CJ_RADAR-C.TEX")
    write_tex(out, size, size, image.convert("RGBA").tobytes())
    print(f"radar: {len(up)} of {len(polygons)} polygons, world x {x0:.0f}..{x1:.0f} z {z0:.0f}..{z1:.0f} cm, "
          f"{span / size:.0f} cm per pixel -> {out}")
    return x0, z0, x1, z1


def bsp_box(gothic_dir):
    """WORLD.ZEN's BSP root box (mins x y z, maxs x y z): the first node of the tree chunk."""
    vdf = VdfReader(os.path.join(gothic_dir, "Data", "worlds.VDF"))
    data = vdf.read(next(k for k in vdf.files if k.upper().endswith("/WORLD.ZEN")))
    o = data.index(b"[MeshAndBsp % 0 0]") + len(b"[MeshAndBsp % 0 0]")
    _, size = struct.unpack_from("<II", data, o)
    o += 8
    end = o + size
    while o < end:
        cid, length = struct.unpack_from("<HI", data, o)
        if cid == 0xC040:  # u32 node count, u32 leaf count, then the root's bbox
            return struct.unpack_from("<6f", data, o + 6 + 8)
        o += 6 + length
    raise ValueError("WORLD.ZEN: no BSP tree chunk")


GOTHIC_GROUND = (34, 27, 20)  # under the parchment's torn edges: off the map


def build_gothic(gothic_dir, size, preview=None):
    """Map_World.tga on the dark ground, size x size, and the world rectangle it shows."""
    import io
    vdf = VdfReader(os.path.join(gothic_dir, "Data", "textures.VDF"))
    key = next(k for k in vdf.files if k.upper().endswith("/MAP_WORLD.TGA"))  # the source, sharper than the compiled 512
    page = Image.open(io.BytesIO(vdf.read(key))).convert("RGBA")
    ground = Image.new("RGBA", page.size, GOTHIC_GROUND + (255,))
    image = Image.alpha_composite(ground, page).convert("RGB").resize((size, size), Image.LANCZOS)
    if preview:
        image.save(preview)
    out = os.path.join(gothic_dir, "_work", "DATA", "TEXTURES", "_COMPILED", "CJ_RADAR-C.TEX")
    write_tex(out, size, size, image.convert("RGBA").tobytes())
    lo_x, _, lo_z, hi_x, _, hi_z = bsp_box(gothic_dir)
    dx, dz = hi_x - lo_x, hi_z - lo_z
    print(f"radar: Gothic's Map_World.tga {page.size[0]}x{page.size[1]} -> {size}x{size}, world x {-dx / 2:.0f}..{dx / 2:.0f} "
          f"z {-dz / 2:.0f}..{dz / 2:.0f} cm -> {out}")
    return -dx / 2, -dz / 2, dx / 2, dz / 2


def write_ini(gothic_dir, bounds):
    path = os.path.join(gothic_dir, "system", "Gothic.ini")
    text = open(path, "rb").read().decode("latin-1")
    nl = "\r\n" if "\r\n" in text else "\n"
    lines = text.split(nl)
    start = lines.index("[CJMOVEMENT]")
    end = next((i for i in range(start + 1, len(lines)) if lines[i].startswith("[")), len(lines))
    section = [l for l in lines[start:end] if not re.match(r"(radarX0|radarZ0|radarX1|radarZ1)=", l)
               and not l.startswith("; ... world rectangle of the radar map")]
    last = max(i for i, l in enumerate(section) if l.strip())
    values = [f"radar{k}={v:.0f}" for k, v in zip(("X0", "Z0", "X1", "Z1"), bounds)]
    section = section[:last + 1] + values + ["; ... world rectangle of the radar map (make_cj_radar.py)"] + section[last + 1:]
    lines[start:end] = section
    open(path, "wb").write(nl.join(lines).encode("latin-1"))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--gothic", default=DEFAULT_GOTHIC, help="Gothic 1 install folder")
    parser.add_argument("--style", choices=("gothic", "gta"), default="gothic", help="Gothic's map, or one in GTA's radar colours")
    parser.add_argument("--size", type=int, help="map texture size in pixels (gothic 1024, gta 2048)")
    parser.add_argument("--preview", help="also save the map as this PNG")
    args = parser.parse_args()
    if args.style == "gothic":
        bounds = build_gothic(args.gothic, args.size or 1024, args.preview)
    else:
        bounds = build(args.gothic, args.size or 2048, args.preview)
    write_ini(args.gothic, bounds)


if __name__ == "__main__":
    main()
