"""The Sims 4's look for the Sims mashup's UI (make_sims_ui.py): its icons, its font and its UI sounds,
found by their resource keys in the player's own install.

Icons come from two places (keys found in The Sims 4 1.128.90.1030, Steam, base game; nothing of
the game is kept here, only where to find it):
  "img"  an icon of the client packages: _IMG (type 0x00B2D882, group 0, a DST5/DST1 or 32-bit
         DDS) by instance, the same instance as the 0x2F7D0004 key the game's tuning names. They
         are the interaction, pie menu category (46 x 46), trait, skill and moodlet icons; the
         game's own copy wins (s4files.load_order: the deltas over the full builds)
  "ui"   a part of a UI movie's atlas (s4gfx.py): the n-th part of size w x h that a symbol of
         the movie draws, e.g. the pause button's white "pause" picture; parts are found again
         by symbol and size, so a re-exported movie still resolves
A name may have several choices, the first one found is used; a transform (flip, rot90, rot270)
turns an arrow into the others. Missing names are left out: the UI tool draws its own.

The font: "The Sims Sans SC" (small capitals: lower case drawn as small capitals; Latin with
Polish, German and the other European letters, and Cyrillic), type 0x276CA4B9 (TrueType),
instance D7E4D80287DB5780 (bold: 08062BDC0F4AA1D2), in ClientFullBuild0/ClientDeltaBuild0.

Sounds: the UI's effects by name (PROPX 0xFD04E3BE, FNV-1 64 of the name), each one's SNR variant
(s4audio.py). In 1.128 all of them but three main menu ones ("ui_select", "ui_select_delete",
"ui_select_play": EA-XAS) are EALayer3, which is not decoded: find_sounds returns only what it can
decode (EA-XAS, PCM), so {} for click, pie_open, pie_select, queue_cancel, speed and plumbob.

  python s4icons.py ["<The Sims 4 folder>"] --out "<folder outside this repo>"   (PNG of each icon,
                                     a contact sheet, the font, what was found and what not)
"""
import argparse
import os
import sys

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
from PIL import Image

import s4audio
import s4files
import s4gfx
import s4image
from s4package import Package

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))  # mashups/sims/tools -> the repo
IMG_TYPE, FONT_TYPE = 0x00B2D882, 0x276CA4B9
FONT_REGULAR, FONT_BOLD = 0xD7E4D80287DB5780, 0x08062BDC0F4AA1D2
FONT_NAME = b"T\x00h\x00e\x00 \x00S\x00i\x00m\x00s\x00 \x00S\x00a\x00n\x00s"  # in the name table (UTF-16BE)

ARROW = ("ui", "siminfotray", "widgets.Gameplay.SimInfoHUD.SimInfoTray.PhoneArrowButton", 0, (28, 28))

