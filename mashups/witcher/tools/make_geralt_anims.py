"""Geralt's own Witcher 3 animations for Gothic 1, and Geralt made the hero.

Writes into the Gothic folder:
  _work\\DATA\\ANIMS\\GERALT.MDS               model script: Geralt's animations under Gothic's names
  _work\\DATA\\ANIMS\\GERALT_<animation>.ASC   one file per animation, resampled to Gothic's 25 fps
  _work\\DATA\\ANIMS\\GERALT_SPRINT.MDS        overlay the Witcher plugin applies while sprinting
and points the hero's Mdl_SetVisual / Mdl_SetVisualBody calls in Gothic.dat at GERALT (the
original kept as Gothic.dat.orig, always patched from that copy). make_geralt_model.py writes
the body (GERALT_BODY.ASC) and runs first.

What plays what:
- Gothic's moves (stand, walk, run, jumps, falls, swimming, stumbles, death) are Geralt's
  exploration and movement animations.
- Gothic's one-handed fight mode is Geralt's sword: he draws it from his back (the sword reaches
  his hand at the draw animation's OnWeaponDrawReady event, which becomes DEF_FIGHTMODE "1H"),
  stands in his sword guard, walks and runs with it, steps aside, dodges back and parries with
  his own animations. s_1hAttack is a string of his fast swings, alternating left and right
  stance as the game's CombatStanceLeft/Right events order them; each swing hits at the end of
  its PreAttack event, and the next one may follow from its ActionBlend event (the game's combo
  window), cross-faded over CHAIN_BLEND seconds.
- Animations the Witcher plugin plays itself (t_W3_...): strong attacks, dodges, rolls, guard,
  counter, the five signs, switching swords.
- Every other animation HUMANS.MDS has (items, chests, doors, seats, beds, ...) is added with a
  stand-in (gothic_animations), so Gothic never waits for one Geralt lacks.

usage: python make_geralt_anims.py --w3 "<Witcher 3 folder>" [--gothic DIR] [--no-gothic-anims]
       [--no-hero]
"""
import argparse
import glob
import math
import os
import re
import shutil
import sys

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
from asc import write_animation
from cr2w import CR2W
from daedalus import Script
from gothic import humans_mds, root_motion
from w3anim import AnimationSet
from w3bundle import W3Install
from w3mimic import compose
import make_geralt_model as body

MODEL = "GERALT"
FPS = 25            # Gothic's animation rate
CHAIN_BLEND = 0.2   # seconds of cross-fade where one Witcher 3 animation runs into the next
SETS = {
    "move": "animations/man/movement/man_geralt_movement.w2anims",
    "ex": "animations/man/movement/man_geralt_exploration.w2anims",
    "swim": "animations/man/movement/man_geralt_swimming.w2anims",
    "sword": "animations/man/combat/man_geralt_sword.w2anims",
    "sloco": "animations/man/combat/man_geralt_sword_locomotion.w2anims",
    "signs": "animations/man/combat/man_geralt_signs.w2anims",
    "dialog": "animations/man/dialogs/man_dialog_standing.w2anims",
}
# Gothic's gestures while someone speaks (oCNpc::StartDialogAni, 0x6B2130, plays
# T_DIALOGGESTURE_<rand() % 20 + 1>, layer 2 as HUMANS.MDS has them): Geralt's own from his
# scenes' standing stance (high_standing_determined), the short ones without props or steps.
DIALOG_GESTURES = [f"high_standing_determined_gesture_{g}" for g in (
    "both_hands_1", "both_hands_2", "both_hands_3", "both_hands_flick_1", "both_hands_options", "decline",
    "do_it", "exclamation_01", "exclamation_02", "me", "me_and_you", "you", "question", "question_02",
    "question_04", "question_hand_flick_01", "shrug", "sure", "right_hand_2", "tale")]

