"""Minecraft's items for the Minecraft plugin (the bow and arrows, flint and steel, spawn eggs) and
its first-person hand: meshes and textures from the player's jar.

The bow (and its three drawn stages) and the arrow in Steve's right hand are Minecraft's item
models (ItemModelGenerator: the picture a pixel thick, its edges closed pixel by pixel), placed in
his hand as Minecraft places a held item (ItemInHandLayer on the right arm at rest, then the model's
thirdperson_righthand display), in the frame of his ZS_RIGHTHAND node (make_steve.py's skeleton).
The flying arrow is the arrow entity's model (ArrowModel: two crossed planes and a cross at the
back, entity/projectiles/arrow.png), its tip forward (+z). Only opaque pixels get faces, so the
textures need no alpha.
First person (FirstPersonHandsAndItemsRenderer): his bare right arm as Minecraft draws it there
(renderPlayerArm's turns after the swing, the arm part turned 0.1 about z; his skin, the sleeve
where it is painted), each item at its model's firstperson_righthand display, each block at
block.json's (45 degrees, 0.4); in Minecraft's view space (blocks, -z ahead) at a quarter of the
size and distance (FP_UNIT cm a block: the same on screen, nearer the eyes than any wall), z
turned ahead for Gothic's camera. The plugin puts them in front of the camera with Minecraft's
movements (the swing, the hand's dip on a change, the bow's draw, the sway). Written to
  _work\\DATA\\MESHES\\MC\\MC_ITEM_<ITEM>.3DS   in his hand (third person)
  _work\\DATA\\MESHES\\MC\\MC_ARROW.3DS         a shot arrow
  _work\\DATA\\MESHES\\MC\\MC_FP_ARM.3DS, MC_FP_<ITEM>.3DS, MC_FP_BLK_<BLOCK>.3DS  first person
  _work\\DATA\\TEXTURES\\_COMPILED\\MC_EXPLOSION_<0-15>-C.TEX  the explosion's puff (particle/explosion_*),
                                                         its see-through pixels kept
  _work\\DATA\\TEXTURES\\_COMPILED\\MC_I_<NAME>-C.TEX  their pictures, every pixel 8 x 8 (nearest), mipmaps
Gothic compiles the meshes on first use (_COMPILED\\MC_*.MRM).

usage: python make_mc_items.py [--minecraft DIR] --gothic DIR
"""
import argparse
import io
import math
import os
import sys

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
from mcfiles import DEFAULT_MINECRAFT, MinecraftFiles
from mesh3ds import write_3ds
from tex import write_alpha_tex, write_tex

UPSCALE = 8
# the hand's items: mesh, texture (textures/item), the model's thirdperson_righthand display
# (rotation in degrees, translation in pixels, scale): bow.json's, and generated.json's for the arrow
BOW_DISPLAY = ((-80, 260, -40), (-1, -2, 2.5), 0.9)
GENERATED_DISPLAY = ((0, 0, 0), (0, 3, 1), 0.55)
HAND_ITEMS = [
    ("MC_ITEM_BOW", "bow", BOW_DISPLAY),
    ("MC_ITEM_BOW_PULLING_0", "bow_pulling_0", BOW_DISPLAY),
    ("MC_ITEM_BOW_PULLING_1", "bow_pulling_1", BOW_DISPLAY),
    ("MC_ITEM_BOW_PULLING_2", "bow_pulling_2", BOW_DISPLAY),
    ("MC_ITEM_ARROW", "arrow", GENERATED_DISPLAY),
    ("MC_ITEM_FLINT_AND_STEEL", "flint_and_steel", GENERATED_DISPLAY),
    ("MC_ITEM_ZOMBIE_SPAWN_EGG", "zombie_spawn_egg", GENERATED_DISPLAY),
    ("MC_ITEM_SKELETON_SPAWN_EGG", "skeleton_spawn_egg", GENERATED_DISPLAY),
    ("MC_ITEM_CREEPER_SPAWN_EGG", "creeper_spawn_egg", GENERATED_DISPLAY),
    ("MC_ITEM_ENDER_DRAGON_SPAWN_EGG", "ender_dragon_spawn_egg", GENERATED_DISPLAY),
    ("MC_ITEM_LAVA_BUCKET", "lava_bucket", GENERATED_DISPLAY),
    ("MC_ITEM_BUCKET", "bucket", GENERATED_DISPLAY),
]
ARROW = "MC_ARROW"
ARROW_TEXTURE = "entity/projectiles/arrow"

