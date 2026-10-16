# Plan: The Witcher 3 in Gothic 1

The idea in one sentence: **play Gothic 1 as Geralt of Rivia**, with his model, animations and
voice, The Witcher 3's controls, camera, sword fighting, signs and HUD, and Roach.

It is the same kind of mashup as the GTA San Andreas one (`mashups/gta`, Gothic 1 as CJ). In universal-modder's terms
(`mashup-mods`), it is **pattern 1, porting the content**. Gothic 1 is the host and runs alone.
The Witcher 3 is never started, patched or hooked. Converters read the player's own Witcher 3
files at install time and write Gothic formats into the player's Gothic folder, and a Union
plugin brings the mechanics.

## Feature map

| Witcher 3 | In Gothic 1 | Built like (`mashups/gta`) |
|---|---|---|
| Geralt: meshes, skeleton, textures | the hero's model | CJ's model (`make_cj_model.py`: ASC skeleton, soft-skin mesh, MDS) |
| Geralt's animations | Gothic's animation names (walk, run, jump, climb, swim, fight, die, item use, dialogue gestures) | CJ's GTA animations mapped onto Gothic's MDS |
| Geralt's voice (English or Polish) | the hero's dialogue lines (voice 15) | `make_cj_voice.py` |
| Exploration controls and camera: camera-relative movement, sprint, jump and climb | hero AI override and follow camera | CJMovement controls and camera |
| Combat: steel sword for people, silver sword for monsters, fast and strong attacks, dodge, roll, parry, counter, lock-on | Gothic's one- and two-handed fighting, the plugin's damage rules | CJ's fists and dildo combos, the `OnDamage_Hit` damage rule |
| Signs: Aard, Igni, Yrden, Quen, Axii, with stamina | effects from Gothic's own particle effects, damage, knock-down and AI states | rocket blast (knock-down, fire), dildo (flee states) |
| HUD: vitality, stamina, toxicity, adrenaline, minimap, buffs | drawn over Gothic's view | GTA HUD and radar |
| Witcher senses, the medallion | highlighted objects and NPCs, tinted view, medallion trembling near magic | new |
| Roach | a ridden object with her model, animations and sounds | the Sanchez bike |
| Swords, armour, potions, toxicity, meditation | items patched into Gothic.dat (`datpatch.py`), plugin rules | the pistol, rocket launcher and dildo items |

## Scope (decided with the user, 2026-10-12)

- Geralt is the hero, and his mechanics come straight from The Witcher 3: his sword attacks, his
  two swords (steel and silver), his HUD (Gothic's is removed) and his animations, which replace
  the hero's.
- The Witcher 3's combat has to work in Gothic 1 with The Witcher 3's controls.
- That is the whole requirement. Roach, the voice, alchemy and meditation are extras for later.

## Pipeline

```
Witcher 3 install
  content\content*\bundles\*.bundle, dlc\*, patch folders   -> w3bundle.py (done, see MODLOG)
     CR2W resources: .w2ent .w2mesh .w2rig .w2anims .xbm     -> cr2w.py (next)
  content*\texture.cache                                     -> texture data of the .xbm files
  content*\soundspc.cache, *.w3speech                        -> Wwise sounds and speech
       |
       v  converters (tools\make_geralt_*.py), run on the player's PC
Gothic 1 formats: ASC (skeleton, mesh, animations) + MDS, -C.TEX, .3DS, WAV, Gothic.dat patches,
packed into Data\W3_*.vdf (vdf.py)
```

Gothic-side writers (`asc.py`, `tex.py`, `mesh3ds.py`, `wav.py`, `vdf.py`, `daedalus.py`,
`datpatch.py`) are shared by every mashup in `tools/`; each file's docstring documents its format.

## Phases

Each phase ends with the result seen in game, a log and a screenshot (the universal-modder loop:
one vertical slice first, then widen).

0. **Recon.** Done. The game is the next-gen edition (4.04). MODLOG.md has the findings. Still
   to do: a Gothic test install without the CJ mod.
1. **Formats.** Done and checked offline: bundles (`w3bundle.py`), CR2W (`cr2w.py`), meshes and
   the rig (`w3model.py`), texture.cache (`w3texture.py`) and animations with root motion
   (`w3anim.py`).
2. **Geralt's body.** Done offline. `make_geralt_model.py` writes his skeleton and skinned mesh
   (the Viper set, head, hair, beard) with one texture sheet. The preview renders of the bind
   pose and of sword poses are correct. Not yet seen in game.
3. **Animation set.** Next: `make_geralt_anims.py` (GERALT.MDS and the ASC animations, Gothic's
   names mapped onto Geralt's exploration, movement and sword sets), and the hero patched to
   GERALT. Then the first in-game test.
4. **Controls and camera.** The plugin: Witcher 3 exploration movement and camera.
5. **Combat.** Two swords, fast and strong attacks, dodge, roll, parry, damage rules.
6. **Signs.**
7. **HUD, witcher senses, medallion.**
8. **Roach.**
9. **Voice, items, alchemy, meditation.**
10. **Installer and releases.** Done: the mashups share one GUI installer (`GothicMashupInstaller.exe`,
    choose The Witcher 3) and one GitHub Actions release, in the gothic-game-mixing monorepo.

## Risks and open questions

- **Armour colours.** Tinted materials take their colour from a hue, saturation and luminance
  shift whose shader isn't readable. The converter approximates it, and the result still has to
  be compared with the game.
- **Animation events.** Hit frames and combo windows live in the animation sets' event data, not
  in the bone tracks. `research/combat.md` has them for the sword set, and they need reading in
  code.
- **Polygon count.** Geralt is 32,737 vertices and 45,534 triangles at LOD0. If Gothic 1's
  engine struggles, use LOD1.
- **Hair.** HairWorks fur is replaced by the plain fallback meshes the game itself ships.
- **Sounds.** Wwise `.wem` sounds need a decoder (vgmstream or similar) before ffmpeg can write
  Gothic's WAV files.
- **Classic edition (1.32).** The tools read its bundle layout, but nothing has been tested on
  it, and its CR2W version is older.

## Rules for the project

- Only the player's own copies of both games. No Witcher 3 or Gothic files, and nothing converted
  from them, in the repository or the releases (the `.gitignore` is a whitelist).
- Extracted Witcher 3 data stays outside the repository (`w3bundle.py extract` refuses to write
  inside it).
- Back up every Gothic file before changing it (`.orig`); converters always work from the
  backup.