# name -> choices; ("img", instance [, transform]) or ("ui", movie, symbol, n, (w, h) [, transform])
ICONS = {
    # what the hero does (pie menu, queue)
    "go":        [("img", 0x96DE44C7D16BA610)],   # footprints with arrows
    "run":       [("img", 0x8A6807610D9129AB)],   # running figure
    "sneak":     [("img", 0x13270BDED8CB6A37),    # finger on the lips
                  ("img", 0xB5EDC0BB3ED2A86B)],   # burglar's beanie
    "talk":      [("img", 0xEA4AA419D39866FF)],   # pie menu category "Friendly" (46 x 46)
    "fight":     [("img", 0x73BDAA68F3D6554F),    # fist punching
                  ("img", 0x8E1B8F9070965325)],   # pie menu category "Mean"
    "auto":      [("img", 0xC4E49986DDF7EE0E)],   # golden shield
    "shoot":     [("img", 0xC7FEAE5A578F421F)],   # archery skill: bow and arrow (40 x 40)
    "magic":     [("img", 0x9A104C73EEF29664),    # wand and top hat, sparks
                  ("img", 0xE9C54F7DB2660AB4)],   # wand, sparks
    "take":      [("img", 0x621EF8498327FC98),    # hand holding an apple
                  ("img", 0x798CE053EAF9F2CE)],   # fingers picking something up
    "open":      [("img", 0xC9FBEA442D063F2D),    # golden key
                  ("img", 0xA7926ECF244790AB)],
    "close":     [("img", 0x235FD7428B835446)],   # padlock
    "lockpick":  [("img", 0x98D513091CA332B1),    # eye at a keyhole
                  ("img", 0x4E3074AC80DCCE70)],   # moodlet "lousy lockpicker"
    "use":       [("img", 0xA1E5C3E91B378951),    # pointing hand
                  ("img", 0xA680AF32012B20BE)],   # pie menu category "Upgrade": wrench
    "sleep":     [("img", 0x031D4087EC9F6558),    # bed, Zzz
                  ("img", 0xE522161C9AE24CCE)],   # trait "Lazy": recliner
    "sit":       [("img", 0xE37B9DFBD89FC9E6),    # wooden chair
                  ("img", 0x23E8B0E43B461F68)],
    "read":      [("img", 0x20A8966BF5C9C793),    # open book
                  ("img", 0xCFD751132B7F4196)],   # trait "Bookworm": books
    "cook":      [("img", 0xB5A02B3E3E2EA409)],   # home style cooking skill: frying pan (40 x 40)
    "drink":     [("img", 0x02ADF582CD0E0496),    # glass of something golden
                  ("img", 0x0DA2C38EA358F590)],   # glass of water
    "eat":       [("img", 0xFE356C25838AF559),    # biting a drumstick
                  ("img", 0xB2CD732013BAB94D)],   # trait "Glutton"
    "smith":     [("img", 0xD592AA90805547F9),    # hammer on an anvil
                  ("img", 0x3FB9709DD48DABC6)],   # hammer
    "pray":      [("img", 0x0B080D73AE0FFF20),    # meditating figure, glowing
                  ("img", 0xFA634129169466A0)],   # lotus
    "loot":      [("img", 0x44F87A7DA07D8CFB),    # sack of gems
                  ("img", 0x0402ACFCE579E8F8)],   # gold coins
    "steal":     [("img", 0x1179A319FA04D640)],   # trait "Kleptomaniac": gloved hand with a diamond (64 x 64)
    "finish":    [("img", 0x0FF4302DC3CDC7FC),    # skull
                  ("img", 0xA4B599C747D7BF81)],   # reaping skill: scythe
    "climb":     [("img", 0xD0336B6841E45188),    # wooden ladder
                  ("img", 0x9ECF483162FBF33B)],
    "heart":     [("img", 0x12B31030A3AF9DD9),    # red heart
                  ("img", 0xA842549B22A79623)],   # pie menu category "Romantic"
    "sword":     [("img", 0x3FDAE5054A11D1D7),    # sword in a stone
                  ("img", 0x28F1EBA073E5B1DC)],   # sabre
    "lever":     [("img", 0x2D62D7495B01C1A8)],   # gears (no lever icon in the game)
    "wheel":     [("img", 0x9227642BC6D0C33D),    # wooden wheel
                  ("img", 0x2D62D7495B01C1A8)],
    "look":      [("img", 0x8A8AFB10F4FAB2C5)],   # trait "Observant": binoculars (64 x 64)
    "dig":       [("img", 0xCA1AC6442D20B2D0),    # shovel
                  ("img", 0x807887CD9C94AB88)],   # gardening: trowel and fork
    "alchemy":   [("img", 0x0B3B7EEFA18852D4),    # bubbling cauldron
                  ("img", 0xC89DA500E72CDC2D)],   # trait "Potion Master"
    "fire":      [("img", 0x304FF71504FFFD8B),    # flame
                  ("img", 0x8849DAA3FE16D5C9)],   # moodlet: campfire
    "door":      [("img", 0x08DB07F3F7EBF90D)],   # pie menu category "Allow access": door
    "chest":     [("img", 0x9DB57159E7BF8C9E),    # treasure chest
                  ("img", 0x91FF242BC6AF0409)],
    "weapon":    [("img", 0x4CAC5CA5D5A514F7),    # short sword
                  ("img", 0x28F1EBA073E5B1DC)],
    "trade":     [("img", 0x1938D6109BF1D45D),    # handshake
                  ("img", 0x5FB3F87AA018DB67)],   # pie menu category "Retail": till
    "follow":    [("img", 0xA55E46C382034C39),    # pie menu category: one leading another (46 x 46)
                  ("img", 0xF2F01D7635AF9148)],   # footprints
    "standup":   [("ui", "hudlibrary", "library.HUD.AgeImage", 0, (31, 57))],  # standing adult
    "plumbob":   [("ui", "situationdisplay", "widgets.Gameplay.SituationDisplay.controls."
                   "GetawaySituationAssignmentCellView", 0, (59, 120)),
                  ("img", 0x58C01DA8FC53A041)],   # plumbob with an arrow (level up)
    # HUD buttons
    "stop":      [("ui", "interactionqueue", "PostureInteractionButton", 0, (36, 36)),  # the queue's X
                  ("img", 0xAB4A67C934164FB1)],   # prohibition sign
    "inventory": [("ui", "siminfotray", "widgets.Gameplay.SimInfoHUD.SimInfoTray.InventoryPanelButton", 0,
                   (28, 28))],
    "map":       [("ui", "gametray", "widgets.GameTray.controls.BackToNeighborhoodButton", 0, (51, 39))],
    "log":       [("ui", "gametray", "widgets.GameTray.controls.WallButton", 0, (20, 24))],  # notifications
    "status":    [("ui", "siminfotray", "widgets.Gameplay.SimInfoHUD.SummaryPanel.SummaryPanelButton", 0,
                   (28, 28))],
    "pause":     [("ui", "timecontrols", "TimeControlsPauseButton", 0, (20, 22))],
    "play":      [("ui", "timecontrols", "TimeControlsSpeedOneButton", 0, (26, 26))],
    "speed2":    [("ui", "timecontrols", "TimeControlsSpeedTwoButton", 0, (26, 20))],
    "speed3":    [("ui", "timecontrols", "TimeControlsSpeedThreeButton", 0, (28, 18))],
    "camera":    [("ui", "timecontrols", "widgets.TimeControls.CameraButton", 0, (31, 31))],
    "left":      [ARROW],
    "right":     [ARROW + ("flip",)],
    "up":        [ARROW + ("rot270",),
                  ("ui", "gametray", "widgets.GameTray.controls.FloorUpButton", 0, (24, 18))],
    "down":      [ARROW + ("rot90",),
                  ("ui", "gametray", "widgets.GameTray.controls.FloorDownButton", 0, (24, 18))],
    # extras: the speed buttons as the selected one (green), the queue's slot, the pie menu's
    # bubble and the speech balloon
    "pause_on":  [("ui", "timecontrols", "TimeControlsPauseButton", 1, (20, 22))],
    "play_on":   [("ui", "timecontrols", "TimeControlsSpeedOneButton", 1, (26, 26))],
    "speed2_on": [("ui", "timecontrols", "TimeControlsSpeedTwoButton", 1, (26, 20))],
    "speed3_on": [("ui", "timecontrols", "TimeControlsSpeedThreeButton", 1, (28, 18))],
    "queue_slot": [("ui", "interactionqueue", "InteractionQueue_fla.MixerInteractionButtonEmotionBg_21", 0,
                    (36, 36))],
    "pie_bubble": [("ui", "piemenu", "PieMenu_fla.PickerButtonBackground_6", 0, (169, 47))],
    "balloon":   [("ui", "objectboundelements", "ObjectBoundElements_fla.BalloonBackground_62", 0, (128, 128))],
}

