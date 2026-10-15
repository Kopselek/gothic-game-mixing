"""Minecraft's HUD for the Minecraft plugin: its sprites, item icons and font, from the player's jar.

Writes to _work\\DATA\\TEXTURES\\_COMPILED:
  MC_HUD-C.TEX       one atlas: the HUD's sprites (gui/sprites/hud: hotbar, selection, hearts,
                     armour, food, air, experience bar, crosshair and attack indicators, effect
                     background), the item icons the hotbar shows for Gothic's items (textures/item),
                     the speed effect's icon, and the font's glyphs; every pixel 4 x 4 (nearest), so
                     the plugin's point-sampled drawing keeps Minecraft's pixels and DXT3 keeps their
                     colours (each 4 x 4 block is one pixel)
  MC_VIGNETTE-C.TEX  the vignette (textures/misc/vignette.png) as an alpha mask
  MC_HUD.TXT         where everything is in the atlas: "sprite <name> x y w h", "glyph <byte> x y w h
                     <advance> <top>" (advance and top in Minecraft's GUI pixels; top = 7 - the
                     provider's ascent), "space <byte> <advance>"; the bytes are the game's code page
                     (Gothic's texts are 8-bit: 1250 for the Polish game, 1251 Russian, 1252 else),
                     guessed from the letters in Gothic.dat. Then the blocks of make_mc_blocks.py
                     and the spawn eggs: "block <n> <id> <hardness> <blast resistance> <sound> <name>", "egg <n> <id> <name>",
                     their names Minecraft's in the game's language (Polish, German, Russian or
                     English by the code page and the letters)
  MC_ICONS-C.TEX     the blocks' inventory icons as Minecraft draws a block item (its top and two
                     sides at an angle: the gui display's 30 and 225 degrees, scaled 0.625, the
                     sides darker), each drawn for the HUD's scales 1 to 4 (16 x 16 GUI pixels at
                     16, 32, 48 or 64 screen pixels, each pixel its face's nearest texel, as the
                     game renders them), every pixel 4 x 4 as in MC_HUD-C.TEX; in the table
                     "icon <block> <scale> x y w h"
The atlas also has the spawn eggs, the bow and the arrow, the creative inventory's panel
(creative_inventory/tab_items.png) and a white pixel for plain shapes. The table also names
Minecraft's items the plugin brings: "item <n> <id> <name>".

Glyphs as Minecraft's BitmapProvider makes them: a file's cells are its width / 16 wide and its
height / rows high; a glyph is as wide as its last column with any pixel, drawn at the provider's
height (8 unless given) and advancing (width x scale + 0.5) + 1; the providers of
font/include/default.json in order (the first one with a character wins), the space's advance
from font/include/space.json.

usage: python make_mc_hud.py [--minecraft DIR] --gothic DIR
"""
import argparse
import io
import json
import math
import os
import struct
import sys

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
from mcfiles import DEFAULT_MINECRAFT, MinecraftFiles
from tex import FORMAT_DXT3, _encode_dxt3_numpy, _numpy

SCALE = 4
ATLAS_WIDTH = 1024
ICONS_WIDTH = 2048
ICON_SCALES = (1, 2, 3, 4)
PAD = 2  # transparent pixels (at Minecraft's size) around each picture

SPRITES = [
    "hotbar", "hotbar_selection", "hotbar_offhand_left", "crosshair",
    "crosshair_attack_indicator_full", "crosshair_attack_indicator_background", "crosshair_attack_indicator_progress",
    "hotbar_attack_indicator_background", "hotbar_attack_indicator_progress",
    "experience_bar_background", "experience_bar_progress",
    "armor_empty", "armor_half", "armor_full",
    "food_empty", "food_half", "food_full",
    "air", "air_bursting", "air_empty",
    "heart/container", "heart/container_blinking", "heart/full", "heart/full_blinking", "heart/half",
    "heart/half_blinking", "effect_background",
]
# textures/item (and two of textures/block, mob_effect) the hotbar draws for Gothic's items
ITEMS = [
    "wooden_sword", "stone_sword", "iron_sword", "golden_sword", "diamond_sword", "netherite_sword",
    "wooden_axe", "stone_axe", "iron_axe", "golden_axe", "diamond_axe", "netherite_axe",
    "bow", "crossbow_standby", "arrow", "enchanted_book", "paper", "potion", "potion_overlay",
    "apple", "bread", "beef", "cooked_beef", "cod", "cooked_cod", "mushroom_stew", "honey_bottle",
    "sweet_berries", "carrot", "cookie", "stick", "book", "glass_bottle", "red_mushroom", "rabbit_stew",
]
EXTRA = {"torch": "assets/minecraft/textures/block/torch.png",
         "red_mushroom": "assets/minecraft/textures/block/red_mushroom.png",
         "effect_speed": "assets/minecraft/textures/mob_effect/speed.png"}


