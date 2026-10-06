"""Put the exact CJ from GTA San Andreas into Gothic 1 as the hero.

CJ keeps his own GTA skeleton, his exact mesh, skin weights and textures, and his GTA
animations. Gothic only receives them in its own source formats and compiles them the
first time the hero appears:
  _work\\DATA\\ANIMS\\CJ.MDS                 model script: CJ's animations under Gothic's movement names
  _work\\DATA\\ANIMS\\CJ_BODY.ASC            his skeleton and skinned mesh, bind pose as in GTA
  _work\\DATA\\ANIMS\\CJ_<animation>.ASC     one file per GTA animation, resampled to Gothic's 25 fps
  _work\\DATA\\TEXTURES\\_COMPILED\\CJ-C.TEX  his textures packed unscaled into one sheet (a Gothic
                                           body mesh takes a single texture)
The hero's Mdl_SetVisual / Mdl_SetVisualBody calls in Gothic.dat are pointed at CJ (original
kept as Gothic.dat.orig; the pistol items of make_cj_pistol.py are patched in again). Like a hero
transformed into an animal, CJ only has the moves in ANIMATIONS: fists and the pistol (Gothic's
crossbow mode), no other Gothic weapons, armor or object interactions.

usage: python make_cj_model.py --gta "D:\\Games\\GTA San Andreas" [--gothic DIR] [--preview DIR]
"""
import argparse
import glob
import math
import os
import re
import shutil

from asc import helper_block, write_animation, write_softskin
from daedalus import Script
from dff import Clump
from ifp import read_ifp
from img import ImgReader
from make_cj_bike import DEFAULT_BIKE, bike_anims, vehicle_info
from tex import write_tex
from txd import Texture, read_txd
from vdf import VdfReader

DEFAULT_GOTHIC = r"C:\Program Files (x86)\Steam\steamapps\common\Gothic"
MODEL, BODY, TEXTURE = "CJ", "CJ_BODY", "CJ"
SCALE = 100  # GTA metres -> Gothic centimetres
# GTA keeps a ped's centre (the animations' origin) 1 m above the ground: the ped collision model's
# box and leg line reach 1 m down (CTempColModels ms_colModelPed1). CJ stands 2 cm higher here:
# GTA's walk and run press his soles up to 3.6 cm into that ground, which shows on Gothic's floors.
PED_GROUND = 1.02

# player.img parts making up CJ's classic look, and which of his textures each one uses
PARTS = [("vest", "skin"), ("hands", "skin"), ("jeans", "legs"), ("sneaker", "feet"), ("head", "head")]
ATLAS_SIZE = (512, 512)
ATLAS = {"skin": (0, 0), "legs": (264, 0), "feet": (400, 0), "head": (0, 264)}

