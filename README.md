# Gothic × GTA San Andreas

Play **Gothic 1** as **CJ** from GTA San Andreas. You get his model, animations and voice, plus
GTA's controls, camera and HUD, GTA weapons, and the Sanchez motorbike.

This repository contains **no game files** from GTA San Andreas or Gothic. The Python tools in
`tools/` read **your own** GTA San Andreas installation and build CJ, his animations, sounds,
textures and the bike straight into **your own** Gothic folder. You need legal copies of both
games.

> Not affiliated with or endorsed by Rockstar Games, Take-Two Interactive, Piranha Bytes or
> THQ Nordic. All game content belongs to its owners.

## What it does

- **CJ as the hero:** his GTA model, skeleton and animations, and his voice in place of the
  hero's lines.
- **GTA controls and camera:** camera-relative W/A/S/D, a mouse-orbit follow camera, sprint that
  builds up like GTA's, GTA's jump, and a click combo with your fists.
- **GTA HUD:** health, money (Gothic's ore), weapon icon with ammo, clock, a radar built from
  Gothic's world map, and "wasted" with San Andreas's death camera.
- **Weapons:** the Colt 45 pistol (aim, fire, reload, muzzle flash, bullet impacts), the rocket
  launcher (first-person aim, missile, explosion) and the purple dildo, which makes enemies run
  away. Q/E cycle them like in GTA.
- **Motorbike:** the Sanchez, with GTA's handling numbers, gearbox, engine sounds and rider
  animations. Press F to get on.
- **Clock:** Gothic's day can run at GTA's pace.

## Requirements

- **Gothic 1** (Steam or GOG), with:
  - the **Gothic 1 Player Kit 1.08k**, which provides `system\GothicMod.exe`. The plugin targets
    that exe.
  - **Union 1.0m** for Gothic 1.
- **GTA San Andreas, classic PC version** (not the Definitive Edition). The tools read
  `models\gta3.img`, `anim\ped.ifp`, `audio\SFX` and `data\*`.
- **Python 3.10+** with Pillow: `pip install pillow`
- **ffmpeg** on PATH. Optional, only needed for CJ's voice.
- To build the plugin yourself: **Visual Studio 2022+** (or Build Tools) with the C++ workload
  and CMake, plus **git**.

## Install

1. **Get the code** with its submodules:
   ```
   git clone --recursive https://github.com/Kopselek/gothic-gtasa.git
   ```
2. **Get the plugin** in one of two ways:
   - download `CJMovement.vdf` from the Releases page, or
   - build it with `plugin\CJMovement\build.bat`. The output is
     `plugin\CJMovement\out\build\G1-Release\vdf\CJMovement.vdf`.
3. **Run the installer** with your two game folders:
   ```
   python tools\install.py --gta "D:\Games\GTA San Andreas" --gothic "C:\Program Files (x86)\Steam\steamapps\common\Gothic" --plugin path\to\CJMovement.vdf
   ```
   The installer:
   - checks the requirements;
   - builds everything from your GTA files;
   - points Gothic's hero at CJ;
   - sets GTA's keys in `Gothic.ini` and adds the `[CJMOVEMENT]` settings;
   - copies the plugin into `Data`;
   - creates `Start Gothic with CJ.bat` in your Gothic folder.

   It backs up `Gothic.dat` and `Gothic.ini` first.
4. **Start the game** with `Start Gothic with CJ.bat`. It runs `GothicMod.exe -zautoconvertdata`.
   The first start compiles CJ's model and animations, so it takes a while.

To **uninstall**, run `python tools\install.py --uninstall --gothic "<Gothic folder>"`. This
restores `Gothic.dat` and `Gothic.ini` and removes the mod's files.

Each step can also be run on its own. Run any `tools\make_cj_*.py` with `--help` to see how.

## Controls

| Key | Action |
|---|---|
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

- `runSpeed`, `sprintSpeed`: CJ's speed as a share of GTA's.
- `mouseSpeed`, `invertMouseY`, `camDistance`: camera feel.
- `gtaHud`, `radar`, `gtaClock`, `wastedCam`: switch features on or off.
- `rocketRadius`, `fleeRadius`, `bikeSpeed`, `bikeGrip`: weapon and bike tuning.
- `debugLog=1`: writes `CJMovement.log` in the Gothic folder. Attach it to bug reports.

## Known limitations

- The new items' names are in Polish, for example "Pistolet 9mm". They are set in
  `tools\make_cj_pistol.py`.
- The guns and the dildo replace a few Gothic items, so those items no longer exist as they were:
  - the light crossbow, which becomes the pistol, and its bolts, which become pistol ammo;
  - the second crossbow, which becomes the rocket launcher;
  - an unused alchemy ingredient, which becomes rockets;
  - Jackal's unused sword, which becomes the dildo.
- The bike isn't saved with the game. It appears next to CJ on every load.

## Building the plugin

`plugin/CJMovement` is a [Union](https://gitlab.com/union-framework) plugin, based on
[union-plugin-template](https://github.com/Patrix9999/union-plugin-template). The Union libraries
are git submodules. `build.bat` finds Visual Studio, fetches the submodules if they're missing,
and builds `CJMovement.vdf`. All of the mod's code is in `plugin/CJMovement/src/Plugin.ipp`.

## Credits

- [Union framework](https://gitlab.com/union-framework) (union-api, gothic-api) by Gratt & Union
  Team, under a BSD-style license. Their submodules carry their own licenses.
- [union-plugin-template](https://github.com/Patrix9999/union-plugin-template) by Patrix9999,
  which includes GothicVDFS by NicoDE.
- [gta-reversed](https://github.com/gta-reversed/gta-reversed), used for research into how
  GTA San Andreas works (movement, gearbox, tyres, HUD).

## License

The mod's own code is under the [MIT License](LICENSE). That covers the tools and the plugin
source only, not GTA San Andreas or Gothic content, which you supply from your own copies.