def image(files, path):
    from PIL import Image
    return Image.open(io.BytesIO(files.read(path))).convert("RGBA")


def code_page(gothic_dir):
    """The code page of Gothic's texts, from the letters of the string constants in Gothic.dat."""
    from daedalus import Script
    path = os.path.join(gothic_dir, "_work", "DATA", "scripts", "_compiled", "Gothic.dat")
    script = Script(open(path + ".orig" if os.path.exists(path + ".orig") else path, "rb").read())
    count = [0] * 256
    for symbol in script.symbols:
        if isinstance(symbol.content, list):  # a string's (an int's content is bytes)
            for text in symbol.content:
                for b in text:
                    count[b] += 1
    polish = sum(count[b] for b in (0xB9, 0xEA, 0xB3, 0x9C, 0xBF, 0x9F, 0xE6, 0xF1, 0xA5, 0xCA, 0xA3, 0x8C, 0xAF, 0x8F, 0xC6, 0xD1))
    german = sum(count[b] for b in (0xE4, 0xF6, 0xFC, 0xDF, 0xC4, 0xD6, 0xDC))
    cyrillic = sum(count[0xC0:0x100])
    if cyrillic > 3 * (polish + german) and cyrillic > 1000:
        return "cp1251"
    return "cp1250" if polish > german else "cp1252"


def glyphs(files, page):
    """{byte: (RGBA image of the glyph, advance, top)} for the code page's printable bytes, and
    {byte: advance} of the spaces."""
    space = json.loads(files.read("assets/minecraft/font/include/space.json"))["providers"][0]["advances"]
    providers = json.loads(files.read("assets/minecraft/font/include/default.json"))["providers"]
    sheets = []
    for p in providers:
        if p.get("type") != "bitmap":
            continue
        name = p["file"].split(":", 1)[-1]
        sheet = image(files, f"assets/minecraft/textures/{name}")
        rows = p["chars"]
        cell_w, cell_h = sheet.width // len(rows[0]), sheet.height // len(rows)
        height = p.get("height", 8)
        cells = {}
        for r, row in enumerate(rows):
            for c, ch in enumerate(row):
                if ch != "\0" and ch not in cells:
                    cells[ch] = (c * cell_w, r * cell_h)
        sheets.append((sheet, cells, cell_w, cell_h, height, p.get("ascent", 7)))
    out, spaces = {}, {}
    for byte in range(0x20, 0x100):
        try:
            ch = bytes([byte]).decode(page)
        except UnicodeDecodeError:
            continue
        if ch in space:
            spaces[byte] = space[ch]
            continue
        for sheet, cells, cell_w, cell_h, height, ascent in sheets:
            if ch not in cells:
                continue
            x0, y0 = cells[ch]
            cell = sheet.crop((x0, y0, x0 + cell_w, y0 + cell_h))
            alpha = cell.getchannel("A")
            width = 0
            for x in range(cell_w):
                if alpha.crop((x, 0, x + 1, cell_h)).getextrema()[1] > 0:
                    width = x + 1
            if width == 0:
                break
            scale = height / cell_h
            out[byte] = (cell.crop((0, 0, width, cell_h)), int(width * scale + 0.5) + 1, 7 - ascent, scale)
            break
    return out, spaces


def pack(pictures, width=ATLAS_WIDTH):
    """Shelf packing at SCALE: {key: (x, y, w, h)} in atlas pixels, and the atlas height."""
    order = sorted(pictures, key=lambda k: -pictures[k].height)
    x = y = shelf = 0
    places = {}
    for key in order:
        w, h = (pictures[key].width + 2 * PAD) * SCALE, (pictures[key].height + 2 * PAD) * SCALE
        if x + w > width:
            x, y, shelf = 0, y + shelf, 0
        places[key] = (x + PAD * SCALE, y + PAD * SCALE, pictures[key].width * SCALE, pictures[key].height * SCALE)
        x += w
        shelf = max(shelf, h)
    height = 1
    while height < y + shelf:
        height *= 2
    return places, height


def write_dxt3(path, w, h, rgba):
    np = _numpy()
    if np is None:
        raise RuntimeError("make_mc_hud.py needs numpy")
    header = b"ZTEX" + struct.pack("<8I", 0, FORMAT_DXT3, w, h, 1, w, h, 0xFF808080)
    with open(path, "wb") as f:
        f.write(header + _encode_dxt3_numpy(np, w, h, rgba))


