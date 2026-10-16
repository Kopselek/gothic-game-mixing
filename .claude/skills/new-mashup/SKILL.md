---
name: new-mashup
description: Create a new Gothic mashup in gothic-game-mixing and wire it into the installer, the plugin build and the release. Gothic 1 or Gothic 2 Night of the Raven is the host; another game brings its hero, mechanics and content, converted from the player's own files. Use when someone wants to add a game to the project ("Gothic as <hero> from <game>"), start a new mashups/<game> folder, or asks how a mashup is structured, registered, built, tested or released.
---

# Adding a mashup to gothic-game-mixing

A mashup here always has the same shape:

- **Gothic is the host and runs alone.** Gothic 1 (GothicMod.exe 1.08k_mod) is the tested base;
  Gothic 2 Night of the Raven (Gothic2.exe 2.6) builds from the same sources.
- **The other game (the guest) is only read.** It is never started, patched or hooked.
  Converters (Python, `mashups/<game>/tools`) read the player's own install of it at install time
  and write Gothic's formats into the player's own Gothic folder.
- **A Union plugin** (C++, `mashups/<game>/plugin/src/Plugin.ipp`) brings the guest's mechanics
  into Gothic's engine: controls, camera, combat, HUD and the rest.
- **One installer** (`tools/install.py`, GUI `tools/installer_gui.py`, released as
  `GothicMashupInstaller.exe`) installs any mashup; a Gothic folder holds one mashup at a time.

In universal-modder's terms this is `mashup-mods` pattern 1, "port the content".

## Ground rules

- Only games the user owns, single-player. No DRM or anti-cheat bypass, no cheats against other
  players.
- **Nothing from any game goes into the repository or a release**: no game files, nothing
  extracted or converted from them, no decompiled code. The `.gitignore` is a whitelist (only
  sources ship). Extracted data, decompiles and research dumps stay outside the repository; a tool
  that extracts should refuse to write inside it (`w3bundle.py extract` does).
- Back up before changing a game file. The installer backs up `Gothic.dat` as `Gothic.dat.orig`
  and `Gothic.ini` as the mashup's `INI_BACKUP`; converters always read from the `.orig`
  (`datpatch.load()` does this for you).
- Test in a copy of a Gothic folder when you can, and back up the saves first.
- Edit game files with Python in binary mode. `Gothic.ini` is CRLF and latin-1; `sed -i` from Git
  Bash rewrites it as LF.
- Kill a game process by its exact PID (or process handle), never by name.
- **Ask the user first** before: driving their mouse and keyboard or focusing the game window,
  installing loaders (Union) or changing the registry, and publishing anything (pushes, tags,
  releases, PRs).
- Keep a journal of what was tried and learned in `mashups/<game>/MODLOG.md` and research notes in
  `mashups/<game>/research/`. Both stay local (the whitelist leaves them out).

## Read first

