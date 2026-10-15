"""Minecraft's blocks for the Minecraft plugin: meshes and textures from the player's jar.

Each block is a cube of 1 m (a block is a metre), its faces as Minecraft's block model has them
(grass with its side overlay and the plains' grass colour, a log's rings on top and bottom, the
crafting table's front on two sides), written to
  _work\\DATA\\MESHES\\MC\\MC_BLK_<ID>.3DS        the block the plugin places (centred on its middle)
  _work\\DATA\\MESHES\\MC\\MC_BLK_<ID>_HAND.3DS   a small one he holds in his hand
  _work\\DATA\\TEXTURES\\_COMPILED\\MC_B_<TEXTURE>-C.TEX  the faces' textures, every pixel 8 x 8
                                                 (nearest), with mipmaps; glass and leaves keep
                                                 their see-through pixels
  _work\\DATA\\MESHES\\MC\\MC_FIRE.3DS            Minecraft's fire block, MC_LAVA_<1-9>.3DS lava filled to n ninths
                                                 (write_fire, write_lava: their animated textures too)
  _work\\DATA\\MESHES\\MC\\MC_TNT_FLASH.3DS       lit TNT's flash (TntMinecartRenderer's white overlay:
                                                 its faces 75 % white), every 5 ticks of its fuse
Gothic compiles the meshes on first use (_COMPILED\\MC_BLK_*.MRM). make_mc_hud.py draws their
inventory icons from the same faces.

usage: python make_mc_blocks.py [--minecraft DIR] --gothic DIR
"""
import argparse
import io
import os
import sys

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
from mcfiles import DEFAULT_MINECRAFT, MinecraftFiles
from mesh3ds import write_3ds
from tex import write_alpha_tex, write_tex

UPSCALE = 8
SIZE, HAND = 100.0, 24.0  # cm: a block, the one in his hand
GRASS, FOLIAGE = (0x91, 0xBD, 0x59), (0x77, 0xAB, 0x2F)  # the plains' colours (GrassColor, FoliageColor)

# id, faces (top, bottom, north, east, south, west; a texture name, "a+b" b drawn over a, "*" tinted),
# see-through, hardness (Minecraft's, for breaking)
def _cube(texture):
    return (texture,) * 6


def _column(top, side, bottom=None):
    return (top, bottom or top, side, side, side, side)


BLOCKS = [
    ("grass_block", ("grass_block_top*grass", "dirt") + ("grass_block_side+grass_block_side_overlay*grass",) * 4, False, 0.6),
    ("dirt", _cube("dirt"), False, 0.5),
    ("stone", _cube("stone"), False, 1.5),
    ("cobblestone", _cube("cobblestone"), False, 2.0),
    ("oak_planks", _cube("oak_planks"), False, 2.0),
    ("oak_log", _column("oak_log_top", "oak_log"), False, 2.0),
    ("glass", _cube("glass"), True, 0.3),
    ("bricks", _cube("bricks"), False, 2.0),
    ("stone_bricks", _cube("stone_bricks"), False, 1.5),
    ("sand", _cube("sand"), False, 0.5),
    ("oak_leaves", _cube("oak_leaves*foliage"), True, 0.2),
    ("white_wool", _cube("white_wool"), False, 0.8),
    ("tnt", _column("tnt_top", "tnt_side", "tnt_bottom"), False, 0.0),
    ("crafting_table", ("crafting_table_top", "oak_planks", "crafting_table_front", "crafting_table_side",
                        "crafting_table_side", "crafting_table_front"), False, 2.5),
    ("bookshelf", _column("oak_planks", "bookshelf"), False, 1.5),
    ("glowstone", _cube("glowstone"), False, 0.3),
    ("obsidian", _cube("obsidian"), False, 50.0),
    ("diamond_block", _cube("diamond_block"), False, 5.0),
    ("gold_block", _cube("gold_block"), False, 3.0),
    ("iron_block", _cube("iron_block"), False, 5.0),
]


# blast resistance (Blocks.java: strength's second value, else its hardness; TNT breaks at once)
RESISTANCE = {"grass_block": 0.6, "dirt": 0.5, "stone": 6.0, "cobblestone": 6.0, "oak_planks": 3.0, "oak_log": 2.0,
              "glass": 0.3, "bricks": 6.0, "stone_bricks": 6.0, "sand": 0.5, "oak_leaves": 0.2, "white_wool": 0.8,
              "tnt": 0.0, "crafting_table": 2.5, "bookshelf": 1.5, "glowstone": 0.3, "obsidian": 1200.0,
              "diamond_block": 6.0, "gold_block": 6.0, "iron_block": 6.0}

