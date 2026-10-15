"""Read The Witcher 3's localized text: content\\content0\\<lang>.w3strings (string id -> text).

Every line of dialogue, item name and journal entry is a LocalizedString id in the resources
(scenes, entities); <lang>.w3strings maps the ids to text, and <lang>pc.w3speech (w3speech.py)
maps the same ids to the spoken audio.

Layout (classic from WolvenKit-7's WolvenKit.W3Strings; the next-gen changes were worked out on
4.04's pl/en files):
  "RTSW", uint32 version (162 classic, 164 next-gen), uint16 key1
  block 1  bit6 count, then per string 12 bytes: uint32 id ^ magic, uint32 offset, uint32 length
  block 2  bit6 count, then per entry 8 bytes: uint32 key hash, uint32 id ^ magic
           (the string keys some ids also have; hash = h * 31 + char over the key, 32-bit)
  block 3  bit6 count of text units, then the text; each string is `length` units at
           offset * unit from the start of the text, followed by a 0 unit
  uint16 key2 at the very end of the file

  The language is key1 << 16 | key2, and each language has a fixed `magic` (LANGUAGES).
  Text is obfuscated with a key stream that depends only on the magic and the string's length:
      k = (magic >> 8) & 0xFFFF
      for each unit i: unit ^= ((length + 1) * k) & 0xFFFF;  k = rotl16(k, 1)
  Classic files (162) store UTF-16LE units (2 bytes, the whole 16-bit key is used). Next-gen
  files (164) store UTF-8 bytes: offsets and lengths count bytes, and each byte is XORed with
  the low byte of the key. The reader tells them apart by the size of the text block.

  python w3strings.py <file.w3strings> [--grep REGEX] [--id N ...]
"""
import argparse
import re
import struct
import sys

from cr2w import read_bit6

MAGIC = b"RTSW"
# language key (key1 << 16 | key2) -> (code, magic), from WolvenKit-7's W3Language.cs
LANGUAGES = {
    0x83496237: ("pl", 0x73946816), 0x43975139: ("en", 0x79321793), 0x75886138: ("de", 0x42791159),
    0x45931894: ("it", 0x12375973), 0x23863176: ("fr", 0x75921975), 0x24987354: ("cz", 0x21793217),
    0x18796651: ("es", 0x42387566), 0x18632176: ("zh", 0x16875467), 0x63481486: ("ru", 0x42386347),
    0x42378932: ("hu", 0x67823218), 0x54834893: ("jp", 0x59825646),
}


def language(key):
    """(code, magic) of a language key, ("?", 0) if unknown."""
    return LANGUAGES.get(key, ("?", 0))


def decrypt(raw, length, magic, wide):
    k = (magic >> 8) & 0xFFFF
    if wide:
        units = struct.unpack(f"<{length}H", raw)
        out = []
        for u in units:
            out.append(u ^ (((length + 1) * k) & 0xFFFF))
            k = ((k << 1) | (k >> 15)) & 0xFFFF
        return struct.pack(f"<{length}H", *out).decode("utf-16-le", "replace")
    out = bytearray(raw)
    for i in range(length):
        out[i] ^= ((length + 1) * k) & 0xFF
        k = ((k << 1) | (k >> 15)) & 0xFFFF
    return out.decode("utf-8", "replace")


class W3Strings:
    """.text: {string id: text}; .keys: {key hash: string id}; .lang, .magic, .version."""

    def __init__(self, path):
        self.path = path
        with open(path, "rb") as f:
            data = f.read()
        if data[:4] != MAGIC:
            raise ValueError(f"{path}: not a .w3strings file")
        self.version, key1 = struct.unpack_from("<IH", data, 4)
        key2, = struct.unpack_from("<H", data, len(data) - 2)
        self.key = key1 << 16 | key2
        self.lang, self.magic = language(self.key)
        magic = self.magic
        count, pos = read_bit6(data, 10)
        entries = [struct.unpack_from("<III", data, pos + 12 * i) for i in range(count)]
        pos += 12 * count
        count, pos = read_bit6(data, pos)
        self.keys = {h: i ^ magic for h, i in (struct.unpack_from("<II", data, pos + 8 * j) for j in range(count))}
        pos += 8 * count
        units, start = read_bit6(data, pos)
        text_bytes = len(data) - 2 - start
        if text_bytes == units:
            self.wide = False
        elif text_bytes == 2 * units:
            self.wide = True
        else:  # fall back on the version
            self.wide = self.version < 164
        unit = 2 if self.wide else 1
        self.text = {}
        for hashed, offset, length in entries:
            at = start + offset * unit
            self.text[hashed ^ magic] = decrypt(data[at:at + length * unit], length, magic, self.wide)

    def get(self, string_id, default=None):
        return self.text.get(string_id, default)

    def __getitem__(self, string_id):
        return self.text[string_id]

    def __contains__(self, string_id):
        return string_id in self.text

    def __len__(self):
        return len(self.text)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("file")
    parser.add_argument("--grep", help="only strings matching this regex (case-insensitive)")
    parser.add_argument("--id", type=int, nargs="*", help="only these string ids")
    parser.add_argument("--limit", type=int, default=50)
    args = parser.parse_args(argv)
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    s = W3Strings(args.file)
    print(f"version {s.version}, language {s.lang} (key {s.key:08x}, magic {s.magic:08x}), "
          f"{'UTF-16' if s.wide else 'UTF-8'} text, {len(s)} strings, {len(s.keys)} keys")
    pattern = re.compile(args.grep, re.I) if args.grep else None
    shown = 0
    for sid in (args.id if args.id else sorted(s.text)):
        text = s.text.get(sid)
        if text is None or (pattern and not pattern.search(text)):
            continue
        print(f"{sid}\t{text}")
        shown += 1
        if shown >= args.limit:
            break


if __name__ == "__main__":
    main()
