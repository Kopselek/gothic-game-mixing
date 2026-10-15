"""The Minecraft mashup, as the installer (tools/install.py) installs it: Gothic 1 or Gothic 2 as Steve.

Its tools read your Minecraft Java Edition files (the client jar's textures and models, the
sounds of its asset index) and write Steve, his animations and Minecraft's look into your Gothic
folder, and the Minecraft plugin brings Minecraft's camera, movement and the rest. See PLAN.md.
Steps (each tool can also be run on its own, see its usage):
  make_steve.py       Steve's model from his skin, Minecraft's animations for Gothic's names, and
                      the hero in Gothic.dat pointed at him
  make_mc_blocks.py   Minecraft's blocks: meshes and textures
  make_mc_dragon.py   the Ender Dragon's model parts (the plugin poses and flies it)
  make_mc_items.py    Minecraft's items (bow, arrows, flint and steel, eggs) in his hand, the arrow
                      he shoots, and the first-person hand: his arm, the items and blocks
  make_mc_sounds.py   Minecraft's sounds (explosions, the fuse, blocks; needs ffmpeg on PATH)
  make_mc_hud.py      Minecraft's HUD: its sprites, item and block icons and font in one atlas
"""
import os

from mcfiles import DEFAULT_MINECRAFT, check as check_minecraft

NAME = "minecraft"
TITLE = "Minecraft"
HERO = "Steve"
GAME_FOLDER = "the Minecraft folder (the one with versions and assets in it, usually %APPDATA%\\.minecraft)"
DEFAULT_DIR = DEFAULT_MINECRAFT  # the GUI fills it in when it exists
PLUGIN = "Minecraft"  # Data\Minecraft.vdf
INI_SECTION = "MINECRAFT"
INI_BACKUP = "Gothic.ini.before-steve"
STARTER = "Start Gothic as Steve.bat"
DEFAULT_LANG = None

# the installer's compile run (install.compile_run): the game started once with this argument and
# -zautoconvertdata, the plugin loads Steve so Gothic compiles him, then quits; these files show it did
COMPILE_ARG = "-mccompile"
COMPILED = [r"_work\DATA\ANIMS\_COMPILED\STEVE.MDH", r"_work\DATA\ANIMS\_COMPILED\STEVE_BODY.MDM",
            r"_work\DATA\ANIMS\_COMPILED\STEVE_1HST1.MDH", r"_work\DATA\ANIMS\_COMPILED\MCCREEPER.MDH",
            r"_work\DATA\ANIMS\_COMPILED\MC_ZOMBIE_BODY.MDM", r"_work\DATA\ANIMS\_COMPILED\MC_SKELETON_BODY.MDM"]

# Gothic's key bindings set to Minecraft's where Gothic has the action. On land the plugin moves
# Steve itself (W/S/A/D, Space, Left Shift, Left Ctrl sprints) and the mouse buttons attack and
# use; these bindings are Gothic's for the rest (swimming, ladders, dialogues): W/S forward and
# back, A/D step aside, Space jumps, Left Shift sneaks, the left mouse button (and Ctrl) the action,
# Tab Gothic's inventory (E is Minecraft's creative inventory, the plugin's); Left Alt walks and F
# draws the weapon, which Minecraft doesn't have.
# The arrow keys keep Gothic's moves. Values: DirectInput key codes, two bytes each, low byte first
# (the mouse's left button is 0x20C).
KEYS = {
    "keyUp": "c8001100",
    "keyDown": "d0001f00",
    "keyLeft": "cb00",
    "keyRight": "cd00",
    "keyStrafeLeft": "1e00",
    "keyStrafeRight": "2000",
    "keyAction": "1d000c02",
    "keySlow": "3800",
    "keySMove": "3900",
    "keyWeapon": "2100",
    "keySneak": "2a00",
    "keyLookFP": "5300",
    "keyInventory": "0f00",
    "keyShowStatus": "3000",
}