# first person: cm a block of Minecraft's view space; the firstperson_righthand displays (item/generated,
# handheld and bow share theirs; block/block.json's); the items drawn there: Minecraft's own and those
# the hand shows for Gothic's drawn weapons and spells (the plugin's Icon)
FP_UNIT = 25.0
ITEM_FP = ((0, -90, 25), (1.13, 3.2, 1.13), 0.68)
BLOCK_FP = ((0, 45, 0), (0, 0, 0), 0.40)
FP_ITEMS = ["bow", "bow_pulling_0", "bow_pulling_1", "bow_pulling_2", "arrow", "flint_and_steel",
            "zombie_spawn_egg", "skeleton_spawn_egg", "creeper_spawn_egg", "ender_dragon_spawn_egg", "lava_bucket", "bucket",
            "wooden_sword", "stone_sword", "iron_sword", "diamond_sword", "netherite_sword",
            "wooden_axe", "stone_axe", "iron_axe", "diamond_axe", "netherite_axe",
            "crossbow_standby", "enchanted_book", "paper"]


# --- vectors (column vectors, right-handed rotations as JOML's) ---

def sub(a, b):
    return tuple(a[i] - b[i] for i in range(3))


def add(a, b):
    return tuple(a[i] + b[i] for i in range(3))


def scale(a, k):
    return tuple(c * k for c in a)


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def dot(a, b):
    return sum(a[i] * b[i] for i in range(3))


def rx(deg):
    c, s = math.cos(math.radians(deg)), math.sin(math.radians(deg))
    return lambda v: (v[0], c * v[1] - s * v[2], s * v[1] + c * v[2])


def ry(deg):
    c, s = math.cos(math.radians(deg)), math.sin(math.radians(deg))
    return lambda v: (c * v[0] + s * v[2], v[1], -s * v[0] + c * v[2])


def rz(deg):
    c, s = math.cos(math.radians(deg)), math.sin(math.radians(deg))
    return lambda v: (c * v[0] - s * v[1], s * v[0] + c * v[1], v[2])


class Mesh:
    """Quads in Gothic's engine axes, written as a 3ds file (3ds Max axes) with outward windings."""

    def __init__(self):
        self.verts, self.uvs, self.tris, self.materials = [], [], [], []

    def material(self, texture):
        if texture not in self.materials:
            self.materials.append(texture)
        return self.materials.index(texture)

    def quad(self, corners, uv, normal, material):
        """corners and normal in engine axes (cm), uv: four (u, v) with V up."""
        from make_steve import max_pos
        corners = [max_pos(c) for c in corners]
        normal = max_pos(normal)
        base = len(self.verts)
        self.verts += corners
        self.uvs += uv
        for a, b, c in ((0, 1, 2), (0, 2, 3)):
            n = cross(sub(corners[b], corners[a]), sub(corners[c], corners[a]))
            if dot(n, normal) < 0:  # counter-clockwise seen from outside, as make_mc_blocks.py's
                b, c = c, b
            self.tris.append((base + a, base + b, base + c, material))

    def write(self, path, name):
        write_3ds(path, name, self.verts, self.uvs, self.tris, [(t, t + ".TGA") for t in self.materials], smooth=False)


def picture(files, name):
    from PIL import Image
    return Image.open(io.BytesIO(files.read(f"assets/minecraft/textures/{name}.png"))).convert("RGBA")