# name -> the UI's sound effects that fit it (the first one found that can be decoded is used)
SOUNDS = {
    "click": ["ui_gameplay_button_primary_click", "ui_gameplay_button_secondary_click"],
    "pie_open": ["ui_piemenu_open"],
    "pie_select": ["ui_piemenu_select", "ui_action_select"],
    "pie_close": ["ui_piemenu_close"],
    "queue_cancel": ["ui_queue_item_cancel", "ui_action_cancel"],
    "queue_add": ["ui_queue_new_item"],
    "speed": ["ui_speed_2x_from_lesser", "ui_speed_1x_from_pause"],
    "pause": ["ui_speed_pause"],
    "plumbob": ["ui_world_select_sim", "ui_gameplay_button_select_sim"],
    "select": ["ui_select"],  # an extra: a soft swell of 1.1 s from the main menu, EA-XAS (decoded)
}

TRANSFORMS = {"flip": Image.FLIP_LEFT_RIGHT, "rot90": Image.ROTATE_90, "rot270": Image.ROTATE_270}


class Resources:
    """The game's own copy of the resources of some types: the client packages in load order, the
    last one holding a key wins (a deleted record removes the earlier ones)."""

    def __init__(self, sims_dir, types=None, part="Client"):
        self.entries = {}
        for path in s4files.load_order(sims_dir, part):
            try:
                pkg = Package(path, types=types)
            except (OSError, ValueError):
                continue
            for e in pkg.entries:
                if e.deleted:
                    self.entries.pop(e.key, None)
                else:
                    self.entries[e.key] = e
        self._by_instance = None

    def entry(self, key):
        return self.entries.get(tuple(key))

    def read(self, key):
        """A resource's bytes by its key (type, group, instance), or None."""
        e = self.entries.get(tuple(key))
        return e.package.read(e) if e else None

    def read_any(self, t, instance):
        """A resource of type `t` with this instance in any group (the largest one), or None."""
        if self._by_instance is None:
            self._by_instance = {}
            for e in self.entries.values():
                self._by_instance.setdefault((e.type, e.instance), []).append(e)
        found = self._by_instance.get((t, instance))
        if not found:
            return None
        e = max(found, key=lambda e: e.mem_size)
        return e.package.read(e)


