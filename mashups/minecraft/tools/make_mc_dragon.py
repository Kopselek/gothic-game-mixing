"""Minecraft's Ender Dragon for the Minecraft plugin: its model's parts as meshes, from the player's jar.

EnderDragonModel's boxes (entity/enderdragon/dragon.png, 256 x 256), each part as its own mesh in
the part's own frame (Minecraft model pixels, y down, as cm: a pixel is 100 / 16 cm), so the
plugin puts each where the model's pose (EnderDragonModel.setupAnim) and the renderer
(EnderDragonRenderer) put it, every frame:
  _work\\DATA\\MESHES\\MC\\MC_DRAGON_<PART>.3DS   HEAD, JAW, SPINE (the neck's 5 and the tail's 12),
                                                BODY, WING_L, WINGTIP_L, WING_R, WINGTIP_R, FRONTLEG,
                                                FRONTLEGTIP, FRONTFOOT, HINDLEG, HINDLEGTIP, HINDFOOT
  _work\\DATA\\TEXTURES\\_COMPILED\\MC_DRAGON-C.TEX     the texture (4 x, nearest), for the boxes
  _work\\DATA\\TEXTURES\\_COMPILED\\MC_DRAGON_A-C.TEX   with its see-through pixels, for the wings' skin
  _work\\DATA\\TEXTURES\\_COMPILED\\MC_DRAGON_FIREBALL-C.TEX  its fireball (a picture facing the camera)
The wings' skin is a flat box: both its sides are drawn.

usage: python make_mc_dragon.py [--minecraft DIR] --gothic DIR
"""
import argparse
import io
import os
import sys

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
from mcfiles import DEFAULT_MINECRAFT, MinecraftFiles
from tex import write_alpha_tex, write_tex

PX = 100 / 16  # cm a model pixel
SCALE = 4
TEXTURE = "entity/enderdragon/dragon"

# part: [(corner, size, texture offset, mirror, skin)] (EnderDragonModel.createBodyLayer; mirror()
# stays on for the boxes after it; "skin" boxes are the wings' flat membranes)
PARTS = {
    "HEAD": [((-6, -1, -24), (12, 5, 16), (176, 44), False, False), ((-8, -8, -10), (16, 16, 16), (112, 30), False, False),
             ((-5, -12, -4), (2, 4, 6), (0, 0), True, False), ((-5, -3, -22), (2, 2, 4), (112, 0), True, False),
             ((3, -12, -4), (2, 4, 6), (0, 0), True, False), ((3, -3, -22), (2, 2, 4), (112, 0), True, False)],
    "JAW": [((-6, 0, -16), (12, 4, 16), (176, 65), False, False)],
    "SPINE": [((-5, -5, -5), (10, 10, 10), (192, 104), False, False), ((-1, -9, -3), (2, 4, 6), (48, 0), False, False)],
    "BODY": [((-12, -16, -16), (24, 24, 64), (0, 0), False, False)]
            + [((-1, -22, z), (2, 6, 12), (220, 53), False, False) for z in (-10, 10, 30)],
    "WING_L": [((0, -4, -4), (56, 8, 8), (112, 88), True, False), ((0, 0, 2), (56, 0, 56), (-56, 88), True, True)],
    "WINGTIP_L": [((0, -2, -2), (56, 4, 4), (112, 136), True, False), ((0, 0, 2), (56, 0, 56), (-56, 144), True, True)],
    "WING_R": [((-56, -4, -4), (56, 8, 8), (112, 88), False, False), ((-56, 0, 2), (56, 0, 56), (-56, 88), False, True)],
    "WINGTIP_R": [((-56, -2, -2), (56, 4, 4), (112, 136), False, False), ((-56, 0, 2), (56, 0, 56), (-56, 144), False, True)],
    "FRONTLEG": [((-4, -4, -4), (8, 24, 8), (112, 104), False, False)],
    "FRONTLEGTIP": [((-3, -1, -3), (6, 24, 6), (226, 138), False, False)],
    "FRONTFOOT": [((-4, 0, -12), (8, 4, 16), (144, 104), False, False)],
    "HINDLEG": [((-8, -4, -8), (16, 32, 16), (0, 0), False, False)],
    "HINDLEGTIP": [((-6, -2, 0), (12, 32, 12), (196, 0), False, False)],
    "HINDFOOT": [((-9, 0, -20), (18, 6, 24), (112, 0), False, False)],
}


def part_mesh(boxes):
    from make_mc_items import Mesh
    from make_steve import box_faces
    mesh = Mesh()
    body, skin = mesh.material("MC_DRAGON"), mesh.material("MC_DRAGON_A")
    for corner, size, uv, mirror, flat in boxes:
        for corners, uvs, out in box_faces((0, 0, 0), corner, size, uv, 0.0, None, 0, mirror):
            pts = [tuple(c * PX for c in p) for p in corners]
            e1 = [pts[1][i] - pts[0][i] for i in range(3)]
            e2 = [pts[3][i] - pts[0][i] for i in range(3)]
            n = (e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0])
            if sum(x * x for x in n) < 1e-6:  # a flat box's sides
                continue
            tex = [(u / 256, 1 - v / 256) for u, v in uvs]
            if flat:  # the membrane: both sides
                mesh.quad(pts, tex, n, skin)
                mesh.quad(pts, tex, (-n[0], -n[1], -n[2]), skin)
            else:
                mesh.quad(pts, tex, out, body)
    return mesh


def build(minecraft_dir, gothic_dir):
    from PIL import Image
    files = MinecraftFiles(minecraft_dir)
    meshes = os.path.join(gothic_dir, "_work", "DATA", "MESHES", "MC")
    textures = os.path.join(gothic_dir, "_work", "DATA", "TEXTURES", "_COMPILED")
    compiled = os.path.join(gothic_dir, "_work", "DATA", "MESHES", "_COMPILED")
    os.makedirs(meshes, exist_ok=True)
    os.makedirs(textures, exist_ok=True)
    skin = Image.open(io.BytesIO(files.read(f"assets/minecraft/textures/{TEXTURE}.png"))).convert("RGBA")
    big = skin.resize((skin.width * SCALE, skin.height * SCALE), Image.NEAREST)
    write_tex(os.path.join(textures, "MC_DRAGON-C.TEX"), big.width, big.height, big.convert("RGB").convert("RGBA").tobytes())
    write_alpha_tex(os.path.join(textures, "MC_DRAGON_A-C.TEX"), big.width, big.height, big.tobytes())
    ball = Image.open(io.BytesIO(files.read("assets/minecraft/textures/entity/enderdragon/dragon_fireball.png"))).convert("RGBA")
    ball = ball.resize((ball.width * 8, ball.height * 8), Image.NEAREST)
    write_alpha_tex(os.path.join(textures, "MC_DRAGON_FIREBALL-C.TEX"), ball.width, ball.height, ball.tobytes())
    for name, boxes in PARTS.items():
        part = "MC_DRAGON_" + name
        part_mesh(boxes).write(os.path.join(meshes, part + ".3DS"), part)
        if os.path.exists(os.path.join(compiled, part + ".MRM")):
            os.remove(os.path.join(compiled, part + ".MRM"))
    print(f"Minecraft's Ender Dragon ({files.version}): {len(PARTS)} parts")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--minecraft", default=DEFAULT_MINECRAFT, help="the Minecraft folder (%%APPDATA%%\\.minecraft)")
    parser.add_argument("--gothic", required=True, help="the Gothic 1 or Gothic 2 folder")
    args = parser.parse_args()
    build(args.minecraft, args.gothic)


if __name__ == "__main__":
    main()