# Gothic animation -> CJ's GTA animation.
# (name, layer, next, blend in, blend out, flags, package, GTA animation, frames)
# frames: "loop" = all but the closing frame, "all", "first" / "last" = hold that pose.
ANIMATIONS = [
    ("s_Run", 1, "s_Run", 0.1, 0.1, "MI", "ped", "IDLE_stance", "loop"),
    ("s_RunL", 1, "s_RunL", 0.0, 0.1, "M.", "ped", "run_player", "loop"),
    ("s_Walk", 1, "s_Walk", 0.1, 0.1, "M", "ped", "IDLE_stance", "loop"),
    ("s_WalkL", 1, "s_WalkL", 0.0, 0.1, "M.", "ped", "WALK_player", "loop"),
    ("s_Sneak", 1, "s_Sneak", 0.1, 0.0, "MI", "ped", "Player_Sneak", "first"),
    ("s_SneakL", 1, "s_SneakL", 0.0, 0.1, "M.", "ped", "Player_Sneak", "loop"),
    ("t_Stand_2_Jump", 1, "s_Jump", 0.2, 0.0, "MF", "ped", "JUMP_launch", "all"),
    ("t_RunL_2_Jump", 1, "s_Jump", 0.1, 0.1, "MF", "ped", "JUMP_launch_R", "all"),
    ("t_RunR_2_Jump", 1, "s_Jump", 0.1, 0.1, "MF", "ped", "JUMP_launch_R", "all"),
    ("s_Jump", 1, "s_Jump", 0.0, 0.0, "MF", "ped", "JUMP_glide", "loop"),
    ("t_Jump_2_Stand", 1, "", 0.0, 0.5, "M.", "ped", "JUMP_land", "all"),
    ("s_Fall", 1, "s_Fall", 0.2, 0.1, "MF", "ped", "FALL_fall", "loop"),
    ("s_FallB", 1, "s_FallB", 0.1, 0.0, "MF", "ped", "FALL_fall", "loop"),
    ("s_FallDn", 1, "s_FallDn", 0.0, 0.0, "MF", "ped", "FALL_fall", "loop"),
    ("t_FallDn_2_Stand", 1, "", 0.0, 0.2, "M.", "ped", "FALL_land", "all"),
    ("s_WalkWL", 1, "s_WalkWL", 0.0, 0.0, "M.", "ped", "WALK_player", "loop"),
    ("s_Swim", 1, "s_Swim", 0.0, 0.1, "M.", "swim", "Swim_Tread", "loop"),
    ("s_SwimF", 1, "s_SwimF", 0.0, 0.1, "M.", "swim", "Swim_Breast", "loop"),
    ("s_Dive", 1, "s_Dive", 0.1, 0.1, "MF", "swim", "Swim_Under", "loop"),
    ("s_DiveF", 1, "s_DiveF", 0.1, 0.1, "MF", "swim", "Swim_Under", "loop"),
    ("t_Stumble", 1, "", 0.1, 0.1, "M.", "ped", "HIT_front", "all"),
    ("t_StumbleB", 1, "", 0.1, 0.1, "M.", "ped", "HIT_back", "all"),
    ("t_Dead", 1, "s_Dead", 0.2, 0.0, "M.", "ped", "KO_shot_front", "all"),
    ("s_Dead", 1, "s_Dead", 0.0, 0.1, "M.", "ped", "KO_shot_front", "last"),
    ("t_DeadB", 1, "s_DeadB", 0.2, 0.0, "M.", "ped", "KO_skid_back", "all"),
    ("s_DeadB", 1, "s_DeadB", 0.0, 0.1, "M.", "ped", "KO_skid_back", "last"),
] + [  # dialogue: Gothic picks among T_DIALOGGESTURE_01..21; CJ's gang-talk gestures (upper body)
    (f"T_DIALOGGESTURE_{i:02d}", 2, "", 0.2, 0.4, "M.", "gangs", f"prtial_gngtlk{'ABCDEFGH'[(i - 1) % 8]}", "all")
    for i in range(1, 22)
] + [  # fist fighting (Gothic draws fists with X): GTA's unarmed fight stance, shuffles and punches.
    # A tuple of GTA animations is played back to back in one Gothic animation.
    # Frames can also be (first, last); an optional last element lists (frame offset, tag, value)
    # events. Gothic only switches the fight mode on DEF_FIGHTMODE events, which HUMANS.MDS puts on
    # the fist draw animations: GTA's fight stance carries them here, Fight2Idle puts the fists away.
    ("s_Fist", 1, "s_Fist", 0.0, 0.0, "M.", "ped", "FIGHTIDLE", "first"),
    ("t_Fist_2_FistRun", 1, "s_FistRun", 0.0, 0.2, "M.", "ped", "FIGHTIDLE", (0, 2), [(0, "DEF_FIGHTMODE", "FIST")]),
    ("t_FistRun_2_Fist", 1, "s_Fist", 0.1, 0.0, "M.", "ped", "FIGHTIDLE", (0, 2), [(0, "DEF_FIGHTMODE", "")]),
    ("t_Fist_2_Run", 1, "", 0.0, 0.2, "M.", "ped", "Fight2Idle", "all"),
    ("s_FistRun", 1, "s_FistRun", 0.0, 0.1, "MI", "ped", "FIGHTIDLE", "loop"),
    ("s_FistWalk", 1, "s_FistWalk", 0.0, 0.1, "MI", "ped", "FIGHTIDLE", "loop"),
    ("s_FistRunL", 1, "s_FistRunL", 0.0, 0.1, "M.", "ped", "FightSh_FWD", "loop"),
    ("s_FistWalkL", 1, "s_FistWalkL", 0.0, 0.1, "M.", "ped", "FightSh_FWD", "loop"),
    ("s_FistSneak", 1, "s_FistSneak", 0.1, 0.0, "MI", "ped", "Player_Sneak", "first"),
    ("s_FistSneakL", 1, "s_FistSneakL", 0.0, 0.1, "M.", "ped", "Player_Sneak", "loop"),
    ("t_FistRunStrafeL", 1, "t_FistRunStrafeL", 0.1, 0.1, "M.", "ped", "FightSh_Left", "all"),
    ("t_FistRunStrafeR", 1, "t_FistRunStrafeR", 0.1, 0.1, "M.", "ped", "FightSh_Right", "all"),
    ("t_FistWalkStrafeL", 1, "t_FistWalkStrafeL", 0.1, 0.1, "M.", "ped", "FightSh_Left", "all"),
    ("t_FistWalkStrafeR", 1, "t_FistWalkStrafeR", 0.1, 0.1, "M.", "ped", "FightSh_Right", "all"),
    ("t_FistJumpB", 1, "s_FistRun", 0.1, 0.0, "M.", "ped", "FightSh_BWD", "all"),
    ("t_FistParadeJumpB", 1, "", 0.0, 0.0, "M.", "ped", "FightSh_BWD", "all"),
    ("t_FistParade_O", 1, "", 0.0, 0.0, "M.", "ped", "FightA_block", "all"),
    ("s_FistAttack", 1, "s_FistAttack", 0.0, 0.2, "M.", "ped", ("FightA_1", "FightA_2", "FightA_3"), "all"),
    ("t_FistAttackMove", 2, "", 0.2, 0.2, "M.", "ped", "FightA_M", "all"),
    ("t_Move_2_FistMove", 2, "", 0.2, 0.2, "M.", "ped", "FightA_block", "first", [(0, "DEF_FIGHTMODE", "FIST")]),
    ("t_FistMove_2_Move", 2, "", 0.2, 0.2, "M.", "ped", "FightA_block", "first", [(0, "DEF_FIGHTMODE", "")]),
    ("t_FistRunL_2_Jump", 1, "s_Jump", 0.1, 0.1, "MF", "ped", "JUMP_launch_R", "all"),
    ("t_FistRunR_2_Jump", 1, "s_Jump", 0.1, 0.1, "MF", "ped", "JUMP_launch_R", "all"),
] + [  # pistol, drawn as Gothic's crossbow (make_cj_pistol.py): GTA holds a pistol with CJ's normal
    # moves and switches to it without an animation; DEF_FIGHTMODE "CBOW" puts it in ZS_RIGHTHAND.
    # Strafing and stepping back while aiming use GTA's gun moves.
    ("s_CBow", 1, "s_CBow", 0.0, 0.0, "M.", "ped", "IDLE_stance", "first"),
    ("t_Run_2_CBow", 1, "s_CBow", 0.1, 0.0, "M.", "ped", "IDLE_stance", "first"),
    ("t_CBow_2_CBowRun", 1, "s_CBowRun", 0.0, 0.2, "M.", "ped", "IDLE_stance", (0, 2), [(0, "DEF_FIGHTMODE", "CBOW")]),
    ("t_CBowRun_2_CBow", 1, "s_CBow", 0.1, 0.0, "M.", "ped", "IDLE_stance", (0, 2), [(0, "DEF_FIGHTMODE", "")]),
    ("t_CBow_2_Run", 1, "", 0.0, 0.2, "M.", "ped", "IDLE_stance", "first"),
    ("t_Move_2_CBowMove", 2, "", 0.1, 0.1, "M.", "colt45", "colt45_fire", "first", [(0, "DEF_FIGHTMODE", "CBOW")]),
    ("t_CBowMove_2_Move", 2, "", 0.1, 0.1, "M.", "colt45", "colt45_fire", "first", [(0, "DEF_FIGHTMODE", "")]),
    ("s_CBowRun", 1, "s_CBowRun", 0.1, 0.1, "MI", "ped", "IDLE_stance", "loop"),
    ("s_CBowRunL", 1, "s_CBowRunL", 0.0, 0.1, "M.", "ped", "run_player", "loop"),
    ("s_CBowWalk", 1, "s_CBowWalk", 0.1, 0.1, "M", "ped", "IDLE_stance", "loop"),
    ("s_CBowWalkL", 1, "s_CBowWalkL", 0.0, 0.1, "M.", "ped", "WALK_player", "loop"),
    ("s_CBowWalkBL", 1, "s_CBowWalkBL", 0.1, 0.1, "M.", "ped", "GunMove_BWD", "loop"),
    ("s_CBowSneak", 1, "s_CBowSneak", 0.1, 0.0, "MI", "ped", "Player_Sneak", "first"),
    ("s_CBowSneakL", 1, "s_CBowSneakL", 0.0, 0.1, "M.", "ped", "Player_Sneak", "loop"),
    ("t_CBowRunStrafeL", 1, "t_CBowRunStrafeL", 0.1, 0.1, "M.", "ped", "GunMove_L", "loop"),
    ("t_CBowRunStrafeR", 1, "t_CBowRunStrafeR", 0.1, 0.1, "M.", "ped", "GunMove_R", "loop"),
    ("t_CBowWalkStrafeL", 1, "t_CBowWalkStrafeL", 0.1, 0.1, "M.", "ped", "GunMove_L", "loop"),
    ("t_CBowWalkStrafeR", 1, "t_CBowWalkStrafeR", 0.1, 0.1, "M.", "ped", "GunMove_R", "loop"),
    ("t_CBowSneakStrafeL", 1, "s_CBowSneak", 0.1, 0.1, "M.", "ped", "GunMove_L", "loop"),
    ("t_CBowSneakStrafeR", 1, "s_CBowSneak", 0.1, 0.1, "M.", "ped", "GunMove_R", "loop"),
    ("t_CBowJumpB", 1, "s_CBowRun", 0.1, 0.0, "M.", "ped", "GunMove_BWD", "all"),
    ("t_CBowRunL_2_Jump", 1, "s_Jump", 0.1, 0.1, "MF", "ped", "JUMP_launch_R", "all"),
    ("t_CBowRunR_2_Jump", 1, "s_Jump", 0.1, 0.1, "MF", "ped", "JUMP_launch_R", "all"),
] + [  # the purple dildo (make_cj_pistol.py), drawn in Gothic's one-handed mode: GTA's dildo stance,
    # block and three-swing combo (melee.dat DILDO, anim group dildo_1; DEF_HIT_LIMB ZS_RIGHTHAND as
    # Gothic's one-handers), CJ's own walk and run with it in his hand. DEF_FIGHTMODE "1H" puts it
    # in ZS_RIGHTHAND as the draw ends, "" takes it back.
    ("s_1h", 1, "s_1h", 0.0, 0.0, "M.", "dildo", "DILDO_IDLE", "first"),
    ("t_Run_2_1h", 1, "s_1h", 0.1, 0.0, "M.", "dildo", "DILDO_IDLE", "first"),
    ("t_1h_2_1hRun", 1, "s_1hRun", 0.0, 0.2, "M.", "dildo", "DILDO_IDLE", (0, 2), [(0, "DEF_FIGHTMODE", "1H")]),
    ("t_1hRun_2_1h", 1, "s_1h", 0.1, 0.0, "M.", "dildo", "DILDO_IDLE", (0, 2), [(0, "DEF_FIGHTMODE", "")]),
    ("t_1h_2_Run", 1, "", 0.0, 0.2, "M.", "ped", "IDLE_stance", "first"),
    ("t_Move_2_1hMove", 2, "", 0.1, 0.1, "M.", "dildo", "DILDO_IDLE", "first", [(0, "DEF_FIGHTMODE", "1H")]),
    ("t_1hMove_2_Move", 2, "", 0.1, 0.1, "M.", "dildo", "DILDO_IDLE", "first", [(0, "DEF_FIGHTMODE", "")]),
    ("s_1hRun", 1, "s_1hRun", 0.1, 0.1, "MI", "dildo", "DILDO_IDLE", "loop"),
    ("s_1hRunL", 1, "s_1hRunL", 0.0, 0.1, "M.", "ped", "run_player", "loop"),
    ("s_1hWalk", 1, "s_1hWalk", 0.1, 0.1, "MI", "dildo", "DILDO_IDLE", "loop"),
    ("s_1hWalkL", 1, "s_1hWalkL", 0.0, 0.1, "M.", "ped", "WALK_player", "loop"),
    ("s_1hSneak", 1, "s_1hSneak", 0.1, 0.0, "MI", "ped", "Player_Sneak", "first"),
    ("s_1hSneakL", 1, "s_1hSneakL", 0.0, 0.1, "M.", "ped", "Player_Sneak", "loop"),
    ("t_1hRunStrafeL", 1, "t_1hRunStrafeL", 0.1, 0.1, "M.", "ped", "FightSh_Left", "all"),
    ("t_1hRunStrafeR", 1, "t_1hRunStrafeR", 0.1, 0.1, "M.", "ped", "FightSh_Right", "all"),
    ("t_1hWalkStrafeL", 1, "t_1hWalkStrafeL", 0.1, 0.1, "M.", "ped", "FightSh_Left", "all"),
    ("t_1hWalkStrafeR", 1, "t_1hWalkStrafeR", 0.1, 0.1, "M.", "ped", "FightSh_Right", "all"),
    ("t_1hJumpB", 1, "s_1hRun", 0.1, 0.0, "M.", "ped", "FightSh_BWD", "all"),
    ("t_1hParadeJumpB", 1, "", 0.0, 0.0, "M.", "ped", "FightSh_BWD", "all"),
    ("t_1hParade_O", 1, "", 0.0, 0.0, "M.", "dildo", "DILDO_block", "all"),
    ("s_1hAttack", 1, "s_1hAttack", 0.0, 0.2, "M.", "dildo", ("DILDO_1", "DILDO_2", "DILDO_3"), "all"),
    ("t_1hAttackL", 1, "", 0.0, 0.2, "M.", "dildo", "DILDO_1", "all"),
    ("t_1hAttackR", 1, "", 0.0, 0.2, "M.", "dildo", "DILDO_2", "all"),
    ("t_1hAttackMove", 2, "", 0.2, 0.2, "M.", "dildo", "DILDO_1", "all"),
    ("t_1hRunL_2_Jump", 1, "s_Jump", 0.1, 0.1, "MF", "ped", "JUMP_launch_R", "all"),
    ("t_1hRunR_2_Jump", 1, "s_Jump", 0.1, 0.1, "MF", "ped", "JUMP_launch_R", "all"),
    # played by the CJMovement plugin on top of the moves: the reload, the nine arm poses of
    # t_CJ_PistolAim (see aim_poses), and GTA's gun walk while aiming (CJ keeps facing the
    # camera; their root motion moves him while Gothic's own moves stand still). Forward/back and
    # sideways are on two layers so a diagonal can play one of each at once.
    ("t_CJ_PistolReload", 16, "", 0.15, 0.25, ".", "colt45", "colt45_reload", "all"),
    ("t_CJ_GunMoveF", 13, "t_CJ_GunMoveF", 0.2, 0.2, "M.", "ped", "GunMove_FWD", "loop"),
    ("t_CJ_GunMoveB", 13, "t_CJ_GunMoveB", 0.2, 0.2, "M.", "ped", "GunMove_BWD", "loop"),
    ("t_CJ_GunMoveL", 14, "t_CJ_GunMoveL", 0.2, 0.2, "M.", "ped", "GunMove_L", "loop"),
    ("t_CJ_GunMoveR", 14, "t_CJ_GunMoveR", 0.2, 0.2, "M.", "ped", "GunMove_R", "loop"),
    # the rocket launcher (make_cj_pistol.py; weapon.dat RLAUNCHER, anim group "rocket"): GTA's shot
    # from the shoulder and the reload after it, played by the plugin on each rocket
    ("t_CJ_RocketFire", 16, "", 0.1, 0.2, ".", "rocket", "RocketFire", "all"),
] + [(f"c_CJ_PistolAim_{i + 1}", 4, "", 0.0, 0.0, "..", "cj", f"PistolAim_{i + 1}", "first") for i in range(9)]
# Gothic blends the nine poses of an aniComb by the combine X/Y the plugin sets (zCModel::
# SetCombineAniXY): pose = row * 3 + column, X picks the column and Y the row (DoCombineAni).
COMBINED = [("t_CJ_PistolAim", 15, "t_CJ_PistolAim", 0.2, 0.2, ".", "c_CJ_PistolAim_", 9)]
# Riding GTA's motorbike (make_cj_bike.py; the plugin plays these). GTA's rider animation
# blends the bike group's Ride pose with Left/Right by the steering and with Fwd/Back by the
# rider's lean (CBike::ProcessRiderAnims): here nine baked poses (bike_poses) for Gothic to blend,
# X = steering (left .. right), Y = lean (forward .. back). Still: one foot down while stopped;
# Pushes: CJ walking the bike backwards with his feet (reversing).
# Getting on and off: the group's jump-on and get-off animations with the root's path taken
# out (the plugin moves CJ along it, see CJBike.ini [PATHS]); Mount/DismountL is the left side.
BIKE_ANIMATIONS = [
    ("t_CJ_BikeMountL", 1, "s_CJ_BikeRide", 0.1, 0.0, ".", "cj", "BikeMountL", "all"),
    ("t_CJ_BikeMountR", 1, "s_CJ_BikeRide", 0.1, 0.0, ".", "cj", "BikeMountR", "all"),
    ("t_CJ_BikeDismountL", 1, "s_Run", 0.0, 0.1, ".", "cj", "BikeDismountL", "all"),
    ("t_CJ_BikeDismountR", 1, "s_Run", 0.0, 0.1, ".", "cj", "BikeDismountR", "all"),
    ("s_CJ_BikeStill", 2, "s_CJ_BikeStill", 0.3, 0.3, ".", "cj", "BikeStill", "first"),
    ("s_CJ_BikePushes", 2, "s_CJ_BikePushes", 0.2, 0.2, ".", "cj", "BikePushes", "loop"),
] + [(f"c_CJ_BikeRide_{i + 1}", 3, "", 0.0, 0.0, "..", "cj", f"BikeRide_{i + 1}", "first") for i in range(9)]
BIKE_COMBINED = [("s_CJ_BikeRide", 1, "s_CJ_BikeRide", 0.1, 0.1, ".", "c_CJ_BikeRide_", 9)]
# Aim directions of the poses relative to CJ's facing, in degrees: X = 0..1 turns right, Y = 0..1
# turns down. The plugin steers within this grid; the yaw reaches past GTA's gun walk, which turns
# CJ's body up to 50 degrees to the right.
AIM_YAW, AIM_PITCH = 60.0, 55.0
# Brows and jaw (L Brow, R Brow, Jaw) stay in the face's modelled pose: GTA drives them with
# separate facial animations (CTaskSimpleFacial, for talking or chewing), and the slightly
# different brow keys in the body animations half-close CJ's eyelids.
FACE_BONES = {6, 7, 8}
# Punch hit frames (GTA frames at 30 fps) from data/melee.dat, combo UNARMED_1: ATTACK1-3 and AMOVING.
# Gothic registers a fist hit only on frames its animation tags say.
MELEE_HITS = {"FightA_1": 5.0, "FightA_2": 8.0, "FightA_3": 10.0, "FightA_M": 7.0,
              "DILDO_1": 11.0, "DILDO_2": 15.0, "DILDO_3": 13.0}  # combo DILDO
