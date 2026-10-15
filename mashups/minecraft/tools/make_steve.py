"""Steve from Minecraft as Gothic's hero: his blocky model from the player's own skin, playing every
animation of Gothic's hero.

Steve is built as Minecraft's player model builds him (net.minecraft.client.model.PlayerModel,
the wide arms): six boxes from the 64x64 skin, each on one bone, scaled by Minecraft's 0.9375 to
5.86 cm a pixel (1.875 m tall). The second layer (hat, jacket, sleeves, trousers) is added where a
face of it is fully painted. The skin is scaled up 8 times without smoothing, so the pixels stay
sharp in Gothic.

His skeleton has every node of Gothic's human (HUMANS.MDH), so whatever Gothic attaches to a node
(weapons in the hands and on the back, spell effects, the hit limb of the fists) finds it, but
on his blocks: the hands and feet at the ends of his arms and legs, the head on top of his body.
Each of Gothic's animations is played by these blocks: a block turns as Gothic's bone it hangs on
(upper arm, thigh, head, the spine), so Steve walks, fights, climbs ladders, sits and swims with
Gothic's timing, events and root motion, with stiff arms and legs as in Minecraft. The walk and
run cycles swing his arms and legs as Minecraft does (HumanoidModel.setupAnim: cos(p * 0.6662)
* 1.4 * amount for the legs, the arms opposite), at the amount Minecraft gives that speed. The
root's height follows Gothic's in his shorter legs' proportion, and in full in Gothic's fly
animations (ladders, jumps, climbing up ledges, diving).

The hero's overlays (weapon skills, the sprint potion, acrobatics, the torch) get Steve versions
too: STEVE_<overlay>.MDS for HUMANS_<overlay>.MDS; the Minecraft plugin applies them in their place.

  _work\\DATA\\ANIMS\\STEVE.MDS, STEVE_<overlay>.MDS   model scripts
  _work\\DATA\\ANIMS\\STEVE_BODY.ASC                  his skeleton and mesh
  _work\\DATA\\ANIMS\\STEVE-<ANI>.ASC                 one source per animation (STEVE_<overlay>-<ANI>.ASC)
  _work\\DATA\\TEXTURES\\_COMPILED\\STEVE-C.TEX         his skin
The hero's Mdl_SetVisual / Mdl_SetVisualBody in Gothic.dat are pointed at him (Gothic.dat.orig kept).

usage: python make_steve.py [--minecraft DIR] --gothic DIR [--preview DIR]
"""
import argparse
import glob
import io
import math
import os
import re
import sys

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
from asc import helper_block, write_animation, write_softskin
from datpatch import load as load_dat, save as save_dat, set_hero_visual
from gothic import compiled_animation, hierarchy, mds_entries, model_script, model_scripts
from man import quaternion_matrix, read_man
from mcfiles import DEFAULT_MINECRAFT, MinecraftFiles
from tex import write_tex

MODEL = "STEVE"
BODY = "STEVE_BODY"
TEXTURE = "STEVE"
SKIN = "assets/minecraft/textures/entity/player/wide/steve.png"
PX = 0.9375 * 100 / 16  # cm a skin pixel: a block is 16 pixels and 1 m, the player drawn at 0.9375
UPSCALE = 8

# PlayerModel's boxes (Minecraft model space: pixels, y down, the front at -z): part, the node
# it hangs on, pivot, box corner, size, texture offset, the second layer's offset and growth
PARTS = [
    ("head", "BIP01 HEAD", (0, 0, 0), (-4, -8, -4), (8, 8, 8), (0, 0), (32, 0), 0.5),
    ("body", "BIP01 SPINE1", (0, 0, 0), (-4, 0, -2), (8, 12, 4), (16, 16), (16, 32), 0.25),
    ("right arm", "BIP01 R UPPERARM", (-5, 2, 0), (-3, -2, -2), (4, 12, 4), (40, 16), (40, 32), 0.25),
    ("left arm", "BIP01 L UPPERARM", (5, 2, 0), (-1, -2, -2), (4, 12, 4), (32, 48), (48, 48), 0.25),
    ("right leg", "BIP01 R THIGH", (-1.9, 12, 0), (-2, 0, -2), (4, 12, 4), (0, 16), (0, 32), 0.25),
    ("left leg", "BIP01 L THIGH", (1.9, 12, 0), (-2, 0, -2), (4, 12, 4), (16, 48), (0, 48), 0.25),
]