def write_picture(image, path):
    """The picture 8 times larger (nearest), its see-through pixels filled with their opaque
    neighbours' colour (the faces cover only opaque pixels; filtering then brings no dark edge)."""
    from PIL import Image
    w, h = image.size
    src = image.load()
    out = image.copy()
    dst = out.load()
    for y in range(h):
        for x in range(w):
            if src[x, y][3] == 0:
                near = [src[x + dx, y + dy] for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (-1, -1), (1, -1), (-1, 1))
                        if 0 <= x + dx < w and 0 <= y + dy < h and src[x + dx, y + dy][3] > 0]
                dst[x, y] = near[0][:3] + (255,) if near else (0, 0, 0, 255)
            else:
                dst[x, y] = src[x, y][:3] + (255,)
    big = out.resize((w * UPSCALE, h * UPSCALE), Image.NEAREST)
    write_tex(path, big.width, big.height, big.tobytes())


def opaque(image):
    w, h = image.size
    px = image.load()
    return lambda x, y: 0 <= x < w and 0 <= y < h and px[x, y][3] > 0


def hand_transform(display, hand_rot, hand_pos):
    """Item model pixels (0-16, y up) -> engine cm in the frame of Steve's ZS_RIGHTHAND, the right
    arm at rest: ItemTransform.apply, then ItemInHandLayer.submitArmWithItem's turns and offset, then
    the arm's pivot (PlayerModel's right arm at -5, 2, 0), in Minecraft's model space (y down, 16
    pixels a unit); then make_steve.py's model pixels -> engine cm."""
    from make_steve import from_minecraft
    (ax, ay, az), (tx, ty, tz), k = display
    turn_x, turn_y, turn_z = rx(ax), ry(ay), rz(az)
    hand_rx, hand_ry = rx(-90), ry(180)

    def point(q):
        v = scale(q, 1 / 16)
        v = sub(v, (0.5, 0.5, 0.5))
        v = scale(v, k)
        v = turn_x(turn_y(turn_z(v)))  # rotationXYZ: x(y(z(v)))
        v = add(v, (tx / 16, ty / 16, tz / 16))
        v = add(v, (1 / 16, 2 / 16, -10 / 16))
        v = hand_rx(hand_ry(v))
        v = add(v, (-5 / 16, 2 / 16, 0))
        world = from_minecraft(scale(v, 16))
        rel = sub(world, hand_pos)
        return tuple(sum(hand_rot[r][i] * rel[r] for r in range(3)) for i in range(3))  # hand_rot^T rel

    def normal(q, n):
        return sub(point(add(q, n)), point(q))
    return point, normal


def view_transform(display):
    """Item model pixels -> the first-person frame (cm, +z ahead): the display (ItemTransform.apply)
    in Minecraft's view space, FP_UNIT cm a block, its -z turned to +z."""
    (ax, ay, az), (tx, ty, tz), k = display
    turn_x, turn_y, turn_z = rx(ax), ry(ay), rz(az)

    def point(q):
        v = scale(sub(scale(q, 1 / 16), (0.5, 0.5, 0.5)), k)
        v = add(turn_x(turn_y(turn_z(v))), (tx / 16, ty / 16, tz / 16))
        return (v[0] * FP_UNIT, v[1] * FP_UNIT, -v[2] * FP_UNIT)

    def normal(q, n):
        return sub(point(add(q, n)), point(q))
    return point, normal


