"""Textures for the GTA San Andreas HUD that the CJMovement plugin draws in Gothic 1.

Writes to _work\\DATA\\TEXTURES\\_COMPILED (DXT3, like Gothic's own HUD textures):
  CJ_HUD_GLYPH_<name>-C.TEX  Pricedown digits, ':' and '$' from GTA's fonts.txd (font1), white with
                             GTA's black text edge (CFont::SetEdge(2)) baked in, so the plugin can
                             tint them with the HUD colours
  CJ_HUD_GLYPH_SUB_<name>    the ammo counter's digits and '-' (CHud::DrawAmmo: FONT_SUBTITLES, which
                             also uses font1, edge 1)
  CJ_HUD_GLYPH_GOTH_<letter> the letters of "wasted" (GXT DEAD) in font2, the FONT_GOTHIC style of
                             CHud::DrawBustedWastedMessage (edge 3)
  CJ_HUD_FIST-C.TEX          the unarmed weapon icon from hud.txd
  CJ_HUD_COLT45-C.TEX        the pistol's icon (colt45icon in gta3.img's colt45.txd)
  CJ_HUD_ROCKETLA-C.TEX      the rocket launcher's icon (rocketlaicon in rocketla.txd)
  CJ_HUD_DILDO-C.TEX         the purple dildo's icon (gun_dildo1icon in gun_dildo1.txd)
  CJ_HUD_SITEROCKET_<corner> the rocket launcher's first-person crosshair: hud.txd siterocket (its
                             top left corner) mirrored into the four corners
  CJ_HUD_SITEM16_<corner>    the aiming crosshair's quarters (hud.txd sitem16; CHud::DrawCrossHairs
                             draws it mirrored into the four corners: TL, TR, BL, BR)
  CJ_HUD_RADARDISC-C.TEX     the radar's rim: hud.txd radardisc, its top left quarter (the plugin
                             draws it mirrored into the four corners and tints it black)
  CJ_HUD_RADAR_CENTRE-C.TEX  CJ's arrow in the middle of the radar (radar_centre)
  CJ_HUD_RADAR_NORTH-C.TEX   the north marker on the radar's rim (radar_north)
  CJ_HUD_WHITE-C.TEX         plain white, tinted for the bars and the radar's blips

Glyph cells: font1 is a 16-column grid of 32x40 px cells (CFont::PrintChar: u = index % 16 / 16,
v = index / 16 / 12.8); Pricedown digits start at index 144, ':' is 154, '$' 93 (data/fonts.dat);
the subtitles style uses the plain letter IDs (character - 32): digits from 16, '-' 13.
Each glyph texture is 64x64 with the cell at (GLYPH_X, GLYPH_Y).

usage: python make_cj_hud.py --gta "D:\\Games\\GTA San Andreas" [--gothic DIR]
"""
import argparse
import os
import sys

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
from img import ImgReader
from tex import write_tex
from txd import read_txd

DEFAULT_GOTHIC = r"C:\Program Files (x86)\Steam\steamapps\common\Gothic"
CELL_W, CELL_H = 32, 40
GLYPH_SIZE, GLYPH_X, GLYPH_Y = 64, 16, 12
EDGE = 3.5  # SetEdge(2) at the HUD's text scale is about 3.5 px of the 40 px cell
SUB_EDGE = 2.8  # SetEdge(1) at the ammo counter's smaller scale (0.7 instead of 1.1 high)
GLYPHS = {str(d): (144 + d, EDGE) for d in range(10)}
GLYPHS.update({"COLON": (154, EDGE), "DOLLAR": (93, EDGE)})
GLYPHS.update({f"SUB_{d}": (16 + d, SUB_EDGE) for d in range(10)})
GLYPHS["SUB_DASH"] = (13, SUB_EDGE)
WASTED = "wasted"  # american.gxt DEAD
GOTHIC_EDGE = 3.0  # SetEdge(3) at the message's 2.1 scale: about 3 px of the 40 px cell


