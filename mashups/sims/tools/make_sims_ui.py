"""The Sims 4's UI for the Sims plugin: one atlas of its sprites, icons and fonts, and the table of it.

Writes to _work\\DATA\\TEXTURES\\_COMPILED:
  SIMS_UI-C.TEX  the atlas, 2048 wide (heights a power of two), one level, uncompressed ARGB_4444
                 (ZTEX format 6: Gothic's Direct3D 7 renderer has no 32-bit textures, it would
                 turn them into ARGB_1555 with one bit of alpha; 4444 keeps DXT3's 16 alpha levels
                 without its 4 x 4 blocks); --dxt3 writes DXT3 instead. Straight (not premultiplied)
                 alpha, every transparent pixel coloured as its nearest visible ones, so bilinear
                 filtering leaves no dark fringes. Soft alpha (shadows, glows) is dithered.
  SIMS_UI.TXT    the table (CRLF, in the game's code page), one record per line:
                   atlas <width> <height>
                   codepage <1250|1251|1252>
                   lang <pl|de|ru|en>
                   ref <height>        the screen height the sprites are drawn for at scale 1 (1080)
                   sprite <name> <x> <y> <w> <h>                 pixels in the atlas
                   slice <name> <left> <top> <right> <bottom>    9-slice insets of a stretchable sprite
                   font <name> <pixelSize> <lineHeight> <ascent>
                   glyph <font> <byte> <x> <y> <w> <h> <xoff> <yoff> <advance>
                                       offsets from the pen at the top of the line; pixels at the
                                       font's own size; advance may be fractional
                   space <font> <advance>
                   label <key> <text>  the UI's words in the game's language (the rest of the line)

Sprites (sizes at 1080 lines): white 8 x 8; pointer and pointer_hand 32 x 32 (the arrow's tip at
(1, 1), the finger's at (12, 2)); bubble, bubble_hover, bubble_disabled (the pie menu's pills),
tag, panel, bar, bar_back (9-slice); circle, ring, glow, button, button_hover, button_on, cancel,
target, plumbob; icon_<name> 96 x 96, a picture on a white round badge. The icons are The Sims 4's
own where s4icons.py finds them in the player's game; else the player's colour emoji font (Windows'
Segoe UI Emoji), else its outlines in one colour; the speed controls, the arrows, the winch's wheel
and the plumbob are drawn here. The text is The Sims 4's font where s4icons.py finds it and it has
every letter of the game's code page, else Windows' Segoe UI (regular and semibold). Nothing of either game or of Windows is kept in the
repository: all of it is read from the player's files at install time.

The language is the game's, guessed from the letters of Gothic.dat's texts (Polish 1250, Russian
1251, German or English 1252), or --lang.

usage: python make_sims_ui.py [--sims DIR] --gothic DIR [--lang pl|de|ru|en] [--dxt3] [--preview DIR]
  --preview DIR  also writes SIMS_UI_PREVIEW.PNG (the atlas as stored) and SIMS_UI_MOCKUP.PNG (a
                 pie menu, the queue and the HUD drawn from the atlas and its table) into DIR
"""
import argparse
import io
import math
import os
import struct
import sys
import unicodedata

# Gothic's side (asc, tex, vdf, daedalus, ...) is shared by every mashup: <repo>/tools
sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
from tex import FORMAT_ARGB_4444, FORMAT_DXT3, _encode_dxt3_numpy, write_raw_tex

try:
    from s4files import DEFAULT_SIMS
except Exception:  # the tool works without The Sims 4
    DEFAULT_SIMS = None

ATLAS_WIDTH = 2048
REF = 1080
PAD = 2  # transparent pixels around each picture (its cell is a multiple of 4: DXT3's blocks)
SS = 4   # shapes are drawn this many times larger, then reduced: anti-aliased
FONTS_DIR = os.path.join(os.environ.get("WINDIR", r"C:\Windows"), "Fonts")

# The Sims 4's colours, each channel a multiple of 17 so ARGB_4444 keeps them exactly
WHITE = (255, 255, 255)
NAVY = (34, 51, 85)          # text and pictograms (#1d2b4f-ish)
BLUE = (51, 136, 238)        # hover, active (#2f80ed-ish)
BLUE_DARK = (34, 102, 204)
GREEN = (102, 204, 51)       # "on", the plumbob (#6fd13e-ish)
GREEN_DARK = (68, 153, 34)
RED = (221, 68, 68)          # cancel
GREY = (221, 221, 221)       # disabled
LINE = (204, 221, 238)       # light borders
SHADOW = (17, 34, 68)

FONTS = [("small", 16, False), ("normal", 20, False), ("bold", 20, True), ("title", 28, True)]

# icon name: the emoji drawn for it (one code point), or None where it is drawn here
ICONS = {
    "go": "📍", "run": "🏃", "sneak": "🥷", "talk": "💬", "fight": "👊", "auto": "🛡", "shoot": "🏹",
    "magic": "✨", "take": "🫳", "open": "🔓", "close": "🔒", "lockpick": "🗝", "use": "⚙", "sleep": "🛏",
    "sit": "🪑", "read": "📖", "cook": "🍲", "drink": "🍺", "eat": "🍖", "smith": "🔨", "pray": "🙏",
    "loot": "💰", "steal": "🤏", "finish": "💀", "climb": "🪜", "heart": "🫀", "sword": "🗡", "lever": "🕹",
    "wheel": None, "stop": "🛑", "inventory": "🎒", "map": "🗺", "log": "📜", "status": "👤",
    "pause": None, "play": None, "speed2": None, "speed3": None, "camera": "🎥",
    "left": None, "right": None, "up": None, "down": None, "standup": "🧍", "look": "👁", "dig": "⛏",
    "alchemy": "⚗", "fire": "🔥", "door": "🚪", "chest": "🧰", "weapon": "⚔", "trade": "🤝",
    "follow": "👣", "plumbob": None,
}
ICON_SIZE = 96
EMOJI_BOX = 0.6  # of the icon: the emoji's longer side

