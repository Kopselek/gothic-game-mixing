"""Read REDengine 3 CR2W resources: .w2mesh, .w2ent, .w2rig, .w2anims, .xbm and the rest.

Every Witcher 3 resource in a bundle (see w3bundle.py) is a CR2W file. Its large binary data
(vertex buffers, animation tracks) lives next to it in companion files <name>.<index>.buffer,
which w3bundle.py extracts with it.

Layout (classic from WolvenKit-7's CR2W/CR2WHeaderStructs.cs and friends; 4.04 writes version 164):
  "CR2W", uint32 version, flags, uint64 timestamp, uint32 build, file_size, buffer_size, crc32,
  num_chunks, then 10 tables of {uint32 offset, count, crc32}:
    0 strings   NUL-terminated latin-1 text, `count` bytes
    1 names     {uint32 string offset, hash}: CNames and type names, index 0 is ""
    2 imports   {uint32 depot path offset, uint16 class name, uint16 flags}: other resources
    3 properties (not needed for reading)
    4 exports   {uint16 class name, uint16 flags, uint32 parent, data_size, data_offset,
                 template, crc32}: the objects ("chunks") in the file
    5 buffers   {uint32 flags, index, offset, disk_size, mem_size, crc32}
    6 embedded  (not read yet)
  A chunk's data, like every struct value inside it, is a 0 byte, then its properties, each
  {uint16 name, uint16 type, uint32 size (counting itself), value}, ended by a uint16 0. Some classes write more binary
  data after the properties (CMesh: bone names and matrices, see w2mesh.py); `Chunk.extra` is
  where it starts.

Values: numbers, Bool and LocalizedString (its string id) as Python numbers; CName and enums as
str; String as str; handle: and ptr: as int (>0 chunk index + 1, <0 import -index); soft: as the import's depot path;
DeferredDataBuffer as its buffer index; arrays as lists; structs as dicts of their properties.
A type this reader cannot decode is kept as bytes, so a file always reads.

  python cr2w.py <file> [--chunk N]     prints the chunks and their properties
"""
import argparse
import os
import re
import struct
import sys

MAGIC = b"CR2W"
PRIMITIVES = {
    "Bool": "<B", "Int8": "<b", "Uint8": "<B", "Int16": "<h", "Uint16": "<H", "Int32": "<i",
    "Uint32": "<I", "Int64": "<q", "Uint64": "<Q", "Float": "<f", "Double": "<d",
    "LocalizedString": "<I",  # a string id in <lang>.w3strings (w3strings.py)
}
ENUM = re.compile(r"^E[A-Z]")


class Chunk:
    def __init__(self, file, index, class_name, parent, data_offset, data_size):
        self.file, self.index, self.type = file, index, class_name
        self.parent = parent  # chunk index + 1 of the parent, 0 for none
        self.data_offset, self.data_size = data_offset, data_size
        self.props = {}
        self.extra = data_offset + data_size  # where class-specific data after the properties starts

    def get(self, name, default=None):
        return self.props.get(name, default)

    def __repr__(self):
        return f"<{self.type} #{self.index}>"


