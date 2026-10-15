"""List one speaker's voiced lines in The Witcher 3 (Geralt by default) as a TSV table.

Dialogue lives in the scene files (.w2scene, CR2W, see cr2w.py): each CStorySceneLine has a
`voicetag` (the speaker, a CName such as GERALT), a `dialogLine` (LocalizedString id), an optional
`speakingTo` voicetag and `approvedDuration`; its export parent is the CStorySceneSection that
plays it (`sectionName`, `isGameplay`, `hasCinematicOneliners` for in-game one-liners).

A scene's entry points are CStorySceneInput chunks (`inputName`). Voicesets are scenes whose
input names are what the game scripts ask for: Geralt's is quests\\main_npcs\\voicesets\\
geralt_vset.w2scene, where r4Player.ws plays e.g. PlayVoiceset(90, 'BattleCryHumansStart'); the
inputs lead (through CStorySceneLinkElement, CStorySceneRandomizer `outputs`, flow conditions
`trueLink`/`falseLink` and switches `cases`/`thenLink`/`defaultLink`) to the sections. This tool
walks those links to name each section's inputs.

Text comes from <lang>.w3strings (w3strings.py), the length in seconds from <lang>pc.w3speech
(w3speech.py). Categories are a guess from the voiceset input, the section and the scene path:
  combat_start, combat_hit, combat_taunt, combat_attack (grunts), combat_bad (losing, hurt),
  combat_end (after the fight), combat (other fight lines), horse, exploration (one-liners
  Geralt says alone in gameplay), reaction, dialogue (the rest).

  python w3voicelines.py --w3 "<Witcher 3 folder>" --out <file.tsv outside this repo>
      [--voicetag GERALT] [--lang pl] [--ref-lang en]
"""
import argparse
import collections
import os
import re
import sys

from cr2w import CR2W
from w3bundle import W3Install
from w3speech import W3Speech
from w3strings import W3Strings

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))  # mashups/witcher/tools -> the repo
LINKS = ("nextLinkElement", "outputs", "trueLink", "falseLink", "cases", "thenLink", "defaultLink")
NO_LISTENER = (None, "None", "NONE")
ORDER = ["combat_start", "combat_hit", "combat_taunt", "combat_attack", "combat_bad", "combat_end",
         "combat", "exploration", "horse", "reaction", "dialogue"]


def section_inputs(scene):
    """{section chunk index: set of input names that reach it}."""
    reach = collections.defaultdict(set)
    for chunk in scene.of_type("CStorySceneInput"):
        name = chunk.get("inputName")
        if not name:
            continue
        seen, todo = set(), [chunk]
        while todo:
            c = todo.pop()
            if c.index in seen:
                continue
            seen.add(c.index)
            if c is not chunk and "sectionName" in c.props:
                reach[c.index].add(name)
                continue
            for key in LINKS:
                value = c.get(key)
                for h in (value if isinstance(value, list) else [value]):
                    target = scene.ref(h) if isinstance(h, int) else None
                    if target is not None:
                        todo.append(target)
    return reach


def scene_lines(install, path):
    scene = CR2W(install.read(path), path, install)
    inputs = section_inputs(scene)
    lines = []
    for chunk in scene.chunks:
        if "dialogLine" not in chunk.props or "voicetag" not in chunk.props:
            continue
        section = chunk
        while section.parent and "sectionName" not in section.props:
            section = scene.chunks[section.parent - 1]
        lines.append(dict(scene=path, chunk=chunk, section=section, inputs=inputs.get(section.index, set()),
                          voicetag=chunk.get("voicetag"), id=chunk.get("dialogLine")))
    return lines


COMBAT = [  # voiceset input name (BattleCry...) or voiceset section name pattern -> category
    (r"BadSituation|battle_?bad", "combat_bad"), (r"(Humans|Monsters)End|_end_?\d|battlecry_end", "combat_end"),
    (r"Taunt", "combat_taunt"), (r"Attack", "combat_attack"), (r"Start", "combat_start"), (r"Hit", "combat_hit"),
    (r"BattleCry", "combat"),
]


