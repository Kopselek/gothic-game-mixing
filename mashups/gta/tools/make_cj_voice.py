"""Give the Gothic 1 hero CJ's voice from GTA San Andreas.

Every hero line (voice 15) in Gothic's speech archives is replaced by random CJ
lines from his in-game speech banks, chained together so the new line roughly
matches the original's length (Gothic shows the subtitle for as long as the
sound plays). Pain and death screams use CJ's pain bank.

The result is one archive, Data\\CJ_Voice.vdf. Delete it to restore the original voice.

usage: python make_cj_voice.py --gta "D:\\Games\\GTA San Andreas" [--gothic DIR] [--seed N] [--out FILE]
Needs the classic PC GTA San Andreas files (audio\\config, audio\\sfx) and ffmpeg on PATH.
"""
import argparse
import glob
import os
import random
import re
import struct
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
from sa_audio import BANK_PAIN_CJ, BANKS_SPEECH_CJ, SfxBanks
from vdf import VdfReader, write_vdf

DEFAULT_GOTHIC = r"C:\Program Files (x86)\Steam\steamapps\common\Gothic"
OUTPUT_NAME = "CJ_Voice.vdf"
SPEECH_DIR = "_WORK/DATA/SOUND/SPEECH/"
HERO_LINE = re.compile(r"(_15_\d+[A-Z]?|/SVM_15_.*)\.WAV$")
PAIN_LINE = re.compile(r"/SVM_15_(AARGH|DEAD)")
RATE = 44100  # Gothic's own speech is 44.1 kHz mono IMA ADPCM
GAP = b"\0\0" * int(RATE * 0.25)
# no console window per ffmpeg call (one per line) when the GUI installer runs this
NO_WINDOW = getattr(subprocess, "CREATE_NO_WINDOW", 0)


def wav_seconds(header):
    pos, bytes_per_sec = 12, None
    while pos + 8 <= len(header):
        chunk, length = struct.unpack_from("<4sI", header, pos)
        if chunk == b"fmt ":
            bytes_per_sec = struct.unpack_from("<I", header, pos + 16)[0]
        elif chunk == b"data":
            return length / bytes_per_sec
        pos += 8 + length + (length & 1)
    raise ValueError("no data chunk")


def hero_lines(data_dir):
    """{archive path: seconds} for every hero voice line in the game's archives."""
    lines = {}
    for path in sorted(glob.glob(os.path.join(data_dir, "*.vdf"))):
        if os.path.basename(path).lower() == OUTPUT_NAME.lower():
            continue
        archive = VdfReader(path)
        for name in archive.files:
            if name.startswith(SPEECH_DIR) and HERO_LINE.search(name) and name not in lines:
                lines[name] = wav_seconds(archive.read(name, limit=512))
    return lines


def resample(pcm, rate):
    return subprocess.run(
        ["ffmpeg", "-v", "error", "-f", "s16le", "-ar", str(rate), "-ac", "1", "-i", "pipe:0",
         "-f", "s16le", "-ar", str(RATE), "pipe:1"],
        input=pcm, capture_output=True, check=True, creationflags=NO_WINDOW).stdout


def load_clips(banks, bank_ids):
    """All sounds of `bank_ids` as 16-bit mono PCM at RATE."""
    by_rate = {}
    for bank in bank_ids:
        for rate, pcm in banks.sounds(bank):
            by_rate.setdefault(rate, []).append(pcm[:len(pcm) // 2 * 2])
    clips = []
    for rate, pcms in by_rate.items():
        # One ffmpeg call per sample rate, then cut the clips back out.
        joined = b"".join(pcms)
        out = joined if rate == RATE else resample(joined, rate)
        ratio, pos = RATE / rate, 0
        for pcm in pcms:
            start, pos = round(pos * ratio), pos + len(pcm) // 2
            clips.append(out[start * 2:round(pos * ratio) * 2])
    return [c for c in clips if c]


def build_line(rng, clips, seconds):
    target = seconds * RATE * 2 * 0.85
    parts, total = [], 0
    while not parts or total < target:
        if parts:
            parts.append(GAP)
            total += len(GAP)
        clip = rng.choice(clips)
        parts.append(clip)
        total += len(clip)
    return b"".join(parts)


def encode(pcm, tmp_path):
    # ffmpeg can only finalize WAV chunk sizes on a seekable file, not a pipe.
    subprocess.run(
        ["ffmpeg", "-v", "error", "-y", "-f", "s16le", "-ar", str(RATE), "-ac", "1", "-i", "pipe:0",
         "-c:a", "adpcm_ima_wav", "-block_size", "1024", "-bitexact", tmp_path],
        input=pcm, capture_output=True, check=True, creationflags=NO_WINDOW)
    with open(tmp_path, "rb") as f:
        wav = bytearray(f.read())
    # ffmpeg writes a bitrate estimate as nAvgBytesPerSec; Gothic's files use the exact
    # value (rate * block_align / samples_per_block), which players use for the length.
    fmt = wav.index(b"fmt ") + 8
    align, samples_per_block = struct.unpack_from("<H", wav, fmt + 12)[0], struct.unpack_from("<H", wav, fmt + 18)[0]
    struct.pack_into("<I", wav, fmt + 8, RATE * align // samples_per_block)
    return bytes(wav)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--gta", required=True, help="GTA San Andreas (classic PC) install folder")
    parser.add_argument("--gothic", default=DEFAULT_GOTHIC, help="Gothic 1 install folder")
    parser.add_argument("--seed", default="cj", help="change it to reshuffle which CJ lines play where")
    parser.add_argument("--out", help=f"output archive (default: <gothic>\\Data\\{OUTPUT_NAME})")
    args = parser.parse_args()

    banks = SfxBanks(args.gta)
    speech = load_clips(banks, BANKS_SPEECH_CJ)
    pain = load_clips(banks, [BANK_PAIN_CJ]) or speech
    print(f"CJ clips: {len(speech)} speech, {len(pain)} pain")

    lines = hero_lines(os.path.join(args.gothic, "Data"))
    print(f"Gothic hero lines: {len(lines)} ({sum(lines.values()) / 60:.0f} min)")

    with tempfile.TemporaryDirectory() as tmp:
        def make(item):
            index, (name, seconds) = item
            rng = random.Random(f"{args.seed}:{name}")
            pcm = rng.choice(pain) if PAIN_LINE.search(name) else build_line(rng, speech, seconds)
            return name, encode(pcm, os.path.join(tmp, f"{index}.wav"))

        with ThreadPoolExecutor(os.cpu_count()) as pool:
            files = dict(pool.map(make, enumerate(sorted(lines.items()))))

    out = args.out or os.path.join(args.gothic, "Data", OUTPUT_NAME)
    write_vdf(out, files, comment="CJ voice for the Gothic 1 hero (GTA San Andreas speech banks)")
    print(f"Wrote {len(files)} lines to {out} ({os.path.getsize(out) / 2**20:.0f} MB)")


if __name__ == "__main__":
    main()
