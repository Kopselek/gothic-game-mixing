"""Read GTA San Andreas IMG archives (VER2): "VER2", uint32 count, then 32-byte
entries {uint32 offset_sectors, uint16 streaming_sectors, uint16 archive_sectors,
char name[24]}; one sector is 2048 bytes."""
import struct

SECTOR = 2048


class ImgReader:
    def __init__(self, path):
        self.path = path
        self.files = {}  # lower-case name -> (offset_bytes, size_bytes)
        with open(path, "rb") as f:
            magic, count = struct.unpack("<4sI", f.read(8))
            if magic != b"VER2":
                raise ValueError(f"{path}: not a VER2 IMG archive")
            raw = f.read(count * 32)
        for i in range(count):
            offset, streaming, archive, name = struct.unpack_from("<IHH24s", raw, i * 32)
            name = name.split(b"\0")[0].decode("latin-1").lower()
            self.files[name] = (offset * SECTOR, (streaming or archive) * SECTOR)

    def read(self, name):
        offset, size = self.files[name.lower()]
        with open(self.path, "rb") as f:
            f.seek(offset)
            return f.read(size)
