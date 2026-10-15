"""Read skinned RenderWare clumps (.dff) from GTA San Andreas.

Only what a skinned ped needs: the frame hierarchy (names, matrices, HAnim bone ids),
geometry (positions, UVs, triangles, material ids), materials (texture names) and the
skin (per-vertex bone indices/weights and inverse bind matrices).
"""
import struct

STRUCT, STRING, EXTENSION = 0x01, 0x02, 0x03
TEXTURE, MATERIAL, MATERIAL_LIST = 0x06, 0x07, 0x08
FRAME_LIST, GEOMETRY, CLUMP, ATOMIC, GEOMETRY_LIST = 0x0E, 0x0F, 0x10, 0x14, 0x1A
SKIN, HANIM, FRAME_NAME = 0x116, 0x11E, 0x253F2FE

GEO_TRISTRIP, GEO_POSITIONS, GEO_TEXTURED, GEO_PRELIT = 0x01, 0x02, 0x04, 0x08
GEO_NORMALS, GEO_TEXTURED2, GEO_NATIVE = 0x10, 0x80, 0x01000000


def chunks(data, start=0, end=None):
    """Yield (type, payload_start, payload_end) for the chunks in data[start:end]."""
    pos, end = start, len(data) if end is None else end
    while pos + 12 <= end:
        kind, size, _ = struct.unpack_from("<III", data, pos)
        yield kind, pos + 12, pos + 12 + size
        pos += 12 + size


def child(data, start, end, kind):
    return next(((s, e) for k, s, e in chunks(data, start, end) if k == kind), None)


class Frame:
    def __init__(self, name, matrix, parent):
        self.name, self.matrix, self.parent = name, matrix, parent  # matrix: local 4x4, row vectors
        self.bone_id = None


class Geometry:
    def __init__(self):
        self.positions, self.normals, self.uvs, self.triangles = [], [], [], []  # triangles: (a, b, c, material)
        self.materials = []  # texture name or None
        self.material_colors = []  # (r, g, b, a) of each material
        self.bone_indices = self.bone_weights = self.inverse_bind = None