def _ui_part(ui, movie, symbol, n, size):
    m = ui.movie(movie)
    if m is None or symbol not in m.symbols:
        return None
    same = [p for p in m.parts_of(m.symbols[symbol])
            if (m.parts[p][3] - m.parts[p][1], m.parts[p][4] - m.parts[p][2]) == tuple(size)]
    return ui.part(m, same[n]) if n < len(same) else None


def find_icons(sims_dir, names, log=None):
    """{name: PIL RGBA image} of the names found (see ICONS); the others are left out."""
    wanted = {n: ICONS[n] for n in names if n in ICONS}
    if log and len(wanted) < len(names):
        log(f"no icon known for: {', '.join(n for n in names if n not in ICONS)}")
    need_img = any(c[0] == "img" for cs in wanted.values() for c in cs)
    res = Resources(sims_dir, types=(IMG_TYPE,)) if need_img else None
    ui = None
    found = {}
    for name, choices in wanted.items():
        for choice in choices:
            img = None
            try:
                if choice[0] == "img":
                    data = res.read((IMG_TYPE, 0, choice[1])) if res else None
                    if data is None and res:
                        data = res.read_any(IMG_TYPE, choice[1])
                    img = s4image.decode(data) if data else None
                    transform = choice[2] if len(choice) > 2 else None
                else:
                    if ui is None:
                        try:
                            ui = s4gfx.UI(sims_dir)
                        except (OSError, ValueError) as e:
                            ui = False
                            if log:
                                log(f"no UI.package to read ({e})")
                    img = _ui_part(ui, *choice[1:5]) if ui else None
                    transform = choice[5] if len(choice) > 5 else None
            except (OSError, ValueError, KeyError, NotImplementedError) as e:
                if log:
                    log(f"{name}: {choice[:2]} unreadable ({e})")
                img = None
            if img is not None:
                if transform:
                    img = img.transpose(TRANSFORMS[transform])
                found[name] = img.convert("RGBA")
                break
        else:
            if log:
                log(f"{name}: not found in this install")
    return found


