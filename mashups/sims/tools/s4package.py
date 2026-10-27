"""Read The Sims 4's .package files (DBPF 2.1), where the game keeps every resource.

The Sims 4 keeps its images, sounds, models, UI (Scaleform GFx) and tuning in .package files under
Data\\Client, Data\\Simulation and Delta\\<pack> (s4files.py lists them in load order). A resource is
known by its key: type (uint32), group (uint32) and instance (uint64), written
TTTTTTTT:GGGGGGGG:IIIIIIIIIIIIIIII as the community's tools (s4pi / Sims4Tools) write it.

Layout (s4pi, s4pi/Package/Package.cs and PackageIndex.cs; the Mod The Sims wiki's "DBPF"
page; checked against the game's own packages, 1.128.90.1030: 2.58 million entries in 629
packages, all of them DBPF 2.1 and read here):
  header, 96 bytes, little-endian:
    0x00 "DBPF", 0x04 uint32 major version 2, 0x08 uint32 minor version 1, 0x0C..0x23 user
    version, flags and dates (zero), 0x24 uint32 index entry count,
    0x28 uint32 index position (version 1's; zero in 2.x), 0x2C uint32 index size,
    0x30..0x3B hole index (zero), 0x3C uint32 index version (3), 0x40 uint64 index position,
    0x48..0x5F zero
  index at its position, `size` bytes:
    uint32 flags: bit 0 every entry has the same type, bit 1 the same group, bit 2 the same
           instance-high; the shared values follow, one uint32 each in that order
    then per entry the fields not shared:
      uint32 type, uint32 group, uint32 instance high (each only when not shared),
      uint32 instance low, uint32 offset (from the file's start),
      uint32 file size (bit 31 set: the entry has the two uint16 fields below, always so in
             The Sims 4), uint32 memory size (unpacked),
      uint16 compression, uint16 committed (1)
  compression: 0x0000 none, 0x5A42 zlib (a zlib stream with its header; nearly everything),
    0xFFFF RefPack ("internal compression", EA's LZ77 also known as QFS: a 0x10FB or 0x50FB header,
    below; the string tables and two small others), 0xFFFE streamable (not met in the game's files;
    read as RefPack or zlib by its first bytes), 0xFFE0 a deleted resource (no data: an earlier
    package's copy is removed; the deltas have many)

RefPack: byte 0 flags (0x80: sizes are 4 bytes, else 3; 0x01: a compressed size comes first;
0x10 and 0x40 are seen and mean nothing here),
byte 1 0xFB, then the unpacked size big-endian; then commands, each some literal bytes copied
from the input and then a copy of earlier output:
    0x00-0x7F  b0 b1        literal b0&3, copy (b0>>2&7)+3 from ((b0&0x60)<<3)+b1+1 back
    0x80-0xBF  b0 b1 b2     literal b1>>6, copy (b0&0x3F)+4 from ((b1&0x3F)<<8)+b2+1 back
    0xC0-0xDF  b0 b1 b2 b3  literal b0&3, copy ((b0&0x0C)<<6)+b3+5 from ((b0&0x10)<<12)+(b1<<8)+b2+1 back
    0xE0-0xFB  b0           literal ((b0&0x1F)<<2)+4, no copy
    0xFC-0xFF  b0           literal b0&3, then the end

  python s4package.py list <package or folder> [--type 0x2F7D0004] [--grep 0123abcd]
  python s4package.py types <package or folder>
  python s4package.py dump <package or folder> <folder outside this repo> [--type 0x...] [--grep ...]

A folder means every .package under it. Dumped files are named TTTTTTTT_GGGGGGGG_IIIIIIIIIIIIIIII
with an extension from what they hold (png, dds, rle, gfx, xml, ...). They are the user's game
data: keep them outside the repository (dump refuses a folder inside it).
"""
import argparse
import os
import struct
import sys
import zlib

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
import numpy as np

MAGIC = b"DBPF"
HEADER_SIZE = 96
REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))  # mashups/sims/tools -> the repo

NONE, ZLIB, REFPACK, STREAMABLE, DELETED = 0x0000, 0x5A42, 0xFFFF, 0xFFFE, 0xFFE0
COMPRESSION = {NONE: "none", ZLIB: "zlib", REFPACK: "refpack", STREAMABLE: "stream", DELETED: "deleted"}

