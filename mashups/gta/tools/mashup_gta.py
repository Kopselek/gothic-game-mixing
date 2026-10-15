"""The GTA San Andreas mashup, as the installer (tools/install.py) installs it: Gothic 1 as CJ.

Its tools read your GTA San Andreas (classic PC) install and write CJ, his animations, voice,
weapons, HUD, radar and motorbike into your Gothic folder, and the CJMovement plugin brings GTA's
controls, camera and the rest. Steps (each tool can also be run on its own, see its usage):
  make_cj_pistol.py   pistol, rocket launcher and dildo: models, sounds, the items in Gothic.dat
  make_cj_hud.py      HUD textures (fonts, weapon icons, crosshairs, radar sprites)
  make_cj_radar.py    the radar's map from Gothic's world map (writes the radar bounds into Gothic.ini)
  make_cj_bike.py     the motorbike (Data\\CJ_Bike.vdf, system\\CJBike.ini)
  make_cj_voice.py    CJ's voice for the hero's lines (Data\\CJ_Voice.vdf; needs ffmpeg on PATH)
  make_cj_model.py    CJ's model and animations, and the hero in Gothic.dat pointed at him
"""
import io
import os
import shutil

NAME = "gta"
TITLE = "GTA San Andreas"
HERO = "CJ"
GAME_FOLDER = "the classic PC GTA San Andreas folder (the one with models and anim in it)"
PLUGIN = "CJMovement"  # Data\CJMovement.vdf
INI_SECTION = "CJMOVEMENT"
INI_BACKUP = "Gothic.ini.before-cj"
STARTER = "Start Gothic with CJ.bat"
DEFAULT_LANG = None  # CJ speaks as in GTA: no dub to choose

# Gothic's own key bindings, set to GTA's where Gothic reads them itself (dialogue, menus): F is
# also the action key, Left Alt walks, Left Shift jumps, C sneaks, X is the weapon key, F and S
# leave the look and status keys. In play the CJMovement plugin answers every movement and combat
# key from GTA's controls anyway.
KEYS = {
    "keyAction": "1d002100",
    "keySlow": "3800",
    "keySMove": "2a00",
    "keyWeapon": "2d00",
    "keySneak": "2e003700",
    "keyLookFP": "5300",
    "keyShowStatus": "3000",
}

# [CJMOVEMENT]: the plugin's settings with their defaults (see ../plugin/src/Plugin.ipp); only
# missing ones are added, so your own values stay.
SETTINGS = [
    ("turnRate", "450", "CJ's turn speed in degrees per second"),
    ("mouseSpeed", "0.25", "camera degrees per mouse unit"),
    ("invertMouseY", "0", "1 = inverted mouse look"),
    ("camDistance", "300", "follow camera distance (cm)"),
    ("camHeight", "50", "height of the point the camera looks at (cm)"),
    ("camSmooth", "0.08", "seconds the camera takes to catch up with CJ"),
    ("gtaHud", "1", "GTA's HUD instead of Gothic's bars"),
    ("debugLog", "0", "1 = write CJMovement.log in the Gothic folder"),
    ("fistDamage", "1", "punch damage, share of GTA's"),
    ("runSpeed", "0.75", "run speed, share of GTA's"),
    ("sprintSpeed", "0.7", "sprint speed, share of GTA's"),
    ("sprintAccel", "0.8", "seconds from running to full sprint"),
    ("sprintDecel", "0.5", "seconds from full sprint back to running"),
    ("gunDamage", "1", "pistol damage, share of GTA's"),
    ("aimDistance", "150", "aiming camera distance (cm)"),
    ("aimSide", "40", "aiming camera's shift to the right (cm)"),
    ("aimHeight", "75", "aiming camera's look height (cm)"),
    ("startPistol", "1", "give CJ the pistol on a new game or load if he has none"),
    ("startRocket", "1", "give CJ the rocket launcher if he has none"),
    ("rocketRadius", "500", "rocket blast radius (cm)"),
    ("startDildo", "1", "give CJ the dildo if he has none"),
    ("fleeRadius", "2000", "how near (cm) enemies run from the drawn dildo"),
    ("slopeWalk", "60", "steepest ground (degrees) CJ walks on"),
    ("slopeSlide", "70", "where Gothic's steeper slide starts (degrees)"),
    ("maxFrameTime", "40", "longest step (ms) one frame may take with GTA controls"),
    ("radar", "1", "GTA's radar"),
    ("radarRange", "18000", "radar range (cm) from its centre to its rim"),
    ("gtaClock", "1", "Gothic's day at GTA's pace"),
    ("wastedCam", "1", "San Andreas's camera when CJ dies"),
    ("wastedHeight", "1500", "how high (cm) the wasted camera rises"),
    ("bike", "1", "GTA's motorbike next to CJ"),
    ("bikeCamDistance", "450", "camera distance behind the bike (cm)"),
    ("bikeGrip", "1", "tyre grip, share of GTA's"),
    ("bikeSpeed", "0.6", "bike top speed and pull, share of GTA's"),
    ("lightFlashes", "1", "shots and explosions light up their surroundings"),
    ("bikeLight", "1", "the bike's headlight at night"),
    ("crashDamage", "1", "how much a bike crash hurts CJ, share of the plugin's (0 = never)"),
]

