"""Find the player's own The Sims 4 (PC) and list its packages in the order the game loads them.

Where it is installed:
  Steam    <library>\\steamapps\\common\\The Sims 4, the libraries listed in
           <Steam>\\steamapps\\libraryfolders.vdf (Steam's folder from the registry,
           HKCU\\Software\\Valve\\Steam "SteamPath")
  EA app   HKLM\\SOFTWARE\\Maxis\\The Sims 4 "Install Dir" (also under WOW6432Node), usually
           C:\\Program Files\\EA Games\\The Sims 4 (Origin: C:\\Program Files (x86)\\Origin Games)
Both editions hold the same files:
  Game\\Bin\\TS4_x64.exe                 the game (never started by these tools)
  Data\\Client\\*.package                 the client's resources: images, UI, sounds, models
  Data\\Client\\Resource.cfg              their priorities: "Priority <n>" then "PackedFile <glob>"
                                         lines; a higher priority wins (the deltas -20 over the
                                         full builds -30 over UI.package -40); "Select X .. End"
                                         blocks are for consoles
  Data\\Simulation\\*.package             the simulation's tuning, same scheme
  Delta\\<pack>\\*.package                updates of the packs (EP expansions, GP game packs,
                                         SP stuff packs, FP free packs), there for every pack
  <pack>\\*.package                       an installed pack's own resources (EP01, GP01, ...)
  Delta\\<pack>\\Version.ini              "packversion = 1.128.90.1030": the game's version

  python s4files.py [--sims DIR] [info | packages [--simulation]]
"""
import argparse
import fnmatch
import glob
import os
import re
import sys

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))

EXE = os.path.join("Game", "Bin", "TS4_x64.exe")
CLIENT = os.path.join("Data", "Client")
SIMULATION = os.path.join("Data", "Simulation")
PACK_ORDER = "EP", "GP", "SP", "FP"  # packs load after the base game, in this order and by number
STEAM_FOLDER = r"C:\Program Files (x86)\Steam"
EA_FOLDERS = [r"C:\Program Files\EA Games\The Sims 4", r"C:\Program Files (x86)\EA Games\The Sims 4",
              r"C:\Program Files (x86)\Origin Games\The Sims 4", r"C:\Program Files\Origin Games\The Sims 4"]


def _registry(root, path, name):
    try:
        import winreg
        with winreg.OpenKey(root, path) as key:
            return winreg.QueryValueEx(key, name)[0]
    except (ImportError, OSError):
        return None


