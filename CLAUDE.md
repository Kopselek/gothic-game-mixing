# gothic-game-mixing: Gothic mashups (monorepo)

Gothic mashed up with other games. Gothic 1 or Gothic 2 Night of the Raven is always the base (the
host, run alone); the other game brings its hero, mechanics and content, built at install time from
the player's own files of it. Public repo: github.com/Kopselek/gothic-game-mixing. The root
`README.md` is the player's guide and explains how the mashups work.

| Folder | Other game | Hero | Plugin | `Gothic.ini` section, log |
|---|---|---|---|---|
| `mashups/gta` | GTA San Andreas (classic PC) | CJ | `CJMovement` | `[CJMOVEMENT]`, `CJMovement.log` |
| `mashups/witcher` | The Witcher 3 (next-gen) | Geralt | `Witcher` | `[WITCHER]`, `Witcher.log` |
| `mashups/minecraft` | Minecraft Java Edition | Steve | `Minecraft` | `[MINECRAFT]`, `Minecraft.log` |
| `mashups/sims` | The Sims 4 | Gothic's hero played as a Sim | `Sims` | `[SIMS]`, `Sims.log` |

Gothic 1 (GothicMod.exe 1.08k_mod) is the tested base for all of them; Gothic 2 NotR (Gothic2.exe
2.6) builds from the same sources (its status per mashup is in the root README). One installer for
all of them (`tools/install.py --game gta|witcher|minecraft|sims --game-dir DIR`, GUI
`tools/installer_gui.py`, released as `GothicMashupInstaller.exe`); a Gothic folder holds one
mashup at a time.

**To add a mashup, use the project skill `/new-mashup`** (`.claude/skills/new-mashup`): recon, the
plan, converters, plugin, every place a mashup is registered, testing and known pitfalls.

## Layout
- `tools/`: Gothic's side, shared: the installer, `gothic.py` (Gothic 1 or 2), `union.py`, Gothic's
  formats (`asc`, `tex`, `vdf`, `wav`, `mesh3ds`, `man`, `msb`), `Gothic.dat` (`daedalus`,
  `datpatch`), previews (`png`, `render`). Each file's docstring documents its format.
- `mashups/<game>/tools/`: the other game's formats, `make_*.py` converters, and `mashup_<game>.py`
  (what the installer does for that mashup). Standalone tools put `<repo>/tools` on `sys.path`
  with one line at the top; module names must be unique across mashups (one namespace in the exe).