# a weapon's swings hit with what is in the hand slot, as Gothic's own one-handers
HIT_LIMBS = {"DILDO_1": "ZS_RIGHTHAND", "DILDO_2": "ZS_RIGHTHAND", "DILDO_3": "ZS_RIGHTHAND"}
# Overlay models the CJMovement plugin switches on and off: name -> animations (as above)
# replacing CJ.MDS's while applied. CJ_SPRINT: GTA's player sprint, on Space.
# CJ_ROCKET: carrying the rocket launcher, which Gothic draws as a crossbow like the pistol: GTA's
# rocket stance, walk and run in place of the crossbow mode's moves while it is out.
OVERLAYS = {"CJ_SPRINT": [("s_RunL", 1, "s_RunL", 0.0, 0.1, "M.", "ped", "sprint_civi", "loop"),
                          ("s_CBowRunL", 1, "s_CBowRunL", 0.0, 0.1, "M.", "ped", "sprint_civi", "loop"),
                          ("s_1hRunL", 1, "s_1hRunL", 0.0, 0.1, "M.", "ped", "sprint_civi", "loop")],
            "CJ_ROCKET": [("s_CBow", 1, "s_CBow", 0.0, 0.0, "M.", "rocket", "idle_rocket", "loop"),
                          ("s_CBowRun", 1, "s_CBowRun", 0.1, 0.1, "MI", "rocket", "idle_rocket", "loop"),
                          ("s_CBowRunL", 1, "s_CBowRunL", 0.0, 0.1, "M.", "rocket", "run_rocket", "loop"),
                          ("s_CBowWalk", 1, "s_CBowWalk", 0.1, 0.1, "M", "rocket", "idle_rocket", "loop"),
                          ("s_CBowWalkL", 1, "s_CBowWalkL", 0.0, 0.1, "M.", "rocket", "walk_rocket", "loop"),
                          ("s_CBowSneak", 1, "s_CBowSneak", 0.1, 0.0, "MI", "rocket", "idle_rocket", "loop"),
                          ("s_CBowSneakL", 1, "s_CBowSneakL", 0.0, 0.1, "M.", "rocket", "walk_rocket", "loop")]}
