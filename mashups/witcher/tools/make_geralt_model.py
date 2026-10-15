"""Put Geralt of Rivia from The Witcher 3 into Gothic 1: his skeleton, skinned body and textures.

Geralt keeps his own Witcher 3 skeleton (man_base.w2rig), his exact meshes and skin weights and
his textures. Gothic receives them in its own source formats and compiles them the first time
the hero appears:
  _work\\DATA\\ANIMS\\GERALT_BODY.ASC            his skeleton and skinned mesh (cut into soft skins of
                                               fewer than 2048 positions each), bind pose as in W3
  _work\\DATA\\TEXTURES\\_COMPILED\\GERALT_<nn>-C.TEX  his textures, each its own material at up to
                                               --tile pixels (the game's own go to 4096),
                                               colour-shifted, with the normal maps baked in
The animations (GERALT.MDS and GERALT_<animation>.ASC) come from make_geralt_anims.py.

His look is the start of the game: the items the game gives him (gameplay/items/*.xml:
Starting Armor/Pants/Gloves/Boots = the Viper set, head_2, the "Half With Tail" hairstyle),
each item's entity template resolved to its mesh components (w3model.entity_meshes). HairWorks
fur (hair, beard) is replaced by the plain mesh the game itself falls back to. Bones the
skeleton lacks (face, hair and cloth dangles) are folded into the nearest skeleton bone.

Armour colours: a tinted material's diffuse holds placeholder hues that the game shifts by the
item's colour entries (colorShift1/2: hue, saturation, luminance). Approximated here: pixels
near red take colorShift1, near blue colorShift2, grey ones stay (the materials' KeepGray);
the shift works like a hue/saturation/lightness adjustment, its hue 180 leaving the hue as it is
(checked against the game's inventory icons of the same items); grey ones keep their hue but take
colorShift1's lightness. The hue rule fails for the red placeholders, so each item is then matched
to its inventory icon (item_gain: the red placeholder takes the icon's hue, and the item's mean
colour moves to the icon's).

usage: python make_geralt_model.py --w3 "<Witcher 3 folder>" [--gothic DIR] [--preview DIR]
       [--tile 2048]
"""
import argparse
import colorsys
import glob
import math
import os
import sys

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
from asc import helper_block, write_softskin_meshes
from tex import write_alpha_tex, write_tex
from w3bundle import W3Install
from w3model import Bone, Mesh, entity_colorings, entity_meshes, material_params, read_rig
from cr2w import CR2W
from w3texture import TextureCache, load_rgba

DEFAULT_GOTHIC = r"C:\Program Files (x86)\Steam\steamapps\common\Gothic"
DEFAULT_W3 = r"C:\Program Files (x86)\Steam\steamapps\common\The Witcher 3"
BODY, TEXTURE = "GERALT_BODY", "GERALT"
SCALE = 100  # Witcher 3 metres -> Gothic centimetres
RIG = "characters/base_entities/man_base/man_base.w2rig"
# Witcher 3 space faces +Y, Gothic's ASC space faces -Y (both Z up): a half turn about Z.
W3_TO_GOTHIC = [[-1, 0, 0], [0, -1, 0], [0, 0, 1]]
# Geralt's equipment at the start of the game, by equip template (gameplay/items/*.xml).
PARTS = [
    "h_02_mg__geralt.w2ent",        # head_2: head, eyes, beard
    "c_01b_mg__witcher.w2ent",      # Half With Tail Hairstyle
    "t_01_mg__viper_lvl3.w2ent",    # Starting Armor
    "l_01_mg__viper_lvl3.w2ent",    # Starting Pants
    "g_01a_mg__viper_lvl3.w2ent",   # Starting Gloves
    "s_01_mg__viper_lvl3.w2ent",    # Starting Boots
]
# The game's inventory icons of those items (their icon_path in gameplay/items/*.xml, in
# content0's texture.cache): renders of the items in their real colours, which the colour shift
# only approximates (see item_gain). With each, the item's main diffuse.
_ICON_DIR = "gameplay\\gui_new\\icons\\inventory\\armors\\"
_VIPER_DIR = "characters\\models\\geralt\\armor\\armor__viper\\"
ICONS = {
    "t_01_mg__viper_lvl3.w2ent": (_ICON_DIR + "armor-01.png", _VIPER_DIR + "t_01_mg__viper_d01.xbm"),
    "l_01_mg__viper_lvl3.w2ent": (_ICON_DIR + "trousers-01.png", _VIPER_DIR + "l_01_mg__viper_d01.xbm"),
    "g_01a_mg__viper_lvl3.w2ent": (_ICON_DIR + "gauntlet-01.png", _VIPER_DIR + "g_01_mg__viper_d01.xbm"),
    "s_01_mg__viper_lvl3.w2ent": (_ICON_DIR + "boots-01.png", _VIPER_DIR + "s_01_mg__viper_d01.xbm"),
}
EXTRA_MESHES = ["characters/models/geralt/head/model/h_mg__neck_transition.w2mesh"]  # geralt_player.w2ent
# His face moves as in The Witcher 3: the face rig's bones (h_01_mg__geralt.w3fac, w3mimic.py) that
# the poses of FACE_ANIS move join his skeleton under the head, and his head meshes keep their own
# skinning to them instead of being folded into the head. make_geralt_anims.py writes FACE_ANIS as
# layered animations of those bones alone, the Witcher plugin plays them (talking while he
# speaks, blinking, hurt, expressions). A Gothic morph-mesh head (what Gothic plays VISEME,
# R_EYESBLINK and the S_ shapes on) cannot hold him: zCProgMeshProto::RenderProgMesh (0x5A6B5F)
# draws no mesh of 2048 positions or more, and his head has 7131.
MIMIC_FACE = "characters\\models\\geralt\\head\\model\\h_01_mg__geralt.w3fac"
HEAD_NODE = "head"


def _lr(pattern, w):
    """A left/right pair of the face rig's poses: pattern with {} for left/right."""
    return {pattern.format("left"): w, pattern.format("right"): w}


def _scaled(weights, k):
    return {n: w * k for n, w in weights.items()}


MOUTH = {
    "rest": {},
    "a": {"jaw_open_a": 0.35},
    "aa": {"jaw_open_a": 0.6},
    "o": {"jaw_open_o": 0.5, "lips_out": 0.35},
    "u": {"jaw_open_o": 0.2, "lips_out": 0.6},
    "e": {"jaw_open_a": 0.25} | _lr("corner_{}_spread", 0.45),
    "i": {"jaw_open_a": 0.12} | _lr("corner_{}_spread", 0.6),
    "m": {"lips_in": 0.35},
    "f": {"jaw_open_a": 0.12, "lip_lower_in": 0.5},
}
BLINK = _lr("eyelids_upper_{}_down", 1.0) | _lr("eyelids_lower_{}_up", 0.3)
GRIMACE = (_lr("brow_{}_down", 0.7) | _lr("brow_{}_in", 0.6) | _lr("eyelids_upper_{}_down", 0.5)
           | _lr("eyelids_lower_{}_up", 0.5) | _lr("cheek_{}_up", 0.4) | _lr("corner_{}_tight", 0.4)
           | {"nose_up": 0.5, "jaw_open_a": 0.2})
FRIENDLY = _lr("corner_{}_up", 0.55) | _lr("cheek_{}_up", 0.35) | _lr("eyelids_lower_{}_up", 0.15)
ANGRY = _lr("brow_{}_down", 0.7) | _lr("brow_{}_in", 0.6) | _lr("corner_{}_down", 0.3) | {"nose_up": 0.25}
HOSTILE = (_lr("brow_{}_down", 0.9) | _lr("brow_{}_in", 0.8) | _lr("corner_{}_tight", 0.5)
           | _lr("eyelids_upper_{}_down", 0.25) | {"nose_up": 0.5})