class Clump:
    def __init__(self, data):
        self.frames, self.geometries, self.atomics = [], [], []  # atomics: (frame, geometry)
        self.hanim_bones = []  # skin bone index -> HAnim bone id
        top = child(data, 0, None, CLUMP)
        for kind, s, e in chunks(data, *top):
            if kind == FRAME_LIST:
                self._read_frames(data, s, e)
            elif kind == GEOMETRY_LIST:
                for k, gs, ge in chunks(data, s, e):
                    if k == GEOMETRY:
                        self.geometries.append(self._read_geometry(data, gs, ge))
            elif kind == ATOMIC:
                st = child(data, s, e, STRUCT)
                self.atomics.append(struct.unpack_from("<II", data, st[0]))

    def _read_frames(self, data, start, end):
        it = chunks(data, start, end)
        _, s, _ = next(it)
        count = struct.unpack_from("<I", data, s)[0]
        for i in range(count):
            f = struct.unpack_from("<9f3fiI", data, s + 4 + i * 56)
            r, p = f[0:9], f[9:12]
            matrix = [[r[0], r[1], r[2], 0], [r[3], r[4], r[5], 0], [r[6], r[7], r[8], 0], [p[0], p[1], p[2], 1]]
            self.frames.append(Frame(f"frame{i}", matrix, f[12]))
        frame_exts = [(s2, e2) for k, s2, e2 in it if k == EXTENSION]
        for frame, (es, ee) in zip(self.frames, frame_exts):
            for k, s2, e2 in chunks(data, es, ee):
                if k == FRAME_NAME:
                    frame.name = data[s2:e2].split(b"\0")[0].decode("latin-1")
                elif k == HANIM:
                    _, bone_id, num_bones = struct.unpack_from("<iiI", data, s2)
                    frame.bone_id = bone_id
                    if num_bones:
                        for b in range(num_bones):
                            bid, _, _ = struct.unpack_from("<iii", data, s2 + 20 + b * 12)
                            self.hanim_bones.append(bid)

    def _read_geometry(self, data, start, end):
        g = Geometry()
        it = chunks(data, start, end)
        _, s, _ = next(it)
        flags, num_tris, num_verts, num_morph = struct.unpack_from("<IiiI", data, s)
        if flags & GEO_NATIVE:
            raise ValueError("native (platform) geometry is not supported")
        pos = s + 16
        if flags & GEO_PRELIT:
            pos += num_verts * 4
        num_uv_sets = (flags >> 16) & 0xFF or (2 if flags & GEO_TEXTURED2 else 1 if flags & GEO_TEXTURED else 0)
        for uv_set in range(num_uv_sets):
            uvs = struct.unpack_from(f"<{num_verts * 2}f", data, pos)
            if uv_set == 0:
                g.uvs = [uvs[i:i + 2] for i in range(0, len(uvs), 2)]
            pos += num_verts * 8
        for i in range(num_tris):
            v2, v1, mat, v3 = struct.unpack_from("<4H", data, pos + i * 8)
            g.triangles.append((v1, v2, v3, mat))
        pos += num_tris * 8
        pos += 16  # bounding sphere of morph target 0
        has_positions, has_normals = struct.unpack_from("<II", data, pos)
        pos += 8
        if has_positions:
            p = struct.unpack_from(f"<{num_verts * 3}f", data, pos)
            g.positions = [p[i:i + 3] for i in range(0, len(p), 3)]
            pos += num_verts * 12
        if has_normals:
            n = struct.unpack_from(f"<{num_verts * 3}f", data, pos)
            g.normals = [n[i:i + 3] for i in range(0, len(n), 3)]

        for kind, cs, ce in it:
            if kind == MATERIAL_LIST:
                g.materials = self._read_materials(data, cs, ce)
                g.material_colors = self._read_material_colors(data, cs, ce)
            elif kind == EXTENSION:
                skin = child(data, cs, ce, SKIN)
                if skin:
                    self._read_skin(data, skin[0], num_verts, g)
        return g

    def _read_materials(self, data, start, end):
        names = []
        for kind, s, e in chunks(data, start, end):
            if kind != MATERIAL:
                continue
            tex = child(data, s, e, TEXTURE)
            name = None
            if tex:
                strings = [(ts, te) for k, ts, te in chunks(data, *tex) if k == STRING]
                name = data[strings[0][0]:strings[0][1]].split(b"\0")[0].decode("latin-1")
            names.append(name)
        return names

    def _read_material_colors(self, data, start, end):
        """Material struct: flags, colour RGBA, unused, textured, ambient/specular/diffuse."""
        colors = []
        for kind, s, e in chunks(data, start, end):
            if kind == MATERIAL:
                st = child(data, s, e, STRUCT)
                colors.append(tuple(data[st[0] + 4:st[0] + 8]))
        return colors

    def _read_skin(self, data, pos, num_verts, g):
        num_bones, num_used, _, _ = struct.unpack_from("<4B", data, pos)
        pos += 4 + num_used
        idx = data[pos:pos + num_verts * 4]
        pos += num_verts * 4
        w = struct.unpack_from(f"<{num_verts * 4}f", data, pos)
        pos += num_verts * 16
        g.bone_indices = [tuple(idx[i * 4:i * 4 + 4]) for i in range(num_verts)]
        g.bone_weights = [w[i * 4:i * 4 + 4] for i in range(num_verts)]
        g.inverse_bind = []
        for _ in range(num_bones):
            m = struct.unpack_from("<16f", data, pos)
            g.inverse_bind.append([list(m[r * 4:r * 4 + 4]) for r in range(4)])
            pos += 64

    def world_matrix(self, index):
        """World matrix of frame `index` (row-vector convention: v_world = v_local * M)."""
        m = self.frames[index].matrix
        parent = self.frames[index].parent
        return m if parent < 0 else matmul(m, self.world_matrix(parent))


def matmul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]
