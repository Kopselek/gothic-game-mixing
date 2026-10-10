"""Install the GTA San Andreas CJ mod into Gothic 1 from your own game files.

Nothing of either game ships with the mod: this runs the make_cj_*.py tools, which read your
GTA San Andreas (classic PC) install and write CJ, his animations, voice, weapons, HUD, radar and
motorbike into your Gothic folder. Then it sets Gothic.ini up for GTA's keys and copies the
CJMovement plugin into Data.

  python install.py --gta "D:\\Games\\GTA San Andreas" [--gothic DIR] [--plugin CJMovement.vdf] [--install-union]
  python install.py --uninstall [--gothic DIR]

Steps (each tool can also be run on its own, see its usage):
  union.py            Union 1.0m, downloaded and installed if it's missing and you agree (--install-union)
  make_cj_pistol.py   pistol, rocket launcher and dildo: models, sounds, the items in Gothic.dat
  make_cj_hud.py      HUD textures (fonts, weapon icons, crosshairs, radar sprites)
  make_cj_radar.py    the radar's map from Gothic's world map (writes the radar bounds into Gothic.ini)
  make_cj_bike.py     the motorbike (Data\\CJ_Bike.vdf, system\\CJBike.ini)
  make_cj_voice.py    CJ's voice for the hero's lines (Data\\CJ_Voice.vdf; needs ffmpeg on PATH)
  make_cj_model.py    CJ's model and animations, and the hero in Gothic.dat pointed at him

Gothic.dat and Gothic.ini are backed up first (Gothic.dat.orig, Gothic.ini.before-cj);
--uninstall puts both back and removes the mod's files.
"""
import argparse
import glob
import os
import shutil
import sys

import union

DEFAULT_GOTHIC = r"C:\Program Files (x86)\Steam\steamapps\common\Gothic"
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
PLUGIN_BUILD = os.path.join(REPO, "plugin", "CJMovement", "out", "build", "G1-Release", "vdf", "CJMovement.vdf")
INI_BACKUP = "Gothic.ini.before-cj"

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

# [CJMOVEMENT]: the plugin's settings with their defaults (see plugin/CJMovement/src/Plugin.ipp);
# only missing ones are added, so your own values stay.
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
    r"system\CJBike.ini", r"Start Gothic with CJ.bat",
    r"_work\DATA\ANIMS\CJ*.ASC", r"_work\DATA\ANIMS\CJ*.MDS", r"_work\DATA\ANIMS\_COMPILED\CJ*",
    r"_work\DATA\TEXTURES\_COMPILED\CJ_*", r"_work\DATA\MESHES\CJ",
]


def gothic_paths(gothic):
    return (os.path.join(gothic, "_work", "DATA", "scripts", "_compiled", "Gothic.dat"),
            os.path.join(gothic, "system", "Gothic.ini"))


def check(gothic, gta, install_union=False):
    problems = []
    if not os.path.exists(os.path.join(gothic, "system", "GothicMod.exe")):
        problems.append("system\\GothicMod.exe is missing: choose the Gothic 1 folder (the one with system and Data in "
                        "it). Steam and GOG have it; a disk edition needs the Gothic 1 Player Kit 1.08k")
    if install_union:
        try:
            import py7zr  # noqa: F401  (union.py)
        except ImportError:
            problems.append("py7zr is missing (it unpacks Union): pip install py7zr")
    elif not union.installed(gothic):
        problems.append("Union is not installed (Data\\Union.vdf, system\\Union.patch): run again with --install-union "
                        f"to download and install Union 1.0m, or install it yourself ({union.UNION_THREAD})")
    if gta is not None:
        for need in (r"models\gta3.img", r"anim\ped.ifp", r"audio\SFX", r"data\handling.cfg"):
            if not os.path.exists(os.path.join(gta, need)):
                problems.append(f"{need} not found under --gta: it must be the classic PC GTA San Andreas folder")
    try:
        import PIL  # noqa: F401  (make_cj_radar.py)
    except ImportError:
        problems.append("Pillow is missing: pip install pillow")
    return problems


def run(tool, *args):
    """Run a make_cj_*.py tool's main() in this process: the GUI installer is one exe with no python.exe to
    start them with."""
    print(f"\n== {tool.__name__}.py {' '.join(args)}", flush=True)
    argv = sys.argv
    sys.argv = [f"{tool.__name__}.py", *args]
    try:
        tool.main()
    finally:
        sys.argv = argv


def set_up_ini(path):
    text = open(path, "rb").read().decode("latin-1")
    nl = "\r\n" if "\r\n" in text else "\n"  # keep the file's own line ends (Steam's is CRLF)
    lines = text.split(nl)
    for key, value in KEYS.items():
        for i, line in enumerate(lines):
            if line.split("=", 1)[0].strip() == key:
                lines[i] = f"{key}={value}"
    start = next((i for i, l in enumerate(lines) if l.strip().upper() == "[CJMOVEMENT]"), None)
    if start is None:
        lines += ["", "[CJMOVEMENT]"]
        start = len(lines) - 1
    end = next((i for i in range(start + 1, len(lines)) if lines[i].strip().startswith("[")), len(lines))
    have = {l.split("=", 1)[0].strip() for l in lines[start + 1:end] if "=" in l}
    added = []
    for key, value, note in SETTINGS:
        if key not in have:
            added += [f"{key}={value}", f"; ... {note}"]
    lines[end:end] = added
    open(path, "wb").write(nl.join(lines).encode("latin-1"))
    print(f"Gothic.ini: GTA's keys set, {len(added) // 2} [CJMOVEMENT] settings added")