def glyph(font, index, edge_px):
    """64x64 RGBA: the white glyph over a black outline, transparent elsewhere."""
    col, row = index % 16, index // 16
    alpha = [[0.0] * GLYPH_SIZE for _ in range(GLYPH_SIZE)]
    light = [[0.0] * GLYPH_SIZE for _ in range(GLYPH_SIZE)]
    for y in range(CELL_H):
        for x in range(CELL_W):
            o = ((row * CELL_H + y) * font.width + col * CELL_W + x) * 4
            r, g, b, a = font.rgba[o:o + 4]
            alpha[GLYPH_Y + y][GLYPH_X + x] = a / 255
            light[GLYPH_Y + y][GLYPH_X + x] = max(r, g, b) / 255
    reach = int(edge_px) + 1
    out = bytearray(GLYPH_SIZE * GLYPH_SIZE * 4)
    for y in range(GLYPH_SIZE):
        for x in range(GLYPH_SIZE):
            edge = 0.0
            for dy in range(-reach, reach + 1):
                for dx in range(-reach, reach + 1):
                    if dx * dx + dy * dy <= edge_px * edge_px and 0 <= y + dy < GLYPH_SIZE and 0 <= x + dx < GLYPH_SIZE:
                        edge = max(edge, alpha[y + dy][x + dx])
            a = alpha[y][x]
            total = a + edge * (1 - a)
            value = round(255 * light[y][x] * a / total) if total else 0  # white glyph over black edge
            o = (y * GLYPH_SIZE + x) * 4
            out[o:o + 4] = bytes((value, value, value, round(255 * total)))
    return bytes(out)


def mirror(t, flip_x, flip_y):
    out = bytearray(len(t.rgba))
    for y in range(t.height):
        for x in range(t.width):
            sx, sy = t.width - 1 - x if flip_x else x, t.height - 1 - y if flip_y else y
            out[(y * t.width + x) * 4:(y * t.width + x) * 4 + 4] = t.rgba[(sy * t.width + sx) * 4:(sy * t.width + sx) * 4 + 4]
    return bytes(out)


def build(gta_dir, gothic_dir):
    models = os.path.join(gta_dir, "models")
    font = read_txd(open(os.path.join(models, "fonts.txd"), "rb").read())["font1"]
    hud = read_txd(open(os.path.join(models, "hud.txd"), "rb").read())
    out = os.path.join(gothic_dir, "_work", "DATA", "TEXTURES", "_COMPILED")
    os.makedirs(out, exist_ok=True)
    for name, (index, edge) in GLYPHS.items():
        write_tex(os.path.join(out, f"CJ_HUD_GLYPH_{name}-C.TEX"), GLYPH_SIZE, GLYPH_SIZE, glyph(font, index, edge), alpha=True)
    font2 = read_txd(open(os.path.join(models, "fonts.txd"), "rb").read())["font2"]
    for c in sorted(set(WASTED)):  # FONT_GOTHIC: font2, plain letter IDs (character - 32)
        write_tex(os.path.join(out, f"CJ_HUD_GLYPH_GOTH_{c.upper()}-C.TEX"), GLYPH_SIZE, GLYPH_SIZE,
                  glyph(font2, ord(c) - 32, GOTHIC_EDGE), alpha=True)
    gta3 = ImgReader(os.path.join(models, "gta3.img"))
    colt = read_txd(gta3.read("colt45.txd"))["colt45icon"]
    rocket = read_txd(gta3.read("rocketla.txd"))["rocketlaicon"]
    dildo = read_txd(gta3.read("gun_dildo1.txd"))["gun_dildo1icon"]
    for name, t in (("FIST", hud["fist"]), ("COLT45", colt), ("ROCKETLA", rocket), ("DILDO", dildo)):
        write_tex(os.path.join(out, f"CJ_HUD_{name}-C.TEX"), t.width, t.height, t.rgba, alpha=True)
    site = hud["sitem16"]
    for corner, flip_x, flip_y in (("TL", False, True), ("TR", True, True), ("BL", False, False), ("BR", True, False)):
        write_tex(os.path.join(out, f"CJ_HUD_SITEM16_{corner}-C.TEX"), site.width, site.height,
                  mirror(site, flip_x, flip_y), alpha=True)
    site = hud["siterocket"]  # CHud::DrawCrossHairs draws it as is top left, mirrored in the others
    for corner, flip_x, flip_y in (("TL", False, False), ("TR", True, False), ("BL", False, True), ("BR", True, True)):
        write_tex(os.path.join(out, f"CJ_HUD_SITEROCKET_{corner}-C.TEX"), site.width, site.height,
                  mirror(site, flip_x, flip_y), alpha=True)
    for name in ("radardisc", "radar_centre", "radar_north"):
        t = hud[name]
        write_tex(os.path.join(out, f"CJ_HUD_{name.upper()}-C.TEX"), t.width, t.height, t.rgba, alpha=True)
    write_tex(os.path.join(out, "CJ_HUD_WHITE-C.TEX"), 8, 8, b"\xff" * 8 * 8 * 4, alpha=True)
    print(f"HUD: {len(GLYPHS)} glyphs, fist and pistol icons, crosshair, radar sprites, white -> {out}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--gta", required=True, help="GTA San Andreas (classic PC) install folder")
    parser.add_argument("--gothic", default=DEFAULT_GOTHIC, help="Gothic 1 install folder")
    args = parser.parse_args()
    build(args.gta, args.gothic)


if __name__ == "__main__":
    main()