FRIGHTENED = _lr("brow_inner_{}_up", 0.8) | _lr("eyelids_upper_{}_up", 0.6) | _lr("corner_{}_down", 0.4) | {"jaw_open_a": 0.2}
# (Gothic name, layer, next, blend in, blend out (s), frames from one key to the next, keys as
# pose weights): talking loops (his mouth through the shapes of speech, a key every 0.16 s), the
# expressions hold, blinking and hurt play once
FACE_ANIS = [
    ("t_W3_FaceTalk", 25, "t_W3_FaceTalk", 0.15, 0.2, 4,
     [MOUTH[s] for s in ("rest", "a", "e", "aa", "m", "o", "a", "i", "f", "aa", "e", "u", "a", "m", "e", "rest")]),
    ("t_W3_FaceBlink", 27, "", 0.05, 0.05, 1, [_scaled(BLINK, k) for k in (0.0, 0.55, 1.0, 1.0, 0.5, 0.0)]),
    ("t_W3_FaceHurt", 26, "", 0.05, 0.3, 3, [_scaled(GRIMACE, k) for k in (0.0, 0.6, 1.0, 1.0, 1.0, 0.7, 0.3, 0.0)]),
    ("s_W3_FaceFriendly", 24, "s_W3_FaceFriendly", 0.3, 0.3, 1, [FRIENDLY, FRIENDLY]),
    ("s_W3_FaceAngry", 24, "s_W3_FaceAngry", 0.3, 0.3, 1, [ANGRY, ANGRY]),
    ("s_W3_FaceHostile", 24, "s_W3_FaceHostile", 0.3, 0.3, 1, [HOSTILE, HOSTILE]),
    ("s_W3_FaceFrightened", 24, "s_W3_FaceFrightened", 0.3, 0.3, 1, [FRIGHTENED, FRIGHTENED]),
    ("s_W3_FaceEyesClosed", 24, "s_W3_FaceEyesClosed", 0.3, 0.3, 1, [BLINK, BLINK]),
]


def face_keys(keys, per_key):
    """Pose weights per frame: the keys, `per_key` frames apart, blended linearly in between."""
    frames = []
    for a, b in zip(keys, keys[1:]):
        for f in range(per_key):
            t = f / per_key
            frames.append({n: a.get(n, 0.0) * (1 - t) + b.get(n, 0.0) * t for n in set(a) | set(b)})
    return frames + [keys[-1]]
# Rig bones Gothic does not get. The Witcher 3 moves a figure in its pelvis track (in model space;
# Root stays put, Trajectory and the motion extraction repeat the pelvis's ground motion), while
# Gothic moves it by the translation of its root node Bip01 and takes only rotations from the other
# nodes: so Geralt's pelvis becomes Bip01 and Root, Trajectory, Reference and the IK helpers go.
ROOT = "pelvis"
SKIP_BONES = ("Root", "Trajectory", "Reference")
# Gothic 1 renders a soft skin only below 2048 positions, and its dynamic LOD remaps at most 4096
# wedges per submesh into a static table (zCProgMeshProto::RenderProgMesh/RenderDynamicLOD,
# zCMeshSoftSkin::RenderSoftSkin): Geralt in one piece (32,737 vertices) crashed the game when he
# came into view. He is cut into several soft skins below these, with a margin.
MAX_POSITIONS, MAX_WEDGES = 1900, 4000
# Materials by their base graph (engine\materials\graphs\*.w2mg):
SKIP_GRAPHS = ("pbr_eye_shadow",)            # a dark shell over the eyes the game blends; opaque it masked them
ALPHA_GRAPHS = ("pbr_hair", "eyelash")       # hair cards: textures with alpha (DXT3, see write_alpha_tex)
# Colours for materials whose diffuse is no real texture (the editor's grey.xbm, or none cached):
# the material's own VarianceColor where it has one, else these by depot path
FLAT_COLORS = (("eye_meat", (170, 105, 100)), ("neck", (196, 150, 128)))
# Gothic's item slots, hung under Geralt's own weapon bones; ZS_NULL is an empty node that the
# stand-ins for Gothic's layered animations key (make_geralt_anims.py), so they move nothing.
SLOTS = {"ZS_RIGHTHAND": "r_weapon", "ZS_LEFTHAND": "l_weapon", "ZS_NULL": "pelvis"}


# --- maths (row-vector matrices, as asc.py and the Gothic ASC files use) -------------------------
def vecmat(v, m): return [sum(v[k] * m[k][j] for k in range(3)) for j in range(3)]
def matmul(a, b): return [vecmat(r, b) for r in a]
def transpose(m): return [list(c) for c in zip(*m)]
def add(a, b): return [x + y for x, y in zip(a, b)]


def quat_rows(x, y, z, w):
    """Row-vector rotation of a Witcher 3 quaternion (the transpose of its column-vector matrix;
    checked: the rig composed this way gives the mesh's bone positions to 0.1 mm)."""
    return [[1 - 2 * (y * y + z * z), 2 * (x * y + z * w), 2 * (x * z - y * w)],
            [2 * (x * y - z * w), 1 - 2 * (x * x + z * z), 2 * (y * z + x * w)],
            [2 * (x * z + y * w), 2 * (y * z - x * w), 1 - 2 * (x * x + y * y)]]


def rows_quat(m):
    """The quaternion (x, y, z, w) of a row-vector rotation (the inverse of quat_rows)."""
    t = m[0][0] + m[1][1] + m[2][2]
    if t > 0:
        s = math.sqrt(t + 1.0) * 2
        return ((m[1][2] - m[2][1]) / s, (m[2][0] - m[0][2]) / s, (m[0][1] - m[1][0]) / s, 0.25 * s)
    if m[0][0] > m[1][1] and m[0][0] > m[2][2]:
        s = math.sqrt(1.0 + m[0][0] - m[1][1] - m[2][2]) * 2
        return (0.25 * s, (m[1][0] + m[0][1]) / s, (m[2][0] + m[0][2]) / s, (m[1][2] - m[2][1]) / s)
    if m[1][1] > m[2][2]:
        s = math.sqrt(1.0 + m[1][1] - m[0][0] - m[2][2]) * 2
        return ((m[1][0] + m[0][1]) / s, 0.25 * s, (m[2][1] + m[1][2]) / s, (m[2][0] - m[0][2]) / s)
    s = math.sqrt(1.0 + m[2][2] - m[0][0] - m[1][1]) * 2
    return ((m[2][0] + m[0][2]) / s, (m[2][1] + m[1][2]) / s, 0.25 * s, (m[0][1] - m[1][0]) / s)


def geralt_skeleton(w3):
    """(Skeleton, MimicFace): man_base's rig with the face bones FACE_ANIS move under its head,
    each at its rest place relative to its parent (the nearest of them above it, else the head)."""
    from w3mimic import MimicFace, compose, inverse
    rig = read_rig(CR2W(w3.read(RIG), RIG, w3))
    face = MimicFace(CR2W(w3.read(MIMIC_FACE), MIMIC_FACE, w3))
    moved = set()
    for _name, _layer, _next, _bin, _bout, per_key, keys in FACE_ANIS:
        for weights in keys:
            moved |= face.moved(weights)
    have = {b.name for b in rig}
    body_count = len(rig)
    names = [n for n in face.names if n in moved and n not in have and not n.startswith("placer_")]
    index = {b.name: i for i, b in enumerate(rig)}
    for name in names:  # the face rig's order: parents first
        parent = face.parent_in(name, set(names))
        local = compose(face.rest[name], inverse(face.rest[parent])) if parent != HEAD_NODE else face.rest[name]
        index[name] = len(rig)
        rig.append(Bone(name, index[parent], local[1], rows_quat(local[0]), (1.0, 1.0, 1.0)))
    skeleton = Skeleton(rig)
    skeleton.face = set(names)
    skeleton.body_count = body_count
    return skeleton, face


