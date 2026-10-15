"""The Witcher 3 mashup, as the installer (tools/install.py) installs it: Gothic 1 or Gothic 2 as Geralt.

Its tools read your The Witcher 3 install and write Geralt (his model, face, animations and voice)
into your Gothic folder, and the Witcher plugin brings The Witcher 3's controls, camera, sword
fighting and signs. Steps (each tool can also be run on its own, see its usage):
  make_geralt_model.py    his skeleton (with the face rig), skinned body and textures
  make_geralt_anims.py    his animations and face animations, and the hero in Gothic.dat pointed at him
  make_geralt_voice.py    his own lines in fights and while exploring (Data\\Geralt_Voice.vdf; needs ffmpeg)
  make_geralt_dialogue.py his voice for the hero's dialogue lines (Data\\Geralt_Dialogue.vdf; needs ffmpeg)
"""
import glob
import os
import shutil

NAME = "witcher"
TITLE = "The Witcher 3"
HERO = "Geralt"
GAME_FOLDER = "The Witcher 3 folder (the one with bin and content in it)"
PLUGIN = "Witcher"  # Data\Witcher.vdf
INI_SECTION = "WITCHER"
INI_BACKUP = "Gothic.ini.before-geralt"
STARTER = "Start Gothic as Geralt.bat"
STARTER_WINDOWED = "Start Gothic as Geralt (window).bat"
DEFAULT_LANG = "pl"  # the dub Geralt speaks, one of dubs()

KEYS = {}  # the plugin answers The Witcher 3's keys itself; Gothic's own bindings stay

# [WITCHER]: the plugin's settings with their defaults (see ../plugin/src/Plugin.ipp); only missing
# ones are added, so your own values stay.
SETTINGS = [
    ("controls", "1", "The Witcher 3's controls, camera and combat (0: Gothic's own)"),
    ("mouseSpeed", "0.25", "camera degrees per mouse unit"),
    ("invertMouseY", "0", "1 = inverted mouse look"),
    ("turnRate", "720", "degrees a second Geralt turns standing or walking"),
    ("turnRateRun", "450", "... running"),
    ("turnRateSprint", "300", "... sprinting"),
    ("gameTurns", "1", "the game's own 180-degree turns and starts to the side or back"),
    ("camAutoCenter", "1", "the camera swings in behind him while he moves (the game's AutoCameraCenter)"),
    ("camDistance", "185", "exploration camera distance (cm)"),
    ("combatDistance", "150", "combat camera distance (cm)"),
    ("sprintAccel", "0.5", "seconds from running to full sprint"),
    ("sprintDecel", "0.4", "seconds from full sprint back to running"),
    ("fastDamage", "0.15", "share of the victim's maximum health a fast swing takes"),
    ("strongDamage", "0.29", "... a strong swing"),
    ("wrongSword", "1", "damage share with the wrong sword for the victim (1 = full)"),
    ("killHumans", "0", "1 = people he beats down die instead of Gothic's knock-out"),
    ("counterWindow", "0.45", "seconds before a hit in which a guard press counters it"),
    ("signKnockdown", "0.5", "Aard's knock-down chance"),
    ("igniDamage", "0.2", "Igni's share of the victim's health"),
    ("igniBurn", "0.5", "Igni's chance to set them burning"),
    ("startSword", "1", "give Geralt a one-handed sword if he has no melee weapon"),
    ("face", "1", "his face moves: talking, blinking, hurt, expressions"),
    ("voice", "1", "his own lines in fights and while exploring"),
    ("voiceSubtitles", "1", "subtitles for them"),
    ("voiceVolume", "1", "their volume"),
    ("debugLog", "0", "1 = write Witcher.log in the Gothic folder"),
]

