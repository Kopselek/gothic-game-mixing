"""Give the Gothic 1 hero Geralt's voice in every dialogue, the way the GTA mashup gave him CJ's.

Every hero line (voice 15) in Gothic's speech archives is replaced by Geralt's own voiced lines from
The Witcher 3, picked at random from all he says in the game's scenes (.w2scene lines with voicetag
GERALT, about 19,500 in the Polish dub): one line of about the same length where there is one,
else several chained so the new line roughly matches the original (Gothic shows the subtitle for as
long as the sound plays; the subtitles stay Gothic's). Every Witcher line is used once before any
is used again. Pain and death screams (SVM_15_AARGH, SVM_15_DEAD) take his effort grunts. Where the
hero gives his name (FIXED), Geralt gives his, and that subtitle changes with it.

The result is one archive, Data\\Geralt_Dialogue.vdf (newer than the game's, so it wins); delete it
to get the hero's own voice back. Change --seed to reshuffle which line plays where.

usage: python make_geralt_dialogue.py --w3 "<Witcher 3 folder>" [--gothic DIR] [--lang pl] [--seed geralt]
Needs ffmpeg on PATH.
"""
import argparse
import bisect
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
from vdf import VdfReader, write_vdf
from w3bundle import W3Install
from w3speech import W3Speech, wem_to_ogg
from w3strings import W3Strings
from w3voicelines import scene_lines

DEFAULT_GOTHIC = r"C:\Program Files (x86)\Steam\steamapps\common\Gothic"
DEFAULT_W3 = r"C:\Program Files (x86)\Steam\steamapps\common\The Witcher 3"
OUTPUT_NAME = "Geralt_Dialogue.vdf"
SPEECH_DIR = "_WORK/DATA/SOUND/SPEECH/"
HERO_LINE = re.compile(r"(_15_\d+[A-Z]?|/SVM_15_.*)\.WAV$")
PAIN_LINE = re.compile(r"/SVM_15_(AARGH|DEAD)")
VOICESET = "quests\\main_npcs\\voicesets\\geralt_vset.w2scene"
# Lines that keep their sense: the hero giving his name, cut off by Diego ("Jestem...") and by
# Xardas ("Nazywam się..."), say Geralt's (string ids of his own lines in the scenes, sq107 and
# ff302), and their subtitle becomes his words in the dub's own text (SUBTITLES, which the
# Witcher plugin sets in Gothic's output units).
FIXED = {"INFO_DIEGO_GAMESTART_15_01": 1004736,   # "Jestem Geralt z Rivii."
         "INFO_XARDAS_DISTURB_15_02": 504772}     # "Nazywam się Geralt."
SUBTITLES = "Witcher_subtitles.txt"
RATE = 44100  # Gothic's own speech is 44.1 kHz mono IMA ADPCM
GAP = b"\0\0" * int(RATE * 0.25)
SHORTEST, LONGEST = 0.6, 12.0  # Witcher lines used, in seconds (shorter ones are grunts)
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


def geralt_lines(install, speech):
    """({string id: seconds} of everything Geralt says in the scenes, [grunt ids])."""
    ids = set()
    for path in sorted(k for k in install.entries if k.endswith(".w2scene")):
        try:
            ids |= {line["id"] for line in scene_lines(install, path) if line["voicetag"] == "GERALT"}
        except (ValueError, NotImplementedError):
            continue
    seconds = speech.durations([i for i in ids if i in speech.entries])
    grunts = [line["id"] for line in scene_lines(install, VOICESET)
              if "BattleCryAttack" in line["inputs"] and line["id"] in speech.entries]
    return {i: s for i, s in seconds.items() if SHORTEST <= s <= LONGEST}, grunts


class Picker:
    """Witcher lines by length, each used once before any comes back."""

    def __init__(self, seconds, rng):
        self.all = sorted(seconds.items(), key=lambda kv: kv[1])
        self.rng = rng
        self.refill()

    def refill(self):
        self.free = list(self.all)
        self.lengths = [s for _, s in self.free]

    def take(self, low, high):
        """A random unused line between low and high seconds (None if there is none)."""
        i, j = bisect.bisect_left(self.lengths, low), bisect.bisect_right(self.lengths, high)
        if i >= j:
            return None
        k = self.rng.randrange(i, j)
        line = self.free.pop(k)
        self.lengths.pop(k)
        if not self.free:
            self.refill()
        return line

    def line_for(self, seconds):
        """[string ids] for a Gothic line of `seconds`: one of about that length, else a chain."""
        one = self.take(seconds * 0.85, seconds * 1.2)
        if one:
            return [one[0]]
        parts, total = [], 0.0
        while total < seconds * 0.85:
            rest = seconds - total
            line = self.take(min(rest * 0.6, LONGEST * 0.6), min(max(rest, SHORTEST * 2), LONGEST)) or self.take(SHORTEST, LONGEST)
            parts.append(line[0])
            total += line[1] + 0.25
        return parts