# [MINECRAFT]: the plugin's settings with their defaults (see ../plugin/src/Plugin.ipp); only
# missing ones are added, so your own values stay.
SETTINGS = [
    ("controls", "1", "Minecraft's camera (0: Gothic's own)"),
    ("physics", "1", "Minecraft's movement: speeds, sprint, sneak, jump and gravity (0: Gothic's)"),
    ("firstPerson", "1", "start in first person (F5 switches: first person, behind, in front)"),
    ("mouseSpeed", "0.25", "camera degrees per mouse unit"),
    ("invertMouseY", "0", "1 = inverted mouse look"),
    ("fov", "70", "vertical field of view (degrees, Minecraft's default)"),
    ("hud", "1", "Minecraft's HUD (hotbar, hearts, food, air, experience) in place of Gothic's bars"),
    ("guiScale", "0", "the HUD's scale (0: Minecraft's automatic one)"),
    ("vignette", "1", "the darkening towards the screen's edges"),
    ("hunger", "1", "Minecraft's hunger: the food bar, healing when full, starving when empty"),
    ("mobs", "1", "Gothic's zombies and skeletons look like Minecraft's"),
    ("debugLog", "0", "1 = write Minecraft.log in the Gothic folder"),
]

# what the tools write into the Gothic folder (for --uninstall)
MOD_FILES = [
    r"Data\Minecraft.vdf", STARTER, r"Minecraft.log",
    r"_work\DATA\ANIMS\STEVE*", r"_work\DATA\ANIMS\_COMPILED\STEVE*", r"Data\MC_Sounds.vdf",
    r"_work\DATA\ANIMS\MC*", r"_work\DATA\ANIMS\_COMPILED\MC*", r"_work\DATA\MESHES\MC",
    r"_work\DATA\MESHES\_COMPILED\MC_BLK_*", r"_work\DATA\MESHES\_COMPILED\MC_ITEM_*",
    r"_work\DATA\MESHES\_COMPILED\MC_ARROW*", r"_work\DATA\MESHES\_COMPILED\MC_FP_*",
    r"_work\DATA\MESHES\_COMPILED\MC_TNT_*", r"_work\DATA\MESHES\_COMPILED\MC_FIRE*",
    r"_work\DATA\MESHES\_COMPILED\MC_LAVA_*", r"_work\DATA\MESHES\_COMPILED\MC_DRAGON_*",
    r"_work\DATA\TEXTURES\_COMPILED\STEVE*", r"_work\DATA\TEXTURES\_COMPILED\MC_*",
]


def check(minecraft, lang=None):
    problems = check_minecraft(minecraft)
    try:
        import PIL  # noqa: F401  (make_steve.py reads the skin)
    except ImportError:
        problems.append("Pillow is missing: pip install pillow")
    return problems


def build(args, run):
    """Run the tools (install.py's run() calls each one's main())."""
    import make_mc_blocks
    import make_mc_dragon
    import make_mc_hud
    import make_mc_items
    import make_mc_sounds
    import make_steve

    common = ["--minecraft", args.game_dir, "--gothic", args.gothic]
    run(make_steve, *common)
    run(make_mc_blocks, *common)
    run(make_mc_items, *common)
    run(make_mc_dragon, *common)
    run(make_mc_sounds, *common)
    run(make_mc_hud, *common)


def starters(exe):
    """{start file: (its notes, how it starts the game's `exe` from system)}; install.py adds the Union check.

    It starts the game without -zautoconvertdata: the installer's compile run compiled Steve, and
    with the flag Gothic also tries to rebuild some of its own monsters from sources the game
    doesn't ship and stops when a skeleton comes near. Only if Steve isn't compiled (the compile
    run failed) does it pass the flag, so the game compiles him on that start."""
    convert = "set CONVERT=\r\n" + "".join(
        f'if not exist "..\\{path}" set CONVERT=-zautoconvertdata\r\n' for path in COMPILED)
    return {STARTER: (
        "rem Starts Gothic as Steve. The paths are this folder's own, so the Gothic folder can be moved\r\n"
        "rem or copied to another PC. -zautoconvertdata only if Steve isn't compiled yet.\r\n",
        convert + f'start "" {exe} %CONVERT%\r\n')}


def selftest():
    """For the GUI installer's --selftest: every tool imports."""
    import make_mc_blocks, make_mc_dragon, make_mc_hud, make_mc_items, make_mc_sounds, make_steve  # noqa: F401

    print("minecraft: tools import: ok")
