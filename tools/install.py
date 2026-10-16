"""Install a Gothic mashup from your own game files.

The base game is Gothic 1 or Gothic 2 Night of the Raven (--gothic-version 1 or 2, the same
plugin and tools for both). You choose the game it is mashed up with, which brings its hero,
mechanics and content into Gothic:
  gta       GTA San Andreas (classic PC): play as CJ          mashups\\gta
  witcher   The Witcher 3: Wild Hunt: play as Geralt          mashups\\witcher
  minecraft Minecraft (Java Edition): play as Steve           mashups\\minecraft

Nothing of the other game ships with the mod: the mashup's tools read your own copy of that game and
write into your Gothic folder. Then this sets Gothic.ini up for the mashup's plugin and copies the
plugin into Data.

  python install.py --game gta --game-dir "D:\\Games\\GTA San Andreas" [--gothic DIR] [--gothic-version 1|2] [--plugin CJMovement.vdf] [--install-union]
  python install.py --game witcher --game-dir "D:\\Games\\The Witcher 3" [--lang pl] [...]
  python install.py --game minecraft --game-dir "%APPDATA%\\.minecraft" [...]
  python install.py --uninstall [--gothic DIR]

--gta DIR is short for --game gta --game-dir DIR, and --w3 DIR for --game witcher --game-dir DIR.
The Gothic folder must hold the chosen version (system\\GothicMod.exe or system\\Gothic2.exe).

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
import subprocess
import sys
import time

import union
from gothic import EXE, GOTHIC_1, GOTHIC_2, TITLE, check as check_gothic

DEFAULT_GOTHIC = r"C:\Program Files (x86)\Steam\steamapps\common\Gothic"
PLUGIN_PRESET = "G1-G2A-Release"  # plugin\build.bat's: one DLL for Gothic 1 and Gothic 2 NotR
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
INSTALLER_EXE = "GothicMashupInstaller.exe"

# each mashup's own tools (mashups\<game>\tools), next to Gothic's side here; the GUI installer has
# them all built in
for _tools in sorted(glob.glob(os.path.join(REPO, "mashups", "*", "tools"))):
    if _tools not in sys.path:
        sys.path.append(_tools)

import mashup_gta  # noqa: E402
import mashup_minecraft  # noqa: E402
import mashup_witcher  # noqa: E402

MASHUPS = {m.NAME: m for m in (mashup_gta, mashup_witcher, mashup_minecraft)}


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
    return os.path.join(REPO, "plugin", "out", "build", PLUGIN_PRESET, mashup.NAME, "vdf", mashup.PLUGIN + ".vdf")


def find_plugin(mashup, folder=HERE):
    """The mashup's plugin next to the installer (a release has them there) or as plugin\\build.bat built it."""
    return next((p for p in (os.path.join(folder, mashup.PLUGIN + ".vdf"), plugin_build(mashup))
                 if os.path.exists(p)), None)


def check(gothic, game, game_dir, install_union=False, lang=None, version=GOTHIC_1):
    problems = check_gothic(gothic, version)
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


def write_starters(gothic, mashup, version):
    """The mashup's start files, each checking first that Union is still there to load the plugin."""
    for name, (notes, start) in mashup.starters(EXE[version]).items():
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
    problems = check(gothic, args.game, args.game_dir, args.install_union, args.lang, args.gothic_version)
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
    installed_plugin = plugin and os.path.exists(plugin)
    if installed_plugin:
        shutil.copy2(plugin, os.path.join(gothic, "Data", name))
        print(f"plugin: {plugin} -> Data\\{name}")
    else:
        print(f"\nThe {mashup.PLUGIN} plugin is not installed yet: build it (plugin\\build.bat) or download {name} from "
              f"the releases, then run this again with --plugin <path> or copy it into Gothic\\Data.")
    write_starters(gothic, mashup, args.gothic_version)
    compiled = False
    if getattr(mashup, "COMPILE_ARG", None) and installed_plugin and union.installed(gothic) and not args.no_compile:
        compiled = compile_run(gothic, mashup, args.gothic_version)
    starter = next(iter(mashup.starters(EXE[args.gothic_version])))
    print(f"\nDone: {TITLE[args.gothic_version]} with {mashup.HERO}. Start the game with "
          f"\"{os.path.join(gothic, starter)}\""
          + ("." if compiled else f" (the first start compiles {mashup.HERO} and takes a while)."))


