"""Read The Witcher 3's spoken lines: content\\content0\\<lang>pc.w3speech (string id -> audio).

Every voiced line is a LocalizedString id (its text is in <lang>.w3strings, see w3strings.py);
this file holds the line's audio, its length in seconds and its lipsync data.

Layout (classic from WolvenKit-7's WolvenKit.W3Speech/Coder.cs; checked on 4.04's plpc/enpc files,
version 164):
  "CPSW", uint32 version, uint16 key1, bit6 count, then per line 40 bytes:
    uint32 id ^ magic (the language's magic, see w3strings.LANGUAGES), uint32 0,
    uint64 audio offset R, uint64 audio size + 12, uint64 lipsync offset, uint64 lipsync size
  then uint16 key2 (the language key is key1 << 16 | key2, as in .w3strings).
  At R: uint32 audio size S, S bytes of audio (a Wwise .wem), float32 duration in seconds,
  uint32 (1 in 4.04, 4 in WolvenKit-7's writer), then the lipsync data (lipsync offset = R + S + 12;
  in 4.04 it is not a CR2W file any more and is not decoded here).

Audio: 4.04's lines are RIFF/WAVE .wem files with format tag 0x3041, Wwise Opus: fmt extra
  +0x12 uint16 samples per frame (960), +0x18 int32 sample count, +0x1c uint32 packet count,
  +0x20 uint16 pre-skip, +0x22 uint8 version, +0x23 uint8 channel mapping; a "seek" chunk with one
  uint16 size per packet, and a "data" chunk with the raw Opus packets back to back (no Ogg pages).
  ffmpeg does not know tag 0x3041, so `wem_to_ogg` rewraps the packets as a standard Ogg Opus
  stream (OpusHead, OpusTags, the packets, the last granule trimming the end), which ffmpeg's
  opus decoder plays.

  python w3speech.py <file.w3speech> info
  python w3speech.py <file.w3speech> export <string id> [...] --out <folder outside the repo> [--wem]
"""
import argparse
import os
import struct
import subprocess
import sys
import zlib

from cr2w import read_bit6
from w3strings import language

MAGIC = b"CPSW"
ENTRY = struct.Struct("<IIQQQQ")
REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))  # mashups/witcher/tools -> the repo


class SpeechEntry:
    __slots__ = ("id", "lang_id", "offset", "size", "lipsync_offset", "lipsync_size")

    def __init__(self, string_id, lang_id, offset, size, lipsync_offset, lipsync_size):
        self.id, self.lang_id, self.offset, self.size = string_id, lang_id, offset, size
        self.lipsync_offset, self.lipsync_size = lipsync_offset, lipsync_size


class W3Speech:
    """.entries: {string id: SpeechEntry}; read_wem, duration, lipsync read one line's data."""

    def __init__(self, path):
        self.path = path
        with open(path, "rb") as f:
            head = f.read(16)
            if head[:4] != MAGIC:
                raise ValueError(f"{path}: not a .w3speech file")
            self.version, key1 = struct.unpack_from("<IH", head, 4)
            count, pos = read_bit6(head, 10)
            f.seek(pos)
            raw = f.read(ENTRY.size * count + 2)
        key2, = struct.unpack_from("<H", raw, ENTRY.size * count)
        self.key = key1 << 16 | key2
        self.lang, self.magic = language(self.key)
        self.entries = {}
        for i in range(count):
            lang_id, _high, offset, size, lip_off, lip_size = ENTRY.unpack_from(raw, i * ENTRY.size)
            sid = lang_id ^ self.magic
            if size <= 12:  # 4.04 has two such empty lines
                continue
            self.entries[sid] = SpeechEntry(sid, lang_id, offset, size - 12, lip_off, lip_size)

    def _read(self, offset, size):
        with open(self.path, "rb") as f:
            f.seek(offset)
            return f.read(size)

    def read_wem(self, string_id):
        e = self.entries[string_id]
        return self._read(e.offset + 4, e.size)

    def duration(self, string_id):
        e = self.entries[string_id]
        return struct.unpack("<f", self._read(e.offset + 4 + e.size, 4))[0]

    def durations(self, ids=None):
        """{string id: seconds} for many lines, reading the file in offset order."""
        out = {}
        with open(self.path, "rb") as f:
            for e in sorted((self.entries[i] for i in (ids if ids is not None else self.entries)),
                            key=lambda e: e.offset):
                f.seek(e.offset + 4 + e.size)
                out[e.id] = struct.unpack("<f", f.read(4))[0]
        return out

    def lipsync(self, string_id):
        e = self.entries[string_id]
        return self._read(e.lipsync_offset, e.lipsync_size)