EGGS = ["zombie", "skeleton", "creeper", "ender_dragon"]
MC_ITEMS = ["bow", "arrow", "flint_and_steel", "lava_bucket", "bucket"]  # Minecraft's items the plugin brings (make_mc_items.py)


def block_icon(files, faces, n):
    """A block's inventory icon, n x n pixels for 16 x 16 GUI pixels: Minecraft's block item in the
    GUI (the block model's gui display: turned 30 degrees about x and 225 about y, scaled 0.625,
    drawn without perspective), its top full bright, the south side on the left 0.8, the east on
    the right 0.6 (the GUI light); each pixel the nearest texel of the face its centre falls on."""
    from PIL import Image
    from make_mc_blocks import face_image
    top, south, east = (face_image(files, spec) for spec in (faces[0], faces[4], faces[3]))
    out = Image.new("RGBA", (n, n), (0, 0, 0, 0))
    edge = 0.625 * n  # the cube's edge on the screen
    w = edge * math.sqrt(2) / 2   # half the width: the top's diagonal
    h = w * math.sin(math.radians(30))  # half the top's height
    s = edge * math.cos(math.radians(30))  # a side's height
    cx, ty = n / 2, (n - (2 * h + s)) / 2
    T0, T1, T2, T3 = (cx, ty), (cx + w, ty + h), (cx, ty + 2 * h), (cx - w, ty + h)
    B2, B3 = (T2[0], T2[1] + s), (T3[0], T3[1] + s)
    pixels = out.load()
    for picture, origin, u_end, v_end, light in ((top, T3, T0, T2, 1.0), (south, T3, T2, B3, 0.8), (east, T2, T1, B2, 0.6)):
        src = picture.load()
        ux, uy = u_end[0] - origin[0], u_end[1] - origin[1]
        vx, vy = v_end[0] - origin[0], v_end[1] - origin[1]
        det = ux * vy - uy * vx
        for y in range(n):
            for x in range(n):
                px, py = x + 0.5 - origin[0], y + 0.5 - origin[1]
                u = (px * vy - py * vx) / det
                v = (ux * py - uy * px) / det
                if 0 <= u < 1 and 0 <= v < 1:
                    r, g, b, a = src[int(u * 16), int(v * 16)]
                    if a > 0:
                        pixels[x, y] = (int(r * light + 0.5), int(g * light + 0.5), int(b * light + 0.5), a)
    return out


def language(files, page, gothic_dir):
    """Minecraft's language for the game's: Polish (1250), Russian (1251), German or English (1252)."""
    code = {"cp1250": "pl_pl", "cp1251": "ru_ru"}.get(page)
    if not code:
        from daedalus import Script
        path = os.path.join(gothic_dir, "_work", "DATA", "scripts", "_compiled", "Gothic.dat")
        script = Script(open(path + ".orig" if os.path.exists(path + ".orig") else path, "rb").read())
        text = b" ".join(t for sym in script.symbols if isinstance(sym.content, list) for t in sym.content)
        code = "de_de" if text.count(b" und ") > text.count(b" and ") else "en_us"
    try:
        return files.language(code)
    except (KeyError, OSError):
        return files.language("en_us")