# transitions Gothic asks for, blended straight into the next state. Gothic stops a run, walk
# or sneak with the L or R variant depending on the foot in front; both stop at once.
BLENDS = [("t_Run_2_RunL", "s_RunL"), ("t_RunL_2_Run", "s_Run"), ("t_RunR_2_Run", "s_Run"),
          ("t_Walk_2_WalkL", "s_WalkL"), ("t_WalkL_2_Walk", "s_Walk"), ("t_WalkR_2_Walk", "s_Walk"),
          ("t_SneakR_2_Sneak", "s_Sneak"), ("t_Run_2_Walk", "s_Walk"), ("t_Walk_2_Run", "s_Run"),
          ("t_RunL_2_WalkL", "s_WalkL"), ("t_WalkL_2_RunL", "s_RunL"), ("t_Run_2_Sneak", "s_Sneak"),
          ("t_Sneak_2_Run", "s_Run"), ("t_Walk_2_Sneak", "s_Sneak"), ("t_Sneak_2_Walk", "s_Walk"),
          ("t_Sneak_2_SneakL", "s_SneakL"), ("t_SneakL_2_Sneak", "s_Sneak"), ("t_Jump_2_RunL", "s_RunL"),
          ("t_Swim_2_SwimF", "s_SwimF"), ("t_SwimF_2_Swim", "s_Swim"), ("t_WalkWL_2_SwimF", "s_SwimF"),
          ("t_Dive_2_DiveF", "s_DiveF"), ("t_DiveF_2_Dive", "s_Dive"),
          ("t_Run_2_Fist", "s_Fist"), ("t_FistRun_2_FistRunL", "s_FistRunL"), ("t_FistRunL_2_FistRun", "s_FistRun"),
          ("t_FistRunR_2_FistRun", "s_FistRun"), ("t_FistWalk_2_FistWalkL", "s_FistWalkL"),
          ("t_FistWalkL_2_FistWalk", "s_FistWalk"), ("t_FistWalkR_2_FistWalk", "s_FistWalk"),
          ("t_FistRun_2_FistWalk", "s_FistWalk"), ("t_FistWalk_2_FistRun", "s_FistRun"),
          ("t_FistRunL_2_FistWalkL", "s_FistWalkL"), ("t_FistWalkL_2_FistRunL", "s_FistRunL"),
          ("t_FistRun_2_FistSneak", "s_FistSneak"), ("t_FistSneak_2_FistRun", "s_FistRun"),
          ("t_FistSneak_2_FistSneakL", "s_FistSneakL"), ("t_FistSneakL_2_FistSneak", "s_FistSneak"),
          ("t_Jump_2_FistRunL", "s_FistRunL"),
          ("t_CBowRun_2_CBowRunL", "s_CBowRunL"), ("t_CBowRunL_2_CBowRun", "s_CBowRun"),
          ("t_CBowRunR_2_CBowRun", "s_CBowRun"), ("t_CBowWalk_2_CBowWalkL", "s_CBowWalkL"),
          ("t_CBowWalkL_2_CBowWalk", "s_CBowWalk"), ("t_CBowWalkR_2_CBowWalk", "s_CBowWalk"),
          ("t_CBowRun_2_CBowWalk", "s_CBowWalk"), ("t_CBowWalk_2_CBowRun", "s_CBowRun"),
          ("t_CBowRunL_2_CBowWalkL", "s_CBowWalkL"), ("t_CBowWalkL_2_CBowRunL", "s_CBowRunL"),
          ("t_CBowWalk_2_CBowWalkBL", "s_CBowWalkBL"), ("t_CBowWalkBL_2_CBowWalk", "s_CBowWalk"),
          ("t_CBowWalkBR_2_CBowWalk", "s_CBowWalk"), ("t_CBowRun_2_CBowSneak", "s_CBowSneak"),
          ("t_CBowSneak_2_CBowRun", "s_CBowRun"), ("t_CBowWalk_2_CBowSneak", "s_CBowSneak"),
          ("t_CBowSneak_2_CBowWalk", "s_CBowWalk"), ("t_CBowSneak_2_CBowSneakL", "s_CBowSneakL"),
          ("t_CBowSneakL_2_CBowSneak", "s_CBowSneak"), ("t_CBowSneakR_2_CBowSneak", "s_CBowSneak"),
          ("t_CBowSneakL_2_CBowRunL", "s_CBowRunL"), ("t_CBowRunL_2_CBowSneakL", "s_CBowSneakL"),
          ("t_Jump_2_CBowRunL", "s_CBowRunL"), ("t_Fist_2_CBow", "s_CBow"), ("t_CBow_2_Fist", "s_Fist"),
          ("t_1hRun_2_1hRunL", "s_1hRunL"), ("t_1hRunL_2_1hRun", "s_1hRun"), ("t_1hRunR_2_1hRun", "s_1hRun"),
          ("t_1hWalk_2_1hWalkL", "s_1hWalkL"), ("t_1hWalkL_2_1hWalk", "s_1hWalk"), ("t_1hWalkR_2_1hWalk", "s_1hWalk"),
          ("t_1hRun_2_1hWalk", "s_1hWalk"), ("t_1hWalk_2_1hRun", "s_1hRun"), ("t_1hRunL_2_1hWalkL", "s_1hWalkL"),
          ("t_1hWalkL_2_1hRunL", "s_1hRunL"), ("t_1hRun_2_1hSneak", "s_1hSneak"), ("t_1hSneak_2_1hRun", "s_1hRun"),
          ("t_1hSneak_2_1hSneakL", "s_1hSneakL"), ("t_1hSneakL_2_1hSneak", "s_1hSneak"), ("t_1hSneakR_2_1hSneak", "s_1hSneak"),
          ("t_Jump_2_1hRunL", "s_1hRunL"), ("t_1h_2_CBow", "s_CBow"), ("t_CBow_2_1h", "s_1h"),
          ("t_1h_2_Fist", "s_Fist"), ("t_Fist_2_1h", "s_1h")]

# Gothic's item slots as extra nodes: slot -> (name, hand bone). GTA draws a held weapon at the
# right hand's own frame (CVisibilityPlugins::RenderWeaponPedsForPC), so the weapon meshes from
# make_cj_pistol.py sit in it unchanged. Gothic hangs a drawn crossbow (the pistol) in
# ZS_RIGHTHAND and items being used in ZS_LEFTHAND; slots for holstered weapons are left out, as
# GTA shows none. ZS_GUNFLASH carries the muzzle flash the CJMovement plugin shows on a shot.
SLOTS = {1001: ("ZS_RIGHTHAND", 24), 1002: ("ZS_LEFTHAND", 34), 1003: ("ZS_GUNFLASH", 24)}

# Gothic's own human animations that CJ has no counterpart for above (picking items up, chests,
# doors, seats, beds, ladders, crafting, instruments, being knocked out, ...): every one HUMANS.MDS
# has and CJ.MDS lacks is added, so Gothic never waits for an animation CJ cannot play (it froze
# him). Each keeps Gothic's layer, next animation, blends, flags, options and events (their frames
# scaled to the new length), and its aliases, blends and combinations come along. The motion is a
# fitting GTA animation where INTERACTIONS has one, else CJ's idle stance for as long as Gothic's.
# Sword, bow and magic animations are left out (CJ cannot draw those weapons), and so are Gothic's
# own ways of moving (sliding down slopes, turning, strafing, jumps, ledges, falls, swimming): CJ
# moves the GTA way, and with an animation for its slide Gothic would slide him down steep paths.
SKIP_GOTHIC = re.compile(r"(^|_)(1h|2h|bow|mag)|slide|turn|strafe|jump|hang|fall|swim|dive|walkw|walkb|runb|sneakb", re.I)
# (Gothic name pattern, package, GTA animation, frames as in ANIMATIONS); the first match wins
INTERACTIONS = [
    (r"^c_Stand_2_IGet_", "bomber", "BOM_Plant_In", "all"),              # pick an item up
    (r"^c_IGet_\d$", "bomber", "BOM_Plant_In", "last"),
    (r"^c_IGet_2_Stand_", "bomber", "BOM_Plant_2Idle", "all"),
    (r"^t_CHEST\w*_Stand_2_S0$", "bomber", "BOM_Plant_In", "all"),        # chests
    (r"^s_CHEST\w*_S\d$", "bomber", "BOM_Plant_In", "last"),
    (r"^t_CHEST\w*_S\d_", "bomber", "BOM_Plant_Loop", "all"),
    (r"^t_(BENCH|CHAIR|THRONE)\w*_Stand_2_S0$", "ped", "SEAT_down", "all"),  # seats
    (r"^s_(BENCH|CHAIR|THRONE)\w*_S\d$", "ped", "SEAT_idle", "loop"),
    (r"^t_(BENCH|CHAIR|THRONE)\w*_S\d_2_Stand$", "ped", "SEAT_up", "all"),
    (r"^t_(BENCH|CHAIR|THRONE)\w*_S\d_2_S\d$", "ped", "SEAT_idle", "all"),
    (r"^t_BED\w*_Stand_2_S0$", "int_house", "BED_In_L", "all"),           # beds
    (r"^s_BED\w*_S\d$", "int_house", "BED_Loop_L", "loop"),
    (r"^t_BED\w*_S\d_2_Stand$", "int_house", "BED_Out_L", "all"),
    (r"^t_BED\w*_S\d_2_S\d$", "int_house", "BED_Loop_L", "all"),
    (r"^t_Door\w*_S0_2_S1$", "ped", "DOOR_LHinge_O", "all"),              # doors
    (r"^t_Stand_2_Wounded$", "ped", "KO_shot_front", "all"),              # knocked out
    (r"^(s_Wounded|t_Wounded_Try|t_Wounded_2_Dead)$", "ped", "KO_shot_front", "last"),
    (r"^t_Wounded_2_Stand$", "ped", "getup", "all"),
    (r"^t_Stand_2_WoundedB$", "ped", "KO_skid_back", "all"),
    (r"^(s_WoundedB|t_WoundedB_Try|t_WoundedB_2_DeadB)$", "ped", "KO_skid_back", "last"),
    (r"^t_WoundedB_2_Stand$", "ped", "getup_front", "all"),
]

# GTA ped spaces (row-vector matrices): the skin's bind space faces +X; animations play in
# ped space (Z up, facing +Y); Gothic's ASC space faces -Y. BIND_TO_GOTHIC = bind -> ped -> Gothic.
BIND_TO_PED = [[0, 1, 0], [-1, 0, 0], [0, 0, 1]]
PED_TO_GOTHIC = [[-1, 0, 0], [0, -1, 0], [0, 0, 1]]


# GTA's "breast" bones are shoulder helpers: no animation drives them; every frame
# CPed::ShoulderBoneRotation (gta-reversed, Entity/Ped/Ped.cpp) sets each one to its upper
# arm's transform relative to the clavicle with half of one Euler angle. breast -> (upper arm, clavicle)
SHOULDERS = {302: (32, 31), 301: (22, 21)}


def shoulder(upper_arm):
    """CPed::ShoulderBoneRotation for one side: the helper's rotation relative to the clavicle,
    from the upper arm's (both row-vector rows = CMatrix right/forward/up). Ports
    CMatrix::ConvertToEulerAngles / ConvertFromEulerAngles for ORDER_ZYX | SWAP_XZ."""
    m = upper_arm
    i1, i2, i3 = 2, 1, 0  # ORDER_ZYX, Tait-Bryan angles
    cy = math.sqrt(m[i1][i1] ** 2 + m[i2][i1] ** 2)
    if cy > 0.0000019073486:
        x, y, z = math.atan2(m[i3][i2], m[i3][i3]), math.atan2(-m[i3][i1], cy), math.atan2(m[i2][i1], m[i1][i1])
    else:
        x, y, z = math.atan2(-m[i2][i3], m[i2][i2]), math.atan2(-m[i3][i1], cy), 0.0
    x, z = z, x  # SWAP_XZ
    x, y, z = -x, -y, -z  # the ZYX order flips signs
    x /= 2  # "Half it's X rotation"
    x, z = z, x
    x, y, z = -x, -y, -z
    sx, cx, sy, cy, sz, cz = math.sin(x), math.cos(x), math.sin(y), math.cos(y), math.sin(z), math.cos(z)
    f = [[0.0] * 3 for _ in range(3)]
    f[i1][i1], f[i1][i2], f[i1][i3] = cy * cz, -(cx * sz) + cz * sx * sy, sx * sz + cx * cz * sy
    f[i2][i1], f[i2][i2], f[i2][i3] = cy * sz, cx * cz + sx * sy * sz, -(cz * sx) + cx * sy * sz
    f[i3][i1], f[i3][i2], f[i3][i3] = -sy, cy * sx, cx * cy
    return f