# (Gothic name, layer, next, blend in, blend out, flags, set, W3 animation(s), frames[, events])
# W3 animation: a name, "name@start-end" (seconds), or a tuple of those played back to back.
# frames: "loop" (all but the closing frame), "all", "first", "last" or (first, last).
# events: [(frame offset from the first frame, tag, value)].
UNSHEATHE, SHEATHE = "man_geralt_sword_unsheathe_steel", "man_geralt_sword_sheathe_steel"
SWORD_IDLE = "man_geralt_sword_alert_idle_right"
ANIMATIONS = [
    ("s_Run", 1, "s_Run", 0.1, 0.1, "MI", "move", "locomotion_idle", "loop"),
    ("s_Walk", 1, "s_Walk", 0.1, 0.1, "MI", "move", "locomotion_idle", "loop"),
    ("s_Sneak", 1, "s_Sneak", 0.1, 0.0, "MI", "move", "locomotion_idle", "first"),
    ("s_SneakL", 1, "s_SneakL", 0.0, 0.1, "M.", "move", "locomotion_walk_cycle_slow_forward", "loop"),
    ("t_Stand_2_Jump", 1, "s_Jump", 0.2, 0.0, "MF", "ex", "ex_jump_idle_takeoff", "all"),
    ("s_Jump", 1, "s_Jump", 0.0, 0.0, "MF", "ex", "jump_falling_loop", "loop"),
    ("t_Jump_2_Stand", 1, "", 0.0, 0.5, "M.", "ex", "ex_jump_idle_land_stop", "all"),
    ("s_Fall", 1, "s_Fall", 0.2, 0.1, "MF", "ex", "ex_fall_loop", "loop"),
    ("s_FallB", 1, "s_FallB", 0.1, 0.0, "MF", "ex", "jump_falling_loop_b", "loop"),
    ("s_FallDn", 1, "s_FallDn", 0.0, 0.0, "MF", "ex", "ex_fall_loop", "loop"),
    ("t_FallDn_2_Stand", 1, "", 0.0, 0.2, "M.", "ex", "ex_jump_idle_land_stop", "all"),
    ("s_WalkWL", 1, "s_WalkWL", 0.0, 0.0, "M.", "move", "ex_walk_deep_water_f", "loop"),
    ("s_Swim", 1, "s_Swim", 0.0, 0.1, "M.", "swim", "swim_idle", "loop"),
    ("s_SwimF", 1, "s_SwimF", 0.0, 0.1, "M.", "swim", "swim_slow_f", "loop"),
    ("s_Dive", 1, "s_Dive", 0.1, 0.1, "MF", "swim", "swim_underwater_idle", "loop"),
    ("s_DiveF", 1, "s_DiveF", 0.1, 0.1, "MF", "swim", "swim_underwater_f", "loop"),
    ("t_Stumble", 1, "", 0.1, 0.1, "M.", "sword", "man_geralt_sword_hit_front_rp_01", "all"),
    ("t_StumbleB", 1, "", 0.1, 0.1, "M.", "sword", "man_ger_sword_hit_back_1", "all"),
    ("t_Dead", 1, "s_Dead", 0.2, 0.0, "M.", "sword", "man_ger_sword_death_1", "all"),
    ("s_Dead", 1, "s_Dead", 0.0, 0.1, "M.", "sword", "man_ger_sword_death_1", "last"),
    ("t_DeadB", 1, "s_DeadB", 0.2, 0.0, "M.", "sword", "man_ger_sword_death_1", "all"),
    ("s_DeadB", 1, "s_DeadB", 0.0, 0.1, "M.", "sword", "man_ger_sword_death_1", "last"),
] + [  # Geralt's sword as Gothic's one-handed mode
    ("s_1h", 1, "s_1h", 0.0, 0.0, "M.", "sword", SWORD_IDLE, "first"),
    ("t_Run_2_1h", 1, "s_1h", 0.1, 0.0, "M.", "sword", f"{UNSHEATHE}@0-0.73", "all"),
    ("t_1h_2_1hRun", 1, "s_1hRun", 0.0, 0.2, "M.", "sword", f"{UNSHEATHE}@0.73-1.5", "all",
     [(0, "DEF_FIGHTMODE", "1H")]),
    ("t_1hRun_2_1h", 1, "s_1h", 0.1, 0.0, "M.", "sword", f"{SHEATHE}@0-1.12", "all", [(-1, "DEF_FIGHTMODE", "")]),
    ("t_1h_2_Run", 1, "", 0.0, 0.2, "M.", "sword", f"{SHEATHE}@1.12-2.1", "all"),
    ("t_Move_2_1hMove", 2, "", 0.1, 0.1, "M.", "sword", UNSHEATHE, "all", [(18, "DEF_FIGHTMODE", "1H")]),
    ("t_1hMove_2_Move", 2, "", 0.1, 0.1, "M.", "sword", SHEATHE, "all", [(28, "DEF_FIGHTMODE", "")]),
    ("s_1hRun", 1, "s_1hRun", 0.1, 0.1, "MI", "sword", SWORD_IDLE, "loop"),
    ("s_1hRunL", 1, "s_1hRunL", 0.0, 0.1, "M.", "sloco", "combat_locomotion_run_cycle_fast_forward", "loop"),
    ("s_1hWalk", 1, "s_1hWalk", 0.1, 0.1, "MI", "sword", SWORD_IDLE, "loop"),
    ("s_1hWalkL", 1, "s_1hWalkL", 0.0, 0.1, "M.", "sloco", "combat_locomotion_walk_cycle_fast_forward", "loop"),
    ("s_1hSneak", 1, "s_1hSneak", 0.1, 0.0, "MI", "sword", SWORD_IDLE, "first"),
    ("s_1hSneakL", 1, "s_1hSneakL", 0.0, 0.1, "M.", "sloco", "combat_locomotion_walk_cycle_slow_forward", "loop"),
    # Gothic's side steps in fight mode are Geralt's dodges to the side (the Witcher plugin presses
    # Gothic's strafe keys for them, so Gothic moves him with collision as for its own steps)
    ("t_1hRunStrafeL", 1, "", 0.05, 0.2, "M.", "sword", "man_geralt_sword_dodge_left_350m", "all"),
    ("t_1hRunStrafeR", 1, "", 0.05, 0.2, "M.", "sword", "man_geralt_sword_dodge_right_350m", "all"),
    ("t_1hWalkStrafeL", 1, "", 0.05, 0.2, "M.", "sword", "man_geralt_sword_dodge_left_350m", "all"),
    ("t_1hWalkStrafeR", 1, "", 0.05, 0.2, "M.", "sword", "man_geralt_sword_dodge_right_350m", "all"),
    ("t_1hJumpB", 1, "s_1hRun", 0.05, 0.0, "M.", "sword", "man_geralt_sword_dodge_back_350m", "all"),
    ("t_1hParadeJumpB", 1, "", 0.0, 0.0, "M.", "sword", "man_geralt_sword_dodge_back_337cm_rp_01", "all"),
    ("t_1hParade_O", 1, "", 0.0, 0.0, "M.", "sword", "man_geralt_sword_parry_f_rightup_rp", "all"),
    ("s_1hAttack", 1, "s_1hAttack", 0.0, 0.2, "M.", "sword", "@fast_string", "all"),
    ("t_1hAttackL", 1, "", 0.0, 0.2, "M.", "sword", "man_geralt_sword_attack_fast_left_1_rp_40ms", "all"),
    ("t_1hAttackR", 1, "", 0.0, 0.2, "M.", "sword", "man_geralt_sword_attack_fast_right_1_rp_40ms", "all"),
    ("t_1hRunL_2_Jump", 1, "s_Jump", 0.1, 0.1, "MF", "ex", "jump_run_lf_sword", "all"),
    ("t_1hRunR_2_Jump", 1, "s_Jump", 0.1, 0.1, "MF", "ex", "jump_run_rf_sword", "all"),
] + [  # played by the Witcher plugin (layer 1: they take over the body; root motion moves him)

    ("t_W3_DodgeF", 1, "", 0.05, 0.2, "M.", "sword", "man_geralt_sword_dodge_forward_350m", "all"),
    ("t_W3_DodgeB", 1, "", 0.05, 0.2, "M.", "sword", "man_geralt_sword_dodge_back_350m", "all"),
    ("t_W3_DodgeL", 1, "", 0.05, 0.2, "M.", "sword", "man_geralt_sword_dodge_left_350m", "all"),
    ("t_W3_DodgeR", 1, "", 0.05, 0.2, "M.", "sword", "man_geralt_sword_dodge_right_350m", "all"),
    ("t_W3_RollF", 1, "", 0.05, 0.2, "M.", "sword", "man_geralt_sword_dodge_roll_rp_f_01", "all"),
    ("t_W3_RollB", 1, "", 0.05, 0.2, "M.", "sword", "man_geralt_sword_dodge_roll_rp_b_01", "all"),
    ("t_W3_GuardStart", 1, "s_W3_Guard", 0.1, 0.0, "M.", "sword", "man_geralt_sword_parry_start_rp", "all"),
    ("s_W3_Guard", 1, "s_W3_Guard", 0.0, 0.1, "M.", "sword", "man_geralt_sword_parry_idle_rp", "loop"),
    ("t_W3_GuardStop", 1, "", 0.0, 0.2, "M.", "sword", "man_geralt_sword_parry_stop_rp", "all"),
    ("t_W3_Counter", 1, "", 0.05, 0.2, "M.", "sword", "man_geralt_sword_repel_rp_kick", "all"),
    ("t_W3_Aard", 1, "", 0.1, 0.2, "M.", "signs", "man_geralt_aard_front_sword", "all"),
    ("t_W3_Igni", 1, "", 0.1, 0.2, "M.", "signs", "man_geralt_ignii_front_sword", "all"),
    ("t_W3_Quen", 1, "", 0.1, 0.2, "M.", "signs", "man_geralt_quen_front_sword", "all"),
    ("t_W3_Yrden", 1, "", 0.1, 0.2, "M.", "signs", "man_geralt_yrden_ground_sword", "all"),
    ("t_W3_Axii", 1, "", 0.1, 0.2, "M.", "signs", "man_geralt_axii_release", "all"),
    ("t_W3_AardFree", 1, "", 0.1, 0.2, "M.", "signs", "man_geralt_aard_front", "all"),
    ("t_W3_IgniFree", 1, "", 0.1, 0.2, "M.", "signs", "man_geralt_ignii_front", "all"),
    ("t_W3_QuenFree", 1, "", 0.1, 0.2, "M.", "signs", "man_geralt_quen_front", "all"),
    ("t_W3_YrdenFree", 1, "", 0.1, 0.2, "M.", "signs", "man_geralt_yrden_ground", "all"),
    ("t_W3_SwitchToSilver", 1, "", 0.1, 0.2, "M.", "sword", "man_geralt_sword_switch_steel_to_silver", "all"),
    ("t_W3_SwitchToSteel", 1, "", 0.1, 0.2, "M.", "sword", "man_geralt_sword_switch_silver_to_steel", "all"),
] + [(f"T_DIALOGGESTURE_{i:02d}", 2, "", 0.2, 0.4, "M.", "dialog", g, "all") for i, g in enumerate(DIALOG_GESTURES, 1)]
# Geralt's single swings, which the Witcher plugin picks click by click as the game's combat.ws
# does (distance class, direction to the target, stance), each played until the game lets it
# blend away (its AllowBlend event). Names: t_W3_<family><n><stance R/L>.
def _swing(gname, w3):
    return (gname, 1, "", 0.05, 0.25, "M.", "sword", w3 + "@allowblend", "all")


