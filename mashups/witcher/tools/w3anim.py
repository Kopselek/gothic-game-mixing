"""Decode The Witcher 3's skeletal animations (.w2anims animation sets).

An animation set (CSkeletalAnimationSet) lists entries; each entry's CSkeletalAnimation has a
name, duration, framesPerSecond, an animation buffer and optionally a motion extraction (the
root motion, kept apart from the bone tracks).

CAnimationBufferBitwiseCompressed (layout as dingdio's Witcher3-Blender-Tools dc_anims.py reads
it, checked here on 4.04's files):
  numFrames, dt, duration, orientationCompressionMethod (or compressionSettings), bones: one
  {position, orientation, scale} per skeleton bone, each {dt, compression, numFrames, dataAddr},
  the frames' bytes at dataAddr of the animation data. The data is the `data` property, or the
  file's .N.buffer (`deferredData` = N), or for ABSO_PartiallyStreamable `data` followed by it.
  positions and scales: 3 floats a frame; compression 0 float32, 1 the top 3 bytes of a float32,
  2 the top 2 bytes of a float32 (not IEEE half)
  orientations: ABOCM_PackIn48bitsW 4x 12 bits of a big-endian 48-bit int, (2047 - v) / 2048;
  ABOCM_PackIn64bitsW 4x uint16, (32768 - v) / 32767; ABOCM_PackIn40bitsW 4x 10 bits of a
  big-endian 40-bit int, (511 - v) / 512; ABOCM_AsFloat_XYZSignedWInLastBit 3 floats, w from
  them; component order x, y, z, w
  A track with numFrames frames spreads them evenly over the duration; one frame is constant.
CLineMotionExtraction2: `flags` (1 x, 2 y, 4 z, 8 yaw), `frames` (the set components per key,
  cumulative from the start, metres), `deltaTimes` (frames at framesPerSecond between keys).
Events: after a CSkeletalAnimationSetEntry's properties, uint32 count, then per event
  {uint32 size, uint16 class name, struct}: eventName, startTime, duration and class fields
  (CPreAttackEvent: the hit is applied at the end of its window; CExtAnimDurationEvent
  ActionBlend: when the next action may start; ...).
"""
import math
import struct

from cr2w import CR2W

ORIENTATION_BYTES = {"ABOCM_PackIn48bitsW": 6, "ABOCM_PackIn64bitsW": 8, "ABOCM_PackIn40bitsW": 5,
                     "ABOCM_AsFloat_XYZSignedWInLastBit": 12}
# Motion extraction yaw is in radians, anticlockwise from above (Z up) positive, which turns the
# figure left (it faces +Y, its left is -X). Checked on Geralt's movement set: the starts to the
# side (locomotion_run_idle_to_left / _right) read +-pi/2 and travel to -X / +X, the 180-degree
# turns +-pi. The factor W3-Maya-AnimUtil uses (360 / -8.07) read them as -+70 and -+140 degrees.
YAW_FACTOR = 180.0 / math.pi


def _float(data, pos, compression):
    if compression == 1:
        return struct.unpack_from("<f", b"\0" + data[pos:pos + 3])[0], pos + 3
    if compression == 2:
        return struct.unpack_from("<f", b"\0\0" + data[pos:pos + 2])[0], pos + 2
    return struct.unpack_from("<f", data, pos)[0], pos + 4


def _vec3(data, pos, compression):
    x, pos = _float(data, pos, compression)
    y, pos = _float(data, pos, compression)
    z, pos = _float(data, pos, compression)
    return (x, y, z), pos


def _quat(data, pos, method, compression=0):
    if method == "ABOCM_PackIn48bitsW":
        bits = int.from_bytes(data[pos:pos + 6], "big")
        q = [(2047.0 - ((bits >> s) & 0xFFF)) / 2048.0 for s in (36, 24, 12, 0)]
        pos += 6
    elif method == "ABOCM_PackIn64bitsW":
        q = [(32768.0 - v) / 32767.0 for v in struct.unpack_from("<4H", data, pos)]
        pos += 8
    elif method == "ABOCM_PackIn40bitsW":
        bits = int.from_bytes(data[pos:pos + 5], "big")
        q = [(511.0 - ((bits >> s) & 0x3FF)) / 512.0 for s in (30, 20, 10, 0)]
        pos += 5
    else:
        (x, y, z), pos = _vec3(data, pos, compression)
        w = math.sqrt(max(0.0, 1.0 - min(x * x + y * y + z * z, 1.0)))
        if data[pos - 4] & 1:  # the sign of w rides in the last bit of z
            w = -w
        q = [x, y, z, w]
    n = math.sqrt(sum(c * c for c in q)) or 1.0
    return tuple(c / n for c in q), pos


class Track:
    """Evenly spaced keys over the animation (one key: constant)."""
    __slots__ = ("keys",)

    def __init__(self, keys):
        self.keys = keys

    def sample(self, t, duration, slerp=False):
        n = len(self.keys)
        if n == 1 or duration <= 0:
            return self.keys[0]
        f = max(0.0, min(1.0, t / duration)) * (n - 1)
        i = min(int(f), n - 2)
        a, b, k = self.keys[i], self.keys[i + 1], f - i
        if slerp:
            return _nlerp(a, b, k)
        return tuple(x + (y - x) * k for x, y in zip(a, b))


def _nlerp(a, b, k):
    if sum(x * y for x, y in zip(a, b)) < 0:
        b = tuple(-c for c in b)
    q = [x + (y - x) * k for x, y in zip(a, b)]
    n = math.sqrt(sum(c * c for c in q)) or 1.0
    return tuple(c / n for c in q)