# key: Polish, English, German, Russian
LABELS = {
    "go_here": ("Idź tutaj", "Go here", "Hierher gehen", "Идти сюда"),
    "run_here": ("Biegnij tutaj", "Run here", "Hierher rennen", "Бежать сюда"),
    "walk_here": ("Idź powoli tutaj", "Walk here", "Langsam hierher gehen", "Идти сюда шагом"),
    "sneak_here": ("Skradaj się tutaj", "Sneak here", "Hierher schleichen", "Красться сюда"),
    "talk": ("Porozmawiaj", "Talk", "Reden", "Поговорить"),
    "attack": ("Walcz", "Attack", "Angreifen", "Атаковать"),
    "shoot": ("Strzelaj", "Shoot", "Schießen", "Стрелять"),
    "cast": ("Rzuć czar", "Cast a spell", "Zaubern", "Колдовать"),
    "steal": ("Okradnij", "Pickpocket", "Bestehlen", "Обокрасть"),
    "follow": ("Idź za", "Follow", "Folgen", "Следовать"),
    "look": ("Przyjrzyj się", "Take a look", "Betrachten", "Осмотреть"),
    "loot": ("Przeszukaj", "Search", "Durchsuchen", "Обыскать"),
    "finish": ("Dobij", "Finish off", "Erledigen", "Добить"),
    "take": ("Podnieś", "Pick up", "Aufheben", "Поднять"),
    "open": ("Otwórz", "Open", "Öffnen", "Открыть"),
    "close": ("Zamknij", "Close", "Schließen", "Закрыть"),
    "pick_lock": ("Otwórz wytrychem", "Pick the lock", "Schloss knacken", "Взломать отмычкой"),
    "unlock": ("Otwórz kluczem", "Unlock", "Aufschließen", "Открыть ключом"),
    "use": ("Użyj", "Use", "Benutzen", "Использовать"),
    "sleep": ("Śpij", "Sleep", "Schlafen", "Спать"),
    "sit": ("Usiądź", "Sit down", "Hinsetzen", "Сесть"),
    "read": ("Czytaj", "Read", "Lesen", "Читать"),
    "cook": ("Gotuj", "Cook", "Kochen", "Готовить"),
    "smith": ("Kuj", "Forge", "Schmieden", "Ковать"),
    "sharpen": ("Naostrz", "Sharpen", "Schärfen", "Заточить"),
    "cool": ("Schłódź", "Cool", "Abkühlen", "Остудить"),
    "heat": ("Rozgrzej", "Heat", "Erhitzen", "Нагреть"),
    "pray": ("Módl się", "Pray", "Beten", "Молиться"),
    "drink": ("Pij", "Drink", "Trinken", "Пить"),
    "eat": ("Jedz", "Eat", "Essen", "Есть"),
    "dig": ("Kop rudę", "Mine ore", "Erz hacken", "Добывать руду"),
    "climb": ("Wejdź po drabinie", "Climb the ladder", "Leiter hochklettern", "Залезть по лестнице"),
    "pull_lever": ("Pociągnij dźwignię", "Pull the lever", "Hebel ziehen", "Потянуть рычаг"),
    "turn_wheel": ("Obróć kołowrót", "Turn the winch", "Winde drehen", "Повернуть ворот"),
    "press": ("Naciśnij", "Press", "Drücken", "Нажать"),
    "stab_heart": ("Przebij serce", "Pierce the heart", "Herz durchbohren", "Пронзить сердце"),
    "use_sword": ("Użyj miecza", "Use the sword", "Schwert benutzen", "Использовать меч"),
    "alchemy": ("Warz mikstury", "Brew potions", "Tränke brauen", "Варить зелья"),
    "make_rune": ("Twórz runy", "Make runes", "Runen herstellen", "Создавать руны"),
    "smoke": ("Zapal", "Smoke", "Rauchen", "Закурить"),
    "stand_up": ("Wstań", "Stand up", "Aufstehen", "Встать"),
    "left": ("W lewo", "Left", "Links", "Влево"),
    "right": ("W prawo", "Right", "Rechts", "Вправо"),
    "up": ("W górę", "Up", "Hoch", "Вверх"),
    "down": ("W dół", "Down", "Runter", "Вниз"),
    "stop": ("Przerwij", "Cancel", "Abbrechen", "Прервать"),
    "fight": ("Walcz", "Fight", "Kämpfen", "Сражаться"),
    "auto_fight": ("Auto-walka", "Auto fight", "Auto-Kampf", "Автобой"),
    "no_target": ("Wskaż przeciwnika", "Pick an enemy", "Wähle einen Gegner", "Укажите противника"),
    "draw": ("Dobądź broni", "Draw weapon", "Waffe ziehen", "Достать оружие"),
    "sheathe": ("Schowaj broń", "Sheathe weapon", "Waffe wegstecken", "Убрать оружие"),
    "inventory": ("Ekwipunek", "Inventory", "Inventar", "Инвентарь"),
    "status": ("Postać", "Character", "Charakter", "Персонаж"),
    "log": ("Dziennik", "Journal", "Tagebuch", "Дневник"),
    "map": ("Mapa", "Map", "Karte", "Карта"),
    "pause": ("Pauza", "Pause", "Pause", "Пауза"),
    "speed1": ("Normalnie", "Normal speed", "Normal", "Обычно"),
    "speed2": ("Szybko", "Fast", "Schnell", "Быстро"),
    "speed3": ("Bardzo szybko", "Very fast", "Sehr schnell", "Очень быстро"),
    "camera_follow": ("Kamera za postacią", "Camera follows the hero", "Kamera folgt dem Helden", "Камера следует за героем"),
    "gothic_controls": ("Sterowanie Gothica", "Gothic controls", "Gothic-Steuerung", "Управление Gothic"),
    "sims_controls": ("Sterowanie Simsów", "Sims controls", "Sims-Steuerung", "Управление Sims"),
    "cant_reach": ("Nie mogę tam dojść", "I can't get there", "Da komme ich nicht hin", "Я не могу туда пройти"),
    "busy": ("Jestem zajęty", "I'm busy", "Ich bin beschäftigt", "Я занят"),
    "queue": ("Kolejka", "Queue", "Warteschlange", "Очередь"),
    "target": ("Cel", "Target", "Ziel", "Цель"),
    "fighting": ("Walczy z", "Fighting", "Kämpft gegen", "Сражается с"),
    "wake_up": ("Obudź się", "Wake up", "Aufwachen", "Проснуться"),
    "trade": ("Handluj", "Trade", "Handeln", "Торговать"),
    "give": ("Daj", "Give", "Geben", "Отдать"),
    "jump": ("Skocz tam", "Jump there", "Dorthin springen", "Прыгнуть туда"),
    "magic": ("Czary", "Spells", "Zauber", "Заклинания"),
    "no_bow": ("Nie mam łuku ani kuszy", "I have no bow or crossbow", "Ich habe keinen Bogen", "У меня нет лука"),
    "no_spells": ("Nie znam żadnego czaru", "I know no spells", "Ich kenne keinen Zauber", "Я не знаю заклинаний"),
    "space_key": ("Spacja", "Space", "Leertaste", "Пробел"),
    "your_items": ("Twój ekwipunek", "Your things", "Deine Sachen", "Твои вещи"),
    "their_offer": ("Oferta handlarza", "Their offer", "Sein Angebot", "Его предложение"),
    "your_offer": ("Twoja oferta", "Your offer", "Dein Angebot", "Твоё предложение"),
    "take_all": ("Weź wszystko", "Take all", "Alles nehmen", "Взять всё"),
    "accept_trade": ("Handluj", "Trade", "Handeln", "Обменять"),
    "reset_trade": ("Wyczyść oferty", "Reset", "Zurücksetzen", "Сбросить"),
    "xfer_hint": ("Klik: cały stos, Shift+klik: jedna sztuka", "Click: the whole stack, Shift+click: one",
                  "Klick: der ganze Stapel, Umschalt+Klick: eins", "Клик: вся стопка, Shift+клик: одна штука"),
    "trade_hint": ("Klik: jedna sztuka, Shift+klik: cały stos", "Click: one, Shift+click: the whole stack",
                   "Klick: eins, Umschalt+Klick: der ganze Stapel", "Клик: одна штука, Shift+клик: вся стопка"),
}
LANGS = ("pl", "en", "de", "ru")
CODE_PAGES = {"pl": "cp1250", "en": "cp1252", "de": "cp1252", "ru": "cp1251"}


# --- the game's language --------------------------------------------------------------------------