SWINGS = [_swing(f"t_W3_F{i}{s}", f"man_geralt_sword_attack_fast_{i}_{st}_40ms")
          for i in (1, 2, 3, 4) for s, st in (("R", "rp"), ("L", "lp"))]
SWINGS += [_swing(f"t_W3_FX{i}{s}", f"man_geralt_sword_attack_fast_{i}_{st}_40ms_extra_dist_remaster")
           for i in (1, 2) for s, st in (("R", "rp"), ("L", "lp"))]
SWINGS += [_swing(f"t_W3_{f}{s}", f"man_geralt_sword_attack_{w}_{st}_{ms}")
           for f, w, ms in (("FF", "fast_far_forward_1", "50ms"), ("FFX", "fast_far_forward_1", "50ms_extra_dist_remaster"),
                            ("FL", "fast_left_1", "40ms"), ("FR", "fast_right_1", "40ms"), ("FB", "fast_back_1", "40ms"),
                            ("SF", "strong_far_forward_1", "80ms"), ("SL", "strong_left_1", "70ms"),
                            ("SR", "strong_right_1", "70ms"), ("SB", "strong_back_1", "70ms"))
           for s, st in (("R", "rp"), ("L", "lp"))]
SWINGS += [_swing(f"t_W3_S{i}{s}", f"man_geralt_sword_attack_strong_{i}_{st}_70ms")
           for i in (1, 2, 3) for s, st in (("R", "rp"), ("L", "lp"))]
ANIMATIONS += SWINGS

# Fast and strong swing strings: (right-stance list, left-stance list); each swing's
# CombatStanceLeft/Right event says which list the next one comes from.
STRINGS = {
    "fast_string": ([f"man_geralt_sword_attack_fast_{i}_rp_40ms" for i in (1, 2, 3, 4)],
                    [f"man_geralt_sword_attack_fast_{i}_lp_40ms" for i in (1, 2, 3, 4)]),
    "strong_string": ([f"man_geralt_sword_attack_strong_{i}_rp_70ms" for i in (1, 2, 3)],
                      [f"man_geralt_sword_attack_strong_{i}_lp_70ms" for i in (1, 2, 3)]),
}
STRING_HITS = 4  # swings in s_1hAttack (Gothic chains as many as comboMax allows)
# the sprint overlay (the plugin applies it while the sprint key is held); its run loop, start and
# stops come from LOCOMOTION
SPRINT_OVERLAY = f"{MODEL}_SPRINT"
OVERLAYS = {SPRINT_OVERLAY: [
    ("s_1hRunL", 1, "s_1hRunL", 0.0, 0.1, "M.", "sloco", "combat_locomotion_sprint_cycle_forward", "loop")]}

# Moving as in the game: Geralt starts, stops and turns with his own movement animations
# (man_geralt_movement), not with Gothic's cross-fades.
# - Starts: Gothic's t_Run_2_RunL / t_Walk_2_WalkL (and the sprint overlay's) are the game's
#   idle_to_forward, and each loop begins at the phase where its start ends (the closest legs), so
#   the start runs on into the loop.
# - Stops: Gothic ends a loop with t_<m>L_2_<m> while the loop is within 15 % of its start or end,
#   with t_<m>R_2_<m> within 15 % of its middle, and runs on in between
#   (oCAniCtrl_Human::GetFootPos, 0x628A50, called from _Stand). Each of the game's two stops (left
#   or right foot forward) goes where the loop's legs are closest to its first frame; the running
#   jumps likewise. A stop is cut once his slide is 95 % done plus STOP_SETTLE seconds (at least
#   STOP_MIN), and blends into the stand.
# - Turns the Witcher plugin plays (t_W3_...): 180-degree turns while running, walking or sprinting
#   and starts to the side or back. Their turn is taken out of the animation (Builder.turn_frames)
#   and written to Witcher_turns.txt in the Gothic folder; the plugin turns Geralt by it, frame by
#   frame, so Gothic's heading follows the body.
STOP_SETTLE, STOP_MIN = 0.3, 0.6
# mode: (Gothic loop, start, stop near the loop's start, stop near its middle, stand,
#        W3 loop, W3 start, W3 stops (left foot forward, right foot forward))
LOCOMOTION = {
    "run": ("s_RunL", "t_Run_2_RunL", "t_RunL_2_Run", "t_RunR_2_Run", "s_Run",
            "locomotion_run_cycle_fast_forward", "locomotion_run_idle_to_forward",
            ("locomotion_run_leftforward_to_idle", "locomotion_run_rightforward_to_idle")),
    "walk": ("s_WalkL", "t_Walk_2_WalkL", "t_WalkL_2_Walk", "t_WalkR_2_Walk", "s_Walk",
             "locomotion_walk_cycle_fast_forward", "locomotion_walk_idle_to_forward",
             ("locomotion_walk_leftforward_to_idle", "locomotion_walk_rightforward_to_idle")),
    "sprint": ("s_RunL", "t_Run_2_RunL", "t_RunL_2_Run", "t_RunR_2_Run", "s_Run",
               "locomotion_sprint_cycle_forward", "locomotion_sprint_idle_to_forward",
               ("locomotion_sprint_leftforward_to_idle", "locomotion_sprint_rightforward_to_idle")),
}
RUN_JUMPS = (("t_RunL_2_Jump", "t_RunR_2_Jump"), ("jump_run_lf", "jump_run_rf"))  # set "ex"
# (Gothic name of the left turn, of the right one, what follows, the game's pair): which of the
# pair turns left is read from its yaw (the game's locomotion_run_turn180_left turns right)
TURNS = [
    ("t_W3_RunTurnL", "t_W3_RunTurnR", "s_RunL", ("locomotion_run_turn180_left", "locomotion_run_turn180_right")),
    ("t_W3_WalkTurnL", "t_W3_WalkTurnR", "s_WalkL", ("locomotion_walking_turn_l180_lf", "locomotion_walking_turn_r180_rf")),
    ("t_W3_SprintTurnL", "t_W3_SprintTurnR", "s_RunL",
     ("locomotion_sprint_leftforward_turn_left_180", "locomotion_sprint_rightforward_turn_right_180")),
    ("t_W3_RunStartL", "t_W3_RunStartR", "s_RunL", ("locomotion_run_idle_to_left", "locomotion_run_idle_to_right")),
    ("t_W3_RunStartBL", "t_W3_RunStartBR", "s_RunL", ("locomotion_run_idle_to_back_ccw", "locomotion_run_idle_to_back_cw")),
    ("t_W3_WalkStartL", "t_W3_WalkStartR", "s_WalkL", ("locomotion_walk_idle_to_left", "locomotion_walk_idle_to_right")),
    ("t_W3_WalkStartBL", "t_W3_WalkStartBR", "s_WalkL",
     ("locomotion_walk_idle_to_back_ccw", "locomotion_walk_idle_to_back_cw")),
]
TURNS_FILE = "Witcher_turns.txt"
LEGS = ("l_thigh", "l_shin", "l_foot", "r_thigh", "r_shin", "r_foot")