# Steve's skeleton: Gothic's node names, each hung where it belongs on his blocks (parent, place in
# Minecraft model pixels). The blocks' nodes hang straight on the body, so nothing comes apart;
# the slots on his back and belt are placed from Gothic's (slot_places).
NODES = [
    ("BIP01", None, (0, 12, 0)),
    ("BIP01 PELVIS", "BIP01", (0, 12, 0)),
    ("BIP01 SPINE", "BIP01 PELVIS", (0, 12, 0)),
    ("BIP01 SPINE1", "BIP01 SPINE", (0, 12, 0)),
    ("BIP01 SPINE2", "BIP01 SPINE1", (0, 6, 0)),
    ("BIP01 NECK", "BIP01 SPINE1", (0, 0, 0)),
    ("BIP01 HEAD", "BIP01 SPINE1", (0, 0, 0)),
    ("ZS_HELMET", "BIP01 HEAD", (0, -8, 0)),
    ("BIP01 R CLAVICLE", "BIP01 SPINE1", (-2, 1, 0)),
    ("BIP01 R UPPERARM", "BIP01 SPINE1", (-5, 2, 0)),
    ("BIP01 R FOREARM", "BIP01 R UPPERARM", (-6, 7, 0)),
    ("BIP01 R HAND", "BIP01 R UPPERARM", (-6, 11, 0)),
    ("BIP01 R FINGER0", "BIP01 R HAND", (-6, 12, 0)),
    ("ZS_RIGHTHAND", "BIP01 R HAND", (-6, 11.5, 0)),
    ("BIP01 L CLAVICLE", "BIP01 SPINE1", (2, 1, 0)),
    ("BIP01 L UPPERARM", "BIP01 SPINE1", (5, 2, 0)),
    ("BIP01 L FOREARM", "BIP01 L UPPERARM", (6, 7, 0)),
    ("ZS_LEFTARM", "BIP01 L FOREARM", (8, 8, 0)),
    ("BIP01 L HAND", "BIP01 L UPPERARM", (6, 11, 0)),
    ("BIP01 L FINGER0", "BIP01 L HAND", (6, 12, 0)),
    ("ZS_LEFTHAND", "BIP01 L HAND", (6, 11.5, 0)),
    ("BIP01 R THIGH", "BIP01 PELVIS", (-1.9, 12, 0)),
    ("BIP01 R CALF", "BIP01 R THIGH", (-1.9, 18, 0)),
    ("BIP01 R FOOT", "BIP01 R THIGH", (-1.9, 23, 0)),
    ("BIP01 R TOE0", "BIP01 R FOOT", (-1.9, 24, -2)),
    ("BIP01 L THIGH", "BIP01 PELVIS", (1.9, 12, 0)),
    ("BIP01 L CALF", "BIP01 L THIGH", (1.9, 18, 0)),
    ("BIP01 L FOOT", "BIP01 L THIGH", (1.9, 23, 0)),
    ("BIP01 L TOE0", "BIP01 L FOOT", (1.9, 24, -2)),
    ("ZS_SWORD", "BIP01 SPINE", None),
    ("ZS_LONGSWORD", "BIP01 SPINE1", None),
    ("ZS_BOW", "BIP01 SPINE1", None),
    ("ZS_CROSSBOW", "BIP01 SPINE1", None),
    ("ZS_SHIELD", "BIP01 SPINE1", None),
]
# The nodes an animation may move: the blocks', the hands' (what they hold) and those above them.
# The rest keep their place on their block.
MOVING = {"BIP01", "BIP01 PELVIS", "BIP01 SPINE", "BIP01 SPINE1", "BIP01 HEAD", "BIP01 R UPPERARM",
          "BIP01 L UPPERARM", "BIP01 R HAND", "BIP01 L HAND", "BIP01 R THIGH", "BIP01 L THIGH"}

# Minecraft's mobs. The zombie and the skeleton are Gothic's own (their scripts' bodies ZOM_BODY and
# SKE_BODY get these in the plugin) on Steve's skeleton and animations: HumanoidModel's boxes
# (zombie.png) and SkeletonModel's thin limbs (skeleton.png); a left limb is the right one
# mirrored, as Minecraft draws them.
ENTITY = "assets/minecraft/textures/entity/"
MOBS = [
    ("MC_ZOMBIE", ENTITY + "zombie/zombie.png", [
        ("head", "BIP01 HEAD", (0, 0, 0), (-4, -8, -4), (8, 8, 8), (0, 0), (32, 0), 0.5),
        ("body", "BIP01 SPINE1", (0, 0, 0), (-4, 0, -2), (8, 12, 4), (16, 16), None, 0),
        ("right arm", "BIP01 R UPPERARM", (-5, 2, 0), (-3, -2, -2), (4, 12, 4), (40, 16), None, 0),
        ("left arm", "BIP01 L UPPERARM", (5, 2, 0), (-1, -2, -2), (4, 12, 4), (40, 16), None, 0, True),
        ("right leg", "BIP01 R THIGH", (-1.9, 12, 0), (-2, 0, -2), (4, 12, 4), (0, 16), None, 0),
        ("left leg", "BIP01 L THIGH", (1.9, 12, 0), (-2, 0, -2), (4, 12, 4), (0, 16), None, 0, True)]),
    ("MC_SKELETON", ENTITY + "skeleton/skeleton.png", [
        ("head", "BIP01 HEAD", (0, 0, 0), (-4, -8, -4), (8, 8, 8), (0, 0), (32, 0), 0.5),
        ("body", "BIP01 SPINE1", (0, 0, 0), (-4, 0, -2), (8, 12, 4), (16, 16), None, 0),
        ("right arm", "BIP01 R UPPERARM", (-5, 2, 0), (-1, -2, -1), (2, 12, 2), (40, 16), None, 0),
        ("left arm", "BIP01 L UPPERARM", (5, 2, 0), (-1, -2, -1), (2, 12, 2), (40, 16), None, 0, True),
        ("right leg", "BIP01 R THIGH", (-2, 12, 0), (-1, 0, -1), (2, 12, 2), (0, 16), None, 0),
        ("left leg", "BIP01 L THIGH", (2, 12, 0), (-1, 0, -1), (2, 12, 2), (0, 16), None, 0, True)]),
]
# the zombie's raised arms (AnimationUtils.animateZombieArms, then bobArms) in an overlay the plugin
# puts on Gothic's zombies: forward 80 degrees, 120 with its fists up (fighting)
ZOMBIE_OVERLAY = "STEVE_ZOMBIE"
ZOMBIE_MOVES = {"s_run", "s_runl", "s_walk", "s_walkl", "t_run_2_runl", "t_runl_2_run", "t_walk_2_walkl", "t_walkl_2_walk",
                "s_fistrun", "s_fistrunl", "s_fistwalk", "s_fistwalkl", "t_fistrun_2_fistrunl", "t_fistrunl_2_fistrun",
                "t_fistwalk_2_fistwalkl", "t_fistwalkl_2_fistwalk"}
