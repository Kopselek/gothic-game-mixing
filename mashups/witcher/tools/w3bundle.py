"""Read The Witcher 3's .bundle archives (REDengine 3), the first step of every converter.

A Witcher 3 install keeps its resources (meshes .w2mesh, entities .w2ent, rigs .w2rig, animations
.w2anims, textures .xbm, ...) in bundles under content\\content*\\bundles, dlc\\*\\content\\bundles
and the patch folders. Large texture and sound data lives next to them in texture.cache and
soundspc.cache, which this file does not read.

Layout (classic from WolvenKit-7, WolvenKit.Bundles/Bundle.cs and BundleItem.cs; next-gen read
from 4.04's own bundles):
  header  32 bytes: "POTATO70", uint32 bundle_size, uint32 dummy_size, uint32 toc_size, padding
  TOC     from 0x20 up to 0x20 + toc_size, in one of two entry layouts:
            classic (1.3x), 0x140 bytes: name[0x100] (latin-1, NUL-terminated, a depot path like
              characters\\models\\...), hash[16], uint32 empty, uint32 size, uint32 zsize,
              uint32 offset (from the file start), uint32 date, uint32 time, zero[16],
              uint32 crc32 (of the unpacked data), uint32 compression
            next-gen (4.x), 0x130 bytes: name[0x100], hash[16], uint64 offset, uint32 size,
              uint32 zsize, uint32 crc32, uint32 compression, zero[8]
          Each bundle gets the layout under which every entry is plausible.
  data    each entry's zsize bytes at its offset, packed by `compression`:
            0 none, 1 zlib, 2 snappy (raw), 3 doboz, 4/5 LZ4 block (5 is LZ4HC, same decoder)

LZ4 and snappy are decoded here in pure Python (the lz4 package speeds LZ4 up when installed);
doboz is not supported yet.

  python w3bundle.py scan "<Witcher 3 folder>"
  python w3bundle.py list "<bundle or Witcher 3 folder>" [--grep geralt]
  python w3bundle.py extract "<Witcher 3 folder>" <regex> --out "<folder outside this repo>"

Regexes match the depot path written with forward slashes (characters/models/geralt/...), so they
need no escaped backslashes; a file's .N.buffer companions match with it.

Extracted files are the user's game data: keep them outside the repository.
"""
import argparse
import os
import re
import struct
import sys
import zlib

MAGIC = b"POTATO70"
HEADER = struct.Struct("<8s3I")
CLASSIC = struct.Struct("<256s16s4I2I16s2I")
NEXTGEN = struct.Struct("<256s16sQ4I8s")
COMPRESSION = {0: "none", 1: "zlib", 2: "snappy", 3: "doboz", 4: "lz4", 5: "lz4hc"}
REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))  # mashups/witcher/tools -> the repo


class BundleEntry:
    __slots__ = ("bundle", "name", "size", "zsize", "offset", "crc", "compression")

    def __init__(self, bundle, name, size, zsize, offset, crc, compression):
        self.bundle, self.name, self.size, self.zsize = bundle, name, size, zsize
        self.offset, self.crc, self.compression = offset, crc, compression


class BundleReader:
    def __init__(self, path):
        self.path = path
        self.entries = []
        with open(path, "rb") as f:
            magic, self.bundle_size, self.dummy_size, toc_size = HEADER.unpack(f.read(HEADER.size))
            if magic != MAGIC:
                raise ValueError(f"{path}: not a Witcher 3 bundle (magic {magic!r})")
            f.seek(0x20)
            raw = f.read(toc_size)
        file_size = os.path.getsize(path)
        for layout in (NEXTGEN, CLASSIC):
            entries = self._parse(raw, layout)
            if entries and all(e.compression in COMPRESSION and 0x20 + toc_size <= e.offset
                               and e.offset + e.zsize <= file_size for e in entries):
                self.layout = "next-gen" if layout is NEXTGEN else "classic"
                self.entries = entries
                break
        else:
            raise ValueError(f"{path}: the TOC fits neither the classic nor the next-gen entry layout")

    def _parse(self, raw, layout):
        if len(raw) % layout.size:
            return None
        entries = []
        for pos in range(0, len(raw), layout.size):
            fields = layout.unpack_from(raw, pos)
            if layout is NEXTGEN:
                name, _hash, offset, size, zsize, crc, compression, _zero = fields
            else:
                name, _hash, _empty, size, zsize, offset, _date, _time, _zero, crc, compression = fields
            name = name.split(b"\0")[0].decode("latin-1")
            if name:
                entries.append(BundleEntry(self, name, size, zsize, offset, crc, compression))
        return entries

    def read(self, entry):
        with open(self.path, "rb") as f:
            f.seek(entry.offset)
            packed = f.read(entry.zsize)
        data = unpack(packed, entry.size, entry.compression)
        if len(data) != entry.size:
            raise ValueError(f"{entry.name}: unpacked {len(data)} bytes, expected {entry.size}")
        if entry.crc and zlib.crc32(data) != entry.crc:
            raise ValueError(f"{entry.name}: CRC mismatch")
        return data


