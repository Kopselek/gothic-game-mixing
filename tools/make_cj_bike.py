"""GTA San Andreas' motorbike for CJ in Gothic 1 (the Sanchez dirt bike unless --bike says
otherwise: any of GTA's motorbikes, e.g. pcj600, freeway, fcr900, nrg500, bf400, faggio).

The CJMovement plugin puts the bike into the world next to CJ; F gets him on and off, and he
rides it with the bike's numbers from GTA's handling.cfg. Writes:
  _work\\DATA\\MESHES\\CJ\\CJ_BIKE_CHASSIS.3DS   frame, tank, seat, engine and swing arm (bike space)
  _work\\DATA\\MESHES\\CJ\\CJ_BIKE_FRONT.3DS     forks, handlebars and mudguard, around the steering pivot
  _work\\DATA\\MESHES\\CJ\\CJ_BIKE_WHEELF.3DS    the front wheel, around its hub
  _work\\DATA\\MESHES\\CJ\\CJ_BIKE_WHEELR.3DS    the rear wheel, around its hub
  _work\\DATA\\TEXTURES\\_COMPILED\\CJ_BIKE_<n>-C.TEX   each material's texture times its colour (GTA
                                               tints vehicle textures by the material colour; the
                                               paint placeholders get the bike's first colours
                                               from carcols.dat, the light placeholders white)
  Data\\CJ_Bike.vdf     the engine loops of the bike's GTA engine banks (idle, revs, cruise), and
                        GTA's skid loops, crashes, suspension thud, water loop and ped hit
  system\\CJBike.ini    what the plugin needs: the bike's points and size, handling.cfg's numbers
                        and the paths CJ's centre takes getting on and off
Points in CJBike.ini are GTA metres in bike space: x right, y forward, z up. CJ's riding
animations go into CJ.MDS (make_cj_model.py, run it after this with the same --bike).

usage: python make_cj_bike.py --gta "D:\\Games\\GTA San Andreas" [--gothic DIR] [--bike sanchez]
"""
import argparse
import math
import os
import struct

from dff import Clump
from ifp import read_ifp
from img import ImgReader
from make_cj_pistol import chunk, cstr, wav
from sa_audio import SfxBanks
from tex import write_tex
from txd import read_txd
from vdf import write_vdf

DEFAULT_GOTHIC = r"C:\Program Files (x86)\Steam\steamapps\common\Gothic"
DEFAULT_BIKE = "sanchez"
SCALE = 100  # GTA metres -> Gothic centimetres
# frames of the bike's dff that make up each part the plugin moves
PARTS = {
    "CHASSIS": ["chassis", "forks_rear"],                # fixed to the bike (the swing arm at rest)
    "FRONT": ["forks_front", "handlebars", "mudguard"],  # turns with the steering, around forks_front
    "WHEELF": ["wheel_front"],                           # spin around their hubs
    "WHEELR": ["wheel_rear"],
}
PAINT = {(60, 255, 0): 0, (255, 0, 175): 1, (0, 255, 255): 2, (255, 0, 255): 3}  # carcols colour 1-4
LIGHTS = {(255, 175, 0), (0, 255, 200), (185, 255, 0), (255, 60, 0)}           # head and tail lights
# CAEVehicleAudioEntity's settings (gta-reversed AEVehicleAudioEntity.VehicleAudioSettings.h):
# bike -> (dummy engine bank, player engine bank, engine pitch)
ENGINE_BANKS = {
    "pcj600": (124, 125, 1.1892071), "freeway": (139, 140, 1.0), "sanchez": (47, 48, 1.0),
    "fcr900": (114, 115, 1.0594631), "nrg500": (124, 125, 1.0), "bf400": (124, 125, 1.0),
    "copbike": (40, 41, 1.0), "faggio": (118, 119, 1.0), "pizzaboy": (118, 119, 1.0), "wayfarer": (40, 41, 0.8908987),
}
# the engine loops: the dummy bank's revs (sound 0) and idle (sound 1), the player bank's cruise (sound 1)
SOUNDS = {"CJ_BIKE_REV": (0, 0), "CJ_BIKE_IDLE": (0, 1), "CJ_BIKE_CRUISE": (1, 1)}
# GTA's general sounds the plugin plays around the bike: name -> (bank, sound, loop)
# (gta-reversed Audio/Enums/eSoundBank.h and SoundIDs.h: SND_BANK_GENRL_VEHICLE_GEN 138,
# SND_BANK_GENRL_COLLISIONS 39)
GENERAL_SOUNDS = {
    "CJ_BIKE_SKID": (138, 0x18, True),         # TARSKIDTWIN1: skidding on hard ground
    "CJ_BIKE_SKID_GRASS": (138, 0x06, True),   # GRASS_SKID_A: on loose ground
    "CJ_BIKE_SKID_GRAVEL": (138, 0x08, True),  # GRAVEL_SKID_A: on wet ground
    "CJ_BIKE_BOUNCE": (138, 0x1F, False),      # BOUNCE_SUSPENSION: landing a jump
    "CJ_BIKE_WATER": (39, 0x03, True),         # WATER_LOOP: riding through water
    "CJ_BIKE_HITPED": (39, 0x1D, False),       # COLCARPED: running someone down
    **{f"CJ_BIKE_CRASH{i}": (39, 0x04 + i, False) for i in range(1, 6)},  # BIKE_CRASH1-5
}
# handling.cfg columns of a vehicle line and of a bike ("!") line
HANDLING = ["mass", "turnMass", "dragMult", "comX", "comY", "comZ", "percentSubmerged", "tractionMult",
            "tractionLoss", "tractionBias", "gears", "maxVelocity", "engineAccel", "engineInertia", "driveType",
            "engineType", "brakeDecel", "brakeBias", "abs", "steeringLock", "suspensionForce", "suspensionDamping",
            "suspensionHighSpeedDamping", "suspensionUpper", "suspensionLower", "suspensionBias", "antiDive"]