# --- small math (row vectors; 3x3 rotations as row lists) --------------------------------
def vecmat(v, m): return [sum(v[k] * m[k][j] for k in range(3)) for j in range(3)]
def matmul(a, b): return [vecmat(row, b) for row in a]
def transpose(m): return [list(c) for c in zip(*m)]
BIND_TO_GOTHIC = matmul(BIND_TO_PED, PED_TO_GOTHIC)


def quat_rows(x, y, z, w):
    """RenderWare quaternion -> row-vector rotation (rows = right, up, at)."""
    return [[1 - 2 * (y * y + z * z), 2 * (x * y + w * z), 2 * (x * z - w * y)],
            [2 * (x * y - w * z), 1 - 2 * (x * x + z * z), 2 * (y * z + w * x)],
            [2 * (x * z + w * y), 2 * (y * z - w * x), 1 - 2 * (x * x + y * y)]]


def slerp(a, b, t):
    d = sum(x * y for x, y in zip(a, b))
    if d < 0:
        b, d = [-x for x in b], -d
    if d > 0.9995:
        q = [x + (y - x) * t for x, y in zip(a, b)]
    else:
        th = math.acos(d)
        q = [(math.sin((1 - t) * th) * x + math.sin(t * th) * y) / math.sin(th) for x, y in zip(a, b)]
    n = math.sqrt(sum(x * x for x in q))
    return [x / n for x in q]


def sample(keys, t):
    """Rotation quaternion and translation (or None) of an IFP track at time t."""
    if t <= keys[0][0]:
        return keys[0][1], keys[0][2]
    for (t0, q0, p0), (t1, q1, p1) in zip(keys, keys[1:]):
        if t <= t1:
            f = (t - t0) / (t1 - t0) if t1 > t0 else 0
            p = None if p0 is None else [a + (b - a) * f for a, b in zip(p0, p1)]
            return slerp(q0, q1, f), p
    return keys[-1][1], keys[-1][2]


def rows_quat(m):
    """Row-vector rotation -> RenderWare quaternion (the inverse of quat_rows)."""
    tr = m[0][0] + m[1][1] + m[2][2]
    if tr > 0:
        s = math.sqrt(tr + 1.0) * 2
        q = ((m[1][2] - m[2][1]) / s, (m[2][0] - m[0][2]) / s, (m[0][1] - m[1][0]) / s, s / 4)
    elif m[0][0] >= m[1][1] and m[0][0] >= m[2][2]:
        s = math.sqrt(1.0 + m[0][0] - m[1][1] - m[2][2]) * 2
        q = (s / 4, (m[0][1] + m[1][0]) / s, (m[0][2] + m[2][0]) / s, (m[1][2] - m[2][1]) / s)
    elif m[1][1] >= m[2][2]:
        s = math.sqrt(1.0 + m[1][1] - m[0][0] - m[2][2]) * 2
        q = ((m[0][1] + m[1][0]) / s, s / 4, (m[1][2] + m[2][1]) / s, (m[2][0] - m[0][2]) / s)
    else:
        s = math.sqrt(1.0 + m[2][2] - m[0][0] - m[1][1]) * 2
        q = ((m[0][2] + m[2][0]) / s, (m[1][2] + m[2][1]) / s, s / 4, (m[0][1] - m[1][0]) / s)
    return q


def norm(v):
    n = math.sqrt(sum(x * x for x in v))
    return [x / n for x in v]


def cross(a, b): return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]
def dot(a, b): return sum(x * y for x, y in zip(a, b))


def turn(v, u):
    """Row-vector rotation taking direction v to direction u the shortest way."""
    v, u = norm(v), norm(u)
    axis, c = cross(v, u), max(-1.0, min(1.0, dot(v, u)))
    s = math.sqrt(sum(x * x for x in axis))
    if s < 1e-9:
        return [[1, 0, 0], [0, 1, 0], [0, 0, 1]]
    x, y, z = [a / s for a in axis]
    t = 1 - c
    col = [[c + x * x * t, x * y * t - z * s, x * z * t + y * s],  # Rodrigues, column vectors
           [y * x * t + z * s, c + y * y * t, y * z * t - x * s],
           [z * x * t - y * s, z * y * t + x * s, c + z * z * t]]
    return transpose(col)


# CJ's right arm while aiming the pistol. GTA has no aiming animation for it: colt45_fire only
# kicks the hand, and an IK chain points the arm at the target every frame (CTaskSimpleUseGun::
# AimGun -> g_ikChainMan.PointArm). Baked here for the nine directions of AIM_YAW x AIM_PITCH:
# from the shoulder the arm reaches nearly straight along the aim, elbow down and out, and the
# gun (barrel = the hand's X axis, top = its Z axis, as colt45.dff sits in the hand) lies level
# along it; fingers grip as in colt45_fire. Rest of the body: IDLE_stance (unkeyed, so the
# moves underneath keep playing).
ARM = (21, 22, 23, 24)  # R clavicle, upper arm, forearm, hand
FINGERS = (25, 26)


def aim_poses(skeleton, packages):
    from ifp import Track
    base, grip = packages["ped"]["IDLE_stance"], packages["colt45"]["colt45_fire"]
    world, local = skeleton.pose_ped(base, 0.0)
    _, grip_local = skeleton.pose_ped(grip, 0.2)  # frame 6 (30 fps): where GTA fires
    clavicle, upper, fore, hand = ARM
    shoulder_rows = matmul(local[upper][0], world[clavicle][0])
    shoulder_pos = add(vecmat(local[upper][1], world[clavicle][0]), world[clavicle][1])
    a, b = math.sqrt(dot(local[fore][1], local[fore][1])), math.sqrt(dot(local[hand][1], local[hand][1]))
    poses = {}
    for row, pitch in enumerate((AIM_PITCH, 0.0, -AIM_PITCH)):
        for col, yaw in enumerate((-AIM_YAW, 0.0, AIM_YAW)):
            y, p = math.radians(yaw), math.radians(pitch)
            aim = [math.sin(y) * math.cos(p), math.cos(y) * math.cos(p), math.sin(p)]  # ped space: +Y ahead, +X right
            reach = 0.96 * (a + b)
            target = add(shoulder_pos, scale(aim, reach))
            pole = [0.35, 0.0, -1.0]
            pole = norm([pc - dot(pole, aim) * ac for pc, ac in zip(pole, aim)])
            along = (a * a - b * b + reach * reach) / (2 * reach)
            elbow = add(add(shoulder_pos, scale(aim, along)), scale(pole, math.sqrt(max(a * a - along * along, 0.0))))
            upper_rows = matmul(shoulder_rows, turn(vecmat(local[fore][1], shoulder_rows), [e - s for e, s in zip(elbow, shoulder_pos)]))
            fore_rows = matmul(local[fore][0], upper_rows)
            fore_rows = matmul(fore_rows, turn(vecmat(local[hand][1], fore_rows), [t - e for t, e in zip(target, elbow)]))
            up = norm([u - dot([0, 0, 1], aim) * ac for u, ac in zip([0, 0, 1], aim)])
            hand_rows = [aim, cross(up, aim), up]
            rotations = {clavicle: local[clavicle][0], upper: matmul(upper_rows, transpose(world[clavicle][0])),
                         fore: matmul(fore_rows, transpose(upper_rows)), hand: matmul(hand_rows, transpose(fore_rows))}
            rotations.update({f: grip_local[f][0] for f in FINGERS})
            poses[f"PistolAim_{row * 3 + col + 1}"] = [Track("", bone, [(0.0, rows_quat(r), None)]) for bone, r in rotations.items()]
    return poses


def bike_poses(package, names):
    """The rider's nine poses (BIKE_ANIMATIONS) and the get on/off animations without their
    root path, as tracks. A pose adds the full Left/Right and Fwd/Back poses to the Ride pose
    (their change from it, per bone). GTA plays these animations at the time the lean asks for
    (0 = none): full is their last frame, except Fwd, whose second half (CJ standing up over the
    bars) goes past leaning forward."""
    from ifp import Track

    def pose(name, share=1.0):
        """{bone: (quaternion, root translation or None)} at that share of the animation."""
        tracks = package[name]
        end = max(k[0] for tr in tracks for k in tr.keys)
        return {tr.bone_id: sample(tr.keys, end * share) for tr in tracks}

    ride = pose(names["Ride"])
    sides = (pose(names["Left"]), None, pose(names["Right"]))
    leans = (pose(names["Fwd"], 0.5), None, pose(names["Back"]))
    out = {}
    for row, lean in enumerate(leans):
        for col, side in enumerate(sides):
            tracks = []
            for bone, (q, p) in ride.items():
                q, p = list(q), list(p) if p else [0.0, 0.0, 0.0]
                total, moved = list(q), list(p)
                for extra in (side, lean):
                    if extra and bone in extra:
                        eq, ep = extra[bone]
                        sign = 1.0 if dot(eq, q) >= 0 else -1.0
                        total = [t + sign * e - b for t, e, b in zip(total, eq, q)]
                        if bone == 0 and ep:
                            moved = [m + e - b for m, e, b in zip(moved, ep, p)]
                n = math.sqrt(dot(total, total))
                total = [c / n for c in total]
                tracks.append(Track("", bone, [(0.0, tuple(total), tuple(moved) if bone == 0 else None)]))
            out[f"BikeRide_{row * 3 + col + 1}"] = tracks
    out["BikeStill"] = package[names["Still"]]
    out["BikePushes"] = package[names["Pushes"]]
    for role in ("MountL", "MountR", "DismountL", "DismountR"):
        out["Bike" + role] = [Track(tr.name, tr.bone_id, [(t, q, None) for t, q, _ in tr.keys]) if tr.bone_id == 0 else tr
                              for tr in package[names[role]]]
    return out


