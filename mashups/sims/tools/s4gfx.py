"""Read The Sims 4's UI movies (Scaleform GFx, resource type 0x62ECC59A in Data\\Client\\UI.package)
and the pictures they draw: their image atlases and the parts of them each symbol uses.

A movie is a SWF file with Scaleform's own tags (gfxexport); The Sims 4's are uncompressed
("GFX", version 14 to 17). What this reads:
  header    "GFX"|"CFX" (zlib after byte 8), uint8 version, uint32 length, RECT frame size
            (5-bit field width n, then 4 n-bit fields), uint16 frame rate, uint16 frame count
  tags      uint16 code << 6 | length (length 0x3F: a uint32 length follows), then the body
    1000 ExporterInfo        uint16 version, uint32 flags, uint16 bitmap format, uint8 n + the
                             prefix, uint8 n + the movie's name ("timecontrols")
    1009 DefineExternalImage2  uint16 image id, uint16 format, uint16 ?, uint16 width, uint16
                             height, uint8 n + export name, uint8 n + file name
                             ("timecontrols_I3.dds", the atlas)
    1008 DefineSubImage      uint16 id, uint16 image id, uint16 x1, y1, x2, y2: a part of an
                             atlas, used as a bitmap by the shapes' fills
    2, 22, 32, 83 DefineShape(2/3/4)  uint16 id, RECT bounds (Shape4: and edge bounds and a flags
                             byte), the fill styles: uint8 count (0xFF: uint16), each a type
                             byte: 0x00 colour (RGB, Shape3/4 RGBA), 0x10/0x12 gradient
                             (MATRIX, GRADIENT), 0x13 focal gradient (and a FIXED8), 0x40..0x43
                             bitmap (uint16 bitmap id, MATRIX)
    39 DefineSprite          uint16 id, uint16 frame count, then its own tags; its
                             PlaceObject2 (26) and PlaceObject3 (70) tags place other
                             characters (flags, [flags2], depth, [class name], character id)
    76 SymbolClass / 56 ExportAssets  uint16 count, then (uint16 id, NUL-terminated name)
The atlases are _IMG resources (type 0x00B2D882) of UI.package, instance the FNV-1 64-bit hash
of the lower-case file name without its extension ("timecontrols_i3"), in two planes: group
0x00064DCA full size, ATI2 (BC5) with alpha in its first channel and luma (Y) in the second;
group 0x00064DC9 half size, ATI2 with the chroma (Co, Cg, 128 = none). Colour:
R = Y + Co - Cg, G = Y + Cg, B = Y - Co - Cg (s4image.decode_ui_atlas). Some atlases are
group 0 instead, plain 32-bit DDS.

  python s4gfx.py movies ["<The Sims 4 folder>"]
  python s4gfx.py symbols <movie> ["<The Sims 4 folder>"]
  python s4gfx.py sheet <movie> <out.png> ["<The Sims 4 folder>"] [--symbol NAME]

`sheet` draws every part (or a symbol's parts) of a movie's atlases with its id, for finding
pictures; the output is the user's game data: keep it outside the repository.
"""
import argparse
import os
import struct
import sys
import zlib

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
import s4image
from s4package import Package

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))  # mashups/sims/tools -> the repo
GFX_TYPE = 0x62ECC59A
IMG_TYPE = 0x00B2D882
PLANE_FULL, PLANE_HALF = 0x00064DCA, 0x00064DC9
SHAPES = {2: 1, 22: 2, 32: 3, 83: 4}