class Animation:
    def __init__(self, name, duration, fps):
        self.name, self.duration, self.fps = name, duration, fps
        self.bones = []   # per skeleton bone index: (position Track, rotation Track, scale Track)
        self.motion = []  # [(time, x, y, z, yaw degrees)] root motion keys, cumulative
        self.events = []  # [Event] from the animation set entry

    def event(self, name):
        """The first event called `name` (CPreAttackEvent counts as "PreAttack"), or None."""
        return next((e for e in self.events if e.name == name), None)

    def pose(self, t):
        """[(pos, quat, scale)] of every bone at time t (parent space, Witcher 3 units)."""
        return [(p.sample(t, self.duration), r.sample(t, self.duration, True), s.sample(t, self.duration))
                for p, r, s in self.bones]

    def root_motion(self, t):
        """(x, y, z, yaw) the figure has moved by time t."""
        keys = self.motion
        if not keys:
            return (0.0, 0.0, 0.0, 0.0)
        if t <= keys[0][0]:
            return keys[0][1:]
        for a, b in zip(keys, keys[1:]):
            if t <= b[0]:
                k = (t - a[0]) / ((b[0] - a[0]) or 1)
                return tuple(x + (y - x) * k for x, y in zip(a[1:], b[1:]))
        return keys[-1][1:]


def _bytes(value):
    if isinstance(value, (bytes, bytearray)):
        return bytes(value)
    if isinstance(value, list):
        return bytes(v & 0xFF for v in value)
    return b""


def _buffer_data(f, buf):
    inline = _bytes(buf.props.get("data"))
    deferred = buf.props.get("deferredData", 0)
    if deferred:
        external = f.buffer(deferred)
        if buf.props.get("streamingOption") == "ABSO_PartiallyStreamable":
            return inline + external
        return external
    if inline:
        return inline
    return _bytes(buf.props.get("fallbackData"))


def _decode_buffer(f, buf, anim):
    data = _buffer_data(f, buf)
    method = buf.props.get("orientationCompressionMethod")
    if method is None:
        method = (buf.props.get("compressionSettings") or {}).get("orientationCompressionMethod", "ABOCM_PackIn64bitsW")
    for bone in buf.props.get("bones", []):
        tracks = []
        for kind in ("position", "orientation", "scale"):
            t = bone.get(kind, {})
            n = t.get("numFrames", 1) or 1
            pos = t.get("dataAddr", 0)
            comp = t.get("compression", 2 if kind == "scale" else 0)
            keys = []
            for _ in range(n):
                if kind == "orientation":
                    k, pos = _quat(data, pos, method, comp)
                else:
                    k, pos = _vec3(data, pos, comp)
                keys.append(k)
            tracks.append(Track(keys))
        anim.bones.append(tuple(tracks))


def _decode_motion(me, fps):
    flags = me.props.get("flags", 0)
    frames = me.props.get("frames", [])
    deltas = list(_bytes(me.props.get("deltaTimes")))
    comps = [bit for bit in (1, 2, 4, 8) if flags & bit]
    if not comps or not frames:
        return []
    keys, t, i = [], 0.0, 0
    for k in range(len(frames) // len(comps)):
        values = {bit: frames[i + j] for j, bit in enumerate(comps)}
        i += len(comps)
        keys.append((t, values.get(1, 0.0), values.get(2, 0.0), values.get(4, 0.0), values.get(8, 0.0) * YAW_FACTOR))
        if k < len(deltas):
            t += deltas[k] / fps
    return keys


class Event:
    __slots__ = ("cls", "name", "start", "duration", "props")

    def __init__(self, cls, props):
        self.cls, self.props = cls, props
        self.name = props.get("eventName") or {"CPreAttackEvent": "PreAttack"}.get(cls, cls)
        self.start = props.get("startTime", 0.0)
        self.duration = props.get("duration", 0.0)

    @property
    def end(self):
        return self.start + self.duration

    def __repr__(self):
        return f"<{self.name} {self.start:.2f}+{self.duration:.2f}>"


def _read_events(f, entry):
    data, pos, end = f.data, entry.extra, entry.data_offset + entry.data_size
    if pos + 4 > end:
        return []
    count, = struct.unpack_from("<I", data, pos)
    pos += 4
    events = []
    for _ in range(count):
        size, cls = struct.unpack_from("<IH", data, pos)
        props, _ = f._read_struct(pos + 6, pos + size)
        events.append(Event(f.names[cls], props or {}))
        pos += size
    return sorted(events, key=lambda e: e.start)


class AnimationSet:
    """All animations of a .w2anims file by name. `source`: a path or a CR2W."""

    def __init__(self, source):
        self.file = f = source if isinstance(source, CR2W) else CR2W(source)
        self.entries = {}
        for c in f.of_type("CSkeletalAnimation"):
            self.entries[c.props.get("name", "")] = c
        self.set_entries = {e.props.get("animation"): e for e in f.of_type("CSkeletalAnimationSetEntry")}

    def names(self):
        return sorted(self.entries)

    def get(self, name):
        c = self.entries[name]
        f = self.file
        anim = Animation(name, c.props.get("duration", 0.0), c.props.get("framesPerSecond", 30.0))
        buf = f.ref(c.props.get("animBuffer", 0))
        if buf is None:
            raise ValueError(f"{name}: no animation buffer")
        if buf.type == "CAnimationBufferMultipart":
            raise NotImplementedError(f"{name}: multipart animation buffers are not decoded yet")
        _decode_buffer(f, buf, anim)
        me = f.ref(c.props.get("motionExtraction", 0))
        if me is not None and me.type == "CLineMotionExtraction2":
            anim.motion = _decode_motion(me, anim.fps)
        entry = self.set_entries.get(c.index + 1)
        if entry is not None:
            anim.events = _read_events(f, entry)
        return anim
