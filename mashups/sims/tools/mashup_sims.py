"""The Sims 4 mashup, as the installer (tools/install.py) installs it: Gothic 1 or Gothic 2 played as a Sim.

Its tools read your The Sims 4 files (the UI's icons, font and sounds in its packages) and write
The Sims 4's UI into your Gothic folder, and the Sims plugin brings The Sims 4's camera, pie menu,
queue and the hero's autonomy (he walks, uses and fights on his own). The hero keeps Gothic's body.
See PLAN.md.
Steps (each tool can also be run on its own, see its usage):
  make_sims_ui.py   The Sims 4's UI: the pie menu's bubbles, panels, buttons, icons, the plumbob
                    and the font in one atlas, with the UI's texts in the game's language
"""
from s4files import DEFAULT_SIMS, check as check_sims

NAME = "sims"
TITLE = "The Sims 4"
HERO = "a Sim"
GAME_FOLDER = "the The Sims 4 folder (the one with Game and Data in it)"
DEFAULT_DIR = DEFAULT_SIMS  # the GUI fills it in when it exists
PLUGIN = "Sims"  # Data\Sims.vdf
INI_SECTION = "SIMS"
INI_BACKUP = "Gothic.ini.before-sim"
STARTER = "Start Gothic as a Sim.bat"
DEFAULT_LANG = None

# The plugin moves its own pointer with the mouse, which Gothic reads only while its mouse is on.
# Gothic's own key bindings stay: the plugin answers them for the hero while he is played as a Sim,
# and they are Gothic's in its screens (inventory, dialogues) and with F8 (Gothic's controls).
KEYS = {"enableMouse": "1"}

# [SIMS]: the plugin's settings with their defaults (see ../plugin/src/Plugin.ipp); only missing
# ones are added, so your own values stay.
SETTINGS = [
    ("controls", "1", "The Sims 4's camera and controls (0: Gothic's own; F8 switches in game)"),
    ("pointerSpeed", "1.5", "pointer pixels per mouse unit"),
    ("takeSpeed", "2.0", "how much faster he picks things up (Gothic's animation sped up)"),
    ("cutaway", "1", "walls, roofs and objects between the camera and him cut away (0: none)"),
    ("cutRadius", "400", "cm round him kept when the world is cut away"),
    ("edgeScroll", "1", "the pointer at the screen's edge moves the camera"),
    ("uiScale", "1.0", "the size of the UI (1: The Sims 4's at 1080 lines)"),
    ("autoFight", "1", "start with auto fight on: he fights back whoever attacks him"),
    ("fightAI", "0", "0: Gothic's combat messages (AI_Attack) fight for him; 1: the script state ZS_ATTACK"),
    ("icons", "1", "icons over everything he can use near him (0: only under the pointer)"),
    ("plumbob", "1", "the plumbob over his head"),
    ("bridge", "0", "1: the test bridge (Sims.cmd: commands, Sims.state: state, in the Gothic folder)"),
    ("debugLog", "0", "1 = write Sims.log in the Gothic folder"),
]

# what the tools write into the Gothic folder (for --uninstall)
MOD_FILES = [
    r"Data\Sims.vdf", STARTER, r"Sims.log", r"Sims.cmd", r"Sims.state",
    r"_work\DATA\TEXTURES\_COMPILED\SIMS_*",
]


def check(sims, lang=None):
    problems = check_sims(sims)
    try:
        import PIL  # noqa: F401  (make_sims_ui.py draws the atlas)
        import numpy  # noqa: F401
    except ImportError:
        problems.append("Pillow or numpy is missing: pip install pillow numpy")
    return problems


def build(args, run):
    """Run the tools (install.py's run() calls each one's main())."""
    import make_sims_ui

    run(make_sims_ui, "--sims", args.game_dir, "--gothic", args.gothic)


def starters(exe):
    """{start file: (its notes, how it starts the game's `exe` from system)}; install.py adds the Union check.

    Nothing to compile: the UI is written compiled (-C.TEX), so the game starts without
    -zautoconvertdata, with which Gothic stops near some of its own skeletons."""
    return {STARTER: (
        "rem Starts Gothic played as a Sim. The paths are this folder's own, so the Gothic folder can be\r\n"
        "rem moved or copied to another PC.\r\n",
        f'start "" {exe}\r\n')}


def selftest():
    """For the GUI installer's --selftest: every tool imports."""
    import make_sims_ui, s4audio, s4files, s4gfx, s4icons, s4image, s4package  # noqa: F401

    print("sims: tools import: ok")