# what the tools write into the Gothic folder (for --uninstall)
MOD_FILES = [
    r"Data\Witcher.vdf", r"Data\Geralt_Voice.vdf", r"Data\Geralt_Dialogue.vdf",
    r"Witcher_voice.txt", r"Witcher_turns.txt", r"Witcher_subtitles.txt", r"Witcher.log",
    STARTER, STARTER_WINDOWED,
    r"_work\DATA\ANIMS\GERALT*", r"_work\DATA\ANIMS\_COMPILED\GERALT*",
    r"_work\DATA\TEXTURES\_COMPILED\GERALT*",
]


def dubs(w3):
    """The Witcher 3 dubs installed (<lang>pc.w3speech in content0)."""
    found = glob.glob(os.path.join(w3, "content", "content0", "*pc.w3speech"))
    return sorted(os.path.basename(p)[:-len("pc.w3speech")] for p in found)


def check(w3, lang=None):
    problems = []
    for need in (r"content\content0\texture.cache", r"content\content0\bundles"):
        if not os.path.exists(os.path.join(w3, need)):
            problems.append(f"{need} not found in The Witcher 3 folder: it must be The Witcher 3 folder (the one with "
                            "bin and content in it)")
    if lang and lang not in dubs(w3) and os.path.isdir(os.path.join(w3, "content", "content0")):
        problems.append(f"The Witcher 3 has no {lang} dub ({lang}pc.w3speech); it has: {', '.join(dubs(w3)) or 'none'}")
    try:
        import numpy  # noqa: F401  (make_geralt_model.py)
    except ImportError:
        problems.append("numpy is missing: pip install numpy")
    return problems


def build(args, run):
    """Run the tools (install.py's run() calls each one's main())."""
    # imported here, after check(), so a missing numpy is reported instead of raised
    import make_geralt_anims
    import make_geralt_dialogue
    import make_geralt_model
    import make_geralt_voice

    common = ["--w3", args.game_dir, "--gothic", args.gothic]
    run(make_geralt_model, *common)
    run(make_geralt_anims, *common)
    if args.skip_voice:
        print("\n(voice skipped)")
    elif shutil.which("ffmpeg"):
        run(make_geralt_voice, *common, "--lang", args.lang)
        run(make_geralt_dialogue, *common, "--lang", args.lang)
    else:
        print("\nffmpeg is not on PATH: Geralt's voice is skipped (install ffmpeg and run the installer again)")


def starters(exe):
    """{start file: (its notes, how it starts the game's `exe` from system)}; install.py adds the Union check.

    They let Gothic compile Geralt only while he isn't compiled yet: with -zautoconvertdata Gothic
    also tries to rebuild some of its own monsters from sources the game doesn't ship and stops
    with "ASC not found: SKE_SPAWN_A02"."""
    notes = ("rem Starts Gothic with Geralt. The first start after an install compiles his model and\r\n"
             "rem animations (-zautoconvertdata) and takes longer; later ones go without it.\r\n")
    convert = ("set CONVERT=\r\n"
               'if not exist "..\\_work\\DATA\\ANIMS\\_COMPILED\\GERALT.MDH" set CONVERT=-zautoconvertdata\r\n'
               'if not exist "..\\_work\\DATA\\ANIMS\\_COMPILED\\GERALT_BODY.MDM" set CONVERT=-zautoconvertdata\r\n'
               'if not exist "..\\_work\\DATA\\ANIMS\\_COMPILED\\GERALT_SPRINT.MDH" set CONVERT=-zautoconvertdata\r\n')
    return {STARTER: (notes, convert + f'start "" {exe} %CONVERT%\r\n'),
            STARTER_WINDOWED: (notes, convert + f'start "" {exe} %CONVERT% -zwindow\r\n')}


def selftest():
    """For the GUI installer's --selftest: every tool imports, and numpy is there."""
    import make_geralt_anims, make_geralt_dialogue, make_geralt_model, make_geralt_voice  # noqa: F401
    import numpy
    import w3mimic  # noqa: F401  (imported by the model tool inside a function)

    print("witcher: tools import: ok, numpy", numpy.__version__)