1. The root `README.md`, "How it works" and "Adding a mashup".
2. **The existing mashups are the templates.** For every feature of the new mashup, find the
   closest one already built, read its code first and reuse its engine calls and hooks:

   | Mashup | Hero, plugin | Good template for |
   |---|---|---|
   | `mashups/gta` | CJ, `CJMovement` | third-person controls and camera, HUD and radar, guns and projectiles, a rideable vehicle, voice lines, new items in Gothic.dat, Gothic's keys set to the guest's |
   | `mashups/witcher` | Geralt, `Witcher` | a skinned model and animations from a complex engine format, sword combat, signs (effects from Gothic's particles), choosing a dub (`dubs()`), dialogue voice |
   | `mashups/minecraft` | Steve, `Minecraft` | a model generated from a texture, animations computed from the guest's formulas, first person, objects placed in the world and saved per savegame, Gothic's monsters given the guest's look, the compile run |

   Each `PLAN.md` has a **Feature map** (guest feature, how it is done in Gothic, which earlier
   mashup it is built like). Do the same for the new one.
3. The engine API: the gothic-api headers in `plugin/dependencies/union` (a submodule; run
   `git submodule update --init --recursive` if it is empty). They have the classes and both
   engines' addresses, which `SIGNATURE_OF(&Class::Method)` resolves.
4. The Gothic-side Python modules in `tools/` (shared by every mashup; each file's docstring
   explains its format):

   | Module | Use |
   |---|---|
   | `asc.py` | ASC skeletons, soft-skinned meshes and animations |
   | `tex.py` | `-C.TEX` textures (DXT1 with mipmaps, DXT3 with alpha) |
   | `mesh3ds.py` | `.3DS` static meshes (items, objects, effects) |
   | `wav.py` | WAV files Gothic plays |
   | `vdf.py` | read and write VDF archives (`VdfReader`, `write_vdf`) |
   | `daedalus.py`, `datpatch.py` | read and patch `Gothic.dat` without the script sources (`set_hero_visual`, item fields) |
   | `gothic.py` | Gothic 1 or 2: `version()`, `EXE`, the `HUMANS` model script, hierarchy, root motion |
   | `man.py`, `msb.py` | compiled animations and model scripts read back (Gothic 2 ships only `HUMANS.MSB`) |
   | `png.py`, `render.py` | preview images to check a model offline |
   | `union.py` | download and install Union 1.0m |

5. Optional but useful: the public [universal-modder](https://github.com/rehan-remade/universal-modder)
   plugin for Claude Code (`/plugin marketplace add rehan-remade/universal-modder`, then
   `/plugin install universal-modder@universal-modder`). Its skills `game-recon`, `mashup-mods`,
   `reverse-engineering`, `asset-pipeline`, `game-automation` and `showcase-video` cover the
   general method.

## Workflow

### 0. Recon and scope (no code yet)

- Ask the user: which game and which edition or version they own, where it is installed, who the
  hero is, which features they want (the character, controls and camera, HUD, combat, items,
  vehicles, world content, enemies, voice), and whether Gothic 2 is in scope.
- Find what the installer has to read: the guest's files in its install folder (and any per-user
  folder, like Minecraft's `%APPDATA%\.minecraft`). Decide which editions are accepted and how
  `check()` tells a wrong one apart (GTA San Andreas: the classic PC version only; its files
  `models\gta3.img`, `anim\ped.ifp`, ... prove it).
- Find the formats. Look for community references (WolvenKit-7 for The Witcher 3, gta-reversed
  for GTA San Andreas) and check them against the real files. For the guest's numbers and
  behaviour its own code is the reference; if it has to be decompiled, do it into a folder outside
  the repository, read it for reference only and never publish it.
- Pick the names (they must not clash with the other mashups or with Gothic's own assets):

  | What | Rule | GTA / Witcher / Minecraft |
  |---|---|---|
  | `NAME` | lowercase; the `--game` value, the folder under `mashups/`, the plugin's build folder | `gta`, `witcher`, `minecraft` |
  | `PLUGIN` | the CMake `project()`, `Data\<PLUGIN>.vdf`, `<PLUGIN>.log`, the C++ namespace | `CJMovement`, `Witcher`, `Minecraft` |
  | `INI_SECTION` | the plugin's section in `Gothic.ini` | `CJMOVEMENT`, `WITCHER`, `MINECRAFT` |
  | `INI_BACKUP` | `Gothic.ini.before-<hero>` | `...before-cj`, `...before-geralt`, `...before-steve` |
  | `STARTER` | `Start Gothic as <Hero>.bat` | `Start Gothic with CJ.bat`, `Start Gothic as Geralt.bat`, `Start Gothic as Steve.bat` |
  | Python modules | unique across all mashups (the installer exe holds them in one namespace) | `make_cj_*`, `dff.py`; `make_geralt_*`, `w3*.py`; `make_steve.py`, `make_mc_*`, `mcfiles.py` |
  | Gothic asset names | one prefix, so `MOD_FILES` globs catch exactly the mod's files | `CJ*`, `GERALT*`, `STEVE*` and `MC_*` |

### 1. PLAN.md

Write `mashups/<game>/PLAN.md` from [plan-template.md](plan-template.md): the idea in one
sentence, the scope decided with the user (dated), what the installer reads, the feature map, the
phases (each one ends with the result seen in game), the risks and the rules. Get the user's OK
on the scope before building.

### 2. The first vertical slice: the hero, installed

The goal is that `python tools\install.py --game <game> ...` works end to end: the hero's model in
Gothic, a plugin that loads and logs, the start file, and `--uninstall` leaving nothing behind.

- `mashups/<game>/tools/`: the format readers, `make_<prefix>_model.py` (skeleton and soft skin
  through `asc.py`, textures through `tex.py`, an MDS that maps Gothic's animation names to the
  hero's, and `datpatch.set_hero_visual()` on `Gothic.dat`), and `mashup_<game>.py` as described
  in [mashup-module.md](mashup-module.md).
- `mashups/<game>/plugin/`: `CMakeLists.txt` and `src/Plugin.ipp` as described in
  [plugin.md](plugin.md).
- `mashups/<game>/README.md`: the player's view, with the same sections as the other mashups'
  (What you need, Start the game, What the installer changes, Uninstall by hand, What it does,
  Controls, Settings, Known limitations, How it works) and the "not affiliated" note naming the
  guest's owners.
- Register the mashup everywhere (the checklist below).
- Build, install into a test Gothic folder, start, read the log. The user plays and says what
  they see; you read the log and screenshots.

### 3. Feature by feature

Follow the plan's phases. For each feature: read how the template mashup did it, build the
converter part and the plugin part, test in Gothic 1 (and Gothic 2 if in scope), then update the
mashup's README (What it does, Controls, Settings, Known limitations) and the plan's status.

## Registration checklist

A new mashup is wired in at these places, and nowhere else:

1. **`tools/install.py`**: `import mashup_<game>` and add it to the tuple in `MASHUPS` (the
   order is the GUI's order). Add a line to the module docstring's list of games and a usage
   example. `installer_gui.py` needs no change: it builds its choices from `install.MASHUPS`.
2. **`plugin/CMakeLists.txt`**: `add_subdirectory(../mashups/<game>/plugin <game>)`. The second
   argument is the build folder and **must equal `NAME`**: `install.plugin_build()` looks for
   `plugin\out\build\G1-G2A-Release\<NAME>\vdf\<PLUGIN>.vdf`.
3. **`plugin/build.bat`**: its comment lists the outputs (`build.bat <PLUGIN>` builds the target
   `build_vdf_<PLUGIN>`).
4. **`.github/workflows/release.yml`**: `--paths mashups/<game>/tools` in the PyInstaller call;
   the plugin's VDF in "Verify the plugins were built", in "Assemble release package"
   (`Copy-Item`) and in the release's `files`. If the tools need a new pip package, add it to
   "Install packaging dependencies".
5. **Root `README.md`**: the mashups table; "What you need" (the guest's edition and the files the
   installer needs); "Steps" (the files in the zip, where the guest's folder is); the start file
   table; "What the installer changes" (backup name, INI section, VDF); Troubleshooting rows (the
   guest's folder errors, the missing VDF, the log file name); a command line example; "Building
   the plugins" outputs; the layout tree; Credits for any reference project used; the "not
   affiliated" owners; the License sentence's list of games.

## Build, install, test

- **Plugin**: `plugin\build.bat <PLUGIN>` (or `plugin\build.bat` for all) with Visual Studio
  2022+ (C++ workload, CMake) and git. Output:
  `plugin\out\build\G1-G2A-Release\<game>\vdf\<PLUGIN>.vdf`. For a quick iteration close Gothic,
  copy the VDF into `<Gothic>\Data\` and start again.
- **Install**: `python tools\install.py --game <game> --game-dir "<guest folder>" --gothic
  "<Gothic folder>" [--gothic-version 2]`. Python 3.10+ with what the tools need (Pillow, numpy;
  py7zr if Union has to be installed). Run plain `python`, not `python -I` (it drops the script's
  folder from `sys.path`). The plugin is taken from `plugin\out` once built, or from `--plugin`.
- **One tool at a time**: every `mashups\<game>\tools\make_*.py` runs on its own (`--help`).
- **Start**: the mashup's start file in the Gothic folder, or `system\GothicMod.exe
  -zautoconvertdata` (`system\Gothic2.exe` for Gothic 2); add `-zwindow` for a window.
- **Logs**: `debugLog=1` in the mashup's section of `system\Gothic.ini`, then
  `<Gothic>\<PLUGIN>.log`. Archive the old log before a test session.
- **Uninstall**: `python tools\install.py --uninstall --gothic "<Gothic folder>"`, then check the
  folder: anything left over is missing from `MOD_FILES`. Install another mashup on top too: the
  installer removes the old one first.
- **Installer exe**: build it as `release.yml` does (PyInstaller with `--paths tools` and every
  `mashups/<game>/tools`), then `GothicMashupInstaller.exe --selftest selftest.log` must end with
  `selftest: ok`. From source: `python tools\installer_gui.py --selftest selftest.log` (it
  downloads Union into a scratch folder, so it needs the network).

## Pitfalls the earlier mashups hit

- **`-zautoconvertdata` stops near skeletons.** With it Gothic also tries to rebuild some of its
  own monsters from sources it doesn't ship and stops ("ASC not found: SKE_SPAWN_A02"). The start
  files pass it only while the hero isn't compiled yet, and the installer's compile run
  (`COMPILE_ARG` and `COMPILED`, see [mashup-module.md](mashup-module.md)) compiles the hero up
  front.
- **Steam puts Gothic's own `system\Shw32.dll` back** over Union's when it updates or verifies
  the game. The start files check for Union (`install.write_starters` adds that) and the
  installer puts Union back.
- **The hero's overlays** (`HUMANS_*.MDS`: weapon skills, sprint, torch) are made for Gothic's
  skeleton. A hero with another skeleton needs `<HERO>_*.MDS` versions, swapped in when Gothic
  applies an overlay (the Minecraft plugin hooks `zCModel::ApplyModelProtoOverlay`).
- **Progressive meshes**: Gothic thins characters' meshes out with distance, which takes corners
  off hard-edged models (the Minecraft plugin keeps Steve's in full).
- **Armour** replaces the body's look through the item's `visual_change`; skip it while it goes
  on if the hero keeps his look (`oCNpc::EquipArmor`).
- **Gothic 1 and Gothic 2 differ in content**: instance names (`ZOMBIE` and `ZOMBIE01`), items,
  worlds, and Gothic 2 ships only the compiled `HUMANS.MSB` (`gothic.humans_mds()` reads either).
  Pick with `gothic.version()` in the tools and `GOTHIC2 ? ... : ...` in the plugin.
- **Objects the plugin creates and a load**: objects placed in the world go into Gothic's
  savegame with it; give them a name to find them again after a load (the Minecraft plugin's
  blocks are vobs named `MCBLOCK`). The plugin's own lists and passing objects (arrows, fire) must
  be dropped before a world goes: hook `oCGame::LoadSavegame`, `LoadGame` and `ChangeLevel`.
- **ffmpeg is optional**: when it is not on PATH, skip the voice or sounds with a message instead
  of failing the install (`build()` checks `shutil.which("ffmpeg")`).
- **The GUI exe has no python.exe**: tools run in the installer's own process (`install.run()`
  sets `sys.argv` and calls the tool's `main()`), so a tool must not start Python itself. Other
  programs (ffmpeg) start with `creationflags=subprocess.CREATE_NO_WINDOW`.
- **Import the tools inside `build()`**, not at the top of `mashup_<game>.py`, so `check()` can
  report a missing Pillow or numpy before an import fails.
