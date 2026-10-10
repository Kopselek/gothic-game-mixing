"""Download Union 1.0m and install it into a Gothic 1 folder.

Union (by Gratt and the Union team) is what loads the CJMovement plugin. A Steam or GOG Gothic 1
needs nothing else: this fetches "Union 1.0m.7z" from Gratt's own archive on GitHub
(Gratt-5r2/gothic-fix-archive), checks its SHA-256, and copies it in the way that archive's notes
say for Gothic 1: Data\\Union.vdf, the System files, and the BinkMss DLLs into System as well.
Every file it replaces is saved in system\\_backup_before_union first.

  python union.py [--gothic DIR]

Needs py7zr to unpack the archive (pip install py7zr); the GUI installer has it built in.
"""
import argparse
import hashlib
import os
import shutil
import tempfile
import urllib.request

DEFAULT_GOTHIC = r"C:\Program Files (x86)\Steam\steamapps\common\Gothic"
UNION_URL = "https://raw.githubusercontent.com/Gratt-5r2/gothic-fix-archive/main/union/Union%201.0m.7z"
UNION_SHA256 = "9a5fdff7c1e8d33c0e6b9051375f2569a352b1aac835040b6ef81cb7f92cf308"
UNION_THREAD = "https://worldofplayers.ru/threads/40376/"
BACKUP_DIR = "_backup_before_union"
# archive folder -> game folder (Gothic 1 takes BinkMss into System too)
LAYOUT = [("Data", "Data"), ("System", "system"), ("BinkMss", "system")]


def installed(gothic):
    """Union's files are there and system\\Shw32.dll is its loader. Steam puts Gothic's own Shw32.dll (and
    Vdfs32g, Mss32, binkw32) back when it updates or verifies the game or switches its beta branch, and
    leaves Union.vdf and Union.patch behind: then Union, and with it the plugin, doesn't load."""
    shw32 = os.path.join(gothic, "system", "Shw32.dll")
    if not all(os.path.exists(p) for p in (os.path.join(gothic, "Data", "Union.vdf"),
                                           os.path.join(gothic, "system", "Union.patch"), shw32)):
        return False
    with open(shw32, "rb") as f:
        return b"Union.patch" in f.read()


def download(url, path):
    request = urllib.request.Request(url, headers={"User-Agent": "gothic-gtasa installer"})
    with urllib.request.urlopen(request, timeout=60) as response, open(path, "wb") as f:
        shutil.copyfileobj(response, f)


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def install(gothic):
    import py7zr  # here, so the rest of the installer works without it

    with tempfile.TemporaryDirectory() as tmp:
        archive = os.path.join(tmp, "Union 1.0m.7z")
        print(f"Union: downloading {UNION_URL}", flush=True)
        download(UNION_URL, archive)
        if sha256(archive) != UNION_SHA256:
            raise RuntimeError(f"the downloaded Union archive isn't the expected one (SHA-256 mismatch). Install Union "
                               f"1.0m for Gothic 1 yourself ({UNION_THREAD}) and run the installer again.")
        files = os.path.join(tmp, "files")
        with py7zr.SevenZipFile(archive) as z:
            z.extractall(files)

        backup = os.path.join(gothic, "system", BACKUP_DIR)
        for source, target in LAYOUT:
            for name in sorted(os.listdir(os.path.join(files, source))):
                dest = os.path.join(gothic, target, name)
                saved = os.path.join(backup, name)
                if os.path.exists(dest) and not os.path.exists(saved):
                    os.makedirs(backup, exist_ok=True)
                    shutil.copy2(dest, saved)
                os.makedirs(os.path.dirname(dest), exist_ok=True)
                shutil.copy2(os.path.join(files, source, name), dest)
                print(f"Union: {target}\\{name}")
    print(f"Union 1.0m installed (replaced files are in system\\{BACKUP_DIR})")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--gothic", default=DEFAULT_GOTHIC, help="Gothic 1 install folder")
    args = parser.parse_args()
    install(args.gothic)


if __name__ == "__main__":
    main()