BIKE_HANDLING = ["leanFwdCOM", "leanFwdForce", "leanBakCOM", "leanBakForce", "maxLean", "fullAnimLean",
                 "desLean", "speedSteer", "slipSteer", "noPlayerCOMz", "wheelieAng", "stoppieAng",
                 "wheelieSteer", "wheelieStabMult", "stoppieStabMult"]


def vehicle_info(gta_dir, bike):
    """vehicles.ide: model, txd, handling id, anim group, front and rear wheel size."""
    for line in open(os.path.join(gta_dir, "data", "vehicles.ide"), encoding="latin-1"):
        p = [x.strip() for x in line.split("#")[0].split(",")]
        if len(p) >= 14 and p[0].isdigit() and p[1].lower() == bike.lower():
            if p[3].lower() != "bike":
                raise SystemExit(f"{bike} is a {p[3]}, not a motorbike")
            return {"id": int(p[0]), "model": p[1], "txd": p[2], "handling": p[4], "group": p[6].lower(),
                    "wheelFront": float(p[12]), "wheelRear": float(p[13])}
    raise SystemExit(f"{bike}: not in vehicles.ide")


def handling(gta_dir, handling_id):
    main = bike = None
    for line in open(os.path.join(gta_dir, "data", "handling.cfg"), encoding="latin-1"):
        p = line.split()
        if not p or p[0].startswith(";"):
            continue
        if p[0] == handling_id:
            main = dict(zip(HANDLING, p[1:]))
        elif p[0] == "!" and len(p) > 1 and p[1] == handling_id:
            bike = dict(zip(BIKE_HANDLING, p[2:]))
    if not main or not bike:
        raise SystemExit(f"{handling_id}: no vehicle or bike line in handling.cfg")
    return main, bike


def paint(gta_dir, bike):
    """carcols.dat: the colour table and the bike's first colour combination."""
    table, colours, section = {}, None, None
    for line in open(os.path.join(gta_dir, "data", "carcols.dat"), encoding="latin-1"):
        text = line.split("#")[0].strip()
        if text in ("col", "car", "car4", "end"):
            section = text
            continue
        if not text:
            continue
        if section == "col":
            r, g, b = (int(v) for v in text.replace(".", ",").split(",")[:3])
            table[len(table)] = (r, g, b)
        elif section in ("car", "car4"):
            p = [v.strip() for v in text.split(",")]
            if p[0].lower() == bike.lower() and colours is None:
                colours = [int(v) for v in p[1:1 + (4 if section == "car4" else 2)]]
    colours = colours or [1, 1]
    return [table[c] for c in colours] + [(255, 255, 255)] * (4 - len(colours))