# The creeper (CreeperModel: head and body on top of four short legs) has a skeleton of its own,
# Gothic's node names on its boxes: the front legs as the arms (they swing opposite to the hind legs
# on the same side, as a creeper walks), the hind legs as the legs. The plugin spawns it.
CREEPER = "MCCREEPER"
CREEPER_SKIN = ENTITY + "creeper/creeper.png"
CREEPER_PARTS = [
    ("head", "BIP01 HEAD", (0, 6, 0), (-4, -8, -4), (8, 8, 8), (0, 0), None, 0),
    ("body", "BIP01 SPINE1", (0, 6, 0), (-4, 0, -2), (8, 12, 4), (16, 16), None, 0),
    ("right hind leg", "BIP01 R THIGH", (-2, 18, 4), (-2, 0, -2), (4, 6, 4), (0, 16), None, 0),
    ("left hind leg", "BIP01 L THIGH", (2, 18, 4), (-2, 0, -2), (4, 6, 4), (0, 16), None, 0),
    ("right front leg", "BIP01 R UPPERARM", (-2, 18, -4), (-2, 0, -2), (4, 6, 4), (0, 16), None, 0),
    ("left front leg", "BIP01 L UPPERARM", (2, 18, -4), (-2, 0, -2), (4, 6, 4), (0, 16), None, 0),
]
CREEPER_NODES = [
    ("BIP01", None, (0, 18, 0)),
    ("BIP01 PELVIS", "BIP01", (0, 18, 0)),
    ("BIP01 SPINE", "BIP01 PELVIS", (0, 18, 0)),
    ("BIP01 SPINE1", "BIP01 SPINE", (0, 18, 0)),
    ("BIP01 SPINE2", "BIP01 SPINE1", (0, 12, 0)),
    ("BIP01 NECK", "BIP01 SPINE1", (0, 6, 0)),
    ("BIP01 HEAD", "BIP01 SPINE1", (0, 6, 0)),
    ("ZS_HELMET", "BIP01 HEAD", (0, -2, 0)),
    ("BIP01 R CLAVICLE", "BIP01 SPINE1", (-2, 17, -3)),
    ("BIP01 R UPPERARM", "BIP01 SPINE1", (-2, 18, -4)),
    ("BIP01 R FOREARM", "BIP01 R UPPERARM", (-2, 21, -4)),
    ("BIP01 R HAND", "BIP01 R UPPERARM", (-2, 23, -4)),
    ("BIP01 R FINGER0", "BIP01 R HAND", (-2, 24, -4)),
    ("ZS_RIGHTHAND", "BIP01 R HAND", (-2, 23.5, -4)),
    ("BIP01 L CLAVICLE", "BIP01 SPINE1", (2, 17, -3)),
    ("BIP01 L UPPERARM", "BIP01 SPINE1", (2, 18, -4)),
    ("BIP01 L FOREARM", "BIP01 L UPPERARM", (2, 21, -4)),
    ("ZS_LEFTARM", "BIP01 L FOREARM", (4, 21, -4)),
    ("BIP01 L HAND", "BIP01 L UPPERARM", (2, 23, -4)),
    ("BIP01 L FINGER0", "BIP01 L HAND", (2, 24, -4)),
    ("ZS_LEFTHAND", "BIP01 L HAND", (2, 23.5, -4)),
    ("BIP01 R THIGH", "BIP01 PELVIS", (-2, 18, 4)),
    ("BIP01 R CALF", "BIP01 R THIGH", (-2, 21, 4)),
    ("BIP01 R FOOT", "BIP01 R THIGH", (-2, 23, 4)),
    ("BIP01 R TOE0", "BIP01 R FOOT", (-2, 24, 2)),
    ("BIP01 L THIGH", "BIP01 PELVIS", (2, 18, 4)),
    ("BIP01 L CALF", "BIP01 L THIGH", (2, 21, 4)),
    ("BIP01 L FOOT", "BIP01 L THIGH", (2, 23, 4)),
    ("BIP01 L TOE0", "BIP01 L FOOT", (2, 24, 2)),
    ("ZS_SWORD", "BIP01 SPINE", None),
    ("ZS_LONGSWORD", "BIP01 SPINE1", None),
    ("ZS_BOW", "BIP01 SPINE1", None),
    ("ZS_CROSSBOW", "BIP01 SPINE1", None),
    ("ZS_SHIELD", "BIP01 SPINE1", None),
]

# Minecraft's walk: the loops of walking and running, with a weapon too (the arms then hold it)
SWING = re.compile(r"^s_(Fist|Mag|1h|2h|Bow|CBow)?(Run|Walk)L$", re.I)
# the overlays of other characters, not the hero's
OTHER_OVERLAYS = re.compile(r"^HUMANS_SKELETON", re.I)
EVENT = re.compile(r"^\s*(\*\w+)\s*\(\s*(-?\d+)(.*)\)\s*$")
FRAME_TAGS = re.compile(r'"(DEF_OPT_FRAME|DEF_HIT_END|DEF_WINDOW)"\s*"([^"]*)"')


# --- vectors and rotations (engine axes: x right, y up, z forward; column vectors) ---

def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def mul(a, b):
    return [[a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j] for j in range(3)] for i in range(3)]


def apply(m, v):
    return (m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2],
            m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2],
            m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2])


def transpose(m):
    return [[m[j][i] for j in range(3)] for i in range(3)]


