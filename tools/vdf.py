"""Read and write Gothic VDFS archives (.vdf / .mod).

Layout (Gothic 1 and 2):
  header  296 bytes: comment[256], signature[16], entry_count, file_count,
                     dos_timestamp, data_size, catalog_offset, entry_size (80)
  catalog entry_count * 80 bytes: name[64] (space padded), offset_or_child_index,
                     size, type flags, attributes
  data    file contents

Directories list their children as a contiguous block starting at the entry index
stored in `offset`; the last entry of every block has the LAST flag set.
"""
import struct
import time

SIGNATURE_G1 = b"PSVDSC_V2.00\r\n\r\n"
SIGNATURE_G2 = b"PSVDSC_V2.00\n\r\n\r"
HEADER = struct.Struct("<256s16s6I")
ENTRY = struct.Struct("<64s4I")
TYPE_DIR = 0x80000000
TYPE_LAST = 0x40000000
ATTR_ARCHIVE = 0x20


def dos_timestamp(t=None):
    lt = time.localtime(t)
    date = ((lt.tm_year - 1980) << 9) | (lt.tm_mon << 5) | lt.tm_mday
    clock = (lt.tm_hour << 11) | (lt.tm_min << 5) | (lt.tm_sec // 2)
    return (date << 16) | clock


class VdfReader:
    def __init__(self, path):
        self.path = path
        self.files = {}  # "DIR/SUB/NAME.EXT" -> (offset, size)
        with open(path, "rb") as f:
            (self.comment, self.signature, count, self.file_count,
             self.timestamp, self.data_size, catalog, _) = HEADER.unpack(f.read(HEADER.size))
            f.seek(catalog)
            raw = f.read(count * ENTRY.size)
        entries = [ENTRY.unpack_from(raw, i * ENTRY.size) for i in range(count)]
        self._walk(entries, 0, "")

    def _walk(self, entries, index, prefix):
        while True:
            name, offset, size, kind, _ = entries[index]
            name = name.decode("latin-1").rstrip(" \0")
            if kind & TYPE_DIR:
                self._walk(entries, offset, prefix + name + "/")
            else:
                self.files[prefix + name] = (offset, size)
            if kind & TYPE_LAST:
                return
            index += 1

    def read(self, name, limit=None):
        offset, size = self.files[name]
        with open(self.path, "rb") as f:
            f.seek(offset)
            return f.read(size if limit is None else min(size, limit))


def write_vdf(path, files, comment="", signature=SIGNATURE_G1, timestamp=None):
    """Write `files` ({"DIR/SUB/NAME.EXT": bytes}) to a new VDF archive.

    Gothic resolves duplicate names across archives by timestamp (newest wins),
    so the default is "now" to override the original game archives.
    """
    tree = {}
    for name, data in files.items():
        node = tree
        *dirs, leaf = name.upper().replace("\\", "/").split("/")
        for d in dirs:
            node = node.setdefault(d, {})
        node[leaf] = data

    def ordered(block):  # directories first, then files, alphabetical
        return sorted(block, key=lambda n: (not isinstance(block[n], dict), n))

    # Breadth-first layout: each directory's children form one contiguous block.
    blocks = [tree]
    first_index = [0]
    i = 0
    while i < len(blocks):
        for name in ordered(blocks[i]):
            if isinstance(blocks[i][name], dict):
                first_index.append(first_index[-1] + len(blocks[-1]))
                blocks.append(blocks[i][name])
        i += 1

    entry_count = sum(len(b) for b in blocks)
    data_start = HEADER.size + entry_count * ENTRY.size
    catalog = bytearray()
    payload = bytearray()
    dir_cursor = 1
    for block in blocks:
        names = ordered(block)
        for n, name in enumerate(names):
            child = block[name]
            flags = TYPE_LAST if n == len(names) - 1 else 0
            if isinstance(child, dict):
                entry = (first_index[dir_cursor], 0, flags | TYPE_DIR)
                dir_cursor += 1
            else:
                entry = (data_start + len(payload), len(child), flags)
                payload += child
            catalog += ENTRY.pack(name.encode("latin-1").ljust(64, b" "), *entry, ATTR_ARCHIVE)

    file_count = sum(1 for b in blocks for c in b.values() if not isinstance(c, dict))
    header = HEADER.pack(comment.encode("latin-1").ljust(256, b"\x1a"), signature,
                         entry_count, file_count, dos_timestamp(timestamp),
                         len(payload), HEADER.size, ENTRY.size)
    with open(path, "wb") as f:
        f.write(header + catalog + payload)
