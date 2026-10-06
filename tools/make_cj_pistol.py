"""GTA San Andreas' pistol (the Colt 45) and rocket launcher for CJ in Gothic 1.

Gothic's light crossbow becomes the pistol and its bolts the 9 mm rounds; the next crossbow
(ITRW_CROSSBOW_02, sold only by Wolf and Scorpio from chapter 4) becomes the rocket launcher, and
an alchemy ingredient no script or world uses (ITMI_ALCHEMY_SYRIANOIL_01) its rockets. The
CJMovement plugin draws both as Gothic's crossbow (so NPCs react to the drawn weapon as usual) and
aims and fires them as GTA does. Jackal's unused sword (JACKALSSWORD) becomes GTA's purple dildo
(gun_dildo1), a one-handed weapon CJ swings with GTA's dildo combo. Writes:
  _work\\DATA\\MESHES\\CJ\\CJ_COLT45.3DS          colt45.dff (gta3.img), in the hand's frame
  _work\\DATA\\MESHES\\CJ\\CJ_BULLET.3DS          a 9 mm round for the ammunition item
  _work\\DATA\\MESHES\\CJ\\CJ_GUNFLASH.3DS        colt45.dff's muzzle flash, moved from its "gunflash"
                                               frame into the hand's (the plugin shows it on a shot)
  _work\\DATA\\MESHES\\CJ\\CJ_ROCKETLA.3DS         rocketla.dff, in the hand's frame like the pistol
  _work\\DATA\\MESHES\\CJ\\CJ_MISSILE.3DS          missile.dff: the rocket item and the flying rocket
  _work\\DATA\\MESHES\\CJ\\CJ_DILDO.3DS            gun_dildo1.dff, in the hand's frame
  _work\\DATA\\TEXTURES\\_COMPILED\\CJ_COLT45-C.TEX, CJ_BULLET-C.TEX, CJ_GUNFLASH-C.TEX, CJ_ROCKETLA-C.TEX,
                                               CJ_MISSILE-C.TEX
  Data\\CJ_Weapons.vdf                           sounds from GTA's weapon and explosion banks (see SOUNDS)
and patches the four items in Gothic.dat (see patch_items).

usage: python make_cj_pistol.py --gta "D:\\Games\\GTA San Andreas" [--gothic DIR]
"""
import argparse
import math
import os
import shutil
import struct

from daedalus import OP_PUSH_I, OP_PUSH_V, OP_PUSH_VV, Script
from dff import Clump
from img import ImgReader
from sa_audio import SfxBanks
from tex import write_tex
from txd import read_txd
from vdf import write_vdf

DEFAULT_GOTHIC = r"C:\Program Files (x86)\Steam\steamapps\common\Gothic"
SCALE = 100  # GTA metres -> Gothic centimetres
PISTOL, AMMO = "ITRW_CROSSBOW_01", "ITAMBOLT"
PISTOL_MESH, BULLET_MESH, FLASH_MESH = "CJ_COLT45", "CJ_BULLET", "CJ_GUNFLASH"
LAUNCHER, ROCKETS = "ITRW_CROSSBOW_02", "ITMI_ALCHEMY_SYRIANOIL_01"
LAUNCHER_MESH, MISSILE_MESH = "CJ_ROCKETLA", "CJ_MISSILE"
DILDO, DILDO_MESH = "JACKALSSWORD", "CJ_DILDO"

# Gothic.dat: what the two items become (data/weapon.dat PISTOL: 25 damage, 17 round clip).
# The pistol's "dexterity needed" line shows the clip size instead: its condition becomes
# "max health >= 17", which every character meets.
PISTOL_NAME, AMMO_NAME = "Pistolet 9mm", "Naboje 9mm"
PISTOL_DAMAGE, CLIP = 25, 17
# the rocket launcher (weapon.dat RLAUNCHER: 75 damage, one rocket a load); its "needed" line too
# shows the load. A rocket is worth 50 ore.
LAUNCHER_NAME, ROCKETS_NAME = "Wyrzutnia rakiet", "Rakiety"
LAUNCHER_DAMAGE, LAUNCHER_CLIP, ROCKET_VALUE = 75, 1, 50
# the dildo (melee.dat DILDO: 18 a hit), blunt, needing no strength
DILDO_NAME, DILDO_DAMAGE = "Fioletowe dildo", 18