def ini_value(path, section, key, value):
    """Set `key` in [section] of an ini file (Gothic.ini) to `value` if it is there; the old value, or None."""
    text = open(path, "rb").read().decode("latin-1")
    nl = "\r\n" if "\r\n" in text else "\n"
    lines = text.split(nl)
    inside = False
    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith("["):
            inside = stripped.upper() == f"[{section.upper()}]"
        elif inside and stripped.split("=", 1)[0].strip().lower() == key.lower() and "=" in stripped:
            old = stripped.split("=", 1)[1]
            if old != value:
                lines[i] = f"{key}={value}"
                open(path, "wb").write(nl.join(lines).encode("latin-1"))
            return old
    return None


def ini_read(path, section, key):
    """`key` in [section] of an ini file, stripped, or None."""
    inside = False
    for line in open(path, "rb").read().decode("latin-1").splitlines():
        stripped = line.strip()
        if stripped.startswith("["):
            inside = stripped.upper() == f"[{section.upper()}]"
        elif inside and "=" in stripped and stripped.split("=", 1)[0].strip().lower() == key.lower():
            return stripped.split("=", 1)[1].strip()
    return None


def screen_resolution():
    """The screen's display mode in real pixels (EnumDisplaySettings), or None."""
    if os.name != "nt":
        return None
    import ctypes
    from ctypes import wintypes

    class DEVMODEW(ctypes.Structure):
        _fields_ = [("dmDeviceName", wintypes.WCHAR * 32), ("dmSpecVersion", wintypes.WORD),
                    ("dmDriverVersion", wintypes.WORD), ("dmSize", wintypes.WORD), ("dmDriverExtra", wintypes.WORD),
                    ("dmFields", wintypes.DWORD), ("dmPositionX", wintypes.LONG), ("dmPositionY", wintypes.LONG),
                    ("dmDisplayOrientation", wintypes.DWORD), ("dmDisplayFixedOutput", wintypes.DWORD),
                    ("dmColor", ctypes.c_short), ("dmDuplex", ctypes.c_short), ("dmYResolution", ctypes.c_short),
                    ("dmTTOption", ctypes.c_short), ("dmCollate", ctypes.c_short), ("dmFormName", wintypes.WCHAR * 32),
                    ("dmLogPixels", wintypes.WORD), ("dmBitsPerPel", wintypes.DWORD), ("dmPelsWidth", wintypes.DWORD),
                    ("dmPelsHeight", wintypes.DWORD), ("dmDisplayFlags", wintypes.DWORD),
                    ("dmDisplayFrequency", wintypes.DWORD), ("dmRest", wintypes.DWORD * 8)]

    mode = DEVMODEW()
    mode.dmSize = ctypes.sizeof(DEVMODEW)
    if not ctypes.windll.user32.EnumDisplaySettingsW(None, -1, ctypes.byref(mode)):  # ENUM_CURRENT_SETTINGS
        return None
    return (mode.dmPelsWidth, mode.dmPelsHeight) if mode.dmPelsWidth and mode.dmPelsHeight else None


