"""Install a Gothic 1 mashup from your own game files.

Gothic 1 is the base of every mashup. You choose the game it is mashed up with, which brings its
hero, mechanics and content into Gothic:
  gta       GTA San Andreas (classic PC): play as CJ          mashups\\gta
  witcher   The Witcher 3: Wild Hunt: play as Geralt          mashups\\witcher

Nothing of either game ships with the mod: the mashup's tools read your own copy of that game and
write into your Gothic folder. Then this sets Gothic.ini up for the mashup's plugin and copies the
plugin into Data.

  python install.py --game gta --game-dir "D:\\Games\\GTA San Andreas" [--gothic DIR] [--plugin CJMovement.vdf] [--install-union]
  python install.py --game witcher --game-dir "D:\\Games\\The Witcher 3" [--lang pl] [...]
  python install.py --uninstall [--gothic DIR]

--gta DIR is short for --game gta --game-dir DIR, and --w3 DIR for --game witcher --game-dir DIR.

  union.py   Union 1.0m, downloaded and installed if it's missing and you agree (--install-union)
  then the mashup's own steps: see mashups\\<game>\\tools\\mashup_<game>.py

A Gothic folder holds one mashup at a time: installing one removes the other first, as --uninstall
does. Gothic.dat and Gothic.ini are backed up before anything changes (Gothic.dat.orig and the
mashup's Gothic.ini.before-*); --uninstall puts both back and removes the mashup's files.
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
INSTALLER_EXE = "GothicMashupInstaller.exe"

# each mashup's own tools (mashups\<game>\tools), next to Gothic's side here; the GUI installer has
# them all built in
for _tools in sorted(glob.glob(os.path.join(REPO, "mashups", "*", "tools"))):
    if _tools not in sys.path:
        sys.path.append(_tools)

import mashup_gta  # noqa: E402
import mashup_witcher  # noqa: E402

MASHUPS = {m.NAME: m for m in (mashup_gta, mashup_witcher)}


def gothic_paths(gothic):
    return (os.path.join(gothic, "_work", "DATA", "scripts", "_compiled", "Gothic.dat"),
            os.path.join(gothic, "system", "Gothic.ini"))


def installed_mashups(gothic):
    """The mashups in this Gothic folder: their plugin is in Data, or their Gothic.ini backup in system."""
    return [m for m in MASHUPS.values()
            if os.path.exists(os.path.join(gothic, "Data", m.PLUGIN + ".vdf"))
            or os.path.exists(os.path.join(gothic, "system", m.INI_BACKUP))]


def plugin_build(mashup):
    """Where plugin\\build.bat puts the mashup's plugin."""
    return os.path.join(REPO, "plugin", "out", "build", "G1-Release", mashup.NAME, "vdf", mashup.PLUGIN + ".vdf")


def find_plugin(mashup, folder=HERE):
    """The mashup's plugin next to the installer (a release has them there) or as plugin\\build.bat built it."""
    return next((p for p in (os.path.join(folder, mashup.PLUGIN + ".vdf"), plugin_build(mashup))
                 if os.path.exists(p)), None)


def check(gothic, game, game_dir, install_union=False, lang=None):
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
        problems.append("Union is not installed, or Steam put Gothic's own system\\Shw32.dll back over it (it does when "
                        "it updates or verifies the game): run again with --install-union to download and install "
                        f"Union 1.0m, or install it yourself ({union.UNION_THREAD})")
    return problems + MASHUPS[game].check(game_dir, lang)


def run(tool, *args):
    """Run a mashup tool's main() in this process: the GUI installer is one exe with no python.exe to
    start them with."""
    print(f"\n== {tool.__name__}.py {' '.join(args)}", flush=True)
    argv = sys.argv
    sys.argv = [f"{tool.__name__}.py", *args]
    try:
        tool.main()
    finally:
        sys.argv = argv


