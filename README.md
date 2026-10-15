# Gothic Game Mixing

**Gothic** mashed up with other games. Gothic is always the base: you play its world and its story,
in **Gothic 1** or **Gothic 2 Night of the Raven**, your choice. The game you combine it with
brings its hero, controls, mechanics and content. Choose one:

| Combine Gothic with | You play as | What it brings | Details |
| --- | --- | --- | --- |
| **GTA San Andreas** | CJ | GTA's controls, camera and HUD, a radar, GTA weapons, the Sanchez motorbike, CJ's voice | [mashups/gta](mashups/gta/README.md) |
| **The Witcher 3** | Geralt | The Witcher 3's controls and camera, sword fighting, signs, Geralt's face and voice | [mashups/witcher](mashups/witcher/README.md) |

Gothic 1 is the tested base for both. Gothic 2 Night of the Raven is new: CJ installs into it and
the game starts with his plugin; playing it is still to be tried
([status](mashups/gta/README.md#gothic-2-status)). Geralt in Gothic 2 hasn't been tried at all.

This repository contains **no game files** from Gothic, GTA San Andreas or The Witcher 3. The
installer reads **your own** copy of the game you choose and builds its hero and content straight
into **your own** Gothic folder. You need legal copies of Gothic and of that game.

> Not affiliated with or endorsed by Piranha Bytes, THQ Nordic, Rockstar Games, Take-Two
> Interactive or CD PROJEKT RED. All game content belongs to its owners.

## Install

### What you need

* **Windows** with the
  [Microsoft Visual C++ Redistributable (x86)](https://aka.ms/vs/17/release/vc_redist.x86.exe).
  Most PCs already have it.
* **Gothic 1** or **Gothic 2 Night of the Raven**, from Steam or GOG:
  * Gothic 1: a disk edition needs the Gothic 1 Player Kit 1.08k first.
  * Gothic 2: the Gold Edition (Gothic II with Night of the Raven, `Gothic2.exe` 2.6). Gothic 2
    without the addon won't work.
* The game you combine it with, installed while you install the mod:
  * **GTA San Andreas, classic PC version.** The Definitive Edition won't work: the installer
    needs `models\gta3.img`, `anim\ped.ifp`, `audio\SFX` and `data\handling.cfg`.
  * **The Witcher 3: Wild Hunt** for PC (Steam or GOG). The mod was made and tested with the
    next-gen version (4.0 and later). The installer needs `content\content0` with its bundles and
    `texture.cache`.
* Optional: **ffmpeg** for the hero's voice (see [Troubleshooting](#troubleshooting)).

You don't need Python. You also don't need to install Union yourself, because the installer offers
to do it.

### Steps

1. Download **`GothicMashupInstaller.zip`** from the
   [latest release](https://github.com/Kopselek/gothic-game-mixing/releases/latest).
2. Extract **all** its files (`GothicMashupInstaller.exe`, `CJMovement.vdf` and `Witcher.vdf`)
   into the same folder.
3. Run `GothicMashupInstaller.exe`.
4. Under **Base game**, choose **Gothic 1** or **Gothic 2 Night of the Raven**, and pick its
   folder.
5. Under **Combine it with**, choose **GTA San Andreas** or **The Witcher 3**, and pick that game's
   folder. The installer needs both folders.
   * Steam usually has them in `C:\Program Files (x86)\Steam\steamapps\common\`, for example
     `...\common\Gothic`, `...\common\Gothic II` and `...\common\The Witcher 3`.
   * For Gothic, choose the folder that contains `system` and `Data`. For GTA San Andreas, the one
     with `models` and `anim`. For The Witcher 3, the one with `bin` and `content`.
6. For The Witcher 3, pick the language Geralt speaks (the dubs your copy has, Polish by default).
7. Click **Install**. If the installer says **Union** is missing, click **Yes**. Union is the
   loader that runs the mod's plugin. The installer downloads Union 1.0m from its author's archive
   ([Gratt-5r2/gothic-fix-archive](https://github.com/Gratt-5r2/gothic-fix-archive)), checks the
   file and installs it.
8. Wait for "Installation finished". GTA San Andreas takes a few minutes. The Witcher 3 takes
   several, most of them for Geralt's voice in the dialogues.

A Gothic folder holds **one mashup at a time**. If the other one is already installed, the
installer asks first, then removes it as an uninstall would, and installs the one you chose. Run
it again to switch back.

### Start the game

Start the game with the start file the installer puts into your Gothic folder, not with Steam's
Play button:

| Mashup | Start file |
| --- | --- |
| GTA San Andreas | `Start Gothic with CJ.bat` |
| The Witcher 3 | `Start Gothic as Geralt.bat`, or `Start Gothic as Geralt (window).bat` for a window |

Gothic has to compile the hero's model and animations once. For GTA San Andreas the installer
does it: at the end Gothic opens in a window and closes by itself. For The Witcher 3 the **first
start takes longer**. Each mashup's README says more.

### What the installer changes

* It backs up the files it edits first: `Gothic.dat` → `Gothic.dat.orig`, `Gothic.ini` →
  `Gothic.ini.before-cj` or `Gothic.ini.before-geralt`, and the files Union replaces →
  `system\_backup_before_union`.
* It builds the hero (and for GTA San Andreas the weapons, HUD, radar and bike) from your game
  files into `_work\DATA` and `Data\*.vdf`, and makes Gothic's hero CJ or Geralt in `Gothic.dat`.
* In `Gothic.ini` it adds the plugin's settings, `[CJMOVEMENT]` or `[WITCHER]`. Your own values
  stay when you install again. The GTA mashup also sets Gothic's own keys to GTA's.
* It copies the plugin to `Data\CJMovement.vdf` or `Data\Witcher.vdf` and creates the start files.

The other game is never started or changed. Only its files are read. Running the installer again
is safe: it updates the mod and puts Union back if it's gone.

### Troubleshooting

| Problem | Fix |
| --- | --- |
| The start file says **Union is not active**, or the hero is there but has Gothic's controls | Steam puts Gothic's own DLLs back over Union's when it updates or verifies the game, or when you switch its beta branch (public/workshop). Run the installer again. Keep Gothic on one branch. |
| Still Gothic's controls although Union is installed | Install the [Visual C++ Redistributable (x86)](https://aka.ms/vs/17/release/vc_redist.x86.exe). |
| **CJMovement.vdf** or **Witcher.vdf was not found next to the installer** | Extract all files from the zip into the same folder. |
| **Can't write …** / permission error | Gothic is in a protected folder (often GOG in `Program Files`). Run the installer as administrator. |
| **GothicMod.exe is missing** / **Gothic2.exe is missing** | That's the wrong folder: pick the one with `system` and `Data` in it. A Gothic 1 disk edition needs the Gothic 1 Player Kit 1.08k. |
| **That folder holds Gothic 2 … not Gothic 1** (or the other way round) | Choose the base game that matches the folder, or the other game's folder. |
| **Gothic 2 without Night of the Raven** | The mashups need the addon. Gothic II Gold Edition has it. |
| **… not found in the GTA San Andreas folder** | That isn't the classic PC GTA San Andreas folder. Pick the folder that contains `models` and `anim`. |
| **… not found in The Witcher 3 folder** | That isn't The Witcher 3's folder, or the game isn't installed any more. Pick the folder that contains `bin` and `content`. |
| Gothic's hero keeps his own voice | The hero's voice needs ffmpeg. Install it (`winget install Gyan.FFmpeg`), then start the installer again. |
| Windows SmartScreen or an antivirus warns about the installer | The exe isn't code-signed. Use "More info → Run anyway", or run the installer from source (see below). |
| Anything else | Set `debugLog=1` in `system\Gothic.ini`, in the mashup's section (`[CJMOVEMENT]` or `[WITCHER]`), reproduce the problem, and attach the log from the Gothic folder (`CJMovement.log` or `Witcher.log`) to an [issue](https://github.com/Kopselek/gothic-game-mixing/issues). |

### Uninstall

The GUI installer has no uninstall button yet. Each mashup's README says how to remove it by hand.

With Python, `python tools\install.py --uninstall --gothic "<Gothic folder>"` restores
`Gothic.dat` and `Gothic.ini` from the backups and removes every file of the installed mashup.
Union stays installed.

### Command line (developers)

You need **Python 3.10+** with Pillow for GTA San Andreas or numpy for The Witcher 3
(`pip install pillow numpy`), plus py7zr (`pip install py7zr`) if Union has to be installed.
ffmpeg on PATH is optional and only needed for the hero's voice.

```bat
git clone --recursive https://github.com/Kopselek/gothic-game-mixing.git
cd gothic-game-mixing
python tools\install.py --game gta --game-dir "D:\Games\GTA San Andreas" --gothic "C:\Program Files (x86)\Steam\steamapps\common\Gothic"
python tools\install.py --game witcher --game-dir "D:\Games\The Witcher 3" --gothic "C:\Program Files (x86)\Steam\steamapps\common\Gothic" --lang pl
python tools\install.py --game gta --game-dir "D:\Games\GTA San Andreas" --gothic "C:\Program Files (x86)\Steam\steamapps\common\Gothic II" --gothic-version 2
```

* `--gothic-version 2` installs into Gothic 2 Night of the Raven (the default is 1). The folder
  has to hold that version.
* `--gta DIR` is short for `--game gta --game-dir DIR`, and `--w3 DIR` for `--game witcher
  --game-dir DIR`.
* The plugin is taken from `plugin\out` once you have built it (see
  [Building the plugins](#building-the-plugins)). Or pass `--plugin path\to\CJMovement.vdf`
  (or `Witcher.vdf`) from a release.
* `--install-union` installs Union without asking. `--skip-voice` keeps Gothic's hero voice.
  `--lang en` (or `de`, `fr`, ...) picks Geralt's dub.
* Each step can also be run on its own: run any `mashups\<game>\tools\make_*.py` with `--help`.

## How it works

Every mashup has the same two parts:

* **Converters** (Python, `mashups/<game>/tools`). They read the other game's files from your
  install and write Gothic's formats: ASC models and animations, `-C.TEX` textures, WAV sounds, a
  patched `Gothic.dat` and VDF archives.
* **A Union plugin** (C++, `mashups/<game>/plugin/src/Plugin.ipp`). It brings the other game's
  mechanics into Gothic's engine: controls, camera, combat, HUD and the rest.

Gothic 1 and Gothic 2 Night of the Raven run on the same engine, ZenGin, in different versions.
The plugin's code is compiled once for each (union-plugin-template's per-engine build), into one
DLL that hooks the running game's own functions (gothic-api has both games' addresses). Where the
two games' content differs, the tools and the plugin pick by version: Gothic 2's animation list
comes from its compiled model script, its items and its worlds are other ones.

Everything on Gothic's side is shared:

```text
tools/              Gothic's side, used by every mashup:
                      install.py, installer_gui.py   the installer (command line and window)
                      gothic.py                      Gothic 1 or Gothic 2: version, exe, HUMANS model script
                      union.py                       download and install Union 1.0m into Gothic 1 or 2
                      asc, tex, vdf, wav, mesh3ds    Gothic's formats
                      msb                            read Gothic 2's compiled model scripts back into MDS
                      daedalus, datpatch             read and patch Gothic's compiled scripts (Gothic.dat)
                      png, render                    preview images
plugin/             the plugins' build (union-plugin-template; the Union libraries are submodules)
mashups/gta/        GTA San Andreas: README, tools (GTA's formats, make_cj_*.py), plugin (CJMovement)
mashups/witcher/    The Witcher 3: README, PLAN, tools (The Witcher 3's formats, make_geralt_*.py), plugin (Witcher)
```

What the installer does for a mashup is described in `mashups/<game>/tools/mashup_<game>.py`: its
name and hero, its plugin, its `Gothic.ini` settings, the tools it runs, the files it writes and
its start files.

### Adding a mashup

1. `mashups/<game>/tools`: the converters, and `mashup_<game>.py` with the same names as
   `mashup_gta.py` (`NAME`, `TITLE`, `HERO`, `PLUGIN`, `SETTINGS`, `MOD_FILES`, `check()`,
   `build()`, `starters()`, `selftest()` and the rest). A tool that runs on its own puts the shared
   `tools` folder on `sys.path` with the same line the other tools have. Module names must be
   unique across the mashups, because the installer exe holds them all side by side.
2. `mashups/<game>/plugin`: `src/Plugin.ipp` and a `CMakeLists.txt` with the plugin's `project()`
   and `add_union_plugin()`. Add the folder to `plugin/CMakeLists.txt`.
3. Import `mashup_<game>` in `tools/install.py` (`MASHUPS`). In
   `.github/workflows/release.yml`, add the tools folder to `--paths` and the plugin to the
   package.

## Building the plugins

`plugin/` builds every mashup's [Union](https://gitlab.com/union-framework) plugin. It is based on
[union-plugin-template](https://github.com/Patrix9999/union-plugin-template), with the build made
per plugin (`plugin/cmake/add_union_plugin.cmake`). Building needs **Visual Studio 2022+** (or
Build Tools) with the C++ workload and CMake, plus **git**. The Union libraries are git
submodules. `plugin\build.bat` finds Visual Studio, fetches the submodules if they're missing, and
builds, each for Gothic 1 and Gothic 2 Night of the Raven (CMake preset `G1-G2A-Release`):

* `plugin\out\build\G1-G2A-Release\gta\vdf\CJMovement.vdf`
* `plugin\out\build\G1-G2A-Release\witcher\vdf\Witcher.vdf`

`plugin\build.bat CJMovement` (or `Witcher`) builds only that one. Copy the VDF into
`<Gothic>\Data` and restart the game.

Releases are built by GitHub Actions (`.github/workflows/release.yml`) when a `v*` tag is pushed:
the plugins, then the installer exe, which must pass `GothicMashupInstaller.exe --selftest` before
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
* [WolvenKit-7](https://github.com/WolvenKit/WolvenKit-7), used for research into The Witcher 3's
  file formats.
* [universal-modder](https://github.com/rehan-remade/universal-modder): the method for modding
  and mashing up games with an AI agent, which these mashups follow.
* Built with AI assistance (Claude Code).

## License

The mod's own code is under the [MIT License](LICENSE). That covers the tools and the plugin
source only, not Gothic, GTA San Andreas or The Witcher 3 content, which you supply from your own
copies.
