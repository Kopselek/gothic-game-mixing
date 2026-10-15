# Gothic × The Witcher 3

Play **Gothic 1** as **Geralt of Rivia** from The Witcher 3: Wild Hunt. You get his model, face,
animations and voice, plus The Witcher 3's controls, camera, sword fighting and signs.

This is one of the Gothic 1 mashups in [gothic-game-mixing](../../README.md). Install it with
`GothicMashupInstaller.exe` by choosing **The Witcher 3** (see
[Install](../../README.md#install)). The installer reads **your own** Witcher 3 installation and
builds Geralt, his animations, face and voice straight into **your own** Gothic folder.

> Not affiliated with or endorsed by CD PROJEKT RED, Piranha Bytes or THQ Nordic. All game content
> belongs to its owners.

## What you need

* **The Witcher 3: Wild Hunt** for PC (Steam or GOG), installed while you install the mod. The
  mod was made and tested with the next-gen version (4.0 and later); the classic 1.32 reads too
  but isn't tested. The installer needs `content\content0` with its bundles and `texture.cache`.
  Pick the folder that contains `bin` and `content`.
* The language Geralt speaks: one of the dubs your Witcher 3 has, Polish by default.
* Optional: **ffmpeg** for Geralt's voice. Without it, Gothic's hero keeps his own voice.

## Start the game

Start the game with **`Start Gothic as Geralt.bat`** in your Gothic folder, not with Steam's Play
button. `Start Gothic as Geralt (window).bat` starts it in a window.

* The **first start takes longer**, because Gothic compiles Geralt's model and animations. The
  loading screen may stop responding for a while.
* Later starts skip that step. With it, Gothic would also try to rebuild some of its own monsters
  from sources the game doesn't ship.

## What the installer changes

* It backs up the files it edits first: `Gothic.dat` → `Gothic.dat.orig`,
  `Gothic.ini` → `Gothic.ini.before-geralt`, and the files Union replaces →
  `system\_backup_before_union`.
* It builds Geralt from your Witcher 3 files into `_work\DATA` (model, face, textures,
  animations) and `Data\Geralt_*.vdf` (voice), plus `Witcher_*.txt` in the Gothic folder.
* It makes Gothic's hero Geralt and gives the hero's dialogue lines Geralt's voice. Where the hero
  tries to give his name to Diego and Xardas, Geralt gives his.
* In `Gothic.ini` it adds the `[WITCHER]` settings. Your own values stay when you install again.
* It copies the plugin to `Data\Witcher.vdf` and creates the two start files.

## Uninstall by hand

1. Delete `Data\Witcher.vdf`, `Data\Geralt_Voice.vdf`, `Data\Geralt_Dialogue.vdf` and the two
   `Start Gothic as Geralt` files.
2. Copy `system\Gothic.ini.before-geralt` over `system\Gothic.ini`.
3. Copy `_work\DATA\scripts\_compiled\Gothic.dat.orig` over `Gothic.dat` in the same folder. On
   Steam you can instead use Gothic 1 → Properties → Installed Files → **Verify integrity of game
   files**, which also puts Gothic's own DLLs back in place of Union's.

The rest of the mod's files in `_work` are only used by the mod and do no harm.
`python tools\install.py --uninstall` removes them too.

## What it does

* **Geralt as the hero:** his Witcher 3 skeleton, skinned body and starting gear, coloured as in
  the game (matched to its inventory icons).
* **His face moves:** The Witcher 3's own face rig and poses. He talks while he speaks, blinks,
  grimaces when hit, and shows the expressions Gothic's scripts ask for.
* **His voice:** his lines in fights and while exploring, and his own recorded lines in place of
  the hero's in dialogues.
* **Movement as in the game:** his walk, run and sprint, with The Witcher 3's starts, stops,
  180-degree turns and pivots, and a camera that swings in behind him.
* **Sword fighting:** fast and strong swings as the game's combat picks them (by distance and
  stance, with gap closers), dodges, rolls, guard, parry and counter.
* **Signs:** Aard, Yrden, Igni, Quen and Axii with his casting animations.
* **Dialogue gestures:** Geralt's own gestures from the game's scenes while he talks.

## Controls

The Witcher 3's PC defaults:

| Key | Action |
| --- | --- |
| W/A/S/D | move, relative to the camera |
| Mouse | camera |
| Left Shift | sprint |
| Left Ctrl | walk / run |
| Space | jump; with the sword out, roll |
| E | talk, take, open, use |
| 1 / 2 | draw the sword (steel / silver) |
| C | sheathe the sword |
| Left mouse | fast swing (draws the sword if it's away) |
| Left Shift + left mouse | strong swing |
| Alt, or a direction key tapped twice | dodge |
| Right mouse (hold) | guard; press it just before a hit to counter |
| Z | lock on a target |
| Q | cast the chosen sign |
| 3 / 4 / 5 / 6 / 7 | choose Aard / Yrden / Igni / Quen / Axii |

Gothic's menu keys stay the same: Tab for the inventory, B for status, and its log and map keys.

## Settings

All settings are in `Gothic.ini`, section `[WITCHER]`, each with a short note. Some useful ones:

* `mouseSpeed`, `invertMouseY`, `camDistance`, `camAutoCenter`: camera feel.
* `turnRate`, `turnRateRun`, `turnRateSprint`, `gameTurns`: how Geralt turns.
* `fastDamage`, `strongDamage`, `killHumans`: sword damage, and whether beaten people die.
* `face`, `voice`, `voiceSubtitles`, `voiceVolume`: his face and voice.
* `controls=0`: Gothic's own controls and combat with Geralt's look.
* `debugLog=1`: writes `Witcher.log` in the Gothic folder. Attach it to bug reports.

## Known limitations

* Geralt fights with Gothic's one-handed sword items: his own steel and silver swords are not
  items yet, and the silver sword does no extra damage to monsters.
* Gothic's armour would replace his body, so the hero can't put armour on.
* There is no Witcher 3 HUD yet; Gothic's bars stay.

## How it works

The converters in `tools/` read The Witcher 3's resources from your install (bundles, then the
meshes, rigs, face rig, animations, textures and sounds inside them) and write Gothic's formats
with the shared Gothic tools (`../../tools`). The Union plugin in `plugin/src/Plugin.ipp` brings
The Witcher 3's mechanics into Gothic's engine: controls, camera, movement, combat, signs, face
and voice. The plan and phases are in [PLAN.md](PLAN.md).

| File | Use |
| --- | --- |
| `tools/mashup_witcher.py` | What the installer does for this mashup: settings, steps, files, start files |
| `tools/make_geralt_model.py` | Geralt's skeleton (with the face rig), skinned body and textures |
| `tools/make_geralt_anims.py` | His animations, face animations and turns; the hero pointed at him in `Gothic.dat` |
| `tools/make_geralt_voice.py`, `tools/make_geralt_dialogue.py` | His voice lines and his voice in the dialogues |
| `tools/w3bundle.py` | Read The Witcher 3's `.bundle` archives (classic and next-gen layouts): `scan`, `list`, `extract`, or read files straight from them (`W3Install`) |
| `tools/cr2w.py` | Read CR2W resources (`.w2mesh`, `.w2ent`, `.w2rig`, `.w2anims`, `.w3fac`, ...) |
| `tools/w3model.py`, `tools/w3anim.py`, `tools/w3mimic.py` | Skeletons and skinned meshes, skeletal animations with root motion, the face rig and its poses |
| `tools/w3texture.py` | Read `texture.cache` and decode BC1/BC2/BC3 textures |
| `tools/w3speech.py`, `tools/w3strings.py`, `tools/w3voicelines.py` | Speech, localized text and Geralt's lines in the scenes |
| `plugin/src/Plugin.ipp` | The Witcher plugin: controls, camera, movement, combat, signs, face, voice |

Each tool runs on its own too: `python mashups\witcher\tools\make_geralt_model.py --help`.
Extracted files are your game's data. Keep them outside the repository (`w3bundle.py extract`
refuses to write inside it).