def rot_x(angle):
    c, s = math.cos(angle), math.sin(angle)
    return [[1.0, 0.0, 0.0], [0.0, c, -s], [0.0, s, c]]


def rot_z(angle):
    c, s = math.cos(angle), math.sin(angle)
    return [[c, -s, 0.0], [s, c, 0.0], [0.0, 0.0, 1.0]]


def from_minecraft(p):
    """Minecraft model pixels (y down, the front at -z, the right at -x) -> engine cm, feet at y 0."""
    return (-p[0] * PX, (24 - p[1]) * PX, -p[2] * PX)


# engine (x, y, z) -> 3ds Max (-x, -z, y), as the ASC sources have it (gothic.root_motion)
TO_MAX = [[-1.0, 0.0, 0.0], [0.0, 0.0, -1.0], [0.0, 1.0, 0.0]]


def max_rows(m):
    """An engine rotation as the row-vector rows of an ASC node in 3ds Max axes."""
    return transpose(mul(mul(TO_MAX, m), transpose(TO_MAX)))


def max_pos(p):
    return apply(TO_MAX, p)


class Gothic:
    """Gothic's human skeleton and its animations, as world rotations of its nodes."""

    def __init__(self, gothic_dir):
        self.dir = gothic_dir
        nodes, self.root_bind = hierarchy(gothic_dir)
        self.names = [n for n, _, _ in nodes]
        self.parent = [p for _, p, _ in nodes]
        self.bind_local = [([r[:3] for r in m], (m[0][3], m[1][3], m[2][3])) for _, _, m in nodes]
        self.index = {n: i for i, n in enumerate(self.names)}
        self.bind_rot, self.bind_pos = self.world({}, self.root_bind)
        stand = read_man(compiled_animation(gothic_dir, "S_RUN"))
        self.stand = stand["positions"][0][stand["indices"].index(0)]

    def world(self, local, root=None):
        """World rotations and positions of every node; `local` {node index: (rotation, position)}
        of the animated ones, the bind pose's for the rest."""
        rots, poss = [], []
        for i, p in enumerate(self.parent):
            r, t = local.get(i) or self.bind_local[i]
            if p < 0:
                rots.append(r)
                poss.append(tuple(root) if root is not None and i not in local else t)
            else:
                rots.append(mul(rots[p], r))
                poss.append(add(poss[p], apply(rots[p], t)))
        return rots, poss

    def frames(self, data):
        """[(world rotations by name, root position)] per frame of a compiled animation, and the set
        of node names it animates."""
        man = read_man(data, rotations=True)
        out = []
        for f in range(man["frames"]):
            local = {i: (quaternion_matrix(q), t) for i, q, t in zip(man["indices"], man["rotations"][f], man["positions"][f])}
            rots, poss = self.world(local)
            out.append(({n: rots[i] for i, n in enumerate(self.names)}, poss[0]))
        return out, {self.names[i] for i in man["indices"]}

    def ancestors(self, name, stop):
        """Gothic's nodes from `name` up to `stop` (not included): whose motion turns `name`
        against `stop`."""
        out, i = [], self.index[name]
        while i >= 0 and self.names[i] != stop:
            out.append(self.names[i])
            i = self.parent[i]
        return out


class Steve:
    """Steve's skeleton on Gothic's: bind pose, and the ASC samples of an animation."""

    def __init__(self, gothic, nodes=None, arm_swing=1.0):
        self.g = gothic
        self.nodes = nodes or NODES
        self.arm_swing = arm_swing  # the arms' swing against Minecraft's (a creeper's front legs: 1.4)
        self.order = [n for n, _, _ in self.nodes]
        self.parent = {n: p for n, p, _ in self.nodes}
        places = {n: from_minecraft(p) for n, _, p in self.nodes if p is not None}
        places.update(self.slot_places(places))
        self.bind_pos = places
        self.bind_rot = {n: gothic.bind_rot[gothic.index[n]] for n in self.order}
        self.offset = {}  # each node's place in its parent's frame
        for n in self.order:
            p = self.parent[n]
            if p:
                self.offset[n] = apply(transpose(self.bind_rot[p]), sub(self.bind_pos[n], self.bind_pos[p]))
        # what turns a node against its parent in Gothic
        self.influence = {n: set(gothic.ancestors(n, self.parent[n])) if self.parent[n] else {n} for n in self.order}
        self.pelvis = self.bind_pos["BIP01"][1]

    def slot_places(self, places):
        """The weapon slots on the back and the belt where Gothic has them, stretched from Gothic's
        body (pelvis to neck, shoulder to shoulder) to Steve's."""
        g = self.g
        gp, gn = g.bind_pos[g.index["BIP01"]], g.bind_pos[g.index["BIP01 NECK"]]
        gs = g.bind_pos[g.index["BIP01 L UPPERARM"]]
        sp, sn, ss = places["BIP01"], places["BIP01 NECK"], places["BIP01 L UPPERARM"]
        kx = ss[0] / gs[0]
        ky = (sn[1] - sp[1]) / (gn[1] - gp[1])
        out = {}
        for n, _, p in self.nodes:
            if p is None:
                q = g.bind_pos[g.index[n]]
                out[n] = (q[0] * kx, sp[1] + (q[1] - gp[1]) * ky, (q[2] - gp[2]) * kx)
        return out

    def helpers(self, samples=None, keyed=()):
        return [helper_block(n, self.parent[n], max_rows(self.bind_rot[n]), max_pos(self.bind_pos[n]),
                             [(max_rows(r), max_pos(p)) for r, p in samples[n]] if samples and n in keyed else None)
                for n in self.order]

    def pose(self, rots, root, full_height):
        """Steve's local rotations for one frame of Gothic's (world rotations by name, root place)."""
        local = {}
        for n in self.order:
            p = self.parent[n]
            local[n] = mul(transpose(rots[p]), rots[n]) if p else rots[n]
        sx, sy, sz = self.g.stand
        k = 1.0 if full_height else self.pelvis / sy
        return local, (root[0] - sx, self.pelvis + (root[1] - sy) * k, root[2] - sz)

    def samples(self, poses, keyed):
        """The ASC's world samples: a node's sample is its parent's (or the parent's bind pose
        where the animation leaves the parent alone, as Gothic reads it back) turned by its local
        rotation, at its fixed place on the parent."""
        out = {n: [] for n in self.order}
        for local, root in poses:
            world = {}
            for n in self.order:
                p = self.parent[n]
                if not p:
                    world[n] = (local[n], root)
                    continue
                pr, pp = world[p] if p in keyed else (self.bind_rot[p], self.bind_pos[p])
                world[n] = (mul(pr, local[n]), add(pp, apply(pr, self.offset[n])))
            for n in keyed:
                out[n].append(world[n])
        return out

    def world(self, local, root):
        """Steve's world transforms for a preview: everything follows its parent's motion."""
        world = {}
        for n in self.order:
            p = self.parent[n]
            if not p:
                world[n] = (local[n], root)
            else:
                pr, pp = world[p]
                world[n] = (mul(pr, local[n]), add(pp, apply(pr, self.offset[n])))
        return world


