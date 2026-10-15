"""Read the player's own Minecraft Java Edition files: the client jar's assets and the sounds.

The Minecraft folder (%APPDATA%\\.minecraft) holds each version in versions\\<id>\\<id>.json, with
the client in <id>.jar next to it once the launcher has run that version. A modded version
(Fabric) names its vanilla version in "inheritsFrom" and may keep the vanilla client under its own
name. The jar has the textures (assets/minecraft/textures/...), the block and item models (JSON)
and the languages; the sounds are in assets\\objects\\<2 hex>\\<sha1>, found through the version's
asset index (assets\\indexes\\<id>.json, "minecraft/sounds/<name>.ogg" -> hash).

  python mcfiles.py [--minecraft DIR] [list <prefix> | extract <path> <out>]
"""
import argparse
import json
import os
import sys
import zipfile

DEFAULT_MINECRAFT = os.path.join(os.environ.get("APPDATA", ""), ".minecraft")
REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))  # mashups/minecraft/tools -> the repo


class MinecraftFiles:
    """The newest client jar with assets in a Minecraft folder, and the sounds of its asset index."""

    def __init__(self, minecraft_dir):
        self.dir = minecraft_dir
        self.version, jar, index = self._client()
        self.jar = zipfile.ZipFile(jar)
        self.jar_path = jar
        with open(index, encoding="utf-8") as f:
            self.objects = json.load(f)["objects"]

    def _client(self):
        """(version id, jar path, asset index path) of the newest version whose client jar and asset
        index are both on disk."""
        versions = os.path.join(self.dir, "versions")
        found = []
        for vid in os.listdir(versions) if os.path.isdir(versions) else []:
            jar = os.path.join(versions, vid, vid + ".jar")
            meta = os.path.join(versions, vid, vid + ".json")
            if not (os.path.exists(jar) and os.path.exists(meta)):
                continue
            with open(meta, encoding="utf-8") as f:
                info = json.load(f)
            base = info
            if "assetIndex" not in info and info.get("inheritsFrom"):  # Fabric and the like
                parent = os.path.join(versions, info["inheritsFrom"], info["inheritsFrom"] + ".json")
                if not os.path.exists(parent):
                    continue
                with open(parent, encoding="utf-8") as f:
                    base = json.load(f)
            index = os.path.join(self.dir, "assets", "indexes", base.get("assetIndex", {}).get("id", "") + ".json")
            with zipfile.ZipFile(jar) as z:
                has_assets = any(n.startswith("assets/minecraft/textures/") for n in z.namelist())
            if has_assets and os.path.exists(index):
                found.append((base.get("releaseTime", ""), base.get("id", vid), jar, index))
        if not found:
            raise FileNotFoundError(f"no Minecraft client with its assets in {versions}: start the game once in "
                                    "the launcher, so it downloads them")
        _, version, jar, index = max(found)
        return version, jar, index

    def names(self, prefix):
        return [n for n in self.jar.namelist() if n.startswith(prefix)]

    def read(self, path):
        """A file of the jar, e.g. "assets/minecraft/textures/entity/player/wide/steve.png"."""
        return self.jar.read(path)

    def sound(self, name):
        """A sound's OGG bytes, e.g. "mob/creeper/say1" (assets index "minecraft/sounds/<name>.ogg")."""
        return self.asset(f"sounds/{name}.ogg")

    def asset(self, path):
        """A file of the asset index, e.g. "lang/pl_pl.json" (the languages but English are there)."""
        h = self.objects[f"minecraft/{path}"]["hash"]
        with open(os.path.join(self.dir, "assets", "objects", h[:2], h), "rb") as f:
            return f.read()

    def language(self, code):
        """The translations of a language ("pl_pl"; English from the jar)."""
        if code == "en_us":
            return json.loads(self.read("assets/minecraft/lang/en_us.json"))
        return json.loads(self.asset(f"lang/{code}.json"))


def check(minecraft_dir):
    """Problems with `minecraft_dir` as the Minecraft folder."""
    try:
        MinecraftFiles(minecraft_dir)
    except (FileNotFoundError, OSError, KeyError, ValueError, zipfile.BadZipFile) as e:
        return [f"{e} (the Minecraft folder is the one with versions and assets in it, usually %APPDATA%\\.minecraft)"]
    return []


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--minecraft", default=DEFAULT_MINECRAFT, help="the Minecraft folder")
    sub = parser.add_subparsers(dest="cmd")
    sub.add_parser("info")
    ls = sub.add_parser("list")
    ls.add_argument("prefix", nargs="?", default="assets/minecraft/")
    ex = sub.add_parser("extract")
    ex.add_argument("path")
    ex.add_argument("out")
    args = parser.parse_args()
    mc = MinecraftFiles(args.minecraft)
    if args.cmd == "list":
        print("\n".join(mc.names(args.prefix)))
    elif args.cmd == "extract":
        out = os.path.abspath(args.out)
        if os.path.commonpath([out, REPO]) == REPO:
            sys.exit(f"{out} is inside the repository; game data must stay outside it")
        with open(out, "wb") as f:
            f.write(mc.read(args.path) if args.path.startswith("assets/") else mc.sound(args.path))
    else:
        sounds = sum(1 for k in mc.objects if k.startswith("minecraft/sounds/"))
        print(f"Minecraft {mc.version}: {mc.jar_path}, {len(mc.jar.namelist())} files in the jar, {sounds} sounds")


if __name__ == "__main__":
    main()