# Resource types met in the game's packages (names as the community's tools give them)
TYPES = {
    0x00B2D882: "_IMG",   # DDS, often DST1/DST5 (shuffled DXT1/DXT5): s4image.py
    0x2F7D0004: "PNG",    # PNG images: the interaction and UI icons the tuning names
    0x2E75C764: "ICON",   # PNG icons (also 0x2E75C765..0x2E75C767)
    0x2E75C765: "ICON",
    0x2E75C766: "ICON",
    0x2E75C767: "ICON",
    0x3453CF95: "RLE2",   # DXT5 run-length (RLE2 and RLES): s4image.py
    0xBA856C78: "RLES",
    0x2BC04EDF: "LRLE",   # lossless run-length (LRLE): s4image.py
    0x3C1AF1F2: "THUM",   # thumbnails (PNG or JFIF with an ALFA chunk)
    0x5B282D45: "THUM",
    0xCD9DE247: "THUM",
    0x0D338A3A: "THUM",
    0x16CCF748: "THUM",
    0x3C2A8647: "THUM",
    0x9C925813: "THUM",
    0x62ECC59A: "GFX",    # Scaleform GFx movies (the UI)
    0x220557DA: "STBL",   # string tables
    0x01A527DB: "SNR",    # EA audio (SNR header and data)
    0x01EEF63A: "SNS",    # EA audio (streamed)
    0xFD04E3BE: "PROPX",  # audio properties
    0x01661233: "MODL",
    0x01D10F34: "MLOD",
    0x015A1849: "GEOM",
    0x01D0E75D: "MATD",
    0x02019972: "MTST",
    0xD3044521: "RSLT",
    0x736884F1: "VPXY",
    0xC0DB5AE7: "OBJD",
    0x319E4F1D: "COBJ",
    0x034AEECB: "CASP",
    0x545AC67A: "DATA",   # SimData
    0x0166038C: "NMAP",   # name map (instance -> name)
    0x8EAF13DE: "RIG",
    0x6B20C4F3: "CLIP",
    0xBC4A5044: "CLHD",
    0x02D5DF13: "JAZZ",
    0x62E94D38: "CTUN",   # combined tuning
    0x03B33DDF: "TUNE",   # tuning (XML)
    0xE882D22F: "TUNE",   # interaction tuning (XML)
    0x6017E896: "TUNE",   # buff tuning (XML)
    0xD382BF57: "FTPT",
    0xAC16FBEC: "RMAP",
}
EXTENSIONS = [  # first bytes -> extension of a dumped resource
    (b"\x89PNG", "png"), (b"DDS ", "dds"), (b"\xff\xd8", "jpg"), (b"DXT5RLE", "rle"), (b"LRLE", "lrle"),
    (b"GFX", "gfx"), (b"CFX", "gfx"), (b"FWS", "swf"), (b"CWS", "swf"), (b"<?xml", "xml"), (b"STBL", "stbl"),
    (b"\x00\x01\x00\x00", "ttf"), (b"OTTO", "otf"), (b"true", "ttf"), (b"ttcf", "ttc"), (b"RIFF", "riff"),
    (b"OggS", "ogg"), (b"DBPF", "package"), (b"MODL", "modl"), (b"MLOD", "mlod"), (b"GEOM", "geom"),
    (b"RCOL", "rcol"),
]


def key_text(t, g, i):
    return f"{t:08X}:{g:08X}:{i:016X}"


def parse_key(text):
    """(type, group, instance) from 'TTTTTTTT:GGGGGGGG:IIIIIIIIIIIIIIII' (also with '-' or '_')."""
    parts = text.replace("-", ":").replace("_", ":").split(":")
    if len(parts) != 3:
        raise ValueError(f"{text!r} is not a resource key (type:group:instance in hex)")
    return int(parts[0], 16), int(parts[1], 16), int(parts[2], 16)


class Entry:
    """One resource of a package: its key, where it is and how it is packed."""
    __slots__ = ("package", "type", "group", "instance", "offset", "file_size", "mem_size", "compression",
                 "committed")

    def __init__(self, package, t, g, i, offset, file_size, mem_size, compression, committed):
        self.package, self.type, self.group, self.instance = package, t, g, i
        self.offset, self.file_size, self.mem_size = offset, file_size, mem_size
        self.compression, self.committed = compression, committed

    @property
    def key(self):
        return self.type, self.group, self.instance

    @property
    def deleted(self):
        return self.compression == DELETED

    def __str__(self):
        return key_text(*self.key)

    def __repr__(self):
        return f"<{self} {COMPRESSION.get(self.compression, hex(self.compression))} {self.mem_size}>"


