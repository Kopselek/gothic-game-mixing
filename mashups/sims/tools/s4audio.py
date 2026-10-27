"""Read The Sims 4's sounds: EA's SNR audio (EAAudioCore "EAAC") and the effects that name them.

A sound effect is played by name ("ui_piemenu_open"); its resources are keyed by the FNV-1 64-bit
hash of the lower-case name (s4gfx.fnv64), group 0:
  PROPX  (type 0xFD04E3BE) the effect: uint16 version (3), 8 bytes, uint32 count, then count
         uint64 instances of its SNR variants (one is picked when played), then its properties
  SNR    (type 0x01A527DB, group 0x001407EC for the UI sounds) one sound, header and data:
    uint32 big-endian: version << 28 | codec << 24 | (channels - 1) << 18 | sample rate
    uint32 big-endian: type << 30 (0 RAM: the data follows; 1 streamed: in the SNS resource,
           type 0x01EEF63A, same key) | loop << 29 | sample count; loop: uint32 loop start
    blocks: uint32 big-endian flag << 24 (0x80 the last) | block size (with this header), uint32
           big-endian samples in the block, the codec's data
  codecs (vgmstream's ea_eaac.c): 2 PCM16BE, 3 EA-XMA, 4 EA-XAS v1, 5 EALayer3 v1, 6/7 EALayer3
  v2, 8 GC-ADPCM, 9 EASpeex, 10 ATRAC3plus, 11 EA-MP3, 12 EA-Opus
EA-XAS v1 is decoded here (numpy); The Sims 4 1.128 keeps nearly all its sounds, the UI's among
them, in EALayer3 (MPEG layer III frames rebuilt by EA's own scheme), which this does not decode.

EA-XAS v1 (FFmpeg's adpcm.c ADPCM_EA_XAS, vgmstream's ea_xas_decoder.c): a block's data is frames of
128 samples, 0x4C bytes per channel, the channels one after the other. A frame: 4 group headers
(uint32 little-endian: bits 0-3 the predictor, 4-15 the first sample, 16-19 the shift, 20-31 the
second sample, both samples' low 4 bits zero), then 15 rows of 4 bytes, byte g of a row two
4-bit samples (high nibble first) of group g. Each group is 32 samples: its two header samples
and 30 predicted ones: s = clip16((nibble << (20 - shift)) + s1 * c1 + s2 * c2 + 0x80 >> 8),
c1, c2 from (0, 0), (240, 0), (460, -208), (392, -220) by the predictor.

  python s4audio.py info <package> <key>
  python s4audio.py wav <package> <key> <out.wav>          (XAS only)
  python s4audio.py effect "<The Sims 4 folder>" <effect name> [<out.wav>]

The output is the user's game data: keep it outside the repository (refused inside it).
"""
import argparse
import os
import struct
import sys
import wave

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
import numpy as np

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))  # mashups/sims/tools -> the repo
SNR_TYPE, SNS_TYPE, PROPX_TYPE = 0x01A527DB, 0x01EEF63A, 0xFD04E3BE
CODECS = {0: "none", 1: "reserved", 2: "PCM16BE", 3: "EA-XMA", 4: "EA-XAS v1", 5: "EALayer3 v1",
          6: "EALayer3 v2 PCM", 7: "EALayer3 v2 spike", 8: "GC-ADPCM", 9: "EASpeex", 10: "EATrax",
          11: "EA-MP3", 12: "EA-Opus", 13: "EA-ATRAC9", 14: "EA-Opus MU", 15: "EA-Opus M"}
XAS, PCM16BE = 4, 2
EA_TABLE = np.array([0, 240, 460, 392, 0, 0, -208, -220, 0, 1, 3, 4, 7, 8, 10, 11, 0, -1, -3, -4], np.int64)


class Snr:
    """An SNR resource's header and its blocks' data."""

    def __init__(self, data, stream=None):
        h1, h2 = struct.unpack_from(">II", data, 0)
        self.version = h1 >> 28
        self.codec = (h1 >> 24) & 0x0F
        self.channels = ((h1 >> 18) & 0x3F) + 1
        self.rate = h1 & 0x3FFFF
        self.kind = h2 >> 30
        self.loop = (h2 >> 29) & 1
        self.samples = h2 & 0x1FFFFFFF
        pos = 8
        if self.loop:
            self.loop_start = struct.unpack_from(">I", data, pos)[0]
            pos += 4
        body = data[pos:] if self.kind == 0 else (stream or b"")
        self.blocks = []  # (samples, data)
        p = 0
        while p + 8 <= len(body):
            head, count = struct.unpack_from(">II", body, p)
            size = head & 0xFFFFFF
            if size < 8:
                break
            self.blocks.append((count, body[p + 8:p + size]))
            p += size
            if head >> 24 == 0x80:
                break

    @property
    def codec_name(self):
        return CODECS.get(self.codec, str(self.codec))

    def decode(self):
        """int16 samples: (n,) mono or (n, channels)."""
        if self.codec == XAS:
            parts = [decode_xas(data, self.channels, count) for count, data in self.blocks]
        elif self.codec == PCM16BE:
            parts = [np.frombuffer(data, ">i2", count * self.channels).reshape(count, self.channels)
                     for count, data in self.blocks]
        else:
            raise NotImplementedError(f"SNR codec {self.codec} ({self.codec_name}) is not decoded")
        pcm = np.concatenate(parts, 0)[:self.samples] if parts else np.zeros((0, self.channels), np.int16)
        pcm = pcm.astype(np.int16)
        return pcm[:, 0] if self.channels == 1 else pcm


