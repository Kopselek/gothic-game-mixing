"""The base game of a mashup: Gothic 1 or Gothic 2 Night of the Raven, and where they differ for the tools.

Both run on the ZenGin engine and share the folder layout (system, Data\\*.vdf, _work\\DATA) and
the file formats. What differs and matters here:
  Gothic 1 (Steam/GOG with the Player Kit 1.08k)   system\\GothicMod.exe
  Gothic 2 Night of the Raven (Gold, 2.6 fix)      system\\Gothic2.exe, the addon's Data\\*_Addon.vdf
Gothic 2 has no HUMANS.MDS source, only the compiled HUMANS.MSB (humans_mds() reads either).
"""
import functools
import glob
import os
import re
import struct

from man import root_track
from msb import to_mds
from vdf import VdfReader

GOTHIC_1, GOTHIC_2 = 1, 2
TITLE = {GOTHIC_1: "Gothic 1", GOTHIC_2: "Gothic 2 Night of the Raven"}
EXE = {GOTHIC_1: "GothicMod.exe", GOTHIC_2: "Gothic2.exe"}


def version(gothic_dir):
    """GOTHIC_1 or GOTHIC_2 by the game's exe in system, None if the folder has neither."""
    for v in (GOTHIC_2, GOTHIC_1):
        if os.path.exists(os.path.join(gothic_dir, "system", EXE[v])):
            return v
    return None


def check(gothic_dir, wanted):
    """Problems with `gothic_dir` as the folder of Gothic `wanted` (1 or 2)."""
    found = version(gothic_dir)
    if found is None:
        if wanted == GOTHIC_1:
            return ["system\\GothicMod.exe is missing: choose the Gothic 1 folder (the one with system and Data in "
                    "it). Steam and GOG have it; a disk edition needs the Gothic 1 Player Kit 1.08k"]
        return ["system\\Gothic2.exe is missing: choose the Gothic 2 folder (the one with system and Data in it)"]
    if found != wanted:
        return [f"that folder holds {TITLE[found]}, not {TITLE[wanted]}: choose the {TITLE[wanted]} folder, or "
                f"{TITLE[found]} as the base game"]
    if found == GOTHIC_2 and not glob.glob(os.path.join(gothic_dir, "Data", "*_Addon.vdf")):
        return ["Gothic 2 without Night of the Raven: the mashups need the addon (Gothic II Gold Edition has it)"]
    return []


def anims_vdfs(gothic_dir):
    """Gothic's animation archives, the addon's first (its files override the base game's)."""
    data = os.path.join(gothic_dir, "Data")
    names = {os.path.basename(p).lower(): p for p in glob.glob(os.path.join(data, "*.[vV][dD][fF]"))}
    return [names[n] for n in ("anims_addon.vdf", "anims.vdf") if n in names]


def model_script(gothic_dir, model="HUMANS"):
    """The text of a model script of the game (HUMANS, an overlay such as HUMANS_1HST1): Gothic 1's
    MDS source, or Gothic 2's compiled MSB turned back into MDS."""
    model = model.upper()
    for path in anims_vdfs(gothic_dir):
        archive = VdfReader(path)
        for suffix, read in ((f"/{model}.MDS", lambda b: b.decode("latin-1")),
                             (f"/_COMPILED/{model}.MSB", lambda b: to_mds(b, model))):
            key = next((k for k in archive.files if k.upper().endswith(suffix)), None)
            if key:
                return read(archive.read(key))
    raise FileNotFoundError(f"no {model}.MDS or {model}.MSB in {gothic_dir}\\Data's animation archives")


def humans_mds(gothic_dir):
    """The text of Gothic's human model script (model_script)."""
    return model_script(gothic_dir, "HUMANS")


def model_scripts(gothic_dir, prefix):
    """The names of the game's model scripts that start with `prefix` (e.g. HUMANS_: the overlays)."""
    names = set()
    for path in anims_vdfs(gothic_dir):
        for key in VdfReader(path).files:
            name = key.upper().rsplit("/", 1)[-1]
            if name.startswith(prefix.upper()) and name.endswith((".MDS", ".MSB")):
                names.add(name[:-4])
    return sorted(names)


@functools.lru_cache(maxsize=None)
def _compiled_animations(gothic_dir):
    """{(MODEL, ANI): (archive, file name)} of the compiled <MODEL>-<ANI>.MAN files, the addon's first."""
    found = {}
    for path in anims_vdfs(gothic_dir):
        archive = VdfReader(path)
        for key in archive.files:
            name = key.upper().rsplit("/", 1)[-1]
            if "/_COMPILED/" in key.upper() and "-" in name and name.endswith(".MAN"):
                model, ani = name[:-len(".MAN")].split("-", 1)
                found.setdefault((model, ani), (archive, key))
    return found


