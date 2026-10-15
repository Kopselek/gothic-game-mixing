"""The base game of a mashup: Gothic 1 or Gothic 2 Night of the Raven, and where they differ for the tools.

Both run on the ZenGin engine and share the folder layout (system, Data\\*.vdf, _work\\DATA) and
the file formats. What differs and matters here:
  Gothic 1 (Steam/GOG with the Player Kit 1.08k)   system\\GothicMod.exe
  Gothic 2 Night of the Raven (Gold, 2.6 fix)      system\\Gothic2.exe, the addon's Data\\*_Addon.vdf
Gothic 2 has no HUMANS.MDS source, only the compiled HUMANS.MSB (humans_mds() reads either).
"""
import glob
import os

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


def humans_mds(gothic_dir):
    """The text of Gothic's human model script: Gothic 1's HUMANS.MDS source, or Gothic 2's
    compiled HUMANS.MSB turned back into MDS."""
    for path in anims_vdfs(gothic_dir):
        archive = VdfReader(path)
        for suffix, read in (("/HUMANS.MDS", lambda b: b.decode("latin-1")),
                             ("/_COMPILED/HUMANS.MSB", lambda b: to_mds(b, "HUMANS"))):
            key = next((k for k in archive.files if k.upper().endswith(suffix)), None)
            if key:
                return read(archive.read(key))
    raise FileNotFoundError(f"no HUMANS.MDS or HUMANS.MSB in {gothic_dir}\\Data's animation archives")


def _compiled_animation(gothic_dir, name):
    """The compiled HUMANS-<name>.MAN, the addon's if it has one; None if the game has none."""
    suffix = f"/_COMPILED/HUMANS-{name.upper()}.MAN"
    for path in anims_vdfs(gothic_dir):
        archive = VdfReader(path)
        key = next((k for k in archive.files if k.upper().endswith(suffix)), None)
        if key:
            return archive.read(key)
    return None


def root_motion(gothic_dir, ani):
    """Where Gothic's hero's root (Bip01) is in each frame of his animation `ani`, relative to
    where it is when he stands (the first frame of S_RUN), in 3ds Max axes as the ASC sources have
    them (x, y backwards, z up; cm): Gothic moves a character by this motion in animations flagged
    M, and in fly animations (F: ladders) also up and down. None if the game has no compiled `ani`."""
    data, stand = _compiled_animation(gothic_dir, ani), _compiled_animation(gothic_dir, "S_RUN")
    if data is None or stand is None:
        return None
    sx, sy, sz = root_track(stand)[0]
    return [(x - sx, -(z - sz), y - sy) for x, y, z in root_track(data)]
