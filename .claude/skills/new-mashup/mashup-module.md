# `mashup_<game>.py`: what the installer needs from a mashup

`tools/install.py` and `tools/installer_gui.py` know nothing about any mashup except what its
`mashups/<game>/tools/mashup_<game>.py` says. Keep the same names as `mashup_gta.py`,
`mashup_witcher.py` and `mashup_minecraft.py`; read one of them next to this page.

## Names

| Name | Needed | What it is |
|---|---|---|
| `NAME` | yes | lowercase id: `--game <NAME>`, `mashups/<NAME>`, the plugin's build folder |
| `TITLE` | yes | the guest game's name in the GUI and messages ("The Witcher 3") |
| `HERO` | yes | the hero's name ("Geralt") |
| `GAME_FOLDER` | yes | the guest's folder described for errors and the GUI's folder dialog: "the ... folder (the one with X and Y in it)" |
| `DEFAULT_DIR` | no | a usual place the GUI fills in when it exists (Minecraft's `%APPDATA%\.minecraft`) |
| `PLUGIN` | yes | the plugin's name: CMake `project()`, `Data\<PLUGIN>.vdf`; a VDF there means the mashup is installed |
| `INI_SECTION` | yes | the plugin's `Gothic.ini` section |
| `INI_BACKUP` | yes | `Gothic.ini.before-<hero>` in `system`; it also marks the mashup as installed |
| `STARTER` | by convention | the start file's name (used by `starters()` and `MOD_FILES`) |
| `DEFAULT_LANG` | yes | `None`, or the default dub when the hero speaks one of several (then define `dubs()`) |
| `COMPILE_ARG`, `COMPILED` | no | the compile run (below) |
| `KEYS` | yes | `Gothic.ini` key bindings set to the guest's (`{}` to leave Gothic's) |
| `SETTINGS` | yes | the plugin's settings: `(key, default, note)` |
| `MOD_FILES` | yes | every file the mashup writes into the Gothic folder, as globs |

- **`KEYS`**: `{"keyUp": "c8001100", ...}`. `install.set_up_ini()` replaces those keys wherever
  they are in `Gothic.ini`. A value is up to two DirectInput key codes, two bytes each, low byte
  first (`c800` is the up arrow, `1100` is W; the left mouse button is `0c02`).
- **`SETTINGS`**: added to `[INI_SECTION]` only when missing, so the player's own values stay on a
  reinstall. The defaults must be the plugin's own (`LoadSettings()` in `Plugin.ipp`), and the
  list should end with `("debugLog", "0", "1 = write <PLUGIN>.log in the Gothic folder")`.
- **`MOD_FILES`**: globs relative to the Gothic folder, for `--uninstall` and for switching to
  another mashup. Include the plugin's VDF, every start file, the log, and everything the tools
  write under `_work\DATA` (sources and `_COMPILED`) and `Data`. With one prefix for the mod's
  assets the globs stay exact (`r"_work\DATA\ANIMS\_COMPILED\STEVE*"`). Never a glob that could
  match Gothic's own files.

## Functions

```python
def check(game_dir, lang=None):
    """Problems that stop the install, as sentences the player can act on; [] when it can go."""

def build(args, run):
    """Run the tools: run(tool_module, *argv) calls the tool's main() with that argv.
    args has game_dir, gothic, gothic_version, lang, skip_voice."""

def starters(exe):
    """{start file: (its "rem" notes, how it starts `exe` from system)}. install.py adds
    "@echo off", the check that Union is still installed, and "cd system"."""

def selftest():
    """For GothicMashupInstaller.exe --selftest: import every tool, try a dependency, print ok."""

def dubs(game_dir):  # only with DEFAULT_LANG
    """The dubs the guest has installed, for the GUI's choice and check()."""
```

- `check()` looks for the files that prove the folder is the right game and edition (named in the
  message), the dub if there is one, and the Python packages the tools need (`import PIL`,
  `import numpy` inside a `try`).
- `build()` imports the tools inside the function, so a missing package is reported by `check()`
  instead of raised. Steps that need ffmpeg are skipped with a message when
  `shutil.which("ffmpeg")` is `None`, or when `args.skip_voice` is set.
- `starters()` lines use `\r\n`. Paths are relative to the Gothic folder, so it can be moved.

## The compile run (`COMPILE_ARG`, `COMPILED`)