def set_up_ini(path, mashup):
    text = open(path, "rb").read().decode("latin-1")
    nl = "\r\n" if "\r\n" in text else "\n"  # keep the file's own line ends (Steam's is CRLF)
    lines = text.split(nl)
    for key, value in mashup.KEYS.items():
        for i, line in enumerate(lines):
            if line.split("=", 1)[0].strip() == key:
                lines[i] = f"{key}={value}"
    section = f"[{mashup.INI_SECTION}]"
    start = next((i for i, l in enumerate(lines) if l.strip().upper() == section), None)
    if start is None:
        lines += ["", section]
        start = len(lines) - 1
    end = next((i for i in range(start + 1, len(lines)) if lines[i].strip().startswith("[")), len(lines))
    have = {l.split("=", 1)[0].strip() for l in lines[start + 1:end] if "=" in l}
    added = []
    for key, value, note in mashup.SETTINGS:
        if key not in have:
            added += [f"{key}={value}", f"; ... {note}"]
    lines[end:end] = added
    open(path, "wb").write(nl.join(lines).encode("latin-1"))
    keys = f"Gothic's keys set to {mashup.TITLE}'s, " if mashup.KEYS else ""
    print(f"Gothic.ini: {keys}{len(added) // 2} {section} settings added")


def write_starters(gothic, mashup):
    """The mashup's start files, each checking first that Union is still there to load the plugin."""
    for name, (notes, start) in mashup.starters().items():
        open(os.path.join(gothic, name), "w", newline="").write(
            "@echo off\r\n"
            + notes
            + "rem Union loads the plugin: Steam puts Gothic's own Shw32.dll back when it updates the game\r\n"
            'findstr /m /c:"Union.patch" "%~dp0system\\Shw32.dll" >nul || goto nounion\r\n'
            'cd /d "%~dp0system"\r\n'
            + start
            + "exit /b\r\n"
            ":nounion\r\n"
            "echo Union is not active: Steam put Gothic's own system\\Shw32.dll back (it does that when it\r\n"
            f"echo updates or verifies the game). Run {INSTALLER_EXE} again, it reinstalls Union.\r\n"
            "pause\r\n")


def install(args):
    mashup, gothic = MASHUPS[args.game], args.gothic
    problems = check(gothic, args.game, args.game_dir, args.install_union, args.lang)
    if problems:
        sys.exit("Can't install:\n  - " + "\n  - ".join(problems))
    for other in installed_mashups(gothic):
        if other is not mashup:
            print(f"{other.HERO} ({other.TITLE}) is installed here: removing him first, a Gothic folder holds one "
                  "mashup at a time")
            uninstall(gothic, other)
    dat, ini = gothic_paths(gothic)
    if not os.path.exists(dat + ".orig"):
        shutil.copy2(dat, dat + ".orig")
    if not os.path.exists(os.path.join(os.path.dirname(ini), mashup.INI_BACKUP)):
        shutil.copy2(ini, os.path.join(os.path.dirname(ini), mashup.INI_BACKUP))
    if args.install_union and not union.installed(gothic):
        union.install(gothic)
    # before the tools, which may add to the mashup's section (make_cj_radar.py writes the radar's bounds)
    set_up_ini(ini, mashup)

    mashup.build(args, run)

    name = mashup.PLUGIN + ".vdf"
    plugin = args.plugin or find_plugin(mashup)
    if plugin and os.path.exists(plugin):
        shutil.copy2(plugin, os.path.join(gothic, "Data", name))
        print(f"plugin: {plugin} -> Data\\{name}")
    else:
        print(f"\nThe {mashup.PLUGIN} plugin is not installed yet: build it (plugin\\build.bat) or download {name} from "
              f"the releases, then run this again with --plugin <path> or copy it into Gothic\\Data.")
    write_starters(gothic, mashup)
    starter = next(iter(mashup.starters()))
    print(f"\nDone. Start the game with \"{os.path.join(gothic, starter)}\" (the first start compiles "
          f"{mashup.HERO} and takes a while).")