def fnv64(text):
    """FNV-1 64-bit of the lower-case text, as The Sims 4 names its resources."""
    h = 0xCBF29CE484222325
    for b in text.lower().encode("utf-8"):
        h = (h * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
        h ^= b
    return h


class Bits:
    def __init__(self, data, pos=0):
        self.data, self.pos, self.bit = data, pos, 0

    def ub(self, n):
        v = 0
        for _ in range(n):
            byte = self.data[self.pos]
            v = v << 1 | (byte >> (7 - self.bit)) & 1
            self.bit += 1
            if self.bit == 8:
                self.bit = 0
                self.pos += 1
        return v

    def sb(self, n):
        v = self.ub(n)
        return v - (1 << n) if n and v >> (n - 1) else v

    def align(self):
        if self.bit:
            self.bit = 0
            self.pos += 1
        return self.pos


def skip_rect(data, pos):
    b = Bits(data, pos)
    n = b.ub(5)
    for _ in range(4):
        b.sb(n)
    return b.align()


def skip_matrix(data, pos):
    b = Bits(data, pos)
    if b.ub(1):
        n = b.ub(5)
        b.sb(n)
        b.sb(n)
    if b.ub(1):
        n = b.ub(5)
        b.sb(n)
        b.sb(n)
    n = b.ub(5)
    b.sb(n)
    b.sb(n)
    return b.align()


def shape_bitmaps(code, body):
    """Bitmap ids of a DefineShape's fill styles."""
    version = SHAPES[code]
    pos = skip_rect(body, 2)
    if version == 4:
        pos = skip_rect(body, pos) + 1
    count = body[pos]
    pos += 1
    if count == 0xFF and version >= 2:
        count = struct.unpack_from("<H", body, pos)[0]
        pos += 2
    colour = 4 if version >= 3 else 3
    found = []
    for _ in range(count):
        kind = body[pos]
        pos += 1
        if kind == 0x00:
            pos += colour
        elif kind in (0x10, 0x12, 0x13):
            pos = skip_matrix(body, pos)
            n = body[pos] & 0x0F
            pos += 1 + n * (1 + colour) + (2 if kind == 0x13 else 0)
        elif 0x40 <= kind <= 0x43:
            found.append(struct.unpack_from("<H", body, pos)[0])
            pos = skip_matrix(body, pos + 2)
        else:
            break  # an unknown fill: stop reading this shape
    return found


def tags(data, pos):
    while pos + 2 <= len(data):
        head = struct.unpack_from("<H", data, pos)[0]
        pos += 2
        code, length = head >> 6, head & 0x3F
        if length == 0x3F:
            length = struct.unpack_from("<I", data, pos)[0]
            pos += 4
        yield code, data[pos:pos + length]
        pos += length
        if code == 0:
            break


def _pstr(data, pos):
    n = data[pos]
    return data[pos + 1:pos + 1 + n].decode("latin-1"), pos + 1 + n


class Movie:
    """One GFx movie: its name, atlases ({image id: file name}), parts ({id: (image id, x1, y1, x2,
    y2)}), symbols ({name: id}) and what each character draws or places."""

    def __init__(self, data):
        if data[:3] not in (b"GFX", b"CFX", b"FWS", b"CWS"):
            raise ValueError(f"not a GFx movie (starts {data[:4]!r})")
        if data[:3] in (b"CFX", b"CWS"):
            data = data[:8] + zlib.decompress(data[8:])
        self.version = data[3]
        self.name = None
        self.images = {}    # image id -> (file name, width, height)
        self.parts = {}     # sub-image id -> (image id, x1, y1, x2, y2)
        self.symbols = {}   # class or export name -> character id
        self.children = {}  # character id -> [character ids it places or bitmap ids it fills with]
        pos = skip_rect(data, 8) + 4
        self._read(tags(data, pos))

    def _read(self, stream):
        for code, body in stream:
            if code == 1000 and len(body) > 9:
                _prefix, p = _pstr(body, 8)
                self.name, _ = _pstr(body, p)
            elif code == 1009:
                image, _fmt, _x, w, h = struct.unpack_from("<5H", body, 0)
                _export, p = _pstr(body, 10)
                name, _ = _pstr(body, p)
                self.images[image] = (name, w, h)
            elif code == 1001:
                image, _fmt, w, h = struct.unpack_from("<4H", body, 0)
                _export, p = _pstr(body, 8)
                name, _ = _pstr(body, p)
                self.images[image] = (name, w, h)
            elif code == 1008:
                sid, image, x1, y1, x2, y2 = struct.unpack_from("<6H", body, 0)
                self.parts[sid] = (image, x1, y1, x2, y2)
            elif code in SHAPES:
                try:
                    self.children[struct.unpack_from("<H", body, 0)[0]] = shape_bitmaps(code, body)
                except (IndexError, struct.error):
                    pass
            elif code == 39:
                sprite = struct.unpack_from("<H", body, 0)[0]
                placed = []
                for c, b in tags(body, 4):
                    if c == 26 and b[0] & 0x02:
                        placed.append(struct.unpack_from("<H", b, 3)[0])
                    elif c == 70 and b[0] & 0x02:
                        p = 4
                        if b[1] & 0x08 or (b[1] & 0x10 and b[0] & 0x02):
                            p = b.index(b"\0", p) + 1
                        if p + 2 <= len(b):
                            placed.append(struct.unpack_from("<H", b, p)[0])
                    elif c == 4:
                        placed.append(struct.unpack_from("<H", b, 0)[0])
                self.children[sprite] = list(dict.fromkeys(placed))
            elif code in (56, 76):
                n = struct.unpack_from("<H", body, 0)[0]
                p = 2
                for _ in range(n):
                    cid = struct.unpack_from("<H", body, p)[0]
                    end = body.index(b"\0", p + 2)
                    self.symbols[body[p + 2:end].decode("latin-1")] = cid
                    p = end + 1

    def parts_of(self, character):
        """The atlas parts a character draws, itself and through what it places, in order."""
        found, seen, stack = [], set(), [character]
        while stack:
            c = stack.pop(0)
            if c in seen:
                continue
            seen.add(c)
            if c in self.parts:
                found.append(c)
            stack[0:0] = self.children.get(c, [])
        return found


class UI:
    """The movies and atlases of UI.package."""

    def __init__(self, sims_dir_or_package):
        path = sims_dir_or_package
        if os.path.isdir(path):
            path = os.path.join(path, "Data", "Client", "UI.package")
        self.package = Package(path)
        self._movies = None
        self._atlases = {}

    def movies(self):
        """{movie name: Movie} of every GFx movie."""
        if self._movies is None:
            self._movies = {}
            for e in self.package.find(type=GFX_TYPE):
                try:
                    m = Movie(self.package.read(e))
                except (ValueError, struct.error, IndexError, zlib.error):
                    continue
                m.key = e.key
                self._movies[m.name or str(e)] = m
        return self._movies

    def movie(self, name):
        return self.movies().get(name)

    def atlas(self, file_name):
        """An atlas (its file name in a movie) as a PIL RGBA image, or None if it is not there."""
        if file_name not in self._atlases:
            instance = fnv64(os.path.splitext(file_name)[0])
            full = self.package.get(IMG_TYPE, PLANE_FULL, instance)
            half = self.package.get(IMG_TYPE, PLANE_HALF, instance)
            plain = self.package.get(IMG_TYPE, 0, instance)
            if full and half:
                img = s4image.decode_ui_atlas(self.package.read(full), self.package.read(half))
            elif plain:
                img = s4image.decode(self.package.read(plain))
            else:
                img = None
            self._atlases[file_name] = img
        return self._atlases[file_name]

    def part(self, movie, part_id):
        """One part (DefineSubImage) of a movie's atlas as a PIL RGBA image, or None."""
        m = self.movie(movie) if isinstance(movie, str) else movie
        if m is None or part_id not in m.parts:
            return None
        image, x1, y1, x2, y2 = m.parts[part_id]
        if image not in m.images:
            return None
        atlas = self.atlas(m.images[image][0])
        if atlas is None:
            return None
        return atlas.crop((x1, y1, x2, y2))


def contact_sheet(items, columns=12, cell=96, label=14):
    """[(caption, PIL image)] drawn on a grey sheet, each scaled to fit a cell."""
    from PIL import Image, ImageDraw
    rows = max(1, (len(items) + columns - 1) // columns)
    sheet = Image.new("RGBA", (columns * cell, rows * (cell + label)), (70, 74, 82, 255))
    draw = ImageDraw.Draw(sheet)
    for n, (caption, img) in enumerate(items):
        x, y = n % columns * cell, n // columns * (cell + label)
        if img is not None and img.width and img.height:
            scale = min((cell - 4) / img.width, (cell - 4) / img.height, 2.0)
            thumb = img.resize((max(1, int(img.width * scale)), max(1, int(img.height * scale))))
            sheet.alpha_composite(thumb, (x + (cell - thumb.width) // 2, y + (cell - thumb.height) // 2))
        draw.text((x + 2, y + cell), str(caption)[:16], fill=(255, 255, 255, 255))
    return sheet


def main(argv=None):
    from s4files import DEFAULT_SIMS
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("movies", help="every UI movie with its atlases")
    p.add_argument("sims", nargs="?", default=DEFAULT_SIMS)
    p = sub.add_parser("symbols", help="a movie's symbols and the atlas parts each one draws")
    p.add_argument("movie")
    p.add_argument("sims", nargs="?", default=DEFAULT_SIMS)
    p = sub.add_parser("sheet", help="a contact sheet of a movie's atlas parts")
    p.add_argument("movie")
    p.add_argument("out")
    p.add_argument("sims", nargs="?", default=DEFAULT_SIMS)
    p.add_argument("--symbol", help="only the parts this symbol draws")
    args = parser.parse_args(argv)
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    ui = UI(args.sims)
    if args.command == "movies":
        for name, m in sorted(ui.movies().items()):
            print(f"{name}: {len(m.parts)} parts, {len(m.symbols)} symbols; atlases "
                  + ", ".join(f"{f} {w}x{h}" for f, w, h in m.images.values()))
        return
    m = ui.movie(args.movie)
    if m is None:
        sys.exit(f"no movie {args.movie} in UI.package")
    if args.command == "symbols":
        for name, cid in sorted(m.symbols.items(), key=lambda kv: kv[1]):
            print(f"{cid:5} {name}: parts {m.parts_of(cid)}")
        return
    out = os.path.abspath(args.out)
    if os.path.commonpath([out, REPO]) == REPO:
        sys.exit(f"{out} is inside the repository; game data must stay outside it")
    ids = m.parts_of(m.symbols[args.symbol]) if args.symbol else sorted(m.parts)
    items = []
    for pid in ids:
        img = ui.part(m, pid)
        _image, x1, y1, x2, y2 = m.parts[pid]
        items.append((f"{pid} {x2 - x1}x{y2 - y1}", img))
    contact_sheet(items).save(out)
    print(f"{len(items)} parts -> {out}")


if __name__ == "__main__":
    main()