# --- GTA side ----------------------------------------------------------------------------
class Skeleton:
    """CJ's bones, parents before children.

    GTA skins the clothing parts (37-bone skin, bind pose in their inverse bind matrices)
    but animates them on player.dff's hierarchy, where e.g. the chest bones hang off the
    spine instead of the upper arms. Hierarchy and rest offsets therefore come from
    player.dff; the 5 unweighted end bones only the clothing skeleton has keep its values.
    The chest bones are re-hung under the clavicles and posed by shoulder(), as GTA does
    in code each frame, so every animation that moves an arm carries them along.
    """

    def __init__(self, clothes, player):
        geometry = clothes.geometries[0]
        local, parent_of, names = {}, {}, {}
        for clump in (clothes, player):  # player.dff wins where both have the bone
            for f in clump.frames:
                if f.bone_id is None:
                    continue
                p = clump.frames[f.parent].bone_id if f.parent >= 0 else None
                local[f.bone_id] = ([r[:3] for r in f.matrix[:3]], f.matrix[3][:3])
                parent_of[f.bone_id] = p
                names.setdefault(f.bone_id, f.name.strip())  # the clothing names (Spine 1, ...)
        skin = {b: s for s, b in enumerate(clothes.hanim_bones)}
        self.local, self.parent = local, {b: parent_of[b] for b in skin}
        for breast, (_, clavicle) in SHOULDERS.items():  # posed from the arm in code, see shoulder()
            self.parent[breast] = clavicle
        self.order = []  # parents before children
        while len(self.order) < len(skin):
            self.order += [b for b in skin if b not in self.order
                           and (self.parent[b] is None or self.parent[b] in self.order)]
        self.names = {b: self._gothic_name(names[b], b) for b in skin}
        self.skin_names = [self.names[b] for b in clothes.hanim_bones]
        # bind pose (skin space) of every bone = inverse of its inverse bind matrix
        self.bind = {}
        for b, s in skin.items():
            m = geometry.inverse_bind[s]
            rt = transpose([r[:3] for r in m[:3]])
            self.bind[b] = (rt, [-x for x in vecmat(m[3][:3], rt)])
        for slot, (name, hand) in SLOTS.items():  # at the hand's own frame, never animated
            self.local[slot] = ([[1, 0, 0], [0, 1, 0], [0, 0, 1]], [0.0, 0.0, 0.0])
            self.parent[slot], self.bind[slot], self.names[slot] = hand, self.bind[hand], name
            self.order.append(slot)

    @staticmethod
    def _gothic_name(name, bone_id):
        """Gothic-style node names, so engine lookups such as BIP01 HEAD find CJ's bones."""
        if bone_id == 0:
            return "Bip01"
        name = {"Spine 1": "Spine", "Spine 2": "Spine1", "L breast": "L Breast", "R breast": "R Breast",
                "L Finger": "L Finger0", "R Finger": "R Finger0", "L ForeArm": "L Forearm",
                "R ForeArm": "R Forearm"}.get(name, name)
        return name if name.startswith("Bip01") else "Bip01 " + name

    def rest_world(self, lift):
        """{bone: (rows, pos)} of the bind pose in Gothic space."""
        out = {}
        for b, (rows, pos) in self.bind.items():
            out[b] = (matmul(rows, BIND_TO_GOTHIC), add(scale(vecmat(pos, BIND_TO_GOTHIC), SCALE), [0, 0, lift]))
        return out

    def pose_world(self, tracks, t, lift):
        """{bone: (rows, pos)} in Gothic space at time t of a GTA animation; bones the
        animation does not move keep their rest pose relative to their parent."""
        world, _ = self.pose_ped(tracks, t)
        return {b: (matmul(r, PED_TO_GOTHIC), add(scale(vecmat(p, PED_TO_GOTHIC), SCALE), [0, 0, lift]))
                for b, (r, p) in world.items()}

    def pose_ped(self, tracks, t):
        """({bone: (rows, pos)} in GTA ped space, {bone: local (rows, pos)}) at time t."""
        keyed = {tr.bone_id: tr for tr in tracks}
        rest_root = (matmul(self.bind[0][0], BIND_TO_PED), vecmat(self.bind[0][1], BIND_TO_PED))
        world, local = {}, {}
        for b in self.order:
            rows, pos = self.local[b]
            if b in keyed:
                q, p = sample(keyed[b].keys, t)
                rows = quat_rows(*q)
                if p is not None:
                    pos = list(p)
            if b in SHOULDERS:
                upper_arm = SHOULDERS[b][0]
                if upper_arm not in local:  # breasts follow the arm, so pose the arm first
                    raise AssertionError("bone order: upper arm after breast")
                rows, pos = shoulder(local[upper_arm][0]), local[upper_arm][1]
            local[b] = (rows, pos)
            parent = self.parent[b]
            if parent is None:
                if b not in keyed:
                    rows, pos = rest_root
                world[b] = (rows, pos)
            else:
                prow, ppos = world[parent]
                world[b] = (matmul(rows, prow), add(vecmat(pos, prow), ppos))
        return world, local


def add(a, b): return [x + y for x, y in zip(a, b)]
def scale(a, s): return [x * s for x in a]


def load_gta(gta_dir):
    models = os.path.join(gta_dir, "models")
    player = ImgReader(os.path.join(models, "player.img"))

    def texture(txd, name):
        return read_txd(player.read(txd + ".txd"))[name]

    skin, vest = texture("player_torso", "torso"), texture("vest", "vest")
    body = bytearray(skin.rgba)
    for i in range(0, len(body), 4):  # the vest is drawn over CJ's skin, like GTA's clothes builder does
        a = vest.rgba[i + 3] / 255
        for c in range(3):
            body[i + c] = round(vest.rgba[i + c] * a + body[i + c] * (1 - a))
    textures = {"skin": Texture("skin", skin.width, skin.height, bytes(body)),
                "legs": texture("jeansdenim", "legs"),
                "feet": texture("sneakerbincblk", "sneakerbincblk"),
                "head": texture("player_face", "face")}  # main.scm: PLAYER_FACE on model HEAD
    parts = {name: Clump(player.read(name + ".dff")) for name, _ in PARTS}
    skeleton = Clump(ImgReader(os.path.join(models, "gta3.img")).read("player.dff"))  # CJ's animated skeleton
    anim = os.path.join(gta_dir, "anim")
    packages = {"ped": read_ifp(open(os.path.join(anim, "ped.ifp"), "rb").read())}
    anim_img = ImgReader(os.path.join(anim, "anim.img"))
    for name in ({a[6] for a in ANIMATIONS} | {i[1] for i in INTERACTIONS}) - {"ped", "cj"}:  # "cj": made here
        packages[name] = read_ifp(anim_img.read(name + ".ifp"))
    return parts, skeleton, textures, packages


def pack_atlas(textures):
    """One texture holding CJ's textures at ATLAS; gaps repeat each region's edge pixels."""
    width, height = ATLAS_SIZE
    out = bytearray(width * height * 4)
    for name, (x0, y0) in ATLAS.items():
        t = textures[name]
        for y in range(max(y0 - 4, 0), min(y0 + t.height + 4, height)):
            sy = min(max(y - y0, 0), t.height - 1)
            for x in range(max(x0 - 4, 0), min(x0 + t.width + 4, width)):
                sx = min(max(x - x0, 0), t.width - 1)
                out[(y * width + x) * 4:(y * width + x) * 4 + 4] = t.rgba[(sy * t.width + sx) * 4:(sy * t.width + sx) * 4 + 4]
    return Texture(TEXTURE, width, height, bytes(out))


# --- building -----------------------------------------------------------------------------
def build_mesh(parts, skeleton, textures):
    """Vertices (Gothic space, before lift), weights, faces, ASC UVs of all parts as one mesh."""
    verts, weights, faces, uvs, uv_faces = [], [], [], [], []
    for part, region in PARTS:
        g = parts[part].geometries[0]
        (x0, y0), t = ATLAS[region], textures[region]
        weld, index = {}, []
        for p, idx, wts in zip(g.positions, g.bone_indices, g.bone_weights):
            key = tuple(round(c, 5) for c in p)
            if key not in weld:
                weld[key] = len(verts)
                verts.append(scale(vecmat(p, BIND_TO_GOTHIC), SCALE))
                links = {}
                for i, w in zip(idx, wts):
                    if w > 0.001:
                        links[skeleton.skin_names[i]] = links.get(skeleton.skin_names[i], 0) + w
                total = sum(links.values())
                weights.append([(n, w / total) for n, w in links.items()])
            index.append(weld[key])
        base = len(uvs)
        # RenderWare V runs down, ASC V runs up
        uvs += [((x0 + u * t.width) / ATLAS_SIZE[0], 1 - (y0 + v * t.height) / ATLAS_SIZE[1]) for u, v in g.uvs]
        for a, b, c, _ in g.triangles:
            faces.append((index[a], index[b], index[c], 0))
            uv_faces.append((base + a, base + b, base + c))
    return verts, weights, faces, uvs, uv_faces