- `mashups/<game>/plugin/`: `src/Plugin.ipp` + a `CMakeLists.txt` (`project()` + `add_union_plugin()`).
- `mashups/<game>/README.md` (the player's view) and `PLAN.md` (scope, feature map, phases).
- `plugin/`: the shared plugin build (union-plugin-template made per plugin,
  `cmake/add_union_plugin.cmake`; Union libraries as submodules in `plugin/dependencies/union`).
- `.claude/skills/`: the project's Claude Code skills.
- Local only (the `.gitignore` is a whitelist and leaves them out): journals `MODLOG.md` (shared
  and monorepo work) and `mashups/<game>/MODLOG.md`, research notes `mashups/<game>/research/`,
  and `CLAUDE.local.md`.

## Paths: CLAUDE.local.md
Where Gothic and the other games are installed differs per PC, so it is not in this file. Each
contributor keeps their own in `CLAUDE.local.md` at the repo root (Claude Code reads it after this
file; it is never committed): the test Gothic folders (Gothic 1 and 2), the other games' folders,
the state of their test installs, anything about their PC. If it is missing or doesn't say, ask
the user for the paths instead of guessing, and offer to write them there.

## Knowledge to use first
- **Each mashup is the template for the others:** every feature of one has a counterpart in
  another (the "Feature map" in each `PLAN.md`). Read the matching part of the other mashup's
  `Plugin.ipp` and tools before building a feature, and reuse its engine calls and hooks.
- **Engine API:** the gothic-api headers in `plugin/dependencies/union` (run
  `git submodule update --init --recursive` if empty) have both engines' classes and addresses;
  `SIGNATURE_OF(&Class::Method)` resolves them.
- **GTA San Andreas:** gta-reversed (github.com/gta-reversed/gta-reversed) shows how it works. For
  research only: clone it outside the repo or into `gta-reversed/` at the root (ignored).
- **The Witcher 3 formats:** WolvenKit-7 (github.com/WolvenKit/WolvenKit-7) is the reference for
  bundles, CR2W, texture.cache, sounds and animation buffers; check it against the real files.
- **Minecraft:** its own code is the reference for numbers and behaviour, decompiled locally with
  Vineflower outside the repo and never published.
- **The Sims 4 formats:** s4pi / Sims4Tools (github.com/s4ptacle/Sims4Tools) and the Mod The Sims
  wiki document its DBPF packages and resources; check them against the real files.
- **Optional plugin** [universal-modder](https://github.com/rehan-remade/universal-modder)
  (`/plugin marketplace add rehan-remade/universal-modder`, then
  `/plugin install universal-modder@universal-modder`): skills `mod-any-game`, `mashup-mods`
  (pattern 1, "port the content"), `game-recon`, `reverse-engineering`, `asset-pipeline`,
  `game-automation`, `showcase-video`, `publish-mod`, `share-field-notes`; CLI `um` (needs `uv`).
  Its fal skills (AI art, 3D, audio) need a `FAL_KEY` in the environment, a paid service.

## Rules
- Mod only games the user owns, single-player. No anti-cheat bypass, DRM bypass or cheats against
  other players.
- The mashups work inside Gothic only: the other game is read, never started, patched or hooked.
- Never commit or publish game files, extracted or converted assets, or decompiled code; keep
  decompiles and extracted data outside the repo. A new kind of file ships only once the
  whitelist `.gitignore` lists it.
- Back up saves and every game file before changing it (`Gothic.dat.orig`, the `Gothic.ini`
  backup). Test installs go to a copy of the Gothic folder when possible.
- Edit game files with Python in binary mode and keep their line ends: `sed -i` from Git Bash
  rewrites `Gothic.ini`'s CRLF as LF. Some repo files are CRLF too; check before matching text.
- Kill processes by exact PID, never by name pattern.
- Ask first before: driving the user's mouse and keyboard or focusing the game, installing loaders
  into game folders or changing the registry, publishing anything (pushes, tags, releases, PRs,
  field notes).
- Keep a journal of what was tried and learned (`MODLOG.md`); at the end it can become a
  universal-modder `um kb` field note, with the user's OK.

## Build and test
- Needs: Windows; Python 3.10+ with Pillow and numpy (py7zr to install Union; ffmpeg on PATH for
  voices and sounds); Visual Studio 2022+ with the C++ workload and CMake; git.
- Run plain `python`: `-I` drops the script's folder from `sys.path`, and `-s` hides user
  site-packages, where Pillow and numpy may be.
- Plugins: `plugin\build.bat` (all) or `plugin\build.bat CJMovement|Witcher|Minecraft|Sims` -> copy
  `plugin\out\build\G1-G2A-Release\<game>\vdf\<Plugin>.vdf` to `<Gothic>\Data\`, restart the game.
  One DLL for both engines: each `Plugin.ipp` compiles for Gothic 1 and Gothic 2 NotR, hooks go
  through `SIGNATURE_OF(&Class::Method)`, content that differs uses `GOTHIC2 ? ... : ...`.
- Install: `python tools\install.py --game <gta|witcher|minecraft|sims> --game-dir "<other game's
  folder>" --gothic "<Gothic folder>" [--gothic-version 2]` (`--lang pl` picks Geralt's dub). It
  takes the built plugin from `plugin\out`. `--uninstall --gothic "<Gothic folder>"` removes the
  mashup. Each `mashups\<game>\tools\make_*.py` also runs on its own (`--help`).
- Start with the mashup's start file in the Gothic folder (`Start Gothic with CJ.bat`,
  `Start Gothic as Geralt.bat`, `Start Gothic as Steve.bat`, `Start Gothic as a Sim.bat`), or `system\GothicMod.exe
  -zautoconvertdata` (Gothic 2: `system\Gothic2.exe`); `-zwindow` for a window.
- Debug: `debugLog=1` in the mashup's `Gothic.ini` section, then `<Gothic>\<Plugin>.log`; archive
  old logs before a test session.
- Installer exe: as `.github/workflows/release.yml` does (PyInstaller with `--paths` for `tools`
  and every `mashups/<game>/tools`), then `GothicMashupInstaller.exe --selftest <log>`.
- Releases: pushing a `v*` tag runs `release.yml` (plugins, installer exe, selftest, GitHub
  release). Ask first.