def game_language(gothic_dir):
    """The language of Gothic's texts, from the letters of the string constants in Gothic.dat (its
    backup if the installer patched it): Russian by Cyrillic (1251), Polish by its letters (1250),
    German by its letters and words, else English; None without Gothic.dat."""
    from daedalus import Script
    path = os.path.join(gothic_dir, "_work", "DATA", "scripts", "_compiled", "Gothic.dat")
    if os.path.exists(path + ".orig"):
        path += ".orig"
    if not os.path.exists(path):
        return None
    script = Script(open(path, "rb").read())
    text = b" ".join(t for symbol in script.symbols if isinstance(symbol.content, list) for t in symbol.content)
    count = [0] * 256
    for b in text:
        count[b] += 1
    polish = sum(count[b] for b in (0xB9, 0xEA, 0xB3, 0x9C, 0xBF, 0x9F, 0xE6, 0xF1, 0xA5, 0xCA, 0xA3, 0x8C, 0xAF, 0x8F, 0xC6, 0xD1))
    german = sum(count[b] for b in (0xE4, 0xF6, 0xFC, 0xDF, 0xC4, 0xD6, 0xDC))
    cyrillic = sum(count[0xC0:0x100])
    if cyrillic > 3 * (polish + german) and cyrillic > 1000:
        return "ru"
    if polish > german:
        return "pl"
    words = lambda *ws: sum(text.count(w) for w in ws)
    return "de" if words(b" und ", b" der ", b" ich ") > words(b" and ", b" the ", b" you ") else "en"


# --- drawing -------------------------------------------------------------------------------------

def _np():
    import numpy
    return numpy


class Canvas:
    """A picture drawn SS times larger in its own pixel units (floats), reduced when done."""

    def __init__(self, w, h):
        from PIL import Image, ImageDraw
        self.w, self.h = w, h
        self.image = Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 0))
        self.draw = ImageDraw.Draw(self.image)

    @staticmethod
    def box(x0, y0, x1, y1):
        return [x0 * SS, y0 * SS, x1 * SS - 1, y1 * SS - 1]

    @staticmethod
    def points(pts):
        return [(x * SS, y * SS) for x, y in pts]

    def rounded(self, x0, y0, x1, y1, r, fill):
        self.draw.rounded_rectangle(self.box(x0, y0, x1, y1), radius=r * SS, fill=fill)

    def ellipse(self, x0, y0, x1, y1, fill):
        self.draw.ellipse(self.box(x0, y0, x1, y1), fill=fill)

    def polygon(self, pts, fill):
        self.draw.polygon(self.points(pts), fill=fill)

    def line(self, pts, width, fill):
        self.draw.line(self.points(pts), width=round(width * SS), fill=fill, joint="curve")
        for x, y in pts:  # round ends
            r = width / 2
            self.ellipse(x - r, y - r, x + r, y + r, fill)

    def done(self):
        from PIL import Image
        return self.image.resize((self.w, self.h), Image.BOX)


def with_shadow(picture, blur, dy, opacity, dx=0):
    """The picture over a soft shadow of itself (SHADOW, blurred, moved down)."""
    from PIL import Image, ImageFilter
    alpha = Image.new("L", picture.size, 0)
    alpha.paste(picture.getchannel("A"), (dx, dy))
    alpha = alpha.filter(ImageFilter.GaussianBlur(blur)).point(lambda v: int(v * opacity + 0.5))
    shadow = Image.new("RGBA", picture.size, SHADOW + (0,))
    shadow.putalpha(alpha)
    return Image.alpha_composite(shadow, picture)


def pill(w, h, body, radius, fill, border=None, border_width=0, shadow=None):
    """A rounded rectangle `body` (x0, y0, x1, y1) in a w x h picture, with a border and a shadow
    (blur, dy, opacity)."""
    c = Canvas(w, h)
    x0, y0, x1, y1 = body
    if border:
        c.rounded(x0, y0, x1, y1, radius, border + (255,))
        b = border_width
        c.rounded(x0 + b, y0 + b, x1 - b, y1 - b, radius - b, fill)
    else:
        c.rounded(x0, y0, x1, y1, radius, fill)
    picture = c.done()
    return with_shadow(picture, *shadow) if shadow else picture


def disc(size, diameter, fill, border=None, border_width=0, centre=None, shadow=None):
    c = Canvas(size, size)
    cx, cy = centre or (size / 2, size / 2)
    r = diameter / 2
    if border:
        c.ellipse(cx - r, cy - r, cx + r, cy + r, border + (255,))
        r -= border_width
    c.ellipse(cx - r, cy - r, cx + r, cy + r, fill)
    picture = c.done()
    return with_shadow(picture, *shadow) if shadow else picture


def outlined(shapes, outline, width, fill, size, lines=()):
    """Shapes (('rounded', x0, y0, x1, y1, r) | ('poly', pts) | ('ellipse', x0, y0, x1, y1)) filled
    with `fill` inside an `outline` `width` pixels wide all round them, and `lines` in the outline's
    colour over them."""
    c = Canvas(*size)
    for colour, grow in ((outline, width), (fill, 0)):
        for shape in shapes:
            kind = shape[0]
            if kind == "rounded":
                _, x0, y0, x1, y1, r = shape
                c.rounded(x0 - grow, y0 - grow, x1 + grow, y1 + grow, r + grow, colour + (255,))
            elif kind == "ellipse":
                _, x0, y0, x1, y1 = shape
                c.ellipse(x0 - grow, y0 - grow, x1 + grow, y1 + grow, colour + (255,))
            else:
                if grow:
                    c.line(list(shape[1]) + [shape[1][0]], 2 * grow, colour + (255,))
                c.polygon(shape[1], colour + (255,))
    for pts, w in lines:
        c.line(pts, w, outline + (255,))
    return c.done()


def pointer():
    """The arrow, white with a blue edge, its tip at (1, 1)."""
    arrow = [(2.6, 2.6), (2.6, 23.5), (7.6, 18.8), (11.2, 26.6), (14.6, 25.1), (11.0, 17.4), (18.0, 17.4)]
    picture = outlined([("poly", arrow)], BLUE_DARK, 1.5, WHITE, (32, 32))
    return with_shadow(picture, 1.2, 1, 0.45, dx=1)


def pointer_hand():
    """A pointing hand, white with a blue edge, the finger's tip at (12, 2)."""
    shapes = [
        ("rounded", 9.6, 3.5, 14.6, 19, 2.5),     # the index finger
        ("rounded", 14.2, 11.5, 18.6, 20, 2.2),   # the folded fingers
        ("rounded", 18.2, 12.5, 22.4, 21, 2.1),
        ("rounded", 22.0, 14.0, 25.8, 22, 1.9),
        ("rounded", 9.6, 15.5, 25.8, 29.5, 5),    # the palm
        ("poly", [(4.6, 17.6), (7.0, 15.6), (11.5, 20.5), (10.5, 24.5)]),  # the thumb
    ]
    lines = [([(14.4, 13.5), (14.4, 18.2)], 0.9), ([(18.4, 14.0), (18.4, 19.0)], 0.9), ([(22.2, 15.5), (22.2, 20.0)], 0.9)]
    picture = outlined(shapes, BLUE_DARK, 1.4, WHITE, (32, 32), lines)
    return with_shadow(picture, 1.2, 1, 0.45, dx=1)