BLENDS = [("t_SneakR_2_Sneak", "s_Sneak"), ("t_Run_2_Walk", "s_Walk"), ("t_Walk_2_Run", "s_Run"),
          ("t_RunL_2_WalkL", "s_WalkL"), ("t_WalkL_2_RunL", "s_RunL"), ("t_Run_2_Sneak", "s_Sneak"),
          ("t_Sneak_2_Run", "s_Run"), ("t_Walk_2_Sneak", "s_Sneak"), ("t_Sneak_2_Walk", "s_Walk"),
          ("t_Sneak_2_SneakL", "s_SneakL"), ("t_SneakL_2_Sneak", "s_Sneak"), ("t_Jump_2_RunL", "s_RunL"),
          ("t_Swim_2_SwimF", "s_SwimF"), ("t_SwimF_2_Swim", "s_Swim"), ("t_WalkWL_2_SwimF", "s_SwimF"),
          ("t_Dive_2_DiveF", "s_DiveF"), ("t_DiveF_2_Dive", "s_Dive"),
          ("t_1hRun_2_1hRunL", "s_1hRunL"), ("t_1hRunL_2_1hRun", "s_1hRun"), ("t_1hRunR_2_1hRun", "s_1hRun"),
          ("t_1hWalk_2_1hWalkL", "s_1hWalkL"), ("t_1hWalkL_2_1hWalk", "s_1hWalk"), ("t_1hWalkR_2_1hWalk", "s_1hWalk"),
          ("t_1hRun_2_1hWalk", "s_1hWalk"), ("t_1hWalk_2_1hRun", "s_1hRun"), ("t_1hRunL_2_1hWalkL", "s_1hWalkL"),
          ("t_1hWalkL_2_1hRunL", "s_1hRunL"), ("t_1hRun_2_1hSneak", "s_1hSneak"), ("t_1hSneak_2_1hRun", "s_1hRun"),
          ("t_1hSneak_2_1hSneakL", "s_1hSneakL"), ("t_1hSneakL_2_1hSneak", "s_1hSneak"),
          ("t_1hSneakR_2_1hSneak", "s_1hSneak"), ("t_Jump_2_1hRunL", "s_1hRunL")]

# Gothic's HUMANS.MDS animations Geralt has no counterpart for get a stand-in (gothic_animations):
# two-handed, bow, crossbow and magic modes are left out (Geralt fights with his sword and signs),
# and so are Gothic's own ways of moving that the Witcher plugin replaces.
SKIP_GOTHIC = re.compile(r"(^|_)(2h|bow|cbow|mag)|slide|turn|strafe|jump|hang|fall|swim|dive|walkw|walkb|runb|sneakb", re.I)
# The ladder's animations, whose stand-ins move like Gothic's hero (gothic_animations): its climbs are
# fly animations, and the way on and off keeps him where they start and end.
LADDER = re.compile(r"(^|_)Ladder_", re.I)


# --- maths ---------------------------------------------------------------------------------------
def rows_quat(m):
    """Quaternion (x, y, z, w) of a row-vector rotation (inverse of body.quat_rows)."""
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


def blend_pose(a, b, w):
    """Cross-fade of two Gothic-space poses ({name: (rows, pos)}), w = 0 -> a, 1 -> b."""
    out = {}
    for name, (rb, pb) in b.items():
        ra, pa = a[name]
        qa, qb = rows_quat(ra), rows_quat(rb)
        if sum(x * y for x, y in zip(qa, qb)) < 0:
            qb = tuple(-c for c in qb)
        q = [x + (y - x) * w for x, y in zip(qa, qb)]
        n = math.sqrt(sum(c * c for c in q)) or 1.0
        out[name] = (body.quat_rows(*(c / n for c in q)), [x + (y - x) * w for x, y in zip(pa, pb)])
    return out


def leg_distance(a, b):
    """How differently two Gothic-space poses hold the legs (squared cm, relative to the pelvis)."""
    ra, rb = a[body.ROOT][1], b[body.ROOT][1]
    return sum(((pa - qa) - (pb - qb)) ** 2
               for name in LEGS for pa, pb, qa, qb in zip(a[name][1], b[name][1], ra, rb))