def item_mesh(image, texture, transform):
    """ItemModelGenerator's model: the picture between z 7.5 and 8.5 (front +z), edges per pixel;
    transform: (point, normal) of item model pixels."""
    point, normal = transform
    filled = opaque(image)
    mesh = Mesh()
    m = mesh.material(texture)
    w, h = image.size
    unit = 16 / w  # model pixels a picture pixel

    def face(q0, q1, q2, q3, n, uv):
        mesh.quad([point(q) for q in (q0, q1, q2, q3)], uv, normal(q0, n), m)

    for py in range(h):
        y0, y1 = (h - 1 - py) * unit, (h - py) * unit
        vt, vb = 1 - py / h, 1 - (py + 1) / h
        px = 0
        while px < w:  # the front and back as runs of a row's opaque pixels
            if not filled(px, py):
                px += 1
                continue
            end = px
            while filled(end + 1, py):
                end += 1
            x0, x1 = px * unit, (end + 1) * unit
            u0, u1 = px / w, (end + 1) / w
            uv = [(u0, vb), (u1, vb), (u1, vt), (u0, vt)]
            face((x0, y0, 8.5), (x1, y0, 8.5), (x1, y1, 8.5), (x0, y1, 8.5), (0, 0, 1), uv)
            face((x0, y0, 7.5), (x1, y0, 7.5), (x1, y1, 7.5), (x0, y1, 7.5), (0, 0, -1), uv)
            px = end + 1
        for px in range(w):  # the edges: a pixel's side next to a see-through one
            if not filled(px, py):
                continue
            x0, x1 = px * unit, (px + 1) * unit
            u0, u1 = px / w, (px + 1) / w
            uv = [(u0, vb), (u1, vb), (u1, vt), (u0, vt)]
            if not filled(px - 1, py):
                face((x0, y0, 7.5), (x0, y0, 8.5), (x0, y1, 8.5), (x0, y1, 7.5), (-1, 0, 0), uv)
            if not filled(px + 1, py):
                face((x1, y0, 7.5), (x1, y0, 8.5), (x1, y1, 8.5), (x1, y1, 7.5), (1, 0, 0), uv)
            if not filled(px, py - 1):
                face((x0, y1, 7.5), (x1, y1, 7.5), (x1, y1, 8.5), (x0, y1, 8.5), (0, 1, 0), uv)
            if not filled(px, py + 1):
                face((x0, y0, 7.5), (x1, y0, 7.5), (x1, y0, 8.5), (x0, y0, 8.5), (0, -1, 0), uv)
    return mesh


def arrow_mesh(image, texture):
    """ArrowModel (32 x 32 texture): two planes 16 x 4 (texture 16 x 5) crossed along the shaft
    (x -12..4, turned 45 and 135 degrees about it), the back a cross 5 x 5 at x -11 (turned 45,
    scaled 0.8), all scaled 0.9; Minecraft's model x (the tip's way) is the engine's z."""
    filled = opaque(image)
    w, h = image.size
    mesh = Mesh()
    m = mesh.material(texture)
    cm = 0.9 * 100 / 16  # model pixels -> cm

    def to_engine(v):
        return (v[2] * cm, v[1] * cm, v[0] * cm)

    def plane(origin, du, dv, n, region, turn, flip_u=False):
        """A face: the texels of `region` (u, v, columns, rows) laid from `origin` along du (a
        column) and dv (a row), facing n; turned by `turn` (model pixels)."""
        u0r, v0r, cols, rows = region
        for r in range(rows):
            for c in range(cols):
                tu = u0r + (cols - 1 - c if flip_u else c)
                if not filled(tu, v0r + r):
                    continue
                q0 = add(origin, add(scale(du, c), scale(dv, r)))
                q1, q2, q3 = add(q0, du), add(add(q0, du), dv), add(q0, dv)
                uv = [(a / w, 1 - b / h) for a, b in ((tu, v0r + r), (tu + 1, v0r + r), (tu + 1, v0r + r + 1), (tu, v0r + r + 1))]
                if flip_u:
                    uv = [uv[1], uv[0], uv[3], uv[2]]
                corners = [to_engine(turn(q)) for q in (q0, q1, q2, q3)]
                mesh.quad(corners, uv, sub(to_engine(turn(n)), to_engine(turn((0, 0, 0)))), m)

    for angle in (45, 135):
        turn = rx(angle)
        # front (-z) texels 0-16, back (+z) texels 16-32 mirrored; rows 0-5 over y -2..2
        plane((-12, -2, 0), (1, 0, 0), (0, 0.8, 0), (0, 0, -1), (0, 0, 16, 5), turn)
        plane((-12, -2, 0), (1, 0, 0), (0, 0.8, 0), (0, 0, 1), (16, 0, 16, 5), turn, flip_u=True)
    back_turn = rx(45)

    def back(v):
        return add(scale(back_turn(v), 0.8), (-11, 0, 0))
    plane((0, -2.5, -2.5), (0, 0, 1), (0, 1, 0), (-1, 0, 0), (0, 5, 5, 5), back)
    plane((0, -2.5, -2.5), (0, 0, 1), (0, 1, 0), (1, 0, 0), (5, 5, 5, 5), back)
    return mesh


