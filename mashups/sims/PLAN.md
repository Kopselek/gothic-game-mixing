# Plan: The Sims 4 in Gothic

The idea in one sentence: **play Gothic as a Sim**: The Sims 4's live mode camera, the hero walking
where you click, Sims 4's pie menu on everything you can use, the hero fighting on his own, and
The Sims 4's look (its icons, the plumbob, its UI sounds). Gothic's inventory stays Gothic's.

It is the fourth mashup of gothic-game-mixing, after GTA San Andreas (CJ), The Witcher 3 (Geralt)
and Minecraft (Steve), and the same kind (universal-modder `mashup-mods` pattern 1, porting the
content): Gothic 1 or Gothic 2 Night of the Raven is the host and runs alone. The Sims 4 is never
started or changed. Converters read the player's own The Sims 4 files at install time, and a Union
plugin brings the mechanics.

Unlike the other mashups the hero keeps Gothic's body: what The Sims 4 brings is the way he is
played. The hero is "the Sim", with the plumbob over his head.

## Scope (decided with the user, 2026-10-27)

- **The whole game can be played this way**, Gothic 1 to the Sleeper's end and Gothic 2 Night of
  the Raven: everything the hero has to do is a click.
- **The Sims 4 camera**: it looks at a point on the ground, not at the hero; it pans, turns,
  tilts and zooms as Sims 4's does.
- **The hero is controlled as a Sim**: a click on the ground walks him there; a click on
  something opens Sims 4's pie menu of what he can do with it. Gothic's own AI does it (its
  movement, its route over the waynet, its interactions), as Gothic's characters do.
- **Every interactive thing has its icon and its actions**: people (talk, attack, steal, follow),
  bodies (search, finish off), items (take), chests and doors (open, pick the lock), beds (sleep),
  benches, levers, wheels, anvils, cauldrons, ore, books, ladders, and every other `oCMobInter`
  of both games; at the end of Gothic 1 **the Sleeper's five hearts**
  (`EVT_TPL_TRIGGER_SLEEPERHEARTS_01..05`, `SPORTAL_SLEEPER.MDS`, Uriziel) and the swords in the
  stone (`EVT_TPL_LASTEEVENT_SWORD_01..05`).
- **Fighting**: target someone and press "Fight" (Walcz): Gothic's combat AI fights for the hero.
  "Auto fight" (Auto-walka): whoever attacks him, he fights back on his own.
- **Gothic's inventory**, its trading and its chests, as Gothic has them, also with the mouse.
- **Gothic's dialogues**, their choices clickable.
- The look of The Sims 4: its interaction icons, the plumbob, its UI sounds and font where the
  player's files have them; drawn from Windows' own fonts where they don't.
- Out of scope for now: the hero with a Sim's body (Sims 4's CAS parts and rig), Simlish voice,
  needs and moods as game rules.
- The reference for The Sims 4's behaviour: the game itself (camera keys, pie menu, the queue)
  and its files, read with the community's documentation of its formats (DBPF packages,
  s4pi/Sims4Tools); nothing decompiled.

## What the installer reads

The Sims 4 folder (Steam: `steamapps\common\The Sims 4`; EA app: `C:\Program Files\EA Games\The
Sims 4`):
- `Data\Client\*.package` and `Delta\...`: DBPF packages with the UI's images (interaction icons,
  the plumbob), its sounds and fonts; read by `s4package.py`, images by `s4image.py`.
- Which resources: found once in the player's install (research, kept as resource keys in the
  tools), never copied into the repository.

Editions: the PC game, Steam or EA app (the same files). `check()` looks for `Game\Bin\TS4_x64.exe`
and `Data\Client`. Without usable icons or sounds the tools fall back on what they draw themselves.

## Feature map

