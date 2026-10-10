# Gothic × GTA San Andreas

Play **Gothic 1** as **CJ** from GTA San Andreas. You get his model, animations and voice, plus
GTA's controls, camera and HUD, GTA weapons, and the Sanchez motorbike.

This repository contains **no game files** from GTA San Andreas or Gothic. The installer reads
**your own** GTA San Andreas installation and builds CJ, his animations, sounds, textures and the
bike straight into **your own** Gothic folder. You need legal copies of both games.

> Not affiliated with or endorsed by Rockstar Games, Take-Two Interactive, Piranha Bytes or
> THQ Nordic. All game content belongs to its owners.

## Install

### What you need

* **Windows** with the
  [Microsoft Visual C++ Redistributable (x86)](https://aka.ms/vs/17/release/vc_redist.x86.exe).
  Most PCs already have it.
* **Gothic 1** from Steam or GOG. A disk edition needs the Gothic 1 Player Kit 1.08k first.
* **GTA San Andreas, classic PC version.** The Definitive Edition won't work: the installer needs
  `models\gta3.img`, `anim\ped.ifp`, `audio\SFX` and `data\handling.cfg`.
* Optional: **ffmpeg** for CJ's voice (see [Troubleshooting](#troubleshooting)).

You don't need Python. You also don't need to install Union yourself, because the installer offers
to do it.

### Steps

1. Download **`GothicGTAInstaller.zip`** from the
   [latest release](https://github.com/Kopselek/gothic-gtasa/releases/latest).
2. Extract **both** files, `GothicGTAInstaller.exe` and `CJMovement.vdf`, into the same folder.
3. Run `GothicGTAInstaller.exe`.
4. Pick your **GTA San Andreas** folder and your **Gothic 1** folder, then click **Install**.
   * Steam usually has Gothic in `C:\Program Files (x86)\Steam\steamapps\common\Gothic`.
   * Choose the folder that contains `system` and `Data`.
5. If the installer says **Union** is missing, click **Yes**. Union is the loader that runs the
   mod's plugin. The installer downloads Union 1.0m from its author's archive
   ([Gratt-5r2/gothic-fix-archive](https://github.com/Gratt-5r2/gothic-fix-archive)), checks the
   file and installs it.
6. Wait for "Installation finished". It takes a few minutes.

### Start the game

Start the game with **`Start Gothic with CJ.bat`** in your Gothic folder, not with Steam's Play
button. The file runs `system\GothicMod.exe -zautoconvertdata`.

* The **first start takes longer**, because Gothic compiles CJ's model and animations.
* In the **first session**, CJ may hop or fall on slopes while the game is still compiling and
  caching. That goes away from the next start on.

### What the installer changes

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

Running the installer again is safe. It updates the mod and puts Union back if it's gone.

### Troubleshooting

| Problem | Fix |
| --- | --- |
| `Start Gothic with CJ.bat` says **Union is not active**, or CJ is there but there's no GTA HUD, controls or bike | Steam puts Gothic's own DLLs back over Union's when it updates or verifies the game, or when you switch its beta branch (public/workshop). Run the installer again. Keep Gothic on one branch. |
| Still no HUD, controls or bike although Union is installed | Install the [Visual C++ Redistributable (x86)](https://aka.ms/vs/17/release/vc_redist.x86.exe). |
| **CJMovement.vdf was not found next to the installer** | Extract both files from the zip into the same folder. |
| **Can't write …** / permission error | Gothic is in a protected folder (often GOG in `Program Files`). Run the installer as administrator. |
| **GothicMod.exe is missing** | That's the wrong folder: pick the one with `system` and `Data` in it. A disk edition needs the Gothic 1 Player Kit 1.08k. |
| **… not found under --gta** | That isn't the classic PC GTA San Andreas folder. Pick the folder that contains `models` and `anim`. |
| Gothic's hero keeps his own voice | CJ's voice needs ffmpeg. Install it (`winget install Gyan.FFmpeg`), then start the installer again. |
| Windows SmartScreen or an antivirus warns about the installer | The exe isn't code-signed. Use "More info → Run anyway", or build it yourself from source (see below). |
| Anything else | Set `debugLog=1` in `system\Gothic.ini`, section `[CJMOVEMENT]`, reproduce the problem, and attach `CJMovement.log` from the Gothic folder to an [issue](https://github.com/Kopselek/gothic-gtasa/issues). |

### Uninstall

The GUI installer has no uninstall button yet. To remove the mod by hand:

1. Delete `Data\CJMovement.vdf`, `Data\CJ_Weapons.vdf`, `Data\CJ_Bike.vdf`, `Data\CJ_Voice.vdf`
   and `Start Gothic with CJ.bat`.
2. Copy `system\Gothic.ini.before-cj` over `system\Gothic.ini`. That brings back your keys and
   settings from before the mod.
3. Copy `_work\DATA\scripts\_compiled\Gothic.dat.orig` over `Gothic.dat` in the same folder. On
   Steam you can instead use Gothic 1 → Properties → Installed Files → **Verify integrity of game
   files**, which also puts Gothic's own DLLs back in place of Union's.

The rest of the mod's files in `_work` are only used by the mod and do no harm.

With Python, `python tools\install.py --uninstall --gothic "<Gothic folder>"` restores
`Gothic.dat` and `Gothic.ini` from the backups and removes every file the mod added. Union stays
installed.

### Command line (developers)

You need **Python 3.10+** with Pillow (`pip install pillow`), plus py7zr (`pip install py7zr`) if
Union has to be installed. ffmpeg on PATH is optional and only needed for CJ's voice.

```bat
git clone --recursive https://github.com/Kopselek/gothic-gtasa.git
cd gothic-gtasa
python tools\install.py --gta "D:\Games\GTA San Andreas" --gothic "C:\Program Files (x86)\Steam\steamapps\common\Gothic" --plugin path\to\CJMovement.vdf
```

* Get `CJMovement.vdf` from the releases, or build it with `plugin\CJMovement\build.bat` (see
  [Building the plugin](#building-the-plugin)).
* `--install-union` installs Union without asking.
* `--skip-voice` keeps Gothic's hero voice.
* Each step can also be run on its own: run any `tools\make_cj_*.py` with `--help`.

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

## Building the plugin

`plugin/CJMovement` is a [Union](https://gitlab.com/union-framework) plugin, based on
[union-plugin-template](https://github.com/Patrix9999/union-plugin-template). Building it needs
**Visual Studio 2022+** (or Build Tools) with the C++ workload and CMake, plus **git**. The Union
libraries are git submodules. `build.bat` finds Visual Studio, fetches the submodules if they're
missing, and builds `CJMovement.vdf`. All of the mod's code is in
`plugin/CJMovement/src/Plugin.ipp`.

Releases are built by GitHub Actions (`.github/workflows/release.yml`) when a `v*` tag is pushed:
the plugin, then the installer exe, which must pass `GothicGTAInstaller.exe --selftest` before
the release is published.

## Credits

* [Union framework](https://gitlab.com/union-framework) (union-api, gothic-api) by Gratt & Union
  Team, under a BSD-style license. Their submodules carry their own licenses. Union itself is
  downloaded at install time from Gratt's
  [gothic-fix-archive](https://github.com/Gratt-5r2/gothic-fix-archive).
* [union-plugin-template](https://github.com/Patrix9999/union-plugin-template) by Patrix9999,
  which includes GothicVDFS by NicoDE.
* [gta-reversed](https://github.com/gta-reversed/gta-reversed), used for research into how
  GTA San Andreas works (movement, gearbox, tyres, HUD).

## License

The mod's own code is under the [MIT License](LICENSE). That covers the tools and the plugin
source only, not GTA San Andreas or Gothic content, which you supply from your own copies.