# Wwise .wem

def riff_chunks(wem):
    if wem[:4] != b"RIFF" or wem[8:12] != b"WAVE":
        raise ValueError("not a RIFF/WAVE .wem")
    chunks, pos = {}, 12
    while pos + 8 <= len(wem):
        tag, size = struct.unpack_from("<4sI", wem, pos)
        chunks[tag.decode("latin-1")] = (pos + 8, size)
        pos += 8 + size + (size & 1)
    return chunks


def wem_info(wem):
    """Format tag, channels, rate and (for Wwise Opus) sample count, packet count and pre-skip."""
    chunks = riff_chunks(wem)
    fmt, _ = chunks["fmt "]
    tag, channels, rate = struct.unpack_from("<HHI", wem, fmt)
    info = {"format": tag, "channels": channels, "rate": rate}
    if tag == 0x3041:
        info.update(zip(("samples", "packets", "preskip", "version", "mapping"),
                        struct.unpack_from("<iIHBB", wem, fmt + 0x18)))
    return info


def opus_packets(wem):
    chunks = riff_chunks(wem)
    info = wem_info(wem)
    if info["format"] != 0x3041:
        raise ValueError(f"format 0x{info['format']:04x} is not Wwise Opus (0x3041)")
    seek, seek_size = chunks["seek"]
    data, data_size = chunks["data"]
    sizes = struct.unpack_from(f"<{info['packets']}H", wem, seek)
    if 2 * len(sizes) > seek_size or sum(sizes) > data_size:
        raise ValueError("Wwise Opus: the packet table does not fit the data")
    packets, pos = [], data
    for size in sizes:
        packets.append(wem[pos:pos + size])
        pos += size
    return info, packets


def opus_samples(packet):
    """Samples (at 48 kHz) in one Opus packet, from its TOC byte (RFC 6716, 3.1)."""
    toc = packet[0]
    config = toc >> 3
    if config < 12:
        frame = (480, 960, 1920, 2880)[config & 3]
    elif config < 16:
        frame = (480, 960)[config & 1]
    else:
        frame = (120, 240, 480, 960)[config & 3]
    code = toc & 3
    frames = 1 if code == 0 else 2 if code < 3 else packet[1] & 0x3F
    return frame * frames


def _ogg_crc_table():
    table = []
    for i in range(256):
        r = i << 24
        for _ in range(8):
            r = ((r << 1) ^ 0x04C11DB7) if r & 0x80000000 else r << 1
        table.append(r & 0xFFFFFFFF)
    return table


OGG_CRC = _ogg_crc_table()


def ogg_crc(data):
    crc = 0
    for b in data:
        crc = ((crc << 8) & 0xFFFFFFFF) ^ OGG_CRC[(crc >> 24) ^ b]
    return crc