| The Sims 4 | In Gothic | Built like |
|---|---|---|
| Live mode camera: looks at a point on the ground; W/A/S/D and the arrows pan, Q/E turn, the wheel and Z/X zoom, the middle (or right) button held turns and tilts, the screen's edges pan; it follows the Sim when he walks | the plugin's camera (`zCAICamera::DoAI`): a focus point, yaw, pitch and distance; Gothic's camera only in dialogues and cutscenes | Minecraft's camera (`UpdateCamera`) |
| The pointer, what is under it highlighted | the plugin's pointer from the mouse's movement; a ray from the camera through it (`zCCamera` back-projection, `zCWorld::TraceRayNearestHit`, the vobs' boxes); the name of what is under it | Minecraft's creative inventory pointer and `PickBlock` |
| A click on the ground: "Go here" | Gothic's movement AI for the hero: a route over the waynet (`zCWayNet::FindRoute`) and `oCMsgMovement` `EV_GOTOPOS` to the spot, running (Shift walks) | new (Gothic's own NPC movement) |
| The pie menu of what the Sim can do with what was clicked | a ring of bubbles round the click, with the thing's name; the actions by what it is (a person, a monster, a body, an item, an `oCMobInter` by its scheme), each with its icon | new; an action is Gothic's own (`Interact` of the Minecraft mashup: the hero walks to it, faces it, and gets Gothic's action key) |
| The interaction queue, top left: icons of what he does and will do, X cancels | the plugin's queue of actions, drawn with their icons | new |
| Icons over what can be used | an icon over every interactive thing near the pointer and the hero, the action's | new |
| The Sim acting on his own | "Fight" (Walcz): Gothic's NPC combat AI (`oCMsgWeapon EV_ATTACK`, the fight AI tables) for the hero; "Auto fight" (Auto-walka): he fights back whoever attacks him | new (Gothic's `oCNpc` fight AI) |
| Buttons during an interaction (stand up, left, right) | Gothic's direction keys for the mob he uses (lock picking, leaving a bench), as buttons | Minecraft's `OverrideKey` |
| The plumbob over the Sim | a green gem over the hero's head, turning; its colour his health | the Minecraft mashup's blocks (an object of the plugin's) |
| Game speed: pause, 1, 2, 3 | Gothic's timer factor | the GTA mashup's `zCTimer` hook |
| Gothic's inventory with the mouse | the item under the pointer chosen in Gothic's container views; left click uses or equips (Gothic's action), right click drops; trading and chests too | new |
| Dialogue choices clicked | the choice under the pointer chosen in Gothic's dialogue box, a click says it | new |
| Sims 4's UI: icons, bubbles, panels, font, sounds | one atlas made at install time (`make_sims_ui.py`): the player's Sims 4 icons where found, Windows' fonts for the text and for missing icons; UI sounds as WAV | `make_mc_hud.py`, `make_mc_sounds.py` |

## Phases

Each phase ends with the result seen in game (the user plays; the agent installs, starts the game
and reads logs and screenshots), in Gothic 1 and Gothic 2.

0. **Recon.** Done for Gothic: the hooks and Gothic's AI messages the plugin uses, the end of
   Gothic 1 (above). The Sims 4's files: once the game is installed.
1. **The Sim's controls.** The plugin: the camera, the pointer, a click walks him there, the
   pie menu with every kind of target and Gothic's action on it, the queue, the buttons;
   `mashup_sims.py` in the installer; install and uninstall clean.
2. **Fighting.** "Fight" and "Auto fight" with Gothic's combat AI; bows, crossbows, spells.
3. **Gothic's screens with the mouse.** Inventory, trading, chests, dialogue choices.
4. **The Sims 4's look.** The atlas from the player's files (icons, bubbles, font), the plumbob,
   the UI sounds, game speed.
5. **The whole game.** Gothic 1 played through (each chapter's special interactions: the focus
   stones, the mines, the Sleeper's temple, the hearts) and Gothic 2's; what the pie menu or the
   paths miss, fixed.

## Status (2026-10-27, release 1.4.0)

- Phases 1–4 built and played in Gothic 1 and Gothic 2 by the user while they were made: the
  camera (no collision, the cutaway; with GD3D11 the camera goes into rooms), the pointer, the
  Sim's own routing (Gothic's movement messages end at once for the player), the pie menu on every
  kind of target, Gothic 1's held action key for mobs, the queue, fighting (he runs to the enemy,
  Gothic's combat AI strikes in reach, his own blows if it doesn't), Space/1/2/3, the item screen
  for chests, bodies and trading, the dialogue's choices with the mouse, The Sims 4's icons.
- Phase 5, the whole game played through, is still to come; the story-critical interactions are
  listed in the local research notes (mobs.md).
- The Sims 4's UI sounds are EALayer3, not decoded yet.

## Risks and open questions

- **Gothic's AI moving the player.** Scripts walk the hero with AI messages (`AI_GotoNpc` in
  dialogues), so `EV_GOTOPOS` and routes should work for him; to be seen how they end, how they
  are interrupted, and where they get stuck (jumps, climbs, water). Gothic's own controls stay as
  a fallback (a key switches).
- **Gothic's combat AI for the player.** `EV_ATTACK` and `ThinkNextFightAction` are made for
  NPCs; the hero may need his fight tactic set, or the script state `ZS_ATTACK`.
- **Interactions that read the player's keys** (lock picks, wheels, leaving a mob): buttons that
  give Gothic those keys.
- **The mouse**: Gothic reads it through DirectInput (relative movement), so the pointer is the
  plugin's own.
- **The Sims 4's formats**: DBPF packages (zlib and RefPack compression), its images (DST, RLE2,
  PNG), its sounds (EA's SNR/SNS); the icons' and sounds' keys found in the player's install.

## Rules

- Only the player's own The Sims 4 and Gothic. No The Sims 4 file, and nothing converted from one,
  in the repository or the releases (the `.gitignore` is a whitelist).
- Extracted The Sims 4 data stays outside the repository, read for reference only.
- Back up every Gothic file before changing it; converters always work from the backup.