def unpack(packed, size, compression):
    if compression == 0:
        return packed
    if compression == 1:
        return zlib.decompress(packed)
    if compression == 2:
        return snappy_raw(packed)
    if compression in (4, 5):
        return lz4_block(packed, size)
    raise NotImplementedError(f"compression {compression} ({COMPRESSION.get(compression, '?')}) not supported yet")


def lz4_block(src, size):
    """Decode one raw LZ4 block (no frame header) of `size` unpacked bytes."""
    try:
        import lz4.block
        return lz4.block.decompress(src, uncompressed_size=size)
    except ImportError:
        pass
    dst = bytearray()
    i, n = 0, len(src)
    while i < n:
        token = src[i]
        i += 1
        length = token >> 4
        if length == 15:
            while True:
                b = src[i]
                i += 1
                length += b
                if b != 255:
                    break
        dst += src[i:i + length]
        i += length
        if i >= n:
            break
        distance = src[i] | src[i + 1] << 8
        i += 2
        length = token & 15
        if length == 15:
            while True:
                b = src[i]
                i += 1
                length += b
                if b != 255:
                    break
        length += 4
        start = len(dst) - distance
        if distance >= length:
            dst += dst[start:start + length]
        else:  # the match overlaps what it writes: it repeats the last `distance` bytes
            pattern = bytes(dst[start:])
            count, rest = divmod(length, distance)
            dst += pattern * count + pattern[:rest]
    return bytes(dst)


def snappy_raw(src):
    """Decode raw snappy (a varint length, then literal and copy elements)."""
    i, size, shift = 0, 0, 0
    while True:
        b = src[i]
        i += 1
        size |= (b & 0x7F) << shift
        shift += 7
        if b < 0x80:
            break
    dst = bytearray()
    while i < len(src):
        tag = src[i]
        i += 1
        kind = tag & 3
        if kind == 0:
            length = tag >> 2
            if length >= 60:
                width = length - 59
                length = int.from_bytes(src[i:i + width], "little")
                i += width
            length += 1
            dst += src[i:i + length]
            i += length
            continue
        if kind == 1:
            length = ((tag >> 2) & 7) + 4
            distance = (tag >> 5) << 8 | src[i]
            i += 1
        elif kind == 2:
            length = (tag >> 2) + 1
            distance = src[i] | src[i + 1] << 8
            i += 2
        else:
            length = (tag >> 2) + 1
            distance = int.from_bytes(src[i:i + 4], "little")
            i += 4
        start = len(dst) - distance
        if distance >= length:
            dst += dst[start:start + length]
        else:
            pattern = bytes(dst[start:])
            count, rest = divmod(length, distance)
            dst += pattern * count + pattern[:rest]
    if len(dst) != size:
        raise ValueError(f"snappy: unpacked {len(dst)} bytes, expected {size}")
    return bytes(dst)


def priority(path):
    """Sort key for bundles: content, then patches, then DLC; a later bundle's copy of a file wins.
    (The engine's own order is still to be checked against a real install.)"""
    p = path.replace("\\", "/").lower()
    group = 2 if "/dlc/" in p else (1 if "/patch" in p else 0)
    return (group, p)


def find_bundles(root):
    found = []
    for folder, _dirs, files in os.walk(root):
        found += [os.path.join(folder, f) for f in files if f.lower().endswith(".bundle")]
    return sorted(found, key=priority)


def open_all(target):
    if os.path.isfile(target):
        return [BundleReader(target)]
    return [BundleReader(p) for p in find_bundles(target)]


class W3Install:
    """Every file in an install's bundles by depot path, read straight from the bundles.

    Paths are matched case-insensitively with either slash. Where several bundles hold a file
    (4.04 repeats many in its world bundles), the one sorting last by `priority` is used."""

    def __init__(self, root):
        self.root = root
        self.entries = {}
        for reader in open_all(root):
            for e in reader.entries:
                self.entries[self.key(e.name)] = e
        self._by_base = None

    @staticmethod
    def key(name):
        return name.replace("/", "\\").lower()

    def __contains__(self, name):
        return self.key(name) in self.entries

    def read(self, name):
        e = self.entries.get(self.key(name))
        if e is None:
            raise FileNotFoundError(f"{name}: not in the Witcher 3 bundles")
        return e.bundle.read(e)

    def find(self, basename):
        """Depot paths whose file name is `basename` (any folder), e.g. 'h_02_mg__geralt.w2ent'."""
        if self._by_base is None:
            self._by_base = {}
            for k in self.entries:
                self._by_base.setdefault(k.rsplit("\\", 1)[-1], []).append(k)
        return self._by_base.get(basename.lower(), [])


