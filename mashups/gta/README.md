# Gothic × GTA San Andreas

Play **Gothic 1** as **CJ** from GTA San Andreas. You get his model, animations and voice, plus
GTA's controls, camera and HUD, GTA weapons, and the Sanchez motorbike.

This is one of the Gothic 1 mashups in [gothic-game-mixing](../../README.md). Install it with
`GothicMashupInstaller.exe` by choosing **GTA San Andreas** (see
[Install](../../README.md#install)). The installer reads **your own** GTA San Andreas
installation and builds CJ, his animations, sounds, textures and the bike straight into
**your own** Gothic folder.

> Not affiliated with or endorsed by Rockstar Games, Take-Two Interactive, Piranha Bytes or
> THQ Nordic. All game content belongs to its owners.

## What you need

* **GTA San Andreas, classic PC version.** The Definitive Edition won't work: the installer needs
  `models\gta3.img`, `anim\ped.ifp`, `audio\SFX` and `data\handling.cfg`. Pick the folder that
  contains `models` and `anim`.
* Optional: **ffmpeg** for CJ's voice. Without it, Gothic's hero keeps his own voice.

## Start the game

Start the game with **`Start Gothic with CJ.bat`** in your Gothic folder, not with Steam's Play
button. The file runs `system\GothicMod.exe -zautoconvertdata`.

* The **first start takes longer**, because Gothic compiles CJ's model and animations.
* In the **first session**, CJ may hop or fall on slopes while the game is still compiling and
  caching. That goes away from the next start on.

## What the installer changes

* It backs up the files it edits first: `Gothic.dat` → `Gothic.dat.orig`,
  `Gothic.ini` → `Gothic.ini.before-cj`, and the files Union replaces →
  `system\_backup_before_union`.
* It builds CJ, the weapons, the HUD, the radar and the bike from your GTA files into `_work\DATA`
  and `Data\CJ_*.vdf`.
* It makes Gothic's hero CJ and turns two crossbows, the bolts and two unused items into CJ's
  weapons (see [Known limitations](#known-limitations)).
* In `Gothic.ini` it sets GTA's keys and adds the `[CJMOVEMENT]` settings. Your own values stay
  when you install again.
* It copies the plugin to `Data\CJMovement.vdf` and creates `Start Gothic with CJ.bat`.

## Uninstall by hand

1. Delete `Data\CJMovement.vdf`, `Data\CJ_Weapons.vdf`, `Data\CJ_Bike.vdf`, `Data\CJ_Voice.vdf`
   and `Start Gothic with CJ.bat`.
2. Copy `system\Gothic.ini.before-cj` over `system\Gothic.ini`. That brings back your keys and
   settings from before the mod.
3. Copy `_work\DATA\scripts\_compiled\Gothic.dat.orig` over `Gothic.dat` in the same folder. On
   Steam you can instead use Gothic 1 → Properties → Installed Files → **Verify integrity of game
   files**, which also puts Gothic's own DLLs back in place of Union's.

The rest of the mod's files in `_work` are only used by the mod and do no harm.
`python tools\install.py --uninstall` removes them too.

## What it does

* **CJ as the hero:** his GTA model, skeleton and animations, and his voice in place of the
  hero's lines.

* **GTA controls and camera:** camera-relative W/A/S/D, a mouse-orbit follow camera, sprint that
  builds up like GTA's, GTA's jump, and a click combo with your fists.

* **GTA HUD:** health, money (Gothic's ore), weapon icon with ammo, clock, a radar built from
  Gothic's world map, and "wasted" with San Andreas's death camera.

* **Weapons:** the Colt 45 pistol (aim, fire, reload, muzzle flash, bullet impacts and splashes),
  the rocket launcher (first-person aim, missile, an explosion that kills and throws people and the
  bike) and the purple dildo, which makes enemies run away. Shots and explosions light up their
  surroundings. Q/E cycle them like in GTA.

* **Motorbike:** the Sanchez, with GTA's handling numbers, gearbox, rider animations, and its
  engine, skid, crash and suspension sounds. Its wheels throw up dust and water spray, it has a
  headlight at night, and it drowns in deep water. Crashing hurts CJ. Press F to get on.

* **Clock:** Gothic's day can run at GTA's pace.

## Controls

| Key | Action |
| --- | --- |
| W/A/S/D | move, relative to the camera |
| Mouse | camera |
| Space | sprint |
| Left Shift | jump / climb |
| Left Alt | walk |
| C | sneak |
| F | talk, take, open, use; get on and off the bike |
| Q / E, mouse wheel | cycle weapons: fists, pistol, rocket launcher, dildo |
| Left mouse | punch (click again for the combo), fire, swing |
| Right mouse | block, aim |
| On the bike | W/S throttle and brake, A/D steer, Space rear brake, ↑/↓ lean |

Gothic's menu keys stay the same: Tab for the inventory, B for status, and its log and map keys.

## Settings

All settings are in `Gothic.ini`, section `[CJMOVEMENT]`, each with a short note. Some useful ones:

* `runSpeed`, `sprintSpeed`: CJ's speed as a share of GTA's.
* `mouseSpeed`, `invertMouseY`, `camDistance`: camera feel.
* `gtaHud`, `radar`, `gtaClock`, `wastedCam`: switch features on or off.
* `rocketRadius`, `fleeRadius`, `bikeSpeed`, `bikeGrip`: weapon and bike tuning.
* `lightFlashes`, `bikeLight`: light from shots and explosions, the bike's headlight.
* `crashDamage`: how much bike crashes hurt CJ (0 = not at all).
* `debugLog=1`: writes `CJMovement.log` in the Gothic folder. Attach it to bug reports.

## Known limitations

* The new items' names are in Polish, for example "Pistolet 9mm". They are set in
  `tools\make_cj_pistol.py`.
* The guns and the dildo replace a few Gothic items, so those items no longer exist as they were:
  * the light crossbow, which becomes the pistol, and its bolts, which become pistol ammo;
  * the second crossbow, which becomes the rocket launcher;
  * an unused alchemy ingredient, which becomes rockets;
  * Jackal's unused sword, which becomes the dildo.
* The bike isn't saved with the game. It appears next to CJ on every load. That is also how you
  get it back after it drowns in deep water.

## How it works

The converters in `tools/` read GTA San Andreas's files and write Gothic's formats with the shared
Gothic tools (`../../tools`). The Union plugin in `plugin/src/Plugin.ipp` brings GTA's mechanics
into Gothic's engine.

| File | Use |
| --- | --- |
| `tools/mashup_gta.py` | What the installer does for this mashup: settings, steps, files, start file |
| `tools/make_cj_model.py` | CJ's model and animations, and the hero in `Gothic.dat` pointed at him |
| `tools/make_cj_pistol.py` | Pistol, rocket launcher and dildo: models, sounds, the items in `Gothic.dat` |
| `tools/make_cj_hud.py`, `tools/make_cj_radar.py` | HUD textures, and the radar's map from Gothic's world map |
| `tools/make_cj_bike.py` | The motorbike (`Data\CJ_Bike.vdf`, `system\CJBike.ini`) |
| `tools/make_cj_voice.py` | CJ's voice for the hero's lines (`Data\CJ_Voice.vdf`, needs ffmpeg) |
| `tools/img.py`, `tools/dff.py`, `tools/txd.py`, `tools/ifp.py`, `tools/sa_audio.py` | GTA San Andreas's formats: IMG archives, models, textures, animations, SFX banks |
| `plugin/src/Plugin.ipp` | The CJMovement plugin: controls, camera, combat, HUD, radar, bike, effects |

Each tool runs on its own too: `python mashups\gta\tools\make_cj_model.py --help`.