# GTA's weapon bank (SND_BANK_GENRL_WEAPONS) and how CAEWeaponAudioEntity plays the pistol:
# PlayGunSounds(EAGLE_DRYFIRE 52, EAGLE_LOWS 53, EAGLE_SHOT_L 6, EAGLE_SHOT_R 7, EAGLE_TAIL_L 8)
# at speed 1.41421, the tail 20 dB down at 1.0 and 1.189 of that; WeaponReload: 55 and 66 at the
# reload times of colt45 in weapon.dat (254 and 633 ms).
# The rocket: SND_GENRL_WEAPONS_ROCKET_LAUNCH (68). Its explosion: CAEExplosionAudioEntity plays
# sounds 4, 3 and 2 of SND_BANK_GENRL_EXPLOSIONS around the blast and sound 1 as the boom heard
# everywhere.
WEAPON_BANK, EXPLOSION_BANK = 143, 52
SOUNDS = {  # name -> (bank, [(sound ID, start s, speed, gain)])
    "CJ_PISTOL_SHOT": (WEAPON_BANK, [(52, 0.0, 1.41421, 1.0), (53, 0.0, 1.41421, 1.0), (6, 0.0, 1.41421, 1.0),
                                     (7, 0.0, 1.41421, 1.0), (8, 0.0, 1.0, 0.1), (8, 0.0, 1.18921, 0.1)]),
    "CJ_PISTOL_RELOAD": (WEAPON_BANK, [(55, 0.254, 1.0, 1.0), (66, 0.633, 1.0, 1.0)]),
    "CJ_ROCKET_LAUNCH": (WEAPON_BANK, [(68, 0.0, 1.0, 1.0)]),
    "CJ_ROCKET_EXPLOSION": (EXPLOSION_BANK, [(4, 0.0, 1.0, 1.0), (3, 0.0, 1.0, 1.0), (2, 0.0, 1.0, 1.0), (1, 0.0, 1.0, 0.7)]),
    "CJ_DILDO_HIT": (WEAPON_BANK, [(50, 0.0, 1.0, 1.0)]),  # SND_GENRL_WEAPONS_DILDO
}
RATE = 44100


# --- meshes ---------------------------------------------------------------------------------
def chunk(cid, body):
    return struct.pack("<HI", cid, 6 + len(body)) + body


def cstr(text):
    return text.encode("latin-1") + b"\0"


def write_3ds(path, name, verts, uvs, faces, texture):
    """A one-object 3D Studio mesh with one textured material. verts in centimetres, faces
    (a, b, c) counter-clockwise from outside, uvs with V up."""
    white = chunk(0x0011, bytes((255, 255, 255)))
    material = chunk(0xAFFF, chunk(0xA000, cstr(name)) + chunk(0xA010, white) + chunk(0xA020, white)
                     + chunk(0xA200, chunk(0x0030, struct.pack("<H", 100)) + chunk(0xA300, cstr(texture))))
    mesh = chunk(0x4110, struct.pack("<H", len(verts)) + b"".join(struct.pack("<3f", *v) for v in verts))
    mesh += chunk(0x4140, struct.pack("<H", len(uvs)) + b"".join(struct.pack("<2f", *uv) for uv in uvs))
    mesh += chunk(0x4160, struct.pack("<12f", 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0))
    groups = chunk(0x4130, cstr(name) + struct.pack("<H", len(faces)) + struct.pack(f"<{len(faces)}H", *range(len(faces))))
    mesh += chunk(0x4120, struct.pack("<H", len(faces)) + b"".join(struct.pack("<4H", a, b, c, 7) for a, b, c in faces) + groups)
    obj = chunk(0x4000, cstr(name) + chunk(0x4100, mesh))
    editor = chunk(0x3D3D, chunk(0x3D3E, struct.pack("<I", 3)) + material + chunk(0x0100, struct.pack("<f", 1.0)) + obj)
    with open(path, "wb") as f:
        f.write(chunk(0x4D4D, chunk(0x0002, struct.pack("<I", 3)) + editor))