def human(n):
    for unit in ("B", "KB", "MB", "GB"):
        if n < 1024 or unit == "GB":
            return f"{n:.0f} {unit}" if unit == "B" else f"{n:.1f} {unit}"
        n /= 1024


def cmd_scan(args):
    root = args.w3
    exes = [p for p in ("bin/x64/witcher3.exe", "bin/x64_dx12/witcher3.exe")
            if os.path.isfile(os.path.join(root, p))]
    print(f"Witcher 3 at {root}")
    print("  executables:", ", ".join(exes) or "none found (is this the game folder?)")
    if any("x64_dx12" in p for p in exes):
        print("  next-gen edition (4.x: DX12 build present)")
    elif exes:
        print("  classic edition (1.3x: no DX12 build)")
    other = {}
    for folder, _dirs, files in os.walk(root):
        for f in files:
            ext = os.path.splitext(f)[1].lower()
            if ext in (".cache", ".w3speech", ".w3strings", ".store"):
                other.setdefault(ext, []).append(os.path.relpath(os.path.join(folder, f), root))
    bundles = find_bundles(root)
    stats, total = {}, 0
    print(f"  {len(bundles)} bundles:")
    for path in bundles:
        try:
            reader = BundleReader(path)
        except ValueError as e:
            print(f"    {os.path.relpath(path, root)}: {e}")
            continue
        total += len(reader.entries)
        for e in reader.entries:
            stats[e.compression] = stats.get(e.compression, 0) + 1
        print(f"    {os.path.relpath(path, root)}: {len(reader.entries)} files, {human(os.path.getsize(path))}, "
              f"{reader.layout} TOC")
    print(f"  {total} files in bundles; compression: "
          + ", ".join(f"{COMPRESSION.get(c, c)} {n}" for c, n in sorted(stats.items())))
    for ext in sorted(other):
        print(f"  {ext}: " + ", ".join(sorted(other[ext])))


def cmd_list(args):
    pattern = re.compile(args.grep, re.I) if args.grep else None
    for reader in open_all(args.target):
        for e in reader.entries:
            if pattern and not pattern.search(e.name.replace("\\", "/")):
                continue
            print(f"{e.size:>10}  {COMPRESSION.get(e.compression, e.compression):>6}  {e.name}"
                  + (f"   [{os.path.basename(reader.path)}]" if os.path.isdir(args.target) else ""))


def cmd_extract(args):
    out = os.path.abspath(args.out)
    if os.path.commonpath([out, REPO]) == REPO:
        sys.exit(f"--out {out} is inside the repository; game data must stay outside it")
    pattern = re.compile(args.pattern, re.I)
    chosen = {}  # name -> entry from the bundle that wins
    for reader in open_all(args.w3):
        for e in reader.entries:
            if pattern.search(e.name.replace("\\", "/")):
                if e.name in chosen:
                    print(f"  {e.name}: {os.path.basename(chosen[e.name].bundle.path)} overridden by "
                          f"{os.path.basename(reader.path)}")
                chosen[e.name] = e
    for name, e in sorted(chosen.items()):
        dest = os.path.join(out, *name.replace("\\", "/").split("/"))
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        try:
            data = e.bundle.read(e)
        except (NotImplementedError, ValueError) as err:
            print(f"  skipped {name}: {err}")
            continue
        with open(dest, "wb") as f:
            f.write(data)
    print(f"{len(chosen)} files matched, written under {out}")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("scan", help="edition, bundles, caches and compression of an install")
    p.add_argument("w3", help="the Witcher 3 folder (with bin and content)")
    p.set_defaults(func=cmd_scan)
    p = sub.add_parser("list", help="the files in one bundle or in every bundle of an install")
    p.add_argument("target", help="a .bundle file or the Witcher 3 folder")
    p.add_argument("--grep", help="only depot paths matching this regex (case-insensitive, written with /)")
    p.set_defaults(func=cmd_list)
    p = sub.add_parser("extract", help="unpack the files whose depot path matches a regex")
    p.add_argument("w3", help="the Witcher 3 folder, or one .bundle")
    p.add_argument("pattern", help="regex on the depot path written with /, e.g. '^characters/models/geralt/'")
    p.add_argument("--out", required=True, help="destination folder, outside this repository")
    p.set_defaults(func=cmd_extract)
    args = parser.parse_args(argv)
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    args.func(args)


if __name__ == "__main__":
    main()