# each block's sounds (Minecraft's SoundType: dig/<kind> placing and breaking; glass breaks as glass)
SOUND = {"grass_block": "grass", "dirt": "gravel", "sand": "sand", "oak_leaves": "grass", "white_wool": "cloth",
         "tnt": "grass", "oak_planks": "wood", "oak_log": "wood", "crafting_table": "wood", "bookshelf": "wood",
         "glass": "glass", "glowstone": "glass"}  # the rest: stone


def texture_name(spec):
    """MC_B_<...> for a face's texture spec."""
    return "MC_B_" + spec.replace("+", "_ON_").replace("*", "_").upper()


def face_image(files, spec):
    """A face's 16 x 16 picture: textures/block/<name>.png, "a+b" b over a, "*grass"/"*foliage"
    multiplied by the colour (Minecraft tints those grey textures)."""
    from PIL import Image, ImageChops
    layers = []
    for part in spec.split("+"):
        name, _, tint = part.partition("*")
        image = Image.open(io.BytesIO(files.read(f"assets/minecraft/textures/block/{name}.png"))).convert("RGBA")
        image = image.crop((0, 0, 16, 16))  # animated textures are frames stacked downwards
        if tint:
            colour = Image.new("RGBA", image.size, (GRASS if tint == "grass" else FOLIAGE) + (255,))
            image = ImageChops.multiply(image, colour)
        layers.append(image)
    out = layers[0]
    for layer in layers[1:]:
        out = Image.alpha_composite(out, layer)
    return out


# the cube's faces in 3ds Max's axes (z up, Gothic's forward -y): corners counter-clockwise seen
# from outside, from the texture's top left; the order of BLOCKS' faces
FACES = [
    ((-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1)),        # top: its top edge to the north (+y)
    ((-1, 1, -1), (1, 1, -1), (1, -1, -1), (-1, -1, -1)),    # bottom
    ((1, 1, 1), (-1, 1, 1), (-1, 1, -1), (1, 1, -1)),        # north (+y)
    ((-1, 1, 1), (-1, -1, 1), (-1, -1, -1), (-1, 1, -1)),    # east (-x: Gothic's +x is max -x)
    ((-1, -1, 1), (1, -1, 1), (1, -1, -1), (-1, -1, -1)),    # south (-y)
    ((1, -1, 1), (1, 1, 1), (1, 1, -1), (1, -1, -1)),        # west (+x)
]


def write_mesh(path, name, faces, half, suffix=""):
    textures = []
    for spec in faces:
        if spec not in textures:
            textures.append(spec)
    verts, uvs, tris = [], [], []
    for spec, corners in zip(faces, FACES):
        base = len(verts)
        verts += [tuple(c * half for c in corner) for corner in corners]
        uvs += [(0.0, 1.0), (1.0, 1.0), (1.0, 0.0), (0.0, 0.0)]  # V up
        m = textures.index(spec)
        outward = [sum(corner[i] for corner in corners) for i in range(3)]  # the cube is centred on 0
        for a, b, c in ((base, base + 1, base + 2), (base, base + 2, base + 3)):
            pa, pb, pc = verts[a], verts[b], verts[c]
            e1 = [pb[i] - pa[i] for i in range(3)]
            e2 = [pc[i] - pa[i] for i in range(3)]
            n = (e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0])
            if sum(n[i] * outward[i] for i in range(3)) < 0:  # counter-clockwise seen from outside
                b, c = c, b
            tris.append((a, b, c, m))
    write_3ds(path, name, verts, uvs, tris, [(texture_name(t) + suffix, texture_name(t) + suffix + ".TGA") for t in textures],
              smooth=False)


