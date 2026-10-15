"""Extract sounds from GTA San Andreas (classic PC) SFX banks.

Layout taken from gta-reversed (source/game_sa/Audio/Loaders/AEBankLoader.h):
  AUDIO/CONFIG/PAKFILES.DAT  52-byte records, pak file name in the first 12 bytes
  AUDIO/CONFIG/BANKLKUP.DAT  12-byte records indexed by eSoundBank:
                             uint8 pak, 3 pad, uint32 file_offset (bytes), uint32 num_bytes
  AUDIO/SFX/<pak>            at file_offset: int16 num_sounds, int16 pad,
                             400 x {uint32 offset, int32 loop, uint16 rate, int16 headroom},
                             then num_bytes of 16-bit signed mono PCM
"""
import os
import struct

BANK_HEADER = struct.Struct("<hh")
SLOT = struct.Struct("<IiHh")
MAX_SOUNDS = 400
BANK_HEADER_SIZE = BANK_HEADER.size + MAX_SOUNDS * SLOT.size  # 4804

# eSoundBank values (gta-reversed source/game_sa/Audio/Enums/eSoundBank.h)
BANK_PAIN_CJ = 144                       # SND_BANK_PAIN_A_CARL
BANKS_SPEECH_CJ = list(range(690, 710))  # SND_BANK_SPC_PA_AG .. SND_BANK_SPC_PA_WR2


def _find(root, *parts):
    """Case-insensitive path lookup (the game ships mixed-case names)."""
    path = root
    for part in parts:
        match = next((e for e in os.listdir(path) if e.lower() == part.lower()), None)
        if match is None:
            raise FileNotFoundError(os.path.join(path, part))
        path = os.path.join(path, match)
    return path


class SfxBanks:
    def __init__(self, game_dir):
        self.game_dir = game_dir
        with open(_find(game_dir, "audio", "config", "pakfiles.dat"), "rb") as f:
            raw = f.read()
        self.paks = [raw[i:i + 12].split(b"\0")[0].decode("ascii") for i in range(0, len(raw), 52)]
        with open(_find(game_dir, "audio", "config", "banklkup.dat"), "rb") as f:
            raw = f.read()
        self.lookup = [struct.unpack_from("<B3xII", raw, i) for i in range(0, len(raw) - 11, 12)]

    def _bank(self, bank):
        pak, offset, num_bytes = self.lookup[bank]
        with open(_find(self.game_dir, "audio", "sfx", self.paks[pak]), "rb") as f:
            f.seek(offset)
            header = f.read(BANK_HEADER_SIZE)
            data = f.read(num_bytes)
        count, _ = BANK_HEADER.unpack_from(header)
        slots = [SLOT.unpack_from(header, BANK_HEADER.size + i * SLOT.size) for i in range(count)]
        return [(rate, data[start:slots[i + 1][0] if i + 1 < count else num_bytes])
                for i, (start, _, rate, _) in enumerate(slots)]

    def sounds(self, bank):
        """Yield (sample_rate, pcm_bytes) for every sound in `bank`."""
        for rate, pcm in self._bank(bank):
            if pcm and rate:
                yield rate, pcm

    def sound(self, bank, index):
        """(sample_rate, pcm_bytes) of sound `index` (the game's sound ID) in `bank`."""
        return self._bank(bank)[index]

    def loop(self, bank, index):
        """(sample_rate, pcm_bytes) of the part of sound `index` the game loops (from its loop
        start, in samples, to the end); the whole sound if it has no loop start."""
        pak, offset, _ = self.lookup[bank]
        with open(_find(self.game_dir, "audio", "sfx", self.paks[pak]), "rb") as f:
            f.seek(offset)
            header = f.read(BANK_HEADER_SIZE)
        _, loop_start, _, _ = SLOT.unpack_from(header, BANK_HEADER.size + index * SLOT.size)
        rate, pcm = self.sound(bank, index)
        return rate, pcm[max(loop_start, 0) * 2:]