class Package:
    """A .package file: `entries` in index order, `read(entry)` the unpacked bytes of one.
    `types`: keep only the entries of these resource types (faster and smaller for big packages)."""

    def __init__(self, path, types=None):
        self.path = path
        size = os.path.getsize(path)
        with open(path, "rb") as f:
            head = f.read(HEADER_SIZE)
            if len(head) < HEADER_SIZE or head[:4] != MAGIC:
                raise ValueError(f"{path}: not a DBPF package (magic {head[:4]!r})")
            self.version = struct.unpack_from("<II", head, 4)
            if self.version[0] != 2:
                raise ValueError(f"{path}: DBPF {self.version[0]}.{self.version[1]}, not The Sims 4's 2.x")
            count, pos_v1, index_size = struct.unpack_from("<III", head, 0x24)
            pos = struct.unpack_from("<Q", head, 0x40)[0] or pos_v1
            if count and not (HEADER_SIZE <= pos and pos + index_size <= size):
                raise ValueError(f"{path}: index at {pos:#x} + {index_size} is outside the file ({size} bytes)")
            f.seek(pos)
            index = f.read(index_size) if count else b""
        self.count = count
        self.entries = self._parse_index(index, count, size, types)
        self._by_key = None

    def _parse_index(self, index, count, file_size, types):
        if not count:
            return []
        flags = struct.unpack_from("<I", index, 0)[0]
        pos = 4
        shared = {}
        for bit, name in enumerate(("t", "g", "hi")):
            if flags & (1 << bit):
                shared[name] = struct.unpack_from("<I", index, pos)[0]
                pos += 4
        fields = [(n, "<u4") for n in ("t", "g", "hi") if n not in shared]
        fields += [("lo", "<u4"), ("off", "<u4"), ("fsize", "<u4"), ("msize", "<u4"), ("comp", "<u2"),
                   ("committed", "<u2")]
        row = np.dtype(fields)
        if pos + count * row.itemsize > len(index):
            raise ValueError(f"{self.path}: {count} index entries of {row.itemsize} bytes do not fit the index "
                             f"({len(index)} bytes)")
        table = np.frombuffer(index, row, count, pos)
        column = {n: (table[n] if n not in shared else np.full(count, shared[n], np.uint32))
                  for n in ("t", "g", "hi")}
        if not (table["fsize"] & 0x80000000).all():  # no extended compression info: the two uint16 are not there
            raise ValueError(f"{self.path}: entries without the extended compression fields (bit 31 of the file "
                             "size); not a package of The Sims 4")
        fsize = table["fsize"] & 0x7FFFFFFF
        live = table["comp"] != DELETED
        outside = live & (table["off"].astype(np.uint64) + fsize > file_size)
        if outside.any():
            n = int(np.nonzero(outside)[0][0])
            raise ValueError(f"{self.path}: entry {n} lies outside the file")
        rows = np.arange(count)
        if types is not None:
            rows = rows[np.isin(column["t"], np.array(sorted(types), np.uint32))]
        t, g, hi, lo = (column["t"][rows].tolist(), column["g"][rows].tolist(), column["hi"][rows].tolist(),
                        table["lo"][rows].tolist())
        off, fs, ms = table["off"][rows].tolist(), fsize[rows].tolist(), table["msize"][rows].tolist()
        comp, com = table["comp"][rows].tolist(), table["committed"][rows].tolist()
        return [Entry(self, t[k], g[k], hi[k] << 32 | lo[k], off[k], fs[k], ms[k], comp[k], com[k])
                for k in range(len(rows))]

    def find(self, type=None, group=None, instance=None):
        """Entries matching the given parts of a key (deleted ones left out)."""
        return [e for e in self.entries if not e.deleted and (type is None or e.type == type)
                and (group is None or e.group == group) and (instance is None or e.instance == instance)]

    def get(self, t, g, i):
        """The entry with this key, or None."""
        if self._by_key is None:
            self._by_key = {e.key: e for e in self.entries}
        return self._by_key.get((t, g, i))

    def read_raw(self, entry):
        with open(self.path, "rb") as f:
            f.seek(entry.offset)
            return f.read(entry.file_size)

    def peek(self, entry, n=128):
        """The first `n` unpacked bytes of a resource (a header), without unpacking all of it."""
        if entry.compression == ZLIB:
            with open(self.path, "rb") as f:
                f.seek(entry.offset)
                z = zlib.decompressobj()
                out = b""
                left = entry.file_size
                while len(out) < n and left > 0 and not z.eof:
                    chunk = f.read(min(4096, left))
                    left -= len(chunk)
                    out += z.decompress(chunk, n - len(out))
                    while z.unconsumed_tail and len(out) < n:
                        out += z.decompress(z.unconsumed_tail, n - len(out))
                return out[:n]
        if entry.compression == NONE:
            with open(self.path, "rb") as f:
                f.seek(entry.offset)
                return f.read(min(n, entry.file_size))
        return self.read(entry)[:n]

    def read(self, entry):
        """The unpacked bytes of one resource (an Entry of this package, or a key tuple)."""
        if not isinstance(entry, Entry):
            found = self.get(*entry)
            if found is None:
                raise KeyError(f"{key_text(*entry)}: not in {self.path}")
            entry = found
        if entry.deleted:
            raise KeyError(f"{entry}: deleted in {self.path}")
        raw = self.read_raw(entry)
        data = unpack(raw, entry.compression, entry.mem_size)
        if len(data) != entry.mem_size:
            raise ValueError(f"{entry} in {self.path}: unpacked {len(data)} bytes, expected {entry.mem_size}")
        return data