def install(args):
    gothic, gta = args.gothic, args.gta
    problems = check(gothic, gta, args.install_union)
    if problems:
        sys.exit("Can't install:\n  - " + "\n  - ".join(problems))
    dat, ini = gothic_paths(gothic)
    if not os.path.exists(dat + ".orig"):
        shutil.copy2(dat, dat + ".orig")
    if not os.path.exists(os.path.join(os.path.dirname(ini), INI_BACKUP)):
        shutil.copy2(ini, os.path.join(os.path.dirname(ini), INI_BACKUP))
    if args.install_union and not union.installed(gothic):
        union.install(gothic)
    # before the tools: make_cj_radar.py writes the radar's bounds into [CJMOVEMENT]
    set_up_ini(ini)

    # imported here, after check(), so a missing Pillow is reported instead of raised
    import make_cj_bike
    import make_cj_hud
    import make_cj_model
    import make_cj_pistol
    import make_cj_radar
    import make_cj_voice

    common = ["--gta", gta, "--gothic", gothic]
    run(make_cj_pistol, *common)
    run(make_cj_hud, *common)
    run(make_cj_radar, "--gothic", gothic)
    run(make_cj_bike, *common)
    if args.skip_voice:
        print("\n(voice skipped)")
    elif shutil.which("ffmpeg"):
        run(make_cj_voice, *common)
    else:
        print("\nffmpeg is not on PATH: CJ's voice is skipped (install ffmpeg and run make_cj_voice.py later)")
    run(make_cj_model, *common)

    plugin = args.plugin or next((p for p in (os.path.join(HERE, "CJMovement.vdf"), PLUGIN_BUILD) if os.path.exists(p)), None)
    if plugin and os.path.exists(plugin):
        shutil.copy2(plugin, os.path.join(gothic, "Data", "CJMovement.vdf"))
        print(f"plugin: {plugin} -> Data\\CJMovement.vdf")
    else:
        print("\nThe CJMovement plugin is not installed yet: build it (plugin\\CJMovement\\build.bat) or download "
              "CJMovement.vdf from the releases, then run this again with --plugin <path> or copy it into Gothic\\Data.")
    starter = os.path.join(gothic, "Start Gothic with CJ.bat")
    open(starter, "w").write(
        "@echo off\r\n"
        "rem -zautoconvertdata lets Gothic compile CJ's model and animations the first time they are needed;\r\n"
        "rem the path is this folder's own, so the Gothic folder can be moved or copied to another PC\r\n"
        'cd /d "%~dp0system"\r\n'
        'start "" GothicMod.exe -zautoconvertdata\r\n')
    print(f"\nDone. Start the game with \"{starter}\" (the first start compiles CJ and takes a while).")


def uninstall(args):
    gothic = args.gothic
    dat, ini = gothic_paths(gothic)
    if os.path.exists(dat + ".orig"):
        shutil.copy2(dat + ".orig", dat)
        os.remove(dat + ".orig")
        print("Gothic.dat restored")
    backup = os.path.join(os.path.dirname(ini), INI_BACKUP)
    if os.path.exists(backup):
        shutil.copy2(backup, ini)
        os.remove(backup)
        print("Gothic.ini restored")
    removed = 0
    for pattern in MOD_FILES:
        for path in glob.glob(os.path.join(gothic, pattern)):
            if os.path.isdir(path):
                shutil.rmtree(path)
            else:
                os.remove(path)
            removed += 1
    print(f"{removed} mod files removed")
    if os.path.isdir(os.path.join(gothic, "system", union.BACKUP_DIR)):
        print(f"Union stays installed (the files it replaced are in system\\{union.BACKUP_DIR})")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--gta", help="GTA San Andreas (classic PC) install folder")
    parser.add_argument("--gothic", default=DEFAULT_GOTHIC, help="Gothic 1 install folder")
    parser.add_argument("--plugin", help="CJMovement.vdf to install (default: a built or downloaded one next to the tools)")
    parser.add_argument("--skip-voice", action="store_true", help="keep Gothic's hero voice")
    parser.add_argument("--install-union", action="store_true",
                        help="if Union is missing, download Union 1.0m (Gratt's GitHub archive) and install it")
    parser.add_argument("--uninstall", action="store_true", help="restore Gothic.dat and Gothic.ini, remove the mod's files")
    args = parser.parse_args(argv)
    if args.uninstall:
        uninstall(args)
        return
    if not args.gta:
        parser.error("--gta is required (your GTA San Andreas folder)")
    if not args.install_union and not union.installed(args.gothic) and sys.stdin and sys.stdin.isatty():
        answer = input("Union is not installed in this Gothic folder. Download Union 1.0m and install it now? [y/N] ")
        args.install_union = answer.strip().lower() in ("y", "yes", "t", "tak")
    try:
        install(args)
    except PermissionError as e:
        sys.exit(f"Can't write {e.filename}: run the installer as administrator, or move Gothic out of Program Files")


if __name__ == "__main__":
    main()