def full_screen_settings(gothic):
    """The settings that start Gothic in a window although it starts without -zwindow, set to full
    screen: Gothic.ini's zStartupWindowed, SystemPack.ini's BorderlessWindow and FixAppCompat=2 (a
    window without a frame). The changes, as text."""
    changed = []
    ini = gothic_paths(gothic)[1]
    pack = os.path.join(gothic, "system", "SystemPack.ini")
    for path, section, key, bad, good in ((ini, "VIDEO", "zStartupWindowed", None, "0"),
                                          (pack, "DEBUG", "BorderlessWindow", None, "0"),
                                          (pack, "DEBUG", "FixAppCompat", "2", "1")):
        old = ini_read(path, section, key) if os.path.exists(path) else None
        if old is not None and old != good and (bad is None or old == bad):
            ini_value(path, section, key, good)
            changed.append(f"{os.path.basename(path)} {key}={good}")
    return changed


def resolution_settings(gothic):
    """Gothic's full-screen resolution set to the screen's own, 32 bits, when it isn't already. The
    change, as text."""
    screen = screen_resolution()
    ini = gothic_paths(gothic)[1]
    if not screen:
        return []
    want = (str(screen[0]), str(screen[1]), "32")
    keys = ("zVidResFullscreenX", "zVidResFullscreenY", "zVidResFullscreenBPP")
    old = tuple(ini_read(ini, "VIDEO", k) for k in keys)
    if old == want or None in old:
        return []
    for key, value in zip(keys, want):
        ini_value(ini, "VIDEO", key, value)
    return [f"Gothic.ini resolution {want[0]} x {want[1]}, 32 bits (was {old[0]} x {old[1]}, {old[2]} bits)"]


def error_box(pid):
    """The text of Gothic's error box ("Error-Message", shown before it stops on a fatal error) when
    the process `pid` has one open, else None."""
    if os.name != "nt":
        return None
    import ctypes
    from ctypes import wintypes
    user32 = ctypes.windll.user32
    proc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    found = []

    def text(hwnd):
        buffer = ctypes.create_unicode_buffer(1024)
        user32.GetWindowTextW(hwnd, buffer, 1024)
        return buffer.value

    def child(hwnd, _):
        name = ctypes.create_unicode_buffer(64)
        user32.GetClassNameW(hwnd, name, 64)
        if name.value.lower() == "static" and text(hwnd).strip():
            found.append(text(hwnd).strip())
        return True

    def top(hwnd, _):
        owner = wintypes.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        if owner.value == pid and user32.IsWindowVisible(hwnd) and text(hwnd) == "Error-Message":
            found.append("")
            user32.EnumChildWindows(hwnd, proc(child), 0)
        return True

    user32.EnumWindows(proc(top), 0)
    return " ".join(found).strip() if found else None


def wait_gothic(process, timeout):
    """Wait for Gothic to close: None when it did, "timeout", or the text of its error box (Gothic is
    then closed by its process handle, as clicking OK would)."""
    end = time.time() + timeout
    while process.poll() is None:
        error = error_box(process.pid)
        if error is not None or time.time() > end:
            process.kill()  # this one process, by its handle
            process.wait()
            return "timeout" if error is None else error or "(no text)"
        time.sleep(0.5)
    return None


def video_fix(gothic, window):
    """Gothic's Direct3D 7 renderer didn't start (XD3D_InitPerDX: "Can't create D3DX context"): in a
    window it can't on some PCs, and full screen only at a resolution the screen has. Changes the
    next thing to try: full screen where its settings ask for a window, then the screen's own
    resolution, then a window (returns "window"). False when nothing is left to try."""
    changed = [] if window else full_screen_settings(gothic) or resolution_settings(gothic)
    if changed:
        print(f"Gothic's renderer didn't start with its video settings: starting it again with {', '.join(changed)}",
              flush=True)
        return True
    if not window:
        print("Gothic's renderer didn't start full screen at the screen's resolution either: trying a window", flush=True)
        return "window"
    print("Gothic's renderer (Direct3D 7) starts neither full screen nor in a window on this PC, with or without "
          "the mod. Update the graphics driver; on a laptop with two graphics cards, give GothicMod.exe or "
          "Gothic2.exe the dedicated one in Windows's graphics settings.", flush=True)
    return False


