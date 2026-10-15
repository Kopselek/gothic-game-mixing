"""Read GTA San Andreas animation packages (.ifp, "ANP3").

  "ANP3", uint32 size, char[24] package name, uint32 anim count, then per animation:
    char[24] name, uint32 object count, uint32 frame data size, uint32 unknown,
    per object: char[24] bone name, uint32 frame type, uint32 frame count, int32 bone id,
                frames: int16 qx, qy, qz, qw (/4096), int16 time (/60 s)
                        [+ int16 tx, ty, tz (/1024) when frame type is 2 or 4]
Rotations are the bone's local rotation (relative to its parent); only root-like bones
carry translations.
"""
import struct


class Track:
    def __init__(self, name, bone_id, keys):
        self.name, self.bone_id = name, bone_id
        self.keys = keys  # [(time_s, (qx, qy, qz, qw), (tx, ty, tz) or None)]


def read_ifp(data):
    magic, _, _, count = struct.unpack_from("<4sI24sI", data, 0)
    if magic != b"ANP3":
        raise ValueError(f"unsupported IFP version {magic!r}")
    pos = 36
    anims = {}
    for _ in range(count):
        name, objects, _, _ = struct.unpack_from("<24sIII", data, pos)
        pos += 36
        tracks = []
        for _ in range(objects):
            bone, kind, frames, bone_id = struct.unpack_from("<24sIIi", data, pos)
            pos += 36
            with_t = kind in (2, 4)
            keys = []
            for _ in range(frames):
                qx, qy, qz, qw, t = struct.unpack_from("<5h", data, pos)
                pos += 10
                trans = None
                if with_t:
                    trans = tuple(v / 1024 for v in struct.unpack_from("<3h", data, pos))
                    pos += 6
                keys.append((t / 60, (qx / 4096, qy / 4096, qz / 4096, qw / 4096), trans))
            tracks.append(Track(bone.split(b"\0")[0].decode("latin-1"), bone_id, keys))
        anims[name.split(b"\0")[0].decode("latin-1")] = tracks
    return anims