def decode_xas(data, channels, samples):
    """One block of EA-XAS v1 as (samples, channels) int16."""
    frame = 0x4C * channels
    frames = (samples + 127) // 128
    raw = np.frombuffer(data, np.uint8, min(frames, len(data) // frame) * frame)
    raw = raw.reshape(-1, channels, 0x4C)
    f = raw.shape[0]
    heads = raw[:, :, :16].copy().view("<u4").reshape(f, channels, 4).astype(np.int64)
    pred = heads & 0x0F
    c1 = EA_TABLE[pred]
    c2 = EA_TABLE[pred + 4]
    s0 = ((heads & 0xFFF0) ^ 0x8000) - 0x8000           # the first sample, signed 16-bit
    s1 = (((heads >> 16) & 0xFFF0) ^ 0x8000) - 0x8000   # the second
    shift = 20 - ((heads >> 16) & 0x0F)
    rows = raw[:, :, 16:].reshape(f, channels, 15, 4).astype(np.int64)  # [.., row, group]
    out = np.empty((f, channels, 4, 32), np.int64)
    out[..., 0], out[..., 1] = s0, s1
    prev2, prev1 = s0, s1
    for r in range(15):
        byte = rows[:, :, r, :]
        for hi in (True, False):
            nib = (byte >> 4) if hi else (byte & 0x0F)
            nib = (nib ^ 8) - 8
            level = nib << shift
            s = np.clip((level + prev1 * c1 + prev2 * c2 + 0x80) >> 8, -32768, 32767)
            out[..., 2 + 2 * r + (0 if hi else 1)] = s
            prev2, prev1 = prev1, s
    pcm = out.reshape(f, channels, 128).transpose(0, 2, 1).reshape(f * 128, channels)
    return pcm[:samples].astype(np.int16)


def propx_variants(data):
    """The SNR instances a PROPX effect plays."""
    if len(data) < 14:
        return []
    count = struct.unpack_from("<I", data, 10)[0]
    if count > 64 or 14 + 8 * count > len(data):
        return []
    return [struct.unpack_from("<Q", data, 14 + 8 * i)[0] for i in range(count)]


def write_wav(path, pcm, rate):
    pcm = np.asarray(pcm, np.int16)
    channels = 1 if pcm.ndim == 1 else pcm.shape[1]
    with wave.open(path, "wb") as w:
        w.setnchannels(channels)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(pcm.astype("<i2").tobytes())


def _out(path):
    out = os.path.abspath(path)
    if os.path.commonpath([out, REPO]) == REPO:
        sys.exit(f"{out} is inside the repository; game data must stay outside it")
    return out


def main(argv=None):
    from s4package import Package, parse_key
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    for name in ("info", "wav"):
        p = sub.add_parser(name)
        p.add_argument("package")
        p.add_argument("key")
        if name == "wav":
            p.add_argument("out")
    p = sub.add_parser("effect", help="find an effect by name in the client packages and describe it")
    p.add_argument("sims")
    p.add_argument("name")
    p.add_argument("out", nargs="?")
    args = parser.parse_args(argv)
    if args.command == "effect":
        from s4icons import Resources
        from s4gfx import fnv64
        res = Resources(args.sims, types=(PROPX_TYPE, SNR_TYPE))
        h = fnv64(args.name)
        effect = res.read((PROPX_TYPE, 0, h))
        variants = propx_variants(effect) if effect else [h]
        print(f"{args.name}: FNV64 {h:016X}, effect {'found' if effect else 'missing'}, variants "
              + ", ".join(f"{v:016X}" for v in variants))
        for v in variants:
            data = res.read_any(SNR_TYPE, v)
            if data is None:
                print(f"  {v:016X}: no SNR")
                continue
            snr = Snr(data)
            print(f"  {v:016X}: {snr.codec_name}, {snr.channels} ch, {snr.rate} Hz, {snr.samples} samples")
            if args.out and snr.codec in (XAS, PCM16BE):
                write_wav(_out(args.out), snr.decode(), snr.rate)
                print(f"  -> {args.out}")
                break
        return
    data = Package(args.package).read(parse_key(args.key))
    snr = Snr(data)
    print(f"{snr.codec_name}, {snr.channels} ch, {snr.rate} Hz, {snr.samples} samples, {len(snr.blocks)} blocks")
    if args.command == "wav":
        write_wav(_out(args.out), snr.decode(), snr.rate)
        print(f"-> {args.out}")


if __name__ == "__main__":
    main()