def swing(steve, frames, name):
    """Minecraft's walk for a walk or run loop: Gothic's loop with Steve's legs (and, without a
    weapon, arms) swung as HumanoidModel.setupAnim does. Minecraft's swing amount is the distance
    moved a tick times 4, at most 1, and its phase advances by the amount every tick (20 a second);
    the loop gets a whole number of swings."""
    n = len(frames)
    (_, first), (_, last) = frames[0], frames[-1]
    speed = math.hypot(last[0] - first[0], last[2] - first[2]) * 25 / max(n - 1, 1)  # cm/s
    amount = min(1.0, speed / 500)  # (speed / 100 / 20 blocks a tick) * 4
    swings = max(1, round(0.6662 * amount * 20 / 25 * n / (2 * math.pi)))
    g = steve.g
    bind = lambda node: g.bind_rot[g.index[node]]  # noqa: E731
    arms = not re.match(r"^s_(Fist|Mag|1h|2h|Bow|CBow)", name, re.I)
    for f, (local, _) in enumerate(frames):
        phase = 2 * math.pi * swings * f / n
        leg = math.cos(phase) * 1.4 * amount
        for node, angle in (("BIP01 R THIGH", leg), ("BIP01 L THIGH", -leg)):
            local[node] = mul(mul(transpose(bind("BIP01 PELVIS")), rot_x(angle)), bind(node))
        if arms:
            arm = math.cos(phase + math.pi) * amount * steve.arm_swing
            for node, angle in (("BIP01 R UPPERARM", arm), ("BIP01 L UPPERARM", -arm)):
                local[node] = mul(mul(transpose(bind("BIP01 SPINE1")), rot_x(angle)), bind(node))
    return frames


def zombie_arms(steve):
    """A pose hook: the zombie's arms raised (animateZombieArms: -pi / 2.25, fighting -pi / 1.5) with
    bobArms' sway (z: cos(age * 0.09) * 0.05 + 0.05 outwards, x: sin(age * 0.067) * 0.05)."""
    g = steve.g
    bind = lambda node: g.bind_rot[g.index[node]]  # noqa: E731

    def hook(name, poses):
        drop = -math.pi / (1.5 if name.lower().startswith(("s_fist", "t_fist")) else 2.25)
        for f, (local, _) in enumerate(poses):
            age = f * 20 / 25
            bob_x, bob_z = math.sin(age * 0.067) * 0.05, math.cos(age * 0.09) * 0.05 + 0.05
            for node, x, z in (("BIP01 R UPPERARM", drop + bob_x, bob_z), ("BIP01 L UPPERARM", drop - bob_x, -bob_z)):
                local[node] = mul(mul(mul(transpose(bind("BIP01 SPINE1")), rot_z(z)), rot_x(x)), bind(node))
        return poses
    return hook


def tokens(args):
    return re.findall(r'"[^"]*"|\S+', args)


def shift_events(events, first, frames):
    """The event block of an animation moved to its own source's frames (first -> 0)."""
    out = []
    for line in events.strip("{} \t\n").splitlines():
        m = EVENT.match(line)
        if not m:
            continue
        frame = int(m.group(2))
        if frame >= 0:
            frame = min(max(frame - first, 0), frames - 1)
        rest = FRAME_TAGS.sub(lambda t: f'"{t.group(1)}" "' + " ".join(
            str(min(max(int(v) - first, 0), frames - 1)) for v in t.group(2).split()) + '"', m.group(3))
        out.append(f"\t\t\t{m.group(1)}\t({frame}{rest})\n")
    return out