def find_font(sims_dir, bold=False):
    """The bytes of The Sims 4's UI font ("The Sims Sans SC", TrueType), or None."""
    res = Resources(sims_dir, types=(FONT_TYPE,))
    data = res.read((FONT_TYPE, 0, FONT_BOLD if bold else FONT_REGULAR))
    if data and data[:4] in (b"\x00\x01\x00\x00", b"OTTO", b"true"):
        return data
    fonts = [res.read(k) for k in sorted(res.entries)]
    named = [d for d in fonts if d and FONT_NAME in d]
    return named[0] if named else None


def find_sounds(sims_dir, names, log=None):
    """{name: (int16 samples, mono (n,) or stereo (n, 2), sample rate)} of the names found (see
    SOUNDS) whose sound this can decode; the others are left out."""
    res = None
    found = {}
    for name in names:
        for effect in SOUNDS.get(name, []):
            if res is None:
                res = Resources(sims_dir, types=(s4audio.PROPX_TYPE, s4audio.SNR_TYPE))
            h = s4gfx.fnv64(effect)
            propx = res.read((s4audio.PROPX_TYPE, 0, h))
            variants = s4audio.propx_variants(propx) if propx else [h]
            for v in variants:
                data = res.read_any(s4audio.SNR_TYPE, v)
                if data is None:
                    continue
                snr = s4audio.Snr(data)
                if snr.codec in (s4audio.XAS, s4audio.PCM16BE):
                    found[name] = (snr.decode(), snr.rate)
                    break
                if log:
                    log(f"{name}: {effect} is {snr.codec_name}, not decoded")
            if name in found:
                break
        if log and name not in SOUNDS:
            log(f"{name}: no sound known")
    return found


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("sims", nargs="?", default=s4files.DEFAULT_SIMS, help="The Sims 4's folder")
    parser.add_argument("--out", required=True, help="where to write them, outside this repository")
    parser.add_argument("--names", help="comma-separated icon names (default: all)")
    args = parser.parse_args()
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    out = os.path.abspath(args.out)
    if os.path.commonpath([out, REPO]) == REPO:
        sys.exit(f"{out} is inside the repository; game data must stay outside it")
    problems = s4files.check(args.sims)
    if problems:
        sys.exit("\n".join(problems))
    os.makedirs(out, exist_ok=True)
    names = args.names.split(",") if args.names else list(ICONS)
    icons = find_icons(args.sims, names, log=print)
    for name, img in icons.items():
        img.save(os.path.join(out, f"{name}.png"))
    s4gfx.contact_sheet(list(icons.items()), columns=10, cell=96).save(os.path.join(out, "_icons.png"))
    print(f"{len(icons)} of {len(names)} icons -> {out}")
    font = find_font(args.sims)
    if font:
        with open(os.path.join(out, "TheSimsSansSC.ttf"), "wb") as f:
            f.write(font)
    print(f"font: {'The Sims Sans SC, %d bytes' % len(font) if font else 'not found'}")
    sounds = find_sounds(args.sims, list(SOUNDS), log=print)
    for name, (pcm, rate) in sounds.items():
        s4audio.write_wav(os.path.join(out, f"{name}.wav"), pcm, rate)
    print(f"{len(sounds)} of {len(SOUNDS)} sounds decoded")


if __name__ == "__main__":
    main()