class Skeleton:
    def __init__(self, bones):
        self.bones = [b for b in bones if b.name not in SKIP_BONES and not b.name.startswith("IK_")]
        self.world = {}  # W3 name -> (rows, pos) bind pose in Witcher 3 space
        for b in bones:
            rows = quat_rows(*b.rot)
            if b.parent < 0:
                self.world[b.name] = (rows, list(b.pos))
            else:
                prows, ppos = self.world[bones[b.parent].name]
                self.world[b.name] = (matmul(rows, prows), add(vecmat(b.pos, prows), ppos))
        self.rig = bones
        self.parent = {b.name: (bones[b.parent].name if b.parent >= 0 else None) for b in bones}
        self.face = set()  # the face rig's bones (geralt_skeleton)
        self.body_count = 0  # man_base's bones before them
        for name in self.parent:  # skipped bones' children hang on their nearest kept ancestor
            p = self.parent[name]
            while p is not None and (p in SKIP_BONES or p.startswith("IK_")):
                p = self.parent[p]
            self.parent[name] = p

    def pose_world(self, pose, motion=(0.0, 0.0, 0.0, 0.0)):
        """{W3 name: (rows, pos)} in Witcher 3 model space for an animation pose
        ([(pos, quat, scale)] per rig bone, parent space) at root motion `motion` (x, y, z, yaw).

        The game places the pelvis relative to the Trajectory bone and moves that by the root
        motion: Root's own track (a quarter turn about X and an offset in many animations) is
        not applied, and pelvis and Trajectory both carry the ground motion, which cancels.
        Checked on Geralt's sword set: the pelvis stays over the origin at the start of
        attacks that begin 2 m behind it, and moves exactly the root motion's distance."""
        # bones an animation has no track for (some sets key fewer than the rig) keep their rest pose
        local = {b.name: (quat_rows(*b.rot), list(b.pos)) for b in self.rig}
        body = self.rig[:self.body_count] if self.body_count else self.rig  # tracks are man_base's bones
        local.update({b.name: (quat_rows(*quat), list(pos)) for b, (pos, quat, _s) in zip(body, pose)})
        trows, tpos = local.get("Trajectory", (quat_rows(0, 0, 0, 1), [0.0, 0.0, 0.0]))
        prows, ppos = local[ROOT]
        inv = transpose(trows)
        rel_rows = matmul(prows, inv)
        rel_pos = vecmat([a - c for a, c in zip(ppos, tpos)], inv)
        x, y, z, yaw = motion
        c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
        yaw_rows = [[c, s, 0], [-s, c, 0], [0, 0, 1]]
        world = {ROOT: (matmul(rel_rows, yaw_rows), add(vecmat(rel_pos, yaw_rows), [x, y, z]))}
        for b in self.rig:
            if b.name == ROOT or b.parent < 0:
                continue
            parent = self.rig[b.parent].name
            if parent not in world:
                continue  # Root's other children: Trajectory, Reference, the IK helpers
            rows, pos = local[b.name]
            prw, ppw = world[parent]
            world[b.name] = (matmul(rows, prw), add(vecmat(pos, prw), ppw))
        return world

    def to_gothic(self, world):
        """{W3 name: (rows, pos)} of kept bones and slots in Gothic space (centimetres). Bones keep
        their Witcher 3 local axes (so W3 animation rotations apply unchanged); only the whole
        figure turns into Gothic space."""
        out = {}
        for b in self.bones:
            rows, pos = world[b.name]
            out[b.name] = (matmul(rows, W3_TO_GOTHIC), [c * SCALE for c in vecmat(pos, W3_TO_GOTHIC)])
        for slot, bone in SLOTS.items():
            out[slot] = out[bone]
        return out

    @staticmethod
    def gothic_name(name):
        """Gothic-style node names, so engine lookups such as BIP01 HEAD find Geralt's bones."""
        if name in SLOTS:
            return name
        return "Bip01" if name == ROOT else "Bip01 " + name

    def nearest(self, pos):
        """The body bone nearest to a point (a bone Gothic does not get is folded into it; face
        bones the poses leave still go to the body, not to a moving neighbour)."""
        return min((b for b in self.bones if b.name not in self.face),
                   key=lambda b: sum((a - c) ** 2 for a, c in zip(self.world[b.name][1], pos))).name

    def rest_gothic(self):
        """{W3 name: (rows, pos)} of the bind pose in Gothic space (centimetres), slots included."""
        return self.to_gothic(self.world)

    def helper_blocks(self, rest, frames=None, animated=None):
        """ASC skeleton nodes at the rest pose; with `frames` ([{W3 name: (rows, pos)}] in Gothic
        space, one per frame) the `animated` nodes also carry those world-space samples (by
        default all but the face's: Gothic leaves the nodes an animation has no samples for to
        other layers, so the body's animations leave the face to its own)."""
        blocks = []
        for name in [b.name for b in self.bones] + list(SLOTS):
            parent = SLOTS.get(name) or self.parent[name]
            rows, pos = rest[name]
            moves = name not in self.face if animated is None else name in animated
            animation = [f[name] for f in frames] if frames and moves else None
            blocks.append(helper_block(self.gothic_name(name), self.gothic_name(parent) if parent else None,
                                       rows, pos, animation))
        return blocks


# --- textures ------------------------------------------------------------------------------------
def shift_pixel(r, g, b, shift):
    hue, sat, lum = shift.get("hue", 0), shift.get("saturation", 0), shift.get("luminance", 0)
    h, l, s = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
    # 180 is no change: checked against the game's own inventory icons of the starting gear
    # (armor-01, trousers-01, boots-01, gauntlet-01): the trousers' blue placeholders with hue 326
    # come out brown knee pads, the boots' red ones with 324 a dark olive grey
    h = (h + (hue - 180) / 360) % 1.0
    s = s * (1 + sat / 100) if sat < 0 else s + (1 - s) * sat / 100
    l = l * (1 + lum / 100) if lum < 0 else l + (1 - l) * lum / 100
    return tuple(int(max(0.0, min(1.0, c)) * 255 + 0.5) for c in colorsys.hls_to_rgb(h, l, s))


def _np():
    import numpy
    return numpy


def _hls(np, rgb):
    """colorsys.rgb_to_hls over an (n, 3) array in 0..1."""
    mx, mn = rgb.max(1), rgb.min(1)
    l = (mx + mn) / 2
    d = mx - mn
    s = np.where(d == 0, 0.0, d / np.maximum(1e-9, 1 - np.abs(2 * l - 1)))
    r, g, b = rgb[:, 0], rgb[:, 1], rgb[:, 2]
    dd = np.maximum(d, 1e-9)
    h = np.where(mx == r, (g - b) / dd, np.where(mx == g, 2 + (b - r) / dd, 4 + (r - g) / dd))
    h = np.where(d == 0, 0.0, (h / 6) % 1.0)
    return h, l, s


def _rgb(np, h, l, s):
    """colorsys.hls_to_rgb over arrays."""
    c = (1 - np.abs(2 * l - 1)) * s
    x = c * (1 - np.abs((h * 6) % 2 - 1))
    m = l - c / 2
    k = (h * 6).astype(int) % 6
    z = np.zeros_like(c)
    r = np.choose(k, [c, x, z, z, x, c])
    g = np.choose(k, [x, c, c, x, z, z])
    b = np.choose(k, [z, z, x, c, c, x])
    return np.stack([r + m, g + m, b + m], 1)


def color_shift(rgba, shifts, red_hue=None):
    """The approximation of the game's colour shift described in the module docstring
    (shift_pixel for every pixel: near red with colorShift1, near blue with colorShift2).
    `red_hue` (degrees): the hue the red placeholder takes instead (see item_gain)."""
    np = _np()
    a = np.frombuffer(rgba, np.uint8).reshape(-1, 4).astype(np.float64)
    rgb = a[:, :3] / 255
    h, l, s = _hls(np, rgb)
    keep = (s < 0.15) | (l < 0.04)
    near_red = np.minimum(h, 1 - h) < np.abs(h - 2 / 3)
    out = rgb.copy()
    second = shifts[1] if len(shifts) > 1 else None
    for shift, where, fixed in ((shifts[0], near_red & ~keep, red_hue), (second, ~near_red & ~keep, None)):
        if not shift or not where.any():
            continue
        hue, sat, lum = shift.get("hue", 0), shift.get("saturation", 0), shift.get("luminance", 0)
        if fixed is not None:
            hh = np.full(int(where.sum()), (fixed / 360) % 1.0)
        else:
            hh = (h[where] + (hue - 180) / 360) % 1.0  # 180 is no change (see shift_pixel)
        ss = s[where] * (1 + sat / 100) if sat < 0 else s[where] + (1 - s[where]) * sat / 100
        ll = l[where] * (1 + lum / 100) if lum < 0 else l[where] + (1 - l[where]) * lum / 100
        out[where] = _rgb(np, hh, ll, ss)
    # grey texels (KeepGray) keep their hue and saturation but take colorShift1's lightness: the
    # icons show the jacket's grey panels as dark as its leather, not the texture's light grey
    grey = keep & (l >= 0.04)
    lum = shifts[0].get("luminance", 0) if shifts[0] else 0
    if lum < 0 and grey.any():
        out[grey] = rgb[grey] * (1 + lum / 100)
    a[:, :3] = np.clip(out, 0, 1) * 255 + 0.5
    return a.astype(np.uint8).tobytes()