def animation_frames(tracks):
    duration = max(k[0] for tr in tracks for k in tr.keys)
    return max(1, round(duration * 25)) + 1


MDS_ENTRY = re.compile(r"\b(aniAlias|aniBlend|aniComb|aniDisable|ani)\s*\(([^)]*)\)\s*(\{[^}]*\})?", re.S)
MDS_EVENT = re.compile(r"^\s*(\*\w+)\s*\(\s*(-?\d+)(.*)\)\s*$")


def gothic_animations(gothic_dir, have, source, null_source):
    """MDS lines for the HUMANS.MDS animations CJ lacks (see INTERACTIONS). `have`: CJ's own
    animations, lower-case name -> kind ("ani", "aniComb", "aniBlend"); `source(gta_names,
    package)` writes an animation source, `null_source(frames)` one that moves nothing visible.
    Animations Gothic plays over the moves (layer above 1: looking at things, random gestures)
    get the latter where INTERACTIONS has nothing, so they never freeze CJ's body; layer 1 ones
    (states Gothic plays alone) get his idle stance. An entry is kept only if everything it names resolves:
    its next animation, an alias's target (which must be an ani: Gothic cannot alias a blend) and a
    combination's parts; Gothic's model script compiler crashes on a broken link."""
    vdf = VdfReader(os.path.join(gothic_dir, "Data", "anims.VDF"))
    key = next(k for k in vdf.files if k.upper().endswith("/HUMANS.MDS"))
    text = re.sub(r"//[^\n]*", "", vdf.read(key).decode("latin-1"))
    idle_frames = source(("IDLE_stance",), "ped")[1] - 1  # one loop
    entries = []  # (name, kind, next, [(dependency, required kind)], MDS text)
    seen = set(have)
    for kind, args, events in MDS_ENTRY.findall(text):
        a = [t.strip('"') for t in re.findall(r'"[^"]*"|\S+', args)]
        name = a[0]
        if name.lower() in seen or SKIP_GOTHIC.search(name) or kind == "aniDisable":
            continue
        seen.add(name.lower())
        if kind == "aniAlias":  # name layer next in out flags target dir
            entries.append((name, kind, a[2], [(a[6], "ani")],
                            f'\t\taniAlias\t("{name}"\t{a[1]}\t"{a[2]}"\t{a[3]}\t{a[4]}\t{a[5]}\t"{a[6]}"\t{a[7]})\n'))
            continue
        if kind == "aniBlend":  # name next [in out]
            entries.append((name, kind, a[1], [], f'\t\taniBlend\t("{name}"\t"{a[1]}"' + "".join(f"\t{v}" for v in a[2:]) + ")\n"))
            continue
        if kind == "aniComb":  # name layer next in out flags prefix count
            parts = [(f"{a[6]}{i}", "ani") for i in range(1, int(a[7]) + 1)]
            entries.append((name, kind, a[2], parts,
                            f'\t\taniComb\t("{name}"\t{a[1]}\t"{a[2]}"\t{a[3]}\t{a[4]}\t{a[5]}\t"{a[6]}"\t{a[7]})\n'))
            continue
        # ani: name layer next in out flags asc dir first last [options]
        first, last, options = int(a[8]), int(a[9]), a[10:]
        frames = abs(last - first) + 1
        match = next((i for i in INTERACTIONS if re.search(i[0], name, re.I)), None)
        if match:
            asc, n, _, _ = source((match[2],), match[1])
            span = {"loop": (0, max(n - 2, 0)), "all": (0, n - 1), "first": (0, 0), "last": (n - 1, n - 1)}[match[3]]
            options = [o for o in options if not o.upper().startswith("FPS:")]  # GTA's own timing
        elif int(a[1]) > 1:
            asc, n, _, _ = null_source(frames)
            span = (0, n - 1)
        else:
            asc, _, _, _ = source(("IDLE_stance",) * max(1, -(-frames // idle_frames)), "ped")
            span = (0, frames - 1)
        line = (f'\t\tani\t("{name}"\t{a[1]}\t"{a[2]}"\t{a[3]}\t{a[4]}\t{a[5]}\t"{asc}"\tF\t{span[0]}\t{span[1]}'
                + "".join(f"\t{o}" for o in options) + ")\n")
        tags = []
        for event in events.strip("{}").splitlines():
            m = MDS_EVENT.match(event)
            if not m:
                continue
            share = (int(m.group(2)) - first) / (last - first) if last != first else 0.0
            share = 1.0 - share if a[7].upper() == "R" else share
            frame = span[0] + round(min(max(share, 0.0), 1.0) * (span[1] - span[0]))
            tags.append(f"\t\t\t{m.group(1)}\t({frame}{m.group(3)})\n")
        entries.append((name, "ani", a[2], [], line + ("\t\t{\n" + "".join(tags) + "\t\t}\n" if tags else "")))

    kinds = dict(have)
    kinds.update({e[0].lower(): e[1] for e in entries})
    while True:  # drop entries with a broken link until none is left
        broken = {e[0].lower() for e in entries
                  if (e[2] and e[2].lower() not in kinds)
                  or any(kinds.get(d.lower()) != k for d, k in e[3])}
        if not broken:
            break
        entries = [e for e in entries if e[0].lower() not in broken]
        for name in broken:
            kinds.pop(name, None)
    # combinations, aliases and blends after the animations they name
    entries.sort(key=lambda e: {"ani": 0, "aniComb": 1, "aniAlias": 2, "aniBlend": 3}[e[1]])
    print(f"Gothic animations added for CJ: {len(entries)}")
    return [e[4] for e in entries]


def build(gta, gothic_dir, preview=None, gothic_anims=True, bike=DEFAULT_BIKE):
    parts, player_skeleton, textures, packages = load_gta(gta)
    for name, clump in parts.items():
        assert clump.hanim_bones == parts["vest"].hanim_bones, f"{name}: different skin skeleton"
    skeleton = Skeleton(parts["vest"], player_skeleton)
    packages["cj"] = aim_poses(skeleton, packages)
    group = vehicle_info(gta, bike)["group"]  # the motorbike's rider animations (make_cj_bike.py)
    packages[group] = read_ifp(ImgReader(os.path.join(gta, "anim", "anim.img")).read(group + ".ifp"))
    packages["cj"].update(bike_poses(packages[group], bike_anims(packages[group])))
    atlas = pack_atlas(textures)
    verts, weights, faces, uvs, uv_faces = build_mesh(parts, skeleton, textures)
    lift = PED_GROUND * SCALE  # GTA's ground under the ped's centre becomes Gothic's floor
    verts = [add(v, [0, 0, lift]) for v in verts]
    rest = skeleton.rest_world(lift)
    order = skeleton.order

    def helpers(animation=None, animated=()):
        return [helper_block(skeleton.names[b], skeleton.names[skeleton.parent[b]] if skeleton.parent[b] is not None else None,
                             rest[b][0], rest[b][1], animation[b] if animation and b in animated else None)
                for b in order]

    anims = os.path.join(gothic_dir, "_work", "DATA", "ANIMS")
    os.makedirs(anims, exist_ok=True)
    write_softskin(os.path.join(anims, BODY + ".ASC"), helpers(), [TEXTURE + ".TGA"],
                   verts, faces, uvs, uv_faces, weights, mesh_name="zm_CJ")

    files = {}
    root = order[0]

    def source(gta_names, package):
        """Write one animation source with the GTA animation(s) back to back (once per
        combination); returns (file name, frame count, [(GTA name, first, last frame)], poses)."""
        key = (package,) + gta_names
        if key not in files:
            poses, segments, animated = [], [], set()
            for name in gta_names:
                tracks = packages[package][name]
                n = animation_frames(tracks)
                segment = [skeleton.pose_world(tracks, f / 25, lift) for f in range(n)]
                if poses:  # carry on from where the previous animation left CJ, horizontally
                    end, start = poses[-1][root][1], segment[0][root][1]
                    shift = [end[0] - start[0], end[1] - start[1], 0.0]
                    segment = [{b: (r, add(p, shift)) for b, (r, p) in pose.items()} for pose in segment]
                segments.append((name, len(poses), len(poses) + n - 1))
                poses += segment
                animated |= {tr.bone_id for tr in tracks}
            animated &= set(order) - FACE_BONES
            animated |= {breast for breast, (arm, _) in SHOULDERS.items() if arm in animated}
            animated -= {breast for breast, (arm, _) in SHOULDERS.items() if arm not in animated}
            path = os.path.join(anims, "CJ_" + "_".join(name.upper() for name in gta_names) + ".ASC")
            write_animation(path, helpers({b: [pose[b] for pose in poses] for b in order}, animated), len(poses))
            files[key] = (os.path.basename(path), len(poses), segments, poses)
        return files[key]

    def hit_events(segments, poses):
        """Gothic's melee tags for the punches in an animation: hit frames from melee.dat, each
        punch's last frame, the combo windows between them, and the hand that lands them."""
        hits = [(start + round(MELEE_HITS[name] * 25 / 30), end) for name, start, end in segments if name in MELEE_HITS]
        if not hits:
            return []
        hands = [b for b in order if skeleton.names[b] in ("Bip01 L Hand", "Bip01 R Hand")]
        # how far each hand is in front of CJ at the hit frames (he faces -Y in Gothic space)
        reach = {b: sum(poses[h][root][1][1] - poses[h][b][1][1] for h, _ in hits) for b in hands}
        limb = skeleton.names[max(reach, key=reach.get)].upper()
        limb = next((HIT_LIMBS[name] for name, _, _ in segments if name in HIT_LIMBS), limb)
        return [(0, "DEF_HIT_LIMB", limb), (0, "DEF_OPT_FRAME", " ".join(str(h) for h, _ in hits)),
                (0, "DEF_HIT_END", " ".join(str(e) for _, e in hits)),
                (0, "DEF_WINDOW", " ".join(f"{h} {e}" for h, e in hits))]

    def ani(gname, layer, nxt, bin_, bout, flags, package, gta_name, frames, events=()):
        """Write the animation source if needed and return its MDS entry."""
        names = gta_name if isinstance(gta_name, tuple) else (gta_name,)
        asc_name, n, segments, poses = source(names, package)
        if isinstance(frames, tuple):
            first, last = frames
        else:
            first, last = {"loop": (0, max(n - 2, 0)), "all": (0, n - 1), "first": (0, 0), "last": (n - 1, n - 1)}[frames]
        line = f'\t\tani\t("{gname}"\t{layer}\t"{nxt}"\t{bin_:.1f}\t{bout:.1f}\t{flags}\t"{asc_name}"\tF\t{first}\t{last})\n'
        tags = [(first + offset, tag, value) for offset, tag, value in events] + hit_events(segments, poses)
        if not tags:
            return line
        return line + "\t\t{\n" + "".join(f'\t\t\t*eventTag\t({frame}\t"{tag}"\t"{value}")\n' for frame, tag, value in tags) + "\t\t}\n"

    mds = [ani(*entry) for entry in ANIMATIONS + BIKE_ANIMATIONS]
    mds += [f'\t\taniComb\t("{name}"\t{layer}\t"{nxt}"\t{bin_:.1f}\t{bout:.1f}\t{flags}\t"{prefix}"\t{count})\n'
            for name, layer, nxt, bin_, bout, flags, prefix, count in COMBINED + BIKE_COMBINED]
    mds += [f'\t\taniBlend\t("{name}"\t"{target}"\t0.2\t0.2)\n' for name, target in BLENDS]
    have = {e[0].lower(): "ani" for e in ANIMATIONS + BIKE_ANIMATIONS}
    have.update({c[0].lower(): "aniComb" for c in COMBINED + BIKE_COMBINED})
    have.update({b[0].lower(): "aniBlend" for b in BLENDS})
    def null_source(frames):
        """`frames` frames keying only the empty ZS_GUNFLASH slot at its rest pose."""
        from ifp import Track
        name = f"Null_{frames}"
        if name not in packages["cj"]:
            rest = (0.0, 0.0, 0.0, 1.0)
            packages["cj"][name] = [Track("", 1003, [(0.0, rest, None), (max(frames - 1, 1) / 25.0, rest, None)])]
        return source((name,), "cj")

    if gothic_anims:
        mds += gothic_animations(gothic_dir, have, source, null_source)
    with open(os.path.join(anims, MODEL + ".MDS"), "w", newline="\r\n") as f:
        f.write(f'// CJ from GTA San Andreas: his skeleton, mesh and animations (generated by make_cj_model.py)\n'
                f'Model ("{MODEL}")\n{{\n\tmeshAndTree ("{BODY}.ASC" DONT_USE_MESH)\n\tregisterMesh ("{BODY}.ASC")\n'
                f'\taniEnum\n\t{{\n{"".join(mds)}\t}}\n}}\n')
    for overlay, entries in OVERLAYS.items():
        with open(os.path.join(anims, overlay + ".MDS"), "w", newline="\r\n") as f:
            f.write(f'// Overlay for {MODEL}.MDS, applied by the CJMovement plugin (generated by make_cj_model.py)\n'
                    f'Model ("{overlay}")\n{{\n\tmeshAndTree ("{BODY}.ASC" DONT_USE_MESH)\n'
                    f'\taniEnum\n\t{{\n{"".join(ani(*entry) for entry in entries)}\t}}\n}}\n')

    compiled_tex = os.path.join(gothic_dir, "_work", "DATA", "TEXTURES", "_COMPILED")
    os.makedirs(compiled_tex, exist_ok=True)
    write_tex(os.path.join(compiled_tex, TEXTURE + "-C.TEX"), atlas.width, atlas.height, atlas.rgba)

    # Remove what Gothic compiled from earlier sources (and the earlier fitted-body attempt).
    compiled = os.path.join(anims, "_COMPILED")
    stale = glob.glob(os.path.join(compiled, MODEL + ".*")) + glob.glob(os.path.join(compiled, MODEL + "-*.MAN"))
    stale += glob.glob(os.path.join(compiled, BODY + ".*")) + glob.glob(os.path.join(compiled, "HUM_*_CJ.*"))
    stale += glob.glob(os.path.join(anims, "HUM_*_CJ.*"))
    stale += [os.path.join(compiled_tex, n) for n in ("CJ_BODY-C.TEX", "CJ_HEAD-C.TEX")]
    for overlay in OVERLAYS:
        stale += glob.glob(os.path.join(compiled, overlay + ".*")) + glob.glob(os.path.join(compiled, overlay + "-*.MAN"))
    for path in stale:
        if os.path.exists(path):
            os.remove(path)

    print(f"CJ: {len(order)} bones, {len(verts)} vertices, {len(faces)} faces, "
          f"{len(files)} GTA animations -> {len(ANIMATIONS)} Gothic animations + {len(BLENDS)} blends")
    if preview:
        render_preview(preview, skeleton, packages, verts, weights, faces, uvs, uv_faces, atlas, rest, lift)


def render_preview(folder, skeleton, packages, verts, weights, faces, uvs, uv_faces, atlas, rest, lift):
    """Skin the Gothic-space mesh with Gothic-space bone transforms exactly as written to the
    ASC files, for the bind pose and a few animation frames."""
    from render import render
    os.makedirs(folder, exist_ok=True)
    by_name = {skeleton.names[b]: b for b in rest}
    inv_rest = {b: (transpose(r), p) for b, (r, p) in rest.items()}
    poses = [("bind", rest)] + [(f"{n}_{t:.2f}", skeleton.pose_world(packages[pkg][n], t, lift))
                                for pkg, n, t in (("ped", "run_player", 0.2), ("ped", "WALK_player", 0.5),
                                                  ("ped", "IDLE_stance", 0.0), ("gangs", "prtial_gngtlkA", 1.0))]
    for label, pose in poses:
        out = []
        for v, links in zip(verts, weights):
            p = [0.0, 0.0, 0.0]
            for name, w in links:
                b = by_name[name]
                rt, rp = inv_rest[b]
                local = vecmat([v[k] - rp[k] for k in range(3)], rt)
                r, t = pose[b]
                p = add(p, scale(add(vecmat(local, r), t), w))
            out.append(p)
        tris = [((out[a], out[b], out[c]), [(uvs[i][0], 1 - uvs[i][1]) for i in uf], atlas)
                for (a, b, c, _), uf in zip(faces, uv_faces)]
        render(os.path.join(folder, f"cj_{label}_front.png"), tris, up=2, right=0, depth=1, size=(300, 520))
        render(os.path.join(folder, f"cj_{label}_side.png"), tris, up=2, right=1, depth=0, size=(300, 520))


def patch_hero(gothic_dir):
    path = os.path.join(gothic_dir, "_work", "DATA", "scripts", "_compiled", "Gothic.dat")
    backup = path + ".orig"
    if not os.path.exists(backup):
        shutil.copy2(path, backup)
    script = Script(open(backup, "rb").read())
    model = script.string_args_of_call("PC_HERO", "MDL_SETVISUAL")[0][-1]
    (body, head), = script.string_args_of_call("PC_HERO", "MDL_SETVISUALBODY")
    for index, value in ((model, MODEL + ".MDS"), (body, BODY), (head, "")):
        assert script.references(index) == 1, "string literal is shared with other code"
        script.symbols[index].content = [value.encode()]
    from make_cj_pistol import patch_items  # rebuilt from the original too, so keep the pistol
    patch_items(script)
    with open(path, "wb") as f:
        f.write(script.to_bytes())
    print(f"Gothic.dat: hero is now {MODEL}.MDS / {BODY} (original saved as {os.path.basename(backup)})")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--gta", required=True, help="GTA San Andreas (classic PC) install folder")
    parser.add_argument("--gothic", default=DEFAULT_GOTHIC, help="Gothic 1 install folder")
    parser.add_argument("--preview", help="also render PNG previews (bind pose and animation frames) into this folder")
    parser.add_argument("--no-gothic-anims", action="store_true", help="leave out the HUMANS.MDS stand-ins (gothic_animations)")
    parser.add_argument("--bike", default=DEFAULT_BIKE, help="the motorbike of make_cj_bike.py, for its rider animations")
    args = parser.parse_args()
    build(args.gta, args.gothic, args.preview, gothic_anims=not args.no_gothic_anims, bike=args.bike)
    patch_hero(args.gothic)


if __name__ == "__main__":
    main()
