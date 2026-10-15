# Plan: Minecraft in Gothic

The idea in one sentence: **play Gothic as Steve**, with his blocky model and Minecraft's
animations, Minecraft's first-person camera, movement, HUD and sounds, and Minecraft's blocks,
items and mobs in Gothic's world.

It is the third mashup of gothic-game-mixing, after GTA San Andreas (CJ) and The Witcher 3
(Geralt), and the same kind (universal-modder `mashup-mods` pattern 1, porting the content):
Gothic 1 or Gothic 2 Night of the Raven is the host and runs alone. Minecraft is never started or
changed. Converters read the player's own Minecraft Java Edition files at install time, and a
Union plugin brings the mechanics.

## Scope (decided with the user, 2026-10-15)

- The hero is **Steve**, from the game's own skin.
- All four parts: the character with Minecraft's movement, camera and HUD; **blocks** placed and
  broken in Gothic's world; Minecraft's **items** in place of Gothic's; Minecraft's **mobs** in
  place of some of Gothic's monsters.
- The camera is **first person**, F5 switches to third person (behind, then in front), as in
  Minecraft.
- Minecraft's own code is the reference for numbers and behaviour: the 26.3 client (no
  obfuscation since 26.1), decompiled with Vineflower into a local folder,
  outside the repository and never published.

## What the installer reads

The Minecraft folder (`%APPDATA%\.minecraft`):
- a client jar in `versions\<version>\<version>.jar`: `assets/minecraft/textures` (skins, blocks,
  items, mobs, GUI), `models/block` and `models/item` (JSON), `lang`;
- `assets\indexes\<id>.json` and `assets\objects`: the sounds (OGG) by hash.

On this PC: the 1.21.11 jar (as `versions\fabric-loader-0.18.4-1.21.11`, the vanilla client)
with asset index 29, complete. The 26.x versions have only their JSON there.

## Feature map

| Minecraft | In Gothic | Built like |
|---|---|---|
| Steve: the 64x64 skin on the player model's boxes (head, body, arms, legs; the second layer a bit larger) | the hero's soft-skinned model, each box on one bone, Gothic's bone names | CJ's model (`make_cj_model.py`) |
| Minecraft's animations: HumanoidModel's limb swing, idle arm bob, attack swing, sneaking, swimming, bow, death (computed, not keyframed) | ASC animations for Gothic's names, generated from Minecraft's formulas | Geralt's animation set, stand-ins and ladder root motion (`make_geralt_anims.py`) |
| First-person camera, F5 third person | the plugin's camera | CJ's follow camera |
| Movement: walk, sprint (double W or Ctrl), jump, sneak, swim, fall damage; Minecraft's speeds and physics | the plugin drives the hero | CJ's GTA controls |
| HUD: hotbar (9 slots, 1-9 and wheel), hearts, hunger, XP bar and level, crosshair | drawn over Gothic's view; hearts from Gothic's health, XP from Gothic's experience | GTA HUD |
| Sounds: steps by material, hurt, eating, item break, level up | WAV from the OGG assets, Gothic sound events | CJ's weapon sounds (`make_cj_pistol.py`) |
| Blocks: place and break, with Minecraft's break times, cracks and particles | blocks as objects (vobs) on a 1 m grid in Gothic's world; Gothic's own world mesh stays as it is | the bike's objects |
| Items: swords, pickaxe, axe, bow, food, as Minecraft draws them (extruded sprites) | Gothic's items get Minecraft's look by kind and strength (Gothic.dat patch, 3DS meshes) | the pistol and the dildo |
| Mobs: zombie, skeleton, spider, creeper (explodes), wolf, sheep, pig | Gothic's matching monsters get the mob's model, animations and sounds; the creeper's blast in the plugin | the rocket's blast |

## Phases

Each phase ends with the result seen in game (the user plays; the agent installs, starts the game
and reads logs and screenshots), in Gothic 1 and Gothic 2.

0. **Recon and reference.** Done: the Minecraft folder (above), the 26.3 client decompiled.
1. **Steve.** The model from the skin, the skeleton, Minecraft's animations for Gothic's names
   and the interaction stand-ins (ladders with Gothic's root motion), the hero pointed at him;
   the installer's third game (`mashup_minecraft.py`); the plugin with the first-person camera
   and F5.
2. **Movement and HUD.** Minecraft's movement and physics, the hotbar, hearts, hunger, XP,
   crosshair; the sounds.
3. **Items.** Gothic's weapons, tools and food as Minecraft's; Minecraft's attack (cooldown,
   swing), the bow.
4. **Blocks.** Placing and breaking blocks, their inventory in the hotbar, saved with the game.
5. **Mobs.** Zombies, skeletons, spiders, creepers, wolves, sheep, pigs.

## Risks and open questions

- **First person in ZenGin.** Gothic draws the hero's whole model; a first-person view needs the
  head hidden and the camera at the eyes (Gothic's own first-person look, keyLookFP, shows the
  way).
- **Blocks as objects.** Thousands of block vobs may cost Gothic's renderer; merging placed
  blocks into fewer meshes may be needed.
- **Saving blocks.** Vobs the plugin creates are not in Gothic's savegame on their own (the bike
  has the same limit): the plugin keeps its own file per savegame.
- **Mobs on Gothic's skeletons.** Gothic's monster AI plays its animation names; each mob needs
  its own set for the names its Gothic counterpart uses.
- **Two Minecraft versions.** The reference is 26.3, the assets the installer reads may be
  older (1.21.11 here); the numbers that matter have not changed between them.

## Rules

- Only the player's own Minecraft and Gothic. No Minecraft file, and nothing converted from one,
  in the repository or the releases (the `.gitignore` is a whitelist).
- Decompiled Minecraft stays in `mc-research`, outside the repository, read for reference only.
- Back up every Gothic file before changing it (`.orig`); converters always work from the backup.