def frames(files, name, size):
    """An animated texture's frames (textures/block/<name>.png: size x size frames stacked
    downwards), in its .mcmeta's order when it gives one."""
    import json
    from PIL import Image
    sheet = Image.open(io.BytesIO(files.read(f"assets/minecraft/textures/block/{name}.png"))).convert("RGBA")
    pictures = [sheet.crop((0, i * size, size, (i + 1) * size)) for i in range(sheet.height // size)]
    try:
        order = json.loads(files.read(f"assets/minecraft/textures/block/{name}.png.mcmeta"))["animation"].get("frames")
    except KeyError:
        order = None
    return [pictures[i if isinstance(i, int) else i["index"]] for i in order] if order else pictures


def write_fire(files, meshes, textures):
    """Minecraft's fire block as it stands on the ground (blockstates/fire.json with no neighbour:
    template_fire_floor's four planes leaning in 22.5 degrees, rescaled, and template_fire_side's
    plane at each edge, all 1.4 blocks tall), both sides of every plane; its two animations
    (fire_0 on the floor planes, fire_1 on the edges) as MC_FIRE0_<n> and MC_FIRE1_<n>, 32 frames a
    second's 20 ticks in the .mcmeta's order, see-through. Its origin is the cell's bottom middle;
    the plugin swaps the frames."""
    import math
    from PIL import Image
    from make_mc_items import Mesh
    for variant in (0, 1):
        for i, frame in enumerate(frames(files, f"fire_{variant}", 16)):
            big = frame.resize((16 * UPSCALE, 16 * UPSCALE), Image.NEAREST)
            write_alpha_tex(os.path.join(textures, f"MC_FIRE{variant}_{i}-C.TEX"), big.width, big.height, big.tobytes())
    mesh = Mesh()
    floor, side = mesh.material("MC_FIRE0_0"), mesh.material("MC_FIRE1_0")
    k = 1 / math.cos(math.radians(22.5))

    def turned(p, axis, angle):
        """p (block pixels) turned about the axis through (8, 8, 8), the other two axes rescaled"""
        c, s = math.cos(math.radians(angle)), math.sin(math.radians(angle))
        x, y, z = p[0] - 8, p[1] - 8, p[2] - 8
        if axis == "x":
            y, z = (c * y - s * z) * k, (s * y + c * z) * k
        else:
            x, y = (c * x - s * y) * k, (s * x + c * y) * k
        return (x + 8, y + 8, z + 8)

    def to_cm(p):  # block pixels -> cm from the cell's bottom middle
        return (p[0] / 16 * SIZE - SIZE / 2, p[1] / 16 * SIZE, p[2] / 16 * SIZE - SIZE / 2)

    def plane(corners, material, flip_u=False):
        """corners: bottom left, bottom right, top right, top left (pixels); both sides"""
        pts = [to_cm(c) for c in corners]
        uv = [(0, 0), (1, 0), (1, 1), (0, 1)]
        if flip_u:
            uv = [(1 - u, v) for u, v in uv]
        e1 = [pts[1][i] - pts[0][i] for i in range(3)]
        e2 = [pts[3][i] - pts[0][i] for i in range(3)]
        n = (e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0])
        mesh.quad(pts, uv, n, material)
        mesh.quad(pts, uv, (-n[0], -n[1], -n[2]), material)
    h = 22.4
    for z, angle in ((8.8, -22.5), (7.2, 22.5)):
        plane([turned(p, "x", angle) for p in ((0, 0, z), (16, 0, z), (16, h, z), (0, h, z))], floor)
    for x, angle in ((8.8, -22.5), (7.2, 22.5)):
        plane([turned(p, "z", angle) for p in ((x, 0, 16), (x, 0, 0), (x, h, 0), (x, h, 16))], floor)
    e = 0.01
    plane([(0, 0, e), (16, 0, e), (16, h, e), (0, h, e)], side)
    plane([(16 - e, 0, 0), (16 - e, 0, 16), (16 - e, h, 16), (16 - e, h, 0)], side, True)
    plane([(16, 0, 16 - e), (0, 0, 16 - e), (0, h, 16 - e), (16, h, 16 - e)], side)
    plane([(e, 0, 16), (e, 0, 0), (e, h, 0), (e, h, 16)], side, True)
    mesh.write(os.path.join(meshes, "MC_FIRE.3DS"), "MC_FIRE")


def write_lava(files, meshes, textures):
    """Minecraft's lava (LiquidBlockRenderer, simplified to level tops): MC_LAVA_<1-9>.3DS a cell
    filled to n ninths (Minecraft's height: the fluid's amount / 9; 9 when lava is above it), its
    top and bottom lava_still, its sides lava_flow (the frame's top left quarter, from 1 - height
    halved, as Minecraft's sides); the animations as MC_LAVA_STILL_<n> (20 frames, played there and
    back every 2 ticks) and MC_LAVA_FLOW_<n> (32 frames every 3 ticks). Origin: the cell's bottom middle."""
    from PIL import Image
    from make_mc_items import Mesh
    sheet = Image.open(io.BytesIO(files.read("assets/minecraft/textures/block/lava_still.png"))).convert("RGBA")
    for i in range(sheet.height // 16):
        big = sheet.crop((0, i * 16, 16, i * 16 + 16)).resize((16 * UPSCALE, 16 * UPSCALE), Image.NEAREST)
        write_tex(os.path.join(textures, f"MC_LAVA_STILL_{i}-C.TEX"), big.width, big.height, big.tobytes())
    sheet = Image.open(io.BytesIO(files.read("assets/minecraft/textures/block/lava_flow.png"))).convert("RGBA")
    for i in range(sheet.height // 32):
        big = sheet.crop((0, i * 32, 32, i * 32 + 32)).resize((32 * UPSCALE, 32 * UPSCALE), Image.NEAREST)
        write_tex(os.path.join(textures, f"MC_LAVA_FLOW_{i}-C.TEX"), big.width, big.height, big.tobytes())
    half = SIZE / 2
    for level in range(1, 10):
        mesh = Mesh()
        still, flow = mesh.material("MC_LAVA_STILL_0"), mesh.material("MC_LAVA_FLOW_0")
        top = SIZE * level / 9
        v0, v1 = 1 - (1 - level / 9) * 0.5, 0.5  # (V up) from 1 - height halved down to the half
        mesh.quad([(-half, top, -half), (half, top, -half), (half, top, half), (-half, top, half)],
                  [(0, 0), (1, 0), (1, 1), (0, 1)], (0, 1, 0), still)
        mesh.quad([(-half, 0, -half), (half, 0, -half), (half, 0, half), (-half, 0, half)],
                  [(0, 0), (1, 0), (1, 1), (0, 1)], (0, -1, 0), still)
        for (ax, az) in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            # the side's two bottom and two top corners
            if ax:
                c = [(ax * half, 0, -half), (ax * half, 0, half), (ax * half, top, half), (ax * half, top, -half)]
            else:
                c = [(-half, 0, az * half), (half, 0, az * half), (half, top, az * half), (-half, top, az * half)]
            mesh.quad(c, [(0, v1), (0.5, v1), (0.5, v0), (0, v0)], (ax, 0, az), flow)
        mesh.write(os.path.join(meshes, f"MC_LAVA_{level}.3DS"), f"MC_LAVA_{level}")


def build(minecraft_dir, gothic_dir):
    from PIL import Image
    files = MinecraftFiles(minecraft_dir)
    meshes = os.path.join(gothic_dir, "_work", "DATA", "MESHES", "MC")
    textures = os.path.join(gothic_dir, "_work", "DATA", "TEXTURES", "_COMPILED")
    compiled = os.path.join(gothic_dir, "_work", "DATA", "MESHES", "_COMPILED")
    os.makedirs(meshes, exist_ok=True)
    os.makedirs(textures, exist_ok=True)
    done = set()
    for block, faces, see_through, _ in BLOCKS:
        for spec in faces:
            if spec in done:
                continue
            done.add(spec)
            big = face_image(files, spec).resize((16 * UPSCALE, 16 * UPSCALE), Image.NEAREST)
            path = os.path.join(textures, texture_name(spec) + "-C.TEX")
            if see_through:
                write_alpha_tex(path, big.width, big.height, big.tobytes())
            else:
                write_tex(path, big.width, big.height, big.convert("RGB").convert("RGBA").tobytes())
        name = "MC_BLK_" + block.upper()
        write_mesh(os.path.join(meshes, name + ".3DS"), name, faces, SIZE / 2)
        write_mesh(os.path.join(meshes, name + "_HAND.3DS"), name + "_HAND", faces, HAND / 2)
        for stale in (name + ".MRM", name + "_HAND.MRM"):  # Gothic compiles the new ones
            if os.path.exists(os.path.join(compiled, stale)):
                os.remove(os.path.join(compiled, stale))
    tnt = next(faces for block, faces, _, _ in BLOCKS if block == "tnt")
    for spec in set(tnt):
        face = face_image(files, spec).convert("RGB")
        white = Image.new("RGB", face.size, (255, 255, 255))
        big = Image.blend(face, white, 0.75).resize((16 * UPSCALE, 16 * UPSCALE), Image.NEAREST)
        write_tex(os.path.join(textures, texture_name(spec) + "_FLASH-C.TEX"), big.width, big.height, big.convert("RGBA").tobytes())
    write_mesh(os.path.join(meshes, "MC_TNT_FLASH.3DS"), "MC_TNT_FLASH", tnt, SIZE / 2, "_FLASH")
    write_fire(files, meshes, textures)
    write_lava(files, meshes, textures)
    for stale in ["MC_TNT_FLASH", "MC_FIRE"] + [f"MC_LAVA_{level}" for level in range(1, 10)]:
        if os.path.exists(os.path.join(compiled, stale + ".MRM")):
            os.remove(os.path.join(compiled, stale + ".MRM"))
    print(f"Minecraft's blocks ({files.version}): {len(BLOCKS)} blocks, {len(done)} textures")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--minecraft", default=DEFAULT_MINECRAFT, help="the Minecraft folder (%%APPDATA%%\\.minecraft)")
    parser.add_argument("--gothic", required=True, help="the Gothic 1 or Gothic 2 folder")
    args = parser.parse_args()
    build(args.minecraft, args.gothic)


if __name__ == "__main__":
    main()