def convert_script(gothic, steve, text, model, steve_model, anims, written, only=None, pose_hook=None):
    """Steve's model script entries for one of Gothic's (HUMANS or an overlay): every ani gets a
    source of its own (STEVE-<ANI>.ASC, STEVE_<OVERLAY>-<ANI>.ASC). Returns the entry lines.
    `only`: the names of the animations to take (lower case; the rest left out); `pose_hook(name,
    poses)`: changes an animation's poses before they are written."""
    lines = []
    for kind, args, events in mds_entries(text):
        a = tokens(args)
        if only is not None and a[0].strip('"').lower() not in only:
            continue
        if kind != "ani":
            lines.append(f"\t\t{kind}\t(" + "\t".join(a) + ")\n")
            continue
        name = a[0].strip('"')
        data = compiled_animation(gothic.dir, name, model)
        if data is None:
            print(f"  {model}: no compiled {name}, left out")
            continue
        frames, moved = gothic.frames(data)
        flags = a[5].upper()
        poses = [steve.pose(rots, root, "F" in flags) for rots, root in frames]
        if SWING.match(name):
            poses = swing(steve, poses, name)
            keyed = set(MOVING)
        else:
            keyed = {n for n in MOVING if steve.influence[n] & moved}
        if pose_hook:
            poses = pose_hook(name, poses)
            keyed = set(MOVING)
        source = f"{steve_model}-{name.upper()}.ASC"
        if not keyed:  # moves nothing of Steve's (the fingers): key a node of no consequence
            keyed = {"BIP01 R FINGER0"}
        write_animation(os.path.join(anims, source), steve.helpers(steve.samples(poses, keyed), keyed), len(poses))
        written.add(source.upper())
        first = min(int(a[8]), int(a[9]))
        a[6], a[8], a[9] = f'"{source}"', "0", str(len(poses) - 1)
        line = f"\t\tani\t(" + "\t".join(a) + ")\n"
        tags = shift_events(events, first, len(poses))
        lines.append(line + ("\t\t{\n" + "".join(tags) + "\t\t}\n" if tags else ""))
    return lines


def load_skin(minecraft_dir):
    from PIL import Image
    files = MinecraftFiles(minecraft_dir)
    return Image.open(io.BytesIO(files.read(SKIN))).convert("RGBA"), files.version


def box_faces(pivot, corner, size, uv, grow, skin, layer, mirror=False):
    """The faces of one of Minecraft's boxes as ModelPart.Cube makes them: [(4 corners in model
    pixels, 4 UVs in skin pixels, outward direction)]; on the second layer only the fully
    painted ones. mirror: Minecraft's mirror() (a left limb drawn with the right one's pixels):
    the box's x ends change places, each corner keeping its pixels."""
    x0, y0, z0 = (pivot[i] + corner[i] - grow for i in range(3))
    x1, y1, z1 = (pivot[i] + corner[i] + size[i] + grow for i in range(3))
    centre = ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2)
    if mirror:
        x0, x1 = x1, x0
    w, h, d = size
    u, v = uv
    t0, t1, t2, t3 = (x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0)
    l0, l1, l2, l3 = (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)
    u0, u1, u2, u22, u3, u4 = u, u + d, u + d + w, u + d + w + w, u + d + w + d, u + d + w + d + w
    v0, v1, v2 = v, v + d, v + d + h
    faces = [((l1, l0, t0, t1), (u1, v0, u2, v1), (0, -1, 0)),   # DOWN: Minecraft's -y, the top
             ((t2, t3, l3, l2), (u2, v1, u22, v0), (0, 1, 0)),   # UP: the bottom
             ((t0, l0, l3, t3), (u0, v1, u1, v2), (-1, 0, 0)),   # WEST: his right side
             ((t1, t0, t3, t2), (u1, v1, u2, v2), (0, 0, -1)),   # NORTH: the front
             ((l1, t1, t2, l2), (u2, v1, u3, v2), (1, 0, 0)),    # EAST: his left side
             ((l0, l1, l2, l3), (u3, v1, u4, v2), (0, 0, 1))]    # SOUTH: the back
    out = []
    for corners, (fu0, fv0, fu1, fv1), normal in faces:
        if layer:
            area = skin.crop((min(fu0, fu1), min(fv0, fv1), max(fu0, fu1), max(fv0, fv1)))
            if area.width == 0 or area.height == 0 or area.getchannel("A").getextrema()[0] < 255:
                continue
        # Polygon: vertex 0 gets (u1, v0), 1 (u0, v0), 2 (u0, v1), 3 (u1, v1)
        uvs = [(fu1, fv0), (fu0, fv0), (fu0, fv1), (fu1, fv1)]
        middle = [sum(c[i] for c in corners) / 4 for i in range(3)]
        out.append((corners, uvs, tuple(middle[i] - centre[i] for i in range(3))))
    return out


def build_mesh(skin, parts=None):
    """A model's mesh in Gothic's bind pose (engine axes): vertices, faces, UVs, UV faces, weights.
    Every face has its own corners, so the boxes stay flat-shaded. parts: [(part, node, pivot,
    corner, size, texture offset, the second layer's offset or None, its growth[, mirror])]."""
    verts, faces, uvs, uv_faces, weights = [], [], [], [], []
    for part in parts or PARTS:
        _, node, pivot, corner, size, uv, uv2, grow = part[:8]
        mirror = len(part) > 8 and part[8]
        for layer, (offset, g) in enumerate(((uv, 0.0), (uv2, grow))):
            if offset is None:
                continue
            for corners, face_uvs, normal in box_faces(pivot, corner, size, offset, g, skin, layer, mirror):
                base = len(verts)
                points = [from_minecraft(c) for c in corners]
                verts += points
                uvs += [(fu / skin.width, 1 - fv / skin.height) for fu, fv in face_uvs]
                weights += [[(node, 1.0)]] * 4
                out = (-normal[0], -normal[1], -normal[2])  # outwards, in engine axes
                for tri in ((0, 1, 2), (0, 2, 3)):
                    a, b, c = (base + i for i in tri)
                    # counter-clockwise from outside in 3ds Max's axes (an ASC's front)
                    pa, pb, pc = (max_pos(verts[i]) for i in (a, b, c))
                    e1, e2 = sub(pb, pa), sub(pc, pa)
                    n = (e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0])
                    if sum(x * y for x, y in zip(n, max_pos(out))) < 0:
                        b, c = c, b
                    faces.append((a, b, c, 0))
                    uv_faces.append((a, b, c))
    return verts, faces, uvs, uv_faces, weights