def compiled_animation(gothic_dir, name, model="HUMANS"):
    """The compiled <model>-<name>.MAN, the addon's if it has one; None if the game has none."""
    entry = _compiled_animations(gothic_dir).get((model.upper(), name.upper()))
    return entry[0].read(entry[1]) if entry else None


def _compiled_animation(gothic_dir, name):
    return compiled_animation(gothic_dir, name)


def hierarchy(gothic_dir, model="HUMANS"):
    """A model's skeleton from its compiled <model>.MDH: ([(name, parent index or -1, 3x4 rows)],
    root position). Chunk 0xD100: uint32 version, uint16 node count, per node its name + "\\n", int16
    parent and the 4x4 transform from the parent (column vectors, row by row), then the model's
    two boxes and the root's place in the bind pose. Engine axes: x right, y up, z forward, cm."""
    for path in anims_vdfs(gothic_dir):
        archive = VdfReader(path)
        key = next((k for k in archive.files if k.upper().endswith(f"/_COMPILED/{model.upper()}.MDH")), None)
        if key:
            data = archive.read(key)
            break
    else:
        raise FileNotFoundError(f"no {model}.MDH in {gothic_dir}\\Data's animation archives")
    pos = 0
    while pos + 6 <= len(data):
        kind, size = struct.unpack_from("<HI", data, pos)
        body, pos = data[pos + 6:pos + 6 + size], pos + 6 + size
        if kind == 0xD100:
            count, = struct.unpack_from("<H", body, 4)
            p, nodes = 6, []
            for _ in range(count):
                end = body.index(b"\n", p)
                name, p = body[p:end].decode("latin-1").upper(), end + 1
                parent, = struct.unpack_from("<h", body, p)
                m = struct.unpack_from("<16f", body, p + 2)
                p += 66
                nodes.append((name, parent, [list(m[0:4]), list(m[4:8]), list(m[8:12])]))
            root = struct.unpack_from("<3f", body, p + 48)  # after the two boxes (2 x 6 floats)
            return nodes, root
    raise ValueError(f"{model}.MDH has no hierarchy chunk")


def mds_entries(text):
    """[(kind, args, events)] of a model script, a statement per line: Gothic 1's HUMANS.MDS has an
    ani line without its closing parenthesis (c_Ore_S1_3, before the aniComb s_Ore_S1), so a
    statement's arguments end at ')' or at the end of its line, and an event block belongs to the
    statement right before it."""
    out = []
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        m = _MDS_START.match(lines[i])
        i += 1
        if not m:
            continue
        kind = {k.lower(): k for k in ("ani", "aniAlias", "aniBlend", "aniComb", "aniDisable")}[m.group(1).lower()]
        args = m.group(2).split(")")[0]
        events = ""
        j = i
        while j < len(lines) and not lines[j].strip():
            j += 1
        if j < len(lines) and lines[j].strip().startswith("{"):
            block = []
            while j < len(lines):
                block.append(lines[j])
                if "}" in lines[j]:
                    break
                j += 1
            events, i = "\n".join(block), j + 1
        out.append((kind, args, events))
    return out


_MDS_START = re.compile(r"^\s*(aniAlias|aniBlend|aniComb|aniDisable|ani)\s*\((.*)$", re.I)


def root_motion(gothic_dir, ani):
    """Where Gothic's hero's root (Bip01) is in each frame of his animation `ani`, relative to
    where it is when he stands (the first frame of S_RUN), in 3ds Max axes as the ASC sources have
    them (x to the left, y backwards, z up; cm; engine (x, y, z) is max (-x, -z, y): the hero's
    left hand is at engine -x and at max +x): Gothic moves a character by this motion in animations flagged
    M, and in fly animations (F: ladders) also up and down. The frames are in the source's order
    (Gothic plays a reversed animation back to front). None if the game has no compiled `ani` or it
    leaves the root alone."""
    data, stand = _compiled_animation(gothic_dir, ani), _compiled_animation(gothic_dir, "S_RUN")
    track = root_track(data) if data is not None and stand is not None else None
    if track is None:
        return None
    sx, sy, sz = root_track(stand)[0]
    return [(-(x - sx), -(z - sz), y - sy) for x, y, z in track]