# --- building ------------------------------------------------------------------------------------
class Builder:
    def __init__(self, w3, skeleton, rest, anims_dir):
        self.w3, self.skeleton, self.rest, self.dir = w3, skeleton, rest, anims_dir
        self.sets, self.files, self.cache = {}, {}, {}
        self.names = set()  # ASC file names written (one per source)
        self.turns = {}     # (set, "name@turn") -> turn per frame (turn_frames)
        self.turn_of = {}   # Gothic animation -> its turn per frame, for TURNS_FILE

    def animation(self, set_name, name):
        key = (set_name, name)
        if key not in self.cache:
            if set_name not in self.sets:
                path = SETS[set_name]
                self.sets[set_name] = AnimationSet(CR2W(self.w3.read(path), path, self.w3))
            self.cache[key] = self.sets[set_name].get(name)
        return self.cache[key]

    def frames(self, anim, start=0.0, end=None):
        """Gothic-space poses at 25 fps from start to end (seconds) of a W3 animation."""
        end = anim.duration if end is None else min(end, anim.duration)
        n = max(1, round((end - start) * FPS)) + 1
        out = []
        for f in range(n):
            t = min(start + f / FPS, end)
            world = self.skeleton.pose_world(anim.pose(t), anim.root_motion(t))
            out.append(self.skeleton.to_gothic(world))
        return out

    def cycle_frames(self, anim, offset):
        """A loop's poses (25 fps) from `offset` seconds round to it again, the root carried on
        over the wrap by one stride, placed where the loop's own first frame stands."""
        d = anim.duration
        n = max(1, round(d * FPS)) + 1
        stride = [b - a for a, b in zip(anim.root_motion(0.0), anim.root_motion(d))]
        out = []
        for f in range(n):
            t = offset + f * d / (n - 1)
            if t > d:
                t -= d
                motion = tuple(m + s for m, s in zip(anim.root_motion(t), stride))
            else:
                motion = anim.root_motion(t)
            out.append(self.skeleton.to_gothic(self.skeleton.pose_world(anim.pose(t), motion)))
        sx, sy = out[0][body.ROOT][1][:2]
        ex, ey = self.frames(anim, 0.0, 0.0)[0][body.ROOT][1][:2]
        return [{k: (r, [p[0] + ex - sx, p[1] + ey - sy, p[2]]) for k, (r, p) in pose.items()} for pose in out]

    def turn_frames(self, anim):
        """(poses, turn) of an animation that turns Geralt by its root motion's yaw, with the turn
        taken out: he keeps facing as at its first frame and his path is turned back to match, so
        once Gothic turns him by `turn` (degrees from the first frame at each frame, left
        positive) his path and body are the game's."""
        d = anim.duration
        n = max(1, round(d * FPS)) + 1
        times = [min(f / FPS, d) for f in range(n)]
        motion = [anim.root_motion(t) for t in times]
        yaw0 = motion[0][3]
        x, y = motion[0][0], motion[0][1]
        poses, turn = [], []
        for i, (t, m) in enumerate(zip(times, motion)):
            if i:
                a = motion[i - 1]
                theta = math.radians((a[3] + m[3]) / 2 - yaw0)
                dx, dy = m[0] - a[0], m[1] - a[1]
                c, s = math.cos(theta), math.sin(theta)
                x, y = x + dx * c + dy * s, y - dx * s + dy * c
            turn.append(m[3] - yaw0)
            poses.append(self.skeleton.to_gothic(self.skeleton.pose_world(anim.pose(t), (x, y, m[2], yaw0))))
        return poses, turn

    def best_phase(self, cycle, pose):
        """The time (s) in a loop where its legs are closest to `pose`'s."""
        frames = self.frames(cycle)
        k = min(range(max(1, len(frames) - 1)), key=lambda i: leg_distance(frames[i], pose))
        return k / FPS

    def pair_by_phase(self, loop, set_name, pair):
        """Of two animations, (the one whose first frame is closest to the loop's start, the
        one closest to its middle), going by the legs."""
        a, b = (self.frames(self.animation(set_name, name), 0.0, 0.0)[0] for name in pair)
        mid = loop[(len(loop) - 2) // 2]
        if leg_distance(a, loop[0]) + leg_distance(b, mid) <= leg_distance(b, loop[0]) + leg_distance(a, mid):
            return pair
        return pair[1], pair[0]

    def stop_end(self, set_name, name):
        """Where a stop is cut: 95 % of its slide done, plus STOP_SETTLE (at least STOP_MIN)."""
        anim = self.animation(set_name, name)
        m0 = anim.root_motion(0.0)

        def travel(t):
            m = anim.root_motion(t)
            return math.hypot(m[0] - m0[0], m[1] - m0[1])
        total = travel(anim.duration)
        t95 = 0.0
        if total > 0.05:
            t95 = next((f / FPS for f in range(int(anim.duration * FPS) + 1) if travel(f / FPS) >= 0.95 * total),
                       anim.duration)
        return min(anim.duration, max(STOP_MIN, t95 + STOP_SETTLE))

    def string(self, set_name, string_name, hits):
        """Segments [(W3 name, start, end)] of a swing string, each cut where the game lets the
        next swing begin (ActionBlend), the last one played out until it may blend away."""
        right, left = STRINGS[string_name]
        segments, side, used = [], "right", {"right": 0, "left": 0}
        for i in range(hits):
            pool = right if side == "right" else left
            name = pool[used[side] % len(pool)]
            used[side] += 1
            anim = self.animation(set_name, name)
            blend = anim.event("ActionBlend")
            last = i == hits - 1
            if last:
                allow = anim.event("AllowBlend")
                end = allow.start if allow else anim.duration
            else:
                end = blend.start if blend else anim.duration
            segments.append((name, 0.0, end))
            side = "left" if anim.event("CombatStanceLeft") else ("right" if anim.event("CombatStanceRight") else
                                                                  ("left" if side == "right" else "right"))
        return segments

    def special_source(self, gothic_name, set_name, spec):
        """A loop begun at another phase ("name@cycle=<s>") or a turn ("name@turn", its turn kept
        in self.turns): (file name, frames, segments) as source."""
        key = (set_name, spec)
        if key not in self.files:
            name, _, mode = spec.partition("@")
            anim = self.animation(set_name, name)
            if mode == "turn":
                poses, self.turns[key] = self.turn_frames(anim)
            else:
                poses = self.cycle_frames(anim, float(mode.split("=", 1)[1]))
            file_name = self._file_name(gothic_name)
            write_animation(os.path.join(self.dir, file_name), self.skeleton.helper_blocks(self.rest, poses), len(poses))
            self.files[key] = (file_name, len(poses), [(name, 0, len(poses) - 1, 0.0)])
        return self.files[key]

    def source(self, gothic_name, set_name, spec):
        """Write the animation source for a W3 spec; returns (file name, frames, segments) where
        segments are [(W3 name, first frame, last frame, start s)] in the file."""
        if isinstance(spec, str) and ("@cycle=" in spec or spec.endswith("@turn")):
            return self.special_source(gothic_name, set_name, spec)
        if isinstance(spec, str) and spec.startswith("@"):
            parts = self.string(set_name, spec[1:], STRING_HITS if spec == "@fast_string" else 3)
        else:
            parts = []
            for item in (spec if isinstance(spec, tuple) else (spec,)):
                name, _, span = item.partition("@")
                if span == "allowblend":  # until the game lets it blend away
                    allow = self.animation(set_name, name).event("AllowBlend")
                    parts.append((name, 0.0, allow.start if allow else None))
                    continue
                start, _, end = span.partition("-")
                parts.append((name, float(start) if start else 0.0, float(end) if end else None))
        key = (set_name, tuple(parts))
        if key in self.files:
            return self.files[key]
        poses, segments = [], []
        for name, start, end in parts:
            anim = self.animation(set_name, name)
            seg = self.frames(anim, start, end)
            if poses:  # carry on from where the previous one left Geralt, horizontally, cross-faded
                ex, ey = poses[-1][body.ROOT][1][:2]
                sx, sy = seg[0][body.ROOT][1][:2]
                seg = [{n: (r, [p[0] + ex - sx, p[1] + ey - sy, p[2]]) for n, (r, p) in pose.items()} for pose in seg]
                k = max(1, round(CHAIN_BLEND * FPS))
                last = poses[-1]
                seg = [blend_pose(last, pose, (i + 1) / (k + 1)) if i < k else pose for i, pose in enumerate(seg)]
            segments.append((name, len(poses), len(poses) + len(seg) - 1, start))
            poses += seg
        file_name = self._file_name(gothic_name)
        write_animation(os.path.join(self.dir, file_name), self.skeleton.helper_blocks(self.rest, poses), len(poses))
        self.files[key] = (file_name, len(poses), segments)
        return self.files[key]

    def _file_name(self, gothic_name):
        """GERALT_<first Gothic animation using it>.ASC, numbered when an overlay's animation of
        the same name has other motion."""
        base = f"{MODEL}_{gothic_name.upper()}"
        name, i = base + ".ASC", 2
        while name.lower() in self.names:
            name, i = f"{base}_{i}.ASC", i + 1
        self.names.add(name.lower())
        return name

    def hit_events(self, set_name, segments):
        """Gothic's melee tags for the swings in an animation: each hit at the end of the swing's
        PreAttack event, the window from the hit to where the next swing begins."""
        hits = []
        for name, first, last, start in segments:
            pre = self.animation(set_name, name).event("PreAttack")
            if pre is not None:
                hits.append((first + round((pre.end - start) * FPS), last))
        if not hits:
            return []
        return [(0, "DEF_HIT_LIMB", "ZS_RIGHTHAND"), (0, "DEF_OPT_FRAME", " ".join(str(h) for h, _ in hits)),
                (0, "DEF_HIT_END", " ".join(str(e) for _, e in hits)),
                (0, "DEF_WINDOW", " ".join(f"{h} {e}" for h, e in hits))]

    def ani(self, gname, layer, nxt, bin_, bout, flags, set_name, spec, frames, events=()):
        asc_name, n, segments = self.source(gname, set_name, spec)
        if isinstance(frames, tuple):
            first, last = frames
        else:
            first, last = {"loop": (0, max(n - 2, 0)), "all": (0, n - 1), "first": (0, 0), "last": (n - 1, n - 1)}[frames]
        if (set_name, spec) in self.turns:
            self.turn_of[gname] = self.turns[(set_name, spec)][first:last + 1]
        line = f'\t\tani\t("{gname}"\t{layer}\t"{nxt}"\t{bin_:.2f}\t{bout:.2f}\t{flags}\t"{asc_name}"\tF\t{first}\t{last})\n'
        tags = [((last if offset < 0 else first + offset), tag, value) for offset, tag, value in events]
        if set_name in ("sword",) and gname in ("s_1hAttack", "t_1hAttackL", "t_1hAttackR"):
            tags += self.hit_events(set_name, segments)
        if not tags:
            return line
        return line + "\t\t{\n" + "".join(f'\t\t\t*eventTag\t({f}\t"{t}"\t"{v}")\n' for f, t, v in tags) + "\t\t}\n"

    def rooted_source(self, gothic_name, set_name, name, first, offsets):
        """The W3 idle `name` held for first + len(offsets) frames, his whole skeleton moved in
        each by Gothic's hero's root motion in `gothic_name` (gothic.root_motion; frames before
        `first`, which Gothic doesn't play, by its first offset): (file name, frames, segments)."""
        key = ("rooted", gothic_name)
        if key not in self.files:
            loop = self.frames(self.animation(set_name, name))
            period = max(1, len(loop) - 1)
            moves = [offsets[0]] * first + list(offsets)
            poses = [{n: (r, [p[0] + d[0], p[1] + d[1], p[2] + d[2]]) for n, (r, p) in loop[f % period].items()}
                     for f, d in enumerate(moves)]
            file_name = self._file_name(gothic_name)
            write_animation(os.path.join(self.dir, file_name), self.skeleton.helper_blocks(self.rest, poses), len(poses))
            self.files[key] = (file_name, len(poses), [])
        return self.files[key]

    def null_source(self, frames):
        """An animation of `frames` frames that keys only the empty ZS_NULL slot (layered stand-ins)."""
        key = ("null", frames)
        if key not in self.files:
            file_name = self._file_name(f"NULL_{frames}")
            rest = self.rest
            blocks = []
            for name in [b.name for b in self.skeleton.bones] + list(body.SLOTS):
                parent = body.SLOTS.get(name) or self.skeleton.parent[name]
                rows, pos = rest[name]
                anim = [rest[name]] * frames if name == "ZS_NULL" else None
                blocks.append(body.helper_block(self.skeleton.gothic_name(name),
                                                self.skeleton.gothic_name(parent) if parent else None, rows, pos, anim))
            write_animation(os.path.join(self.dir, file_name), blocks, frames)
            self.files[key] = (file_name, frames, [])
        return self.files[key]


def locomotion(builder):
    """(entries for GERALT.MDS, entries for the sprint overlay) of the game's loops, starts,
    stops, running jumps and turns (LOCOMOTION, RUN_JUMPS, TURNS)."""
    main, sprint = [], []
    for mode, (loop_g, start_g, stop0_g, stop50_g, stand_g, loop_w3, start_w3, stops_w3) in LOCOMOTION.items():
        cycle = builder.animation("move", loop_w3)
        offset = builder.best_phase(cycle, builder.frames(builder.animation("move", start_w3))[-1])
        loop = builder.cycle_frames(cycle, offset)
        stop0, stop50 = builder.pair_by_phase(loop, "move", stops_w3)
        entries = [
            (loop_g, 1, loop_g, 0.0, 0.1, "M.", "move", f"{loop_w3}@cycle={offset:.2f}", "loop"),
            (start_g, 1, loop_g, 0.1, 0.05, "M.", "move", start_w3, "all"),
            (stop0_g, 1, stand_g, 0.05, 0.3, "M.", "move", f"{stop0}@0-{builder.stop_end('move', stop0):.2f}", "all"),
            (stop50_g, 1, stand_g, 0.05, 0.3, "M.", "move", f"{stop50}@0-{builder.stop_end('move', stop50):.2f}", "all"),
        ]
        line = f"{mode}: loop from {offset:.2f} s, {stop0_g} = {stop0}, {stop50_g} = {stop50}"
        if mode == "run":
            (jump0_g, jump50_g), pair = RUN_JUMPS
            jump0, jump50 = builder.pair_by_phase(loop, "ex", pair)
            entries += [(jump0_g, 1, "s_Jump", 0.1, 0.1, "MF", "ex", jump0, "all"),
                        (jump50_g, 1, "s_Jump", 0.1, 0.1, "MF", "ex", jump50, "all")]
            line += f", {jump0_g} = {jump0}, {jump50_g} = {jump50}"
        print(line)
        (sprint if mode == "sprint" else main).extend(entries)
    for left_g, right_g, nxt, pair in TURNS:
        yaw = {}
        for name in pair:
            anim = builder.animation("move", name)
            yaw[name] = anim.root_motion(anim.duration)[3] - anim.root_motion(0.0)[3]
        left, right = sorted(pair, key=lambda name: -yaw[name])
        main += [(left_g, 1, nxt, 0.1, 0.1, "M.", "move", f"{left}@turn", "all"),
                 (right_g, 1, nxt, 0.1, 0.1, "M.", "move", f"{right}@turn", "all")]
        print(f"turns: {left_g} = {left} ({yaw[left]:+.0f}), {right_g} = {right} ({yaw[right]:+.0f})")
    return main, sprint


def face_animations(builder, face):
    """MDS lines of make_geralt_model.FACE_ANIS: each keys only the face bones its poses move,
    where The Witcher's face rig (w3mimic) puts them, so it plays over whatever the body does."""
    sk = builder.skeleton
    head = sk.world[body.HEAD_NODE]
    lines = []
    for name, layer, nxt, blend_in, blend_out, per_key, keys in body.FACE_ANIS:
        poses, animated = [], set()
        for weights in body.face_keys(keys, per_key):
            relative = face.relative(weights)
            animated |= face.moved(weights) & sk.face
            world = dict(sk.world)
            for bone in sk.face:
                world[bone] = compose(relative[bone], head)
            poses.append(sk.to_gothic(world))
        file_name = builder._file_name(name)
        write_animation(os.path.join(builder.dir, file_name), sk.helper_blocks(builder.rest, poses, animated), len(poses))
        last = len(poses) - 2 if nxt == name and len(poses) > 2 else len(poses) - 1  # a loop's closing frame is its first
        lines.append(f'\t\tani\t("{name}"\t{layer}\t"{nxt}"\t{blend_in:.2f}\t{blend_out:.2f}\t.\t"{file_name}"\tF\t0\t{last})\n')
        print(f"face: {name}, {len(poses)} frames, {len(animated)} bones")
    return lines


MDS_START = re.compile(r"^\s*(aniAlias|aniBlend|aniComb|aniDisable|ani)\s*\((.*)$", re.I)
MDS_EVENT = re.compile(r"^\s*(\*\w+)\s*\(\s*(-?\d+)(.*)\)\s*$")


def mds_entries(text):
    """[(kind, args, events)] of a model script, a statement per line: Gothic's HUMANS.MDS has
    an ani line without its closing parenthesis (c_Ore_S1_3, before the aniComb s_Ore_S1), so
    a statement's arguments end at ')' or at the end of its line, and an event block belongs
    to the statement right before it."""
    out = []
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        m = MDS_START.match(lines[i])
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


def gothic_animations(gothic_dir, have, builder):
    """MDS lines for the HUMANS.MDS animations Geralt lacks: Geralt's idle (his sword guard for
    one-handed ones) for layer-1 ones, a null animation for layered ones. An entry is kept only
    if everything it names resolves (Gothic's model script compiler crashes on a broken link).
    The same method as the GTA mashup's make_cj_model.py, where it kept CJ from freezing."""
    text = re.sub(r"//[^\n]*", "", humans_mds(gothic_dir))  # Gothic 2: from its compiled HUMANS.MSB
    idle_frames = {"move": builder.source("idle_standin", "move", "locomotion_idle")[1] - 1,
                   "sword": builder.source("1h_standin", "sword", SWORD_IDLE)[1] - 1}

    def idle(frames, set_name, name):
        """The idle looped for at least `frames` frames, so the stand-in keeps Gothic's length
        and every event frame of the original stays inside it."""
        loops = max(1, -(-frames // idle_frames[set_name]))
        return builder.source(f"{set_name}_standin_{loops}", set_name, (name,) * loops)
    entries, seen = [], set(have)
    for kind, args, events in mds_entries(text):
        a = [t.strip('"') for t in re.findall(r'"[^"]*"|\S+', args)]
        name = a[0]
        if name.lower() in seen or SKIP_GOTHIC.search(name) or kind == "aniDisable":
            continue
        seen.add(name.lower())
        if kind == "aniAlias":
            entries.append((name, kind, a[2], [(a[6], "ani")],
                            f'\t\taniAlias\t("{name}"\t{a[1]}\t"{a[2]}"\t{a[3]}\t{a[4]}\t{a[5]}\t"{a[6]}"\t{a[7]})\n'))
            continue
        if kind == "aniBlend":
            entries.append((name, kind, a[1], [], f'\t\taniBlend\t("{name}"\t"{a[1]}"' + "".join(f"\t{v}" for v in a[2:]) + ")\n"))
            continue
        if kind == "aniComb":
            parts = [(f"{a[6]}{i}", "ani") for i in range(1, int(a[7]) + 1)]
            entries.append((name, kind, a[2], parts,
                            f'\t\taniComb\t("{name}"\t{a[1]}\t"{a[2]}"\t{a[3]}\t{a[4]}\t{a[5]}\t"{a[6]}"\t{a[7]})\n'))
            continue
        # the stand-in is at least as long as the original's last frame, so the original's frame
        # numbers stay valid, its events (single frames and the lists in DEF_OPT_FRAME,
        # DEF_HIT_END and DEF_WINDOW alike) included
        first, last, options = int(a[8]), int(a[9]), a[10:]
        frames = max(first, last) + 1
        if int(a[1]) > 1:
            asc, n, _ = builder.null_source(frames)
        elif (("F" in a[5].upper() or LADDER.search(name)) and a[7].upper() == "F"
              and len(offsets := root_motion(gothic_dir, name) or []) == abs(last - first) + 1):
            # fly animations (the ladders' climbs, a drowned body) move him by their root motion
            # alone, up and down too, so they get Gothic's hero's (the GTA mashup's make_cj_model.py)
            asc, n, _ = builder.rooted_source(name, "move", "locomotion_idle", min(first, last), offsets)
        elif re.search(r"1h", name, re.I):
            asc, n, _ = idle(frames, "sword", SWORD_IDLE)
        else:
            asc, n, _ = idle(frames, "move", "locomotion_idle")
        line = (f'\t\tani\t("{name}"\t{a[1]}\t"{a[2]}"\t{a[3]}\t{a[4]}\t{a[5]}\t"{asc}"\t{a[7]}\t{first}\t{last}'
                + "".join(f"\t{o}" for o in options) + ")\n")
        tags = []
        for event in events.strip("{}").splitlines():
            m = MDS_EVENT.match(event)
            if m:
                tags.append(f"\t\t\t{m.group(1)}\t({m.group(2)}{m.group(3)})\n")
        entries.append((name, "ani", a[2], [], line + ("\t\t{\n" + "".join(tags) + "\t\t}\n" if tags else "")))
    kinds = dict(have)
    kinds.update({e[0].lower(): e[1] for e in entries})
    while True:
        broken = {e[0].lower() for e in entries
                  if (e[2] and e[2].lower() not in kinds) or any(kinds.get(d.lower()) != k for d, k in e[3])}
        if not broken:
            break
        entries = [e for e in entries if e[0].lower() not in broken]
        for name in broken:
            kinds.pop(name, None)
    entries.sort(key=lambda e: {"ani": 0, "aniComb": 1, "aniAlias": 2, "aniBlend": 3}[e[1]])
    print(f"Gothic animations added for Geralt: {len(entries)}")
    return [e[4] for e in entries]


def build(w3_dir, gothic_dir, gothic_anims=True, model=None):
    w3 = W3Install(w3_dir)
    skeleton, face = body.geralt_skeleton(w3)
    rest = skeleton.rest_gothic()
    if model is not None:
        skeleton, rest = model.skeleton, model.rest
    anims = os.path.join(gothic_dir, "_work", "DATA", "ANIMS")
    os.makedirs(anims, exist_ok=True)
    builder = Builder(w3, skeleton, rest, anims)
    moves, sprint_moves = locomotion(builder)
    overlays = {name: (sprint_moves if name == SPRINT_OVERLAY else []) + entries for name, entries in OVERLAYS.items()}
    mds = [builder.ani(*entry) for entry in ANIMATIONS + moves] + face_animations(builder, face)
    mds += [f'\t\taniBlend\t("{name}"\t"{target}"\t0.2\t0.2)\n' for name, target in BLENDS]
    have = {e[0].lower(): "ani" for e in ANIMATIONS + moves}
    have.update({e[0].lower(): "ani" for e in body.FACE_ANIS})
    have.update({b[0].lower(): "aniBlend" for b in BLENDS})
    if gothic_anims:
        mds += gothic_animations(gothic_dir, have, builder)
    with open(os.path.join(anims, MODEL + ".MDS"), "w", newline="\r\n") as f:
        f.write(f'// Geralt of Rivia from The Witcher 3: his skeleton, mesh and animations (make_geralt_anims.py)\n'
                f'Model ("{MODEL}")\n{{\n\tmeshAndTree ("{body.BODY}.ASC" DONT_USE_MESH)\n\tregisterMesh ("{body.BODY}.ASC")\n'
                f'\taniEnum\n\t{{\n{"".join(mds)}\t}}\n}}\n')
    for overlay, entries in overlays.items():
        with open(os.path.join(anims, overlay + ".MDS"), "w", newline="\r\n") as f:
            f.write(f'// Overlay for {MODEL}.MDS, applied by the Witcher plugin (make_geralt_anims.py)\n'
                    f'Model ("{overlay}")\n{{\n\tmeshAndTree ("{body.BODY}.ASC" DONT_USE_MESH)\n'
                    f'\taniEnum\n\t{{\n{"".join(builder.ani(*e) for e in entries)}\t}}\n}}\n')
    with open(os.path.join(gothic_dir, TURNS_FILE), "w", newline="\r\n") as f:
        f.write("# Geralt's turning animations (make_geralt_anims.py) for the Witcher plugin: animation, frames,\n"
                "# then his turn at each frame (degrees from the first, left positive)\n")
        for gname, turn in builder.turn_of.items():
            f.write(f"{gname}\t{len(turn)}\t" + " ".join(f"{t:.1f}" for t in turn) + "\n")
    # what Gothic compiled from earlier sources would be used instead of the new ones
    compiled = os.path.join(anims, "_COMPILED")
    stale = glob.glob(os.path.join(compiled, MODEL + ".*")) + glob.glob(os.path.join(compiled, MODEL + "-*.MAN"))
    stale += glob.glob(os.path.join(compiled, body.BODY + ".*"))
    for overlay in OVERLAYS:
        stale += glob.glob(os.path.join(compiled, overlay + ".*")) + glob.glob(os.path.join(compiled, overlay + "-*.MAN"))
    # animation sources of earlier runs that nothing uses any more
    stale += [p for p in glob.glob(os.path.join(anims, MODEL + "_*.ASC"))
              if os.path.basename(p).lower() not in builder.names and os.path.basename(p).upper() != body.BODY + ".ASC"]
    for path in stale:
        os.remove(path)
    print(f"Geralt: {len(builder.files)} animation sources -> {len(ANIMATIONS) + len(moves)} animations + "
          f"{len(BLENDS)} blends; {len(builder.turn_of)} turns in {TURNS_FILE}")


def patch_hero(gothic_dir):
    """Point the hero's model calls at Geralt, always from the original Gothic.dat."""
    path = os.path.join(gothic_dir, "_work", "DATA", "scripts", "_compiled", "Gothic.dat")
    backup = path + ".orig"
    if not os.path.exists(backup):
        shutil.copy2(path, backup)
    script = Script(open(backup, "rb").read())
    model = script.string_args_of_call("PC_HERO", "MDL_SETVISUAL")[0][-1]
    (body_arg, head), = script.string_args_of_call("PC_HERO", "MDL_SETVISUALBODY")
    for index, value in ((model, MODEL + ".MDS"), (body_arg, body.BODY), (head, "")):
        assert script.references(index) == 1, "string literal is shared with other code"
        script.symbols[index].content = [value.encode()]
    with open(path, "wb") as f:
        f.write(script.to_bytes())
    print(f"Gothic.dat: the hero is now {MODEL}.MDS / {body.BODY} (original kept as {os.path.basename(backup)})")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--w3", default=body.DEFAULT_W3, help="The Witcher 3 folder (with bin and content)")
    parser.add_argument("--gothic", default=body.DEFAULT_GOTHIC, help="the Gothic 1 folder")
    parser.add_argument("--no-gothic-anims", action="store_true", help="leave out the HUMANS.MDS stand-ins")
    parser.add_argument("--no-hero", action="store_true", help="do not patch Gothic.dat")
    args = parser.parse_args()
    build(args.w3, args.gothic, gothic_anims=not args.no_gothic_anims)
    if not args.no_hero:
        patch_hero(args.gothic)


if __name__ == "__main__":
    main()
