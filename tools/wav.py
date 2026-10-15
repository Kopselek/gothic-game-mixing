"""WAV files Gothic plays.

Effects (zsound->LoadSoundFX("NAME.WAV"), item/animation sounds): 16-bit PCM, mono, 44.1 or
22.05 kHz, in a VDF under _WORK\\DATA\\SOUND\\SFX\\NAME.WAV (or the folder on disk).

Speech (dialogue): _WORK\\DATA\\SOUND\\SPEECH\\<OU name>.WAV, Gothic's own are 44.1 kHz mono IMA
ADPCM. nAvgBytesPerSec must be exact (rate * block_align / samples_per_block): Gothic takes the
line's length (and so how long the subtitle shows) from it. ffmpeg writes an estimate; fix it
with fix_adpcm_rate() after encoding:
    ffmpeg -f s16le -ar 44100 -ac 1 -i in.raw -c:a adpcm_ima_wav -block_size 1024 -bitexact out.wav
The hero's lines are voice 15: files named *_15_*.WAV and SVM_15_*.WAV (pain: SVM_15_AARGH*,
SVM_15_DEAD*).
"""
import struct


def pcm16_wav(pcm, rate=44100, channels=1):
    """A PCM WAV around raw 16-bit little-endian samples."""
    align = 2 * channels
    return (b"RIFF" + struct.pack("<I", 36 + len(pcm)) + b"WAVEfmt "
            + struct.pack("<IHHIIHH", 16, 1, channels, rate, rate * align, align, 16)
            + b"data" + struct.pack("<I", len(pcm)) + pcm)


def wav_seconds(data):
    """Length in seconds from a WAV header (as Gothic computes it: data size / nAvgBytesPerSec)."""
    pos, bytes_per_sec = 12, None
    while pos + 8 <= len(data):
        cid, length = struct.unpack_from("<4sI", data, pos)
        if cid == b"fmt ":
            bytes_per_sec = struct.unpack_from("<I", data, pos + 16)[0]
        elif cid == b"data":
            return length / bytes_per_sec
        pos += 8 + length + (length & 1)
    raise ValueError("no data chunk")


def fix_adpcm_rate(data, rate=44100):
    """Set an IMA ADPCM WAV's nAvgBytesPerSec to the exact value Gothic expects."""
    wav = bytearray(data)
    fmt = wav.index(b"fmt ") + 8
    align = struct.unpack_from("<H", wav, fmt + 12)[0]
    samples_per_block = struct.unpack_from("<H", wav, fmt + 18)[0]
    struct.pack_into("<I", wav, fmt + 8, rate * align // samples_per_block)
    return bytes(wav)
