# Gothic × Minecraft

Play **Gothic 1** or **Gothic 2 Night of the Raven** as **Steve** from Minecraft. You get his
blocky model and Minecraft's first-person camera, movement and HUD. You also get blocks to place
and break, fire, lava, TNT, a bow with arrows, spawn eggs and the Ender Dragon, all in Gothic's
world.

This is one of the Gothic mashups in [gothic-game-mixing](../../README.md). Install it with
`GothicMashupInstaller.exe` by choosing your Gothic and **Minecraft** (see
[Install](../../README.md#install)). The installer reads **your own** Minecraft Java Edition files:
Steve's skin, the block, item and mob textures, the models, the font and the sounds. It builds
Steve, his animations and everything else straight into **your own** Gothic folder.

> Not affiliated with or endorsed by Mojang Studios, Microsoft, Piranha Bytes or THQ Nordic. All
> game content belongs to its owners.

## What you need

* **Minecraft: Java Edition.** Start a release version once in the Minecraft Launcher, so its
  client jar and sounds are in your Minecraft folder (`%APPDATA%\.minecraft`, which the installer
  fills in). It takes the newest version that has them. It was made and tested with 1.21.11.
* Optional: **ffmpeg** for Minecraft's sounds (explosions, blocks, the bow, fire, lava, the dragon).
  Without it the sounds are left out.

## Start the game

Start the game with **`Start Gothic as Steve.bat`** in your Gothic folder, not with Steam's Play
button.

* At the end of the installation **Gothic opens once and closes by itself**, with your own video
  settings. The installer starts it to compile Steve, his animations, the mobs and every block and
  item mesh, so your first start doesn't have to. If Gothic can't start with your video settings
  ("Can't create D3DX context"), the installer changes them until it does and says what it changed.
* The start file passes `-zautoconvertdata` only if that compile run didn't finish, so Gothic
  compiles Steve on your first start instead.

## What the installer changes

* It backs up `Gothic.dat` → `Gothic.dat.orig` and `Gothic.ini` → `Gothic.ini.before-steve`.
* It makes Gothic's hero Steve in `Gothic.dat`.
* It writes Steve's model and animations, Minecraft's mob bodies, the blocks, items, the Ender
  Dragon, fire, lava and the HUD into `_work\DATA`, and the sounds into `Data\MC_Sounds.vdf`.
* In `Gothic.ini` it adds `[MINECRAFT]` (see [Settings](#settings)) and sets Gothic's keys to
  Minecraft's (see [Controls](#controls)).
* It copies the plugin to `Data\Minecraft.vdf` and creates the start file.

## Uninstall by hand

1. In `_work\DATA\scripts\_compiled`, put `Gothic.dat.orig` back as `Gothic.dat`, and put
   `system\Gothic.ini.before-steve` back as `system\Gothic.ini`.
2. Delete `Data\Minecraft.vdf`, `Data\MC_Sounds.vdf`, `Start Gothic as Steve.bat` and
   `Minecraft.log`.
3. Delete `_work\DATA\MESHES\MC`, and these files:
   * `_work\DATA\ANIMS\STEVE*` and `MC*`, and the same names in `_work\DATA\ANIMS\_COMPILED`;
   * `_work\DATA\MESHES\_COMPILED\MC_*`;
   * `_work\DATA\TEXTURES\_COMPILED\STEVE*` and `MC_*`.

With Python, `python tools\install.py --uninstall --gothic "<Gothic folder>"` does all of it.

## What it does

* **Steve**: Minecraft's player model from the game's own skin, playing every one of Gothic's
  animations with stiff Minecraft limbs. The walk and run loops swing his arms and legs as
  Minecraft does. Armour keeps its protection but not its look.
* **Camera**: first person at Minecraft's eye height, in which Steve's body isn't drawn. Instead
  you see Minecraft's hand, or the item in it, with its swing, its dip when you switch items and
  its sway. F5 switches to behind him and to in front of him. The view bobs as you walk, tilts
  when you're hurt, and widens when you sprint.
* **Movement**: Minecraft's own speeds and physics in 20 ticks a second: walking, sprinting,
  sneaking (you don't walk off edges), jumping, falling with fall damage, climbing Gothic's
  ladders Minecraft's way, wading slowly through lava. Gothic still does the collision. In water,
  dialogues and Gothic's interactions Gothic's own movement takes over.
* **HUD**: Minecraft's hotbar, hearts, armour, hunger, air and experience bar at Minecraft's GUI
  scale, plus the crosshair with its attack indicator, the boss bar and the creative inventory (E).
* **Hunger**: Minecraft's food bar. Sprinting, jumping and fighting tire you, a full bar heals
  you, and an empty one starves you. Gothic's food fills it.
* **Combat**: the left mouse button is Minecraft's attack, with its cooldown. Gothic's weapons
  show as Minecraft's swords and axes.
* **Blocks**: twenty of Minecraft's blocks, placed with the right button and broken with the left
  at once, as in creative mode. Placed blocks are saved with the game. Sand falls when nothing is
  under it.
* **Minecraft's grid over Gothic's world**: Gothic's ground, rocks and walls count as solid
  blocks on Minecraft's 1 m grid (unseen). Blocks, fire, lava, falling sand and explosions work
  against it.
* **Fire**: flint and steel lights Minecraft's fire block. It spreads to wood, leaves, wool,
  bookshelves and TNT, burns out on bare ground, and sets whoever stands in it on fire.
* **Lava**: the lava bucket pours a source and the bucket takes it back. Lava flows as
  Minecraft's does (3 blocks out from a source, down first, towards the nearest drop), glows,
  burns, and lights flammable things near it.
* **TNT**: lit by flint and steel, by fire or by an explosion. It flashes and explodes after 4
  seconds, with Minecraft's explosion: rays that break blocks by their blast resistance, damage
  by how much of you the blast can see, a push that flings you and other lit TNT, its puffs of
  smoke and its sound.
* **Bow and arrows**: hold the right button to draw, release to shoot. The arrows fly with
  Minecraft's gravity and drag, stick in the ground and in blocks, and land critical hits at full
  draw.
* **Mobs**: Gothic's zombies and skeletons look like Minecraft's. Spawn eggs bring zombies,
  skeletons and creepers (they swell and explode near you).
* **Ender Dragon**: its spawn egg hatches it, and the spot becomes the middle of its End fight.
  It circles the spot, strafes you with fireballs that leave its breath on the ground, lands,
  roars and breathes, charges, and dies in explosions. Its wings fling you, its head bites, and it
  breaks the blocks it flies through, but flies over Gothic's own ground. It has Minecraft's 200
  health and a pink boss bar.
* **Talking and using**: the right button on someone (or a chest, a door, an item) under the
  crosshair is Gothic's action, whatever you hold. A drawn weapon is put away first.

## Controls

| Key | Action |
| --- | --- |
| W/A/S/D | move |
| Mouse | look |
| Space | jump; swim up in lava |
| Left Shift | sneak |
| Left Ctrl, or W twice | sprint |
| Left mouse | attack; break a block |
| Right mouse | talk, take, open, use; place a block; hatch an egg; draw the bow (hold); light fire; pour or take lava |
| 1–9, mouse wheel | hotbar slot (1 and 2 start with Gothic's melee and ranged weapons) |
| E | Minecraft's creative inventory: click an item to put it into the chosen slot, or press 1–9 over it |
| Tab | Gothic's inventory |
| F5 | first person, behind, in front |
| F | draw Gothic's weapon |
| Left Alt | walk |

## Settings

In `system\Gothic.ini`, section `[MINECRAFT]` (installing again keeps your values):

| Setting | Default | What it does |
| --- | --- | --- |
| `controls` | 1 | Minecraft's camera (0: Gothic's own) |
| `physics` | 1 | Minecraft's movement (0: Gothic's) |
| `firstPerson` | 1 | start in first person |
| `mouseSpeed` | 0.25 | camera degrees per mouse unit |
| `invertMouseY` | 0 | 1 = inverted mouse look |
| `fov` | 70 | vertical field of view in degrees |
| `hud` | 1 | Minecraft's HUD in place of Gothic's bars |
| `guiScale` | 0 | the HUD's scale (0: Minecraft's automatic one) |
| `vignette` | 1 | the darkening towards the screen's edges |
| `hunger` | 1 | Minecraft's hunger |
| `mobs` | 1 | Gothic's zombies and skeletons look like Minecraft's |
| `debugLog` | 0 | 1 = write `Minecraft.log` in the Gothic folder |

The hotbar is kept there too (`hotbar=`).

## Known limitations

* Gothic 1 is where it was made and played. The plugin is built for Gothic 2 Night of the Raven
  too, but that hasn't been tried.
* Gothic's world can't be broken. Only Minecraft's own blocks break, and blocks stand on
  Minecraft's grid, up to half a metre off Gothic's slopes.
* Lava is drawn at flat levels, without Minecraft's sloped surface.
* Fire, lava, lit TNT, arrows and the dragon aren't saved with the game. Hunger is full again
  after every load.
* One Ender Dragon at a time, without End crystals or experience.
* Items in Gothic's own inventory keep Gothic's look; the hotbar shows Minecraft's.

## How it works

| File | Use |
| --- | --- |
| `tools/mashup_minecraft.py` | What the installer does for this mashup: settings, keys, steps, files, start file |
| `tools/mcfiles.py` | Reads your Minecraft folder: the newest client jar with its assets, the sounds by the asset index |
| `tools/make_steve.py` | Steve's model and animations, the zombie, skeleton and creeper bodies, the hero in `Gothic.dat` |
| `tools/make_mc_blocks.py` | The blocks' meshes and textures, fire, lava, lit TNT's flash |
| `tools/make_mc_items.py` | The bow, arrows, flint and steel, buckets and eggs in Steve's hand, and the first-person hand |
| `tools/make_mc_dragon.py` | The Ender Dragon's parts |
| `tools/make_mc_hud.py` | The HUD atlas: sprites, item and block icons, the font, the names in your game's language |
| `tools/make_mc_sounds.py` | Minecraft's sounds (`Data\MC_Sounds.vdf`, needs ffmpeg) |
| `plugin/src/Plugin.ipp` | The Minecraft plugin: camera, movement, HUD, hunger, blocks, fire, lava, explosions, the bow, mobs, the dragon |

The plugin's behaviour follows Minecraft's own: its numbers and rules were taken from studying the
game's client, which isn't part of this repository.
