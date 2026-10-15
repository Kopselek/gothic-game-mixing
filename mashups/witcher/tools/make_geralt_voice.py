"""Give Geralt in Gothic his own voice from The Witcher 3: the lines he says by himself.

They come from his voiceset (quests\\main_npcs\\voicesets\\geralt_vset.w2scene), the scene the
game's scripts play one-liners from (r4Player.ws: PlayBattleCry, PlayVoiceset), by its input
names, in the game's own dub (<lang>pc.w3speech, Polish by default):
  combat   a fight starts (BattleCryHumans/MonstersStart, BattleCryTaunt)
  hit      he lands a blow (BattleCry...Hit)
  kill     the fight is over (BattleCry...End)
           (each of these with _human or _monster when the game says it only to people or only
           to monsters)
  hurt     it goes badly (BattleCryBadSituation, Swears)
  attack   effort grunts on a strong swing (BattleCryAttack)
  explore  remarks on the way and on what he finds (MiscGenericRemarks, MiscInvestigateArea,
           MiscNothingInteresting, the travel lines, the portal lines)
Lines about his horse, monster nests, the weather and The Witcher's own quest items are left out:
Gothic has none of them. The Witcher plugin plays them (see Plugin.ipp, "Geralt's voice").

Writes Data\\Geralt_Voice.vdf (_WORK\\DATA\\SOUND\\SFX\\GERALT_VO_<CATEGORY>_<NN>.WAV, 44.1 kHz mono
16-bit) and Witcher_voice.txt in the Gothic folder (category, file, seconds, subtitle; the
subtitle in the dub's language without diacritics, which Gothic's fonts do not have).

usage: python make_geralt_voice.py --w3 "<Witcher 3 folder>" [--gothic DIR] [--lang pl]
Needs ffmpeg on PATH (w3speech.ogg_to_wav).
"""
import argparse
import os
import sys
import tempfile
import unicodedata
from concurrent.futures import ThreadPoolExecutor

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
from vdf import write_vdf
from w3bundle import W3Install
from w3speech import W3Speech, ogg_to_wav, wem_to_ogg
from w3strings import W3Strings
from w3voicelines import scene_lines

DEFAULT_GOTHIC = r"C:\Program Files (x86)\Steam\steamapps\common\Gothic"
DEFAULT_W3 = r"C:\Program Files (x86)\Steam\steamapps\common\The Witcher 3"
VOICESET = "quests\\main_npcs\\voicesets\\geralt_vset.w2scene"
OUTPUT_NAME, MANIFEST = "Geralt_Voice.vdf", "Witcher_voice.txt"
# category by voiceset input, in order: a line under several inputs takes the first that matches
CATEGORIES = [
    ("combat", ("BattleCryHumansStart", "BattleCryMonstersStart", "BattleCryTaunt", "BadIdeaIn")),
    ("kill", ("BattleCryHumansEnd", "BattleCryMonstersEnd")),
    ("hit", ("BattleCryHumansHit", "BattleCryMonstersHit", "BattleCryMonstersSilverHit")),
    ("hurt", ("BattleCryBadSituation", "Swears")),
    ("attack", ("BattleCryAttack",)),
    ("explore", ("MiscGenericRemarks", "MiscCommentInteresting", "MiscCommentFound", "MiscInvestigateArea",
                 "MiscNothingInteresting", "MiscNothingInterestingHere", "geralt_leaves_map_1",
                 "geralt_leaves_map_2", "geralt_leaves_map_3", "")),  # "": the travel lines (no input)
]
# lines (by their English text) that only fit The Witcher: his horse, nests, girls, quest items
EXCLUDE = ("roach", "nest", "women", "grapeshot", "dancing star", "bomb", "arachnomorph")
MAX_SECONDS = 4.0


def ascii_text(text):
    """Gothic's fonts have no Polish letters: the subtitle without diacritics."""
    text = text.replace("ł", "l").replace("Ł", "L")
    return "".join(c for c in unicodedata.normalize("NFKD", text) if not unicodedata.combining(c))


def choose(lines, speech, english):
    """[(category, string id, seconds)] of Geralt's voiceset lines that fit Gothic."""
    chosen, seen = [], set()
    for line in lines:
        sid = line["id"]
        if line["voicetag"] != "GERALT" or sid in seen or sid not in speech.entries:
            continue
        inputs = line["inputs"] or {""}
        category = next((c for c, names in CATEGORIES if inputs & set(names)), None)
        seconds = speech.duration(sid)
        if not category or not 0.3 <= seconds <= MAX_SECONDS:
            continue
        if any(word in (english.get(sid) or "").lower() for word in EXCLUDE):
            continue
        # combat and hit lines meant only for people or only for monsters get their own category
        # (the plugin picks by whom he fights): "combat_human", "hit_monster", ...
        if category in ("combat", "hit", "kill"):
            human = any("Humans" in n for n in inputs)
            monster = any("Monsters" in n for n in inputs)
            if human != monster:
                category += "_human" if human else "_monster"
        seen.add(sid)
        chosen.append((category, sid, seconds))
    return chosen


def build(w3_dir, gothic_dir, lang="pl"):
    w3 = W3Install(w3_dir)
    content = os.path.join(w3_dir, "content", "content0")
    speech = W3Speech(os.path.join(content, f"{lang}pc.w3speech"))
    text = W3Strings(os.path.join(content, f"{lang}.w3strings"))
    english = W3Strings(os.path.join(content, "en.w3strings"))
    chosen = choose(scene_lines(w3, VOICESET), speech, english)
    count = {}
    names = []
    for category, sid, seconds in chosen:
        count[category] = count.get(category, 0) + 1
        names.append(f"GERALT_VO_{category.upper()}_{count[category]:02d}.WAV")
    with tempfile.TemporaryDirectory() as tmp:
        def convert(item):
            (category, sid, seconds), name = item
            path = os.path.join(tmp, name)
            ogg_to_wav(wem_to_ogg(speech.read_wem(sid)), path)
            with open(path, "rb") as f:
                return name, f.read()
        with ThreadPoolExecutor(max_workers=8) as pool:
            wavs = dict(pool.map(convert, zip(chosen, names)))
    files = {f"_WORK\\DATA\\SOUND\\SFX\\{name}": data for name, data in wavs.items()}
    write_vdf(os.path.join(gothic_dir, "Data", OUTPUT_NAME), files,
              comment=f"Geralt's voice for the Witcher mod (The Witcher 3 voiceset, {lang} dub)")
    with open(os.path.join(gothic_dir, MANIFEST), "w", encoding="ascii", errors="replace", newline="\n") as f:
        f.write(f"# Geralt's lines for the Witcher plugin (make_geralt_voice.py, {lang} dub)\n")
        for (category, sid, seconds), name in zip(chosen, names):
            subtitle = ascii_text(text.get(sid, "")).replace("\t", " ").replace("\n", " ")
            f.write(f"{category}\t{name}\t{seconds:.2f}\t{subtitle}\n")
    print(f"{OUTPUT_NAME}: {len(chosen)} lines (" + ", ".join(f"{c} {n}" for c, n in count.items()) + f"), {MANIFEST}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--w3", default=DEFAULT_W3, help="The Witcher 3 folder")
    parser.add_argument("--gothic", default=DEFAULT_GOTHIC, help="the Gothic 1 folder")
    parser.add_argument("--lang", default="pl", help="the dub: pl, en, de, ... (<lang>pc.w3speech)")
    args = parser.parse_args()
    build(args.w3, args.gothic, args.lang)


if __name__ == "__main__":
    main()