Gothic compiles a new model's ASC sources only when started with `-zautoconvertdata`, and with
that flag it also stops near skeletons. So when `COMPILE_ARG` is set, the installer starts the
game once with `-zautoconvertdata` and `<COMPILE_ARG>` (in a window); the plugin sees the argument on the
command line, loads the hero's model, body and overlays on the menu's first frame (Gothic compiles
them), and quits. `COMPILED` lists the compiled files (`_work\DATA\ANIMS\_COMPILED\<HERO>.MDH`,
`<HERO>_BODY.MDM`, the overlays) that prove it worked. The start files pass `-zautoconvertdata`
only while one of them is missing. See `CompileRun()` and `Compile()` in
`mashups/minecraft/plugin/src/Plugin.ipp`, and `install.compile_run()`.

## A converter tool (`make_<prefix>_*.py`)

- The docstring says what it reads, what it writes, and its usage line:
  `usage: python make_x.py [--<game> DIR] --gothic DIR`.
- Right after the standard imports, the shared tools go on `sys.path` with the line every tool
  has:

  ```python
  # Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
  sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
  ```

- `main()` parses `sys.argv` with argparse (the installer calls it in its own process) and
  returns normally when it is done. It never starts Python; other programs start with
  `creationflags=subprocess.CREATE_NO_WINDOW`.
- It reads the guest's files where they are and writes only into the Gothic folder: model sources
  under `_work\DATA` (`ANIMS`, `MESHES`, `TEXTURES\_COMPILED`), or a `Data\<PREFIX>_*.vdf`
  (`vdf.write_vdf`). Previews go to a scratch folder, never into the repository.
- `Gothic.dat` is patched from `Gothic.dat.orig` (`datpatch.load()`), so running a tool twice
  gives the same result.
- Gothic 1 and 2 differences go through `gothic.version(gothic_dir)`.

## Template

```python
"""The <Game> mashup, as the installer (tools/install.py) installs it: Gothic 1 or Gothic 2 as <Hero>.

Its tools read your <Game> files (...) and write <Hero> and ... into your Gothic folder, and the
<Plugin> plugin brings <Game>'s ... . See PLAN.md.
Steps (each tool can also be run on its own, see its usage):
  make_<prefix>_model.py   <Hero>'s model and animations, and the hero in Gothic.dat pointed at him
"""
import os

NAME = "<game>"
TITLE = "<Game>"
HERO = "<Hero>"
GAME_FOLDER = "the <Game> folder (the one with <a> and <b> in it)"
PLUGIN = "<Plugin>"  # Data\<Plugin>.vdf
INI_SECTION = "<PLUGIN>"
INI_BACKUP = "Gothic.ini.before-<hero>"
STARTER = "Start Gothic as <Hero>.bat"
DEFAULT_LANG = None

COMPILE_ARG = "-<prefix>compile"
COMPILED = [r"_work\DATA\ANIMS\_COMPILED\<HERO>.MDH", r"_work\DATA\ANIMS\_COMPILED\<HERO>_BODY.MDM"]

KEYS = {}

# [<PLUGIN>]: the plugin's settings with their defaults (see ../plugin/src/Plugin.ipp)
SETTINGS = [
    ("controls", "1", "<Game>'s controls and camera (0: Gothic's own)"),
    ("debugLog", "0", "1 = write <Plugin>.log in the Gothic folder"),
]

# what the tools write into the Gothic folder (for --uninstall)
MOD_FILES = [
    r"Data\<Plugin>.vdf", STARTER, r"<Plugin>.log",
    r"_work\DATA\ANIMS\<HERO>*", r"_work\DATA\ANIMS\_COMPILED\<HERO>*",
    r"_work\DATA\TEXTURES\_COMPILED\<HERO>*",
]


def check(game_dir, lang=None):
    problems = []
    for need in (r"<a>\<file>", r"<b>\<file>"):
        if not os.path.exists(os.path.join(game_dir, need)):
            problems.append(f"{need} not found in the <Game> folder: it must be {GAME_FOLDER}")
    return problems


def build(args, run):
    """Run the tools (install.py's run() calls each one's main())."""
    import make_<prefix>_model

    run(make_<prefix>_model, "--<game>", args.game_dir, "--gothic", args.gothic)


def starters(exe):
    """{start file: (its notes, how it starts the game's `exe` from system)}; install.py adds the Union check."""
    convert = "set CONVERT=\r\n" + "".join(
        f'if not exist "..\\{path}" set CONVERT=-zautoconvertdata\r\n' for path in COMPILED)
    return {STARTER: (
        "rem Starts Gothic as <Hero>. -zautoconvertdata only if <Hero> isn't compiled yet.\r\n",
        convert + f'start "" {exe} %CONVERT%\r\n')}


def selftest():
    """For the GUI installer's --selftest: every tool imports."""
    import make_<prefix>_model  # noqa: F401

    print("<game>: tools import: ok")
```