class CR2W:
    """A CR2W resource from a file on disk (`source` a path), or from bytes read out of a bundle
    (`source` the bytes, `install` a w3bundle.W3Install to fetch its .buffer companions from)."""

    def __init__(self, source, name=None, install=None):
        if isinstance(source, (bytes, bytearray)):
            self.path, self.data, self.install = name, bytes(source), install
        else:
            self.path, self.install = source, None
            with open(source, "rb") as f:
                self.data = f.read()
        data, path = self.data, self.path
        if data[:4] != MAGIC:
            raise ValueError(f"{path}: not a CR2W file")
        (self.version, self.flags, self.timestamp, self.build, self.file_size, self.buffer_size,
         _crc, self.num_chunks) = struct.unpack_from("<IIQIIIII", data, 4)
        tables = [struct.unpack_from("<III", data, 40 + 12 * i) for i in range(10)]
        offset, count, _ = tables[0]
        self.strings = data[offset:offset + count]
        self.names = [self.string(v) for v, _h in self._table(tables[1], "<II")]
        self.imports = [(self.string(p), self.names[c], fl) for p, c, fl in self._table(tables[2], "<IHH")]
        self.chunks = []
        for i, (cls, _fl, parent, size, off, _tpl, _crc) in enumerate(self._table(tables[4], "<HHIIIII")):
            self.chunks.append(Chunk(self, i, self.names[cls], parent, off, size))
        self.buffers = [dict(zip(("flags", "index", "offset", "disk_size", "mem_size", "crc"), b))
                        for b in self._table(tables[5], "<IIIIII")]
        for chunk in self.chunks:
            self._read_chunk(chunk)

    def _table(self, table, fmt):
        offset, count, _ = table
        size = struct.calcsize(fmt)
        return [struct.unpack_from(fmt, self.data, offset + i * size) for i in range(count)]

    def string(self, offset):
        end = self.strings.index(b"\0", offset)
        return self.strings[offset:end].decode("latin-1")

    def of_type(self, class_name):
        return [c for c in self.chunks if c.type == class_name]

    def ref(self, handle):
        """The chunk a handle/ptr points to (None for 0 or an import)."""
        return self.chunks[handle - 1] if handle and handle > 0 else None

    def buffer(self, index):
        """Bytes of buffer `index` (1-based, as DeferredDataBuffer stores it): from the companion
        file <path>.<index>.buffer, or from inside this file."""
        companion = f"{self.path}.{index}.buffer"
        if self.install is not None and companion in self.install:
            return self.install.read(companion)
        if self.install is None and os.path.isfile(companion):
            with open(companion, "rb") as f:
                return f.read()
        for b in self.buffers:
            if b["index"] == index and self.buffer_size > self.file_size:
                return self.data[b["offset"]:b["offset"] + b["disk_size"]]
        raise FileNotFoundError(f"{companion}: buffer {index} not found (extract it with the file)")

    # chunk data

    def _read_chunk(self, chunk):
        props, pos = self._read_struct(chunk.data_offset, chunk.data_offset + chunk.data_size)
        if props is None:
            raise ValueError(f"{chunk}: unreadable properties")
        chunk.props, chunk.extra = props, pos

    def _read_struct(self, pos, end):
        """A 0 byte and a property list: (dict, next pos), or (None, pos)."""
        if pos >= end or self.data[pos] != 0:
            return None, pos
        return self._read_props(pos + 1, end)

    def _read_props(self, pos, end):
        """A property list from pos up to its terminating 0 (and not past end): (dict, next pos),
        or (None, pos) if the bytes are not one."""
        data, names, props = self.data, self.names, {}
        while True:
            if pos + 2 > end:
                return None, pos
            name, = struct.unpack_from("<H", data, pos)
            if name == 0:
                return props, pos + 2
            if pos + 8 > end:
                return None, pos
            type_index, size = struct.unpack_from("<HI", data, pos + 2)
            if name >= len(names) or type_index >= len(names) or size < 4 or pos + 4 + size > end:
                return None, pos
            start, value_end = pos + 8, pos + 4 + size
            props[names[name]] = self._value(names[type_index], start, value_end)
            pos = value_end

    def _value(self, type_name, start, end):
        """A property value whose bytes are known to be [start, end)."""
        if type_name in PRIMITIVES or type_name in ("CName", "String") or type_name.startswith(
                ("handle:", "ptr:", "soft:", "array:")) or type_name in ("DeferredDataBuffer", "DataBuffer",
                                                                       "SharedDataBuffer"):
            try:
                value, pos = self._seq(type_name, start, end)
                if pos == end:
                    return value
            except (struct.error, IndexError, ValueError, UnicodeDecodeError):
                pass
            return self.data[start:end]
        if ENUM.match(type_name):
            return self._enum(start, end)
        props, pos = self._read_struct(start, end)
        if props is not None and pos == end:
            return props
        if end - start == 2:
            return self._enum(start, end)
        return self.data[start:end]

    def _enum(self, start, end):
        values = []
        for pos in range(start, end - 1, 2):
            index, = struct.unpack_from("<H", self.data, pos)
            if index == 0:
                break
            values.append(self.names[index] if index < len(self.names) else index)
        return values[0] if len(values) == 1 else values

    def _seq(self, type_name, pos, end):
        """Read one value of type_name at pos without knowing its size: (value, next pos)."""
        data = self.data
        fmt = PRIMITIVES.get(type_name)
        if fmt:
            return struct.unpack_from(fmt, data, pos)[0], pos + struct.calcsize(fmt)
        if type_name == "CName":
            index, = struct.unpack_from("<H", data, pos)
            return self.names[index], pos + 2
        if type_name.startswith(("handle:", "ptr:")):
            return struct.unpack_from("<i", data, pos)[0], pos + 4
        if type_name.startswith("soft:"):
            index, = struct.unpack_from("<H", data, pos)
            return (self.imports[index - 1][0] if 0 < index <= len(self.imports) else None), pos + 2
        if type_name == "DeferredDataBuffer":
            return struct.unpack_from("<H", data, pos)[0], pos + 2
        if type_name in ("DataBuffer", "SharedDataBuffer"):
            size, = struct.unpack_from("<I", data, pos)
            return data[pos + 4:pos + 4 + size], pos + 4 + size
        if type_name == "String":
            return self._string_value(pos)
        if type_name.startswith("array:"):
            inner = type_name.split(",", 2)[2]
            count, = struct.unpack_from("<I", data, pos)
            pos += 4
            if inner == "Uint8":
                return data[pos:pos + count], pos + count
            items = []
            for _ in range(count):
                item, pos = self._seq(inner, pos, end)
                items.append(item)
            return items, pos
        if ENUM.match(type_name):
            index, = struct.unpack_from("<H", data, pos)
            return self.names[index], pos + 2
        props, after = self._read_struct(pos, end)
        if props is None:
            raise ValueError(f"cannot read a {type_name} at {pos}")
        return props, after

    def _string_value(self, pos):
        b = self.data[pos]
        pos += 1
        if b == 0x80:
            return "", pos
        wide = not (b & 0x80)
        length = b & 0x3F
        if b & 0x40:
            length += 64 * self.data[pos]
            pos += 1
        raw = self.data[pos:pos + (length * 2 if wide else length)]
        text = raw.decode("utf-16-le" if wide else "latin-1")
        return text.rstrip("\0"), pos + len(raw)