def unpack(raw, compression, size):
    if compression == NONE:
        return raw
    if compression == ZLIB:
        return zlib.decompress(raw)
    if compression in (REFPACK, STREAMABLE):
        if len(raw) >= 2 and raw[1] == 0xFB:
            return refpack(raw)
        if raw[:1] == b"\x78":  # zlib marked as RefPack (some tools write this)
            return zlib.decompress(raw)
        if len(raw) == size:
            return raw
    raise NotImplementedError(f"compression {compression:#06x} ({COMPRESSION.get(compression, '?')}) "
                              f"with data starting {raw[:4].hex()} is not supported")


def refpack(src):
    """Unpack EA's RefPack (QFS): see the module's docstring."""
    flags = src[0]
    if src[1] != 0xFB:
        raise ValueError(f"RefPack: bad header {src[:2].hex()}")
    width = 4 if flags & 0x80 else 3
    i = 2 + (width if flags & 0x01 else 0)
    size = int.from_bytes(src[i:i + width], "big")
    i += width
    dst = bytearray()
    n = len(src)
    while i < n:
        b0 = src[i]
        if b0 < 0x80:
            b1 = src[i + 1]
            i += 2
            literal = b0 & 3
            length = ((b0 >> 2) & 7) + 3
            distance = ((b0 & 0x60) << 3) + b1 + 1
        elif b0 < 0xC0:
            b1, b2 = src[i + 1], src[i + 2]
            i += 3
            literal = b1 >> 6
            length = (b0 & 0x3F) + 4
            distance = ((b1 & 0x3F) << 8) + b2 + 1
        elif b0 < 0xE0:
            b1, b2, b3 = src[i + 1], src[i + 2], src[i + 3]
            i += 4
            literal = b0 & 3
            length = ((b0 & 0x0C) << 6) + b3 + 5
            distance = ((b0 & 0x10) << 12) + (b1 << 8) + b2 + 1
        elif b0 < 0xFC:
            i += 1
            literal = ((b0 & 0x1F) << 2) + 4
            dst += src[i:i + literal]
            i += literal
            continue
        else:
            i += 1
            literal = b0 & 3
            dst += src[i:i + literal]
            break
        if literal:
            dst += src[i:i + literal]
            i += literal
        start = len(dst) - distance
        if start < 0:
            raise ValueError(f"RefPack: copy from {distance} back at output {len(dst)}")
        if distance >= length:
            dst += dst[start:start + length]
        else:  # the copy overlaps what it writes: it repeats the last `distance` bytes
            pattern = bytes(dst[start:])
            count, rest = divmod(length, distance)
            dst += pattern * count + pattern[:rest]
    if len(dst) != size:
        raise ValueError(f"RefPack: unpacked {len(dst)} bytes, the header says {size}")
    return bytes(dst)