def pistol_mesh(gta3, model="colt45"):
    """A weapon's mesh (the atomic on the root frame of colt45.dff, rocketla.dff, missile.dff) as
    GTA places it: the root frame is the hand's frame. RenderWare V runs down, 3DS V up."""
    clump = Clump(gta3.read(model + ".dff"))
    frame, index = next((f, g) for f, g in clump.atomics if clump.frames[f].parent < 0)
    g = clump.geometries[index]
    verts = [[c * SCALE for c in p] for p in g.positions]
    uvs = [(u, 1.0 - v) for u, v in g.uvs]
    faces = [(a, b, c) for a, b, c, _ in g.triangles]
    return verts, uvs, faces, read_txd(gta3.read(model + ".txd"))[g.materials[0]]


def flash_mesh(gta3):
    """colt45.dff's flash (the atomic on its "gunflash" frame) in the hand's frame: GTA draws it
    there for a moment on each shot (CPed::SetGunFlashAlpha)."""
    clump = Clump(gta3.read("colt45.dff"))
    frame, index = next((f, g) for f, g in clump.atomics if clump.frames[f].parent >= 0)
    m = clump.frames[frame].matrix  # local to the root frame (the hand), row vectors
    g = clump.geometries[index]
    verts = [[sum(p[k] * m[k][j] for k in range(3)) * SCALE + m[3][j] * SCALE for j in range(3)] for p in g.positions]
    uvs = [(u, 1.0 - v) for u, v in g.uvs]
    faces = [(a, b, c) for a, b, c, _ in g.triangles]
    return verts, uvs, faces, read_txd(gta3.read("colt45.txd"))[g.materials[0]]


def bullet_mesh():
    """A 9 mm round lying along X (cm): brass case, copper bullet. U picks the colour."""
    profile = [(0.0, 0.0, 0.25), (0.0, 0.5, 0.25), (1.9, 0.5, 0.25),  # case: base, side
               (1.9, 0.45, 0.75), (2.4, 0.42, 0.75), (2.8, 0.28, 0.75), (3.0, 0.0, 0.75)]  # bullet
    sides = 12
    verts, uvs, faces = [], [], []
    rings = []
    for x, r, u in profile:
        ring = []
        for i in range(sides if r > 0 else 1):
            a = 2 * math.pi * i / sides
            ring.append(len(verts))
            verts.append([x, r * math.cos(a), r * math.sin(a)])
            uvs.append((u, 0.5))
        rings.append(ring)
    for ring, nxt in zip(rings, rings[1:]):
        for i in range(sides):
            a, b = ring[i % len(ring)], ring[(i + 1) % len(ring)]
            c, d = nxt[i % len(nxt)], nxt[(i + 1) % len(nxt)]
            for tri in ((a, c, b), (b, c, d)):
                if len(set(tri)) == 3:
                    faces.append(tri)
    # outward = counter-clockwise from outside (as GTA's meshes): flip any that face inwards
    out = []
    for a, b, c in faces:
        pa, pb, pc = verts[a], verts[b], verts[c]
        u = [pb[k] - pa[k] for k in range(3)]
        v = [pc[k] - pa[k] for k in range(3)]
        n = [u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]]
        centre = [(pa[k] + pb[k] + pc[k]) / 3 for k in range(3)]
        radial = [0.0, centre[1], centre[2]] if abs(n[0]) < abs(n[1]) + abs(n[2]) else [centre[0] - 1.5, 0.0, 0.0]
        out.append((a, b, c) if sum(n[k] * radial[k] for k in range(3)) >= 0 else (a, c, b))
    texture = bytearray()
    for y in range(8):
        for x in range(8):
            texture += bytes((196, 156, 64, 255) if x < 4 else (184, 104, 58, 255))
    return verts, uvs, out, bytes(texture)


