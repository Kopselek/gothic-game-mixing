# Plan: <Game> in Gothic

The idea in one sentence: **play Gothic as <Hero>**, with <his model, animations, voice>,
<Game>'s <controls, camera, combat, HUD>, and <content>.

It is the <n>th mashup of gothic-game-mixing and the same kind as the others (universal-modder
`mashup-mods` pattern 1, porting the content): Gothic 1 or Gothic 2 Night of the Raven is the host
and runs alone. <Game> is never started or changed. Converters read the player's own <Game> files
at install time, and a Union plugin brings the mechanics.

## Scope (decided with the user, <date>)

- The hero is **<Hero>**, from <where in the game's files>.
- The features in scope: <...>.
- Out of scope for now: <...>.
- The reference for <Game>'s numbers and behaviour: <its code, decompiled outside the repository /
  a community reverse-engineering project / measured in game>.

## What the installer reads

The <Game> folder (<the folder it is usually in>):
- `<path>`: <what is in it and which tool reads it>;
- `<path>`: <...>.

Editions: <which ones the installer accepts, which it refuses and how check() tells them apart>.

## Feature map

| <Game> | In Gothic | Built like |
|---|---|---|
| <Hero>: model, skeleton, textures | the hero's soft-skinned model, Gothic's bone names | CJ's model (`make_cj_model.py`) / Geralt's (`make_geralt_model.py`) / Steve's (`make_steve.py`) |
| <Hero>'s animations | ASC animations for Gothic's animation names | `make_geralt_anims.py` / `make_steve.py` |
| controls and camera | the plugin drives the hero and the camera | CJ's GTA controls / Minecraft's camera |
| HUD | drawn over Gothic's view | GTA HUD / Minecraft HUD |
| <...> | <...> | <...> |

## Phases

Each phase ends with the result seen in game (the user plays; the agent installs, starts the game
and reads logs and screenshots), in Gothic 1 and, if in scope, Gothic 2.

0. **Recon and reference.** The game's files and formats, the reference for its mechanics.
1. **The hero.** The model, the skeleton, the animations for Gothic's names, the hero in
   Gothic.dat pointed at him; `mashup_<game>.py` in the installer; the plugin loading and
   logging; install and uninstall clean.
2. **<Controls and camera.>**
3. **<...>**

## Risks and open questions

- <What may not work in Gothic's engine, and how to find out early.>
- <Formats not yet read, editions not yet tried.>

## Rules

- Only the player's own <Game> and Gothic. No <Game> file, and nothing converted from one, in the
  repository or the releases (the `.gitignore` is a whitelist).
- Extracted or decompiled <Game> data stays outside the repository, read for reference only.
- Back up every Gothic file before changing it (`.orig`); converters always work from the backup.