def refpack_store(data):
    """`data` as RefPack with literal commands only (no compression); for testing `refpack`."""
    out = bytearray(b"\x10\xfb" + len(data).to_bytes(3, "big"))
    i = 0
    while len(data) - i >= 4:
        run = min(112, (len(data) - i) & ~3)
        out.append(0xE0 + (run - 4) // 4)
        out += data[i:i + run]
        i += run
    out.append(0xFC + len(data) - i)
    out += data[i:]
    return bytes(out)


def extension(data, t=None):
    for magic, ext in EXTENSIONS:
        if data.startswith(magic):
            return ext
    if t in (0x01A527DB, 0x01EEF63A) and len(data) >= 8 and data[0] >> 4 in (0, 1):
        return "snr"
    name = TYPES.get(t)
    return name.lower().strip("_") if name else "bin"


def packages(target):
    """`target` if it is a file, else every .package under it (sorted)."""
    if os.path.isfile(target):
        return [target]
    found = []
    for folder, _dirs, files in os.walk(target):
        found += [os.path.join(folder, f) for f in files if f.lower().endswith(".package")]
    return sorted(found)


def _filters(args):
    t = int(args.type, 16) if args.type else None
    grep = args.grep.lower() if getattr(args, "grep", None) else None

    def ok(e):
        return (t is None or e.type == t) and (grep is None or grep in str(e).lower())
    return ok


def cmd_list(args):
    ok = _filters(args)
    many = os.path.isdir(args.target)
    for path in packages(args.target):
        pkg = Package(path)
        for e in pkg.entries:
            if ok(e):
                where = f"  [{os.path.relpath(path, args.target)}]" if many else ""
                print(f"{e}  {TYPES.get(e.type, ''):5} {COMPRESSION.get(e.compression, hex(e.compression)):>7} "
                      f"{e.file_size:>9} {e.mem_size:>9}{where}")


def cmd_types(args):
    for path in packages(args.target):
        pkg = Package(path)
        counts = {}
        for e in pkg.entries:
            c = counts.setdefault(e.type, [0, 0, set()])
            c[0] += 1
            c[1] += e.mem_size
            c[2].add(e.compression)
        print(f"{os.path.relpath(path, args.target) if os.path.isdir(args.target) else path}: "
              f"DBPF {pkg.version[0]}.{pkg.version[1]}, {len(pkg.entries)} resources")
        for t, (n, total, comps) in sorted(counts.items(), key=lambda kv: -kv[1][0]):
            print(f"  {t:08X} {TYPES.get(t, ''):5} {n:>7} resources {total / 1048576:>9.1f} MB  "
                  + ", ".join(COMPRESSION.get(c, hex(c)) for c in sorted(comps)))


def cmd_dump(args):
    out = os.path.abspath(args.out)
    if os.path.commonpath([out, REPO]) == REPO:
        sys.exit(f"{out} is inside the repository; game data must stay outside it")
    ok = _filters(args)
    os.makedirs(out, exist_ok=True)
    written = 0
    for path in packages(args.target):
        pkg = Package(path)
        for e in pkg.entries:
            if e.deleted or not ok(e):
                continue
            try:
                data = pkg.read(e)
            except (NotImplementedError, ValueError, zlib.error) as err:
                print(f"  skipped {e}: {err}")
                continue
            name = f"{e.type:08X}_{e.group:08X}_{e.instance:016X}.{extension(data, e.type)}"
            with open(os.path.join(out, name), "wb") as f:
                f.write(data)
            written += 1
    print(f"{written} resources written to {out}")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("list", help="the resources of a package (or of every package in a folder)")
    p.add_argument("target", help="a .package file or a folder")
    p.add_argument("--type", help="only this type (hex, e.g. 0x2F7D0004)")
    p.add_argument("--grep", help="only keys containing this text (hex, e.g. part of an instance)")
    p.set_defaults(func=cmd_list)
    p = sub.add_parser("types", help="how many resources of each type a package holds")
    p.add_argument("target", help="a .package file or a folder")
    p.set_defaults(func=cmd_types)
    p = sub.add_parser("dump", help="write resources to files, outside this repository")
    p.add_argument("target", help="a .package file or a folder")
    p.add_argument("out", help="destination folder, outside this repository")
    p.add_argument("--type", help="only this type (hex)")
    p.add_argument("--grep", help="only keys containing this text")
    p.set_defaults(func=cmd_dump)
    args = parser.parse_args(argv)
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    args.func(args)


if __name__ == "__main__":
    main()