# what the tools write into the Gothic folder (for --uninstall)
MOD_FILES = [
    r"Data\CJMovement.vdf", r"Data\CJ_Weapons.vdf", r"Data\CJ_Bike.vdf", r"Data\CJ_Voice.vdf",
    r"system\CJBike.ini", STARTER,
    r"_work\DATA\ANIMS\CJ*.ASC", r"_work\DATA\ANIMS\CJ*.MDS", r"_work\DATA\ANIMS\_COMPILED\CJ*",
    r"_work\DATA\TEXTURES\_COMPILED\CJ_*", r"_work\DATA\TEXTURES\_COMPILED\CJ-C.TEX", r"_work\DATA\MESHES\CJ",
]


def check(gta, lang=None):
    problems = []
    for need in (r"models\gta3.img", r"anim\ped.ifp", r"audio\SFX", r"data\handling.cfg"):
        if not os.path.exists(os.path.join(gta, need)):
            problems.append(f"{need} not found in the GTA San Andreas folder: it must be the classic PC GTA San "
                            "Andreas folder (the one with models and anim in it)")
    try:
        import PIL  # noqa: F401  (make_cj_radar.py)
    except ImportError:
        problems.append("Pillow is missing: pip install pillow")
    return problems


def build(args, run):
    """Run the tools (install.py's run() calls each one's main())."""
    # imported here, after check(), so a missing Pillow is reported instead of raised
    import make_cj_bike
    import make_cj_hud
    import make_cj_model
    import make_cj_pistol
    import make_cj_radar
    import make_cj_voice

    common = ["--gta", args.game_dir, "--gothic", args.gothic]
    run(make_cj_pistol, *common)
    run(make_cj_hud, *common)
    run(make_cj_radar, "--gothic", args.gothic)
    run(make_cj_bike, *common)
    if args.skip_voice:
        print("\n(voice skipped)")
    elif shutil.which("ffmpeg"):
        run(make_cj_voice, *common)
    else:
        print("\nffmpeg is not on PATH: CJ's voice is skipped (install ffmpeg and run make_cj_voice.py later)")
    run(make_cj_model, *common)


def starters():
    """{start file: (its notes, how it starts GothicMod.exe from system)}; install.py adds the Union check."""
    return {STARTER: (
        "rem -zautoconvertdata lets Gothic compile CJ's model and animations the first time they are needed;\r\n"
        "rem the path is this folder's own, so the Gothic folder can be moved or copied to another PC\r\n",
        'start "" GothicMod.exe -zautoconvertdata\r\n')}


def selftest():
    """For the GUI installer's --selftest: every tool imports, and Pillow reads TGA."""
    import make_cj_bike, make_cj_hud, make_cj_model, make_cj_pistol, make_cj_radar, make_cj_voice  # noqa: F401
    from PIL import Image

    tga = io.BytesIO()
    Image.new("RGBA", (2, 2)).save(tga, "TGA")
    Image.open(io.BytesIO(tga.getvalue())).convert("RGBA")
    print("gta: tools import, Pillow TGA: ok")