def ogg_page(serial, seq, granule, packets, flags):
    lacing = bytearray()
    for p in packets:
        lacing += b"\xff" * (len(p) // 255) + bytes([len(p) % 255])
    header = struct.pack("<4sBBqIIIB", b"OggS", 0, flags, granule, serial, seq, 0, len(lacing)) + lacing
    page = bytearray(header + b"".join(packets))
    struct.pack_into("<I", page, 22, ogg_crc(page))
    return bytes(page)


def wem_to_ogg(wem, serial=0x57335350):
    """A Wwise Opus .wem rewrapped as an Ogg Opus file (bytes)."""
    info, packets = opus_packets(wem)
    channels = info["channels"]
    if channels > 2 or info["mapping"] not in (0, 1):
        raise NotImplementedError(f"Wwise Opus with {channels} channels / mapping {info['mapping']}")
    head = struct.pack("<8sBBHIhB", b"OpusHead", 1, channels, info["preskip"], info["rate"], 0, 0)
    tags = struct.pack("<8sI", b"OpusTags", 9) + b"w3speech" + struct.pack("<I", 0)
    pages = [ogg_page(serial, 0, 0, [head], 0x02), ogg_page(serial, 1, 0, [tags], 0)]
    end = info["preskip"] + info["samples"]  # the last granule cuts the padding off the end
    granule, page, segments, seq = 0, [], 0, 2
    for i, p in enumerate(packets):
        need = len(p) // 255 + 1
        if page and segments + need > 255:
            pages.append(ogg_page(serial, seq, min(granule, end), page, 0))
            page, segments, seq = [], 0, seq + 1
        page.append(p)
        segments += need
        granule += opus_samples(p)
    pages.append(ogg_page(serial, seq, min(granule, end), page, 0x04))
    return b"".join(pages)


def ogg_to_wav(ogg, dest, rate=44100, channels=1):
    """Decode Ogg Opus bytes with ffmpeg to a 16-bit PCM WAV file."""
    subprocess.run(["ffmpeg", "-hide_banner", "-v", "error", "-y", "-f", "ogg", "-i", "pipe:0",
                    "-ar", str(rate), "-ac", str(channels), "-c:a", "pcm_s16le", dest],
                   input=ogg, check=True)


def cmd_info(args, speech):
    print(f"version {speech.version}, language {speech.lang} (key {speech.key:08x}), {len(speech.entries)} lines")
    first = min(speech.entries.values(), key=lambda e: e.offset)
    wem = speech.read_wem(first.id)
    print(f"first line: id {first.id}, {first.size} bytes audio, {speech.duration(first.id):.3f} s, "
          f"lipsync {first.lipsync_size} bytes at {first.lipsync_offset}")
    print("  wem:", ", ".join(f"{k} {v:#x}" if k == "format" else f"{k} {v}" for k, v in wem_info(wem).items()))


def cmd_export(args, speech):
    out = os.path.abspath(args.out)
    if os.path.commonpath([out, REPO]) == REPO:
        sys.exit(f"--out {out} is inside the repository; game data must stay outside it")
    os.makedirs(out, exist_ok=True)
    for sid in args.ids:
        if sid not in speech.entries:
            print(f"  {sid}: no audio")
            continue
        wem = speech.read_wem(sid)
        base = os.path.join(out, f"{sid}")
        if args.wem:
            with open(base + ".wem", "wb") as f:
                f.write(wem)
        ogg_to_wav(wem_to_ogg(wem), base + ".wav", args.rate)
        print(f"  {sid}: {speech.duration(sid):.2f} s -> {base}.wav")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("file")
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("info", help="header, line count and the first line's audio format")
    p.set_defaults(func=cmd_info)
    p = sub.add_parser("export", help="lines as 16-bit mono WAV (decoded by ffmpeg)")
    p.add_argument("ids", type=int, nargs="+")
    p.add_argument("--out", required=True, help="destination folder, outside this repository")
    p.add_argument("--rate", type=int, default=44100)
    p.add_argument("--wem", action="store_true", help="also write the original .wem")
    p.set_defaults(func=cmd_export)
    args = parser.parse_args(argv)
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    args.func(args, W3Speech(args.file))


if __name__ == "__main__":
    main()