# Matching an item to its inventory icon. The colour shift gets hue and lightness only roughly
# right: the hue rule fits the blue placeholders (knee pads, cuffs, straps come out the icons'
# browns) but not the red ones (the boots' comes out green (41, 53, 47) where the icon's boots
# are a dark olive brown (50, 48, 36), the gloves' purple), and the trousers are a neutral grey
# (86) where the icon's are warm, dark leather (48, 41, 35). So the red placeholder takes the
# icon's mean hue (its saturation and lightness still the shift's), and then the mean colour of
# the item's shifted main diffuse is moved to the icon's: the balance of the channels exactly
# (within MATCH_GAIN), the lightness to ICON_LIGHT times the icon's (within MATCH_LIGHT), as
# Gothic lights a character more dimly than the icons are lit. Every tinted texture of the item
# takes the same hue and gains.
ICON_LIGHT, MATCH_LIGHT, MATCH_GAIN = 1.3, (0.65, 1.2), (0.6, 1.6)
_item_gains = {}


def _mean_rgb(rgba, opaque=False):
    np = _np()
    a = np.frombuffer(rgba, np.uint8).reshape(-1, 4).astype(np.float64)
    if opaque:
        a = a[a[:, 3] > 200]
    return a[:, :3].mean(0) if len(a) else None


def item_gain(cache, icon, shifts, tile):
    """(the red placeholder's hue in degrees, per-channel gains (r, g, b)) that bring an item's
    colours to its icon's, or (None, None)."""
    icon_name, diffuse = icon
    key = (icon_name, repr(shifts))
    if key not in _item_gains:
        hue = gain = None
        if cache.get(icon_name) and cache.get(diffuse):
            target = _mean_rgb(load_rgba(cache, icon_name, 512)[2], opaque=True)
            rgba = load_rgba(cache, diffuse, min(tile, 512))[2]
            if target is not None and target.mean() >= 1:
                hue = colorsys.rgb_to_hls(*(target / 255))[0] * 360
                mean = _mean_rgb(color_shift(rgba, shifts, hue) if shifts else rgba)
                if mean is not None and mean.mean() >= 1:
                    np = _np()
                    balance = np.clip((target / target.mean()) / (mean / mean.mean()), *MATCH_GAIN)
                    light = float(np.clip(target.mean() * ICON_LIGHT / mean.mean(), *MATCH_LIGHT))
                    gain = tuple(float(g) * light for g in balance)
                    print(f"{diffuse.split(chr(92))[-1]}: red hue {hue:.0f}, mean {mean.round().tolist()} -> icon "
                          f"{target.round().tolist()}: gains {[round(g, 2) for g in gain]}")
        _item_gains[key] = (hue, gain)
    return _item_gains[key]


def apply_gain(rgba, gain):
    np = _np()
    a = np.frombuffer(rgba, np.uint8).reshape(-1, 4).astype(np.float32)
    a[:, :3] = np.clip(a[:, :3] * np.array(gain, np.float32), 0, 255)
    return (a + 0.5).astype(np.uint8).tobytes()


# The light the normal maps are baked with: from above and in front of the surface, a little to
# the side (tangent space); BAKE_STRENGTH scales how far a texel goes from its flat brightness.
BAKE_LIGHT, BAKE_STRENGTH = (0.2, 0.2, 1.0), 0.8


def bake_normals(w, h, rgba, normal):
    """Gothic lights a character per vertex and has no normal maps, which in The Witcher 3 carry
    the face's wrinkles and pores and the leather's grain: their shading under BAKE_LIGHT is
    multiplied into the diffuse (a flat texel keeps its colour). `normal`: (w, h, rgba) of the
    tangent-space normal map (RGB = XYZ), resampled to the diffuse's size."""
    np = _np()
    nw, nh, nrgba = normal
    n = np.frombuffer(nrgba, np.uint8).reshape(nh, nw, 4)[:, :, :3].astype(np.float32) / 127.5 - 1
    ys = (np.arange(h) * nh // h)[:, None]
    xs = (np.arange(w) * nw // w)[None, :]
    n = n[ys, xs].reshape(-1, 3)
    n /= np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-6)
    light = np.array(BAKE_LIGHT, np.float32)
    light /= np.linalg.norm(light)
    shade = np.clip(1 + BAKE_STRENGTH * ((n @ light) / light[2] - 1), 0.35, 1.25)
    a = np.frombuffer(rgba, np.uint8).reshape(-1, 4).astype(np.float32)
    a[:, :3] = np.clip(a[:, :3] * shade[:, None], 0, 255)
    return a.astype(np.uint8).tobytes()


# Gothic gives a model's mesh library at most four textures (zCModelTexAniState::FinishTexList,
# zMDL_MAX_TEX: "Too many textures in meshLib", the list cut to four; with Geralt's 17 textures
# each its own material the game crashed loading him). They go into four: his face alone at full
# size, a sheet for the rest of him, a sheet with alpha for his hair, beard and lashes, and the
# chain mail, which repeats up to 35 times and so cannot sit in a sheet.
FACE, ARMOR, HAIR, TILE = range(4)
SHEET_NAMES = {FACE: TEXTURE + "_FACE", ARMOR: TEXTURE + "_ARMOR", HAIR: TEXTURE + "_HAIR", TILE: TEXTURE + "_TILE"}
FACE_TEXTURE = "h_01_mg__geralt_d02"  # his face's diffuse
# Regions of power-of-two sizes, packed tallest first, sit on multiples of their own size: each
# mipmap level averages blocks inside one region, so nothing of a neighbour bleeds in and no
# gutter is needed (one would keep two 1024 textures from sharing a 2048 row).
SHEET_SIZE, GUTTER = 2048, 0


# The Witcher's materials lay detail normal maps over the main one (leather grain, cloth weave),
# repeated DetailTile times across the texture at DetailPower: baked as bake_normals does, softer.
DETAIL_STRENGTH = 0.35
DETAIL_MAPS = (("DetailNormal", "DetailTile"), ("DetailNormal1", "DetailTile1"), ("DetailNormal2", "DetailTile2"))


def bake_detail(w, h, rgba, detail, repeat, power):
    """bake_normals for a detail normal map repeated (x, y) times across the diffuse."""
    np = _np()
    dw, dh, drgba = detail
    n = np.frombuffer(drgba, np.uint8).reshape(dh, dw, 4)[:, :, :3].astype(np.float32) / 127.5 - 1
    ys = ((np.arange(h) * repeat[1] / h) % 1.0 * dh).astype(int)[:, None]
    xs = ((np.arange(w) * repeat[0] / w) % 1.0 * dw).astype(int)[None, :]
    n = n[ys, xs].reshape(-1, 3)
    n /= np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-6)
    light = np.array(BAKE_LIGHT, np.float32)
    light /= np.linalg.norm(light)
    strength = DETAIL_STRENGTH * min(max(power, 0.0), 1.5)
    shade = np.clip(1 + strength * ((n @ light) / light[2] - 1), 0.5, 1.15)
    a = np.frombuffer(rgba, np.uint8).reshape(-1, 4).astype(np.float32)
    a[:, :3] = np.clip(a[:, :3] * shade[:, None], 0, 255)
    return a.astype(np.uint8).tobytes()


# Hair cards in The Witcher 3 are alpha-blended; Gothic tests alpha against a threshold, which cut
# away the fainter strands (hair alpha averages 77/255, the beard 30/255): alpha is raised by this.
HAIR_ALPHA_GAIN = 1.6


def gain_alpha(texture, gain):
    np = _np()
    a = np.frombuffer(texture.rgba, np.uint8).reshape(-1, 4).copy()
    a[:, 3] = np.minimum(a[:, 3].astype(np.float32) * gain, 255).astype(np.uint8)
    return Texture(texture.width, texture.height, a.tobytes())


class Texture:
    def __init__(self, width, height, rgba):
        self.width, self.height, self.rgba = width, height, rgba