def compile_run(gothic, mashup, version, timeout=900):
    """Start the game once with -zautoconvertdata and the mashup's COMPILE_ARG: its plugin loads the
    hero's model, overlays and meshes on the first frame (the menu), so Gothic compiles them, and
    quits. The players' starts then go without -zautoconvertdata, with which Gothic stops where a
    skeleton appears (see the mashup's starters). The logo videos are off for the run, and
    Gothic.ini's crash mark is cleared after it. True if the mashup's COMPILED files are there.
    It starts as the start file does, with the player's own video settings, so it also shows that
    Gothic's renderer starts with them; where it doesn't (Gothic's error box "Can't create D3DX
    context", see video_fix), the settings are changed until it does."""
    print(f"\n== compile run: Gothic starts once to compile {mashup.HERO}, and closes by itself", flush=True)
    ini = gothic_paths(gothic)[1]
    videos = ini_value(ini, "GAME", "playLogoVideos", "0")
    exe = os.path.join(gothic, "system", EXE[version])
    started = time.time()
    window = False  # -zwindow, the last thing tried for the renderer
    try:
        # A second start if the first one stopped early. On the first start from a folder Union's
        # SystemPack asks to restart the game, as it has set Windows's compatibility options for it.
        starts = 0
        while starts < 2:
            starts += 1
            process = subprocess.Popen([exe, "-zautoconvertdata", *(["-zwindow"] if window else []), mashup.COMPILE_ARG],
                                       cwd=os.path.dirname(exe))
            error = wait_gothic(process, timeout)
            if error == "timeout":
                print(f"Gothic didn't close within {timeout // 60} minutes: closed it (process {process.pid})")
                break
            if error is not None:
                print(f"Gothic stopped with an error: {error}", flush=True)
                if "D3DX" not in error:
                    break
                fix = video_fix(gothic, window)
                if not fix:
                    break
                if fix == "window":
                    window = True
                starts -= 1
                continue
            if window:
                ini_value(ini, "VIDEO", "zStartupWindowed", "1")
                print("Gothic's renderer started in a window: Gothic.ini zStartupWindowed=1, so the start file "
                      "starts it so too")
                window = False
            missing = [p for p in mashup.COMPILED if not os.path.exists(os.path.join(gothic, p))]
            if not missing or starts == 2:
                break
            print(f"Gothic closed before {mashup.HERO} was compiled: starting it once more", flush=True)
    finally:
        if videos is not None:
            ini_value(ini, "GAME", "playLogoVideos", videos)
        ini_value(ini, "INTERNAL", "gameAbnormalExit", "0")
    missing = [p for p in mashup.COMPILED if not os.path.exists(os.path.join(gothic, p))]
    if missing:
        print(f"compile run: not compiled (Gothic's exit code {process.returncode & 0xFFFFFFFF:#x}): {', '.join(missing)}; "
              f"the start file compiles {mashup.HERO} on the first start")
        return False
    print(f"compile run: {mashup.HERO} compiled in {time.time() - started:.0f} s")
    return True


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
    parser.add_argument("--gothic", default=DEFAULT_GOTHIC, help="the Gothic install folder (the one with system and Data)")
    parser.add_argument("--gothic-version", type=int, choices=(GOTHIC_1, GOTHIC_2), default=GOTHIC_1,
                        help="the base game: 1 = Gothic 1 (default), 2 = Gothic 2 Night of the Raven")
    parser.add_argument("--plugin", help="the mashup's plugin to install, CJMovement.vdf or Witcher.vdf (default: one "
                                         "next to the installer, or the one plugin\\build.bat built)")
    parser.add_argument("--lang", help="The Witcher 3: the dub Geralt speaks: pl (default), en, de, ... "
                                       "(<lang>pc.w3speech)")
    parser.add_argument("--skip-voice", action="store_true", help="keep Gothic's hero voice")
    parser.add_argument("--no-compile", action="store_true",
                        help="don't start the game once to compile the hero (the first start of the game does it then)")
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