def plumbob(w=40, h=80):
    """The plumbob: a green gem of two pyramids, its facets lit from the upper left."""
    c = Canvas(w, h)
    sx, sy = w / 40, h / 80
    P = lambda x, y: (x * sx, y * sy)
    top, bottom, left, right, front = P(20, 1.5), P(20, 78.5), P(1.5, 31), P(38.5, 31), P(16.5, 34)
    edge = (34, 119, 17, 255)
    c.polygon([top, right, bottom, left], edge)  # the silhouette's edge
    shrink = lambda p, k=1.1: (20 * sx + (p[0] - 20 * sx) * (1 - k / 20), 32 * sy + (p[1] - 32 * sy) * (1 - k / 40))
    t, b, l, r = (shrink(p) for p in (top, bottom, left, right))
    for pts, colour in (([t, l, front], (170, 238, 119)), ([t, front, r], (119, 221, 68)),
                        ([l, b, front], (85, 187, 51)), ([front, b, r], (51, 136, 34))):
        c.polygon(pts, colour + (255,))
    shine = Canvas(w, h)  # (ImageDraw replaces pixels: what is translucent goes on a layer of its own)
    light = (221, 255, 187, 170)
    shine.line([t, front], 0.9, light)
    shine.line([l, front], 0.8, light)
    shine.line([front, r], 0.6, (204, 255, 170, 110))
    shine.line([front, b], 0.6, (204, 255, 170, 90))
    gx, gy = P(11.5, 22)
    shine.draw.ellipse([(gx - 3.2 * sx) * SS, (gy - 7 * sy) * SS, (gx + 3.2 * sx) * SS, (gy + 7 * sy) * SS], fill=(255, 255, 255, 150))
    from PIL import Image
    return Image.alpha_composite(c.done(), shine.done())


def glow(size=128):
    np = _np()
    from PIL import Image
    y, x = np.mgrid[0:size, 0:size].astype(np.float32) + 0.5
    r = np.hypot(x - size / 2, y - size / 2) / (size / 2)
    a = np.clip(1 - r, 0, 1) ** 2 * 255
    out = np.zeros((size, size, 4), np.uint8)
    out[..., :3] = 255
    out[..., 3] = (a + 0.5).astype(np.uint8)
    return Image.fromarray(out, "RGBA")


def ring(size=64, width=4):
    c = Canvas(size, size)
    c.ellipse(1, 1, size - 1, size - 1, WHITE + (255,))
    c.draw.ellipse(c.box(1 + width, 1 + width, size - 1 - width, size - 1 - width), fill=(0, 0, 0, 0))
    return c.done()


def cancel():
    picture = disc(28, 24, RED + (255,), centre=(14, 13), shadow=(1, 1, 0.4))
    c = Canvas(28, 28)
    c.line([(10, 9), (18, 17)], 2.6, WHITE + (255,))
    c.line([(18, 9), (10, 17)], 2.6, WHITE + (255,))
    from PIL import Image
    return Image.alpha_composite(picture, c.done())


def target():
    """Sims 4's "go here" marker as it lies on the ground: a white ellipse with a ring inside."""
    from PIL import Image, ImageFilter
    c = Canvas(96, 64)
    c.ellipse(4, 6, 92, 58, (255, 255, 255, 255))
    c.draw.ellipse(c.box(9, 10, 87, 54), fill=(255, 255, 255, 70))
    c.draw.ellipse(c.box(30, 22, 66, 42), fill=(255, 255, 255, 220))
    c.draw.ellipse(c.box(34, 24.5, 62, 39.5), fill=(255, 255, 255, 70))
    picture = c.done()
    halo = picture.getchannel("A").filter(ImageFilter.GaussianBlur(3)).point(lambda v: int(v * 0.55))
    layer = Image.new("RGBA", picture.size, WHITE + (0,))
    layer.putalpha(halo)
    return Image.alpha_composite(layer, picture)


# --- icons ---------------------------------------------------------------------------------------

def badge(size=ICON_SIZE):
    """The white round badge every icon sits on, a light blue edge round it."""
    return disc(size, size - 4, WHITE + (255,), border=LINE, border_width=3)


def fit(picture, box):
    """The picture cropped to what it shows and scaled to fit a box x box square."""
    from PIL import Image
    bbox = picture.getchannel("A").getbbox()
    if bbox:
        picture = picture.crop(bbox)
    k = box / max(picture.size)
    return picture.resize((max(1, round(picture.width * k)), max(1, round(picture.height * k))), Image.LANCZOS)


