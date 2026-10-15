"""Read Gothic's compiled animations (.MAN): the frames' node positions.

Gothic compiles each animation of a model script into _COMPILED\\<MODEL>-<ANI>.MAN (Anims.vdf).
The tools read the root's track from it: Gothic moves a character by its root node's motion in
animations flagged M, and a stand-in animation that keeps still leaves a ladder unclimbable.

A MAN is a run of chunks, each a uint16 type and a uint32 length (as ZenKit's ModelAnimation.cc
reads them): 0xA020 the header (uint16 version, name "\\n", uint32 layer, frame count, node count,
float fps, source fps, the positions' minimum and scale, a bounding box, next "\\n") and 0xA090 the
samples (uint32 checksum, uint32 node index per node, then frame after frame each node's rotation,
three uint16, and position, three uint16 times the scale plus the minimum). Positions are in
Gothic's engine axes: x, y up, z forward, centimetres.

  python man.py <file.MAN>
"""
import math
import struct
import sys

HEADER, SAMPLES = 0xA020, 0xA090


def read_man(data, rotations=False):
    """{name, layer, frames, nodes, fps, indices, positions}: positions[frame][node] = (x, y, z), the
    node's place in its parent's frame (the root's in the model's). With rotations=True also
    rotations[frame][node] = (x, y, z, w), the quaternion of its rotation in its parent's frame for
    column vectors. The file stores it conjugated, as three uint16: (v - 32767) * 2.1 / 65535 each,
    w from the unit length (ZenKit's read_sample_quaternion); the hero's standing frame of S_RUN
    put together from them lies on the bind pose only so."""
    out, pos = {}, 0
    while pos + 6 <= len(data):
        kind, size = struct.unpack_from("<HI", data, pos)
        body = data[pos + 6:pos + 6 + size]
        pos += 6 + size
        if kind == HEADER:
            end = body.index(b"\n", 2)
            out["name"] = body[2:end].decode("latin-1")
            out["layer"], out["frames"], out["nodes"] = struct.unpack_from("<3I", body, end + 1)
            out["fps"], _, minimum, scale = struct.unpack_from("<4f", body, end + 13)
        elif kind == SAMPLES:
            nodes, frames = out["nodes"], out["frames"]
            out["indices"] = struct.unpack_from(f"<{nodes}I", body, 4)
            p = 4 + 4 * nodes
            out["positions"] = [[tuple(c * scale + minimum for c in struct.unpack_from("<3H", body, p + (f * nodes + n) * 12 + 6))
                                 for n in range(nodes)] for f in range(frames)]
            if rotations:
                out["rotations"] = [[_quaternion(struct.unpack_from("<3H", body, p + (f * nodes + n) * 12))
                                     for n in range(nodes)] for f in range(frames)]
    return out


def _quaternion(raw):
    x, y, z = ((v - 32767) * (2.1 / 65535) for v in raw)
    square = x * x + y * y + z * z
    if square > 1.0:
        s = 1.0 / math.sqrt(square)
        x, y, z, w = x * s, y * s, z * s, 0.0
    else:
        w = math.sqrt(1.0 - square)
    return (-x, -y, -z, w)


def quaternion_matrix(q):
    """The 3x3 rotation (column vectors) of a unit quaternion (x, y, z, w)."""
    x, y, z, w = q
    return [[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
            [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
            [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]]


def root_track(data):
    """The root node's (model hierarchy node 0, Bip01) position in each frame, in the source's frame
    order (a reversed animation's file holds them as its source has them; Gothic plays them back to
    front); None if the animation leaves the root alone (one for the head or the arms only)."""
    man = read_man(data)
    if 0 not in man["indices"]:
        return None
    root = man["indices"].index(0)
    return [frame[root] for frame in man["positions"]]


if __name__ == "__main__":
    man = read_man(open(sys.argv[1], "rb").read())
    print(f"{man['name']}: {man['frames']} frames, {man['nodes']} nodes, {man['fps']:g} fps")
    for f, (x, y, z) in enumerate(root_track(open(sys.argv[1], "rb").read())):
        print(f"{f:4} {x:8.1f} {y:8.1f} {z:8.1f}")