def write_skin_texture(skin, gothic_dir, texture=None):
    """A skin scaled up without smoothing, as Gothic's compiled texture (STEVE-C.TEX)."""
    texture = texture or TEXTURE
    from PIL import Image
    big = skin.resize((skin.width * UPSCALE, skin.height * UPSCALE), Image.NEAREST)
    path = os.path.join(gothic_dir, "_work", "DATA", "TEXTURES", "_COMPILED")
    os.makedirs(path, exist_ok=True)
    write_tex(os.path.join(path, texture + "-C.TEX"), big.width, big.height, big.tobytes())


def header_tags(text):
    """The modelTag lines of a model script's header (Gothic 2's HUMANS: the fists' hit limb)."""
    head = text.split("aniEnum", 1)[0]
    return "".join(f"\t{line.strip()}\n" for line in head.splitlines() if line.strip().lower().startswith("modeltag"))


def mob_skin(files, path):
    from PIL import Image
    return Image.open(io.BytesIO(files.read(path))).convert("RGBA")


def write_body(path, model, skin, parts, texture, mesh_name):
    verts, faces, uvs, uv_faces, weights = build_mesh(skin, parts)
    write_softskin(path, model.helpers(), [texture + ".TGA"], [max_pos(v) for v in verts], faces, uvs, uv_faces, weights,
                   mesh_name=mesh_name)
    return verts, faces, uvs, uv_faces, weights


def build(minecraft_dir, gothic_dir, preview=None):
    skin, version = load_skin(minecraft_dir)
    files = MinecraftFiles(minecraft_dir)
    gothic = Gothic(gothic_dir)
    steve = Steve(gothic)
    anims = os.path.join(gothic_dir, "_work", "DATA", "ANIMS")
    os.makedirs(anims, exist_ok=True)

    verts, faces, uvs, uv_faces, weights = write_body(os.path.join(anims, BODY + ".ASC"), steve, skin, PARTS, TEXTURE, "zm_Steve")
    write_skin_texture(skin, gothic_dir)
    written = {BODY + ".ASC"}
    # the zombie's and the skeleton's bodies on Steve's skeleton
    bodies = [BODY]
    for name, path, parts in MOBS:
        mob = mob_skin(files, path)
        write_body(os.path.join(anims, name + "_BODY.ASC"), steve, mob, parts, name, "zm_" + name)
        write_skin_texture(mob, gothic_dir, name)
        written.add(name + "_BODY.ASC")
        bodies.append(name + "_BODY")
    meshes = "".join(f'\tregisterMesh ("{b}.ASC")\n' for b in bodies)

    text = re.sub(r"//[^\n]*", "", model_script(gothic_dir, "HUMANS"))
    entries = convert_script(gothic, steve, text, "HUMANS", MODEL, anims, written)
    with open(os.path.join(anims, MODEL + ".MDS"), "w", newline="\r\n") as f:
        f.write(f"// Steve from Minecraft {version}, playing Gothic's human animations, and the zombie's and the skeleton's "
                f"bodies (generated by make_steve.py)\n"
                f'Model ("{MODEL}")\n{{\n\tmeshAndTree ("{BODY}.ASC" DONT_USE_MESH)\n{meshes}'
                f'{header_tags(text)}\taniEnum\n\t{{\n{"".join(entries)}\t}}\n}}\n')
    overlays = []
    for overlay in model_scripts(gothic_dir, "HUMANS_"):
        if OTHER_OVERLAYS.match(overlay):
            continue
        name = MODEL + overlay[len("HUMANS"):]
        overlay_text = re.sub(r"//[^\n]*", "", model_script(gothic_dir, overlay))
        entries = convert_script(gothic, steve, overlay_text, overlay, name, anims, written)
        with open(os.path.join(anims, name + ".MDS"), "w", newline="\r\n") as f:
            f.write(f"// Overlay for {MODEL}.MDS in place of {overlay}.MDS, applied by the Minecraft plugin (make_steve.py)\n"
                    f'Model ("{name}")\n{{\n\tmeshAndTree ("{BODY}.ASC" DONT_USE_MESH)\n'
                    f'\taniEnum\n\t{{\n{"".join(entries)}\t}}\n}}\n')
        overlays.append(name)
    # the zombie's raised arms
    entries = convert_script(gothic, steve, text, "HUMANS", ZOMBIE_OVERLAY, anims, written, only=ZOMBIE_MOVES,
                             pose_hook=zombie_arms(steve))
    with open(os.path.join(anims, ZOMBIE_OVERLAY + ".MDS"), "w", newline="\r\n") as f:
        f.write(f"// The zombie's raised arms over {MODEL}.MDS, applied by the Minecraft plugin (make_steve.py)\n"
                f'Model ("{ZOMBIE_OVERLAY}")\n{{\n\tmeshAndTree ("{BODY}.ASC" DONT_USE_MESH)\n'
                f'\taniEnum\n\t{{\n{"".join(entries)}\t}}\n}}\n')
    overlays.append(ZOMBIE_OVERLAY)

    # the creeper: its own skeleton and Gothic's animations on it
    creeper = Steve(gothic, CREEPER_NODES, arm_swing=1.4)
    creeper_skin = mob_skin(files, CREEPER_SKIN)
    creeper_body = "MC_CREEPER_BODY"
    write_body(os.path.join(anims, creeper_body + ".ASC"), creeper, creeper_skin, CREEPER_PARTS, "MC_CREEPER", "zm_Creeper")
    write_skin_texture(creeper_skin, gothic_dir, "MC_CREEPER")
    written.add(creeper_body + ".ASC")
    entries = convert_script(gothic, creeper, text, "HUMANS", CREEPER, anims, written)
    with open(os.path.join(anims, CREEPER + ".MDS"), "w", newline="\r\n") as f:
        f.write(f"// Minecraft's creeper playing Gothic's human animations (generated by make_steve.py)\n"
                f'Model ("{CREEPER}")\n{{\n\tmeshAndTree ("{creeper_body}.ASC" DONT_USE_MESH)\n'
                f'\tregisterMesh ("{creeper_body}.ASC")\n{header_tags(text)}\taniEnum\n\t{{\n{"".join(entries)}\t}}\n}}\n')

    # what Gothic compiled from earlier sources, and sources this run didn't write
    compiled = os.path.join(anims, "_COMPILED")
    stale = []
    for prefix in (MODEL, CREEPER, "MC_"):
        stale += glob.glob(os.path.join(compiled, prefix + "*.M[DS][HMB]")) + glob.glob(os.path.join(compiled, prefix + "*-*.MAN"))
        stale += [p for p in glob.glob(os.path.join(anims, prefix + "*.ASC")) if os.path.basename(p).upper() not in written]
    for path in stale:
        os.remove(path)
    print(f"Steve (Minecraft {version}): {len(verts)} vertices, {len(faces)} faces; {len(written)} sources; overlays "
          f"{', '.join(overlays)}; the zombie's and the skeleton's bodies; the creeper ({CREEPER}.MDS)")
    if preview:
        render_preview(preview, gothic, steve, skin, verts, faces, uvs, uv_faces, weights)
        for name, path, parts in MOBS:
            mob = mob_skin(files, path)
            render_preview(os.path.join(preview, name), gothic, steve, mob, *build_mesh(mob, parts))
        render_preview(os.path.join(preview, CREEPER), gothic, creeper, creeper_skin, *build_mesh(creeper_skin, CREEPER_PARTS))


