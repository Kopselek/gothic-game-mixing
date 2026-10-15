"""The Witcher 3's facial rig (.w3fac, CMimicFace): its bones and poses, and where a mix of poses
puts each face bone.

Layout (read from Geralt's characters\\models\\geralt\\head\\model\\h_01_mg__geralt.w3fac and
checked by decoding the whole buffer exactly):
  CMimicFace
    mimicSkeleton       the face rig (a CSkeleton, read as any rig: w3model.read_rig): torso3 at
                        its root, every face bone under a placer_<bone> at its rest place (the
                        bone's own rest is the identity), the placers under the head
    floatTrackSkeleton  a CSkeleton whose `tracks` name the poses: default, jaw_open_a,
                        jaw_open_o, ..., eyelids_upper_left_down, ... (148; the 19 after the
                        bone poses, normal0-15 and head1-3, are none)
    mapping             the face rig's bone index of each pose entry
    mimicPoses          uint32 pose count, then per pose: uint32 entry count (len(mapping)),
                        per entry a flag byte and the values it flags: 1 a position (3 floats,
                        metres), 2 a rotation (quaternion x, y, z, w), 4 a scale (3 floats)
A pose's entry is the bone's transform in its placer's space, the rest being the identity; the
game mixes poses by weight (its mimic animations are weights over time). Transforms here are
(rows, pos) with row vectors, as in make_geralt_model.
"""
import math
import struct

from cr2w import CR2W
from w3model import read_rig


def quat_rows(x, y, z, w):
    """Row-vector rotation of a Witcher 3 quaternion (as make_geralt_model.quat_rows)."""
    return [[1 - 2 * (y * y + z * z), 2 * (x * y + z * w), 2 * (x * z - y * w)],
            [2 * (x * y - z * w), 1 - 2 * (x * x + z * z), 2 * (y * z + x * w)],
            [2 * (x * z + y * w), 2 * (y * z - x * w), 1 - 2 * (x * x + y * y)]]


def _vecmat(v, m):
    return [sum(v[k] * m[k][j] for k in range(3)) for j in range(3)]


def compose(a, b):
    """a, then b."""
    return [_vecmat(r, b[0]) for r in a[0]], [x + y for x, y in zip(_vecmat(a[1], b[0]), b[1])]


def inverse(a):
    rows = [list(c) for c in zip(*a[0])]
    return rows, [-x for x in _vecmat(a[1], rows)]


IDENTITY = ([[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]], [0.0, 0.0, 0.0])


def _same(a, b, eps=1e-6):
    return all(abs(x - y) < eps for ra, rb in zip(a[0], b[0]) for x, y in zip(ra, rb)) and \
        all(abs(x - y) < eps for x, y in zip(a[1], b[1]))


class MimicFace:
    def __init__(self, source):
        f = source if isinstance(source, CR2W) else CR2W(source)
        face = f.of_type("CMimicFace")[0]
        self.rig = read_rig(f)
        self.names = [b.name for b in self.rig]
        tracks = f.of_type("CSkeleton")[1].props.get("tracks", [])
        self.tracks = [t["name"] for t in tracks]
        self.mapping = face.props["mapping"]
        self.poses = self._poses(face.props["mimicPoses"])
        self.head = self.names.index("head")
        self.rest = self.relative({})

    def _poses(self, buf):
        """Per pose: {face rig bone index: (position or None, quaternion or None)}."""
        pos = 4
        poses = []
        for _ in range(struct.unpack_from("<I", buf, 0)[0]):
            count, = struct.unpack_from("<I", buf, pos)
            pos += 4
            entries = {}
            for i in range(count):
                flag = buf[pos]
                pos += 1
                t = r = None
                if flag & 1:
                    t = struct.unpack_from("<3f", buf, pos)
                    pos += 12
                if flag & 2:
                    r = struct.unpack_from("<4f", buf, pos)
                    pos += 16
                if flag & 4:
                    pos += 12  # scale: Geralt's poses have none
                if flag:
                    entries[self.mapping[i]] = (t, r)
            poses.append(entries)
        if pos != len(buf):
            raise ValueError(f"mimic poses: {pos} of {len(buf)} bytes read")
        return poses

    def mix(self, weights):
        """{bone index: transform} of the poses mixed by `weights` ({pose name: weight}):
        positions summed, rotations as the normalised sum of their departures from the identity."""
        acc = {}
        for name, w in weights.items():
            if not w:
                continue
            for bone, (t, r) in self.poses[self.tracks.index(name)].items():
                p, q = acc.setdefault(bone, ([0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 1.0]))
                if t is not None:
                    for k in range(3):
                        p[k] += w * t[k]
                if r is not None:
                    for k in range(4):
                        q[k] += w * (r[k] - (0.0, 0.0, 0.0, 1.0)[k])
        out = {}
        for bone, (p, q) in acc.items():
            n = math.sqrt(sum(c * c for c in q)) or 1.0
            out[bone] = (quat_rows(*(c / n for c in q)), p)
        return out

    def relative(self, weights):
        """{bone name: transform} of every face rig bone relative to the head (its world in the
        head bone's space) for the poses mixed by `weights`."""
        deltas = self.mix(weights)
        world = []
        for i, b in enumerate(self.rig):
            local = (quat_rows(*b.rot), list(b.pos))
            if i in deltas:
                local = compose(deltas[i], local)
            world.append(compose(local, world[b.parent]) if b.parent >= 0 else local)
        to_head = inverse(world[self.head])
        return {name: compose(w, to_head) for name, w in zip(self.names, world)}

    def moved(self, weights):
        """Names of the bones the poses mixed by `weights` move."""
        posed = self.relative(weights)
        return {name for name, t in posed.items() if not _same(t, self.rest[name])}

    def parent_in(self, name, names):
        """The nearest of `name`'s ancestors among `names`, else "head"."""
        i = self.rig[self.names.index(name)].parent
        while i >= 0 and self.names[i] != "head":
            if self.names[i] in names:
                return self.names[i]
            i = self.rig[i].parent
        return "head"