def block_mesh(faces):
    """A block model's cube (0-16; faces top, bottom, north, east, south, west as make_mc_blocks.py's
    BLOCKS) in the first-person frame at block.json's display, each face's UV Minecraft's default
    for its place."""
    from make_mc_blocks import texture_name
    point, normal = view_transform(BLOCK_FP)
    mesh = Mesh()
    top, bottom, north, east, south, west = faces
    # corners (x, y, z model pixels) with their (u, v) in texture pixels (v down), outward
    sides = [
        (top, [((0, 16, 0), (0, 0)), ((16, 16, 0), (16, 0)), ((16, 16, 16), (16, 16)), ((0, 16, 16), (0, 16))], (0, 1, 0)),
        (bottom, [((0, 0, 16), (0, 0)), ((16, 0, 16), (16, 0)), ((16, 0, 0), (16, 16)), ((0, 0, 0), (0, 16))], (0, -1, 0)),
        (north, [((16, 16, 0), (0, 0)), ((0, 16, 0), (16, 0)), ((0, 0, 0), (16, 16)), ((16, 0, 0), (0, 16))], (0, 0, -1)),
        (south, [((0, 16, 16), (0, 0)), ((16, 16, 16), (16, 0)), ((16, 0, 16), (16, 16)), ((0, 0, 16), (0, 16))], (0, 0, 1)),
        (west, [((0, 16, 0), (0, 0)), ((0, 16, 16), (16, 0)), ((0, 0, 16), (16, 16)), ((0, 0, 0), (0, 16))], (-1, 0, 0)),
        (east, [((16, 16, 16), (0, 0)), ((16, 16, 0), (16, 0)), ((16, 0, 0), (16, 16)), ((16, 0, 16), (0, 16))], (1, 0, 0)),
    ]
    for spec, corners, n in sides:
        m = mesh.material(texture_name(spec))
        mesh.quad([point(c) for c, _ in corners], [(u / 16, 1 - v / 16) for _, (u, v) in corners], normal(corners[0][0], n), m)
    return mesh


def arm_mesh(skin):
    """His right arm in first person (renderPlayerArm after the swing: translate (-1, 3.6, 3.5), z
    120, x 200, y -135, translate (5.6, 0, 0); the arm part at rest (-5, 2, 0) turned 0.1 about z,
    AvatarRenderer.renderHand); his skin's arm and, where painted, its sleeve."""
    from make_steve import box_faces
    turn_part, turn_y, turn_x, turn_z = rz(math.degrees(0.1)), ry(-135), rx(200), rz(120)

    def point(q):
        v = add(turn_part(scale(q, 1 / 16)), (-5 / 16, 2 / 16, 0))
        v = turn_z(turn_x(turn_y(add(v, (5.6, 0, 0)))))
        v = add(v, (-1, 3.6, 3.5))
        return (v[0] * FP_UNIT, v[1] * FP_UNIT, -v[2] * FP_UNIT)
    mesh = Mesh()
    m = mesh.material("STEVE")
    for layer, (uv, grow) in enumerate((((40, 16), 0.0), ((40, 32), 0.25))):
        for corners, uvs, out in box_faces((0, 0, 0), (-3, -2, -2), (4, 12, 4), uv, grow, skin, layer):
            mesh.quad([point(c) for c in corners], [(u / skin.width, 1 - v / skin.height) for u, v in uvs],
                      sub(point(add(corners[0], out)), point(corners[0])), m)
    return mesh


