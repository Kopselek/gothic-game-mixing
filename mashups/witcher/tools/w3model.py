"""Decode The Witcher 3's skeletons (.w2rig) and meshes (.w2mesh + .1.buffer).

Witcher 3 space: metres, Z up, right-handed; characters face +Y with their right side at +X
(Geralt's toes lie at +Y of his ankles, his right leg at +X).

Skeleton (CSkeleton chunk): properties `bones` (names) and `parentIndices`; after the properties
the reference pose, 48 bytes a bone: position xyzw, rotation quaternion xyzw, scale xyzw, each
relative to the parent bone.

Mesh (CMesh chunk; layout matches dingdio's Witcher3-Blender-Tools reader, checked here on 4.04's
files):
  `chunks`: submeshes {vertexType, numBonesPerVertex (default 4), numVertices, numIndices,
            firstVertex, firstIndex, materialID}
  `cookedData`: quantizationScale/Offset (positions), renderBuffer (the .buffer holding the data),
            indexBufferOffset, and renderChunks, bytes: a count, then 37 bytes a vertex block:
            uint8, uint32 positions offset, uv offset, normals offset, colour offset, other offset,
            uint8, uint32 index offset, uint8, uint16 vertices, uint32 indices, uint8 material, 3x uint8
  after the properties: LOD groups (VLQ count of {bit6 count, uint16 submesh indices, float
            distance}), bone names (VLQ count of CName), inverse bind matrices (VLQ count of
            16 floats, row vectors), per-bone epsilons (VLQ count of floats), a uint32 mapping
            (VLQ count)
  buffer streams per vertex: position 4x uint16 quantized (+ numBonesPerVertex bone indices and
            as many uint8 weights /255 for skinned meshes); UV 2x float16; normal and tangent
            10:10:10 packed uint32; indices uint16
"""
import struct

from cr2w import CR2W, read_bit6, read_vlq


class Bone:
    __slots__ = ("name", "parent", "pos", "rot", "scale")

    def __init__(self, name, parent, pos, rot, scale):
        self.name, self.parent, self.pos, self.rot, self.scale = name, parent, pos, rot, scale


def _open(source):
    return source if isinstance(source, CR2W) else CR2W(source)


def read_rig(source):
    """[Bone] in file order (parents come first), with each bone's reference pose relative to its
    parent. `source`: a path or a CR2W."""
    f = _open(source)
    skel = f.of_type("CSkeleton")[0]
    names = [b["name"] for b in skel.props["bones"]]
    parents = skel.props["parentIndices"]
    bones = []
    for i, name in enumerate(names):
        v = struct.unpack_from("<12f", f.data, skel.extra + 48 * i)
        bones.append(Bone(name, parents[i], v[0:3], v[4:8], v[8:11]))
    return bones


def material_params(f, chunk, install=None, depth=0):
    """{name: value} of a CMaterialInstance chunk, its base materials' values filling in what it
    leaves out (texture handles resolved to .xbm depot paths). After the properties a material
    instance stores uint32 count, then {uint32 size (counting itself), uint16 name, uint16 type,
    value} per parameter."""
    data, pos = f.data, chunk.extra
    count, = struct.unpack_from("<I", data, pos)
    pos += 4
    params = {}
    for _ in range(count):
        size, name, typ = struct.unpack_from("<IHH", data, pos)
        type_name = f.names[typ]
        value = f._value(type_name, pos + 8, pos + size)
        if type_name.startswith("handle:") and isinstance(value, int):
            value = f.imports[-value - 1][0] if value < 0 else f.ref(value)
        params[f.names[name]] = value
        pos += size
    base = chunk.props.get("baseMaterial")
    if install is not None and depth < 8 and isinstance(base, int) and base < 0:
        path = f.imports[-base - 1][0]
        if path.lower().endswith(".w2mi") and path in install:
            bf = CR2W(install.read(path), path, install)
            for k, v in material_params(bf, bf.of_type("CMaterialInstance")[0], install, depth + 1).items():
                params.setdefault(k, v)
    return params


def entity_meshes(install, path, depth=0):
    """[(mesh depot path, component name)] that an entity template (.w2ent) shows: the mesh
    components in its compiled data (CMeshComponent, and the plain fallback mesh of a
    CFurComponent), then those of the templates it includes or uses as appearances."""
    f = CR2W(install.read(path), path, install)
    template = f.of_type("CEntityTemplate")[0]
    found = []
    flat = template.props.get("flatCompiledData")
    if isinstance(flat, bytes) and flat[:4] == b"CR2W":
        inner = CR2W(flat, path + "#compiled", install)
        for c in inner.chunks:
            handle = c.props.get("mesh")
            if isinstance(handle, int) and handle < 0 and c.type in ("CMeshComponent", "CFurComponent"):
                mesh = inner.imports[-handle - 1][0]
                if mesh.lower().endswith(".w2mesh"):
                    found.append((mesh, c.props.get("name", "")))
    if depth < 6:
        handles = list(template.props.get("includes", []) or [])
        for app in template.props.get("appearances", []) or []:
            handles += app.get("includedTemplates", []) or []
        for h in handles:
            if isinstance(h, int) and h < 0:
                sub = f.imports[-h - 1][0]
                if sub.lower().endswith(".w2ent") and sub in install:
                    found += entity_meshes(install, sub, depth + 1)
    return found