def bike_anims(package):
    """GTA names of the bike animations in the group's IFP, by role. The get on/off animations
    are told apart by which way the root travels (GTA's own L/R names are of the movement)."""
    def find(suffix):
        return next(n for n in package if n.lower().endswith("_" + suffix.lower()))

    def root_x(name, index):
        keys = next(t for t in package[name] if t.bone_id == 0).keys
        keys = [k for k in keys if k[2]]
        return keys[index][2][0]

    names = {role: find(role) for role in ("Ride", "Still", "Left", "Right", "Fwd", "Back", "Pushes")}
    for a, b in (("jumponL", "jumponR"), ("getoffLHS", "getoffRHS")):
        a, b = find(a), find(b)
        if a.lower().endswith("jumponl"):  # mounting ends on the seat: from the left it moves right
            names["MountL"], names["MountR"] = (a, b) if root_x(a, -1) > 0 else (b, a)
        else:  # getting off starts on the seat: off to the left it starts right of the end point
            names["DismountL"], names["DismountR"] = (a, b) if root_x(a, 0) > 0 else (b, a)
    return names


def root_path(tracks):
    """CJ's centre (the root's translation, GTA ped space = bike space while he gets on or
    off) at Gothic's 25 fps, metres."""
    from make_cj_model import animation_frames, sample
    root = next(t for t in tracks if t.bone_id == 0)
    out = []
    for f in range(animation_frames(tracks)):
        _, p = sample(root.keys, f / 25)
        out.append(tuple(p) if p else (0.0, 0.0, 0.0))
    return out


# --- meshes ---------------------------------------------------------------------------------
def transform(v, m, translate=True):
    return [sum(v[k] * m[k][j] for k in range(3)) + (m[3][j] if translate else 0.0) for j in range(3)]


def write_3ds_multi(path, name, verts, uvs, faces, materials):
    """A one-object 3D Studio mesh with several textured materials, smooth shaded. faces:
    (a, b, c, material index) counter-clockwise from outside; materials: [(name, texture)]."""
    white = chunk(0x0011, bytes((255, 255, 255)))
    mats = b"".join(chunk(0xAFFF, chunk(0xA000, cstr(n)) + chunk(0xA010, white) + chunk(0xA020, white)
                          + chunk(0xA200, chunk(0x0030, struct.pack("<H", 100)) + chunk(0xA300, cstr(t))))
                    for n, t in materials)
    mesh = chunk(0x4110, struct.pack("<H", len(verts)) + b"".join(struct.pack("<3f", *v) for v in verts))
    mesh += chunk(0x4140, struct.pack("<H", len(uvs)) + b"".join(struct.pack("<2f", *uv) for uv in uvs))
    mesh += chunk(0x4160, struct.pack("<12f", 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0))
    groups = b""
    for m, (n, _) in enumerate(materials):
        idx = [i for i, f in enumerate(faces) if f[3] == m]
        if idx:
            groups += chunk(0x4130, cstr(n) + struct.pack("<H", len(idx)) + struct.pack(f"<{len(idx)}H", *idx))
    smooth = chunk(0x4150, struct.pack(f"<{len(faces)}I", *([1] * len(faces))))
    mesh += chunk(0x4120, struct.pack("<H", len(faces)) + b"".join(struct.pack("<4H", a, b, c, 7) for a, b, c, _ in faces)
                  + groups + smooth)
    obj = chunk(0x4000, cstr(name) + chunk(0x4100, mesh))
    editor = chunk(0x3D3D, chunk(0x3D3E, struct.pack("<I", 3)) + mats + chunk(0x0100, struct.pack("<f", 1.0)) + obj)
    with open(path, "wb") as f:
        f.write(chunk(0x4D4D, chunk(0x0002, struct.pack("<I", 3)) + editor))