def steam_libraries():
    """Every Steam library folder (Steam's own first)."""
    try:
        import winreg
        steam = (_registry(winreg.HKEY_CURRENT_USER, r"Software\Valve\Steam", "SteamPath")
                 or _registry(winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\WOW6432Node\Valve\Steam", "InstallPath"))
    except ImportError:
        steam = None
    steam = os.path.normpath(steam) if steam else STEAM_FOLDER
    found = [steam]
    try:
        with open(os.path.join(steam, "steamapps", "libraryfolders.vdf"), encoding="utf-8", errors="replace") as f:
            text = f.read()
    except OSError:
        return found
    for path in re.findall(r'"path"\s+"([^"]*)"', text):
        path = os.path.normpath(path.replace("\\\\", "\\"))
        if path.lower() not in (p.lower() for p in found):
            found.append(path)
    return found


def candidates():
    """Folders where The Sims 4 may be, most likely first (not checked)."""
    found = [os.path.join(lib, "steamapps", "common", "The Sims 4") for lib in steam_libraries()]
    try:
        import winreg
        for path in (r"SOFTWARE\Maxis\The Sims 4", r"SOFTWARE\WOW6432Node\Maxis\The Sims 4"):
            folder = _registry(winreg.HKEY_LOCAL_MACHINE, path, "Install Dir")
            if folder:
                found.append(os.path.normpath(folder))
    except ImportError:
        pass
    found += EA_FOLDERS
    unique = []
    for f in found:
        if f.lower() not in (u.lower() for u in unique):
            unique.append(f)
    return unique


def find_sims():
    """Every folder holding The Sims 4 (passing check)."""
    return [f for f in candidates() if not check(f)]


def check(sims_dir):
    """Problems with `sims_dir` as The Sims 4's folder (the PC game with its client data)."""
    if not sims_dir or not os.path.isdir(sims_dir):
        return [f"{sims_dir} is not a folder (The Sims 4's folder is the one with Game and Data in it)"]
    problems = []
    if not os.path.isfile(os.path.join(sims_dir, EXE)):
        problems.append(f"{EXE} is missing in {sims_dir}: not The Sims 4's PC folder (the one with Game and Data "
                        "in it), or the game is not fully installed")
    client = os.path.join(sims_dir, CLIENT)
    if not glob.glob(os.path.join(client, "ClientFullBuild*.package")):
        problems.append(f"{CLIENT}\\ClientFullBuild0.package is missing in {sims_dir}: the game is not fully "
                        "installed (let Steam or the EA app finish downloading it)")
    if not os.path.isfile(os.path.join(client, "UI.package")):
        problems.append(f"{CLIENT}\\UI.package is missing in {sims_dir}")
    return problems


def version(sims_dir):
    """The game's version from a pack's Version.ini ("1.128.90.1030"), or None."""
    best = None
    for ini in glob.glob(os.path.join(sims_dir, "Delta", "*", "Version.ini")) + \
            glob.glob(os.path.join(sims_dir, "*", "Version.ini")):
        try:
            with open(ini, encoding="utf-8", errors="replace") as f:
                m = re.search(r"packversion\s*=\s*([\d.]+)", f.read())
        except OSError:
            continue
        if m:
            v = tuple(int(x) for x in m.group(1).split("."))
            best = max(best or v, v)
    return ".".join(map(str, best)) if best else None


def resource_cfg(folder):
    """[(priority, glob pattern)] of a Resource.cfg, the console-only "Select" blocks left out."""
    rules = []
    priority, skipping = 0, False
    try:
        with open(os.path.join(folder, "Resource.cfg"), encoding="utf-8", errors="replace") as f:
            lines = f.read().splitlines()
    except OSError:
        return rules
    for line in lines:
        words = line.split(None, 1)
        if not words:
            continue
        word = words[0].lower()
        if word == "select":
            skipping = True
        elif word == "end":
            skipping = False
        elif skipping:
            continue
        elif word == "priority" and len(words) > 1:
            priority = int(words[1])
        elif word == "packedfile" and len(words) > 1:
            rules.append((priority, words[1].strip()))
    return rules


def _ordered(folder, rules, part=None):
    """The .package files of one folder lowest priority first: by its Resource.cfg's rules (else
    UI and thumbnails, then full builds, then deltas); within a priority as the rules list them.
    `part` "Client" or "Simulation" picks that part's files of a pack folder (which has both)."""
    files = sorted(os.path.basename(p) for p in glob.glob(os.path.join(folder, "*.package")))
    if part == "Client":
        files = [f for f in files if not f.lower().startswith("simulation")]
    elif part == "Simulation":
        files = [f for f in files if f.lower().startswith("simulation")]
    ranked = []
    for n, name in enumerate(files):
        rank = None
        for i, (priority, pattern) in enumerate(rules):
            if fnmatch.fnmatch(name.lower(), pattern.lower()):
                rank = (priority, i, n)
                break
        if rank is None:
            low = name.lower()
            rank = (-20 if "delta" in low else -40 if low.startswith(("ui", "thumbnails", "magalog")) else -30,
                    len(rules), n)
        ranked.append((rank, os.path.join(folder, name)))
    return [path for _rank, path in sorted(ranked)]  # a higher priority wins, so it comes later


def packs(sims_dir):
    """The pack codes present (installed or with delta updates), in load order: EP01.., GP.., SP.., FP.."""
    codes = set()
    for parent in (sims_dir, os.path.join(sims_dir, "Delta")):
        if os.path.isdir(parent):
            codes |= {d.upper() for d in os.listdir(parent) if re.fullmatch(r"(EP|GP|SP|FP)\d+", d, re.I)}
    return sorted(codes, key=lambda c: (PACK_ORDER.index(c[:2]), int(c[2:])))


def load_order(sims_dir, part="Client", deltas_of_missing_packs=True):
    """The packages of `part` ("Client" or "Simulation") in the order the game loads them: lowest
    priority first, so where several hold a resource the last one's copy is the game's. The base
    game (Data\\<part>, by its Resource.cfg), then each pack: its own folder, then its Delta folder.
    `deltas_of_missing_packs`: also the Delta folders of packs that are not installed (they hold
    only those packs' updates)."""
    base = os.path.join(sims_dir, "Data", part)
    order = _ordered(base, resource_cfg(base))
    for code in packs(sims_dir):
        own = os.path.join(sims_dir, code)
        if not os.path.isdir(own) and not deltas_of_missing_packs:
            continue
        for folder in (own, os.path.join(sims_dir, "Delta", code)):
            if os.path.isdir(folder):
                order += _ordered(folder, resource_cfg(folder), part)
    return order


DEFAULT_SIMS = next(iter(find_sims()), os.path.join(STEAM_FOLDER, "steamapps", "common", "The Sims 4"))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--sims", default=DEFAULT_SIMS, help="The Sims 4's folder")
    sub = parser.add_subparsers(dest="cmd")
    sub.add_parser("info")
    p = sub.add_parser("packages")
    p.add_argument("--simulation", action="store_true", help="the simulation's packages instead of the client's")
    args = parser.parse_args()
    if args.cmd == "packages":
        for path in load_order(args.sims, "Simulation" if args.simulation else "Client"):
            print(f"{os.path.getsize(path) / 1048576:>9.1f} MB  {os.path.relpath(path, args.sims)}")
        return
    print(f"candidates: {', '.join(candidates())}")
    problems = check(args.sims)
    print(f"The Sims 4 at {args.sims}: " + ("; ".join(problems) if problems else "OK"))
    if not problems:
        installed = [c for c in packs(args.sims) if os.path.isdir(os.path.join(args.sims, c))]
        print(f"  version {version(args.sims)}, packs installed: "
              f"{', '.join(installed) or 'none (the base game)'}, "
              f"{len(load_order(args.sims))} client packages")


if __name__ == "__main__":
    main()