class Single:
    """A texture of its own, repeating by itself (the face, the chain mail)."""

    def __init__(self, name, alpha=False):
        self.name, self.alpha = name, alpha
        self.width = self.height = 0
        self.rgba = b""

    def pack(self, textures, areas=None):
        (texture,) = textures.values()
        self.width, self.height, self.rgba = texture.width, texture.height, texture.rgba

    def uv(self, key, u, v):
        """ASC UV (V up) of a texture UV (V down)."""
        return u, 1 - v


class Atlas:
    """Shelf-packs textures into one square sheet (see SHEET_SIZE). Until all fit, the texture with the most
    pixels for the surface it covers on Geralt (`areas`, cm2) halves: the hook gives way before
    the jacket."""

    def __init__(self, name, size, alpha=False):
        self.name, self.alpha = name, alpha
        self.width = self.height = size
        self.rgba = b""
        self.places = {}

    def _layout(self, sizes):
        """Shelves, tallest first; GUTTER between regions (none needed at the sheet's border)."""
        places, x, y, row = {}, 0, 0, 0
        for key in sorted(sizes, key=lambda k: (-sizes[k][1], -sizes[k][0])):
            w, h = sizes[key]
            px = x + (GUTTER if x else 0)
            if px + w > self.width:
                x, y, row = 0, y + row + GUTTER, 0
                px = 0
            py = y
            if px + w > self.width or py + h > self.height:
                return None
            places[key] = (px, py, w, h)
            x, row = px + w + GUTTER, max(row, h)
        return places

    def pack(self, textures, areas):
        np = _np()
        sizes = {k: (t.width, t.height) for k, t in textures.items()}
        ratio = lambda k: sizes[k][0] * sizes[k][1] / max(areas.get(k, 1.0), 1.0)
        while (places := self._layout(sizes)) is None:
            k = max((k for k in sizes if sizes[k][0] > 8), key=ratio)
            sizes[k] = (max(8, sizes[k][0] // 2), max(8, sizes[k][1] // 2))
        # then give back what still fits, the least resolved first (a texture halved early may
        # have room again once bigger ones gave way)
        grown = True
        while grown:
            grown = False
            for k in sorted(sizes, key=ratio):
                full = (textures[k].width, textures[k].height)
                if sizes[k][0] >= full[0]:
                    continue
                old = sizes[k]
                sizes[k] = (old[0] * 2, old[1] * 2)
                trial = self._layout(sizes)
                if trial is None:
                    sizes[k] = old
                else:
                    places, grown = trial, True
                    break
        sheet = np.zeros((self.height, self.width, 4), np.uint8)
        sheet[:, :, :3] = 128
        sheet[:, :, 3] = 0 if self.alpha else 255
        for key, (x, y, w, h) in places.items():
            t = textures[key]
            img = np.frombuffer(t.rgba, np.uint8).reshape(t.height, t.width, 4)
            while img.shape[1] > w or img.shape[0] > h:  # halved by whole mip steps
                img = img[:img.shape[0] // 2 * 2, :img.shape[1] // 2 * 2].astype(np.uint16)
                img = ((img[0::2, 0::2] + img[1::2, 0::2] + img[0::2, 1::2] + img[1::2, 1::2]) // 4).astype(np.uint8)
            img = np.pad(img, ((GUTTER, GUTTER), (GUTTER, GUTTER), (0, 0)), mode="edge")
            y0, x0 = max(0, y - GUTTER), max(0, x - GUTTER)  # the gutter cut at the border
            y1, x1 = min(self.height, y + h + GUTTER), min(self.width, x + w + GUTTER)
            sheet[y0:y1, x0:x1] = img[y0 - (y - GUTTER):y1 - (y - GUTTER), x0 - (x - GUTTER):x1 - (x - GUTTER)]
        self.rgba = sheet.tobytes()
        self.places = places
        shrunk = [f"{k[0].rsplit(chr(92), 1)[-1] or k[3].rsplit(chr(92), 1)[-1]} {textures[k].width}->{w}"
                  for k, (_, _, w, _) in places.items() if w < textures[k].width]
        if os.environ.get("GERALT_ATLAS_DEBUG"):
            for k, (x, y, w, h) in sorted(places.items(), key=lambda i: i[1][1]):
                print(f"  {k[0].rsplit(chr(92), 1)[-1] or k[3]:40} {w}x{h} at {x},{y}  area {areas.get(k, 0):.0f} cm2")
        print(f"{self.name}: {len(places)} textures" + (f" (halved: {', '.join(shrunk)})" if shrunk else ""))

    def uv(self, key, u, v):
        """Sheet UV in ASC convention (V up) for a texture UV in 0..1 (V down); a triangle's UVs
        are brought into 0..1 together first (build), never vertex by vertex, which would stretch
        a triangle that crosses a whole number over the region."""
        x, y, w, h = self.places[key]
        u = min(max(u, 0.5 / w), 1 - 0.5 / w)  # half a texel inside: no neighbour filtered in
        v = min(max(v, 0.5 / h), 1 - 0.5 / h)
        return (x + u * w) / self.width, 1 - (y + v * h) / self.height


def tiled_share(sub):
    """Share of a submesh's triangles that span more than one repeat of the texture."""
    if not sub.triangles:
        return 0.0
    n = 0
    for tri in sub.triangles:
        for k in (0, 1):
            vals = [sub.uvs[i][k] for i in tri]
            if max(vals) - math.floor(min(vals)) > 1.001:
                n += 1
                break
    return n / len(sub.triangles)


# Ambient occlusion: The Witcher lights Geralt with SSAO and soft shadows, Gothic per vertex with
# no occlusion at all, so creases (under the belt, armpits, between the armour's layers, eye
# sockets) read as flat as open surfaces. Each vertex's occlusion is traced through a voxel grid
# of the whole body and painted into the textures it uses (AO_STRENGTH: how dark a fully
# occluded texel gets).
AO_VOXEL, AO_REACH, AO_RAYS, AO_STRENGTH = 1.5, 22.0, 32, 0.55


def _hemisphere(np, count):
    """Cosine-weighted directions around +Z (a fixed Fibonacci set)."""
    i = np.arange(count) + 0.5
    r = np.sqrt(i / count)
    phi = i * math.pi * (3 - math.sqrt(5))
    return np.stack([r * np.cos(phi), r * np.sin(phi), np.sqrt(1 - r * r)], 1)


def vertex_occlusion(positions, normals, triangles):
    """Visibility (1 open .. 0 closed) of each vertex over its normal's hemisphere, Gothic cm."""
    np = _np()
    pos = np.asarray(positions, np.float32)
    nrm = np.asarray(normals, np.float32)
    nrm /= np.maximum(np.linalg.norm(nrm, axis=1, keepdims=True), 1e-6)
    lo = pos.min(0) - AO_REACH
    dims = np.ceil((pos.max(0) + AO_REACH - lo) / AO_VOXEL).astype(int) + 1
    grid = np.zeros(dims, bool)
    tri = np.asarray(triangles, np.int64)
    a, b, c = pos[tri[:, 0]], pos[tri[:, 1]], pos[tri[:, 2]]
    longest = np.maximum(np.linalg.norm(b - a, axis=1), np.maximum(np.linalg.norm(c - a, axis=1), np.linalg.norm(c - b, axis=1)))
    steps = np.clip(np.ceil(longest / (AO_VOXEL * 0.5)).astype(int), 1, 64)
    for n in np.unique(steps):  # sample each triangle densely enough to leave no holes
        sel = steps == n
        u, v = np.meshgrid(np.arange(n + 1), np.arange(n + 1))
        keep = (u + v) <= n
        u, v = (u[keep] / n).astype(np.float32), (v[keep] / n).astype(np.float32)
        pts = a[sel, None] + (b[sel] - a[sel])[:, None] * u[None, :, None] + (c[sel] - a[sel])[:, None] * v[None, :, None]
        idx = ((pts.reshape(-1, 3) - lo) / AO_VOXEL).astype(int)
        grid[idx[:, 0], idx[:, 1], idx[:, 2]] = True
    # a tangent frame per vertex and the hemisphere rotated into it
    helper = np.where(np.abs(nrm[:, 2:3]) < 0.9, np.array([[0, 0, 1]], np.float32), np.array([[1, 0, 0]], np.float32))
    t = np.cross(helper, nrm)
    t /= np.maximum(np.linalg.norm(t, axis=1, keepdims=True), 1e-6)
    s = np.cross(nrm, t)
    dirs = _hemisphere(np, AO_RAYS).astype(np.float32)
    open_rays = np.zeros(len(pos), np.float32)
    marches = np.arange(1, int(AO_REACH / AO_VOXEL) + 1, dtype=np.float32) * AO_VOXEL
    start = pos + nrm * (AO_VOXEL * 1.5)
    for d in dirs:
        world = t * d[0] + s * d[1] + nrm * d[2]
        hit = np.zeros(len(pos), bool)
        for m in marches:
            idx = ((start + world * m - lo) / AO_VOXEL).astype(int)
            idx = np.clip(idx, 0, dims - 1)
            hit |= grid[idx[:, 0], idx[:, 1], idx[:, 2]]
        open_rays += ~hit
    return open_rays / AO_RAYS


def paint_occlusion(texture, corners):
    """Multiply a texture by the occlusion of the triangles that use it: corners =
    [((u, v) x3, (ao0, ao1, ao2))] with texture UVs (V down, repeats wrapped per triangle)."""
    np = _np()
    w, h = texture.width, texture.height
    total = np.zeros((h, w), np.float32)
    weight = np.zeros((h, w), np.float32)
    for uvs, ao in corners:
        us = np.array([q[0] for q in uvs], np.float64)
        vs = np.array([q[1] for q in uvs], np.float64)
        us -= math.floor(us.min())
        vs -= math.floor(vs.min())
        if us.max() > 1.001 or vs.max() > 1.001:
            continue
        xs, ys = us * w - 0.5, vs * h - 0.5
        x0, x1 = max(int(math.floor(xs.min())), 0), min(int(math.ceil(xs.max())), w - 1)
        y0, y1 = max(int(math.floor(ys.min())), 0), min(int(math.ceil(ys.max())), h - 1)
        if x1 < x0 or y1 < y0:
            continue
        gx, gy = np.meshgrid(np.arange(x0, x1 + 1), np.arange(y0, y1 + 1))
        d = (ys[1] - ys[2]) * (xs[0] - xs[2]) + (xs[2] - xs[1]) * (ys[0] - ys[2])
        if abs(d) < 1e-9:
            continue
        l0 = ((ys[1] - ys[2]) * (gx - xs[2]) + (xs[2] - xs[1]) * (gy - ys[2])) / d
        l1 = ((ys[2] - ys[0]) * (gx - xs[2]) + (xs[0] - xs[2]) * (gy - ys[2])) / d
        l2 = 1 - l0 - l1
        inside = (l0 >= -0.02) & (l1 >= -0.02) & (l2 >= -0.02)  # a little over the edges: no seams
        value = l0 * ao[0] + l1 * ao[1] + l2 * ao[2]
        total[gy[inside], gx[inside]] += value[inside]
        weight[gy[inside], gx[inside]] += 1
    covered = weight > 0
    ao_map = np.ones((h, w), np.float32)
    ao_map[covered] = total[covered] / weight[covered]
    shade = 1 - AO_STRENGTH * (1 - ao_map)
    a = np.frombuffer(texture.rgba, np.uint8).reshape(h, w, 4).astype(np.float32)
    a[:, :, :3] *= shade[:, :, None]
    return Texture(w, h, np.clip(a, 0, 255).astype(np.uint8).tobytes())


# --- building ------------------------------------------------------------------------------------
def collect_parts(w3):
    """[(mesh path, component name, colour shifts or None, item icon or None)] of Geralt's look."""
    parts = []
    for template in PARTS:
        found = w3.find(template)
        if not found:
            raise FileNotFoundError(f"{template}: not in the Witcher 3 bundles")
        colorings = entity_colorings(w3, found[0])
        default = next(iter(colorings.values()), None)
        for mesh, component in entity_meshes(w3, found[0]):
            parts.append((mesh, component, colorings.get(component, default), ICONS.get(template)))
    parts += [(m, os.path.basename(m), None, None) for m in EXTRA_MESHES]
    seen, unique = set(), []
    for p in parts:
        if p[0].lower() not in seen:
            seen.add(p[0].lower())
            unique.append(p)
    return unique


def pack_soft_skins(parts):
    """Cut the parts (verts, weights, faces, uvs, uv_faces) into soft skins of one material each
    (Gothic gives a GEOMOBJECT one material, asc.write_softskin_meshes) and at most MAX_POSITIONS
    positions and MAX_WEDGES (position, UV) pairs: small parts share a soft skin, big ones are
    split, a triangle at a time in the order the game stores them (its index buffers keep
    neighbours together). A position shared across a cut is repeated in both pieces, with the
    same skinning, so no seam shows. Returns (verts, weights, faces, uvs, uv_faces, material)."""
    pieces = []

    def new_piece(material):
        return {"verts": [], "weights": [], "uvs": [], "faces": [], "uv_faces": [],
                "pos": {}, "uv": {}, "wedges": set(), "material": material}

    for material in sorted({f[3] for _, _, faces, _, _ in parts for f in faces}):
        piece = new_piece(material)
        for k, (verts, weights, faces, uvs, uv_faces) in enumerate(parts):
            for (a, b, c, mid), (ta, tb, tc) in zip(faces, uv_faces):
                if mid != material:
                    continue
                new_pos = {i for i in (a, b, c) if (k, i) not in piece["pos"]}
                new_wedges = {(k, i, t) for i, t in ((a, ta), (b, tb), (c, tc))} - piece["wedges"]
                if (len(piece["pos"]) + len(new_pos) > MAX_POSITIONS
                        or len(piece["wedges"]) + len(new_wedges) > MAX_WEDGES):
                    pieces.append(piece)
                    piece = new_piece(material)
                tri, uvtri = [], []
                for i, t in ((a, ta), (b, tb), (c, tc)):
                    if (k, i) not in piece["pos"]:
                        piece["pos"][(k, i)] = len(piece["verts"])
                        piece["verts"].append(verts[i])
                        piece["weights"].append(weights[i])
                    if (k, t) not in piece["uv"]:
                        piece["uv"][(k, t)] = len(piece["uvs"])
                        piece["uvs"].append(uvs[t])
                    piece["wedges"].add((k, i, t))
                    tri.append(piece["pos"][(k, i)])
                    uvtri.append(piece["uv"][(k, t)])
                piece["faces"].append((tri[0], tri[1], tri[2], 0))  # MTLID: Gothic reads none
                piece["uv_faces"].append(tuple(uvtri))
        if piece["faces"]:
            pieces.append(piece)
    return [(p["verts"], p["weights"], p["faces"], p["uvs"], p["uv_faces"], p["material"]) for p in pieces]


def build(w3_dir, gothic_dir, preview=None, tile=2048, lod=0):
    w3 = W3Install(w3_dir)
    cache = TextureCache(os.path.join(w3_dir, "content", "content0", "texture.cache"))
    skeleton, _face = geralt_skeleton(w3)
    print(f"face: {len(skeleton.face)} of the face rig's bones join the skeleton")
    rest = skeleton.rest_gothic()
    textures = {}  # texture key -> Texture
    groups = {FACE: {}, ARMOR: {}, HAIR: {}, TILE: {}}  # sheet -> {key: Texture} it holds
    areas = {}  # texture key -> the surface (cm2) it covers on Geralt
    part_normals = []  # per part, per vertex: the sum of its W3 normals in Gothic space
    ao_records = {}  # texture key -> [(part, (vertex ids), (source UVs))] for paint_occlusion
    parts = []  # per W3 mesh: (verts, weights, faces, uvs, uv_faces), positions welded
    for mesh_path, component, shifts, icon in collect_parts(w3):
        mesh = Mesh(CR2W(w3.read(mesh_path), mesh_path, w3))
        bone_map = {}
        positions = mesh.file.chunks[0].props["cookedData"].get("bonePositions", [])
        for i, name in enumerate(mesh.bone_names):
            if name in skeleton.world and name not in SKIP_BONES and not name.startswith("IK_"):
                bone_map[i] = name
            else:
                p = positions[i] if i < len(positions) else {"X": 0, "Y": 0, "Z": 0}
                bone_map[i] = skeleton.nearest((p["X"], p["Y"], p["Z"]))
        materials, graphs = [], []
        for m in mesh.materials:
            f, chunk = _material_chunk(mesh, m, w3)
            materials.append(material_params(f, chunk, w3) if chunk is not None else {})
            graphs.append(_material_graph(f, chunk, w3).lower() if chunk is not None else "")
        print(f"{mesh_path} ({component}): {len(mesh.submeshes)} submeshes, {len(mesh.bone_names)} bones")
        lods = sorted({s.lod for s in mesh.submeshes})
        use_lod = max([l for l in lods if l <= lod] or lods[:1])
        pverts, pweights, pfaces, puvs, puv_faces, weld, pnormals = [], [], [], [], [], {}, []
        for sub in mesh.submeshes:
            if sub.lod != use_lod or not sub.positions:
                continue
            material = materials[sub.material] if sub.material < len(materials) else {}
            graph = graphs[sub.material] if sub.material < len(graphs) else ""
            if any(g in graph for g in SKIP_GRAPHS):
                continue
            alpha = any(g in graph for g in ALPHA_GRAPHS) or "eyelash" in str(material.get("Diffuse", "")).lower()
            tinted = "TintMask" in material or "colorshift" in graph
            key = _texture(cache, textures, material, shifts if tinted else None, tile, mesh_path,
                           icon if tinted else None)
            if alpha:
                group = HAIR
            elif tiled_share(sub) > 0.05 and (not groups[TILE] or key in groups[TILE]):
                group = TILE
            elif FACE_TEXTURE in key[0].lower() and (not groups[FACE] or key in groups[FACE]):
                group = FACE
            else:
                group = ARMOR
            groups[group][key] = textures[key]
            single = group in (FACE, TILE)
            index = []
            for p, ids, ws in zip(sub.positions, sub.bones or [()] * len(sub.positions), sub.weights or [()] * len(sub.positions)):
                g = [c * SCALE for c in vecmat(p, W3_TO_GOTHIC)]
                links = {}
                for i, w in zip(ids, ws):
                    if w > 0.001:
                        name = skeleton.gothic_name(bone_map[i])
                        links[name] = links.get(name, 0.0) + w
                if not links:
                    links = {skeleton.gothic_name(skeleton.nearest(p)): 1.0}
                total = sum(links.values())
                links = [(n, w / total) for n, w in sorted(links.items())]
                # one position per point and skinning: W3 splits vertices along UV seams, Gothic
                # keeps the UVs apart in its own wedges
                wkey = (tuple(round(c, 3) for c in g), tuple((n, round(w, 3)) for n, w in links))
                nrm = vecmat(sub.normals[len(index)], W3_TO_GOTHIC) if len(index) < len(sub.normals) else [0.0, 0.0, 0.0]
                if wkey not in weld:
                    weld[wkey] = len(pverts)
                    pverts.append(g)
                    pweights.append(links)
                    pnormals.append(list(nrm))
                else:
                    pnormals[weld[wkey]] = add(pnormals[weld[wkey]], nrm)
                index.append(weld[wkey])
            # hair cards are drawn from both sides in The Witcher 3, Gothic culls the back: the
            # back is a second set of triangles on vertices of their own a hair's breadth behind
            # (shared vertices would get front and back face normals averaged to nothing)
            back = []
            if group == HAIR:
                for vi, p in enumerate(sub.positions):
                    nrm = sub.normals[vi] if sub.normals and vi < len(sub.normals) else (0.0, 0.0, 0.0)
                    g = [c * SCALE for c in vecmat([p[k] - 0.0005 * nrm[k] for k in range(3)], W3_TO_GOTHIC)]
                    links = pweights[index[vi]]
                    wkey = (tuple(round(c, 3) for c in g), tuple((n, round(w, 3)) for n, w in links), "back")
                    if wkey not in weld:
                        weld[wkey] = len(pverts)
                        pverts.append(g)
                        pweights.append(links)
                        pnormals.append([-c for c in vecmat(nrm, W3_TO_GOTHIC)])
                    back.append(weld[wkey])
            # UVs per triangle corner, kept as (sheet, texture, u, v) until the sheets are packed;
            # in a sheet each triangle moves by whole repeats into 0..1 as one
            corner = {}

            def uv_index(u, v):
                q = (group, key, round(u, 6), round(v, 6))
                if q not in corner:
                    corner[q] = len(puvs)
                    puvs.append(q)
                return corner[q]
            # The Witcher 3 winds its triangles clockwise seen from the front (the cross product of
            # the edges points against the vertex normals in every Geralt mesh); an ASC is 3ds Max's
            # counter-clockwise, and Gothic culls the other side: unflipped, Geralt was drawn
            # inside out (the mouth and eye sockets through his face, the inside of his trousers).
            for a, c, b in sub.triangles:
                if len({index[a], index[b], index[c]}) == 3:  # welding can leave degenerate ones
                    du = 0 if single else math.floor(min(sub.uvs[i][0] for i in (a, b, c)) + 1e-6)
                    dv = 0 if single else math.floor(min(sub.uvs[i][1] for i in (a, b, c)) + 1e-6)
                    pfaces.append((index[a], index[b], index[c], group))
                    pa, pb, pc = (pverts[index[i]] for i in (a, b, c))
                    e, f = [pb[k] - pa[k] for k in range(3)], [pc[k] - pa[k] for k in range(3)]
                    n = (e[1] * f[2] - e[2] * f[1], e[2] * f[0] - e[0] * f[2], e[0] * f[1] - e[1] * f[0])
                    areas[key] = areas.get(key, 0.0) + 0.5 * math.sqrt(sum(x * x for x in n))
                    if group in (FACE, ARMOR):
                        ao_records.setdefault(key, []).append((len(parts), (index[a], index[b], index[c]),
                                                               tuple(sub.uvs[i] for i in (a, b, c))))
                    puv_faces.append(tuple(uv_index(sub.uvs[i][0] - du, sub.uvs[i][1] - dv) for i in (a, b, c)))
                    if back and len({back[a], back[b], back[c]}) == 3:
                        pfaces.append((back[a], back[c], back[b], group))
                        puv_faces.append(tuple(uv_index(sub.uvs[i][0] - du, sub.uvs[i][1] - dv) for i in (a, c, b)))
        parts.append((pverts, pweights, pfaces, puvs, puv_faces))
        part_normals.append(pnormals)
    if AO_STRENGTH > 0:
        bases, count = [], 0
        for pv, *_ in parts:
            bases.append(count)
            count += len(pv)
        visible = vertex_occlusion([v for pv, *_ in parts for v in pv], [n for pn in part_normals for n in pn],
                                   [(bases[k] + a, bases[k] + b, bases[k] + c)
                                    for k, part in enumerate(parts) for a, b, c, _ in part[2]])
        # relative to open surface (the 90th percentile): only creases darken, not the whole body
        # (rays near the tangent plane meet the surface's own curve)
        visible = _np().minimum(1.0, visible / max(float(_np().percentile(visible, 90)), 1e-3))
        for key, records in ao_records.items():
            texture = paint_occlusion(textures[key], [(uvs, tuple(visible[bases[k] + i] for i in ids)) for k, ids, uvs in records])
            textures[key] = texture
            for g in (FACE, ARMOR):
                if key in groups[g]:
                    groups[g][key] = texture
        print(f"ambient occlusion: {count} vertices, mean relative visibility {visible.mean():.2f}")
    groups[HAIR] = {k: gain_alpha(t, HAIR_ALPHA_GAIN) for k, t in groups[HAIR].items()}
    used = [g for g in (FACE, ARMOR, HAIR, TILE) if groups[g]]
    sheets = {}
    for g in used:
        sheets[g] = Single(SHEET_NAMES[g]) if g in (FACE, TILE) else Atlas(SHEET_NAMES[g], SHEET_SIZE, alpha=g == HAIR)
        sheets[g].pack(groups[g], areas)
    material_of = {g: i for i, g in enumerate(used)}
    parts = [(pv, pw, [(a, b, c, material_of[m]) for a, b, c, m in pf], [sheets[g].uv(k, u, v) for g, k, u, v in pu], puf)
             for pv, pw, pf, pu, puf in parts]
    by_index = [sheets[g] for g in used]
    assert len(by_index) <= 4, "Gothic takes at most four textures per model (FinishTexList)"
    pieces = pack_soft_skins(parts)
    verts, weights, faces, uvs, uv_faces = [], [], [], [], []
    for pv, pw, pf, pu, puf, material in pieces:  # the whole model, for the previews
        vb, tb = len(verts), len(uvs)
        verts += pv
        weights += pw
        uvs += pu
        faces += [(a + vb, b + vb, c + vb, material) for a, b, c, _ in pf]
        uv_faces += [(a + tb, b + tb, c + tb) for a, b, c in puf]
    print(f"Geralt: {sum(len(p[0]) for p in parts)} positions, {len(faces)} triangles in {len(pieces)} soft skins "
          f"(at most {max(len(p[0]) for p in pieces)} positions each), {len(textures)} textures in {len(used)} sheets, "
          f"{len(skeleton.bones)} bones")
    anims = os.path.join(gothic_dir, "_work", "DATA", "ANIMS")
    compiled_tex = os.path.join(gothic_dir, "_work", "DATA", "TEXTURES", "_COMPILED")
    os.makedirs(anims, exist_ok=True)
    os.makedirs(compiled_tex, exist_ok=True)
    write_softskin_meshes(os.path.join(anims, BODY + ".ASC"), skeleton.helper_blocks(rest),
                          [sheet.name + ".TGA" for sheet in by_index],
                          [(f"zm_Geralt_{i:02d}", pv, pf, pu, puf, pw, material)
                           for i, (pv, pw, pf, pu, puf, material) in enumerate(pieces)], separate=True)
    # Gothic keeps loading the compiled body until it is gone (and the start bat converts only then)
    stale = os.path.join(anims, "_COMPILED", BODY + ".MDM")
    if os.path.exists(stale):
        os.remove(stale)
    # this tool's textures of before (one sheet, the hair sheet, tiled ones, a longer list)
    for pattern in ("", "_HAIR", "_TILE*", "_[0-9][0-9]", "_FACE", "_ARMOR"):
        for old in glob.glob(os.path.join(compiled_tex, TEXTURE + pattern + "-C.TEX")):
            os.remove(old)
    for sheet in by_index:
        target = os.path.join(compiled_tex, sheet.name + "-C.TEX")
        if sheet.alpha:
            write_alpha_tex(target, sheet.width, sheet.height, sheet.rgba)
        else:
            write_tex(target, sheet.width, sheet.height, sheet.rgba)
    print("textures: " + ", ".join(f"{t.name} {t.width}x{t.height}{' alpha' if t.alpha else ''}" for t in by_index))
    model = Model(skeleton, rest, verts, weights, faces, uvs, uv_faces, by_index)
    if preview:
        render_preview(preview, model)
    return model


def _material_chunk(mesh, material, w3):
    if isinstance(material, str):
        f = CR2W(w3.read(material), material, w3) if material in w3 else None
        if f is None:
            return mesh.file, None
        return f, f.of_type("CMaterialInstance")[0]
    return mesh.file, material


def _material_graph(f, chunk, w3, depth=0):
    """The depot path of the material graph (.w2mg) a material instance is based on."""
    base = chunk.props.get("baseMaterial")
    if not isinstance(base, int) or base >= 0:
        return ""
    path = f.imports[-base - 1][0]
    if path.lower().endswith(".w2mi") and path in w3 and depth < 8:
        bf = CR2W(w3.read(path), path, w3)
        return _material_graph(bf, bf.of_type("CMaterialInstance")[0], w3, depth + 1)
    return path


def _texture(cache, textures, material, shifts, tile, mesh_path, icon=None):
    """The key of a material's diffuse in `textures` (made on first use): colour-shifted and
    matched to the item's icon (`icon`: (icon, main diffuse), see item_gain), its normal map
    baked in; an editor or missing texture becomes a flat colour (VarianceColor, else
    FLAT_COLORS)."""
    name, variance = material.get("Diffuse"), material.get("VarianceColor")
    key = (name or "", repr(shifts), repr(variance), "" if name else mesh_path, repr(icon))
    if key in textures:
        return key
    if name and "editor" not in name.lower() and cache.get(name):
        w, h, rgba = load_rgba(cache, name, tile)
        red_hue, gain = item_gain(cache, icon, shifts, tile) if icon else (None, None)
        if shifts:
            rgba = color_shift(rgba, shifts, red_hue)
        if gain:
            rgba = apply_gain(rgba, gain)
        normal = material.get("Normal")
        if isinstance(normal, str) and "editor" not in normal.lower() and cache.get(normal):
            rgba = bake_normals(w, h, rgba, load_rgba(cache, normal, tile))
        for map_key, tile_key in DETAIL_MAPS:
            detail, repeat = material.get(map_key), material.get(tile_key)
            if isinstance(detail, str) and isinstance(repeat, dict) and cache.get(detail) and "normal.xbm" not in detail.lower():
                try:
                    detail_rgba = load_rgba(cache, detail, 512)
                except NotImplementedError:  # BC7, not decoded (w3texture)
                    continue
                rgba = bake_detail(w, h, rgba, detail_rgba, (repeat.get("X", 1.0), repeat.get("Y", 1.0)),
                                   material.get("DetailPower", 1.0))
    else:
        color = (128, 128, 128)
        if isinstance(variance, dict):
            color = (variance.get("Red", 128), variance.get("Green", 128), variance.get("Blue", 128))
        else:
            for text, c in FLAT_COLORS:
                if text in (name or "").lower() or text in mesh_path.lower():
                    color = c
        w = h = 8
        rgba = bytes(color + (255,)) * 64
    textures[key] = Texture(w, h, bytes(rgba))
    return key


class _Image:
    def __init__(self, w, h, rgba):
        self.width, self.height, self.rgba = w, h, rgba


class Model:
    """The converted body: skeleton, rest pose and the Gothic-space mesh as written to the ASC."""

    def __init__(self, skeleton, rest, verts, weights, faces, uvs, uv_faces, sheets):
        self.skeleton, self.rest, self.verts, self.weights = skeleton, rest, verts, weights
        self.faces, self.uvs, self.uv_faces = faces, uvs, uv_faces
        self.sheets = sheets  # Sheets by material index

    def skinned(self, pose):
        """Vertices skinned with Gothic-space bone transforms ({W3 name: (rows, pos)}), the way
        Gothic skins them from the ASC (bind pose -> pose)."""
        by_name = {self.skeleton.gothic_name(n): n for n in self.rest}
        inv = {n: (transpose(r), p) for n, (r, p) in self.rest.items()}
        out = []
        for v, links in zip(self.verts, self.weights):
            p = [0.0, 0.0, 0.0]
            for name, w in links:
                bone = by_name[name]
                rt, rp = inv[bone]
                local = vecmat([v[k] - rp[k] for k in range(3)], rt)
                r, t = pose[bone]
                p = add(p, [c * w for c in add(vecmat(local, r), t)])
            out.append(p)
        return out


def render_preview(folder, model, pose=None, label="bind"):
    """Front and side PNGs of the model in the rest pose or a Gothic-space pose."""
    from png import write_png
    from render import render
    os.makedirs(folder, exist_ok=True)
    images = [_Image(s.width, s.height, s.rgba) for s in model.sheets]
    verts = model.skinned(pose) if pose else model.verts
    tris = [((verts[a], verts[b], verts[c]), [(model.uvs[i][0], 1 - model.uvs[i][1]) for i in uf], images[m])
            for (a, b, c, m), uf in zip(model.faces, model.uv_faces)]
    render(os.path.join(folder, f"geralt_{label}_front.png"), tris, up=2, right=0, depth=1, size=(400, 700), cull=True)
    render(os.path.join(folder, f"geralt_{label}_side.png"), tris, up=2, right=1, depth=0, size=(400, 700), cull=True)
    if pose is None:
        for sheet in model.sheets:
            write_png(os.path.join(folder, sheet.name.lower() + ".png"), sheet.width, sheet.height, sheet.rgba)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--w3", default=DEFAULT_W3, help="The Witcher 3 folder (with bin and content)")
    parser.add_argument("--gothic", default=DEFAULT_GOTHIC, help="the Gothic 1 folder")
    parser.add_argument("--preview", help="write PNG previews of the converted model here")
    parser.add_argument("--tile", type=int, default=2048, help="largest size of a texture (each its own material)")
    parser.add_argument("--lod", type=int, default=0, help="Witcher 3 mesh LOD to use (0 = full detail, 1 = about half)")
    args = parser.parse_args()
    build(args.w3, args.gothic, args.preview, args.tile, args.lod)


if __name__ == "__main__":
    main()