def render_preview(folder, gothic, steve, skin, verts, faces, uvs, uv_faces, weights):
    """PNG previews: the bind pose and frames of a few animations, front and side."""
    from render import render

    class Tex:
        width, height, rgba = skin.width, skin.height, skin.tobytes()
    os.makedirs(folder, exist_ok=True)
    local_bind = {}
    for vi, links in enumerate(weights):
        node = links[0][0]
        local_bind[vi] = apply(transpose(steve.bind_rot[node]), sub(verts[vi], steve.bind_pos[node]))
    shots = [("bind", None, 0)]
    for name, f in (("S_RUN", 0), ("S_RUNL", 0), ("S_RUNL", 5), ("S_WALKL", 10), ("T_LADDER_S0_2_S1", 10),
                    ("S_1HATTACK", 8), ("S_SWIM", 0), ("S_DEAD", 0), ("S_BENCH_S1", 0)):
        shots.append((name, name, f))
    for label, name, f in shots:
        if name is None:
            world = {n: (steve.bind_rot[n], steve.bind_pos[n]) for n in steve.order}
        else:
            data = compiled_animation(gothic.dir, name)
            if data is None:
                continue
            frames, _ = gothic.frames(data)
            poses = [steve.pose(r, root, False) for r, root in frames]
            if SWING.match(name):
                poses = swing(steve, poses, name)
            local, root = poses[min(f, len(poses) - 1)]
            world = steve.world(local, root)
        pts = [max_pos(add(world[weights[i][0][0]][1], apply(world[weights[i][0][0]][0], local_bind[i]))) for i in range(len(verts))]
        image_uv = [(u, 1 - v) for u, v in uvs]  # the renderer reads textures top down
        tris = [((pts[a], pts[b], pts[c]), (image_uv[ua], image_uv[ub], image_uv[uc]), Tex)
                for (a, b, c, _), (ua, ub, uc) in zip(faces, uv_faces)]
        tag = f"{label}_{f}" if name else label
        render(os.path.join(folder, f"steve_{tag}_back.png"), tris, up=2, right=0, depth=1, size=(300, 520), cull=True)
        front = [((tuple((-p[0], -p[1], p[2]) for p in tri)), uv, t) for tri, uv, t in tris]
        render(os.path.join(folder, f"steve_{tag}_front.png"), front, up=2, right=0, depth=1, size=(300, 520), cull=True)
        render(os.path.join(folder, f"steve_{tag}_side.png"), tris, up=2, right=1, depth=0, size=(300, 520), cull=True)


def patch_hero(gothic_dir):
    script, path = load_dat(gothic_dir)
    set_hero_visual(script, MODEL + ".MDS", BODY, "")
    save_dat(script, path)
    print(f"Gothic.dat: hero is now {MODEL}.MDS / {BODY} (original kept as Gothic.dat.orig)")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--minecraft", default=DEFAULT_MINECRAFT, help="the Minecraft folder (%%APPDATA%%\\.minecraft)")
    parser.add_argument("--gothic", required=True, help="the Gothic 1 or Gothic 2 folder")
    parser.add_argument("--preview", help="also render PNG previews of Steve into this folder")
    args = parser.parse_args()
    build(args.minecraft, args.gothic, args.preview)
    patch_hero(args.gothic)


if __name__ == "__main__":
    main()
