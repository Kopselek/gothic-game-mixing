"""Read and write Gothic's 3ds Max ASCII export (.ASC) models.

Gothic compiles these itself: a soft-skinned body (*MESH_SOFTSKIN + *MESH_SOFTSKINVERTS)
becomes an .MDM, a static head mesh referenced from an .MMS becomes an .MMB.
Coordinates are 3ds Max world space: Z up, centimetres, row-vector matrices.
"""
import math
import re


def _block(text, start):
    """Return the end index of the {...} block whose '{' is at or after `start`."""
    depth, i = 0, text.index("{", start)
    while True:
        c = text[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1


def read_helpers(text):
    """[(name, parent, world_matrix 4x4, block_text)] for every *HELPEROBJECT, in file order."""
    nodes = []
    for m in re.finditer(r"^\*HELPEROBJECT\s*\{", text, re.M):
        block = text[m.start():_block(text, m.start())]
        name = re.search(r'\*NODE_NAME\s+"([^"]*)"', block).group(1)
        parent = re.search(r'\*NODE_PARENT\s+"([^"]*)"', block)
        rows = [list(map(float, re.search(rf"\*TM_ROW{i}\s+(\S+)\s+(\S+)\s+(\S+)", block).groups())) for i in range(4)]
        matrix = [r + [0.0] for r in rows[:3]] + [rows[3] + [1.0]]
        nodes.append((name, parent.group(1) if parent else None, matrix, block))
    return nodes


def read_softskin(text):
    """(vertices, faces, tverts, tfaces, weights) of the first soft-skin mesh."""
    verts = [tuple(map(float, m.groups())) for m in re.finditer(r"\*MESH_VERTEX\s+\d+\s+(\S+)\s+(\S+)\s+(\S+)", text)]
    faces = [tuple(map(int, m.groups())) for m in re.finditer(r"\*MESH_FACE\s+\d+:\s+A:\s+(\d+)\s+B:\s+(\d+)\s+C:\s+(\d+)", text)]
    tverts = [tuple(map(float, m.groups())) for m in re.finditer(r"\*MESH_TVERT\s+\d+\s+(\S+)\s+(\S+)", text)]
    tfaces = [tuple(map(int, m.groups())) for m in re.finditer(r"\*MESH_TFACE\s+\d+\s+(\d+)\s+(\d+)\s+(\d+)", text)]
    weights = []
    m = re.search(r"\*MESH_SOFTSKINVERTS\s*\{", text)
    if m:
        lines = text[m.end():_block(text, m.start()) - 1].strip().splitlines()[2:]
        for line in lines:
            parts = re.findall(r'"[^"]*"|\S+', line)
            weights.append([(parts[1 + 2 * i].strip('"'), float(parts[2 + 2 * i])) for i in range(int(parts[0]))])
    return verts, faces, tverts, tfaces, weights


def _fmt(x):
    return f"{x:.6g}"


def _materials(textures):
    if len(textures) == 1:
        return _material(0, textures[0], "\t").replace("*SUBMATERIAL", "*MATERIAL", 1)
    subs = "".join(_material(i, t, "\t\t") for i, t in enumerate(textures))
    return (f'\t*MATERIAL 0 {{\n\t\t*MATERIAL_NAME "Multi"\n\t\t*MATERIAL_CLASS "Multi/Sub-Object"\n'
            f"\t\t*NUMSUBMTLS {len(textures)}\n{subs}\t}}\n")


def _material(index, texture, ind):
    name = texture.rsplit(".", 1)[0]
    return (f"{ind}*SUBMATERIAL {index} {{\n"
            f'{ind}\t*MATERIAL_NAME "{name}"\n'
            f'{ind}\t*MATERIAL_CLASS "Standard"\n'
            f"{ind}\t*MATERIAL_DIFFUSE 1\t1\t1\n"
            f"{ind}\t*MAP_DIFFUSE {{\n"
            f'{ind}\t\t*MAP_NAME "{name}"\n'
            f'{ind}\t\t*MAP_CLASS "Bitmap"\n'
            f'{ind}\t\t*BITMAP "{texture}"\n'
            f"{ind}\t\t*BITMAP_FILTER Pyramidal\n"
            f"{ind}\t}}\n"
            f"{ind}}}\n")


# Gothic builds a node's transform from the decomposed TM_POS/ROTAXIS/ROTANGLE/SCALE fields,
# not only from the rows: leaving them out flipped a compiled head by 180 degrees.
IDENTITY_TM = ("\t*NODE_TM {{\n\t\t*NODE_NAME \"{0}\"\n\t\t*TM_ROW0 1\t0\t0\n\t\t*TM_ROW1 0\t1\t0\n"
               "\t\t*TM_ROW2 0\t0\t1\n\t\t*TM_ROW3 0\t0\t0\n\t\t*TM_POS 0\t0\t0\n\t\t*TM_ROTAXIS 0\t0\t0\n"
               "\t\t*TM_ROTANGLE 0\n\t\t*TM_SCALE 1\t1\t1\n\t\t*TM_SCALEAXIS 0\t0\t0\n\t\t*TM_SCALEAXISANG 0\t0\t0\n\t}}\n")


def _mesh(keyword, verts, faces, uvs, uv_faces):
    out = [f"\t*{keyword} {{\n\t\t*TIMEVALUE 0\n",
           f"\t\t*MESH_NUMVERTEX {len(verts)}\n\t\t*MESH_NUMFACES {len(faces)}\n\t\t*MESH_VERTEX_LIST {{\n"]
    out += [f"\t\t\t*MESH_VERTEX {i}\t{_fmt(v[0])}\t{_fmt(v[1])}\t{_fmt(v[2])}\n" for i, v in enumerate(verts)]
    out.append("\t\t}\n\t\t*MESH_FACE_LIST {\n")
    out += [f"\t\t\t*MESH_FACE {i}:\tA: {a} B: {b} C: {c} AB: 1 BC: 1 CA: 1\t *MESH_SMOOTHING 1 \t*MESH_MTLID {m}\n"
            for i, (a, b, c, m) in enumerate(faces)]
    out.append(f"\t\t}}\n\t\t*MESH_NUMTVERTEX {len(uvs)}\n\t\t*MESH_TVERTLIST {{\n")
    out += [f"\t\t\t*MESH_TVERT {i}\t{_fmt(u)}\t{_fmt(v)}\t0\n" for i, (u, v) in enumerate(uvs)]
    out.append(f"\t\t}}\n\t\t*MESH_NUMTVFACES {len(uv_faces)}\n\t\t*MESH_TFACELIST {{\n")
    out += [f"\t\t\t*MESH_TFACE {i}\t{a}\t{b}\t{c}\n" for i, (a, b, c) in enumerate(uv_faces)]
    out.append("\t\t}\n\t}\n")
    return "".join(out)


HEADER = ("*3DSMAX_ASCIIEXPORT\t110\n*COMMENT \"gothic-game-mixing\"\n*SCENE {{\n\t*SCENE_FILENAME \"{0}\"\n"
          "\t*SCENE_FIRSTFRAME 0\n\t*SCENE_LASTFRAME 100\n\t*SCENE_FRAMESPEED 25\n\t*SCENE_TICKSPERFRAME 192\n"
          "\t*SCENE_BACKGROUND_STATIC 0\t0\t0\n\t*SCENE_AMBIENT_STATIC 0.2\t0.2\t0.2\n}}\n"
          "*MATERIAL_LIST {{\n\t*MATERIAL_COUNT {2}\n{1}}}\n")


def axis_angle(rows):
    """Axis and angle as Gothic's ASC files store them for a row-vector rotation matrix.

    Verified against the game's own animation sources: the stored (axis, angle) is the
    standard right-handed rotation whose column-vector matrix equals these rows.
    """
    m = rows
    w = math.sqrt(max(0.0, 1 + m[0][0] + m[1][1] + m[2][2])) / 2
    x = math.copysign(math.sqrt(max(0.0, 1 + m[0][0] - m[1][1] - m[2][2])) / 2, m[2][1] - m[1][2])
    y = math.copysign(math.sqrt(max(0.0, 1 - m[0][0] + m[1][1] - m[2][2])) / 2, m[0][2] - m[2][0])
    z = math.copysign(math.sqrt(max(0.0, 1 - m[0][0] - m[1][1] + m[2][2])) / 2, m[1][0] - m[0][1])
    s = math.sqrt(x * x + y * y + z * z)
    if s < 1e-9:
        return (0.0, 0.0, 1.0), 0.0
    return (x / s, y / s, z / s), 2 * math.atan2(s, w)


def node_tm(name, rows, pos, indent="\t"):
    (ax, ay, az), angle = axis_angle(rows)
    i = indent
    return (f'{i}*NODE_TM {{\n{i}\t*NODE_NAME "{name}"\n'
            + "".join(f"{i}\t*TM_ROW{k} {_fmt(r[0])}\t{_fmt(r[1])}\t{_fmt(r[2])}\n" for k, r in enumerate(rows))
            + f"{i}\t*TM_ROW3 {_fmt(pos[0])}\t{_fmt(pos[1])}\t{_fmt(pos[2])}\n"
            + f"{i}\t*TM_POS {_fmt(pos[0])}\t{_fmt(pos[1])}\t{_fmt(pos[2])}\n"
            + f"{i}\t*TM_ROTAXIS {_fmt(ax)}\t{_fmt(ay)}\t{_fmt(az)}\n{i}\t*TM_ROTANGLE {_fmt(angle)}\n"
            + f"{i}\t*TM_SCALE 1\t1\t1\n{i}\t*TM_SCALEAXIS 0\t0\t0\n{i}\t*TM_SCALEAXISANG 0\t0\t0\n{i}}}\n")


def helper_block(name, parent, rows, pos, animation=None):
    """A skeleton node; `animation` is [(rows, pos)] per frame in world space, or None."""
    out = [f'*HELPEROBJECT {{\n\t*NODE_NAME "{name}"\n']
    if parent:
        out.append(f'\t*NODE_PARENT "{parent}"\n')
    out.append(node_tm(name, rows, pos))
    if animation:
        out.append(f'\t*TM_ANIMATION {{\n\t\t*NODE_NAME "{name}"\n\t\t*CONTROL_POS_TRACK {{\n')
        out += [f"\t\t\t*CONTROL_POS_SAMPLE {f * 192}\t{_fmt(p[0])}\t{_fmt(p[1])}\t{_fmt(p[2])}\n"
                for f, (_, p) in enumerate(animation)]
        out.append("\t\t}\n\t\t*CONTROL_ROT_TRACK {\n")
        for f, (r, _) in enumerate(animation):
            (ax, ay, az), angle = axis_angle(r)
            out.append(f"\t\t\t*CONTROL_ROT_SAMPLE {f * 192}\t{_fmt(ax)}\t{_fmt(ay)}\t{_fmt(az)}\t{_fmt(angle)}\n")
        out.append("\t\t}\n\t}\n")
    out.append("}\n")
    return "".join(out)


def write_animation(path, helper_blocks, frames):
    """Animation source: every skeleton node, animated ones carrying world-space samples."""
    name = path.replace("\\", "/").rsplit("/", 1)[-1]
    header = (f"*3DSMAX_ASCIIEXPORT\t110\n*COMMENT \"gothic-game-mixing\"\n*SCENE {{\n\t*SCENE_FILENAME \"{name}\"\n"
              f"\t*SCENE_FIRSTFRAME 0\n\t*SCENE_LASTFRAME {frames - 1}\n\t*SCENE_FRAMESPEED 25\n"
              "\t*SCENE_TICKSPERFRAME 192\n\t*SCENE_BACKGROUND_STATIC 0\t0\t0\n\t*SCENE_AMBIENT_STATIC 0.2\t0.2\t0.2\n}\n")
    with open(path, "w", newline="\r\n") as f:
        f.write(header + "".join(helper_blocks))


def write_softskin(path, helper_blocks, textures, verts, faces, uvs, uv_faces, weights, mesh_name="zm_Skin"):
    """Soft-skinned body: skeleton helpers, one mesh in world space, per-vertex [(bone, weight)]."""
    write_softskin_meshes(path, helper_blocks, textures, [(mesh_name, verts, faces, uvs, uv_faces, weights)])


def write_softskin_meshes(path, helper_blocks, textures, meshes, separate=False):
    """Soft-skinned body made of several meshes, each its own *GEOMOBJECT with its
    *MESH_SOFTSKINVERTS: meshes = [(name, verts, faces, uvs, uv_faces, weights)].

    Gothic 1 renders one soft skin only below 2048 positions (zCProgMeshProto::RenderProgMesh
    returns at posList count >= 0x800, and zCMeshSoftSkin::RenderSoftSkin writes every skinned
    position into the static s_posCacheList without a bound, so a bigger one corrupts memory and
    crashes); a model takes a list of soft skins (zCModelMeshLib::meshSoftSkinList).

    Gothic's ASC loader reads no *MESH_MTLID (GothicMod.exe has no such string): every face of a
    GEOMOBJECT gets the first texture of the material its *MATERIAL_REF names, so a Multi material
    painted all of Geralt with his face. With separate=True every texture is a material of its
    own in the MATERIAL_LIST and each mesh names its one material as a seventh item:
    (name, verts, faces, uvs, uv_faces, weights, material index)."""
    name = path.replace("\\", "/").rsplit("/", 1)[-1]
    if separate:
        mats = "".join(_material(i, t, "\t").replace("*SUBMATERIAL", "*MATERIAL", 1) for i, t in enumerate(textures))
        out = [HEADER.format(name, mats, len(textures))]
    else:
        out = [HEADER.format(name, _materials(textures), 1)]
    out += [b if b.endswith("\n") else b + "\n" for b in helper_blocks]
    for mesh in meshes:
        mesh_name, verts, faces, uvs, uv_faces, weights = mesh[:6]
        out.append(f'*GEOMOBJECT {{\n\t*NODE_NAME "{mesh_name}"\n\t*NODE_PARENT "Bip01"\n')
        out.append(IDENTITY_TM.format(mesh_name))
        out.append(f"\t*MATERIAL_REF {mesh[6] if separate else 0}\n")
        out.append(_mesh("MESH_SOFTSKIN", verts, faces, uvs, uv_faces))
        out.append("}\n")
        out.append(f"*MESH_SOFTSKINVERTS {{\n{mesh_name}\n{len(verts)}\n")
        for links in weights:
            out.append(f"{len(links)}\t" + "\t".join(f'"{bone}"\t{_fmt(w)}' for bone, w in links) + "\n")
        out.append("}\n")
    with open(path, "w", newline="\r\n") as f:
        f.write("".join(out))


def write_mesh(path, textures, verts, faces, uvs, uv_faces, mesh_name="zm_Morph"):
    """Static mesh in its own local space (a morph-mesh head is placed on 'Bip01 Head')."""
    name = path.replace("\\", "/").rsplit("/", 1)[-1]
    out = [HEADER.format(name, _materials(textures), 1),
           f'*GEOMOBJECT {{\n\t*NODE_NAME "{mesh_name}"\n', IDENTITY_TM.format(mesh_name), "\t*MATERIAL_REF 0\n",
           _mesh("MESH", verts, faces, uvs, uv_faces), "}\n"]
    with open(path, "w", newline="\r\n") as f:
        f.write("".join(out))