def decode(speech, string_id):
    """A Witcher line as 16-bit mono PCM at RATE."""
    return subprocess.run(["ffmpeg", "-v", "error", "-f", "ogg", "-i", "pipe:0", "-f", "s16le", "-ac", "1",
                           "-ar", str(RATE), "pipe:1"],
                          input=wem_to_ogg(speech.read_wem(string_id)), capture_output=True, check=True,
                          creationflags=NO_WINDOW).stdout


def encode(pcm, tmp_path):
    """16-bit mono PCM as Gothic's speech WAV (IMA ADPCM, as make_cj_voice.py writes it)."""
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


def build(w3_dir, gothic_dir, lang="pl", seed="geralt", out=None):
    content = os.path.join(w3_dir, "content", "content0")
    speech = W3Speech(os.path.join(content, f"{lang}pc.w3speech"))
    seconds, grunts = geralt_lines(W3Install(w3_dir), speech)
    print(f"Geralt's lines: {len(seconds)} ({sum(seconds.values()) / 3600:.1f} h), {len(grunts)} grunts")
    lines = hero_lines(os.path.join(gothic_dir, "Data"))
    print(f"Gothic hero lines: {len(lines)} ({sum(lines.values()) / 60:.0f} min)")
    rng = random.Random(seed)
    picker = Picker(seconds, rng)
    plan = {name: [rng.choice(grunts)] if PAIN_LINE.search(name) and grunts else picker.line_for(length)
            for name, length in sorted(lines.items())}
    texts = W3Strings(os.path.join(content, f"{lang}.w3strings")).text
    subtitles = []
    for name in plan:
        block = os.path.splitext(name.rsplit("/", 1)[-1])[0].upper()
        if block in FIXED and FIXED[block] in speech.entries:
            plan[name] = [FIXED[block]]
            subtitles.append((block, texts.get(FIXED[block], "")))
    with open(os.path.join(gothic_dir, SUBTITLES), "w", encoding="cp1250" if lang == "pl" else "cp1252",
              errors="replace", newline="\r\n") as f:
        f.write("# The hero's lines make_geralt_dialogue.py gave Geralt's words: output unit, tab, subtitle\n")
        f.writelines(f"{block}\t{text}\n" for block, text in subtitles)
    print("fixed lines: " + ", ".join(f"{block} = {text!r}" for block, text in subtitles))
    needed = sorted({i for ids in plan.values() for i in ids})
    with ThreadPoolExecutor(os.cpu_count()) as pool:
        pcm = dict(zip(needed, pool.map(lambda i: decode(speech, i), needed)))
    with tempfile.TemporaryDirectory() as tmp:
        def make(item):
            index, (name, ids) = item
            joined = GAP.join(pcm[i][:len(pcm[i]) // 2 * 2] for i in ids)
            return name, encode(joined, os.path.join(tmp, f"{index}.wav"))
        with ThreadPoolExecutor(os.cpu_count()) as pool:
            files = dict(pool.map(make, enumerate(sorted(plan.items()))))
    out = out or os.path.join(gothic_dir, "Data", OUTPUT_NAME)
    write_vdf(out, files, comment=f"Geralt's voice for the Gothic 1 hero (The Witcher 3, {lang} dub)")
    single = sum(1 for ids in plan.values() if len(ids) == 1)
    print(f"Wrote {len(files)} lines to {out} ({os.path.getsize(out) / 2**20:.0f} MB); "
          f"{single} by one Witcher line, {len(files) - single} chained, {len(needed)} Witcher lines used")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--w3", default=DEFAULT_W3, help="The Witcher 3 folder")
    parser.add_argument("--gothic", default=DEFAULT_GOTHIC, help="the Gothic 1 folder")
    parser.add_argument("--lang", default="pl", help="the dub: pl, en, de, ... (<lang>pc.w3speech)")
    parser.add_argument("--seed", default="geralt", help="change it to reshuffle which line plays where")
    parser.add_argument("--out", help=f"output archive (default: <gothic>\\Data\\{OUTPUT_NAME})")
    args = parser.parse_args()
    build(args.w3, args.gothic, args.lang, args.seed, args.out)


if __name__ == "__main__":
    main()
