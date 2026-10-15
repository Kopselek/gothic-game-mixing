"""Minecraft's sounds for the Minecraft plugin, from the player's asset index (OGG -> WAV with ffmpeg).

Writes Data\\MC_Sounds.vdf with _WORK\\DATA\\SOUND\\SFX\\<NAME>.WAV (as the GTA mashup's weapon
sounds): the explosion (random/explode1-4), the creeper's fuse (random/fuse), and placing and
breaking blocks by their kind (dig/<kind>1-4: stone, wood, grass, gravel, sand, cloth; glass
breaking random/glass1-3), the bow's shot and its arrows' hits (random/bow, random/bowhit1-4), flint and steel (fire/ignite), fire and lava (fire/fire, liquid/lava, lavapop, random/fizz, the
lava bucket's), the Ender Dragon (mob/enderdragon: wings, growl, hit, end; its shot mob/ghast/fireball4). Without ffmpeg on PATH the plugin plays none of them.

usage: python make_mc_sounds.py [--minecraft DIR] --gothic DIR
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
from mcfiles import DEFAULT_MINECRAFT, MinecraftFiles
from vdf import write_vdf

VDF = "MC_Sounds.vdf"
DIG = ["stone", "wood", "grass", "gravel", "sand", "cloth"]
SOUNDS = {f"MC_EXPLODE{i}": f"random/explode{i}" for i in range(1, 5)}
SOUNDS["MC_FUSE"] = "random/fuse"
SOUNDS.update({f"MC_DIG_{kind.upper()}{i}": f"dig/{kind}{i}" for kind in DIG for i in range(1, 5)})
SOUNDS.update({f"MC_GLASS{i}": f"random/glass{i}" for i in range(1, 4)})
SOUNDS["MC_BOW"] = "random/bow"
SOUNDS["MC_IGNITE"] = "fire/ignite"
SOUNDS.update({"MC_FIRE": "fire/fire", "MC_LAVA": "liquid/lava", "MC_LAVAPOP": "liquid/lavapop", "MC_FIZZ": "random/fizz",
               "MC_DRAGON_END": "mob/enderdragon/end", "MC_DRAGON_SHOOT": "mob/ghast/fireball4"})
SOUNDS.update({f"MC_BUCKET_EMPTY_LAVA{i}": f"item/bucket/empty_lava{i}" for i in range(1, 4)})
SOUNDS.update({f"MC_BUCKET_FILL_LAVA{i}": f"item/bucket/fill_lava{i}" for i in range(1, 4)})
SOUNDS.update({f"MC_DRAGON_GROWL{i}": f"mob/enderdragon/growl{i}" for i in range(1, 5)})
SOUNDS.update({f"MC_DRAGON_HIT{i}": f"mob/enderdragon/hit{i}" for i in range(1, 5)})
SOUNDS.update({f"MC_DRAGON_WINGS{i}": f"mob/enderdragon/wings{i}" for i in range(1, 7)})
SOUNDS.update({f"MC_BOWHIT{i}": f"random/bowhit{i}" for i in range(1, 5)})
NO_WINDOW = getattr(subprocess, "CREATE_NO_WINDOW", 0)  # no console window per call from the GUI installer


# Minecraft plays these at another pitch than 1 (ClientPacketListener.handleExplosion: 0.7, give or
# take 0.1 at random): the WAVs have it, as Gothic's sound takes no pitch of its own here
PITCH = {f"MC_EXPLODE{i}": 0.7 for i in range(1, 5)}


def to_wav(ogg, pitch=1.0):
    """OGG bytes -> 16-bit WAV bytes (ffmpeg finalizes a WAV's sizes only on a file, not a pipe);
    pitch: played that much faster or slower (a lower or higher sound, as Minecraft's pitch)."""
    import wave
    with tempfile.TemporaryDirectory() as folder:
        out = os.path.join(folder, "out.wav")
        subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", "pipe:0", "-acodec", "pcm_s16le", out], input=ogg,
                       check=True, creationflags=NO_WINDOW)
        if pitch != 1.0:  # its own rate as if it were pitch times that, back to its rate
            with wave.open(out) as w:
                rate = w.getframerate()
            pitched = os.path.join(folder, "pitched.wav")
            subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", out, "-af", f"asetrate={rate * pitch:.0f},aresample={rate}",
                            "-acodec", "pcm_s16le", pitched], check=True, creationflags=NO_WINDOW)
            out = pitched
        with open(out, "rb") as f:
            return f.read()


def build(minecraft_dir, gothic_dir):
    if not shutil.which("ffmpeg"):
        print("ffmpeg is not on PATH: Minecraft's sounds are left out")
        return
    files = MinecraftFiles(minecraft_dir)
    out = {f"_WORK\\DATA\\SOUND\\SFX\\{name}.WAV": to_wav(files.sound(source), PITCH.get(name, 1.0))
           for name, source in SOUNDS.items()}
    write_vdf(os.path.join(gothic_dir, "Data", VDF), out, comment="Minecraft's sounds for the Minecraft mashup")
    print(f"Minecraft's sounds ({files.version}): {len(out)} into Data\\{VDF}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--minecraft", default=DEFAULT_MINECRAFT, help="the Minecraft folder (%%APPDATA%%\\.minecraft)")
    parser.add_argument("--gothic", required=True, help="the Gothic 1 or Gothic 2 folder")
    args = parser.parse_args()
    build(args.minecraft, args.gothic)


if __name__ == "__main__":
    main()