def build(minecraft_dir, gothic_dir):
    from make_mc_blocks import BLOCKS
    from make_steve import Gothic, Steve, load_skin
    files = MinecraftFiles(minecraft_dir)
    steve = Steve(Gothic(gothic_dir))
    hand_rot, hand_pos = steve.bind_rot["ZS_RIGHTHAND"], steve.bind_pos["ZS_RIGHTHAND"]
    meshes = os.path.join(gothic_dir, "_work", "DATA", "MESHES", "MC")
    textures = os.path.join(gothic_dir, "_work", "DATA", "TEXTURES", "_COMPILED")
    compiled = os.path.join(gothic_dir, "_work", "DATA", "MESHES", "_COMPILED")
    os.makedirs(meshes, exist_ok=True)
    os.makedirs(textures, exist_ok=True)
    written = []
    pictures = {}
    for texture in [t for _, t, _ in HAND_ITEMS] + FP_ITEMS:
        if texture not in pictures:
            pictures[texture] = picture(files, "item/" + texture)
            write_picture(pictures[texture], os.path.join(textures, "MC_I_" + texture.upper() + "-C.TEX"))
    for name, texture, display in HAND_ITEMS:
        item_mesh(pictures[texture], "MC_I_" + texture.upper(), hand_transform(display, hand_rot, hand_pos)).write(
            os.path.join(meshes, name + ".3DS"), name)
        written.append(name)
    for texture in FP_ITEMS:
        name = "MC_FP_" + texture.upper()
        item_mesh(pictures[texture], "MC_I_" + texture.upper(), view_transform(ITEM_FP)).write(os.path.join(meshes, name + ".3DS"), name)
        written.append(name)
    for block, faces, _, _ in BLOCKS:
        name = "MC_FP_BLK_" + block.upper()
        block_mesh(faces).write(os.path.join(meshes, name + ".3DS"), name)
        written.append(name)
    from PIL import Image
    for frame in range(16):  # the explosion's puff, 4 times larger (nearest)
        puff = picture(files, f"particle/explosion_{frame}")
        big = puff.resize((puff.width * 4, puff.height * 4), Image.NEAREST)
        write_alpha_tex(os.path.join(textures, f"MC_EXPLOSION_{frame}-C.TEX"), big.width, big.height, big.tobytes())
    skin, _ = load_skin(minecraft_dir)
    arm_mesh(skin).write(os.path.join(meshes, "MC_FP_ARM.3DS"), "MC_FP_ARM")
    written.append("MC_FP_ARM")
    image = picture(files, ARROW_TEXTURE)
    write_picture(image, os.path.join(textures, "MC_I_ARROW_ENTITY-C.TEX"))
    arrow_mesh(image, "MC_I_ARROW_ENTITY").write(os.path.join(meshes, ARROW + ".3DS"), ARROW)
    written.append(ARROW)
    for name in written:  # Gothic compiles the new ones
        stale = os.path.join(compiled, name + ".MRM")
        if os.path.exists(stale):
            os.remove(stale)
    print(f"Minecraft's items and first-person hand ({files.version}): {len(written)} meshes")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--minecraft", default=DEFAULT_MINECRAFT, help="the Minecraft folder (%%APPDATA%%\\.minecraft)")
    parser.add_argument("--gothic", required=True, help="the Gothic 1 or Gothic 2 folder")
    args = parser.parse_args()
    build(args.minecraft, args.gothic)


if __name__ == "__main__":
    main()