def read_vlq(data, pos):
    """REDengine's signed VLQ int (6 bits in the first byte, 7 in the next): (value, next pos)."""
    b = data[pos]
    pos += 1
    negative, more, value, shift = b & 0x80, b & 0x40, b & 0x3F, 6
    while more:
        b = data[pos]
        pos += 1
        value |= (b & 0x7F) << shift
        more = b & 0x80
        shift += 7
    return (-value if negative else value), pos


def read_bit6(data, pos):
    """REDengine's 'bit6' count used by padded buffers: (value, next pos)."""
    result, shift, i = 0, 0, 1
    while True:
        b = data[pos]
        pos += 1
        if b == 128:
            return 0, pos
        bits, mask = 6, 0xFF
        if b > 127:
            bits, mask = 7, 0x7F
        elif b > 63 and i == 1:
            mask = 0x3F
        result |= (b & mask) << shift
        shift += bits
        i += 1
        if b < 64 or (i >= 3 and b < 128):
            return result, pos


def _show(value, indent):
    pad = "  " * indent
    if isinstance(value, dict):
        for k, v in value.items():
            if isinstance(v, (dict, list)) and v:
                print(f"{pad}{k}:")
                _show(v, indent + 1)
            else:
                print(f"{pad}{k}: {_short(v)}")
    elif isinstance(value, list):
        for i, v in enumerate(value[:20]):
            if isinstance(v, (dict, list)):
                print(f"{pad}[{i}]")
                _show(v, indent + 1)
            else:
                print(f"{pad}[{i}] {_short(v)}")
        if len(value) > 20:
            print(f"{pad}... {len(value)} items")


def _short(v):
    if isinstance(v, bytes):
        return f"<{len(v)} bytes>"
    return repr(v)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("file")
    parser.add_argument("--chunk", type=int, help="only this chunk")
    args = parser.parse_args(argv)
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    f = CR2W(args.file)
    print(f"CR2W version {f.version}, {len(f.chunks)} chunks, {len(f.buffers)} buffers, "
          f"{len(f.imports)} imports")
    for path, cls, _flags in f.imports:
        print(f"  import {cls} {path}")
    for chunk in f.chunks:
        if args.chunk is not None and chunk.index != args.chunk:
            continue
        rest = chunk.data_offset + chunk.data_size - chunk.extra
        print(f"#{chunk.index} {chunk.type} (parent {chunk.parent - 1}){f', {rest} bytes after the properties' if rest else ''}")
        _show(chunk.props, 1)


if __name__ == "__main__":
    main()