class Materials:
    """Gothic materials for GTA's (texture, colour) pairs, each with its own baked texture."""

    def __init__(self, textures, colours, out_dir):
        self.textures, self.colours, self.out_dir = textures, colours, out_dir
        self.index = {}

    def get(self, texture, colour):
        rgb = tuple(colour[:3])
        rgb = self.colours[PAINT[rgb]] if rgb in PAINT else (255, 255, 255) if rgb in LIGHTS else rgb
        key = (texture.lower() if texture else None, rgb)
        if key not in self.index:
            name = f"CJ_BIKE_{len(self.index)}"
            t = self.textures.get(key[0]) if key[0] else None
            if t is None:  # untextured (or a texture GTA makes at runtime, such as the plate): the colour
                w = h = 8
                rgba = bytes(rgb + (255,)) * (w * h)
            else:
                w, h = t.width, t.height
                src = t.rgba
                rgba = bytearray(len(src))
                for i in range(0, len(src), 4):
                    for c in range(3):
                        rgba[i + c] = src[i + c] * rgb[c] // 255
                    rgba[i + 3] = 255
                rgba = bytes(rgba)
            write_tex(os.path.join(self.out_dir, name + "-C.TEX"), w, h, rgba)
            self.index[key] = name
        return self.index[key]


def build_parts(clump, materials):
    """{part: (verts cm, uvs, faces, [(material, texture)])}, the part's origin, and every
    point of the bike (metres, bike space) for its size."""
    by_name = {f.name.lower(): i for i, f in enumerate(clump.frames)}
    atomics = {f: g for f, g in clump.atomics}
    world = {i: clump.world_matrix(i) for i in range(len(clump.frames))}
    origins = {"CHASSIS": [0.0, 0.0, 0.0], "FRONT": world[by_name["forks_front"]][3][:3],
               "WHEELF": world[by_name["wheel_front"]][3][:3], "WHEELR": world[by_name["wheel_rear"]][3][:3]}
    parts, points = {}, []
    for part, frames in PARTS.items():
        verts, uvs, faces, mats = [], [], [], []
        for frame in frames:
            index = by_name.get(frame)
            if index is None or index not in atomics:
                continue
            g = clump.geometries[atomics[index]]
            m = world[index]
            base = len(verts)
            for p in g.positions:
                if part.startswith("WHEEL"):  # around the hub, keeping the frame's own turn
                    q = transform(p, m, translate=False)
                else:
                    q = [a - o for a, o in zip(transform(p, m), origins[part])]
                points.append([a + o for a, o in zip(q, origins[part])])
                verts.append([c * SCALE for c in q])
            uvs += [(u, 1.0 - v) for u, v in g.uvs] if g.uvs else [(0.0, 0.0)] * len(g.positions)
            local = {}
            for a, b, c, mat in g.triangles:
                if mat not in local:
                    name = materials.get(g.materials[mat], g.material_colors[mat])
                    if name not in [n for n, _ in mats]:
                        mats.append((name, name + ".TGA"))
                    local[mat] = [n for n, _ in mats].index(name)
                faces.append((base + a, base + b, base + c, local[mat]))
        parts[part] = (verts, uvs, faces, mats)
    return parts, origins, points


def wheel_radius(parts, part):
    """Largest distance of the wheel's points from its axle (the bike's x axis)."""
    verts = parts[part][0]
    return max(math.hypot(v[1], v[2]) for v in verts) / SCALE