def uninstall(gothic, mashup):
    print(f"\n== removing {mashup.HERO} ({mashup.TITLE})")
    dat, ini = gothic_paths(gothic)
    if os.path.exists(dat + ".orig"):
        shutil.copy2(dat + ".orig", dat)
        os.remove(dat + ".orig")
        print("Gothic.dat restored")
    backup = os.path.join(os.path.dirname(ini), mashup.INI_BACKUP)
    if os.path.exists(backup):
        shutil.copy2(backup, ini)
        os.remove(backup)
        print("Gothic.ini restored")
    removed = 0
    for pattern in mashup.MOD_FILES:
        for path in glob.glob(os.path.join(gothic, pattern)):
            if os.path.isdir(path):
                shutil.rmtree(path)
            else:
                os.remove(path)
            removed += 1
    print(f"{removed} mod files removed")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--game", choices=sorted(MASHUPS),
                        help="the game Gothic is mashed up with: " + ", ".join(f"{m.NAME} ({m.TITLE}, play as {m.HERO})"
                                                                              for m in MASHUPS.values()))
    parser.add_argument("--game-dir", help="that game's install folder")
    parser.add_argument("--gta", metavar="DIR", help="short for --game gta --game-dir DIR")
    parser.add_argument("--w3", metavar="DIR", help="short for --game witcher --game-dir DIR")
    parser.add_argument("--gothic", default=DEFAULT_GOTHIC, help="Gothic 1 install folder")
    parser.add_argument("--plugin", help="the mashup's plugin to install, CJMovement.vdf or Witcher.vdf (default: one "
                                         "next to the installer, or the one plugin\\build.bat built)")
    parser.add_argument("--lang", help="The Witcher 3: the dub Geralt speaks: pl (default), en, de, ... "
                                       "(<lang>pc.w3speech)")
    parser.add_argument("--skip-voice", action="store_true", help="keep Gothic's hero voice")
    parser.add_argument("--install-union", action="store_true",
                        help="if Union is missing, download Union 1.0m (Gratt's GitHub archive) and install it")
    parser.add_argument("--uninstall", action="store_true",
                        help="restore Gothic.dat and Gothic.ini and remove the installed mashup's files")
    args = parser.parse_args(argv)
    for flag, game in (("gta", "gta"), ("w3", "witcher")):
        if getattr(args, flag):
            if args.game_dir or args.game not in (None, game):
                parser.error(f"--{flag} DIR is short for --game {game} --game-dir DIR: give one or the other")
            args.game, args.game_dir = game, getattr(args, flag)

    if args.uninstall:
        installed = installed_mashups(args.gothic)
        if args.game and MASHUPS[args.game] not in installed:
            sys.exit(f"{MASHUPS[args.game].HERO} ({MASHUPS[args.game].TITLE}) is not installed in {args.gothic}")
        if not installed:
            sys.exit(f"No mashup is installed in {args.gothic}")
        for mashup in [MASHUPS[args.game]] if args.game else installed:
            uninstall(args.gothic, mashup)
        if os.path.isdir(os.path.join(args.gothic, "system", union.BACKUP_DIR)):
            print(f"Union stays installed (the files it replaced are in system\\{union.BACKUP_DIR})")
        return
    if not args.game:
        parser.error("choose the game: --game gta or --game witcher (with --game-dir), or --gta DIR or --w3 DIR")
    if not args.game_dir:
        parser.error(f"--game-dir is required: {MASHUPS[args.game].GAME_FOLDER}")
    args.lang = args.lang or MASHUPS[args.game].DEFAULT_LANG
    if not args.install_union and not union.installed(args.gothic) and sys.stdin and sys.stdin.isatty():
        answer = input("Union is not installed in this Gothic folder (or Steam put Gothic's own DLLs back over it). "
                       "Download Union 1.0m and install it now? [y/N] ")
        args.install_union = answer.strip().lower() in ("y", "yes", "t", "tak")
    try:
        install(args)
    except PermissionError as e:
        sys.exit(f"Can't write {e.filename}: run the installer as administrator, or move Gothic out of Program Files")


if __name__ == "__main__":
    main()