def entity_colorings(install, path):
    """{component name: (colorShift1, colorShift2)} of an entity template's default appearance."""
    f = CR2W(install.read(path), path, install)
    out = {}
    for e in f.of_type("CEntityTemplate")[0].props.get("coloringEntries", []) or []:
        if e.get("appearance") in ("default", None):
            out[e.get("componentName", "")] = (e.get("colorShift1"), e.get("colorShift2"))
    return out


class Submesh:
    def __init__(self):
        self.positions, self.normals, self.uvs = [], [], []
        self.bones, self.weights = [], []  # per vertex: mesh bone indices and weights (skinned meshes)
        self.triangles = []                 # (a, b, c) into this submesh's vertices
        self.material = 0
        self.lod = 0


class Mesh:
    """A decoded CMesh. `source`: a path or a CR2W (one read from the bundles finds its .buffer there)."""

    def __init__(self, source):
        f = self.file = _open(source)
        self.path = f.path
        mesh = f.of_type("CMesh")[0]
        props = mesh.props
        cooked = props["cookedData"]
        self.materials = [self._material(h) for h in props.get("materials", [])]
        self.bone_names, self.inverse_bind, self.lods = self._extra(mesh)
        lod_of = {i: lod for lod, (subs, _dist) in enumerate(self.lods) for i in subs}
        q_scale = [cooked["quantizationScale"][k] for k in "XYZ"]
        q_offset = [cooked["quantizationOffset"][k] for k in "XYZ"]
        buffer = f.buffer(cooked["renderBuffer"])
        blocks = self._render_chunks(cooked["renderChunks"])
        index_base = cooked["indexBufferOffset"]
        self.submeshes = []
        for i, info in enumerate(props.get("chunks", [])):
            sub = Submesh()
            sub.material = info.get("materialID", 0)
            sub.lod = lod_of.get(i, 0)
            skinned = info.get("vertexType") == "MVT_SkinnedMesh"
            nbpv = info.get("numBonesPerVertex", 4) if skinned else 0
            count, first = info.get("numVertices", 0), info.get("firstVertex", 0)
            n_idx, first_idx = info.get("numIndices", 0), info.get("firstIndex", 0)
            block, vstart = self._block(blocks, "vertices", first)
            _, istart = self._block(blocks, "indices", first_idx)
            stride = 8 + 2 * nbpv
            base = block["positions"] + vstart * stride
            for v in range(count):
                o = base + v * stride
                x, y, z = struct.unpack_from("<3H", buffer, o)
                sub.positions.append((x / 65535 * q_scale[0] + q_offset[0], y / 65535 * q_scale[1] + q_offset[1],
                                      z / 65535 * q_scale[2] + q_offset[2]))
                if nbpv:
                    ids = buffer[o + 8:o + 8 + nbpv]
                    ws = buffer[o + 8 + nbpv:o + 8 + 2 * nbpv]
                    sub.bones.append(tuple(ids))
                    sub.weights.append(tuple(w / 255 for w in ws))
            for v in range(count):
                u, w = struct.unpack_from("<2e", buffer, block["uvs"] + (vstart + v) * 4)
                sub.uvs.append((u, w))
                packed, = struct.unpack_from("<I", buffer, block["normals"] + (vstart + v) * 8)
                sub.normals.append(tuple(((packed >> s) & 0x3FF) * 2 / 1023 - 1 for s in (0, 10, 20)))
            o = index_base + block["indices"] + istart * 2
            idx = struct.unpack_from(f"<{n_idx}H", buffer, o)
            sub.triangles = [idx[t:t + 3] for t in range(0, n_idx - 2, 3)]
            self.submeshes.append(sub)

    def _material(self, handle):
        if handle < 0:
            return self.file.imports[-handle - 1][0]  # a .w2mi (or .w2mg) depot path
        chunk = self.file.ref(handle)
        return chunk  # a material instance inside the mesh file

    def _extra(self, mesh):
        data, pos = self.file.data, mesh.extra
        lods = []
        n, pos = read_vlq(data, pos)
        for _ in range(n):
            k, pos = read_bit6(data, pos)
            subs = struct.unpack_from(f"<{k}H", data, pos)
            pos += 2 * k
            dist, = struct.unpack_from("<f", data, pos)
            pos += 4
            lods.append((subs, dist))
        n, pos = read_vlq(data, pos)
        names = [self.file.names[i] for i in struct.unpack_from(f"<{n}H", data, pos)]
        pos += 2 * n
        n, pos = read_vlq(data, pos)
        mats = [struct.unpack_from("<16f", data, pos + 64 * i) for i in range(n)]
        return names, [[list(m[r * 4:r * 4 + 4]) for r in range(4)] for m in mats], lods

    @staticmethod
    def _render_chunks(raw):
        count = raw[0]
        blocks, pos = [], 1
        for _ in range(count):
            (_u, positions, uvs, normals, colors, _other, _u2, indices, _u3, n_vert, n_idx, material,
             _b1, _b2, _b3) = struct.unpack_from("<B5IBIBHIB3B", raw, pos)
            pos += 37
            blocks.append({"positions": positions, "uvs": uvs, "normals": normals, "colors": colors,
                           "indices": indices, "vertices_count": n_vert, "indices_count": n_idx})
        return blocks

    @staticmethod
    def _block(blocks, kind, first):
        """The vertex block holding a submesh's first vertex (or index), and the offset in it."""
        total = 0
        for b in blocks:
            n = b[f"{kind}_count"]
            if first < total + n:
                return b, first - total
            total += n
        raise ValueError(f"{kind} {first} beyond the mesh's vertex blocks")
