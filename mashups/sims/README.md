# Gothic × The Sims 4

Play **Gothic 1** or **Gothic 2 Night of the Raven** the way you play **The Sims 4**. The hero
keeps Gothic's body, but you no longer steer him. You point and click: The Sims 4's camera looks
at the world from above, a click on the ground walks him there, and a click on someone or
something opens The Sims 4's pie menu of what he can do with it. He finds his own way, uses what
you chose, and fights on his own. Gothic's world, story, dialogues and inventory stay Gothic's,
and every step to the end of the game can be done with the mouse, the Sleeper's hearts included.

This is one of the Gothic mashups in [gothic-game-mixing](../../README.md). Install it with
`GothicMashupInstaller.exe` by choosing your Gothic and **The Sims 4** (see
[Install](../../README.md#install)). The installer reads **your own** The Sims 4 files: its
interaction icons, its HUD symbols and the plumbob. It builds The Sims 4's UI from them straight
into **your own** Gothic folder.

> Not affiliated with or endorsed by Electronic Arts, Maxis, Piranha Bytes or THQ Nordic. All game
> content belongs to its owners.

## What you need

* **The Sims 4** for PC, from Steam or the EA app, installed while you install the mod. The base
  game is enough. The installer needs `Game\Bin\TS4_x64.exe`, `Data\Client\ClientFullBuild0.package`
  and `Data\Client\UI.package` in its folder.
* Nothing else: the UI's text is drawn in Windows' own Segoe UI.

## Start the game

Start the game with **`Start Gothic as a Sim.bat`** in your Gothic folder, not with Steam's Play
button. There is nothing to compile, so the first start is as quick as any other.

## What the installer changes

* It backs up `Gothic.dat` → `Gothic.dat.orig` (it doesn't change it: the hero stays Gothic's)
  and `Gothic.ini` → `Gothic.ini.before-sim`.
* It writes The Sims 4's UI into `_work\DATA\TEXTURES\_COMPILED\SIMS_UI-C.TEX` and `SIMS_UI.TXT`:
  the pie menu's bubbles, the panels and buttons, The Sims 4's icons, the plumbob, the font, and
  the UI's texts in your game's language (Polish, English, German or Russian).
* In `Gothic.ini` it adds `[SIMS]` (see [Settings](#settings)) and turns Gothic's mouse on
  (`enableMouse=1`). Gothic's own key bindings stay.
* It copies the plugin to `Data\Sims.vdf` and creates the start file.

## Uninstall by hand

1. Put `system\Gothic.ini.before-sim` back as `system\Gothic.ini`.
2. Delete `Data\Sims.vdf`, `Start Gothic as a Sim.bat` and `Sims.log`.
3. Delete `_work\DATA\TEXTURES\_COMPILED\SIMS_*`.

With Python, `python tools\install.py --uninstall --gothic "<Gothic folder>"` does all of it.

## What it does

* **The Sims 4's camera**: it looks at a point on the ground from above, not over the hero's
  shoulder, and follows him until you move it yourself. It goes through Gothic's world without
  bumping into it. When a wall, a roof or a rock hides him, it is cut away so you see him with the
  room round him, and objects in the way turn see-through. In dialogues it looks between him and
  whom he talks with.
* **The pointer and the icons**: the mouse moves a pointer. Everything the hero can use near him
  shows the icon of what he would do with it (talk, fight, take, open, sleep, sit, climb, pray,
  read, cook, smith, dig, pull a lever, stab the Sleeper's heart...); the one under the pointer
  shows its name.
* **Go here**: a click on the ground walks him there. He finds his own way round walls and
  obstacles (a route over a grid of half metres, the waynet for long walks), climbs ledges with
  Gothic's jump, and says so in a thought bubble when there is no way.
* **The pie menu**: a click on someone or something opens The Sims 4's pie menu, its name in the
  middle and what he can do round it: talk, fight, cast a spell, follow; search a body, finish
  someone off; take an item; open a chest or a door (with the key or by picking the lock), sleep,
  sit, read, climb, use any of Gothic's objects; shoot at a target. A right click on the ground
  offers run, walk, sneak and jump. On the hero himself: his inventory, weapon, status and log.
* **The queue**: what you chose goes into The Sims 4's queue, top left. A new choice replaces it,
  with Shift it is added; a click on a queued action cancels it.
* **Gothic does the rest**: he walks to the thing and Gothic's own action does what it does for
  a player: the dialogue, the chest, the lock pick, the item a mob needs, the sleep menu. While he
  uses something, buttons give Gothic's direction keys (stand up, left, right, up, down: lock
  picks, wheels). On a ladder he climbs on his own.
* **Fighting**: "Fight" on someone (in the pie menu, or the HUD's button for the one under the
  pointer, the last one you clicked or the nearest enemy) makes him run to the enemy with his
  weapon drawn, and Gothic's own combat AI fights for him there. If it doesn't strike, he swings
  himself with Gothic's attack. **Auto fight** (on by default): whoever attacks him, he fights back
  on his own. After a fight the weapon goes away.
* **Spells, bows and targets**: the HUD's spell button (and "Cast a spell" in the pie menu) lists
  his runes and scrolls; he draws the spell and casts it at the target or ahead. Telekinesis
  reaches items; a bow or a crossbow shoots at the targets of the Sleeper's temple and Irdorath.
* **The Sims' item screen**: chests, bodies and trading open The Sims 4-style screen over
  Gothic's: the things on each side as rows, a click moves them, "Take all" takes everything. In
  Gothic 1's barter you put things into the two offers and press "Trade"; in Gothic 2 a click buys
  or sells at Gothic's prices.
* **Dialogues**: Gothic's own, with the choice under the pointer chosen and a click to say it.
* **The HUD**: the hero's panel with the plumbob, his health and mana; the buttons; the game's
  speed (pause, 1, 2, 3); the plumbob turning over his head.

## Controls

| Key or button | Action |
| --- | --- |
| Left click on the ground | go there (Ctrl: walk; Shift: add to the queue) |
| Left click on someone or something | the pie menu |
| Right click on the ground | run, walk, sneak or jump there |
| Right click, Esc | close the pie menu |
| W/A/S/D, arrow keys, the pointer at the screen's edge | move the camera |
| Q/E | turn the camera |
| Mouse wheel, Z/X | zoom |
| Middle or right button held | turn and tilt the camera |
| Home | the camera back on the hero |
| Space | pause and go on |
| 1, 2, 3 | the game's speed |
| F8 | Gothic's own controls and back (a note at the top says so) |
| Tab, B, L, M | Gothic's inventory, status, log and map (also the HUD's buttons) |
| 4–0 | Gothic's spell keys |

## Settings

In `system\Gothic.ini`, section `[SIMS]` (installing again keeps your values):

| Setting | Default | What it does |
| --- | --- | --- |
| `controls` | 1 | The Sims 4's camera and controls (0: Gothic's own; F8 switches in game) |
| `pointerSpeed` | 1.5 | pointer pixels per mouse unit |
| `takeSpeed` | 2.0 | how much faster he picks things up |
| `cutaway` | 1 | walls, roofs and objects between the camera and him cut away |
| `cutRadius` | 400 | centimetres round him kept when the world is cut away |
| `edgeScroll` | 1 | the pointer at the screen's edge moves the camera |
| `uiScale` | 1.0 | the size of the UI |
| `autoFight` | 1 | start with auto fight on |
| `fightAI` | 0 | 0: Gothic's combat messages fight for him; 1: Gothic's script state ZS_ATTACK |
| `icons` | 1 | icons over everything he can use near him (0: only under the pointer) |
| `plumbob` | 1 | the plumbob over his head |
| `bridge` | 0 | 1: a test bridge for developers (`Sims.cmd` and `Sims.state` in the Gothic folder) |
| `debugLog` | 0 | 1 = write `Sims.log` in the Gothic folder |

## Known limitations

* It was made and played in Gothic 1 and Gothic 2 Night of the Raven, but neither has been played
  to its end yet.
* With the D3D11 renderer (GD3D11) Gothic's world can't be cut away: the camera goes into the
  room instead when the hero is under a roof. Objects still turn see-through.
* The Sims 4's UI sounds are in a format the installer can't read yet (EALayer3), so the UI is
  silent.
* Some places may still need a jump or a climb the route doesn't find: the right click's "Jump
  there", or F8 for Gothic's own controls.
* The hero keeps Gothic's body; a Sim's body isn't brought over.

## How it works

| File | Use |
| --- | --- |
| `tools/mashup_sims.py` | What the installer does for this mashup: settings, steps, files, start file |
| `tools/s4files.py` | Finds and checks your The Sims 4 folder, its packages in the game's load order |
| `tools/s4package.py` | Reads The Sims 4's packages (DBPF: zlib and RefPack) |
| `tools/s4image.py` | Decodes its images (DDS and its shuffled DST, RLE2, RLES, PNG, the UI's atlases) |
| `tools/s4gfx.py` | Reads its UI movies and the parts of their atlases (the HUD's symbols) |
| `tools/s4audio.py` | Reads its sounds (EA's SNR: PCM and EA-XAS) |
| `tools/s4icons.py` | Which of its icons, symbols, font and sounds the UI takes |
| `tools/make_sims_ui.py` | The UI atlas: bubbles, panels, buttons, icons, the plumbob, the font, the texts |
| `plugin/src/Plugin.ipp` | The Sims plugin: camera, cutaway, pointer, routing, pie menu, queue, actions, fighting, item screen, HUD |

The plan, with what each of The Sims 4's features became in Gothic, is in [PLAN.md](PLAN.md).