def build(minecraft_dir, gothic_dir):
    from PIL import Image
    from make_mc_blocks import BLOCKS, RESISTANCE, SOUND
    files = MinecraftFiles(minecraft_dir)
    pictures = {f"sprite {name}": image(files, f"assets/minecraft/textures/gui/sprites/hud/{name}.png") for name in SPRITES}
    for name in ITEMS:
        path = EXTRA.get(name, f"assets/minecraft/textures/item/{name}.png")
        pictures[f"sprite item/{name}"] = image(files, path)
    pictures["sprite item/torch"] = image(files, EXTRA["torch"])
    pictures["sprite effect/speed"] = image(files, EXTRA["effect_speed"])
    for bar in ("pink_background", "pink_progress"):  # the Ender Dragon's boss bar (BossHealthOverlay)
        pictures[f"sprite boss_bar/{bar}"] = image(files, f"assets/minecraft/textures/gui/sprites/boss_bar/{bar}.png")
    for egg in EGGS:
        pictures[f"sprite item/{egg}_spawn_egg"] = image(files, f"assets/minecraft/textures/item/{egg}_spawn_egg.png")
    for item in MC_ITEMS:
        pictures.setdefault(f"sprite item/{item}", image(files, f"assets/minecraft/textures/item/{item}.png"))
    tab = image(files, "assets/minecraft/textures/gui/container/creative_inventory/tab_items.png")
    pictures["sprite creative/tab_items"] = tab.crop((0, 0, 195, 136))
    pictures["sprite white"] = Image.new("RGBA", (1, 1), (255, 255, 255, 255))
    page = code_page(gothic_dir)
    font, spaces = glyphs(files, page)
    for byte, (glyph, _, _, _) in font.items():
        pictures[f"glyph {byte}"] = glyph
    places, height = pack(pictures)
    atlas = Image.new("RGBA", (ATLAS_WIDTH, height), (0, 0, 0, 0))
    for key, picture in pictures.items():
        x, y, w, h = places[key]
        atlas.paste(picture.resize((w, h), Image.NEAREST), (x, y))
    out = os.path.join(gothic_dir, "_work", "DATA", "TEXTURES", "_COMPILED")
    os.makedirs(out, exist_ok=True)
    write_dxt3(os.path.join(out, "MC_HUD-C.TEX"), ATLAS_WIDTH, height, atlas.tobytes())
    icons = {f"{block} {k}": block_icon(files, faces, 16 * k) for block, faces, _, _ in BLOCKS for k in ICON_SCALES}
    icon_places, icons_height = pack(icons, ICONS_WIDTH)
    sheet = Image.new("RGBA", (ICONS_WIDTH, icons_height), (0, 0, 0, 0))
    for key, picture in icons.items():
        x, y, w, h = icon_places[key]
        sheet.paste(picture.resize((w, h), Image.NEAREST), (x, y))
    write_dxt3(os.path.join(out, "MC_ICONS-C.TEX"), ICONS_WIDTH, icons_height, sheet.tobytes())
    vignette = image(files, "assets/minecraft/textures/misc/vignette.png")
    mask = Image.new("RGBA", vignette.size, (0, 0, 0, 0))
    mask.putalpha(vignette.convert("L"))
    write_dxt3(os.path.join(out, "MC_VIGNETTE-C.TEX"), mask.width, mask.height, mask.tobytes())
    lines = [f"atlas {ATLAS_WIDTH} {height} {SCALE}", f"codepage {page[2:]}"]
    for key, (x, y, w, h) in sorted(places.items()):
        if key.startswith("sprite "):
            lines.append(f"{key} {x} {y} {w} {h}")
    for byte, (_, advance, top, scale) in sorted(font.items()):
        x, y, w, h = places[f"glyph {byte}"]
        lines.append(f"glyph {byte} {x} {y} {w} {h} {advance} {top} {scale:g}")
    for byte, advance in sorted(spaces.items()):
        lines.append(f"space {byte} {advance}")
    lines.append(f"icons {ICONS_WIDTH} {icons_height}")
    for key, (x, y, w, h) in sorted(icon_places.items()):
        lines.append(f"icon {key} {x} {y} {w} {h}")
    names = language(files, page, gothic_dir)

    def name(key, fallback):
        return names.get(key, fallback).encode(page, "replace").decode("latin-1")
    for i, (block, _, _, hardness) in enumerate(BLOCKS):
        lines.append(f"block {i} {block} {hardness:g} {RESISTANCE.get(block, hardness):g} {SOUND.get(block, 'stone')} "
                     + name(f"block.minecraft.{block}", block))
    for i, egg in enumerate(EGGS):
        lines.append(f"egg {i} {egg} " + name(f"item.minecraft.{egg}_spawn_egg", egg))
    for i, item in enumerate(MC_ITEMS):
        lines.append(f"item {i} {item} " + name(f"item.minecraft.{item}", item))
    lines.append("title " + name("itemGroup.buildingBlocks", "Building Blocks"))
    lines.append("boss " + name("entity.minecraft.ender_dragon", "Ender Dragon"))
    with open(os.path.join(out, "MC_HUD.TXT"), "w", newline="\r\n", encoding="latin-1") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Minecraft's HUD ({files.version}): {len(SPRITES)} sprites, {len(ITEMS) + 2} icons, {len(font)} glyphs "
          f"({page}), atlas {ATLAS_WIDTH} x {height}; {len(BLOCKS)} blocks' icons, {ICONS_WIDTH} x {icons_height}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--minecraft", default=DEFAULT_MINECRAFT, help="the Minecraft folder (%%APPDATA%%\\.minecraft)")
    parser.add_argument("--gothic", required=True, help="the Gothic 1 or Gothic 2 folder")
    args = parser.parse_args()
    build(args.minecraft, args.gothic)


if __name__ == "__main__":
    main()