def on_badge(picture, share=EMOJI_BOX, size=ICON_SIZE):
    from PIL import Image
    out = badge(size)
    pic = fit(picture, size * share)
    layer = Image.new("RGBA", out.size, (0, 0, 0, 0))
    layer.paste(pic, ((size - pic.width) // 2, (size - pic.height) // 2))
    return Image.alpha_composite(out, layer)


# The Sims 4's HUD symbols (from its UI atlases): white for its dark buttons, navy on the white badge
HUD_SYMBOLS = {"stop", "inventory", "status", "map", "log", "pause", "play", "speed2", "speed3", "camera",
               "left", "right", "up", "down", "standup"}


def sims_icon(picture, size=ICON_SIZE, name=None):
    """One of The Sims 4's icons on the badge: a square picture with no transparency fills the
    badge's inside (cut round), any other sits in it; the HUD's white symbols in navy."""
    from PIL import Image, ImageDraw
    picture = picture.convert("RGBA")
    if picture.getchannel("A").getextrema()[0] < 250:
        if name in HUD_SYMBOLS:
            np = _np()
            a = np.asarray(picture).astype(float)
            shade = np.empty_like(a)
            shade[..., :3] = NAVY
            shade[..., 3] = a[..., 3]
            picture = Image.fromarray(shade.astype(np.uint8), "RGBA")
        return on_badge(picture, 0.72, size)
    out = badge(size)
    inner = size - 10
    pic = picture.resize((inner * SS, inner * SS), Image.LANCZOS)
    mask = Image.new("L", pic.size, 0)
    ImageDraw.Draw(mask).ellipse([0, 0, pic.width - 1, pic.height - 1], fill=255)
    pic.putalpha(Image.fromarray(_np().minimum(_np().asarray(mask), _np().asarray(pic.getchannel("A")))))
    pic = pic.resize((inner, inner), Image.BOX)
    layer = Image.new("RGBA", out.size, (0, 0, 0, 0))
    layer.paste(pic, (5, 5))
    return Image.alpha_composite(out, layer)


class Emoji:
    """The player's emoji font: colour where Pillow draws its COLR layers, else its outlines."""

    def __init__(self):
        from PIL import ImageFont
        self.fonts = []
        for name in ("seguiemj.ttf", "seguisym.ttf"):
            try:
                self.fonts.append(ImageFont.truetype(os.path.join(FONTS_DIR, name), 160))
            except OSError:
                pass
        self.colour = bool(self.fonts) and self._colour_works()

    def _colour_works(self):
        np = _np()
        picture = self._draw(self.fonts[0], "🔥", True)
        a = np.asarray(picture).reshape(-1, 4).astype(int)
        a = a[a[:, 3] > 128]
        return len(a) > 0 and int((a[:, :3].max(1) - a[:, :3].min(1)).max()) > 80

    @staticmethod
    def _draw(font, ch, colour):
        from PIL import Image, ImageDraw
        picture = Image.new("RGBA", (240, 240), (0, 0, 0, 0))
        draw = ImageDraw.Draw(picture)
        if colour:
            draw.text((20, 20), ch, font=font, embedded_color=True)
        else:
            draw.text((20, 20), ch, font=font, fill=NAVY + (255,))
        return picture

    def get(self, ch):
        """(picture, 'colour emoji' | 'emoji outline') or (None, None) if no font has it."""
        for i, font in enumerate(self.fonts):
            if has_glyph(font, ch):
                colour = self.colour and i == 0
                return self._draw(font, ch, colour), "colour emoji" if colour else "emoji outline"
        return None, None


_NOTDEF = {}


def has_glyph(font, ch):
    """Whether the font has the character (else FreeType draws the .notdef glyph)."""
    key = id(font)
    if key not in _NOTDEF:
        m = font.getmask("\U000F0000")
        _NOTDEF[key] = (m.size, bytes(m))
    m = font.getmask(ch)
    return (m.size, bytes(m)) != _NOTDEF[key]


def pictogram(name, size=ICON_SIZE):
    """The icons drawn here: the speed controls and arrows (navy, as Sims 4's HUD), the winch's
    wheel and the plumbob."""
    from PIL import Image
    if name == "plumbob":
        return on_badge(plumbob(160, 320), 0.66, size)
    c = Canvas(size, size)
    k = size / 96
    s = lambda pts: [(48 * k + x * k, 48 * k + y * k) for x, y in pts]
    ink = NAVY + (255,)
    tri = lambda x, w=22, h=28: [(x, -h / 2), (x + w, 0), (x, h / 2)]
    if name == "play":
        c.polygon(s(tri(-9, 26, 32)), ink)
    elif name == "pause":
        c.rounded(*[48 * k + v * k for v in (-13, -15, -4, 15)], 2.5 * k, ink)
        c.rounded(*[48 * k + v * k for v in (4, -15, 13, 15)], 2.5 * k, ink)
    elif name == "speed2":
        for x in (-19, 0):
            c.polygon(s(tri(x, 19, 26)), ink)
    elif name == "speed3":
        for x in (-25, -9, 7):
            c.polygon(s(tri(x, 16, 24)), ink)
    elif name in ("left", "right", "up", "down"):
        arrow = [(-20, -5), (2, -5), (2, -15), (21, 0), (2, 15), (2, 5), (-20, 5)]
        turn = {"right": 0, "down": 90, "left": 180, "up": 270}[name]
        a = math.radians(turn)
        c.polygon(s([(x * math.cos(a) - y * math.sin(a), x * math.sin(a) + y * math.cos(a)) for x, y in arrow]), ink)
    elif name == "wheel":  # a winch's wheel, wood
        wood, dark, hub = (170, 102, 51, 255), (102, 51, 17, 255), (221, 170, 68, 255)
        for i in range(8):
            a = math.radians(i * 45 + 22.5)
            c.line(s([(0, 0), (31 * math.cos(a), 31 * math.sin(a))]), 4.2 * k, dark)
            c.line(s([(0, 0), (30 * math.cos(a), 30 * math.sin(a))]), 2.6 * k, wood)
        cx = cy = 48 * k
        for r, colour in ((22.5, dark), (20.5, wood), (16, (0, 0, 0, 0))):
            c.draw.ellipse([(cx - r * k) * SS, (cy - r * k) * SS, (cx + r * k) * SS, (cy + r * k) * SS], fill=colour)
        for i in range(8):
            a = math.radians(i * 45 + 22.5)
            c.line(s([(17 * math.cos(a), 17 * math.sin(a)), (20 * math.cos(a), 20 * math.sin(a))]), 2.6 * k, wood)
        for r, colour in ((7, dark), (5.5, hub)):
            c.draw.ellipse([(cx - r * k) * SS, (cy - r * k) * SS, (cx + r * k) * SS, (cy + r * k) * SS], fill=colour)
    picture = c.done()
    out = badge(size)
    return Image.alpha_composite(out, picture)


def icons(sims_dir):
    """{name: picture} and {name: where it came from}."""
    from PIL import Image
    found = {}
    if sims_dir and os.path.isdir(sims_dir):
        try:
            import s4icons
            found = s4icons.find_icons(sims_dir, list(ICONS)) or {}
        except Exception as e:  # no s4icons.py, or it failed: the fallbacks draw them all
            print(f"  The Sims 4's icons: not read ({type(e).__name__}: {e})")
            found = {}
    emoji = Emoji()
    out, source = {}, {}
    for name, ch in ICONS.items():
        picture = found.get(name)
        if picture is not None:
            try:
                out[name], source[name] = sims_icon(picture, name=name), "The Sims 4"
                continue
            except Exception:
                pass
        if ch is None:
            out[name], source[name] = pictogram(name), "drawn"
            continue
        picture, kind = emoji.get(ch)
        if picture is None:
            out[name], source[name] = pictogram("plumbob"), "drawn (no emoji)"
            continue
        if name == "talk" and kind == "colour emoji":  # a white bubble on a white badge: Sims blue
            np = _np()
            a = np.asarray(picture).copy()
            white = (a[..., :3].min(2) > 225) & (a[..., 3] > 0)
            a[white, :3] = (119, 170, 238)
            picture = Image.fromarray(a, "RGBA")
        if name == "heart":  # the Sleeper's hearts: a dagger through it
            dagger, _ = emoji.get("🗡")
            if dagger is not None:
                heart = fit(picture, 200)
                blade = fit(dagger, 170)
                both = Image.new("RGBA", (260, 260), (0, 0, 0, 0))
                both.alpha_composite(heart, (30, 40))
                both.alpha_composite(blade, (60, 10))
                picture = both
        out[name], source[name] = on_badge(picture), kind
    return out, source


# --- fonts ---------------------------------------------------------------------------------------

def load_fonts(sims_dir, page):
    """{name: (font, fallback font or None, pixel size)}: The Sims 4's font if s4icons.py finds it
    and it has every letter of the code page (Segoe UI behind it for the signs it lacks: one word in
    two typefaces would look wrong), else Windows' Segoe UI, semibold for the bold ones; and which."""
    from PIL import ImageFont
    sims_font = None
    if sims_dir and os.path.isdir(sims_dir):
        try:
            import s4icons
            sims_font = s4icons.find_font(sims_dir)
        except Exception:
            sims_font = None
    if sims_font:
        try:
            font = ImageFont.truetype(io.BytesIO(sims_font), 20)
            missing = "".join(ch for _, ch in printable(page) if ch.isalpha() and not has_glyph(font, ch))
        except OSError:
            missing = "(not a font)"
        if missing:
            print(f"  The Sims 4's font lacks {len(missing)} letters of {page}: the text is Segoe UI")
            sims_font = None

    def windows(bold, size):
        for name in (("seguisb.ttf", "segoeuib.ttf", "segoeui.ttf") if bold else ("segoeui.ttf",)):
            try:
                return ImageFont.truetype(os.path.join(FONTS_DIR, name), size)
            except OSError:
                pass
        return ImageFont.load_default(size)

    fonts = {}
    for name, size, bold in FONTS:
        fallback = windows(bold, size)
        fonts[name] = (ImageFont.truetype(io.BytesIO(sims_font), size), fallback, size) if sims_font else (fallback, None, size)
    return fonts, "The Sims 4's font" if sims_font else "Segoe UI"


def printable(page):
    """The code page's bytes that draw something: 0x21-0x7E and every letter or sign of 0x80-0xFF."""
    out = []
    for byte in list(range(0x21, 0x7F)) + list(range(0x80, 0x100)):
        try:
            ch = bytes([byte]).decode(page)
        except UnicodeDecodeError:
            continue
        if unicodedata.category(ch)[0] in "CZ":  # controls, the soft hyphen, the no-break space
            continue
        out.append((byte, ch))
    return out


def glyphs(fonts, page):
    """{font: (ascent, line height, space advance, {byte: (picture, xoff, yoff, advance)})}: white
    glyphs, their alpha the coverage, offsets from the pen at the top of the line."""
    from PIL import Image, ImageDraw
    out = {}
    for name, (font, fallback, size) in fonts.items():
        ascent, descent = font.getmetrics()
        table = {}
        for byte, ch in printable(page):
            use = font
            if fallback is not None and not has_glyph(font, ch):
                use = fallback
            if not has_glyph(use, ch):
                continue
            x0, y0, x1, y1 = use.getbbox(ch, anchor="ls")
            advance = use.getlength(ch)
            if x1 <= x0 or y1 <= y0:
                continue
            mask = Image.new("L", (x1 - x0, y1 - y0), 0)
            ImageDraw.Draw(mask).text((-x0, -y0), ch, font=use, fill=255, anchor="ls")
            picture = Image.new("RGBA", mask.size, WHITE + (0,))
            picture.putalpha(mask)
            table[byte] = (picture, x0, ascent + y0, advance)
        out[name] = (ascent, ascent + descent, font.getlength(" "), table)
    return out


# --- the atlas -----------------------------------------------------------------------------------

def sprites(sims_dir):
    """{name: picture}, {name: 9-slice insets}, the icons' sources."""
    from PIL import Image
    out, slices = {}, {}
    out["white"] = Image.new("RGBA", (8, 8), WHITE + (255,))
    out["pointer"] = pointer()
    out["pointer_hand"] = pointer_hand()
    bubble = dict(w=64, h=56, body=(6, 4, 58, 48), radius=20)
    out["bubble"] = pill(**bubble, fill=WHITE + (255,), border=LINE, border_width=1, shadow=(3, 2, 0.4))
    out["bubble_hover"] = pill(**bubble, fill=BLUE + (255,), border=BLUE_DARK, border_width=1, shadow=(3, 2, 0.45))
    out["bubble_disabled"] = pill(**bubble, fill=GREY + (255,), border=(204, 204, 204), border_width=1, shadow=(3, 2, 0.25))
    for name in ("bubble", "bubble_hover", "bubble_disabled"):
        slices[name] = (26, 24, 26, 28)
    out["tag"] = pill(40, 36, (2, 2, 38, 34), 10, NAVY + (221,))
    slices["tag"] = (12, 12, 12, 12)
    out["panel"] = pill(96, 96, (8, 6, 88, 86), 18, WHITE + (255,), border=LINE, border_width=1, shadow=(5, 3, 0.35))
    slices["panel"] = (26, 24, 26, 28)
    out["bar"] = pill(48, 20, (2, 2, 46, 18), 8, WHITE + (255,))
    out["bar_back"] = pill(48, 20, (2, 2, 46, 18), 8, SHADOW + (153,))
    slices["bar"] = slices["bar_back"] = (10, 9, 10, 9)
    out["circle"] = disc(64, 62, WHITE + (255,))
    out["ring"] = ring(64, 4)
    out["glow"] = glow(128)
    button = dict(size=64, diameter=56, centre=(32, 31), border_width=2, shadow=(2.5, 2, 0.45))
    out["button"] = disc(fill=WHITE + (255,), border=LINE, **button)
    out["button_hover"] = disc(fill=BLUE + (255,), border=BLUE_DARK, **button)
    out["button_on"] = disc(fill=GREEN + (255,), border=GREEN_DARK, **button)
    out["cancel"] = cancel()
    out["target"] = target()
    out["plumbob"] = plumbob()
    pictures, source = icons(sims_dir)
    for name, picture in pictures.items():
        out[f"icon_{name}"] = picture
    return out, slices, source


def bleed(picture):
    """The colour of every fully transparent pixel made its nearest visible ones' (alpha-weighted
    averages over growing squares), so filtering at the edges mixes in no black."""
    np = _np()
    from PIL import Image
    a = np.asarray(picture).astype(np.float32)
    alpha = a[..., 3:] / 255
    if alpha.min() > 0 or alpha.max() == 0:
        return picture
    pre, weight, rgb = a[..., :3] * alpha, alpha, a[..., :3].copy()
    todo = alpha[..., 0] == 0

    def box(x, r):
        c = np.pad(x, ((r + 1, r), (r + 1, r), (0, 0)), mode="constant").cumsum(0).cumsum(1)
        return c[2 * r + 1:, 2 * r + 1:] - c[:-2 * r - 1, 2 * r + 1:] - c[2 * r + 1:, :-2 * r - 1] + c[:-2 * r - 1, :-2 * r - 1]
    r = 1
    while todo.any() and r < 4 * max(a.shape[:2]):
        p, w = box(pre, r), box(weight, r)
        done = todo & (w[..., 0] > 1e-3)
        rgb[done] = p[done] / w[done]
        todo &= ~done
        r *= 2
    out = np.concatenate([np.clip(rgb + 0.5, 0, 255), a[..., 3:]], 2).astype(np.uint8)
    return Image.fromarray(out, "RGBA")


def pack(pictures, width=ATLAS_WIDTH):
    """Skyline packing, tallest first, each where its top ends lowest: {key: (x, y, w, h)} of the
    pictures and the atlas height (a power of two). Each cell is the picture and PAD pixels round it,
    rounded up to a multiple of 4."""
    up4 = lambda v: (v + 3) // 4 * 4
    order = sorted(pictures, key=lambda k: (-pictures[k].height, -pictures[k].width, k))
    skyline = [(0, width, 0)]  # the bottom of the free space: (x, width, y) segments, left to right
    places = {}
    for key in order:
        w, h = up4(pictures[key].width + 2 * PAD), up4(pictures[key].height + 2 * PAD)
        best = None
        for x, _, _ in skyline:
            if x + w > width:
                break
            y = max(sy for sx, sw, sy in skyline if sx < x + w and sx + sw > x)
            if best is None or (y, x) < best:
                best = (y, x)
        y, x = best
        places[key] = (x + PAD, y + PAD, pictures[key].width, pictures[key].height)
        rest = []
        for sx, sw, sy in skyline:
            if sx < x:
                rest.append((sx, min(sw, x - sx), sy))
            if sx + sw > x + w:
                start = max(sx, x + w)
                rest.append((start, sx + sw - start, sy))
        rest.append((x, w, y + h))
        skyline = []
        for seg in sorted(rest):
            if skyline and skyline[-1][2] == seg[2]:
                skyline[-1] = (skyline[-1][0], skyline[-1][1] + seg[1], seg[2])
            else:
                skyline.append(seg)
    top = max(sy for _, _, sy in skyline)
    height = 1
    while height < top:
        height *= 2
    return places, height


BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]


def to_4444(atlas, dither):
    """RGBA quantized to ARGB_4444's levels (multiples of 17): colours to the nearest, alpha by a
    4 x 4 ordered dither where `dither` is set (soft shadows and glows keep their gradient), to the
    nearest elsewhere (the glyphs). Values already on a level stay as they are."""
    np = _np()
    a = atlas.astype(np.float32) * 15 / 255
    h, w = a.shape[:2]
    t = (np.array(BAYER, np.float32)[np.arange(h)[:, None] % 4, np.arange(w)[None, :] % 4] + 0.5) / 16
    q = np.floor(a + 0.5)
    q[..., 3] = np.where(dither, np.floor(a[..., 3] + t), q[..., 3])
    return (np.clip(q, 0, 15) * 17).astype(np.uint8)


def build(sims_dir, gothic_dir, lang=None, dxt3=False, preview=None):
    np = _np()
    from PIL import Image
    if lang is None:
        lang = game_language(gothic_dir)
        if lang is None:
            print("  Gothic.dat not found: the UI is in English (--lang sets it)")
            lang = "en"
    page = CODE_PAGES[lang]
    pictures, slices, source = sprites(sims_dir)
    fonts, font_source = load_fonts(sims_dir, page)
    font_glyphs = glyphs(fonts, page)
    cells = {f"sprite {name}": picture for name, picture in pictures.items()}
    for font, (_, _, _, table) in font_glyphs.items():
        for byte, (picture, _, _, _) in table.items():
            cells[f"glyph {font} {byte}"] = picture
    places, height = pack(cells)
    atlas = np.zeros((height, ATLAS_WIDTH, 4), np.uint8)
    dither = np.zeros((height, ATLAS_WIDTH), bool)
    for key, picture in cells.items():
        x, y, w, h = places[key]
        cell = Image.new("RGBA", (w + 2 * PAD, h + 2 * PAD), (0, 0, 0, 0))
        cell.paste(picture, (PAD, PAD))
        if key.startswith("glyph "):
            atlas[y - PAD:y + h + PAD, x - PAD:x + w + PAD] = np.asarray(cell)
            atlas[y - PAD:y + h + PAD, x - PAD:x + w + PAD, :3] = 255
        else:
            atlas[y - PAD:y + h + PAD, x - PAD:x + w + PAD] = np.asarray(bleed(cell))
            dither[y - PAD:y + h + PAD, x - PAD:x + w + PAD] = True
    out = os.path.join(gothic_dir, "_work", "DATA", "TEXTURES", "_COMPILED")
    os.makedirs(out, exist_ok=True)
    path = os.path.join(out, "SIMS_UI-C.TEX")
    if dxt3:
        stored = atlas
        header = b"ZTEX" + struct.pack("<8I", 0, FORMAT_DXT3, ATLAS_WIDTH, height, 1, ATLAS_WIDTH, height, 0xFF808080)
        with open(path, "wb") as f:
            f.write(header + _encode_dxt3_numpy(np, ATLAS_WIDTH, height, atlas.tobytes()))
    else:
        stored = to_4444(atlas, dither)
        write_raw_tex(path, ATLAS_WIDTH, height, stored.tobytes(), FORMAT_ARGB_4444)

    def text(s):
        return s.encode(page, "replace").decode("latin-1")
    lines = [f"atlas {ATLAS_WIDTH} {height}", f"codepage {page[2:]}", f"lang {lang}", f"ref {REF}"]
    for name in pictures:
        x, y, w, h = places[f"sprite {name}"]
        lines.append(f"sprite {name} {x} {y} {w} {h}")
    for name, (left, top, right, bottom) in slices.items():
        lines.append(f"slice {name} {left} {top} {right} {bottom}")
    for name, (ascent, line_height, space, table) in font_glyphs.items():
        lines.append(f"font {name} {fonts[name][2]} {line_height} {ascent}")
        for byte, (_, xoff, yoff, advance) in sorted(table.items()):
            x, y, w, h = places[f"glyph {name} {byte}"]
            lines.append(f"glyph {name} {byte} {x} {y} {w} {h} {xoff} {yoff} {advance:g}")
        lines.append(f"space {name} {space:g}")
    column = LANGS.index(lang)
    for key, texts in LABELS.items():
        lines.append(f"label {key} {text(texts[column])}")
    with open(os.path.join(out, "SIMS_UI.TXT"), "w", newline="\r\n", encoding="latin-1") as f:
        f.write("\n".join(lines) + "\n")
    kinds = {}
    for kind in source.values():
        kinds[kind] = kinds.get(kind, 0) + 1
    print(f"The Sims 4's UI: {len(pictures)} sprites ({len(ICONS)} icons: "
          + ", ".join(f"{n} {k}" for k, n in sorted(kinds.items())) + f"), {font_source} "
          + ", ".join(f"{name} {fonts[name][2]} px {len(g[3])} glyphs" for name, g in font_glyphs.items())
          + f" ({page}, {lang}), atlas {ATLAS_WIDTH} x {height} {'DXT3' if dxt3 else 'ARGB_4444'}")
    if preview:
        write_preview(preview, stored, os.path.join(out, "SIMS_UI.TXT"))


# --- the preview (only with --preview: never into the game or the repository) ----------------------

def read_table(path):
    sprites_, slices_, fonts_, labels = {}, {}, {}, {}
    page = "cp1252"
    for line in open(path, "rb").read().decode("latin-1").splitlines():
        parts = line.split(" ")
        kind = parts[0]
        if kind == "codepage":
            page = "cp" + parts[1]
        elif kind == "sprite":
            sprites_[parts[1]] = tuple(int(v) for v in parts[2:6])
        elif kind == "slice":
            slices_[parts[1]] = tuple(int(v) for v in parts[2:6])
        elif kind == "font":
            fonts_[parts[1]] = {"line": int(parts[3]), "ascent": int(parts[4]), "glyphs": {}, "space": 5.0}
        elif kind == "glyph":
            fonts_[parts[1]]["glyphs"][int(parts[2])] = [int(v) for v in parts[3:9]] + [float(parts[9])]
        elif kind == "space":
            fonts_[parts[1]]["space"] = float(parts[2])
        elif kind == "label":
            labels[parts[1]] = line.split(" ", 2)[2].encode("latin-1").decode(page)
    return sprites_, slices_, fonts_, labels, page


def write_preview(folder, stored, table_path):
    """SIMS_UI_PREVIEW.PNG: the atlas as stored, over a checkerboard; SIMS_UI_MOCKUP.PNG: a scene drawn
    the way the plugin draws, from the atlas and the table only."""
    np = _np()
    from PIL import Image
    os.makedirs(folder, exist_ok=True)
    atlas = Image.fromarray(stored, "RGBA")
    h, w = stored.shape[:2]
    check = ((np.arange(h)[:, None] // 16 + np.arange(w)[None, :] // 16) % 2).astype(np.uint8)
    back = np.stack([90 + 40 * check] * 3 + [np.full((h, w), 255, np.uint8)], 2).astype(np.uint8)
    Image.alpha_composite(Image.fromarray(back, "RGBA"), atlas).save(os.path.join(folder, "SIMS_UI_PREVIEW.PNG"))
    sprites_, slices_, fonts_, labels, page = read_table(table_path)

    scene = Image.new("RGBA", (1280, 720))
    sy = np.linspace(0, 1, 720)[:, None]
    sky = np.array([110, 140, 120]) * (1 - sy) + np.array([60, 80, 55]) * sy
    scene = Image.fromarray(np.dstack([np.repeat(sky[:, None, :], 1280, 1).astype(np.uint8)[:, :, i] for i in range(3)]
                                      + [np.full((720, 1280), 255, np.uint8)]), "RGBA")

    def tinted(picture, colour):
        if colour is None:
            return picture
        a = np.asarray(picture).astype(np.float32)
        a[..., :3] *= np.array(colour[:3], np.float32) / 255
        if len(colour) > 3:
            a[..., 3] *= colour[3] / 255
        return Image.fromarray(a.astype(np.uint8), "RGBA")

    px = lambda v: int(math.floor(v + 0.5))  # (round() rounds halves to even: seams between the slices)

    def blit(name, x, y, w=None, h=None, colour=None):
        sx, sy_, sw, sh = sprites_[name]
        picture = atlas.crop((sx, sy_, sx + sw, sy_ + sh))
        if w is not None:
            picture = picture.resize((px(w), px(h)), Image.BILINEAR)
        scene.alpha_composite(tinted(picture, colour), (px(x), px(y)))

    def nine(name, x, y, w, h, colour=None):
        sx, sy_, sw, sh = sprites_[name]
        left, top, right, bottom = slices_[name]
        x, y, w, h = px(x), px(y), px(w), px(h)
        xs, us = [0, left, w - right, w], [0, left, sw - right, sw]
        ys, vs = [0, top, h - bottom, h], [0, top, sh - bottom, sh]
        for i in range(3):
            for j in range(3):
                tw, th = xs[i + 1] - xs[i], ys[j + 1] - ys[j]
                if tw <= 0 or th <= 0:
                    continue
                part = atlas.crop((sx + us[i], sy_ + vs[j], sx + us[i + 1], sy_ + vs[j + 1])).resize((tw, th), Image.BILINEAR)
                scene.alpha_composite(tinted(part, colour), (x + xs[i], y + ys[j]))

    def width(font, s):
        f = fonts_[font]
        return sum(f["glyphs"][b][6] if b in f["glyphs"] else f["space"] for b in s.encode(page, "replace"))

    def write(font, s, x, y, colour):
        f = fonts_[font]
        for b in s.encode(page, "replace"):
            g = f["glyphs"].get(b)
            if g is None:
                x += f["space"]
                continue
            gx, gy, gw, gh, xoff, yoff, advance = g
            picture = tinted(atlas.crop((gx, gy, gx + gw, gy + gh)), colour)
            scene.alpha_composite(picture, (px(x + xoff), px(y + yoff)))
            x += advance

    # the ground marker, the plumbob, the pie menu round a target, the hovered bubble blue
    blit("glow", 860, 470, 200, 120, (102, 204, 51, 140))
    blit("target", 900, 500, 120, 72, (102, 204, 51))
    blit("glow", 1150, 40, 80, 80, (102, 204, 51, 200))
    blit("plumbob", 1170, 40)
    cx, cy = 560, 330
    options = [("talk", "bubble"), ("attack", "bubble_hover"), ("steal", "bubble"), ("follow", "bubble"),
               ("look", "bubble"), ("trade", "bubble_disabled"), ("give", "bubble")]
    name = "Diego"
    tw = width("bold", name) + 28
    nine("tag", cx - tw / 2, cy - 18, tw, 36)
    write("bold", name, cx - tw / 2 + 14, cy - 18 + (36 - fonts_["bold"]["line"]) / 2, (255, 255, 255))
    for i, (key, sprite) in enumerate(options):
        a = math.radians(-90 + i * 360 / len(options))
        text = labels[key]
        bw = width("bold", text) + 2 * 26 + 8
        bx, by = cx + math.cos(a) * 190 - bw / 2, cy + math.sin(a) * 120 - 28
        nine(sprite, bx, by, bw, 56)
        colour = (255, 255, 255) if sprite == "bubble_hover" else (150, 160, 175) if sprite == "bubble_disabled" else (29, 43, 79)
        write("bold", text, bx + 30, by + 4 + (44 - fonts_["bold"]["line"]) / 2, colour)
    # the queue, top left: the current one large with its cancel, the next ones smaller
    blit("icon_fight", 20, 20, 72, 72)
    blit("cancel", 70, 18)
    for i, icon in enumerate(("icon_talk", "icon_loot", "icon_sleep")):
        blit(icon, 100 + i * 54, 34, 48, 48)
    write("small", labels["queue"], 22, 96, (255, 255, 255))
    # the HUD: a panel with the speed controls and the bars
    nine("panel", 20, 560, 470, 140)
    for i, (icon, button) in enumerate((("pause", "button"), ("play", "button_on"), ("speed2", "button"), ("speed3", "button_hover"))):
        blit(button, 40 + i * 70, 584)
        blit(f"icon_{icon}", 40 + i * 70 + 10, 584 + 9, 44, 44)
    write("title", labels["fighting"] + " Diego", 330, 588, (29, 43, 79))
    for i, (key, colour, share) in enumerate((("status", (221, 68, 68), 0.7), ("inventory", (51, 136, 238), 0.4))):
        y = 646 + i * 26
        nine("bar_back", 40, y, 270, 20)
        nine("bar", 40, y, max(20, 270 * share), 20, colour)
        write("small", labels[key], 320, y - 2, (29, 43, 79))
    # every icon at 48 and at 32 pixels
    names = [n for n in sprites_ if n.startswith("icon_")]
    for i, n in enumerate(names):
        blit(n, 760 + (i % 10) * 50, 70 + (i // 10) * 50, 48, 48)
        blit(n, 760 + (i % 18) * 28, 380 + (i // 18) * 34, 32, 32)
    write("normal", labels["go_here"] + " · " + labels["cant_reach"], 760, 20, (255, 255, 255))
    blit("pointer", 640, 352)
    blit("pointer_hand", 1000, 610)
    blit("pointer", 700, 120)
    scene.save(os.path.join(folder, "SIMS_UI_MOCKUP.PNG"))
    print(f"  preview: {os.path.join(folder, 'SIMS_UI_PREVIEW.PNG')}, {os.path.join(folder, 'SIMS_UI_MOCKUP.PNG')}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--sims", default=DEFAULT_SIMS, help="The Sims 4's folder (optional: without it the "
                        "icons and the font come from Windows' fonts)")
    parser.add_argument("--gothic", required=True, help="the Gothic 1 or Gothic 2 folder")
    parser.add_argument("--lang", choices=LANGS, help="the UI's language (default: the game's, from Gothic.dat)")
    parser.add_argument("--dxt3", action="store_true", help="write the atlas as DXT3 instead of ARGB_4444")
    parser.add_argument("--preview", metavar="DIR", help="also write preview PNGs into DIR")
    args = parser.parse_args()
    build(args.sims, args.gothic, args.lang, args.dxt3, args.preview)


if __name__ == "__main__":
    main()