# --- sounds ---------------------------------------------------------------------------------
def mix(banks, bank, layers):
    """Mono 16-bit PCM at RATE: each (sound ID, start s, speed, gain) resampled and summed,
    peak-normalised."""
    out = []
    for sound, start, speed, gain in layers:
        rate, pcm = banks.sound(bank, sound)
        src = struct.unpack(f"<{len(pcm) // 2}h", pcm[:len(pcm) // 2 * 2])
        step = rate * speed / RATE
        offset = int(start * RATE)
        n = int((len(src) - 1) / step)
        if len(out) < offset + n:
            out += [0.0] * (offset + n - len(out))
        for k in range(n):
            p = k * step
            i = int(p)
            out[offset + k] += gain * (src[i] + (src[i + 1] - src[i]) * (p - i))
    peak = max(abs(s) for s in out) or 1.0
    return struct.pack(f"<{len(out)}h", *(int(s / peak * 31000) for s in out))


def wav(pcm):
    return (b"RIFF" + struct.pack("<I", 36 + len(pcm)) + b"WAVEfmt " + struct.pack("<IHHIIHH", 16, 1, 1, RATE, RATE * 2, 2, 16)
            + b"data" + struct.pack("<I", len(pcm)) + pcm)


# --- Gothic.dat -----------------------------------------------------------------------------
def _assignments(script, instance):
    """[(member symbol, array index, value pc, value opcode, value operand)] of an instance."""
    start = struct.unpack_from("<I", script.symbols[script.by_name[instance]].content)[0]
    out, previous = [], None
    for pc, op, arg in script.instructions(start):
        if op in (OP_PUSH_V, OP_PUSH_VV) and previous and (script.symbols[arg].name or b"").startswith(b"C_ITEM."):
            out.append((script.symbols[arg].name.decode(), script.code[pc + 5] if op == OP_PUSH_VV else 0) + previous)
            previous = None
        else:
            previous = (pc, op, arg)
    return out


def patch_items(script):
    """Light crossbow -> pistol, bolts -> 9 mm rounds. Sets absolute values, so it can run on
    an already patched script."""
    code = bytearray(script.code)
    sym = script.by_name

    def operand(pc, value):
        code[pc + 1:pc + 5] = struct.pack("<i", value)

    def literal(index, text, uses=1):
        assert script.references(index) <= uses, "string literal is shared with other code"
        script.symbols[index].content = [text.encode("cp1250")]

    ammo_name = None
    for member, index, pc, op, arg in _assignments(script, AMMO):
        if member == "C_ITEM.NAME" and op == OP_PUSH_V:
            ammo_name = arg
            literal(arg, AMMO_NAME, uses=2)  # also the label of the pistol's clip line
        elif member == "C_ITEM.VISUAL" and op == OP_PUSH_V:
            literal(arg, BULLET_MESH + ".3DS")
        elif member == "C_ITEM.MATERIAL" and op == OP_PUSH_V:
            operand(pc, sym["MAT_METAL"])
    for member, index, pc, op, arg in _assignments(script, PISTOL):
        if member == "C_ITEM.NAME" and op == OP_PUSH_V:
            literal(arg, PISTOL_NAME)
        elif member == "C_ITEM.VISUAL" and op == OP_PUSH_V:
            literal(arg, PISTOL_MESH + ".3DS")
        elif member == "C_ITEM.MATERIAL" and op == OP_PUSH_V:
            operand(pc, sym["MAT_METAL"])
        elif member == "C_ITEM.DAMAGETOTAL" and op == OP_PUSH_I:
            operand(pc, PISTOL_DAMAGE)
        elif member == "C_ITEM.COND_ATR" and index == 2 and op == OP_PUSH_V:
            operand(pc, sym["ATR_HITPOINTS_MAX"])
        elif member == "C_ITEM.COND_VALUE" and index == 2 and op == OP_PUSH_I:
            operand(pc, CLIP)
        elif member == "C_ITEM.TEXT" and index == 3 and op == OP_PUSH_V:
            operand(pc, ammo_name)

    # rockets: the alchemy ingredient made ammunition (munition category, stacking, crossbow
    # ammunition like the bolts), worth ROCKET_VALUE
    rockets = sym[ROCKETS]
    rockets_name = None
    for member, index, pc, op, arg in _assignments(script, ROCKETS):
        if member == "C_ITEM.NAME" and op == OP_PUSH_V:
            rockets_name = arg
            literal(arg, ROCKETS_NAME, uses=2)  # also the label of the launcher's load line
        elif member == "C_ITEM.MAINFLAG" and op == OP_PUSH_V:
            operand(pc, sym["ITEM_KAT_MUN"])
        elif member == "C_ITEM.FLAGS" and op in (OP_PUSH_V, OP_PUSH_I):
            code[pc] = OP_PUSH_I
            operand(pc, script.int_value("ITEM_MULTI") | script.int_value("ITEM_CROSSBOW"))
        elif member == "C_ITEM.VALUE" and op in (OP_PUSH_V, OP_PUSH_I):
            code[pc] = OP_PUSH_I
            operand(pc, ROCKET_VALUE)
        elif member == "C_ITEM.VISUAL" and op == OP_PUSH_V:
            literal(arg, MISSILE_MESH + ".3DS")
        elif member == "C_ITEM.MATERIAL" and op == OP_PUSH_V:
            operand(pc, sym["MAT_METAL"])
    # the launcher: GTA's numbers, its rockets as munition
    for member, index, pc, op, arg in _assignments(script, LAUNCHER):
        if member == "C_ITEM.NAME" and op == OP_PUSH_V:
            literal(arg, LAUNCHER_NAME)
        elif member == "C_ITEM.VISUAL" and op == OP_PUSH_V:
            literal(arg, LAUNCHER_MESH + ".3DS")
        elif member == "C_ITEM.MATERIAL" and op == OP_PUSH_V:
            operand(pc, sym["MAT_METAL"])
        elif member == "C_ITEM.DAMAGETOTAL" and op == OP_PUSH_I:
            operand(pc, LAUNCHER_DAMAGE)
        elif member == "C_ITEM.MUNITION" and op == OP_PUSH_I:
            operand(pc, rockets)
        elif member == "C_ITEM.COND_ATR" and index == 2 and op == OP_PUSH_V:
            operand(pc, sym["ATR_HITPOINTS_MAX"])
        elif member == "C_ITEM.COND_VALUE" and index == 2 and op == OP_PUSH_I:
            operand(pc, LAUNCHER_CLIP)
        elif member == "C_ITEM.TEXT" and index == 3 and op == OP_PUSH_V:
            operand(pc, rockets_name)
    # the dildo: a soft, blunt one-hander anyone can swing
    for member, index, pc, op, arg in _assignments(script, DILDO):
        if member == "C_ITEM.NAME" and op == OP_PUSH_V:
            literal(arg, DILDO_NAME)
        elif member == "C_ITEM.VISUAL" and op == OP_PUSH_V:
            literal(arg, DILDO_MESH + ".3DS")
        elif member == "C_ITEM.MATERIAL" and op == OP_PUSH_V:
            operand(pc, sym["MAT_LEATHER"])
        elif member == "C_ITEM.DAMAGETOTAL" and op == OP_PUSH_I:
            operand(pc, DILDO_DAMAGE)
        elif member == "C_ITEM.DAMAGETYPE" and op == OP_PUSH_V:
            operand(pc, sym["DAM_BLUNT"])
        elif member == "C_ITEM.COND_VALUE" and index == 2 and op == OP_PUSH_I:
            operand(pc, 0)
    script.code = bytes(code)


def patch_gothic_dat(gothic_dir):
    path = os.path.join(gothic_dir, "_work", "DATA", "scripts", "_compiled", "Gothic.dat")
    if not os.path.exists(path + ".orig"):
        shutil.copy2(path, path + ".orig")
    script = Script(open(path, "rb").read())
    patch_items(script)
    with open(path, "wb") as f:
        f.write(script.to_bytes())
    print(f"Gothic.dat: {PISTOL} is now {PISTOL_NAME}, {AMMO} {AMMO_NAME}, {LAUNCHER} {LAUNCHER_NAME}, {ROCKETS} {ROCKETS_NAME}, "
          f"{DILDO} {DILDO_NAME}")


def build(gta_dir, gothic_dir):
    gta3 = ImgReader(os.path.join(gta_dir, "models", "gta3.img"))
    meshes = os.path.join(gothic_dir, "_work", "DATA", "MESHES", "CJ")
    textures = os.path.join(gothic_dir, "_work", "DATA", "TEXTURES", "_COMPILED")
    os.makedirs(meshes, exist_ok=True)
    os.makedirs(textures, exist_ok=True)
    verts, uvs, pistol_faces, texture = pistol_mesh(gta3)
    write_3ds(os.path.join(meshes, PISTOL_MESH + ".3DS"), PISTOL_MESH, verts, uvs, pistol_faces, PISTOL_MESH + ".TGA")
    write_tex(os.path.join(textures, PISTOL_MESH + "-C.TEX"), texture.width, texture.height, texture.rgba)
    verts, uvs, faces, flash = flash_mesh(gta3)
    write_3ds(os.path.join(meshes, FLASH_MESH + ".3DS"), FLASH_MESH, verts, uvs, faces, FLASH_MESH + ".TGA")
    write_tex(os.path.join(textures, FLASH_MESH + "-C.TEX"), flash.width, flash.height, flash.rgba, alpha=True)
    verts, uvs, faces, rgba = bullet_mesh()
    write_3ds(os.path.join(meshes, BULLET_MESH + ".3DS"), BULLET_MESH, verts, uvs, faces, BULLET_MESH + ".TGA")
    write_tex(os.path.join(textures, BULLET_MESH + "-C.TEX"), 8, 8, rgba)
    for mesh, model in ((LAUNCHER_MESH, "rocketla"), (MISSILE_MESH, "missile"), (DILDO_MESH, "gun_dildo1")):
        verts, uvs, faces, texture = pistol_mesh(gta3, model)
        write_3ds(os.path.join(meshes, mesh + ".3DS"), mesh, verts, uvs, faces, mesh + ".TGA")
        write_tex(os.path.join(textures, mesh + "-C.TEX"), texture.width, texture.height, texture.rgba)
    compiled = os.path.join(gothic_dir, "_work", "DATA", "MESHES", "_COMPILED")
    for name in (PISTOL_MESH, BULLET_MESH, FLASH_MESH, LAUNCHER_MESH, MISSILE_MESH, DILDO_MESH):  # Gothic recompiles from the new sources
        path = os.path.join(compiled, name + ".MRM")
        if os.path.exists(path):
            os.remove(path)
    banks = SfxBanks(gta_dir)
    files = {f"_WORK\\DATA\\SOUND\\SFX\\{name}.WAV": wav(mix(banks, bank, layers)) for name, (bank, layers) in SOUNDS.items()}
    write_vdf(os.path.join(gothic_dir, "Data", "CJ_Weapons.vdf"), files, comment="CJ's weapon sounds (GTA San Andreas weapon and explosion banks)")
    print(f"weapons: {PISTOL_MESH}.3DS ({len(pistol_faces)} faces), {FLASH_MESH}.3DS, {BULLET_MESH}.3DS, "
          f"{LAUNCHER_MESH}.3DS, {MISSILE_MESH}.3DS, {DILDO_MESH}.3DS, {len(files)} sounds")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--gta", required=True, help="GTA San Andreas (classic PC) install folder")
    parser.add_argument("--gothic", default=DEFAULT_GOTHIC, help="Gothic 1 install folder")
    args = parser.parse_args()
    build(args.gta, args.gothic)
    patch_gothic_dat(args.gothic)


if __name__ == "__main__":
    main()