def build(gta_dir, gothic_dir, bike=DEFAULT_BIKE):
    info = vehicle_info(gta_dir, bike)
    main, bike_line = handling(gta_dir, info["handling"])
    models = os.path.join(gta_dir, "models")
    gta3 = ImgReader(os.path.join(models, "gta3.img"))
    clump = Clump(gta3.read(info["model"] + ".dff"))
    textures = {k.lower(): v for k, v in read_txd(open(os.path.join(models, "generic", "vehicle.txd"), "rb").read()).items()}
    textures.update({k.lower(): v for k, v in read_txd(gta3.read(info["txd"] + ".txd")).items()})
    tex_dir = os.path.join(gothic_dir, "_work", "DATA", "TEXTURES", "_COMPILED")
    mesh_dir = os.path.join(gothic_dir, "_work", "DATA", "MESHES", "CJ")
    os.makedirs(tex_dir, exist_ok=True)
    os.makedirs(mesh_dir, exist_ok=True)
    colours = paint(gta_dir, info["model"])
    materials = Materials(textures, colours, tex_dir)
    parts, origins, points = build_parts(clump, materials)
    compiled = os.path.join(gothic_dir, "_work", "DATA", "MESHES", "_COMPILED")
    for part, (verts, uvs, faces, mats) in parts.items():
        name = "CJ_BIKE_" + part
        write_3ds_multi(os.path.join(mesh_dir, name + ".3DS"), name, verts, uvs, faces, mats)
        stale = os.path.join(compiled, name + ".MRM")  # Gothic recompiles from the new source
        if os.path.exists(stale):
            os.remove(stale)

    # the plugin's data
    frames = {f.name.lower(): i for i, f in enumerate(clump.frames)}
    seat = clump.world_matrix(frames["ped_frontseat"])[3][:3]
    pivot, hub_f, hub_r = origins["FRONT"], origins["WHEELF"], origins["WHEELR"]
    axis = [p - h for p, h in zip(pivot, hub_f)]  # the forks: from the front hub up to the pivot
    length = math.sqrt(sum(a * a for a in axis))
    axis = [a / length for a in axis]
    lo = [min(p[k] for p in points) for k in range(3)]
    hi = [max(p[k] for p in points) for k in range(3)]
    package = read_ifp(ImgReader(os.path.join(gta_dir, "anim", "anim.img")).read(info["group"] + ".ifp"))
    names = bike_anims(package)

    def vec(v):
        return ",".join(f"{c:.4f}" for c in v)

    lines = [f"; {info['model']} from GTA San Andreas for the CJMovement plugin (generated by make_cj_bike.py)",
             "; points in metres, bike space: x right, y forward, z up", "[BIKE]",
             f"name={info['model']}", f"animGroup={info['group']}", f"seat={vec(seat)}", f"pivot={vec(pivot)}",
             f"steerAxis={vec(axis)}", f"hubFront={vec(hub_f)}", f"hubRear={vec(hub_r)}",
             f"radiusFront={wheel_radius(parts, 'WHEELF'):.4f}", f"radiusRear={wheel_radius(parts, 'WHEELR'):.4f}",
             f"boxMin={vec(lo)}", f"boxMax={vec(hi)}", "[HANDLING]"]
    lines += [f"{k}={v}" for k, v in main.items()]
    lines += ["[BIKEHANDLING]"] + [f"{k}={v}" for k, v in bike_line.items()]
    dummy, player, pitch = ENGINE_BANKS.get(info["model"].lower(), ENGINE_BANKS["sanchez"])
    lines += ["[SOUND]", f"enginePitch={pitch}"]
    lines += ["; CJ's centre while getting on and off, 25 frames a second: the animation's root path",
              "[PATHS]"]
    for role in ("MountL", "MountR", "DismountL", "DismountR"):
        path = root_path(package[names[role]])
        lines.append(f"{role}={len(path)};" + ";".join(vec(p) for p in path))
    with open(os.path.join(gothic_dir, "system", "CJBike.ini"), "w", newline="\r\n") as f:
        f.write("\n".join(lines) + "\n")

    banks = SfxBanks(gta_dir)
    files = {}
    for name, (which, sound) in SOUNDS.items():
        rate, pcm = banks.loop(player if which else dummy, sound)
        files[f"_WORK\\DATA\\SOUND\\SFX\\{name}.WAV"] = wav_at(pcm, rate)
    for name, (bank, sound, loop) in GENERAL_SOUNDS.items():
        rate, pcm = banks.loop(bank, sound) if loop else banks.sound(bank, sound)
        files[f"_WORK\\DATA\\SOUND\\SFX\\{name}.WAV"] = wav_at(pcm, rate)
    write_vdf(os.path.join(gothic_dir, "Data", "CJ_Bike.vdf"), files, comment=f"CJ's {info['model']}: GTA San Andreas bike sounds")
    print(f"bike: {info['model']} ({info['handling']}, anim group {info['group']}), {len(materials.index)} materials, "
          f"parts {', '.join(f'{p} {len(v[2])} faces' for p, v in parts.items())}, {len(files)} sounds")
    return info


def wav_at(pcm, rate):
    """Mono 16-bit PCM WAV at the sound's own rate."""
    data = wav(pcm)
    return data[:24] + struct.pack("<II", rate, rate * 2) + data[32:]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--gta", required=True, help="GTA San Andreas (classic PC) install folder")
    parser.add_argument("--gothic", default=DEFAULT_GOTHIC, help="Gothic 1 install folder")
    parser.add_argument("--bike", default=DEFAULT_BIKE, help="GTA model name of the motorbike (vehicles.ide)")
    args = parser.parse_args()
    build(args.gta, args.gothic, args.bike)


if __name__ == "__main__":
    main()