def categorize(line, solo):
    section = line["section"].get("sectionName") or ""
    battle_inputs = " ".join(sorted(n for n in line["inputs"] if n.startswith("BattleCry")))
    voiceset = "voiceset" in line["scene"]
    for pattern, category in COMBAT:
        if re.search(pattern, battle_inputs, re.I) or (voiceset and re.search(pattern, section, re.I)):
            return category
    names = f"{section} {' '.join(sorted(line['inputs']))}".lower()
    if re.search(r"horse|horsie|roach", names):
        return "horse"
    gameplay = line["section"].get("isGameplay") or line["section"].get("hasCinematicOneliners")
    if solo and (gameplay or voiceset) and re.search(r"combat|fight|battle", names):
        return "combat"
    if solo and re.search(r"reaction|react", f"{line['scene']} {names}".lower()):
        return "reaction"
    if solo and (gameplay or voiceset or re.search(r"oneliner|onliner|comment|gmpl|_gp_|focus|clue",
                                                     line["scene"].lower())):
        return "exploration"
    return "dialogue"


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--w3", required=True, help="the Witcher 3 folder")
    parser.add_argument("--out", required=True, help="the .tsv to write, outside this repository")
    parser.add_argument("--voicetag", default="GERALT")
    parser.add_argument("--lang", default="pl")
    parser.add_argument("--ref-lang", default="en", help="a second language for the text column")
    args = parser.parse_args(argv)
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    out = os.path.abspath(args.out)
    if os.path.commonpath([out, REPO]) == REPO:
        sys.exit(f"--out {out} is inside the repository; game data must stay outside it")
    content = os.path.join(args.w3, "content", "content0")
    text = W3Strings(os.path.join(content, f"{args.lang}.w3strings"))
    ref = W3Strings(os.path.join(content, f"{args.ref_lang}.w3strings"))
    speech = W3Speech(os.path.join(content, f"{args.lang}pc.w3speech"))
    install = W3Install(args.w3)
    scenes = sorted(k for k in install.entries if k.endswith(".w2scene"))
    print(f"{len(scenes)} scenes")
    rows, speakers = {}, collections.defaultdict(set)
    found = []
    for path in scenes:
        try:
            lines = scene_lines(install, path)
        except (ValueError, NotImplementedError) as err:
            print(f"  skipped {path}: {err}")
            continue
        for line in lines:
            speakers[(path, line["section"].index)].add(line["voicetag"])
        found += [line for line in lines if line["voicetag"] == args.voicetag]
    for line in found:
        sid = line["id"]
        solo = (speakers[(line["scene"], line["section"].index)] == {args.voicetag}
                and line["chunk"].get("speakingTo") in NO_LISTENER + (args.voicetag,))
        category = categorize(line, solo)
        if sid in rows and ORDER.index(rows[sid]["category"]) <= ORDER.index(category):
            continue  # a line used in several scenes keeps its most specific category
        rows[sid] = dict(id=sid, scene=line["scene"], section=line["section"].get("sectionName") or "",
                         inputs=",".join(sorted(line["inputs"])), category=category, solo=int(solo),
                         listener=line["chunk"].get("speakingTo") or "")
    seconds = speech.durations([sid for sid in rows if sid in speech.entries])
    clean = lambda s: (s or "").replace("\t", " ").replace("\r", " ").replace("\n", " ")
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write("string_id\tseconds\tscene\tsection\tvoiceset_input\tcategory\tsolo\tlistener\t"
                f"{args.lang}\t{args.ref_lang}\n")
        for r in sorted(rows.values(), key=lambda r: (ORDER.index(r["category"]), r["scene"], r["id"])):
            sec = seconds.get(r["id"])
            f.write(f"{r['id']}\t{'' if sec is None else f'{sec:.2f}'}\t{r['scene']}\t{clean(r['section'])}\t"
                    f"{r['inputs']}\t{r['category']}\t{r['solo']}\t{r['listener']}\t"
                    f"{clean(text.get(r['id']))}\t{clean(ref.get(r['id']))}\n")
    counts = collections.Counter(r["category"] for r in rows.values())
    voiced = collections.Counter(r["category"] for r in rows.values() if r["id"] in seconds)
    short = collections.Counter(r["category"] for r in rows.values()
                                if r["id"] in seconds and 0.5 <= seconds[r["id"]] <= 4.0)
    print(f"{len(found)} {args.voicetag} lines, {len(rows)} distinct string ids, {len(seconds)} voiced -> {out}")
    print(f"  {'category':<14}{'lines':>7}{'voiced':>8}{'0.5-4 s':>9}")
    for c in ORDER:
        print(f"  {c:<14}{counts[c]:>7}{voiced[c]:>8}{short[c]:>9}")


if __name__ == "__main__":
    main()
