// The Sims 4's way of playing for the hero of Gothic 1 (GothicMod.exe 1.08k_mod) or Gothic 2 Night
// of the Raven (Gothic2.exe 2.6): he is played as a Sim. make_sims_ui.py draws The Sims 4's UI into
// one atlas (SIMS_UI-C.TEX, SIMS_UI.TXT: its pie bubbles, panels, icons, font and the texts in the
// game's language); this plugin brings the rest.
//
// Camera (live mode's): it looks at a point on the ground, not at the hero, from `distance` away and
// `pitch` above. W/A/S/D and the arrow keys move the point, Q/E turn round it, the wheel and Z/X
// zoom, the middle (or right) button held turns and tilts, the pointer at the screen's edge moves
// the point. Home (or the camera button in the HUD) brings it back to the hero and follows him again;
// moving it by hand stops the following. It goes through Gothic's world as The Sims' does; what is
// between it and the hero is cut away (see Cutaway). In dialogues it looks between him and whom he
// talks with; Gothic's own camera takes over in cutscenes and on death.
//
// The pointer: the mouse moves the plugin's own pointer (Gothic reads the mouse through
// DirectInput, as movement only). Under it: a ray from the camera through the pointer, the first
// interactive thing it meets before Gothic's world (someone, an item, an oCMobInter: chests, doors,
// beds, benches, levers, the Sleeper's hearts...) or the ground. Every interactive thing near the
// hero shows the icon of what he would do with it; the one under the pointer its name.
//
// A left click on the ground walks him there ("Go here": Gothic's movement AI, a route over the
// waynet when it is far, oCMsgMovement EV_GOTOPOS for the spot; Ctrl walks, a right click on the
// ground offers run, walk and sneak). A left click on something opens The Sims 4's pie menu: the
// thing's name in the middle, what he can do with it round it (talk, fight, follow, search a body,
// finish someone off, take, open, sleep, sit, use, stab the Sleeper's heart...), each with its icon.
// A choice goes into the queue (top left; Shift adds to it, else it replaces it; a click on a
// queued action cancels it). An action is Gothic's own: he walks to the thing (EV_GOTOVOB), turns to
// it, and Gothic's action key is given to him with the thing in his focus, so Gothic does what it
// does for the player (dialogues, lock picks, keys, the item a mob needs, chests' screens). While he
// uses a mob, buttons give him Gothic's direction keys (stand up, left, right, up, down: lock
// picks, wheels).
//
// Fighting: "Fight" on someone (the pie menu or the HUD's button: the one under the pointer, the
// last one clicked, else the nearest enemy) makes Gothic's combat AI fight for him: his weapon
// drawn, the enemy set, and the messages Gothic's characters fight with (oCMsgWeapon EV_ATTACK,
// AI_Attack's), with his fight tactic Gothic's master's (FAI_HUMAN_MASTER) while it lasts (fightAI=1:
// the script state ZS_ATTACK instead). "Auto fight": whoever attacks him (aims at him with a weapon
// drawn, or hits him), he fights back on his own. A fight ends when the enemy is down; the weapon
// goes away a few seconds later.
//
// HUD: the hero's panel (bottom left: the plumbob, his name, health and mana), the buttons (fight,
// auto fight, weapon, inventory, status, log, map, camera; the game's speed: pause, 1, 2, 3; Gothic's
// own controls), the queue, the plumbob over his head. Gothic's own screens (inventory, trading,
// chests, dialogues) stay Gothic's. F8 switches to Gothic's own controls and back.
//
// Settings, read from Gothic.ini section [SIMS] at start (defaults in brackets):
//   controls=1        The Sims 4's camera and controls (0: Gothic's own; F8 switches in game)
//   pointerSpeed=1.5  pointer pixels per mouse unit
//   takeSpeed=2.0     how much faster he picks things up (Gothic's animation sped up)
//   cutaway=1         walls, roofs and objects between the camera and him cut away (0: none)
//   cutRadius=400     cm round him kept when the world is cut away
//   edgeScroll=1      the pointer at the screen's edge moves the camera
//   uiScale=1.0       the size of the UI (1: The Sims 4's at 1080 lines)
//   autoFight=1       start with auto fight on
//   fightAI=0         0: Gothic's combat messages (AI_Attack) for the hero; 1: the script state ZS_ATTACK
//   icons=1           icons over everything he can use near him (0: only under the pointer)
//   plumbob=1         the plumbob over his head
//   bridge=0          1: a test bridge (Sims.cmd in the Gothic folder: commands; Sims.state: state)
//   debugLog=0        1: write Sims.log in the Gothic folder

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace GOTHIC_NAMESPACE
{
	namespace Sims
	{
		// This file is compiled once per engine: Gothic 1 and Gothic 2 Night of the Raven.
		static constexpr bool GOTHIC2 = ENGINE == Engine_G2A;

		struct Settings
		{
			bool controls = true;
			float pointerSpeed = 1.5f;
			float takeSpeed = 2.0f;
			bool cutaway = true;
			float cutRadius = 400.0f;
			bool edgeScroll = true;
			float uiScale = 1.0f;
			bool autoFight = true;
			int fightAI = 0;
			bool icons = true;
			bool plumbob = true;
			bool bridge = false;
			bool debugLog = false;
		};

		static Settings settings;
		static bool settingsLoaded = false;
		static const float PI = 3.14159265f;

		static void LoadSettings()
		{
			if (settingsLoaded || !zoptions)
				return;
			settingsLoaded = true;
			zSTRING s = "SIMS";
			settings.controls = zoptions->ReadBool(s, "controls", 1) != 0;
			settings.pointerSpeed = zoptions->ReadReal(s, "pointerSpeed", settings.pointerSpeed);
			settings.takeSpeed = zoptions->ReadReal(s, "takeSpeed", settings.takeSpeed);
			settings.cutaway = zoptions->ReadBool(s, "cutaway", 1) != 0;
			settings.cutRadius = zoptions->ReadReal(s, "cutRadius", settings.cutRadius);
			settings.edgeScroll = zoptions->ReadBool(s, "edgeScroll", 1) != 0;
			settings.uiScale = zoptions->ReadReal(s, "uiScale", settings.uiScale);
			settings.autoFight = zoptions->ReadBool(s, "autoFight", 1) != 0;
			settings.fightAI = zoptions->ReadInt(s, "fightAI", 0);
			settings.icons = zoptions->ReadBool(s, "icons", 1) != 0;
			settings.plumbob = zoptions->ReadBool(s, "plumbob", 1) != 0;
			settings.bridge = zoptions->ReadBool(s, "bridge", 0) != 0;
			settings.debugLog = zoptions->ReadBool(s, "debugLog", 0) != 0;
			if (settings.uiScale < 0.3f || settings.uiScale > 4.0f)
				settings.uiScale = 1.0f;
		}

		static void Log(const char* format, ...)
		{
			if (!settings.debugLog)
				return;
			FILE* file = nullptr;
			if (fopen_s(&file, "Sims.log", "a") || !file)
				return;
			va_list args;
			va_start(args, format);
			fprintf(file, "%8.2f  ", ztimer ? ztimer->totalTimeFloat / 1000.0f : 0.0f);
			vfprintf(file, format, args);
			fputc('\n', file);
			va_end(args);
			fclose(file);
		}

		// Crash diagnostics: the plugin's step, and every serious exception with where it happened and
		// the plugin's return addresses on the stack, logged before Gothic's or Union's handlers end the
		// game (the Minecraft mashup's).
		static const char* step = "start";
		static HMODULE pluginModule = nullptr;

		static void Where(void* address, char* out, size_t size)
		{
			HMODULE module = nullptr;
			char name[MAX_PATH] = "?";
			if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)address, &module)
				&& module)
				GetModuleFileNameA(module, name, MAX_PATH);
			const char* file = strrchr(name, '\\');
			sprintf_s(out, size, "%s+%X", file ? file + 1 : name, (unsigned)((char*)address - (char*)module));
		}

		static LONG CALLBACK OnException(EXCEPTION_POINTERS* info)
		{
			static int logged = 0;
			DWORD code = info->ExceptionRecord->ExceptionCode;
			if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_ILLEGAL_INSTRUCTION && code != EXCEPTION_INT_DIVIDE_BY_ZERO
				&& code != EXCEPTION_STACK_OVERFLOW && code != EXCEPTION_PRIV_INSTRUCTION)
				return EXCEPTION_CONTINUE_SEARCH;
			if (logged++ >= 5)
				return EXCEPTION_CONTINUE_SEARCH;
			char where[MAX_PATH + 16];
			Where(info->ExceptionRecord->ExceptionAddress, where, sizeof where);
			Log("EXCEPTION %08X at %s (address %p), plugin step: %s", code, where, (void*)info->ExceptionRecord->ExceptionInformation[1], step);
			if (pluginModule)
			{
				DWORD* stack = (DWORD*)info->ContextRecord->Esp;
				char* base = (char*)pluginModule;
				IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + ((IMAGE_DOS_HEADER*)base)->e_lfanew);
				DWORD size = nt->OptionalHeader.SizeOfImage;
				for (int i = 0; i < 2048; i++)
				{
					if (IsBadReadPtr(stack + i, 4))
						break;
					DWORD value = stack[i];
					if (value > (DWORD)base && value < (DWORD)base + size)
						Log("  plugin return address on the stack: Sims.dll+%X", value - (DWORD)base);
				}
			}
			return EXCEPTION_CONTINUE_SEARCH;
		}

		static void InstallCrashLog()
		{
			static bool done = false;
			if (done)
				return;
			done = true;
			GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&InstallCrashLog, &pluginModule);
			AddVectoredExceptionHandler(1, OnException);
		}

		// --- time and input ----------------------------------------------------------------------------
		static float Now() { return ztimer ? ztimer->totalTimeFloat / 1000.0f : 0.0f; } // game seconds
		static float RealNow() { return GetTickCount64() / 1000.0f; }                    // wall clock seconds

		static bool GameHasFocus()
		{
			DWORD process = 0;
			HWND window = GetForegroundWindow();
			return window && GetWindowThreadProcessId(window, &process) && process == GetCurrentProcessId();
		}

		static bool KeyDown(int key) { return zinput && zinput->KeyPressed(key) != 0 && GameHasFocus(); }

		// Gothic's own button query can stay 0 while it holds the mouse (the Witcher mashup)
		static bool ButtonDown(int vk) { return (GetAsyncKeyState(vk) & 0x8000) && GameHasFocus(); }

		struct Key
		{
			bool held = false, pressed = false, released = false;
			void Update(bool down)
			{
				pressed = down && !held;
				released = !down && held;
				held = down;
			}
		};

		static Key mouseLeft, mouseRight, mouseMiddle, keyShift, keyCtrl;
		static Key keySpace, keySpeed[3]; // The Sims' time keys (Space, 1, 2, 3), read from Windows
		static float pointerX = 400.0f, pointerY = 300.0f; // screen pixels
		static float mouseDX = 0.0f, mouseDY = 0.0f;       // this frame's movement (dragging the camera)
		static int wheelSteps = 0;
		static int screenW = 800, screenH = 600;
		static float ui = 1.0f; // screen pixels per pixel of the 1080-line reference
		static float U(float v) { return v * ui; }

		// --- The Sims 4's UI atlas (make_sims_ui.py) ----------------------------------------------------
		struct Sprite
		{
			char name[32];
			float x, y, w, h;
			float l, t, r, b; // 9-slice insets (0: none)
		};
		struct Glyph
		{
			bool ok;
			float x, y, w, h, xoff, yoff, advance;
		};
		struct Font
		{
			char name[16];
			float size = 20.0f, lineHeight = 24.0f, ascent = 18.0f, space = 6.0f;
			Glyph glyphs[256];
		};

		static std::vector<Sprite> sprites;
		static std::vector<Font> fonts;
		static std::unordered_map<std::string, std::string> labels;
		static zCTexture* atlas = nullptr;
		static float atlasW = 1.0f, atlasH = 1.0f, uiRef = 1080.0f;
		static bool uiLoaded = false, uiReady = false;
		static char gameLang[8] = "en";

		static zCOLOR Argb(int a, int r, int g, int b)
		{
			zCOLOR c;
			c.r = (unsigned char)r;
			c.g = (unsigned char)g;
			c.b = (unsigned char)b;
			c.alpha = (unsigned char)a;
			return c;
		}

		static const zCOLOR WHITE = Argb(255, 255, 255, 255);
		static const zCOLOR NAVY = Argb(255, 0x1d, 0x2b, 0x4f);
		static const zCOLOR BLUE = Argb(255, 0x2f, 0x80, 0xed);
		static const zCOLOR GREEN = Argb(255, 0x6f, 0xd1, 0x3e);
		static const zCOLOR RED = Argb(255, 0xe5, 0x3e, 0x3e);

		static void LoadUi()
		{
			if (uiLoaded)
				return;
			uiLoaded = true;
			FILE* file = nullptr;
			if (fopen_s(&file, "_work\\DATA\\TEXTURES\\_COMPILED\\SIMS_UI.TXT", "rb") || !file)
			{
				Log("ui: SIMS_UI.TXT missing (make_sims_ui.py writes it)");
				return;
			}
			char line[512];
			while (fgets(line, sizeof line, file))
			{
				char* end = line + strlen(line);
				while (end > line && (end[-1] == '\n' || end[-1] == '\r'))
					*--end = 0;
				char kind[16] = "";
				if (sscanf_s(line, "%15s", kind, (unsigned)sizeof kind) != 1)
					continue;
				if (!strcmp(kind, "atlas"))
					sscanf_s(line, "%*s %f %f", &atlasW, &atlasH);
				else if (!strcmp(kind, "ref"))
					sscanf_s(line, "%*s %f", &uiRef);
				else if (!strcmp(kind, "lang"))
					sscanf_s(line, "%*s %7s", gameLang, (unsigned)sizeof gameLang);
				else if (!strcmp(kind, "sprite"))
				{
					Sprite sp = {};
					if (sscanf_s(line, "%*s %31s %f %f %f %f", sp.name, (unsigned)sizeof sp.name, &sp.x, &sp.y, &sp.w, &sp.h) == 5)
						sprites.push_back(sp);
				}
				else if (!strcmp(kind, "slice"))
				{
					char name[32];
					float l, t, r, b;
					if (sscanf_s(line, "%*s %31s %f %f %f %f", name, (unsigned)sizeof name, &l, &t, &r, &b) == 5)
						for (Sprite& sp : sprites)
							if (!strcmp(sp.name, name))
							{
								sp.l = l;
								sp.t = t;
								sp.r = r;
								sp.b = b;
							}
				}
				else if (!strcmp(kind, "font"))
				{
					Font f;
					memset(f.glyphs, 0, sizeof f.glyphs);
					if (sscanf_s(line, "%*s %15s %f %f %f", f.name, (unsigned)sizeof f.name, &f.size, &f.lineHeight, &f.ascent) == 4)
						fonts.push_back(f);
				}
				else if (!strcmp(kind, "glyph"))
				{
					char name[16];
					int byte = -1;
					Glyph g = {};
					if (sscanf_s(line, "%*s %15s %d %f %f %f %f %f %f %f", name, (unsigned)sizeof name, &byte, &g.x, &g.y, &g.w, &g.h, &g.xoff,
						&g.yoff, &g.advance) == 9 && byte >= 0 && byte < 256)
						for (Font& f : fonts)
							if (!strcmp(f.name, name))
							{
								g.ok = true;
								f.glyphs[byte] = g;
							}
				}
				else if (!strcmp(kind, "space"))
				{
					char name[16];
					float advance = 0.0f;
					if (sscanf_s(line, "%*s %15s %f", name, (unsigned)sizeof name, &advance) == 2)
						for (Font& f : fonts)
							if (!strcmp(f.name, name))
								f.space = advance;
				}
				else if (!strcmp(kind, "label"))
				{
					char key[48];
					int at = 0;
					if (sscanf_s(line, "%*s %47s %n", key, (unsigned)sizeof key, &at) >= 1 && at > 0)
						labels[key] = line + at;
				}
			}
			fclose(file);
			zSTRING name = "SIMS_UI.TGA";
			atlas = zCTexture::Load(name, 0);
			if (atlas && zresMan)
				zresMan->CacheIn(atlas, -1.0f);
			if (atlas)
				atlas->cacheOutLock = TRUE;
			uiReady = atlas && !sprites.empty() && !fonts.empty();
			Log("ui: %d sprites, %d fonts, %d labels, language %s, atlas %.0f x %.0f %s", (int)sprites.size(), (int)fonts.size(),
				(int)labels.size(), gameLang, atlasW, atlasH, atlas ? "loaded" : "missing");
		}

		static const Sprite* FindSprite(const char* name)
		{
			for (const Sprite& sp : sprites)
				if (!strcmp(sp.name, name))
					return &sp;
			return nullptr;
		}

		static const Font* FindFont(const char* name)
		{
			for (const Font& f : fonts)
				if (!strcmp(f.name, name))
					return &f;
			return fonts.empty() ? nullptr : &fonts[0];
		}

		// a text of the UI in the game's language (SIMS_UI.TXT), else English
		static const char* Label(const char* key)
		{
			auto found = labels.find(key);
			if (found != labels.end())
				return found->second.c_str();
			static const char* const english[][2] = {
				{ "go_here", "Go here" }, { "run_here", "Run here" }, { "walk_here", "Walk here" }, { "sneak_here", "Sneak here" },
				{ "talk", "Talk" }, { "attack", "Fight" }, { "follow", "Follow" }, { "loot", "Search" }, { "finish", "Finish off" },
				{ "take", "Pick up" }, { "open", "Open" }, { "close", "Close" }, { "pick_lock", "Pick the lock" }, { "unlock", "Unlock" },
				{ "use", "Use" }, { "sleep", "Sleep" }, { "sit", "Sit" }, { "read", "Read" }, { "cook", "Cook" }, { "smith", "Forge" },
				{ "sharpen", "Sharpen" }, { "cool", "Cool" }, { "heat", "Heat" }, { "pray", "Pray" }, { "drink", "Drink" }, { "eat", "Eat" },
				{ "dig", "Mine ore" }, { "climb", "Climb" }, { "pull_lever", "Pull the lever" }, { "turn_wheel", "Turn the wheel" },
				{ "press", "Press" }, { "stab_heart", "Stab the heart" }, { "use_sword", "Use the sword" }, { "alchemy", "Brew" },
				{ "make_rune", "Make runes" }, { "smoke", "Smoke" }, { "stand_up", "Stand up" }, { "left", "Left" }, { "right", "Right" },
				{ "up", "Up" }, { "down", "Down" }, { "stop", "Cancel" }, { "fight", "Fight" }, { "auto_fight", "Auto fight" },
				{ "no_target", "Point at an enemy" }, { "draw", "Draw weapon" }, { "sheathe", "Sheathe weapon" }, { "inventory", "Inventory" },
				{ "status", "Character" }, { "log", "Log" }, { "map", "Map" }, { "pause", "Pause" }, { "speed1", "Normal speed" },
				{ "speed2", "Fast" }, { "speed3", "Very fast" }, { "camera_follow", "Camera on the hero" },
				{ "gothic_controls", "Gothic's controls" }, { "sims_controls", "The Sims controls" }, { "cant_reach", "I can't get there" },
				{ "busy", "I'm busy" }, { "queue", "Queue" }, { "target", "Target" }, { "fighting", "Fighting" }, { "trade", "Trade" },
				{ "shoot", "Shoot" }, { "cast", "Cast a spell" }, { "jump", "Jump there" }, { "magic", "Spells" },
				{ "no_bow", "I have no bow or crossbow" }, { "no_spells", "I know no spells" }, { "space_key", "Space" },
				{ "your_items", "Your things" }, { "their_offer", "Their offer" }, { "your_offer", "Your offer" }, { "take_all", "Take all" },
				{ "accept_trade", "Trade" }, { "reset_trade", "Reset" }, { "xfer_hint", "Click: the whole stack, Shift+click: one" },
				{ "trade_hint", "Click: one, Shift+click: the whole stack" },
			};
			for (auto& e : english)
				if (!strcmp(e[0], key))
					return e[1];
			return key;
		}

		// screen pixels -> a textured rectangle (u, v in atlas pixels)
		static void Quad(float x, float y, float w, float h, float u0, float v0, float u1, float v1, const zCOLOR& color)
		{
			if (!atlas || w <= 0.0f || h <= 0.0f)
				return;
			float px[4] = { x, x + w, x + w, x }, py[4] = { y, y, y + h, y + h };
			float pu[4] = { u0, u1, u1, u0 }, pv[4] = { v0, v0, v1, v1 };
			zTRndSimpleVertex v[4];
			for (int i = 0; i < 4; i++)
			{
				v[i].pos = zVEC2(px[i], py[i]);
				v[i].z = 1.0f;
				v[i].uv = zVEC2(pu[i] / atlasW, pv[i] / atlasH);
				v[i].color = color;
			}
			zrenderer->DrawPolySimple(atlas, v, 4);
		}

		static void DrawSprite(const char* name, float x, float y, float w, float h, const zCOLOR& color = WHITE)
		{
			if (const Sprite* sp = FindSprite(name))
				Quad(x, y, w, h, sp->x, sp->y, sp->x + sp->w, sp->y + sp->h, color);
		}

		// a 9-slice sprite stretched over (x, y, w, h); its corners drawn at `corner` screen pixels a sprite pixel
		static void DrawSlice(const char* name, float x, float y, float w, float h, const zCOLOR& color = WHITE)
		{
			const Sprite* sp = FindSprite(name);
			if (!sp)
				return;
			if (sp->l <= 0.0f && sp->t <= 0.0f && sp->r <= 0.0f && sp->b <= 0.0f)
			{
				Quad(x, y, w, h, sp->x, sp->y, sp->x + sp->w, sp->y + sp->h, color);
				return;
			}
			float k = ui;
			float l = sp->l * k, t = sp->t * k, r = sp->r * k, b = sp->b * k;
			if (l + r > w)
				l = r = w / 2.0f;
			if (t + b > h)
				t = b = h / 2.0f;
			float xs[4] = { x, x + l, x + w - r, x + w }, ys[4] = { y, y + t, y + h - b, y + h };
			float us[4] = { sp->x, sp->x + sp->l, sp->x + sp->w - sp->r, sp->x + sp->w };
			float vs[4] = { sp->y, sp->y + sp->t, sp->y + sp->h - sp->b, sp->y + sp->h };
			for (int j = 0; j < 3; j++)
				for (int i = 0; i < 3; i++)
					Quad(xs[i], ys[j], xs[i + 1] - xs[i], ys[j + 1] - ys[j], us[i], vs[j], us[i + 1], vs[j + 1], color);
		}

		static void Fill(float x, float y, float w, float h, const zCOLOR& color)
		{
			if (const Sprite* sp = FindSprite("white"))
				Quad(x, y, w, h, sp->x + sp->w / 2 - 0.5f, sp->y + sp->h / 2 - 0.5f, sp->x + sp->w / 2 + 0.5f, sp->y + sp->h / 2 + 0.5f, color);
		}

		// `px`: the text's pixel size on the 1080-line reference
		static float TextWidth(const char* fontName, const char* text, float px)
		{
			const Font* f = FindFont(fontName);
			if (!f || !text)
				return 0.0f;
			float s = U(px) / f->size, w = 0.0f;
			for (const unsigned char* c = (const unsigned char*)text; *c; c++)
				w += f->glyphs[*c].ok ? f->glyphs[*c].advance : f->space;
			return w * s;
		}

		static float TextHeight(const char* fontName, float px)
		{
			const Font* f = FindFont(fontName);
			return f ? f->lineHeight * U(px) / f->size : U(px);
		}

		static void DrawText(const char* fontName, const char* text, float x, float y, float px, const zCOLOR& color)
		{
			const Font* f = FindFont(fontName);
			if (!f || !text)
				return;
			float s = U(px) / f->size;
			for (const unsigned char* c = (const unsigned char*)text; *c; c++)
			{
				const Glyph& g = f->glyphs[*c];
				if (!g.ok)
				{
					x += f->space * s;
					continue;
				}
				if (g.w > 0.0f && g.h > 0.0f)
					Quad(x + g.xoff * s, y + g.yoff * s, g.w * s, g.h * s, g.x, g.y, g.x + g.w, g.y + g.h, color);
				x += g.advance * s;
			}
		}

		static void DrawTextCentered(const char* fontName, const char* text, float cx, float y, float px, const zCOLOR& color)
		{
			DrawText(fontName, text, cx - TextWidth(fontName, text, px) / 2.0f, y, px, color);
		}

		static void DrawIcon(const char* icon, float cx, float cy, float size, const zCOLOR& color = WHITE)
		{
			char name[48];
			sprintf_s(name, "icon_%s", icon);
			if (!FindSprite(name))
				strcpy_s(name, "icon_use");
			DrawSprite(name, cx - size / 2.0f, cy - size / 2.0f, size, size, color);
		}

		// a dark rounded tag with white text centred on (cx, y)
		static void DrawTag(const char* text, float cx, float y, float px = 18.0f, int alpha = 230)
		{
			float w = TextWidth("normal", text, px) + U(20.0f), h = TextHeight("normal", px) + U(8.0f);
			DrawSlice("tag", cx - w / 2.0f, y, w, h, Argb(alpha, 255, 255, 255));
			DrawTextCentered("normal", text, cx, y + U(4.0f), px, Argb(alpha, 255, 255, 255));
		}

		static bool Inside(float x, float y, float w, float h) { return pointerX >= x && pointerX < x + w && pointerY >= y && pointerY < y + h; }

		// --- the camera (live mode's) ---------------------------------------------------------------------
		static bool simsMode = true;    // F8: false = Gothic's own controls
		static bool cameraActive = false;
		static bool follow = true;      // the point follows the hero
		static zVEC3 camFocus(0.0f, 0.0f, 0.0f);
		static float camYaw = 0.0f, camPitch = 0.85f, camDistance = 1300.0f; // radians; cm
		static float wantDistance = 1300.0f;
		static zVEC3 camPos, camRight(1.0f, 0.0f, 0.0f), camUp(0.0f, 1.0f, 0.0f), camAt(0.0f, 0.0f, 1.0f);
		static bool camValid = false;
		static float ySign = 1.0f;     // the screen's y against the camera's up (checked once with zCCamera::Project)
		static bool ySignChecked = false;
		static const float MIN_DISTANCE = 250.0f, MAX_DISTANCE = 4500.0f;
		static const float MIN_PITCH = 0.15f, MAX_PITCH = 1.45f;

		static zVEC3 Flat(const zVEC3& v)
		{
			zVEC3 out(v[0], 0.0f, v[2]);
			float len = out.Length();
			return len > 0.0001f ? out * (1.0f / len) : zVEC3(0.0f, 0.0f, 1.0f);
		}

		static float RayBox(const zVEC3& from, const zVEC3& dir, const zTBBox3D& box)
		{
			float enter = 0.0f, leave = 1e30f;
			for (int i = 0; i < 3; i++)
			{
				if (fabsf(dir[i]) < 1e-6f)
				{
					if (from[i] < box.mins[i] || from[i] > box.maxs[i])
						return -1.0f;
					continue;
				}
				float a = (box.mins[i] - from[i]) / dir[i], b = (box.maxs[i] - from[i]) / dir[i];
				enter = (a < b ? a : b) > enter ? (a < b ? a : b) : enter;
				leave = (a > b ? a : b) < leave ? (a > b ? a : b) : leave;
				if (enter > leave)
					return -1.0f;
			}
			return enter;
		}

		static bool CameraModeOurs(zCAICamera* ai)
		{
			if (!ai || !ai->camVob)
				return false;
			zSTRING mode = ai->curcammode;
			mode.Upper();
			return mode != "CAMMODDEATH" && mode != "CAMMODFOCUS"; // (dialogues too: The Sims' camera stays)
		}

		static bool InDialog(); // (with the actions)

		// wall clock seconds since the last call, at most 0.1 (the camera keeps real time at any game
		// speed and while Gothic is paused)
		static float RealStep()
		{
			static LARGE_INTEGER frequency = {}, last = {};
			LARGE_INTEGER now;
			QueryPerformanceCounter(&now);
			if (!frequency.QuadPart)
			{
				QueryPerformanceFrequency(&frequency);
				last = now;
			}
			float dt = (float)(now.QuadPart - last.QuadPart) / (float)frequency.QuadPart;
			last = now;
			return dt < 0.0f ? 0.0f : (dt > 0.1f ? 0.1f : dt);
		}

		// the hero is played as a Sim now
		static bool Active(oCNpc* hero)
		{
			return settings.controls && simsMode && hero && ogame && !hero->IsDead();
		}

		// the ground under (x, z) near height y (Gothic's world below), or y
		static float GroundY(oCNpc* hero, const zVEC3& at)
		{
			zCWorld* world = hero ? hero->GetHomeWorld() : nullptr;
			if (!world)
				return at[1];
			zVEC3 from = at + zVEC3(0.0f, 150.0f, 0.0f);
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE | zTRACERAY_POLY_IGNORE_TRANSP;
			if (world->TraceRayNearestHit(from, zVEC3(0.0f, -2000.0f, 0.0f), hero, flags) && world->traceRayReport.foundHit)
				return world->traceRayReport.foundIntersection[1];
			return at[1];
		}

		static bool Alive(zCVob* vob); // (with the things he can use)

		// --- what hides him, cut away -----------------------------------------------------------------------
		// The Sims' camera goes through Gothic's world (no collision). When the world is between it and the
		// hero (a house's wall, a roof, a cave's rock), nothing nearer the camera than cutRadius short of him
		// is drawn: the camera's near clipping plane goes there (zCCamera::nearClipZ, smoothly), so he is
		// seen inside, with the room round him; clicks go through what is cut. Objects in the way (vobs:
		// rocks, trees, furniture, houses made of objects) are seen through (their visualAlpha). GD3D11, the
		// D3D11 renderer, draws with a near plane of its own (1 cm): with it the camera goes into the room
		// instead, when the world hides him under a roof.
		static bool gd3d11 = false, gd3d11Checked = false;
		static zCCamera* cutCamera = nullptr;
		static float gothicNear = -1.0f, cutNear = -1.0f;
		static float cutDistance = 0.0f; // the picking ray starts this far from the camera (what is cut)
		static float roomPull = 0.0f;    // GD3D11: cm the camera came towards him, into the room

		struct Faded
		{
			zCVob* vob; // referenced while faded
			float until, alpha;
			bool enabled;
		};
		static std::vector<Faded> faded;

		static bool UsingGD3D11()
		{
			if (!gd3d11Checked)
			{
				gd3d11Checked = true;
				// its renderer (GD3D11\Bin\g1*.dll, g2a*.dll, loaded by its ddraw.dll) exports GDX_*
				static const char* const DLLS[] = { "ddraw.dll", "g1.dll", "g1_avx.dll", "g1_avx2.dll", "g1a.dll", "g2a.dll", "g2a_avx.dll",
					"g2a_avx2.dll" };
				for (const char* name : DLLS)
					if (HMODULE dll = GetModuleHandleA(name))
						gd3d11 = gd3d11 || GetProcAddress(dll, "GDX_GetVersionString") != nullptr;
				Log("renderer: %s", gd3d11 ? "GD3D11 (the camera goes into rooms)" : "Gothic's (the near plane cuts walls away)");
			}
			return gd3d11;
		}

		static void Unfade(Faded& f)
		{
			if (Alive(f.vob))
			{
				f.vob->visualAlpha = f.alpha;
				f.vob->visualAlphaEnabled = f.enabled ? 1 : 0;
			}
			f.vob->Release();
		}

		static void UnfadeAll()
		{
			for (Faded& f : faded)
				Unfade(f);
			faded.clear();
		}

		// the objects between the camera and him seen through for a moment (a few rays to his body)
		static void FadeVobs(oCNpc* hero, const zVEC3& from)
		{
			zCWorld* world = hero->GetHomeWorld();
			float now = RealNow();
			if (world)
			{
				zCArray<zCVob*> ignore;
				ignore.Insert(hero);
				zVEC3 base = hero->GetPositionWorld();
				zVEC3 targets[3] = { base + zVEC3(0.0f, 70.0f, 0.0f), base + zVEC3(0.0f, -60.0f, 0.0f), base + zVEC3(0.0f, 10.0f, 0.0f) };
				int flags = zTRACERAY_STAT_IGNORE | zTRACERAY_VOB_IGNORE_CHARACTER | zTRACERAY_POLY_IGNORE_TRANSP;
				for (const zVEC3& target : targets)
					for (int n = 0; n < 4; n++)
					{
						if (!world->TraceRayNearestHit(from, target - from, &ignore, flags) || !world->traceRayReport.foundHit
							|| !world->traceRayReport.foundVob)
							break;
						zCVob* vob = world->traceRayReport.foundVob;
						ignore.Insert(vob);
						if (!vob->GetVisual() || zDYNAMIC_CAST<oCNpc>(vob))
							continue;
						bool known = false;
						for (Faded& f : faded)
							if (f.vob == vob)
							{
								f.until = now + 0.4f;
								known = true;
							}
						if (!known)
						{
							vob->AddRef();
							faded.push_back({ vob, now + 0.4f, vob->visualAlpha, vob->visualAlphaEnabled != 0 });
						}
					}
			}
			for (size_t i = 0; i < faded.size();)
				if (faded[i].until < now || !Alive(faded[i].vob))
				{
					Unfade(faded[i]);
					faded.erase(faded.begin() + i);
				}
				else
				{
					faded[i].vob->visualAlphaEnabled = 1;
					faded[i].vob->visualAlpha = 0.3f;
					i++;
				}
		}

		static void RestoreNearClip()
		{
			if (cutCamera && gothicNear >= 0.0f)
				cutCamera->nearClipZ = gothicNear;
			cutNear = -1.0f;
			cutDistance = 0.0f;
			roomPull = 0.0f;
		}

		// the camera's place with what hides him cut away (see above)
		static zVEC3 Cutaway(oCNpc* hero, zCAICamera* ai, const zVEC3& position, float dt)
		{
			zCCamera* cam = zCCamera::activeCam;
			if (cam != cutCamera)
			{
				cutCamera = cam;
				gothicNear = cam ? cam->nearClipZ : -1.0f;
				cutNear = -1.0f;
			}
			if (!settings.cutaway || !cam)
			{
				RestoreNearClip();
				return position;
			}
			FadeVobs(hero, position);
			zVEC3 chest = hero->GetPositionWorld() + zVEC3(0.0f, 40.0f, 0.0f);
			zVEC3 toHero = chest - position;
			float dist = toHero.Length();
			float hit = -1.0f; // how far from the camera Gothic's world hides him
			zCWorld* world = hero->GetHomeWorld();
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE | zTRACERAY_POLY_IGNORE_TRANSP;
			if (world && world->TraceRayNearestHit(position, toHero, (zCVob*)hero, flags) && world->traceRayReport.foundHit)
			{
				float t = (world->traceRayReport.foundIntersection - position).Length();
				if (t < dist - 80.0f)
					hit = t;
			}
			float k = 1.0f - expf(-dt * 8.0f);
			if (UsingGD3D11())
			{
				bool roof = false;
				if (hit >= 0.0f && world && world->TraceRayNearestHit(chest, zVEC3(0.0f, 1200.0f, 0.0f), (zCVob*)hero, flags)
					&& world->traceRayReport.foundHit)
					roof = true;
				float want = hit >= 0.0f && roof ? hit + 40.0f : 0.0f;
				want = want > dist - 120.0f ? dist - 120.0f : want;
				roomPull += (want - roomPull) * k;
				cutDistance = 0.0f;
				return position + toHero * (roomPull / (dist > 1.0f ? dist : 1.0f));
			}
			float base = gothicNear > 0.0f ? gothicNear : 35.0f;
			float target = hit >= 0.0f ? dist - settings.cutRadius : base;
			target = target < base ? base : target;
			cutNear = cutNear < 0.0f ? base : cutNear + (target - cutNear) * k;
			cam->nearClipZ = cutNear;
			cutDistance = cutNear > base + 1.0f ? cutNear : 0.0f;
			return position;
		}

		static Key camKeys[10];
		static void CenterOnHero(oCNpc* hero);

		static void UpdateCamera(zCAICamera* ai)
		{
			oCNpc* hero = oCNpc::player;
			bool dialog = hero && InDialog();
			bool active = Active(hero) && ai && ai->camVob && CameraModeOurs(ai) && (ai->target == hero || dialog);
			float dt = RealStep();
			if (!active)
			{
				if (cameraActive)
				{
					Log("camera: Gothic's");
					RestoreNearClip();
					UnfadeAll();
				}
				cameraActive = false;
				camValid = false;
				return;
			}
			if (!cameraActive) // take over: look at the hero from behind him
			{
				zVEC3 at = Flat(hero->GetAtVectorWorld());
				camYaw = atan2f(at[0], at[2]);
				camFocus = hero->GetPositionWorld();
				follow = true;
				cameraActive = true;
				Log("camera: The Sims'");
			}
			// the keys (not while Gothic's screens, its dialogue's choices or a mob take them)
			bool keysFree = GameHasFocus() && !dialog && !hero->GetInteractMob() && !(hero->inventory2.IsOpen());
			float forward = 0.0f, side = 0.0f, turn = 0.0f, zoom = 0.0f;
			if (keysFree)
			{
				forward = (KeyDown(KEY_W) || KeyDown(KEY_UPARROW) ? 1.0f : 0.0f) - (KeyDown(KEY_S) || KeyDown(KEY_DOWNARROW) ? 1.0f : 0.0f);
				side = (KeyDown(KEY_D) || KeyDown(KEY_RIGHTARROW) ? 1.0f : 0.0f) - (KeyDown(KEY_A) || KeyDown(KEY_LEFTARROW) ? 1.0f : 0.0f);
				turn = (KeyDown(KEY_E) ? 1.0f : 0.0f) - (KeyDown(KEY_Q) ? 1.0f : 0.0f);
				zoom = (KeyDown(KEY_X) ? 1.0f : 0.0f) - (KeyDown(KEY_Z) ? 1.0f : 0.0f);
				if (KeyDown(KEY_HOME))
					CenterOnHero(hero);
			}
			if (settings.edgeScroll && GameHasFocus() && !dialog && !mouseMiddle.held && !mouseRight.held)
			{
				float edge = 3.0f;
				if (pointerX <= edge)
					side -= 1.0f;
				else if (pointerX >= screenW - 1 - edge)
					side += 1.0f;
				if (pointerY <= edge)
					forward += 1.0f;
				else if (pointerY >= screenH - 1 - edge)
					forward -= 1.0f;
			}
			if (forward != 0.0f || side != 0.0f)
			{
				if (follow)
					Log("camera: moved by hand, not following");
				follow = false;
				zVEC3 ahead(sinf(camYaw), 0.0f, cosf(camYaw)), right(cosf(camYaw), 0.0f, -sinf(camYaw));
				float speed = (camDistance * 0.9f + 300.0f) * dt;
				camFocus += (ahead * forward + right * side) * speed;
				camFocus[1] = GroundY(hero, camFocus);
			}
			camYaw += turn * 1.8f * dt;
			if (mouseMiddle.held || mouseRight.held) // turning and tilting with a button held
			{
				camYaw += mouseDX * 0.006f;
				camPitch += mouseDY * 0.004f;
			}
			camPitch = camPitch < MIN_PITCH ? MIN_PITCH : (camPitch > MAX_PITCH ? MAX_PITCH : camPitch);
			if (wheelSteps != 0)
			{
				wantDistance *= powf(0.85f, (float)wheelSteps);
				wheelSteps = 0;
			}
			wantDistance *= 1.0f + zoom * 1.2f * dt;
			wantDistance = wantDistance < MIN_DISTANCE ? MIN_DISTANCE : (wantDistance > MAX_DISTANCE ? MAX_DISTANCE : wantDistance);
			camDistance += (wantDistance - camDistance) * (1.0f - expf(-dt * 10.0f));
			if (follow)
			{
				// in a dialogue between him and whom he talks with
				zVEC3 at = hero->GetPositionWorld();
				oCNpc* other = dialog ? oCInformationManager::GetInformationManager().Npc : nullptr;
				if (other && other != hero && (other->GetPositionWorld() - at).Length() < 1500.0f)
					at = (at + other->GetPositionWorld()) * 0.5f;
				float k = 1.0f - expf(-dt * 6.0f);
				camFocus += (at - camFocus) * k;
			}

			// the camera: from the point, `distance` back along the look and `pitch` above; it goes
			// through Gothic's world as The Sims' does (no collision), and what hides him is cut away
			zVEC3 look(sinf(camYaw) * cosf(camPitch), -sinf(camPitch), cosf(camYaw) * cosf(camPitch));
			zVEC3 pivot = camFocus + zVEC3(0.0f, 100.0f, 0.0f);
			zVEC3 position = pivot + look * -camDistance;
			position = Cutaway(hero, ai, position, dt);
			zVEC3 at0 = look;
			zVEC3 right0(at0[2], 0.0f, -at0[0]);
			right0.Normalize();
			zVEC3 up0(at0[1] * right0[2] - at0[2] * right0[1], at0[2] * right0[0] - at0[0] * right0[2], at0[0] * right0[1] - at0[1] * right0[0]);
			zMAT4 trafo = zMAT4::GetIdentity();
			trafo.SetRightVector(right0);
			trafo.SetUpVector(up0);
			trafo.SetAtVector(at0);
			trafo.SetTranslation(position);
			ai->camVob->SetTrafoObjToWorld(trafo);
			camPos = position;
			camRight = right0;
			camUp = up0;
			camAt = at0;
			camValid = true;
		}

		static void CenterOnHero(oCNpc* hero)
		{
			if (!follow)
				Log("camera: back on the hero");
			follow = true;
		}

		// The camera's projection (zCCamera's: the view distances and the viewport's centre)
		static bool CameraNumbers(float& xc, float& yc, float& vx, float& vy)
		{
			zCCamera* cam = zCCamera::activeCam;
			if (!cam || cam->viewDistanceX <= 0.0f || cam->viewDistanceY <= 0.0f)
				return false;
			xc = cam->vpData.xcenter;
			yc = cam->vpData.ycenter;
			vx = cam->viewDistanceX;
			vy = cam->viewDistanceY;
			if (!ySignChecked)
			{
				ySignChecked = true;
				zVEC3 up(0.0f, 100.0f, 1000.0f);
				float sx = 0.0f, sy = 0.0f;
				cam->Project(&up, sx, sy);
				ySign = sy <= yc ? 1.0f : -1.0f;
				Log("camera: projection centre %.0f x %.0f, view distances %.1f %.1f, up is %s on the screen", xc, yc, vx, vy,
					ySign > 0.0f ? "up" : "down");
			}
			return true;
		}

		// a world point on the screen; false behind the camera
		static bool Project(const zVEC3& p, float& sx, float& sy)
		{
			float xc, yc, vx, vy;
			if (!camValid || !CameraNumbers(xc, yc, vx, vy))
				return false;
			zVEC3 d = p - camPos;
			float cx = d.Dot(camRight), cy = d.Dot(camUp), cz = d.Dot(camAt);
			if (cz < 10.0f)
				return false;
			sx = xc + cx * vx / cz;
			sy = yc - ySign * cy * vy / cz;
			return true;
		}

		// the ray from the camera through a screen point
		static bool ScreenRay(float sx, float sy, zVEC3& from, zVEC3& dir)
		{
			float xc, yc, vx, vy;
			if (!camValid || !CameraNumbers(xc, yc, vx, vy))
				return false;
			from = camPos;
			dir = camAt + camRight * ((sx - xc) / vx) + camUp * (ySign * (yc - sy) / vy);
			dir.Normalize();
			return true;
		}

		// --- what can be used: people, items, mobs ---------------------------------------------------------
		// K_TARGET: a plain oCMOB Gothic's ranged focus takes (focusOverride): the shooting targets of the
		// Sleeper's temple and Irdorath's bridge switches
		enum Kind { K_NONE, K_GROUND, K_HERO, K_NPC, K_BODY, K_DOWN, K_ITEM, K_MOB, K_TARGET };

		static bool Alive(zCVob* vob) { return vob && vob->GetHomeWorld() != nullptr; }

		static int KindOf(oCNpc* hero, zCVob* vob)
		{
			if (!vob)
				return K_NONE;
			if (vob == hero)
				return K_HERO;
			if (oCNpc* npc = zDYNAMIC_CAST<oCNpc>(vob))
				return npc->IsDead() ? K_BODY : (npc->IsUnconscious() ? K_DOWN : K_NPC);
			if (zDYNAMIC_CAST<oCItem>(vob))
				return K_ITEM;
			if (zDYNAMIC_CAST<oCMobInter>(vob))
				return K_MOB;
			if (oCMOB* mob = zDYNAMIC_CAST<oCMOB>(vob))
				if (mob->focusOverride)
					return K_TARGET;
			return K_NONE;
		}

		// Gothic's scheme of a mob (its visual's name up to the first '_'), upper case
		static void SchemeOf(oCMobInter* mob, char* out, size_t size)
		{
			zSTRING scheme = mob->GetScemeName();
			strncpy_s(out, size, scheme.ToChar(), _TRUNCATE);
			_strupr_s(out, size);
		}

		struct Verb
		{
			const char* scheme; // the scheme's start
			const char* label;
			const char* icon;
		};

		// What using a mob is called, by its scheme, from the audit of both games' worlds and the hero's
		// model script (mashups/sims/research, local): the scheme itself, or it followed by '_' (doors and
		// beds, which are oCMobDoor in both games, come as X_FRONT and X_BACK). No label: the hero has no
		// animation for it, so Gothic can't use it for him (torch holders, lamps, the washing tub...).
		static const Verb MOB_VERBS[] = {
			{ "SPORTAL", "stab_heart", "heart" },  // the Sleeper's hearts (Gothic 1's end): the priests' swords
			{ "EXCALIBUR", "use_sword", "sword" }, // the sword stones of the Sleeper's temple
			{ "PRIESTGRAVE", "open", "chest" },
			{ "CHESTBIG", "open", "chest" }, { "CHESTSMALL", "open", "chest" }, { "DOOR", "open", "door" },
			{ "BED", "sleep", "sleep" }, { "BEDLOW", "sleep", "sleep" }, { "BEDHIGH", "sleep", "sleep" }, { "BABEBED", "sleep", "sleep" },
			{ "CHAIR", "sit", "sit" }, { "BENCH", "sit", "sit" }, { "THRONE", "sit", "sit" },
			{ "LADDER", "climb", "climb" },
			{ "ORE", "dig", "dig" }, { "TREASURE", "dig", "dig" },
			{ "LEVER", "pull_lever", "lever" }, { "VWHEEL", "turn_wheel", "wheel" }, { "TURNSWITCH", "turn_wheel", "wheel" },
			{ "TOUCHPLATE", "press", "lever" },
			{ "BOOK", "read", "read" },
			{ "CAULDRON", "cook", "cook" }, { "PAN", "cook", "cook" }, { "STOVE", "cook", "cook" }, { "BARBQ", "cook", "cook" },
			{ "LAB", "alchemy", "alchemy" }, { "RMAKER", "make_rune", "alchemy" }, { "HERB", "use", "alchemy" },
			{ "BSANVIL", "smith", "smith" }, { "BSFIRE", "heat", "fire" }, { "BSCOOL", "cool", "smith" }, { "BSSHARP", "sharpen", "smith" },
			{ "REPAIR", "use", "smith" }, { "BAUMSAEGE", "use", "use" }, { "BELLOW", "use", "fire" },
			{ "SMOKE", "smoke", "fire" }, { "BARRELO", "drink", "drink" },
			{ "INNOS", "pray", "pray" }, // with an item to use (the Adanos temple's door, Irdorath's dragon door): use, a door
			{ "PILLAR", "use", "use" },
			{ "MLUTE", "use", "use" }, { "MHARP", "use", "use" }, { "MCELLO", "use", "use" }, { "MDRUMSCHEIT", "use", "use" },
			{ "MPIPE", "use", "use" }, { "PAUKE", "use", "use" },
			{ "FIREPLACE", nullptr, nullptr }, { "PC", nullptr, nullptr }, { "OC", nullptr, nullptr }, { "DT", nullptr, nullptr },
			{ "STOMPER", nullptr, nullptr }, { "WASH", nullptr, nullptr }, { "ORCDRUM", nullptr, nullptr },
			{ "BATHTUB", nullptr, nullptr }, { "STONEMILL", nullptr, nullptr },
		};

		static bool SchemeIs(const char* scheme, const char* name)
		{
			size_t n = strlen(name);
			return !strncmp(scheme, name, n) && (scheme[n] == 0 || scheme[n] == '_');
		}

		// false: nothing the hero can do with it
		static bool MobVerb(oCMobInter* mob, const char*& label, const char*& icon)
		{
			char scheme[64];
			SchemeOf(mob, scheme, sizeof scheme);
			label = "use";
			icon = "use";
			bool known = false;
			for (const Verb& v : MOB_VERBS)
				if (SchemeIs(scheme, v.scheme))
				{
					if (!v.label)
						return false;
					label = v.label;
					icon = v.icon;
					known = true;
					break;
				}
			if (!known) // by its class
			{
				if (zDYNAMIC_CAST<oCMobContainer>(mob))
				{
					label = "open";
					icon = "chest";
				}
				else if (zDYNAMIC_CAST<oCMobDoor>(mob))
				{
					label = "open";
					icon = "door";
				}
				else if (zDYNAMIC_CAST<oCMobLadder>(mob))
				{
					label = "climb";
					icon = "climb";
				}
				else if (zDYNAMIC_CAST<oCMobWheel>(mob))
				{
					label = "turn_wheel";
					icon = "wheel";
				}
			}
			if (!strcmp(label, "pray") && mob->useWithItem.Length() > 0)
			{
				label = "use";
				icon = "door";
			}
			if (!strcmp(label, "open") && zDYNAMIC_CAST<oCMobDoor>(mob) && mob->GetState() > 0)
				label = "close";
			if (oCMobLockable* lock = zDYNAMIC_CAST<oCMobLockable>(mob))
				if (lock->locked && !strcmp(label, "open"))
				{
					oCNpc* hero = oCNpc::player;
					bool key = false;
					if (hero && lock->keyInstance.Length() > 0)
					{
						zSTRING instance = lock->keyInstance;
						instance.Upper();
						int index = parser ? parser->GetIndex(instance) : -1;
						key = index >= 0 && hero->IsInInv(index, 1) != nullptr;
					}
					label = key ? "unlock" : "pick_lock";
					icon = "lockpick";
				}
			return true;
		}

		// the name Gothic shows for it
		static void NameOf(zCVob* vob, char* out, size_t size)
		{
			out[0] = 0;
			if (oCNpc* npc = zDYNAMIC_CAST<oCNpc>(vob))
				strncpy_s(out, size, npc->GetName(0).ToChar(), _TRUNCATE);
			else if (oCItem* item = zDYNAMIC_CAST<oCItem>(vob))
			{
				if (item->amount > 1)
					sprintf_s(out, size, "%s (%d)", item->name.ToChar(), item->amount);
				else
					strncpy_s(out, size, item->name.ToChar(), _TRUNCATE);
			}
			else if (oCMOB* mob = zDYNAMIC_CAST<oCMOB>(vob))
				strncpy_s(out, size, mob->GetName().ToChar(), _TRUNCATE);
			if (!out[0])
				strncpy_s(out, size, vob->GetObjectName().ToChar(), _TRUNCATE);
		}

		static bool Hostile(oCNpc* hero, oCNpc* npc)
		{
			if (!npc || npc->IsDead() || npc->IsUnconscious())
				return false;
			if (npc->enemy == hero)
				return true;
			return npc->IsHostile(hero) != 0;
		}

		// the default action's icon over it, and its label
		static void DefaultVerb(oCNpc* hero, zCVob* vob, const char*& label, const char*& icon)
		{
			label = "use";
			icon = "use";
			switch (KindOf(hero, vob))
			{
			case K_NPC:
			{
				oCNpc* npc = (oCNpc*)vob;
				bool beast = npc->IsMonster() && !npc->IsOrc();
				if (beast || Hostile(hero, npc))
				{
					label = "attack";
					icon = "fight";
				}
				else
				{
					label = "talk";
					icon = "talk";
				}
				break;
			}
			case K_BODY: label = "loot"; icon = "loot"; break;
			case K_DOWN: label = "finish"; icon = "finish"; break;
			case K_ITEM: label = "take"; icon = "take"; break;
			case K_MOB: MobVerb((oCMobInter*)vob, label, icon); break;
			case K_TARGET: label = "shoot"; icon = "shoot"; break;
			default: break;
			}
		}

		// the interactive things near the hero and the camera, refreshed every 0.2 s; each one kept
		// (its reference) while listed
		static std::vector<zCVob*> things;
		static float thingsTime = -1.0f;

		static void ReleaseThings()
		{
			for (zCVob* vob : things)
				vob->Release();
			things.clear();
		}

		static void CollectThings(oCNpc* hero)
		{
			float now = RealNow();
			if (now - thingsTime < 0.2f && thingsTime >= 0.0f)
				return;
			thingsTime = now;
			ReleaseThings();
			zCWorld* world = hero->GetHomeWorld();
			if (!world || !world->bspTree.bspRoot)
				return;
			zVEC3 a = hero->GetPositionWorld(), b = camFocus;
			zTBBox3D box;
			for (int i = 0; i < 3; i++)
			{
				box.mins[i] = (a[i] < b[i] ? a[i] : b[i]) - 2500.0f;
				box.maxs[i] = (a[i] > b[i] ? a[i] : b[i]) + 2500.0f;
			}
			zCArray<zCVob*> found;
			world->bspTree.bspRoot->CollectVobsInBBox3D(found, box);
			for (int i = 0; i < found.GetNum(); i++)
			{
				zCVob* vob = found[i];
				int kind = KindOf(hero, vob);
				if (kind == K_NONE || kind == K_HERO)
					continue;
				if (kind == K_ITEM && !vob->GetVisual())
					continue;
				if (kind == K_MOB)
				{
					const char* label;
					const char* icon;
					if (!((oCMobInter*)vob)->GetScemeName().Length() || !MobVerb((oCMobInter*)vob, label, icon))
						continue;
				}
				if (std::find(things.begin(), things.end(), vob) != things.end())
					continue;
				vob->AddRef();
				things.push_back(vob);
			}
		}

		struct Pick
		{
			zCVob* vob = nullptr; // what is under the pointer (one of `things`, or the hero)
			bool ground = false;  // else the ground
			zVEC3 point;
		};
		static Pick hover;

		static Pick PickAt(oCNpc* hero, float sx, float sy)
		{
			Pick pick;
			zVEC3 from, dir;
			if (!ScreenRay(sx, sy, from, dir))
				return pick;
			from += dir * cutDistance; // (through what is cut away)
			float reach = 8000.0f, wall = reach;
			zCWorld* world = hero->GetHomeWorld();
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE | zTRACERAY_POLY_IGNORE_TRANSP;
			if (world && world->TraceRayNearestHit(from, dir * reach, hero, flags) && world->traceRayReport.foundHit)
			{
				pick.ground = true;
				pick.point = world->traceRayReport.foundIntersection;
				wall = (pick.point - from).Length();
			}
			float best = wall + 40.0f;
			auto test = [&](zCVob* vob, float grow)
			{
				zTBBox3D box = vob->bbox3D;
				box.mins -= zVEC3(grow, grow, grow);
				box.maxs += zVEC3(grow, grow, grow);
				float t = RayBox(from, dir, box);
				if (t >= 0.0f && t < best)
				{
					best = t;
					pick.vob = vob;
				}
			};
			for (zCVob* vob : things)
				if (Alive(vob))
					test(vob, KindOf(hero, vob) == K_ITEM ? 20.0f : 5.0f);
			test(hero, 0.0f);
			if (pick.vob)
				pick.ground = false;
			return pick;
		}

		// --- the queue of actions ------------------------------------------------------------------------
		enum Act { A_GO, A_TALK, A_ATTACK, A_FOLLOW, A_LOOT, A_FINISH, A_TAKE, A_USE, A_GOTHIC_KEY, A_DRAW, A_SHEATHE, A_JUMP, A_CAST,
			A_SHOOT, A_SPELLS }; // (A_SPELLS: the pie of his spells, not queued)
		static void PressGothicKey(int logical); // (with the hooks)
		static void PressRawKey(int key);
		enum Phase { P_START, P_MOVING, P_TURN, P_PREPARE, P_ACT, P_WAIT, P_INTERACT, P_DONE };
		enum Walk { WALK_RUN = 0, WALK_WALK = 1, WALK_SNEAK = 2 };

		struct Action
		{
			int act = A_GO;
			zCVob* target = nullptr; // referenced while queued
			zVEC3 pos;
			int walk = WALK_RUN;
			const char* label = "go_here";
			const char* icon = "go";
			int phase = P_START;
			float phaseTime = 0.0f;
			int tries = 0;
			zVEC3 lastPos;
			float lastMove = 0.0f;
			int key = 0; // A_GOTHIC_KEY: Gothic's logical key
		};

		static std::vector<Action> queue;
		static const int QUEUE_MAX = 8;
		static zCVob* forcedFocus = nullptr; // Gothic's focus while an action gives him Gothic's action key
		static float actionKeyUntil = -1.0f; // Gothic's action key (and forward, Gothic 1) until then
		static bool actionKeyEdge = false;
		// Gothic's logical keys given to him for a while (the mob buttons, a jump, a ladder held)
		struct HeldKey
		{
			int key = 0;
			float until = -1.0f;
			bool edge = false;
		};
		static HeldKey held[4];

		// (given again while still held: held on, not pressed again)
		static void GiveKey(int key, float seconds)
		{
			HeldKey* slot = &held[0];
			for (HeldKey& h : held)
				if (h.key == key || h.until < Now())
				{
					slot = &h;
					break;
				}
			bool still = slot->key == key && slot->until >= Now();
			slot->key = key;
			slot->until = Now() + seconds;
			if (!still)
				slot->edge = true;
		}

		static bool mobHoldOff = false; // Gothic 1's action key let go for the mob (stand up, down)

		static void ReleaseKeys()
		{
			for (HeldKey& h : held)
				h.until = -1.0f;
		}
		static char thought[96] = "";        // the hero's thought bubble
		static float thoughtUntil = -1.0f;
		static zVEC3 goMarker;               // the spot he walks to
		static bool goMarkerOn = false;

		static void Think(const char* labelKey)
		{
			strncpy_s(thought, Label(labelKey), _TRUNCATE);
			thoughtUntil = RealNow() + 2.5f;
		}

		static void SetPhase(Action& a, int phase)
		{
			a.phase = phase;
			a.phaseTime = Now();
		}

		static bool HasMessage(oCNpc* hero, zCClassDef* type)
		{
			zCEventManager* em = hero->GetEM();
			if (!em)
				return false;
			for (int i = 0; i < em->GetNumMessages(); i++)
			{
				zCEventMessage* msg = em->GetEventMessage(i);
				if (msg && msg->_GetClassDef() == type)
					return true;
			}
			return false;
		}

		static void Send(oCNpc* hero, zCEventMessage* msg) { hero->GetEM()->OnMessage(msg, hero); }

		// --- the Sim's routing ----------------------------------------------------------------------------
		// Gothic's movement messages end at once for the player in many places (EV_GOTOVOB to an item or a
		// chest always did), so the Sim finds his own way, as a Sim does: A* over a grid of 50 cm cells made
		// as it is searched. A cell's ground is Gothic's world (or a solid object) under it; a step to a
		// neighbour is walkable when the ground rises at most 60 cm (a ledge up to 170 cm: Gothic's climb,
		// with its jump key), falls at most 250 cm, and nothing solid is between the two at his knees and
		// his chest. The search runs in slices of 4 ms a frame; far away (35 m) the waynet's route comes
		// first and the last stretch is searched from its end. He walks the route as the player does:
		// Gothic's forward key (and walk or sneak), turned towards the next point; stuck, he jumps once,
		// then searches again from where he is (the step he got stuck on left out), at most three times.
		static const float NAV_CELL = 50.0f, NAV_STEP = 60.0f, NAV_CLIMB = 170.0f, NAV_DROP = 250.0f;
		static const int NAV_MAX_NODES = 6000, NAV_MAX_CELLS = 220; // cells from the start each way

		struct NavNode
		{
			int x, z;
			float y, g;
			int parent;
			bool closed, climb;
		};

		struct NavSearch
		{
			bool active = false;
			zVEC3 start, goal;
			zCVob* goalVob = nullptr; // (not referenced: the action holds it)
			float reach = 0.0f;
			std::unordered_map<long long, int> index;
			std::vector<NavNode> nodes;
			std::vector<std::pair<float, int>> open; // a heap, the least f first
			std::vector<long long> blocked;          // cells left out (where he got stuck)
			int expanded = 0, found = -1, closest = -1;
			float closestH = 1e30f;
		};

		struct Walker
		{
			bool active = false, failed = false, arrived = false;
			std::vector<zVEC3> points;
			std::vector<char> climbs;
			int next = 0;
			zVEC3 goal;
			zCVob* goalVob = nullptr; // (not referenced)
			float reach = 0.0f;
			int walk = 0;
			zVEC3 lastPos;
			float lastMove = 0.0f, lastJump = -10.0f;
			int stuck = 0, searches = 0;
			bool forward = false, slow = false; // Gothic's keys this frame
			bool forwardEdge = false, wasForward = false; // Gothic starts him on the key's press, as a player's
			float lastEdge = -10.0f;
			NavSearch search;
		};
		static Walker walker;
		static int BodyState(oCNpc* hero); // (with the actions)

		static long long NavKey(int x, int z) { return ((long long)(unsigned)x << 32) | (unsigned)z; }

		static int NavFlags()
		{
			return zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER | zTRACERAY_POLY_IGNORE_TRANSP
				| zTRACERAY_POLY_NORMAL;
		}

		// the ground at (x, z) at most NAV_CLIMB above `nearY` and 3 m below it; false: none, or a wall
		static bool NavGround(zCWorld* world, oCNpc* hero, float x, float z, float nearY, float& y)
		{
			zVEC3 from(x, nearY + NAV_CLIMB + 20.0f, z);
			if (!world->TraceRayNearestHit(from, zVEC3(0.0f, -(NAV_CLIMB + 320.0f), 0.0f), (zCVob*)hero, NavFlags())
				|| !world->traceRayReport.foundHit)
				return false;
			if (!world->traceRayReport.foundVob && world->traceRayReport.foundPolyNormal[1] < 0.5f)
				return false; // too steep to stand on
			y = world->traceRayReport.foundIntersection[1];
			return true;
		}

		// nothing solid between a and b at his knees and chest (above the higher of the two for a climb)
		static bool NavClear(zCWorld* world, oCNpc* hero, const zVEC3& a, const zVEC3& b, bool climb)
		{
			static const float HEIGHTS[2] = { 45.0f, 125.0f };
			for (float h : HEIGHTS)
			{
				zVEC3 p = a + zVEC3(0.0f, h, 0.0f), q = b + zVEC3(0.0f, h, 0.0f);
				if (climb)
					p[1] = q[1] = (a[1] > b[1] ? a[1] : b[1]) + h;
				if (world->TraceRayNearestHit(p, q - p, (zCVob*)hero, NavFlags()) && world->traceRayReport.foundHit)
					return false;
			}
			return true;
		}

		static float BoxDistance(const zVEC3& p, const zTBBox3D& box)
		{
			float dx = p[0] < box.mins[0] ? box.mins[0] - p[0] : (p[0] > box.maxs[0] ? p[0] - box.maxs[0] : 0.0f);
			float dz = p[2] < box.mins[2] ? box.mins[2] - p[2] : (p[2] > box.maxs[2] ? p[2] - box.maxs[2] : 0.0f);
			float dy = p[1] < box.mins[1] - 150.0f ? box.mins[1] - 150.0f - p[1] : (p[1] > box.maxs[1] + 150.0f ? p[1] - box.maxs[1] - 150.0f : 0.0f);
			return sqrtf(dx * dx + dz * dz + dy * dy);
		}

		static bool NavAtGoal(const NavSearch& s, const zVEC3& p)
		{
			if (s.goalVob && Alive(s.goalVob))
				return BoxDistance(p, s.goalVob->bbox3D) <= s.reach;
			return zVEC3(p[0] - s.goal[0], 0.0f, p[2] - s.goal[2]).Length() <= (s.reach > 45.0f ? s.reach : 45.0f) && fabsf(p[1] - s.goal[1]) < 180.0f;
		}

		static zVEC3 NavPos(const NavSearch& s, const NavNode& n)
		{
			return zVEC3(s.start[0] + n.x * NAV_CELL, n.y, s.start[2] + n.z * NAV_CELL);
		}

		static void NavPush(NavSearch& s, int x, int z, float y, float g, int parent, bool climb)
		{
			long long key = NavKey(x, z);
			auto it = s.index.find(key);
			int i;
			if (it == s.index.end())
			{
				i = (int)s.nodes.size();
				s.nodes.push_back({ x, z, y, g, parent, false, climb });
				s.index[key] = i;
			}
			else
			{
				i = it->second;
				NavNode& n = s.nodes[i];
				if (n.closed || g >= n.g)
					return;
				n.g = g;
				n.y = y;
				n.parent = parent;
				n.climb = climb;
			}
			zVEC3 p = NavPos(s, s.nodes[i]);
			float h = zVEC3(p[0] - s.goal[0], 0.0f, p[2] - s.goal[2]).Length() / NAV_CELL;
			s.open.push_back({ g + h * 1.1f, i });
			std::push_heap(s.open.begin(), s.open.end(), [](const std::pair<float, int>& a, const std::pair<float, int>& b) { return a.first > b.first; });
		}

		static void NavBegin(NavSearch& s, const zVEC3& start, const zVEC3& goal, zCVob* vob, float reach)
		{
			std::vector<long long> blocked = s.blocked;
			s = NavSearch();
			s.blocked = blocked;
			s.active = true;
			s.start = start;
			s.goal = goal;
			s.goalVob = vob;
			s.reach = reach;
			NavPush(s, 0, 0, start[1], 0.0f, -1, false);
		}

		// up to `budget` seconds of the search; true while it goes on
		static bool NavStep(NavSearch& s, oCNpc* hero, float budget)
		{
			zCWorld* world = hero->GetHomeWorld();
			if (!s.active || !world)
				return false;
			LARGE_INTEGER frequency, begin, now;
			QueryPerformanceFrequency(&frequency);
			QueryPerformanceCounter(&begin);
			auto less = [](const std::pair<float, int>& a, const std::pair<float, int>& b) { return a.first > b.first; };
			static const int DX[8] = { 1, -1, 0, 0, 1, 1, -1, -1 }, DZ[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };
			while (!s.open.empty())
			{
				QueryPerformanceCounter(&now);
				if ((float)(now.QuadPart - begin.QuadPart) / frequency.QuadPart > budget)
					return true;
				std::pop_heap(s.open.begin(), s.open.end(), less);
				int i = s.open.back().second;
				s.open.pop_back();
				if (s.nodes[i].closed)
					continue;
				s.nodes[i].closed = true;
				zVEC3 p = NavPos(s, s.nodes[i]);
				if (NavAtGoal(s, p))
				{
					s.found = i;
					s.active = false;
					return false;
				}
				float h = zVEC3(p[0] - s.goal[0], 0.0f, p[2] - s.goal[2]).Length();
				if (h < s.closestH)
				{
					s.closestH = h;
					s.closest = i;
				}
				if (++s.expanded > NAV_MAX_NODES)
					break;
				for (int d = 0; d < 8; d++)
				{
					int x = s.nodes[i].x + DX[d], z = s.nodes[i].z + DZ[d];
					if (abs(x) > NAV_MAX_CELLS || abs(z) > NAV_MAX_CELLS)
						continue;
					if (std::find(s.blocked.begin(), s.blocked.end(), NavKey(x, z)) != s.blocked.end())
						continue;
					auto it = s.index.find(NavKey(x, z));
					if (it != s.index.end() && s.nodes[it->second].closed)
						continue;
					float qx = s.start[0] + x * NAV_CELL, qz = s.start[2] + z * NAV_CELL, qy;
					if (!NavGround(world, hero, qx, qz, p[1], qy))
						continue;
					float rise = qy - p[1];
					if (rise > NAV_CLIMB || rise < -NAV_DROP)
						continue;
					bool climb = rise > NAV_STEP;
					zVEC3 q(qx, qy, qz);
					if (!NavClear(world, hero, p, q, climb))
						continue;
					float cost = (d < 4 ? 1.0f : 1.414f) + (climb ? 6.0f : 0.0f) + (rise < -120.0f ? 2.0f : 0.0f);
					NavPush(s, x, z, qy, s.nodes[i].g + cost, i, climb);
				}
			}
			s.active = false; // nothing more to search, or too much searched
			return false;
		}

		// a straight walk from a to b: the ground under it continuous (no climbs) and nothing in the way
		static bool NavLine(oCNpc* hero, const zVEC3& a, const zVEC3& b)
		{
			zCWorld* world = hero->GetHomeWorld();
			if (!world)
				return false;
			zVEC3 d(b[0] - a[0], 0.0f, b[2] - a[2]);
			float len = d.Length();
			int steps = (int)(len / 40.0f) + 1;
			zVEC3 prev = a;
			for (int i = 1; i <= steps; i++)
			{
				float t = (float)i / steps, y;
				if (!NavGround(world, hero, a[0] + d[0] * t, a[2] + d[2] * t, prev[1], y))
					return false;
				if (y - prev[1] > NAV_STEP || prev[1] - y > 120.0f)
					return false;
				zVEC3 q(a[0] + d[0] * t, y, a[2] + d[2] * t);
				if (!NavClear(world, hero, prev, q, false))
					return false;
				prev = q;
			}
			return true;
		}

		// the found route as points, shortened where a straight walk does (never over a climb)
		static void NavRoute(oCNpc* hero, const NavSearch& s, int end, std::vector<zVEC3>& points, std::vector<char>& climbs)
		{
			std::vector<int> chain;
			for (int i = end; i >= 0; i = s.nodes[i].parent)
				chain.push_back(i);
			std::reverse(chain.begin(), chain.end());
			points.clear();
			climbs.clear();
			size_t at = 0;
			while (at + 1 < chain.size())
			{
				size_t best = at + 1;
				if (!s.nodes[chain[at + 1]].climb)
					for (size_t j = (chain.size() - 1 < at + 24 ? chain.size() - 1 : at + 24); j > at + 1; j--)
					{
						bool climbBetween = false;
						for (size_t k = at + 1; k <= j; k++)
							climbBetween = climbBetween || s.nodes[chain[k]].climb;
						if (!climbBetween && NavLine(hero, NavPos(s, s.nodes[chain[at]]), NavPos(s, s.nodes[chain[j]])))
						{
							best = j;
							break;
						}
					}
				points.push_back(NavPos(s, s.nodes[chain[best]]));
				climbs.push_back(s.nodes[chain[best]].climb ? 1 : 0);
				at = best;
			}
		}

		static bool WalkActive() { return walker.active; }

		static void StopWalk()
		{
			walker.active = false;
			walker.search.active = false;
			walker.search.blocked.clear();
			walker.forward = walker.slow = false;
		}

		static bool Moving(oCNpc* hero) { return WalkActive() || HasMessage(hero, oCMsgMovement::classDef); }

		static void StopMoving(oCNpc* hero)
		{
			StopWalk();
			if (HasMessage(hero, oCMsgMovement::classDef) || HasMessage(hero, oCMsgManipulate::classDef))
				hero->ClearEM();
		}

		// a new search from where he is (the waynet's route first when it is far)
		static void WalkSearch(oCNpc* hero)
		{
			zVEC3 from = hero->GetPositionWorld();
			if (hero->human_ai)
				from[1] = hero->human_ai->feetY;
			walker.points.clear();
			walker.climbs.clear();
			walker.next = 0;
			walker.searches++;
			float away = zVEC3(walker.goal[0] - from[0], 0.0f, walker.goal[2] - from[2]).Length();
			zCWorld* world = hero->GetHomeWorld();
			if (away > 3500.0f && world && world->wayNet && walker.searches == 1)
				if (zCRoute* route = world->wayNet->FindRoute(from, walker.goal, hero))
				{
					for (zCWaypoint* wp = route->GetNextWP(); wp; wp = route->GetNextWP())
					{
						walker.points.push_back(wp->GetPositionWorld());
						walker.climbs.push_back(0);
					}
					delete route;
					if (!walker.points.empty())
					{
						Log("route: over the waynet, %d waypoints, then searched", (int)walker.points.size());
						return; // (the last stretch is searched when he gets to the route's end)
					}
				}
			if (walker.stuck == 0 && NavLine(hero, from, walker.goal))
			{
				walker.points.push_back(walker.goal);
				walker.climbs.push_back(0);
				return;
			}
			NavBegin(walker.search, from, walker.goal, walker.goalVob, walker.reach);
		}

		static void StartWalk(oCNpc* hero, const zVEC3& goal, zCVob* vob, float reach, int walk)
		{
			StopMoving(hero);
			walker = Walker();
			walker.active = true;
			walker.goal = goal;
			walker.goalVob = vob;
			walker.reach = reach;
			walker.walk = walk;
			walker.lastPos = hero->GetPositionWorld();
			walker.lastMove = Now();
			Send(hero, new oCMsgMovement(oCMsgMovement::EV_SETWALKMODE, walk == 2 ? 2 : 0)); // (walking: Gothic's slow key)
			WalkSearch(hero);
		}

		static bool WalkAtGoal(oCNpc* hero)
		{
			zVEC3 p = hero->GetPositionWorld();
			if (walker.goalVob && Alive(walker.goalVob))
				return BoxDistance(p, walker.goalVob->bbox3D) <= walker.reach;
			return zVEC3(p[0] - walker.goal[0], 0.0f, p[2] - walker.goal[2]).Length() <= (walker.reach > 45.0f ? walker.reach : 45.0f);
		}

		// each frame of his AI while he walks a route
		static void UpdateWalk(oCNpc* hero)
		{
			walker.wasForward = walker.forward;
			walker.forward = walker.slow = walker.forwardEdge = false;
			if (!walker.active)
				return;
			float now = Now();
			if (walker.search.active)
			{
				if (NavStep(walker.search, hero, 0.004f))
					return; // still searching (he stands and thinks, as a Sim does)
				NavSearch& s = walker.search;
				int end = s.found >= 0 ? s.found : (s.closest > 0 && s.closestH < (zVEC3(s.start[0] - s.goal[0], 0.0f, s.start[2] - s.goal[2]).Length() - 300.0f) ? s.closest : -1);
				if (end < 0)
				{
					Log("route: none found (%d cells searched)", s.expanded);
					walker.active = false;
					walker.failed = true;
					return;
				}
				NavRoute(hero, s, end, walker.points, walker.climbs);
				Log("route: %s, %d points (%d cells searched)", s.found >= 0 ? "found" : "part of the way", (int)walker.points.size(), s.expanded);
				walker.next = 0;
				walker.lastMove = now;
				walker.lastPos = hero->GetPositionWorld();
			}
			if (WalkAtGoal(hero))
			{
				walker.active = false;
				walker.arrived = true;
				return;
			}
			zVEC3 at = hero->GetPositionWorld();
			if (walker.next >= (int)walker.points.size())
			{
				// the route's end, not the goal yet (the waynet's end, a part of the way): search on
				if (walker.searches >= 6)
				{
					walker.active = false;
					walker.failed = true;
					return;
				}
				WalkSearch(hero);
				return;
			}
			zVEC3 target = walker.points[walker.next];
			bool last = walker.next == (int)walker.points.size() - 1;
			float d = zVEC3(target[0] - at[0], 0.0f, target[2] - at[2]).Length();
			if (d < (last ? 35.0f : 70.0f) && fabsf(target[1] - at[1]) < 120.0f)
			{
				walker.next++;
				return;
			}
			// turned towards it (quickly, not at once), Gothic's forward key
			zVEC3 want = Flat(target - at), have = Flat(hero->GetAtVectorWorld());
			float angle = atan2f(have[0] * want[2] - have[2] * want[0], have[0] * want[0] + have[2] * want[2]);
			float dt = ztimer ? ztimer->frameTimeFloat / 1000.0f : 0.016f, maxTurn = 10.0f * dt;
			float turn = angle > maxTurn ? maxTurn : (angle < -maxTurn ? -maxTurn : angle);
			float c = cosf(turn), s = sinf(turn); // (angle > 0: want lies from have towards +z, as this turn goes)
			zVEC3 dir(have[0] * c - have[2] * s, 0.0f, have[0] * s + have[2] * c);
			hero->SetHeadingAtWorld(dir);
			walker.forward = fabsf(angle) < 1.6f; // (turning on the spot first)
			walker.slow = walker.walk == 1;
			// the key's press when he starts, and again while he still stands (after a jump, a stop)
			if (walker.forward && (!walker.wasForward || (BodyState(hero) == BS_STAND && now - walker.lastEdge > 0.4f)))
			{
				walker.forwardEdge = true;
				walker.lastEdge = now;
			}
			// a ledge: Gothic's jump (it climbs what it can)
			if (walker.climbs[walker.next] && d < 110.0f && target[1] - at[1] > 40.0f && now - walker.lastJump > 1.5f)
			{
				GiveKey(GAME_SMOVE, 0.3f);
				walker.lastJump = now;
				Log("route: climbing a ledge of %.0f cm", target[1] - at[1]);
			}
			// stuck: a jump, then a new search without that step
			if ((at - walker.lastPos).Length() > 25.0f)
			{
				walker.lastPos = at;
				walker.lastMove = now;
			}
			else if (now - walker.lastMove > 1.2f)
			{
				walker.lastMove = now;
				walker.stuck++;
				if (walker.stuck == 1 && now - walker.lastJump > 1.5f)
				{
					GiveKey(GAME_SMOVE, 0.3f);
					walker.lastJump = now;
					Log("route: stuck, jumping");
				}
				else if (walker.stuck > 4)
				{
					Log("route: stuck, giving up");
					walker.active = false;
					walker.failed = true;
				}
				else
				{
					int cx = (int)floorf((target[0] - at[0]) / NAV_CELL + 0.5f), cz = (int)floorf((target[2] - at[2]) / NAV_CELL + 0.5f);
					walker.search.blocked.push_back(NavKey(cx, cz)); // (relative to the next search's start: here)
					Log("route: stuck, searching again");
					WalkSearch(hero);
				}
			}
		}

		// his animations sped up by takeSpeed for `seconds` (taking things), then Gothic's speed again
		static zCModel* fastModel = nullptr;
		static float fastUntil = -1.0f;

		static void FastAnims(oCNpc* hero, float seconds)
		{
			zCModel* model = hero->GetModel();
			if (!model || settings.takeSpeed <= 1.0f)
				return;
			model->timeScale = settings.takeSpeed;
			fastModel = model;
			fastUntil = Now() + seconds;
		}

		static void UpdateFastAnims(oCNpc* hero)
		{
			if (!fastModel || Now() < fastUntil)
				return;
			if (hero && hero->GetModel() == fastModel)
				fastModel->timeScale = 1.0f;
			fastModel = nullptr;
		}

		static void EndAction(oCNpc* hero, Action& a)
		{
			if (a.target)
			{
				a.target->Release();
				a.target = nullptr;
			}
			if (forcedFocus)
				forcedFocus = nullptr;
			actionKeyUntil = -1.0f;
			goMarkerOn = false;
		}

		static void StopFight(oCNpc* hero, const char* why);

		// the current action given up: his movement stopped (Gothic's messages the plugin sent)
		static void CancelCurrent(oCNpc* hero)
		{
			if (queue.empty())
				return;
			Action& a = queue.front();
			Log("action: %s cancelled", a.label);
			if (a.phase == P_MOVING || a.phase == P_TURN || a.phase == P_PREPARE)
				StopMoving(hero);
			if (a.act == A_FOLLOW)
				StopMoving(hero);
			EndAction(hero, a);
			queue.erase(queue.begin());
		}

		static void ClearQueue(oCNpc* hero)
		{
			while (!queue.empty())
				CancelCurrent(hero);
		}

		static float autoFightBlockedUntil = -1.0f;

		static void Enqueue(oCNpc* hero, Action a, bool add)
		{
			if (!add)
			{
				ClearQueue(hero);
				StopFight(hero, "a new order");
				autoFightBlockedUntil = Now() + 4.0f;
			}
			if ((int)queue.size() >= QUEUE_MAX)
				return;
			if (a.target)
				a.target->AddRef();
			a.phase = P_START;
			a.phaseTime = Now();
			char name[96] = "";
			if (a.target)
				NameOf(a.target, name, sizeof name);
			Log("queue: %s %s%s (%d queued)", a.label, name, add ? ", added" : "", (int)queue.size() + 1);
			queue.push_back(a);
		}

		// how near he must be to act on it (cm from his position to its box, on the ground plane)
		static float Reach(oCNpc* hero, const Action& a)
		{
			switch (a.act)
			{
			case A_TALK: return 180.0f;
			case A_FOLLOW: return 350.0f;
			case A_LOOT: case A_FINISH: return 110.0f;
			case A_TAKE: return 110.0f;
			case A_USE: return 90.0f;
			case A_CAST: return 1200.0f;
			case A_SHOOT: return 1800.0f;
			default: return 100.0f;
			}
		}

		static float DistanceTo(oCNpc* hero, zCVob* vob)
		{
			zVEC3 p = hero->GetPositionWorld();
			zTBBox3D box = vob->bbox3D;
			float dx = p[0] < box.mins[0] ? box.mins[0] - p[0] : (p[0] > box.maxs[0] ? p[0] - box.maxs[0] : 0.0f);
			float dz = p[2] < box.mins[2] ? box.mins[2] - p[2] : (p[2] > box.maxs[2] ? p[2] - box.maxs[2] : 0.0f);
			float dy = p[1] < box.mins[1] - 150.0f ? box.mins[1] - 150.0f - p[1] : (p[1] > box.maxs[1] + 150.0f ? p[1] - box.maxs[1] - 150.0f : 0.0f);
			return sqrtf(dx * dx + dz * dz + dy * dy);
		}

		// his way to the action's spot: a mob's own place for him (oCMobInter::GetFreePosition: where
		// Gothic's interaction starts), else near enough to the thing, else the spot clicked
		static void WalkTo(oCNpc* hero, const Action& a)
		{
			if (!a.target)
			{
				StartWalk(hero, a.pos, nullptr, 45.0f, a.walk);
				return;
			}
			zVEC3 spot;
			if (oCMobInter* mob = zDYNAMIC_CAST<oCMobInter>(a.target))
				if (mob->GetFreePosition(hero, spot))
				{
					Log("route: to %s's place for him", mob->GetScemeName().ToChar());
					StartWalk(hero, spot, nullptr, 40.0f, a.walk);
					return;
				}
			StartWalk(hero, a.target->GetPositionWorld(), a.target, Reach(hero, a) * 0.9f, a.walk);
		}

		static bool InDialog()
		{
			oCInformationManager& info = oCInformationManager::GetInformationManager();
			return !info.HasFinished();
		}

		static bool ContainerOpen(oCNpc* hero)
		{
			if (hero->inventory2.IsOpen())
				return true;
			zCList<oCItemContainer>& list = oCItemContainer::contList;
			for (zCList<oCItemContainer>* node = list.next; node; node = node->next)
				if (node->data && node->data->IsOpen())
					return true;
			return false;
		}

		static void StartFight(oCNpc* hero, oCNpc* enemy, const char* why);

		static int BodyState(oCNpc* hero) { return hero->GetBodyState() & BS_ONLY_STATE; }

		// One frame of the current action (before Gothic's AI for him).
		static void UpdateActions(oCNpc* hero)
		{
			if (queue.empty())
				return;
			Action& a = queue.front();
			float now = Now();
			if (InDialog() && a.phase != P_WAIT)
				return; // Gothic's dialogue first
			if (a.target && !Alive(a.target) && a.act != A_TAKE)
			{
				Log("action: %s: its target is gone", a.label);
				EndAction(hero, a);
				queue.erase(queue.begin());
				return;
			}
			zVEC3 at = hero->GetPositionWorld();
			switch (a.phase)
			{
			case P_START:
			{
				a.lastPos = at;
				a.lastMove = now;
				if (a.act == A_ATTACK)
				{
					StartFight(hero, zDYNAMIC_CAST<oCNpc>(a.target), "chosen");
					SetPhase(a, P_DONE);
					break;
				}
				if (a.act == A_DRAW || a.act == A_SHEATHE || a.act == A_GOTHIC_KEY)
				{
					SetPhase(a, P_ACT);
					break;
				}
				if (a.act == A_JUMP)
				{
					zVEC3 to = a.pos - at;
					to[1] = 0.0f;
					if (to.Length() > 1.0f)
						hero->SetHeadingAtWorld(to * (1.0f / to.Length()));
					SetPhase(a, P_PREPARE);
					break;
				}
				if (a.act == A_CAST && !a.target)
				{
					SetPhase(a, P_PREPARE);
					break;
				}
				if (a.act == A_GO)
				{
					goMarker = a.pos;
					goMarkerOn = true;
					WalkTo(hero, a);
					SetPhase(a, P_MOVING);
					break;
				}
				if (DistanceTo(hero, a.target) <= Reach(hero, a))
					SetPhase(a, a.act == A_FOLLOW ? P_MOVING : P_TURN);
				else
				{
					WalkTo(hero, a);
					SetPhase(a, P_MOVING);
				}
				break;
			}
			case P_MOVING:
			{
				// his own route (UpdateWalk); there: near enough, or at the mob's place for him
				bool there = a.act == A_GO ? zVEC3(at[0] - a.pos[0], 0.0f, at[2] - a.pos[2]).Length() < 60.0f
					: DistanceTo(hero, a.target) <= Reach(hero, a);
				if (a.act == A_FOLLOW) // until cancelled: after him when he is away, a new route when he moved on
				{
					if (there)
					{
						if (WalkActive())
							StopMoving(hero);
					}
					else if (!WalkActive() || ((a.target->GetPositionWorld() - walker.goal).Length() > 250.0f && now - a.lastMove > 1.0f))
					{
						WalkTo(hero, a);
						a.lastMove = now;
					}
					break;
				}
				if (there || (!WalkActive() && walker.arrived))
				{
					StopMoving(hero);
					if (a.act == A_GO)
					{
						Log("action: arrived");
						SetPhase(a, P_DONE);
					}
					else
						SetPhase(a, P_TURN);
					break;
				}
				if (!WalkActive())
				{
					if (walker.failed && ++a.tries <= 1) // once more, from where he is now
					{
						Log("action: %s: no way found, searching once more", a.label);
						WalkTo(hero, a);
						break;
					}
					Log("action: %s: can't get there", a.label);
					StopMoving(hero);
					Think("cant_reach");
					SetPhase(a, P_DONE);
				}
				break;
			}
			case P_TURN:
			{
				// facing it at once (Gothic's turning on the spot takes long, Sims don't)
				zVEC3 to = a.target->GetPositionWorld() - at;
				to[1] = 0.0f;
				if (to.Length() > 1.0f)
					hero->SetHeadingAtWorld(to * (1.0f / to.Length()));
				SetPhase(a, P_PREPARE);
				break;
			}
			case P_PREPARE:
			{
				if (Moving(hero) && now - a.phaseTime < 1.5f)
					break; // turning
				if (a.act == A_JUMP)
				{
					SetPhase(a, P_ACT);
					break;
				}
				// a weapon away before using things; drawn to finish someone off; the spell drawn to cast it,
				// the bow (or crossbow) to shoot
				int mode = hero->GetWeaponMode();
				if (a.act == A_SHOOT)
				{
					if (mode != NPC_WEAPON_BOW && mode != NPC_WEAPON_CBOW)
					{
						oCItem* ranged = hero->GetEquippedRangedWeapon();
						if (!ranged)
						{
							Think("no_bow");
							SetPhase(a, P_DONE);
							break;
						}
						if (a.tries == 0 && !HasMessage(hero, oCMsgWeapon::classDef))
						{
							Send(hero, new oCMsgWeapon(oCMsgWeapon::EV_DRAWWEAPON, ranged->HasFlag(ITM_FLAG_CROSSBOW) ? NPC_WEAPON_CBOW : NPC_WEAPON_BOW, 0));
							a.tries = 1;
						}
						if (now - a.phaseTime < 4.0f)
							break;
						Think("busy");
						SetPhase(a, P_DONE);
						break;
					}
					if (HasMessage(hero, oCMsgWeapon::classDef))
						break;
					SetPhase(a, P_ACT);
					break;
				}
				if (a.act == A_CAST)
				{
					oCMag_Book* book = hero->GetSpellBook();
					oCItem* drawn = book && mode == NPC_WEAPON_MAG && book->GetSelectedSpellNr() >= 0 ? book->GetSpellItem(book->GetSelectedSpellNr()) : nullptr;
					if (!drawn || book->GetKeyByItem(drawn) != a.key)
					{
						if (a.tries == 0 && !HasMessage(hero, oCMsgWeapon::classDef))
						{
							PressRawKey(a.key == 0 ? KEY_0 : KEY_1 + a.key - 1);
							a.tries = 1;
						}
						if (now - a.phaseTime < 3.0f)
							break;
						Log("action: the spell of key %d didn't come", a.key);
						Think("busy");
						SetPhase(a, P_DONE);
						break;
					}
					if (HasMessage(hero, oCMsgWeapon::classDef) || HasMessage(hero, oCMsgMagic::classDef))
						break;
					SetPhase(a, P_ACT);
					break;
				}
				if (a.act == A_FINISH)
				{
					if (mode == NPC_WEAPON_NONE)
					{
						if (now - a.phaseTime < 0.1f)
							Send(hero, new oCMsgWeapon(oCMsgWeapon::EV_DRAWWEAPON, hero->GetEquippedMeleeWeapon() ? NPC_WEAPON_1HS : NPC_WEAPON_FIST, 0));
						if (now - a.phaseTime < 3.0f)
							break;
					}
				}
				else if (mode != NPC_WEAPON_NONE)
				{
					if (!HasMessage(hero, oCMsgWeapon::classDef))
						Send(hero, new oCMsgWeapon(oCMsgWeapon::EV_REMOVEWEAPON, NPC_WEAPON_NONE, 0));
					if (now - a.phaseTime < 3.0f)
						break;
				}
				if (HasMessage(hero, oCMsgWeapon::classDef) && now - a.phaseTime < 3.0f)
					break;
				SetPhase(a, P_ACT);
				break;
			}
			case P_ACT:
			{
				if (a.act == A_DRAW)
				{
					oCItem* melee = hero->GetEquippedMeleeWeapon();
					oCItem* ranged = hero->GetEquippedRangedWeapon();
					int mode = melee ? (melee->HasFlag(ITM_FLAG_2HD_SWD) || melee->HasFlag(ITM_FLAG_2HD_AXE) ? NPC_WEAPON_2HS : NPC_WEAPON_1HS)
						: (ranged ? (ranged->HasFlag(ITM_FLAG_CROSSBOW) ? NPC_WEAPON_CBOW : NPC_WEAPON_BOW) : NPC_WEAPON_FIST);
					Send(hero, new oCMsgWeapon(oCMsgWeapon::EV_DRAWWEAPON, mode, 0));
					SetPhase(a, P_DONE);
					break;
				}
				if (a.act == A_SHEATHE)
				{
					Send(hero, new oCMsgWeapon(oCMsgWeapon::EV_REMOVEWEAPON, NPC_WEAPON_NONE, 0));
					SetPhase(a, P_DONE);
					break;
				}
				if (a.act == A_GOTHIC_KEY)
				{
					PressGothicKey(a.key);
					SetPhase(a, P_DONE);
					break;
				}
				if (a.act == A_JUMP) // Gothic's jump forward (a ledge he can climb: Gothic's climb)
				{
					GiveKey(GAME_SMOVE, 0.35f);
					GiveKey(GAME_UP, 0.6f);
					Log("action: jump");
					SetPhase(a, P_DONE);
					break;
				}
				if (a.act == A_CAST || a.act == A_SHOOT)
				{
					// the spell or the bow drawn: Gothic's action held (aiming, a spell invested) with
					// the target in his focus, forward given halfway (Gothic's attack: action and forward)
					forcedFocus = a.target;
					if (a.target)
						hero->SetFocusVob(a.target);
					actionKeyUntil = now + (a.act == A_SHOOT ? 1.6f : 1.2f);
					actionKeyEdge = true;
					a.tries = 0;
					Log("action: %s (key %d)", a.act == A_SHOOT ? "shooting" : "casting", a.key);
					SetPhase(a, P_WAIT);
					break;
				}
				// Gothic's action key on it (with forward in Gothic 1), the target in his focus; taking
				// something his animation sped up (takeSpeed)
				forcedFocus = a.target;
				hero->SetFocusVob(a.target);
				actionKeyUntil = now + 0.5f;
				actionKeyEdge = true;
				mobHoldOff = false;
				if (a.act == A_TAKE)
					FastAnims(hero, 3.0f);
				Log("action: %s: Gothic's action on it (body state %d)", a.label, BodyState(hero));
				SetPhase(a, P_WAIT);
				break;
			}
			case P_WAIT:
			{
				if (!GOTHIC2 && a.act == A_USE) // Gothic 1 uses a mob while the action key is held
					GiveKey(GAME_ACTION, 0.2f);
				bool started = false;
				switch (a.act)
				{
				case A_TALK: started = InDialog(); break;
				case A_TAKE: started = !Alive(a.target) || a.target->GetHomeWorld() == nullptr; break;
				case A_LOOT: started = ContainerOpen(hero); break;
				case A_USE: started = hero->GetInteractMob() == a.target || ContainerOpen(hero) || BodyState(hero) == BS_MOBINTERACT; break;
				case A_FINISH: started = ((oCNpc*)a.target)->IsDead(); break;
				case A_CAST: case A_SHOOT:
				{
					float fire = a.act == A_SHOOT ? 0.9f : 0.5f;
					if (a.tries == 0 && now - a.phaseTime > fire)
					{
						GiveKey(GAME_UP, 0.3f);
						a.tries = 1;
					}
					started = now - a.phaseTime > fire + 0.8f;
					break;
				}
				default: started = true; break;
				}
				if (started)
				{
					forcedFocus = nullptr;
					actionKeyUntil = -1.0f;
					Log("action: %s: Gothic took it", a.label);
					SetPhase(a, a.act == A_USE ? P_INTERACT : P_DONE);
					break;
				}
				if (now - a.phaseTime > (a.act == A_TAKE ? 1.2f : 2.0f))
				{
					if (++a.tries <= 3)
					{
						if (a.act == A_TAKE && a.tries == 1) // Gothic's own taking (AI_TakeItem's)
						{
							Log("action: take: Gothic's AI takes it");
							Send(hero, new oCMsgManipulate(oCMsgManipulate::EV_TAKEVOB, a.target));
							FastAnims(hero, 3.0f);
							SetPhase(a, P_WAIT);
							break;
						}
						Log("action: %s: nothing yet, Gothic's action again", a.label);
						SetPhase(a, a.act == A_FINISH ? P_ACT : P_TURN);
						break;
					}
					Log("action: %s: Gothic didn't take it", a.label);
					forcedFocus = nullptr;
					actionKeyUntil = -1.0f;
					Think("busy");
					SetPhase(a, P_DONE);
				}
				break;
			}
			case P_INTERACT:
				// using a mob (the buttons give its keys) or a screen of Gothic's (a chest's); on a ladder
				// he climbs to its other end on his own (Gothic's up or down held)
				if (oCMobLadder* ladder = zDYNAMIC_CAST<oCMobLadder>(hero->GetInteractMob()))
					if (!a.key)
					{
						zTBBox3D box = ladder->bbox3D;
						a.key = at[1] < (box.mins[1] + box.maxs[1]) / 2.0f ? GAME_UP : GAME_DOWN;
						GiveKey(a.key, 30.0f);
						Log("action: climbing %s", a.key == GAME_UP ? "up" : "down");
					}
				// Gothic 1: the action key held through the interaction (a player who lets go stops it):
				// till a chest's screen or a dialogue opens, he lets go of the mob, or stands up (the
				// button); sitting down and lying down only till he is there
				if (ContainerOpen(hero) || InDialog())
					mobHoldOff = true; // (not again when the screen closes: it would open the chest again)
				if (!GOTHIC2 && !mobHoldOff && hero->GetInteractMob())
				{
					const char* label;
					const char* icon;
					bool rest = MobVerb(hero->GetInteractMob(), label, icon) && (!strcmp(label, "sit") || !strcmp(label, "sleep"));
					if (!rest || now - a.phaseTime < 3.0f)
						GiveKey(GAME_ACTION, 0.2f);
				}
				if (!hero->GetInteractMob() && !ContainerOpen(hero) && BodyState(hero) != BS_MOBINTERACT && now - a.phaseTime > 0.5f)
				{
					if (a.key)
						ReleaseKeys();
					Log("action: %s: done", a.label);
					SetPhase(a, P_DONE);
				}
				break;
			}
			if (!queue.empty() && queue.front().phase == P_DONE)
			{
				EndAction(hero, queue.front());
				queue.erase(queue.begin());
			}
		}

		// --- fighting -----------------------------------------------------------------------------------------
		static oCNpc* fightTarget = nullptr; // referenced while fighting
		static oCNpc* lastAttacker = nullptr; // whoever hit him last (not referenced: compared only)
		static float lastHitTime = -100.0f;
		static float fightStart = 0.0f, fightEnd = -100.0f, attackSent = -100.0f;
		static bool autoFight = true, autoFightSet = false;
		static int savedTactic = -1;
		static oCNpc* selected = nullptr; // the last one clicked (compared only)
		static float inReachSince = -1.0f; // in reach of the enemy since then (-1: not)
		static int reachHp = 0;             // the enemy's health when he came in reach
		static int gothicStrikes = 0;       // Gothic's combat AI struck for him: 1 yes, -1 no, 0 not known yet
		static bool ownBlows = false;       // this fight with his own blows (Gothic's attack keys)
		static float nextBlow = 0.0f;

		static void StopFight(oCNpc* hero, const char* why)
		{
			if (!fightTarget)
				return;
			Log("fight: over (%s)", why);
			fightTarget->Release();
			fightTarget = nullptr;
			fightEnd = Now();
			forcedFocus = nullptr;
			ReleaseKeys();
			if (WalkActive())
				StopWalk();
			if (hero)
			{
				if (savedTactic >= 0)
					hero->fighttactic = savedTactic;
				savedTactic = -1;
				if (HasMessage(hero, oCMsgWeapon::classDef) || HasMessage(hero, oCMsgAttack::classDef) || Moving(hero))
					hero->ClearEM();
				if (hero->enemy)
					hero->SetEnemy(nullptr);
				if (settings.fightAI == 1 && hero->state.IsScriptStateActive())
					hero->state.EndCurrentState();
			}
		}

		static void StartFight(oCNpc* hero, oCNpc* enemy, const char* why)
		{
			if (!enemy || enemy == hero || enemy->IsDead())
				return;
			if (fightTarget == enemy)
				return;
			StopFight(hero, "another enemy");
			char name[96];
			NameOf(enemy, name, sizeof name);
			Log("fight: %s (%s)", name, why);
			enemy->AddRef();
			fightTarget = enemy;
			fightStart = Now();
			attackSent = -100.0f;
			inReachSince = -1.0f;
			ownBlows = false;
			nextBlow = 0.0f;
			savedTactic = hero->fighttactic;
			hero->fighttactic = 4; // FAI_HUMAN_MASTER (Gothic 1 and 2)
			hero->SetEnemy(enemy);
			follow = true;
			if (settings.fightAI == 1)
			{
				zSTRING state = "ZS_ATTACK";
				hero->state.StartAIState(state, 1, 0, 0.0f, 0);
			}
		}

		// someone attacking him: aiming at him with a weapon drawn (a beast: hunting him), or the last
		// one to hit him, the nearest within 20 m
		static oCNpc* FindAttacker(oCNpc* hero)
		{
			oCWorld* world = ogame ? ogame->GetGameWorld() : nullptr;
			oCNpc* best = nullptr;
			float bestDistance = 2000.0f;
			zVEC3 at = hero->GetPositionWorld();
			for (zCListSort<oCNpc>* node = world && world->voblist_npcs ? world->voblist_npcs->next : nullptr; node; node = node->next)
			{
				oCNpc* npc = node->data;
				if (!npc || npc == hero || npc->IsDead() || npc->IsUnconscious())
					continue;
				bool attacking = npc->enemy == hero && (npc->GetWeaponMode() != NPC_WEAPON_NONE || npc->IsMonster());
				if (npc == lastAttacker && Now() - lastHitTime < 5.0f)
					attacking = true;
				if (!attacking)
					continue;
				float d = (npc->GetPositionWorld() - at).Length();
				if (d < bestDistance)
				{
					bestDistance = d;
					best = npc;
				}
			}
			return best;
		}

		static int MeleeMode(oCNpc* hero)
		{
			oCItem* melee = hero->GetEquippedMeleeWeapon();
			if (!melee)
				return NPC_WEAPON_FIST;
			return melee->HasFlag(ITM_FLAG_2HD_SWD) || melee->HasFlag(ITM_FLAG_2HD_AXE) ? NPC_WEAPON_2HS : NPC_WEAPON_1HS;
		}

		static void UpdateFight(oCNpc* hero)
		{
			if (!autoFightSet)
			{
				autoFight = settings.autoFight;
				autoFightSet = true;
			}
			float now = Now();
			if (!fightTarget && autoFight && now > autoFightBlockedUntil && queue.empty() && !InDialog())
				if (oCNpc* attacker = FindAttacker(hero))
					StartFight(hero, attacker, "auto fight: attacked");
			if (!fightTarget)
			{
				// the weapon away a while after a fight, when nobody is attacking
				if (fightEnd > 0.0f && now - fightEnd > 3.0f && hero->GetWeaponMode() != NPC_WEAPON_NONE && queue.empty()
					&& !HasMessage(hero, oCMsgWeapon::classDef))
				{
					Log("fight: weapon away");
					Send(hero, new oCMsgWeapon(oCMsgWeapon::EV_REMOVEWEAPON, NPC_WEAPON_NONE, 0));
					fightEnd = -100.0f;
				}
				return;
			}
			oCNpc* enemy = fightTarget;
			if (!Alive(enemy) || enemy->IsDead())
			{
				StopFight(hero, "the enemy is dead");
				return;
			}
			if (enemy->IsUnconscious())
			{
				StopFight(hero, "the enemy is down");
				return;
			}
			if ((enemy->GetPositionWorld() - hero->GetPositionWorld()).Length() > 4000.0f)
			{
				StopFight(hero, "the enemy is far away");
				return;
			}
			if (InDialog())
				return;
			if (hero->enemy != enemy)
				hero->SetEnemy(enemy);
			if (settings.fightAI == 1)
				return; // the script state fights
			if (hero->GetWeaponMode() == NPC_WEAPON_NONE)
			{
				if (!HasMessage(hero, oCMsgWeapon::classDef))
				{
					oCItem* ranged = hero->GetEquippedRangedWeapon();
					int mode = MeleeMode(hero);
					if (mode == NPC_WEAPON_FIST && ranged)
						mode = ranged->HasFlag(ITM_FLAG_CROSSBOW) ? NPC_WEAPON_CBOW : NPC_WEAPON_BOW;
					Log("fight: drawing (mode %d)", mode);
					Send(hero, new oCMsgWeapon(oCMsgWeapon::EV_DRAWWEAPON, mode, 0));
				}
				return;
			}
			// out of reach: he runs to the enemy on his own route (Gothic's fight AI would close in with its
			// short fighting steps), after him while he moves
			int mode = hero->GetWeaponMode();
			bool ranged = mode == NPC_WEAPON_BOW || mode == NPC_WEAPON_CBOW || mode == NPC_WEAPON_MAG;
			float gap = DistanceTo(hero, enemy), reach = ranged ? 1500.0f : 140.0f;
			if (gap > reach + (WalkActive() ? 0.0f : 60.0f))
			{
				if (!WalkActive() || (enemy->GetPositionWorld() - walker.goal).Length() > 200.0f)
				{
					if (HasMessage(hero, oCMsgWeapon::classDef) || HasMessage(hero, oCMsgAttack::classDef))
						hero->ClearEM(); // (his AI's messages would hold the player's keys)
					StartWalk(hero, enemy->GetPositionWorld(), enemy, reach * 0.8f, WALK_RUN);
					Log("fight: running to the enemy, %.0f cm away", gap);
				}
				inReachSince = -1.0f;
				return;
			}
			if (WalkActive())
				StopWalk();
			if (inReachSince < 0.0f)
			{
				inReachSince = now;
				reachHp = enemy->attribute[NPC_ATR_HITPOINTS];
			}
			bool struck = enemy->attribute[NPC_ATR_HITPOINTS] < reachHp;
			if (struck && gothicStrikes != 1 && !ownBlows)
			{
				gothicStrikes = 1;
				Log("fight: Gothic's combat AI strikes for him");
			}
			// Gothic's combat AI (AI_Attack): one EV_ATTACK at a time, a new one when Gothic is done with it;
			// when it hasn't struck in 3 s in reach (or didn't in an earlier fight), his own blows
			if (!ownBlows && (gothicStrikes == -1 || (!struck && now - inReachSince > 3.0f && gothicStrikes != 1)))
			{
				ownBlows = true;
				gothicStrikes = -1;
				hero->ClearEM();
				Log("fight: Gothic's combat AI doesn't strike for him: his own blows (Gothic's attack keys)");
			}
			if (!ownBlows)
			{
				if (!HasMessage(hero, oCMsgWeapon::classDef) && !HasMessage(hero, oCMsgAttack::classDef) && !Moving(hero))
				{
					Send(hero, new oCMsgWeapon(oCMsgWeapon::EV_ATTACK, 0, 0));
					if (now - attackSent > 5.0f)
						Log("fight: Gothic's combat AI (EV_ATTACK), %.0f cm to the enemy", gap);
					attackSent = now;
				}
				return;
			}
			// his own blows: facing the enemy, Gothic's attack (action held, forward pressed in the
			// combo's rhythm); a bow or a spell: aimed with the action held, shot with forward
			zVEC3 to = enemy->GetPositionWorld() - hero->GetPositionWorld();
			to[1] = 0.0f;
			if (to.Length() > 1.0f)
				hero->SetHeadingAtWorld(to * (1.0f / to.Length()));
			forcedFocus = enemy;
			GiveKey(GAME_ACTION, 0.25f);
			if (now >= nextBlow)
			{
				GiveKey(GAME_UP, 0.15f);
				nextBlow = now + (ranged ? 1.4f : 0.5f);
			}
		}

		// the HUD's "Fight": the one under the pointer, the last one clicked, else the nearest enemy
		static void FightButton(oCNpc* hero)
		{
			oCNpc* enemy = nullptr;
			if (hover.vob && KindOf(hero, hover.vob) == K_NPC)
				enemy = (oCNpc*)hover.vob;
			if (!enemy && selected)
				for (zCVob* vob : things)
					if (vob == selected && Alive(vob) && KindOf(hero, vob) == K_NPC)
						enemy = selected;
			if (!enemy)
				enemy = FindAttacker(hero);
			if (!enemy)
			{
				float best = 2500.0f;
				for (zCVob* vob : things)
					if (Alive(vob) && KindOf(hero, vob) == K_NPC && Hostile(hero, (oCNpc*)vob))
					{
						float d = (vob->GetPositionWorld() - hero->GetPositionWorld()).Length();
						if (d < best)
						{
							best = d;
							enemy = (oCNpc*)vob;
						}
					}
			}
			if (!enemy)
			{
				Think("no_target");
				return;
			}
			ClearQueue(hero);
			StartFight(hero, enemy, "the fight button");
		}

		// --- Gothic's keys inside his AI -----------------------------------------------------------------------
		static bool overrideInput = false; // inside his DoAI: Gothic's movement keys answered here
		static float dialogConfirmUntil = -1.0f; // Gothic's action key for its dialogue box (a click on a choice)
		static int frameCount = 0, dialogConfirmFrame = -10;

		static bool OverrideKey(unsigned short key, float& state, int& toggled)
		{
			auto set = [&](bool on, bool pressed) { state = on ? 1.0f : 0.0f; toggled = pressed ? 1 : 0; return true; };
			if (key == GAME_ACTION && RealNow() < dialogConfirmUntil)
				return set(true, frameCount <= dialogConfirmFrame + 1);
			if (!overrideInput)
				return false;
			oCNpc* hero = oCNpc::player;
			float now = Now();
			bool action = now < actionKeyUntil;
			for (HeldKey& h : held)
				if (h.key == key && now < h.until)
					return set(true, h.edge);
			// using a mob: the keyboard stays Gothic's (lock picks, wheels, standing up)
			if (hero && hero->GetInteractMob())
				return false;
			switch (key)
			{
			case GAME_ACTION:
				return set(action, action && actionKeyEdge);
			case GAME_UP: // Gothic 1 interacts (and strikes) on the action key with a forward press; shooting
			{             // and casting get forward on their own (GiveKey)
				int mode = hero ? hero->GetWeaponMode() : NPC_WEAPON_NONE;
				bool ranged = mode == NPC_WEAPON_BOW || mode == NPC_WEAPON_CBOW || mode == NPC_WEAPON_MAG;
				bool interact = !GOTHIC2 && action && hero && !ranged && (BodyState(hero) == BS_STAND || mode != NPC_WEAPON_NONE);
				return set(interact || walker.forward, (interact && actionKeyEdge) || walker.forwardEdge);
			}
			case GAME_SLOW: // walking, not running, on his route
				return set(walker.forward && walker.slow, walker.forwardEdge && walker.slow);
			case GAME_DOWN: case GAME_LEFT: case GAME_RIGHT: case GAME_STRAFELEFT: case GAME_STRAFERIGHT:
			case GAME_SNEAK: case GAME_SMOVE: case GAME_SMOVE2: case GAME_LOOK: case GAME_LOOK_FP: case GAME_ACTION2:
			case GAME_PARADE: case GAME_ACTIONLEFT: case GAME_ACTIONRIGHT: case GAME_LOCK_TARGET:
			case GAME_WEAPON: // (Space: The Sims' pause; the weapon is the HUD's button)
				return set(false, false);
			default:
				return false;
			}
		}

		// --- the pie menu ------------------------------------------------------------------------------------
		struct Option
		{
			Action action;
			char text[96];
			float x, y, w, h;
		};
		static bool pieOpen = false;
		static float pieX = 0.0f, pieY = 0.0f, pieOpened = 0.0f;
		static char pieTitle[96] = "";
		static std::vector<Option> pie;
		static zCVob* pieTarget = nullptr; // referenced while the menu is open

		static void ClosePie()
		{
			if (pieTarget)
				pieTarget->Release();
			pieTarget = nullptr;
			pieOpen = false;
			pie.clear();
		}

		static void AddOption(int act, zCVob* target, const zVEC3& pos, const char* label, const char* icon, int walk = WALK_RUN, int key = 0)
		{
			Option o = {};
			o.action.act = act;
			o.action.target = target;
			o.action.pos = pos;
			o.action.label = label;
			o.action.icon = icon;
			o.action.walk = walk;
			o.action.key = key;
			strncpy_s(o.text, Label(label), _TRUNCATE);
			pie.push_back(o);
		}

		// his spells: the runes and scrolls on Gothic's number keys (3 to 0), their key and item
		static int Spells(oCNpc* hero, int* keys, oCItem** items, int max)
		{
			oCMag_Book* book = hero->GetSpellBook();
			int n = 0;
			for (int i = 0; book && i < book->GetNoOfSpells() && n < max; i++)
				if (oCItem* item = book->GetSpellItem(i))
				{
					int key = book->GetKeyByItem(item);
					if (key >= 0 && key <= 9)
					{
						keys[n] = key;
						items[n] = item;
						n++;
					}
				}
			return n;
		}

		static bool HasSpells(oCNpc* hero)
		{
			int keys[10];
			oCItem* items[10];
			return Spells(hero, keys, items, 10) > 0;
		}

		// the pie of his spells, cast at `target` (or wherever he faces)
		static void OpenSpellPie(oCNpc* hero, zCVob* target)
		{
			int keys[10];
			oCItem* items[10];
			int n = Spells(hero, keys, items, 10);
			zVEC3 none(0.0f, 0.0f, 0.0f);
			pie.clear();
			for (int i = 0; i < n; i++)
			{
				AddOption(A_CAST, target, none, "cast", "magic", WALK_RUN, keys[i]);
				strncpy_s(pie.back().text, items[i]->name.ToChar(), _TRUNCATE);
			}
			Log("pie: %d spell(s)", n);
		}

		static void OpenPie(oCNpc* hero, const Pick& pick, bool groundMenu)
		{
			ClosePie();
			pieX = pointerX;
			pieY = pointerY;
			pieOpened = RealNow();
			pieTitle[0] = 0;
			zCVob* vob = pick.vob;
			int kind = KindOf(hero, vob);
			zVEC3 none(0.0f, 0.0f, 0.0f);
			if (groundMenu || kind == K_NONE)
			{
				if (!pick.ground)
					return;
				AddOption(A_GO, nullptr, pick.point, "run_here", "run", WALK_RUN);
				AddOption(A_GO, nullptr, pick.point, "walk_here", "go", WALK_WALK);
				AddOption(A_GO, nullptr, pick.point, "sneak_here", "sneak", WALK_SNEAK);
				AddOption(A_JUMP, nullptr, pick.point, "jump", "up");
			}
			else
			{
				NameOf(vob, pieTitle, sizeof pieTitle);
				switch (kind)
				{
				case K_HERO:
					AddOption(A_GOTHIC_KEY, nullptr, none, "inventory", "inventory", WALK_RUN, GAME_INVENTORY);
					if (hero->GetWeaponMode() == NPC_WEAPON_NONE)
						AddOption(A_DRAW, nullptr, none, "draw", "weapon");
					else
						AddOption(A_SHEATHE, nullptr, none, "sheathe", "weapon");
					AddOption(A_GOTHIC_KEY, nullptr, none, "status", "status", WALK_RUN, GAME_SCREEN_STATUS);
					AddOption(A_GOTHIC_KEY, nullptr, none, "log", "log", WALK_RUN, GAME_SCREEN_LOG);
					break;
				case K_NPC:
				{
					oCNpc* npc = (oCNpc*)vob;
					bool beast = npc->IsMonster() && !npc->IsOrc();
					if (!beast)
						AddOption(A_TALK, vob, none, "talk", "talk", WALK_WALK);
					AddOption(A_ATTACK, vob, none, "attack", "fight");
					if (HasSpells(hero))
						AddOption(A_SPELLS, vob, none, "cast", "magic");
					if (!beast)
						AddOption(A_FOLLOW, vob, none, "follow", "follow");
					selected = npc;
					break;
				}
				case K_DOWN:
					AddOption(A_LOOT, vob, none, "loot", "loot", WALK_WALK);
					AddOption(A_FINISH, vob, none, "finish", "finish");
					break;
				case K_BODY:
					AddOption(A_LOOT, vob, none, "loot", "loot", WALK_WALK);
					break;
				case K_ITEM: // (Telekinesis reaches items out of his reach: Gothic 1's idol, the third focus)
					AddOption(A_TAKE, vob, none, "take", "take");
					if (HasSpells(hero))
						AddOption(A_SPELLS, vob, none, "cast", "magic");
					break;
				case K_MOB:
				{
					const char* label;
					const char* icon;
					if (MobVerb((oCMobInter*)vob, label, icon))
						AddOption(A_USE, vob, none, label, icon);
					break;
				}
				case K_TARGET:
					AddOption(A_SHOOT, vob, none, "shoot", "shoot");
					if (HasSpells(hero))
						AddOption(A_SPELLS, vob, none, "cast", "magic");
					break;
				}
			}
			if (pie.empty())
				return;
			pieTarget = vob;
			if (pieTarget)
				pieTarget->AddRef();
			pieOpen = true;
			Log("pie: %s, %d option(s)", pieTitle[0] ? pieTitle : "the ground", (int)pie.size());
		}

		// the bubbles round the click (The Sims 4's ring: the first at the top, clockwise)
		static void LayoutPie()
		{
			int n = (int)pie.size();
			float radius = U(n <= 2 ? 70.0f : (n <= 4 ? 95.0f : 120.0f));
			float h = U(56.0f); // the bubble sprite's height: its body 44 high from 4 down, its shadow under it
			for (int i = 0; i < n; i++)
			{
				Option& o = pie[i];
				float a = (n == 1 ? -90.0f : -90.0f + i * 360.0f / n) * PI / 180.0f;
				if (n == 2)
					a = (i == 0 ? -150.0f : -30.0f) * PI / 180.0f;
				o.w = TextWidth("bold", o.text, 20.0f) + U(76.0f);
				o.h = h;
				float ax = pieX + cosf(a) * radius * 1.25f, ay = pieY + sinf(a) * radius;
				float c = cosf(a);
				o.x = c > 0.25f ? ax - U(10.0f) : (c < -0.25f ? ax - o.w + U(10.0f) : ax - o.w / 2.0f);
				o.y = ay - h / 2.0f;
			}
			// on the screen
			float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
			for (Option& o : pie)
			{
				minX = o.x < minX ? o.x : minX;
				minY = o.y < minY ? o.y : minY;
				maxX = o.x + o.w > maxX ? o.x + o.w : maxX;
				maxY = o.y + o.h > maxY ? o.y + o.h : maxY;
			}
			float shiftX = minX < U(8.0f) ? U(8.0f) - minX : (maxX > screenW - U(8.0f) ? screenW - U(8.0f) - maxX : 0.0f);
			float shiftY = minY < U(8.0f) ? U(8.0f) - minY : (maxY > screenH - U(8.0f) ? screenH - U(8.0f) - maxY : 0.0f);
			for (Option& o : pie)
			{
				o.x += shiftX;
				o.y += shiftY;
			}
			pieX += shiftX;
			pieY += shiftY;
		}

		static int HoveredOption()
		{
			for (int i = 0; i < (int)pie.size(); i++)
				if (Inside(pie[i].x, pie[i].y, pie[i].w, pie[i].h))
					return i;
			return -1;
		}

		static void ChooseOption(oCNpc* hero, int i)
		{
			if (i < 0 || i >= (int)pie.size())
				return;
			Action a = pie[i].action;
			Log("pie: %s chosen", pie[i].text);
			if (a.act == A_SPELLS) // its spells in a second ring, round the same spot
			{
				OpenSpellPie(hero, a.target);
				pieOpened = RealNow();
				if (pie.empty())
					ClosePie();
				return;
			}
			bool add = keyShift.held;
			Enqueue(hero, a, add);
			ClosePie();
		}

		static void DrawPie()
		{
			if (!pieOpen)
				return;
			LayoutPie();
			float grow = RealNow() - pieOpened;
			grow = grow > 0.12f ? 1.0f : grow / 0.12f;
			int hovered = HoveredOption();
			if (pieTitle[0])
				DrawTag(pieTitle, pieX, pieY - U(16.0f), 20.0f);
			else
				DrawSprite("circle", pieX - U(6.0f), pieY - U(6.0f), U(12.0f), U(12.0f), Argb(220, 255, 255, 255));
			for (int i = 0; i < (int)pie.size(); i++)
			{
				Option& o = pie[i];
				float x = pieX + (o.x - pieX) * grow, y = pieY + (o.y - pieY) * grow;
				bool on = i == hovered;
				DrawSlice(on ? "bubble_hover" : "bubble", x, y, o.w, o.h);
				float body = y + U(26.0f); // the body's middle
				DrawIcon(o.action.icon, x + U(30.0f), body, U(36.0f));
				float th = TextHeight("bold", 20.0f);
				DrawText("bold", o.text, x + U(54.0f), body - th / 2.0f, 20.0f, on ? WHITE : NAVY);
			}
		}

		// --- the HUD -----------------------------------------------------------------------------------------
		struct Button
		{
			const char* id;
			const char* icon;
			const char* label;
			float x, y, size;
			bool on;
		};
		static std::vector<Button> buttons;
		static const char* hoveredButton = nullptr;
		static int speedSetting = 1; // 0 paused, 1, 2, 3
		static int lastSpeed = 1;    // the speed Space goes back to
		static bool gothicPaused = false;

		static void SetSpeed(int speed)
		{
			if (!ztimer || !ogame)
				return;
			speedSetting = speed;
			if (speed > 0)
				lastSpeed = speed;
			static const float factors[4] = { 1.0f, 1.0f, 2.0f, 3.0f };
			if (speed == 0)
			{
				if (!gothicPaused)
					ogame->Pause();
				gothicPaused = true;
			}
			else
			{
				if (gothicPaused)
					ogame->Unpause();
				gothicPaused = false;
				ztimer->SetMotionFactor(factors[speed]);
			}
			Log("speed: %d", speed);
		}

		static void PressGothicKey(int logical);

		static void ContainerKey(oCNpc* hero, int logical, int raw = 0);

		static int xferModeNow(); // (The Sims' item screen, further down)
		static void XferButtons(oCNpc* hero);
		static void XferButton(oCNpc* hero, const char* id);

		static void LayoutButtons(oCNpc* hero)
		{
			buttons.clear();
			if (xferModeNow() != 0) // The Sims' item screen: its own buttons
			{
				XferButtons(hero);
				return;
			}
			if (ContainerOpen(hero)) // Gothic's item screen: its keys as buttons, bottom middle
			{
				float m = U(52.0f), g = U(8.0f), my = screenH - U(20.0f) - m;
				Button keys[] = {
					{ "c_up", "up", "up", 0, my, m, false }, { "c_down", "down", "down", 0, my, m, false },
					{ "c_left", "left", "left", 0, my, m, false }, { "c_right", "right", "right", 0, my, m, false },
					{ "c_use", "use", "use", 0, my, m, false }, { "c_close", "stop", "close", 0, my, m, false },
				};
				int n = sizeof keys / sizeof keys[0];
				float x = (screenW - (n * m + (n - 1) * g)) / 2.0f;
				for (int i = 0; i < n; i++)
				{
					keys[i].x = x + i * (m + g);
					buttons.push_back(keys[i]);
				}
				return;
			}
			float size = U(58.0f), gap = U(10.0f), y = screenH - U(24.0f) - size;
			int mode = hero->GetWeaponMode();
			Button row[] = {
				{ "fight", "fight", "fight", 0, 0, size, fightTarget != nullptr },
				{ "auto", "auto", "auto_fight", 0, 0, size, autoFight },
				{ "weapon", "weapon", mode == NPC_WEAPON_NONE ? "draw" : "sheathe", 0, 0, size, mode != NPC_WEAPON_NONE },
				{ "magic", "magic", "magic", 0, 0, size, mode == NPC_WEAPON_MAG },
				{ "inventory", "inventory", "inventory", 0, 0, size, false },
				{ "status", "status", "status", 0, 0, size, false },
				{ "log", "log", "log", 0, 0, size, false },
				{ "map", "map", "map", 0, 0, size, false },
				{ "camera", "camera", "camera_follow", 0, 0, size, follow },
			};
			int n = sizeof row / sizeof row[0];
			float total = n * size + (n - 1) * gap, x = (screenW - total) / 2.0f;
			for (int i = 0; i < n; i++)
			{
				row[i].x = x + i * (size + gap);
				row[i].y = y;
				buttons.push_back(row[i]);
			}
			// the game's speed, bottom right
			float s = U(44.0f), sx = screenW - U(24.0f) - 4 * s - 3 * U(6.0f), sy = screenH - U(24.0f) - s;
			Button speeds[] = {
				{ "speed0", "pause", "pause", sx, sy, s, speedSetting == 0 },
				{ "speed1", "play", "speed1", sx + (s + U(6.0f)), sy, s, speedSetting == 1 },
				{ "speed2", "speed2", "speed2", sx + 2 * (s + U(6.0f)), sy, s, speedSetting == 2 },
				{ "speed3", "speed3", "speed3", sx + 3 * (s + U(6.0f)), sy, s, speedSetting == 3 },
			};
			for (Button& b : speeds)
				buttons.push_back(b);
			buttons.push_back({ "controls", "camera", "gothic_controls", sx + 3 * (s + U(6.0f)), sy - s - U(10.0f), s, false });
			// while he uses a mob: Gothic's direction keys
			if (hero->GetInteractMob())
			{
				float m = U(52.0f), my = y - m - U(18.0f), mx = screenW / 2.0f - (5 * m + 4 * U(8.0f)) / 2.0f;
				Button mob[] = {
					{ "standup", "standup", "stand_up", mx, my, m, false },
					{ "left", "left", "left", mx + (m + U(8.0f)), my, m, false },
					{ "right", "right", "right", mx + 2 * (m + U(8.0f)), my, m, false },
					{ "up", "up", "up", mx + 3 * (m + U(8.0f)), my, m, false },
					{ "down", "down", "down", mx + 4 * (m + U(8.0f)), my, m, false },
				};
				for (Button& b : mob)
					buttons.push_back(b);
			}
		}

		static const Button* ButtonAt()
		{
			for (const Button& b : buttons)
			{
				float r = b.size / 2.0f, dx = pointerX - (b.x + r), dy = pointerY - (b.y + r);
				if (dx * dx + dy * dy <= r * r)
					return &b;
			}
			return nullptr;
		}

		static void PressButton(oCNpc* hero, const char* id)
		{
			Log("button: %s", id);
			if (!strcmp(id, "fight"))
			{
				if (fightTarget)
					StopFight(hero, "the button");
				else
					FightButton(hero);
			}
			else if (!strcmp(id, "auto"))
			{
				autoFight = !autoFight;
				Log("auto fight %s", autoFight ? "on" : "off");
			}
			else if (!strcmp(id, "weapon"))
			{
				Action a;
				a.act = hero->GetWeaponMode() == NPC_WEAPON_NONE ? A_DRAW : A_SHEATHE;
				a.label = a.act == A_DRAW ? "draw" : "sheathe";
				a.icon = "weapon";
				Enqueue(hero, a, true);
			}
			else if (!strcmp(id, "inventory"))
				PressGothicKey(GAME_INVENTORY);
			else if (!strcmp(id, "status"))
				PressGothicKey(GAME_SCREEN_STATUS);
			else if (!strcmp(id, "log"))
				PressGothicKey(GAME_SCREEN_LOG);
			else if (!strcmp(id, "map"))
				PressGothicKey(GAME_SCREEN_MAP);
			else if (!strcmp(id, "camera"))
				CenterOnHero(hero);
			else if (!strcmp(id, "magic")) // his spells in a ring over the button, cast where he faces
			{
				if (!HasSpells(hero))
					Think("no_spells");
				else
				{
					ClosePie();
					pieX = pointerX;
					pieY = pointerY - U(150.0f);
					strncpy_s(pieTitle, Label("magic"), _TRUNCATE);
					OpenSpellPie(hero, nullptr);
					pieOpen = !pie.empty();
					pieOpened = RealNow();
				}
			}
			else if (!strncmp(id, "speed", 5))
				SetSpeed(id[5] - '0');
			else if (!strcmp(id, "controls"))
			{
				simsMode = false;
				ClosePie();
				Log("controls: Gothic's (F8 back)");
			}
			else if (!strcmp(id, "c_up"))
				ContainerKey(hero, GAME_UP);
			else if (!strcmp(id, "c_down"))
				ContainerKey(hero, GAME_DOWN);
			else if (!strcmp(id, "c_left"))
				ContainerKey(hero, GAME_LEFT);
			else if (!strcmp(id, "c_right"))
				ContainerKey(hero, GAME_RIGHT);
			else if (!strcmp(id, "c_use"))
				ContainerKey(hero, GAME_ACTION);
			else if (!strcmp(id, "c_close"))
				ContainerKey(hero, 0, KEY_ESCAPE);
			else if (!strncmp(id, "x_", 2))
				XferButton(hero, id);
			else
			{
				static const struct { const char* id; int key; } MOB_KEYS[] = {
					{ "standup", GAME_DOWN }, { "left", GAME_LEFT }, { "right", GAME_RIGHT }, { "up", GAME_UP }, { "down", GAME_DOWN },
				};
				oCMobInter* mob = hero->GetInteractMob();
				bool ladder = mob && zDYNAMIC_CAST<oCMobLadder>(mob);
				bool lock = mob && zDYNAMIC_CAST<oCMobLockable>(mob) && ((oCMobLockable*)mob)->locked;
				for (auto& m : MOB_KEYS)
					if (!strcmp(id, m.id))
					{
						bool leave = !strcmp(id, "standup") || !strcmp(id, "down");
						if (leave || ladder) // (left and right keep the action key held: Gothic 1's lock picks)
							ReleaseKeys();
						if (leave)
							mobHoldOff = true;
						GiveKey(m.key, ladder && !leave ? 30.0f : 0.2f); // a ladder: held till the end
						if (!GOTHIC2 && lock && (m.key == GAME_LEFT || m.key == GAME_RIGHT))
							GiveKey(GAME_ACTION, 0.3f); // Gothic 1 turns the lock pick with the action key held
					}
			}
		}

		static void DrawButtons()
		{
			for (const Button& b : buttons)
			{
				bool hot = hoveredButton && !strcmp(hoveredButton, b.id);
				DrawSprite(hot ? "button_hover" : (b.on ? "button_on" : "button"), b.x, b.y, b.size, b.size);
				DrawIcon(b.icon, b.x + b.size / 2.0f, b.y + b.size / 2.0f, b.size * 0.62f);
				if (hot) // its name above it, with its key
				{
					char text[128];
					const char* key = !strcmp(b.id, "controls") ? "F8" : (!strcmp(b.id, "speed0") ? Label("space_key")
						: (!strcmp(b.id, "speed1") ? "1" : (!strcmp(b.id, "speed2") ? "2" : (!strcmp(b.id, "speed3") ? "3" : nullptr))));
					if (key)
						sprintf_s(text, "%s (%s)", Label(b.label), key);
					else
						strncpy_s(text, Label(b.label), _TRUNCATE);
					float tx = b.x + b.size / 2.0f, half = TextWidth("normal", text, 17.0f) / 2.0f + U(14.0f);
					tx = tx + half > screenW - U(4.0f) ? screenW - U(4.0f) - half : tx; // (the speed buttons sit at the edge)
					DrawTag(text, tx, b.y - U(34.0f), 17.0f);
				}
			}
		}

		// the hero's panel: the plumbob, his name, health and mana (bottom left)
		static void DrawPanel(oCNpc* hero)
		{
			float x = U(20.0f), w = U(360.0f), h = U(112.0f), y = screenH - U(20.0f) - h;
			DrawSlice("panel", x, y, w, h);
			float t = RealNow();
			float bob = sinf(t * 2.0f) * U(3.0f), spin = 0.35f + 0.65f * fabsf(cosf(t * 1.3f));
			float pw = U(34.0f), ph = U(68.0f);
			DrawSprite("plumbob", x + U(38.0f) - pw * spin / 2.0f, y + U(22.0f) + bob, pw * spin, ph);
			char name[96];
			NameOf(hero, name, sizeof name);
			DrawText("title", name, x + U(78.0f), y + U(12.0f), 26.0f, NAVY);
			auto bar = [&](float by, int value, int max, const zCOLOR& color)
			{
				float bw = w - U(98.0f), bh = U(18.0f), bx = x + U(78.0f);
				DrawSlice("bar_back", bx, by, bw, bh, Argb(255, 220, 226, 236));
				float share = max > 0 ? (float)value / max : 0.0f;
				share = share < 0.0f ? 0.0f : (share > 1.0f ? 1.0f : share);
				if (share > 0.0f)
					DrawSlice("bar", bx, by, bw * share > bh ? bw * share : bh, bh, color);
				char text[32];
				sprintf_s(text, "%d / %d", value, max);
				DrawText("small", text, bx + bw - TextWidth("small", text, 14.0f) - U(4.0f), by - U(1.0f), 14.0f, NAVY);
			};
			int hp = hero->attribute[NPC_ATR_HITPOINTS], hpMax = hero->attribute[NPC_ATR_HITPOINTSMAX];
			float share = hpMax > 0 ? (float)hp / hpMax : 0.0f;
			zCOLOR healthColor = share > 0.6f ? GREEN : (share > 0.3f ? Argb(255, 0xf2, 0xc0, 0x2c) : RED);
			bar(y + U(54.0f), hp, hpMax, healthColor);
			bar(y + U(80.0f), hero->attribute[NPC_ATR_MANA], hero->attribute[NPC_ATR_MANAMAX], BLUE);
		}

		// the queue, top left: the current action bigger; a click cancels one
		static int hoveredQueue = -1;

		static void DrawQueue()
		{
			hoveredQueue = -1;
			float x = U(24.0f), y = U(24.0f);
			for (int i = 0; i < (int)queue.size(); i++)
			{
				const Action& a = queue[i];
				float size = U(i == 0 ? 76.0f : 56.0f);
				float cy = i == 0 ? y : y + U(10.0f);
				bool hot = Inside(x, cy, size, size);
				if (hot)
					hoveredQueue = i;
				DrawSprite("circle", x - U(3.0f), cy - U(3.0f), size + U(6.0f), size + U(6.0f), Argb(120, 0, 0, 0));
				DrawSprite("circle", x, cy, size, size, i == 0 ? WHITE : Argb(235, 235, 240, 248));
				DrawIcon(a.icon, x + size / 2.0f, cy + size / 2.0f, size * 0.66f);
				if (i == 0)
					DrawSprite("ring", x, cy, size, size, BLUE);
				if (hot)
					DrawSprite("cancel", x + size - U(22.0f), cy - U(2.0f), U(24.0f), U(24.0f));
				if (i == 0)
				{
					char text[128];
					char name[96] = "";
					if (a.target && Alive(a.target))
						NameOf(a.target, name, sizeof name);
					sprintf_s(text, name[0] ? "%s: %s" : "%s", Label(a.label), name);
					DrawText("normal", text, x, y + size + U(6.0f), 18.0f, WHITE);
				}
				x += size + U(10.0f);
			}
			if (fightTarget) // the fight, as Sims 4's queue shows a running interaction
			{
				char name[96], text[128];
				NameOf(fightTarget, name, sizeof name);
				sprintf_s(text, "%s: %s", Label("fighting"), name);
				float size = U(64.0f);
				bool hot = Inside(x, y, size, size);
				DrawSprite("circle", x, y, size, size, WHITE);
				DrawIcon("fight", x + size / 2.0f, y + size / 2.0f, size * 0.66f);
				DrawSprite("ring", x, y, size, size, RED);
				if (hot)
				{
					hoveredQueue = 1000;
					DrawSprite("cancel", x + size - U(22.0f), y - U(2.0f), U(24.0f), U(24.0f));
				}
				DrawText("normal", text, x, y + size + U(6.0f), 18.0f, WHITE);
				// the enemy's health, top middle
				int hp = fightTarget->attribute[NPC_ATR_HITPOINTS], max = fightTarget->attribute[NPC_ATR_HITPOINTSMAX];
				float bw = U(320.0f), bx = (screenW - bw) / 2.0f, by = U(26.0f);
				DrawTag(name, screenW / 2.0f, by - U(4.0f), 18.0f);
				DrawSlice("bar_back", bx, by + U(30.0f), bw, U(12.0f), Argb(220, 60, 20, 20));
				float share = max > 0 ? (float)hp / max : 0.0f;
				share = share < 0.0f ? 0.0f : (share > 1.0f ? 1.0f : share);
				if (share > 0.0f)
					DrawSlice("bar", bx, by + U(30.0f), bw * share > U(12.0f) ? bw * share : U(12.0f), U(12.0f), RED);
			}
		}

		// the icon over each thing he can use near him (the one under the pointer bigger, with its name)
		static void DrawThings(oCNpc* hero)
		{
			zVEC3 at = hero->GetPositionWorld();
			for (zCVob* vob : things)
			{
				if (!Alive(vob))
					continue;
				bool hot = vob == hover.vob;
				float d = (vob->GetPositionWorld() - at).Length();
				if (!hot && (!settings.icons || d > 1500.0f))
					continue;
				zTBBox3D box = vob->bbox3D;
				zVEC3 top((box.mins[0] + box.maxs[0]) / 2.0f, box.maxs[1] + 25.0f, (box.mins[2] + box.maxs[2]) / 2.0f);
				float sx, sy;
				if (!Project(top, sx, sy))
					continue;
				const char* label;
				const char* icon;
				DefaultVerb(hero, vob, label, icon);
				float size = U(hot ? 46.0f : 30.0f);
				int alpha = hot ? 255 : (d < 800.0f ? 220 : 150);
				DrawSprite("circle", sx - size / 2.0f - U(2.0f), sy - size - U(2.0f), size + U(4.0f), size + U(4.0f), Argb(alpha / 2, 0, 0, 0));
				DrawSprite("circle", sx - size / 2.0f, sy - size, size, size, Argb(alpha, 255, 255, 255));
				DrawIcon(icon, sx, sy - size / 2.0f, size * 0.7f, Argb(alpha, 255, 255, 255));
				if (hot)
				{
					char name[96];
					NameOf(vob, name, sizeof name);
					DrawTag(name, sx, sy - size - U(36.0f), 18.0f);
				}
			}
		}

		// the plumbob over his head (turning), his thought bubble over it, the spot he walks to
		static void DrawHero(oCNpc* hero)
		{
			zVEC3 head = hero->GetPositionWorld() + zVEC3(0.0f, 110.0f, 0.0f);
			if (zCModel* model = hero->GetModel())
				if (zCModelNodeInst* node = model->SearchNode(zSTRING("BIP01 HEAD")))
					head = model->GetNodePositionWorld(node) + zVEC3(0.0f, 45.0f, 0.0f);
			float sx, sy;
			float t = RealNow();
			if (settings.plumbob && Project(head, sx, sy))
			{
				float scale = 1300.0f / ((head - camPos).Length() + 1.0f);
				scale = scale < 0.4f ? 0.4f : (scale > 1.6f ? 1.6f : scale);
				float w = U(26.0f) * scale, h = U(52.0f) * scale, spin = 0.3f + 0.7f * fabsf(cosf(t * 1.3f));
				float bob = sinf(t * 2.0f) * U(4.0f) * scale;
				DrawSprite("plumbob", sx - w * spin / 2.0f, sy - h + bob, w * spin, h);
				if (t < thoughtUntil)
					DrawTag(thought, sx, sy - h - U(40.0f), 18.0f);
			}
			if (goMarkerOn && Project(goMarker, sx, sy))
			{
				float pulse = 1.0f + 0.12f * sinf(t * 6.0f);
				float w = U(64.0f) * pulse, h = U(26.0f) * pulse;
				DrawSprite("target", sx - w / 2.0f, sy - h / 2.0f, w, h, Argb(220, 255, 255, 255));
			}
		}

		static void DrawPointer()
		{
			bool hand = (hover.vob && !pieOpen) || (pieOpen && HoveredOption() >= 0) || hoveredButton || hoveredQueue >= 0;
			float s = U(32.0f); // the hotspots: the arrow's tip at (1, 1), the hand's fingertip at (12, 2)
			DrawSprite(hand ? "pointer_hand" : "pointer", pointerX - s * (hand ? 12.0f : 1.0f) / 32.0f, pointerY - s * (hand ? 2.0f : 1.0f) / 32.0f, s, s);
		}

		// --- the frame: input, clicks, the UI ------------------------------------------------------------------
		static void ReadInput()
		{
			float dx = 0.0f, dy = 0.0f, dz = 0.0f;
			if (zinput)
				zinput->GetMousePos(dx, dy, dz);
			if (!GameHasFocus())
				dx = dy = dz = 0.0f;
			mouseLeft.Update(ButtonDown(VK_LBUTTON));
			mouseRight.Update(ButtonDown(VK_RBUTTON));
			mouseMiddle.Update(ButtonDown(VK_MBUTTON));
			keyShift.Update(KeyDown(KEY_LSHIFT) || KeyDown(KEY_RSHIFT));
			keyCtrl.Update(KeyDown(KEY_LCONTROL));
			keySpace.Update(ButtonDown(VK_SPACE));
			for (int i = 0; i < 3; i++)
				keySpeed[i].Update(ButtonDown('1' + i));
			mouseDX = dx;
			mouseDY = dy;
			if (!mouseMiddle.held && !mouseRight.held) // the pointer stays while the camera turns
			{
				pointerX += dx * settings.pointerSpeed;
				pointerY += dy * settings.pointerSpeed;
			}
			pointerX = pointerX < 0.0f ? 0.0f : (pointerX > screenW - 1 ? (float)(screenW - 1) : pointerX);
			pointerY = pointerY < 0.0f ? 0.0f : (pointerY > screenH - 1 ? (float)(screenH - 1) : pointerY);
			if (dz != 0.0f)
				wheelSteps += dz > 0.0f ? 1 : -1;
		}

		// --- Gothic's screens with the mouse ----------------------------------------------------------------
		// The dialogue's choices: the one under the pointer is chosen (zCViewDialogChoice::Select), a click
		// says it (Gothic's action key for a moment), the wheel goes through them.
		static int dialogLogged = -1;

		static void DialogMouse()
		{
			oCInformationManager& info = oCInformationManager::GetInformationManager();
			zCViewDialogChoice* dlg = info.DlgChoice;
			if (!dlg || !info.IsWaitingForSelection || dlg->Choices <= 0)
				return;
			zCFont* font = dlg->ViewFont.Font;
			float lineH = font ? (float)font->height : 20.0f;
			int n = dlg->ListTextLines.GetNum();
			float x0 = (float)dlg->PixelPosition.X, x1 = x0 + dlg->PixelSize.X;
			float top = (float)(dlg->PixelPosition.Y + dlg->OffsetTextPixel.Y);
			if (dialogLogged != dlg->Choices * 1000 + n)
			{
				dialogLogged = dlg->Choices * 1000 + n;
				Log("dialogue: %d choices, %d lines, box %d %d %d x %d, text offset %d %d, line %.0f px, first line at %d, line start %d", dlg->Choices,
					n, dlg->PixelPosition.X, dlg->PixelPosition.Y, dlg->PixelSize.X, dlg->PixelSize.Y, dlg->OffsetTextPixel.X, dlg->OffsetTextPixel.Y,
					lineH, n > 0 && dlg->ListTextLines[0] ? dlg->ListTextLines[0]->PixelPosition.Y : -1, dlg->LineStart);
			}
			int hovered = -1;
			if (pointerX >= x0 && pointerX < x1)
				for (int i = 0; i < n; i++)
				{
					zCViewText2* line = dlg->ListTextLines[i];
					if (!line)
						continue;
					float y = top + line->PixelPosition.Y;
					if (pointerY >= y && pointerY < y + lineH)
						hovered = i;
				}
			if (hovered >= 0 && hovered < dlg->Choices && hovered != dlg->ChoiceSelected)
				dlg->Select(hovered);
			if (wheelSteps != 0)
			{
				if (wheelSteps > 0)
					dlg->SelectPrevious();
				else
					dlg->SelectNext();
				wheelSteps = 0;
			}
			if (mouseLeft.pressed && hovered >= 0)
			{
				Log("dialogue: choice %d clicked", hovered);
				dialogConfirmUntil = RealNow() + 0.12f;
				dialogConfirmFrame = frameCount;
			}
		}

		// Gothic's item screens (his inventory, a chest's, trading, a body's): buttons that give the open
		// screen Gothic's keys (the active one's HandleEvent, the key bound to that logical key); the
		// wheel goes up and down.
		static oCItemContainer* ActiveContainer(oCNpc* hero)
		{
			zCList<oCItemContainer>& list = oCItemContainer::contList;
			oCItemContainer* open = nullptr;
			for (zCList<oCItemContainer>* node = list.next; node; node = node->next)
				if (node->data && node->data->IsOpen())
				{
					if (node->data->IsActive())
						return node->data;
					open = node->data;
				}
			if (hero->inventory2.IsOpen())
				return &hero->inventory2;
			return open;
		}

		static void ContainerKey(oCNpc* hero, int logical, int raw)
		{
			oCItemContainer* c = ActiveContainer(hero);
			if (!c)
				return;
			int key = raw;
			if (!key)
			{
				zCArray<unsigned short> keys;
				zinput->GetBinding((unsigned short)logical, keys);
				key = keys.GetNum() ? keys[0] : 0;
			}
			if (key)
			{
				Log("item screen: key %d (logical %d)", key, logical);
				c->HandleEvent(key);
			}
		}

		static void ContainerMouse(oCNpc* hero)
		{
			if (wheelSteps != 0)
			{
				ContainerKey(hero, wheelSteps > 0 ? GAME_UP : GAME_DOWN);
				wheelSteps = 0;
			}
		}

		// --- The Sims' item screen: chests, bodies and trading ---------------------------------------------
		// Over Gothic's own screen (which stays open underneath, its logic Gothic's): the things on each
		// side as rows; a click moves one row's stack (Shift: one piece; trading: one piece, Shift the
		// stack). A chest's go through its own Insert and Remove (oCMobContainer), a body's through its
		// inventory (RemoveFromInv, PutInInv). Gothic 1's trading is barter: the trader's goods and his
		// offer, your offer and your things; a click moves a thing into an offer and back
		// (oCViewDialogItemContainer::InsertItem, RemoveItem), "Trade" is Gothic's accept (OnAccept: the
		// values must match), "Reset" its TransferReset. Gothic 2 buys and sells at once: the row is
		// chosen in Gothic's list and Gothic's own transfer pays (OnTransferLeft/Right with the count).
		enum { XFER_NONE, XFER_CHEST, XFER_BODY, XFER_TRADE };
		struct XRow
		{
			oCItem* item;
			int column;
			float x, y, w, h;
		};
		struct XColumn
		{
			char title[96];
			std::vector<oCItem*> items;
			float x, y, w, h;
			int scroll;
		};
		static int xferMode = XFER_NONE;
		static oCMobContainer* xferChest = nullptr;
		static oCNpc* xferOwner = nullptr; // the body, the trader
		static oCViewDialogTrade* xferTrade = nullptr;
		static XColumn xcols[4];
		static int xcolCount = 0;
		static std::vector<XRow> xrows;
		static int xHovered = -1;
		static int xScroll[4] = {};
		static int xLastMode = XFER_NONE;
		static float xPanelX, xPanelY, xPanelW, xPanelH;
		static int g2Direction[2] = { 0, 0 }; // Gothic 2: OnTransferLeft (-1) or Right (+1) found to buy, to sell

		static void ListOf(zCListSort<oCItem>* list, std::vector<oCItem*>& out)
		{
			for (zCListSort<oCItem>* node = list ? list->next : nullptr; node; node = node->next)
				if (node->data && node->data->amount > 0)
					out.push_back(node->data);
		}

		static void InventoryOf(oCNpc* npc, std::vector<oCItem*>& out)
		{
			npc->inventory2.UnpackAllItems();
#if ENGINE == Engine_G2A
			ListOf(&npc->inventory2.inventory, out);
#else
			for (int c = 0; c < INV_MAX; c++)
				ListOf(&npc->inventory2.inventory[c], out);
#endif
		}

		static int DetectXfer(oCNpc* hero)
		{
			xferChest = nullptr;
			xferOwner = nullptr;
			xferTrade = nullptr;
			oCInformationManager& info = oCInformationManager::GetInformationManager();
			if (!info.HasFinished() && info.Mode == oCInformationManager::INFO_MGR_MODE_TRADE && info.DlgTrade && info.Npc)
			{
				xferTrade = info.DlgTrade;
				xferOwner = xferTrade->NpcLeft ? xferTrade->NpcLeft : info.Npc;
				return XFER_TRADE;
			}
			if (!ContainerOpen(hero))
				return XFER_NONE;
			if (oCMobContainer* chest = zDYNAMIC_CAST<oCMobContainer>(hero->GetInteractMob()))
			{
				xferChest = chest;
				return XFER_CHEST;
			}
			// a body: the open container that isn't his own inventory belongs to an npc (oCStealContainer)
			for (zCList<oCItemContainer>* node = oCItemContainer::contList.next; node; node = node->next)
				if (node->data && node->data != &hero->inventory2 && node->data->IsOpen())
				{
					oCNpc* owner = ((oCStealContainer*)node->data)->owner;
					for (zCVob* vob : things)
						if (vob == owner && Alive(vob))
							xferOwner = owner;
					if (!xferOwner && hero->GetFocusVob() == owner)
						xferOwner = owner;
				}
			return xferOwner ? XFER_BODY : XFER_NONE;
		}

		// Gothic's own lists of an npc's things rebuilt (its plunder screen, the trader's goods)
		static void RefreshStealLists(oCNpc* hero, oCNpc* owner)
		{
			for (zCList<oCItemContainer>* node = oCItemContainer::contList.next; node; node = node->next)
				if (node->data && node->data != &hero->inventory2 && node->data->IsOpen())
				{
					if (owner && ((oCStealContainer*)node->data)->owner == owner)
						((oCStealContainer*)node->data)->CreateList();
					node->data->CheckSelectedItem();
				}
			hero->inventory2.CheckSelectedItem();
		}

		static void BuildXfer(oCNpc* hero)
		{
			for (XColumn& c : xcols)
			{
				c.items.clear();
				c.title[0] = 0;
			}
			xcolCount = 0;
			char name[96] = "";
			switch (xferMode)
			{
			case XFER_CHEST:
				strncpy_s(xcols[0].title, xferChest->GetName().ToChar(), _TRUNCATE);
				ListOf(&xferChest->containList, xcols[0].items);
				strncpy_s(xcols[1].title, Label("your_items"), _TRUNCATE);
				InventoryOf(hero, xcols[1].items);
				xcolCount = 2;
				break;
			case XFER_BODY:
				NameOf(xferOwner, name, sizeof name);
				strncpy_s(xcols[0].title, name, _TRUNCATE);
				InventoryOf(xferOwner, xcols[0].items);
				strncpy_s(xcols[1].title, Label("your_items"), _TRUNCATE);
				InventoryOf(hero, xcols[1].items);
				xcolCount = 2;
				break;
			case XFER_TRADE:
				NameOf(xferOwner, name, sizeof name);
				strncpy_s(xcols[0].title, name, _TRUNCATE);
				if (xferTrade->DlgInventoryNpc && xferTrade->DlgInventoryNpc->StealContainer)
					ListOf(xferTrade->DlgInventoryNpc->StealContainer->GetContents(), xcols[0].items);
#if ENGINE == Engine_G2A
				strncpy_s(xcols[1].title, Label("your_items"), _TRUNCATE);
				InventoryOf(hero, xcols[1].items);
				xcolCount = 2;
#else
				strncpy_s(xcols[1].title, Label("their_offer"), _TRUNCATE);
				if (xferTrade->DlgContainerNpc && xferTrade->DlgContainerNpc->ItemContainer)
					ListOf(xferTrade->DlgContainerNpc->ItemContainer->GetContents(), xcols[1].items);
				strncpy_s(xcols[2].title, Label("your_offer"), _TRUNCATE);
				if (xferTrade->DlgContainerPlayer && xferTrade->DlgContainerPlayer->ItemContainer)
					ListOf(xferTrade->DlgContainerPlayer->ItemContainer->GetContents(), xcols[2].items);
				strncpy_s(xcols[3].title, Label("your_items"), _TRUNCATE);
				InventoryOf(hero, xcols[3].items);
				xcolCount = 4;
#endif
				break;
			}
			// the panel and its columns' rows (the 1080-line reference: rows 34 high)
			xPanelW = (float)screenW - U(120.0f);
			xPanelW = xPanelW > U(xcolCount == 4 ? 1500.0f : 1100.0f) ? U(xcolCount == 4 ? 1500.0f : 1100.0f) : xPanelW;
			xPanelH = (float)screenH - U(220.0f);
			xPanelH = xPanelH > U(760.0f) ? U(760.0f) : xPanelH;
			xPanelX = (screenW - xPanelW) / 2.0f;
			xPanelY = (screenH - xPanelH) / 2.0f - U(30.0f);
			float gap = U(16.0f), top = xPanelY + U(64.0f), bottom = xPanelY + xPanelH - U(84.0f);
			float cw = (xPanelW - U(40.0f) - gap * (xcolCount - 1)) / (xcolCount > 0 ? xcolCount : 1);
			float rowH = U(34.0f);
			int visible = (int)((bottom - top - U(36.0f)) / rowH);
			xrows.clear();
			for (int c = 0; c < xcolCount; c++)
			{
				XColumn& col = xcols[c];
				col.x = xPanelX + U(20.0f) + c * (cw + gap);
				col.y = top;
				col.w = cw;
				col.h = bottom - top;
				int most = (int)col.items.size() - visible;
				xScroll[c] = xScroll[c] > most ? most : xScroll[c];
				xScroll[c] = xScroll[c] < 0 ? 0 : xScroll[c];
				col.scroll = xScroll[c];
				for (int i = col.scroll; i < (int)col.items.size() && i < col.scroll + visible; i++)
					xrows.push_back({ col.items[i], c, col.x + U(6.0f), top + U(36.0f) + (i - col.scroll) * rowH, cw - U(12.0f), rowH - U(4.0f) });
			}
		}

		// one row clicked: its thing moved (see above)
		static void XferClick(oCNpc* hero, const XRow& row)
		{
			oCItem* item = row.item;
			int stack = item->amount;
			bool trading = xferMode == XFER_TRADE;
			int amount = trading ? (keyShift.held ? stack : 1) : (keyShift.held ? 1 : stack);
			amount = amount < 1 ? 1 : (amount > stack ? stack : amount);
			bool equipped = (item->flags & ITM_FLAG_ACTIVE) != 0;
			char name[96];
			strncpy_s(name, item->name.ToChar(), _TRUNCATE);
			if (xferMode == XFER_CHEST)
			{
				if (row.column == 0)
				{
					if (oCItem* got = xferChest->Remove(item, amount))
						hero->PutInInv(got);
				}
				else if (!equipped)
				{
					if (oCItem* got = hero->RemoveFromInv(item, amount))
						xferChest->Insert(got);
				}
			}
			else if (xferMode == XFER_BODY)
			{
				if (row.column == 0)
				{
					if (oCItem* got = xferOwner->RemoveFromInv(item, amount))
						hero->PutInInv(got);
				}
				else if (!equipped)
				{
					if (oCItem* got = hero->RemoveFromInv(item, amount))
						xferOwner->PutInInv(got);
				}
				RefreshStealLists(hero, xferOwner);
			}
			else if (trading)
			{
#if ENGINE == Engine_G2A
				// Gothic 2: the row chosen in Gothic's list, then Gothic's transfer (it pays)
				bool buy = row.column == 0;
				if (!buy && equipped)
					return;
				oCItemContainer* list = buy ? (oCItemContainer*)xferTrade->DlgInventoryNpc->StealContainer : (oCItemContainer*)&hero->inventory2;
				int index = 0, found = -1;
				for (zCListSort<oCItem>* node = list->GetContents() ? list->GetContents()->next : nullptr; node; node = node->next, index++)
					if (node->data == item)
						found = index;
				if (found < 0)
					return;
				list->selectedItem = found;
				list->offset = found;
				list->CheckSelectedItem();
				xferTrade->OnSection(buy ? oCViewDialogTrade::TRADE_SECTION_LEFT_INVENTORY : oCViewDialogTrade::TRADE_SECTION_RIGHT_INVENTORY);
				int before = item->amount;
				int& dir = g2Direction[buy ? 0 : 1];
				for (int attempt = 0; attempt < 2; attempt++)
				{
					int d = dir ? dir : (buy ? 1 : -1);
					if (attempt == 1)
						d = -d;
					if (d > 0)
						xferTrade->OnTransferRight((short)amount);
					else
						xferTrade->OnTransferLeft((short)amount);
					bool moved = false;
					for (zCListSort<oCItem>* node = list->GetContents() ? list->GetContents()->next : nullptr; node; node = node->next)
						moved = moved || node->data == item;
					moved = !moved || item->amount != before;
					if (moved)
					{
						if (!dir)
							Log("trade: Gothic 2 %s with OnTransfer%s", buy ? "buys" : "sells", d > 0 ? "Right" : "Left");
						dir = d;
						break;
					}
				}
#else
				// Gothic 1: into an offer and back
				oCViewDialogItemContainer* theirs = xferTrade->DlgContainerNpc;
				oCViewDialogItemContainer* yours = xferTrade->DlgContainerPlayer;
				switch (row.column)
				{
				case 0:
					if (oCItem* got = xferOwner->RemoveFromInv(item, amount))
						theirs->InsertItem(got);
					break;
				case 1:
					theirs->RemoveItem(item);
					xferOwner->PutInInv(item);
					break;
				case 2:
					yours->RemoveItem(item);
					hero->PutInInv(item);
					break;
				case 3:
					if (!equipped)
						if (oCItem* got = hero->RemoveFromInv(item, amount))
							yours->InsertItem(got);
					break;
				}
				theirs->UpdateValue();
				yours->UpdateValue();
				RefreshStealLists(hero, xferOwner);
#endif
			}
			Log("items: %s x%d from column %d", name, amount, row.column);
		}

		static void XferTakeAll(oCNpc* hero)
		{
			int moved = 0;
			for (int guard = 0; guard < 200; guard++)
			{
				BuildXfer(hero);
				if (xcols[0].items.empty())
					break;
				oCItem* item = xcols[0].items[0];
				int before = (int)xcols[0].items.size();
				XRow row = { item, 0, 0, 0, 0, 0 };
				bool shift = keyShift.held;
				keyShift.held = false;
				XferClick(hero, row);
				keyShift.held = shift;
				BuildXfer(hero);
				if ((int)xcols[0].items.size() >= before)
					break; // (it wouldn't go)
				moved++;
			}
			Log("items: %d stack(s) taken", moved);
		}

		// each frame an item screen is open: the panel's clicks, wheel and buttons (true: it is open)
		static bool XferInput(oCNpc* hero)
		{
			if (xferMode != xLastMode)
			{
				for (int& s : xScroll)
					s = 0;
				xLastMode = xferMode;
				if (xferMode != XFER_NONE)
					Log("item screen: %s", xferMode == XFER_CHEST ? "a chest" : (xferMode == XFER_BODY ? "a body" : "trading"));
			}
			if (xferMode == XFER_NONE)
				return false;
			BuildXfer(hero);
			xHovered = -1;
			for (int i = 0; i < (int)xrows.size(); i++)
				if (Inside(xrows[i].x, xrows[i].y, xrows[i].w, xrows[i].h))
					xHovered = i;
			if (wheelSteps != 0)
			{
				for (int c = 0; c < xcolCount; c++)
					if (Inside(xcols[c].x, xcols[c].y, xcols[c].w, xcols[c].h))
						xScroll[c] -= wheelSteps * 3;
				wheelSteps = 0;
			}
			if (mouseLeft.pressed)
			{
				if (xHovered >= 0)
					XferClick(hero, xrows[xHovered]);
				else if (hoveredButton)
					PressButton(hero, hoveredButton);
			}
			return true;
		}

		static void DrawXfer(oCNpc* hero)
		{
			if (xferMode == XFER_NONE)
				return;
			BuildXfer(hero);
			Fill(0.0f, 0.0f, (float)screenW, (float)screenH, Argb(170, 10, 14, 24)); // Gothic's screen underneath, dimmed
			DrawSlice("panel", xPanelX, xPanelY, xPanelW, xPanelH);
			const char* title = xferMode == XFER_TRADE ? Label("trade") : (xferMode == XFER_BODY ? Label("loot") : Label("open"));
			DrawText("title", title, xPanelX + U(26.0f), xPanelY + U(16.0f), 28.0f, NAVY);
			for (int c = 0; c < xcolCount; c++)
			{
				XColumn& col = xcols[c];
				DrawSlice("bar_back", col.x, col.y, col.w, col.h, Argb(70, 120, 140, 170));
				DrawText("bold", col.title, col.x + U(10.0f), col.y + U(6.0f), 20.0f, NAVY);
				if ((int)col.items.size() > 0)
				{
					char count[16];
					sprintf_s(count, "%d", (int)col.items.size());
					DrawText("small", count, col.x + col.w - TextWidth("small", count, 16.0f) - U(10.0f), col.y + U(9.0f), 16.0f, NAVY);
				}
			}
			for (int i = 0; i < (int)xrows.size(); i++)
			{
				const XRow& r = xrows[i];
				bool hot = i == xHovered;
				bool equipped = (r.item->flags & ITM_FLAG_ACTIVE) != 0;
				DrawSlice(hot ? "bubble_hover" : "bubble", r.x, r.y - U(2.0f), r.w, r.h + U(8.0f), equipped && !hot ? Argb(255, 225, 235, 215) : WHITE);
				char text[128];
				if (r.item->amount > 1)
					sprintf_s(text, "%s  x%d", r.item->name.ToChar(), r.item->amount);
				else
					strncpy_s(text, r.item->name.ToChar(), _TRUNCATE);
				float vw = 0.0f;
				char value[24] = "";
				if (r.item->GetValue() > 0)
				{
					sprintf_s(value, "%d", r.item->GetValue() * (r.item->amount > 1 ? 1 : 1));
					vw = TextWidth("small", value, 16.0f) + U(12.0f);
					DrawText("small", value, r.x + r.w - vw, r.y + U(6.0f), 16.0f, hot ? WHITE : Argb(255, 0x6a, 0x74, 0x8a));
				}
				// the name cut to fit
				float room = r.w - vw - U(28.0f);
				while (text[0] && TextWidth("normal", text, 18.0f) > room)
					text[strlen(text) - 1] = 0;
				DrawText("normal", text, r.x + U(14.0f), r.y + U(4.0f), 18.0f, hot ? WHITE : NAVY);
			}
#if ENGINE != Engine_G2A
			if (xferMode == XFER_TRADE && xferTrade->DlgContainerNpc && xferTrade->DlgContainerPlayer)
			{
				char text[128];
				sprintf_s(text, "%s: %u   %s: %u", Label("their_offer"), (unsigned)xferTrade->DlgContainerNpc->Value, Label("your_offer"),
					(unsigned)xferTrade->DlgContainerPlayer->Value);
				DrawText("bold", text, xPanelX + U(26.0f), xPanelY + xPanelH - U(72.0f), 20.0f, NAVY);
			}
#endif
			const char* hint = Label(xferMode == XFER_TRADE ? "trade_hint" : "xfer_hint");
			DrawText("small", hint, xPanelX + U(26.0f), xPanelY + xPanelH - U(40.0f), 16.0f, Argb(255, 0x6a, 0x74, 0x8a));
			DrawButtons();
		}

		static int xferModeNow() { return xferMode; }

		// the panel's buttons, bottom right in it: take all (a chest, a body), trade and reset (Gothic 1's
		// barter), close
		static void XferButtons(oCNpc* hero)
		{
			BuildXfer(hero);
			float m = U(56.0f), g = U(12.0f), y = xPanelY + xPanelH - m - U(14.0f);
			std::vector<Button> row;
			if (xferMode == XFER_CHEST || xferMode == XFER_BODY)
				row.push_back({ "x_takeall", "take", "take_all", 0, y, m, false });
#if ENGINE != Engine_G2A
			if (xferMode == XFER_TRADE)
			{
				row.push_back({ "x_accept", "trade", "accept_trade", 0, y, m, false });
				row.push_back({ "x_reset", "stop", "reset_trade", 0, y, m, false });
			}
#endif
			row.push_back({ "x_close", "stop", "close", 0, y, m, false });
			float x = xPanelX + xPanelW - U(20.0f) - row.size() * m - (row.size() - 1) * g;
			for (size_t i = 0; i < row.size(); i++)
			{
				row[i].x = x + i * (m + g);
				buttons.push_back(row[i]);
			}
		}

		static void XferButton(oCNpc* hero, const char* id)
		{
			if (!strcmp(id, "x_takeall"))
				XferTakeAll(hero);
#if ENGINE != Engine_G2A
			else if (!strcmp(id, "x_accept") && xferTrade)
			{
				Log("trade: accept (theirs %u, yours %u)", (unsigned)xferTrade->DlgContainerNpc->Value, (unsigned)xferTrade->DlgContainerPlayer->Value);
				xferTrade->OnAccept();
				RefreshStealLists(hero, xferOwner);
			}
			else if (!strcmp(id, "x_reset") && xferTrade)
			{
				xferTrade->TransferReset();
				RefreshStealLists(hero, xferOwner);
			}
#endif
			else if (!strcmp(id, "x_close"))
			{
				if (xferMode == XFER_TRADE && xferTrade)
					xferTrade->OnExit();
				else
					ContainerKey(hero, 0, KEY_ESCAPE);
			}
		}

		// the item screen over Gothic's drawing (after it), in the 2D state the UI draws in
		static void DrawXferOnTop()
		{
			oCNpc* hero = oCNpc::player;
			if (!uiReady || !Active(hero) || xferMode == XFER_NONE)
				return;
			int zWrite = zrenderer->GetZBufferWriteEnabled();
			zTRnd_ZBufferCmp zCompare = zrenderer->GetZBufferCompare();
			zTRnd_AlphaBlendFunc blend = zrenderer->GetAlphaBlendFunc();
			int bilerp = zrenderer->GetBilerpFilterEnabled();
			zTRnd_ZBufferCmp always = zRND_ZBUFFER_CMP_ALWAYS;
			zTRnd_AlphaBlendFunc alphaBlend = zRND_ALPHA_FUNC_BLEND;
			zrenderer->SetZBufferWriteEnabled(FALSE);
			zrenderer->SetZBufferCompare(always);
			zrenderer->SetAlphaBlendFunc(alphaBlend);
			zrenderer->SetBilerpFilterEnabled(TRUE);
			DrawXfer(hero);
			zrenderer->SetZBufferWriteEnabled(zWrite);
			zrenderer->SetZBufferCompare(zCompare);
			zrenderer->SetAlphaBlendFunc(blend);
			zrenderer->SetBilerpFilterEnabled(bilerp);
		}

		static void BridgeFrame(oCNpc* hero);
		static bool rightDragged = false;
		static float rightDownX = 0.0f, rightDownY = 0.0f;

		// Each frame the screen is drawn while he is played as a Sim: input first (the UI, the pie
		// menu, then the world), then the UI.
		static void Frame()
		{
			oCNpc* hero = oCNpc::player;
			frameCount++;
			screenW = zrenderer->vid_xdim;
			screenH = zrenderer->vid_ydim;
			ui = screenH / uiRef * settings.uiScale;
			LoadUi();
			ReadInput();
			if (!Active(hero) || !ogame->GetShowPlayerStatus())
			{
				ClosePie();
				xferMode = xLastMode = XFER_NONE;
				return;
			}
			if (settings.bridge)
				BridgeFrame(hero);
			bool dialog = InDialog(), items = ContainerOpen(hero);
			if (!dialog && !items) // The Sims' time: Space pauses and goes on, 1, 2, 3 its speeds
			{
				if (keySpace.pressed)
					SetSpeed(speedSetting == 0 ? lastSpeed : 0);
				for (int i = 0; i < 3; i++)
					if (keySpeed[i].pressed)
						SetSpeed(i + 1);
			}
			if (gothicPaused) // Gothic's camera AI rests while it is paused; The Sims' camera doesn't
				UpdateCamera(zCAICamera::current);
			bool screens = dialog || items;
			CollectThings(hero);
			xferMode = DetectXfer(hero); // (trading is a dialogue's mode)
			if (xferMode == XFER_NONE)
				xLastMode = XFER_NONE;
			LayoutButtons(hero);
			const Button* button = dialog && xferMode == XFER_NONE ? nullptr : ButtonAt();
			hoveredButton = button ? button->id : nullptr;
			if (xferMode != XFER_NONE) // The Sims' item screen over Gothic's
			{
				ClosePie();
				XferInput(hero);
				return;
			}
			if (dialog)
			{
				ClosePie();
				DialogMouse();
				return;
			}
			if (items)
			{
				ClosePie();
				ContainerMouse(hero);
				if (mouseLeft.pressed && button)
					PressButton(hero, button->id);
				return;
			}
			bool overUi = button || (pieOpen && HoveredOption() >= 0) || hoveredQueue >= 0;
			hover = Pick();
			if (cameraActive && !screens && !overUi && !pieOpen)
				hover = PickAt(hero, pointerX, pointerY);
			if (mouseRight.pressed)
			{
				rightDownX = pointerX;
				rightDownY = pointerY;
				rightDragged = false;
			}
			if (mouseRight.held && (fabsf(mouseDX) > 2.0f || fabsf(mouseDY) > 2.0f))
				rightDragged = true;

			if (!screens)
			{
				if (mouseLeft.pressed)
				{
					if (pieOpen)
					{
						int i = HoveredOption();
						if (i >= 0)
							ChooseOption(hero, i);
						else
							ClosePie();
					}
					else if (button)
						PressButton(hero, button->id);
					else if (hoveredQueue == 1000)
						StopFight(hero, "cancelled");
					else if (hoveredQueue >= 0 && hoveredQueue < (int)queue.size())
					{
						if (hoveredQueue == 0)
							CancelCurrent(hero);
						else
						{
							EndAction(hero, queue[hoveredQueue]);
							queue.erase(queue.begin() + hoveredQueue);
						}
					}
					else if (hover.vob)
						OpenPie(hero, hover, false);
					else if (hover.ground) // "Go here" at once
					{
						Action a;
						a.act = A_GO;
						a.pos = hover.point;
						a.walk = keyCtrl.held ? WALK_WALK : WALK_RUN;
						a.label = keyCtrl.held ? "walk_here" : "go_here";
						a.icon = keyCtrl.held ? "go" : "run";
						Enqueue(hero, a, keyShift.held);
					}
				}
				if (mouseRight.released && !rightDragged)
				{
					if (pieOpen)
						ClosePie();
					else if (hover.ground || hover.vob)
					{
						Pick p = PickAt(hero, pointerX, pointerY);
						OpenPie(hero, p, !p.vob);
					}
				}
			}
			if (pieOpen && (!pieTarget || !Alive(pieTarget)) && !pie.empty() && pie[0].action.act != A_GO && pie[0].action.target)
				ClosePie();
		}

		static void DrawUi()
		{
			oCNpc* hero = oCNpc::player;
			if (!uiReady || !Active(hero) || !ogame->GetShowPlayerStatus())
				return;
			int zWrite = zrenderer->GetZBufferWriteEnabled();
			zTRnd_ZBufferCmp zCompare = zrenderer->GetZBufferCompare();
			zTRnd_AlphaBlendFunc blend = zrenderer->GetAlphaBlendFunc();
			int bilerp = zrenderer->GetBilerpFilterEnabled(), wrap = zrenderer->GetTextureWrapEnabled(), fog = zrenderer->GetFog();
			zTRnd_ZBufferCmp always = zRND_ZBUFFER_CMP_ALWAYS;
			zTRnd_AlphaBlendFunc alphaBlend = zRND_ALPHA_FUNC_BLEND;
			zrenderer->SetViewport(0, 0, screenW, screenH);
			zrenderer->SetZBufferWriteEnabled(FALSE);
			zrenderer->SetZBufferCompare(always);
			zrenderer->SetAlphaBlendFunc(alphaBlend);
			zrenderer->SetTextureWrapEnabled(FALSE);
			zrenderer->SetFog(FALSE);
			zrenderer->SetBilerpFilterEnabled(TRUE);
			bool screens = InDialog() || ContainerOpen(hero);
			if (!screens && cameraActive)
			{
				DrawThings(hero);
				DrawHero(hero);
			}
			if (!InDialog())
			{
				if (!screens)
				{
					DrawPanel(hero);
					DrawQueue();
				}
				if (xferMode == XFER_NONE) // (The Sims' item screen draws its own, over Gothic's)
					DrawButtons();
				DrawPie();
			}
			zrenderer->SetZBufferWriteEnabled(zWrite);
			zrenderer->SetZBufferCompare(zCompare);
			zrenderer->SetAlphaBlendFunc(blend);
			zrenderer->SetBilerpFilterEnabled(bilerp);
			zrenderer->SetTextureWrapEnabled(wrap);
			zrenderer->SetFog(fog);
		}

		// with Gothic's own controls (F8): which key brings The Sims' back, top middle
		static void DrawGothicControlsHint()
		{
			oCNpc* hero = oCNpc::player;
			if (!uiReady || !settings.controls || simsMode || !hero || !ogame || !ogame->GetShowPlayerStatus() || InDialog())
				return;
			zTRnd_AlphaBlendFunc blend = zrenderer->GetAlphaBlendFunc();
			zTRnd_AlphaBlendFunc alphaBlend = zRND_ALPHA_FUNC_BLEND;
			zrenderer->SetAlphaBlendFunc(alphaBlend);
			char text[96];
			sprintf_s(text, "F8: %s", Label("sims_controls"));
			DrawTag(text, screenW / 2.0f, U(14.0f), 20.0f, 210);
			zrenderer->SetAlphaBlendFunc(blend);
		}

		// the pointer over everything, Gothic's screens too
		static void DrawPointerOnTop()
		{
			oCNpc* hero = oCNpc::player;
			if (!uiReady || !Active(hero) || !ogame->GetShowPlayerStatus())
				return;
			int zWrite = zrenderer->GetZBufferWriteEnabled();
			zTRnd_ZBufferCmp zCompare = zrenderer->GetZBufferCompare();
			zTRnd_AlphaBlendFunc blend = zrenderer->GetAlphaBlendFunc();
			zTRnd_ZBufferCmp always = zRND_ZBUFFER_CMP_ALWAYS;
			zTRnd_AlphaBlendFunc alphaBlend = zRND_ALPHA_FUNC_BLEND;
			zrenderer->SetZBufferWriteEnabled(FALSE);
			zrenderer->SetZBufferCompare(always);
			zrenderer->SetAlphaBlendFunc(alphaBlend);
			DrawPointer();
			zrenderer->SetZBufferWriteEnabled(zWrite);
			zrenderer->SetZBufferCompare(zCompare);
			zrenderer->SetAlphaBlendFunc(blend);
		}

		// before a world goes (a load, another level): the plugin's references let go
		static void ForgetWorld()
		{
			oCNpc* hero = oCNpc::player;
			StopWalk();
			UnfadeAll();
			RestoreNearClip();
			cutCamera = nullptr;
			for (Action& a : queue)
				if (a.target)
				{
					a.target->Release();
					a.target = nullptr;
				}
			queue.clear();
			if (fightTarget)
			{
				fightTarget->Release();
				fightTarget = nullptr;
			}
			savedTactic = -1;
			ClosePie();
			ReleaseThings();
			thingsTime = -1.0f;
			hover = Pick();
			forcedFocus = nullptr;
			selected = nullptr;
			lastAttacker = nullptr;
			if (fastModel && hero && hero->GetModel() == fastModel)
				fastModel->timeScale = 1.0f;
			fastModel = nullptr;
			cameraActive = false;
			goMarkerOn = false;
			if (gothicPaused && ogame)
				ogame->Unpause();
			gothicPaused = false;
			speedSetting = 1;
			if (ztimer)
				ztimer->SetMotionFactor(1.0f);
			(void)hero;
		}

		// --- the test bridge (bridge=1): Sims.cmd in the Gothic folder, one command a line, read and
		// deleted every 0.3 s; Sims.state written after each. Commands: pointer <x> <y> (0..1 of the
		// screen), click [left|right], pie <n>, button <id>, go <x> <y> <z>, use <vob name> (the pie's
		// first option on it), npc <instance> <option n>, beam <waypoint>, auto <0|1>, state.
		static float bridgeTime = 0.0f;
		static bool bridgeClick = false, bridgeRight = false;

		static zCVob* FindNamed(oCNpc* hero, const char* name)
		{
			zCWorld* world = hero->GetHomeWorld();
			if (!world)
				return nullptr;
			zSTRING s(name);
			s.Upper();
			if (zCVob* vob = world->SearchVobByName(s))
				return vob;
			oCWorld* game = ogame->GetGameWorld();
			for (zCListSort<oCNpc>* node = game && game->voblist_npcs ? game->voblist_npcs->next : nullptr; node; node = node->next)
				if (node->data && node->data->GetInstanceName() == s)
					return node->data;
			return nullptr;
		}

		static void WriteState(oCNpc* hero)
		{
			FILE* f = nullptr;
			if (fopen_s(&f, "Sims.state", "w") || !f)
				return;
			zVEC3 p = hero->GetPositionWorld();
			fprintf(f, "hero %.0f %.0f %.0f body %d weapon %d hp %d/%d mob %s dialog %d container %d\n", p[0], p[1], p[2], BodyState(hero),
				hero->GetWeaponMode(), hero->attribute[NPC_ATR_HITPOINTS], hero->attribute[NPC_ATR_HITPOINTSMAX],
				hero->GetInteractMob() ? hero->GetInteractMob()->GetObjectName().ToChar() : "-", InDialog() ? 1 : 0, ContainerOpen(hero) ? 1 : 0);
			fprintf(f, "camera %d focus %.0f %.0f %.0f yaw %.2f pitch %.2f distance %.0f follow %d\n", cameraActive ? 1 : 0, camFocus[0],
				camFocus[1], camFocus[2], camYaw, camPitch, camDistance, follow ? 1 : 0);
			fprintf(f, "pointer %.0f %.0f of %d %d\n", pointerX, pointerY, screenW, screenH);
			char name[96] = "";
			if (hover.vob)
				NameOf(hover.vob, name, sizeof name);
			fprintf(f, "hover %s %s ground %d %.0f %.0f %.0f\n", hover.vob ? hover.vob->GetObjectName().ToChar() : "-", name, hover.ground ? 1 : 0,
				hover.point[0], hover.point[1], hover.point[2]);
			fprintf(f, "pie %d %s\n", pieOpen ? 1 : 0, pieTitle);
			for (int i = 0; i < (int)pie.size(); i++)
				fprintf(f, "  option %d %s\n", i, pie[i].text);
			fprintf(f, "queue %d\n", (int)queue.size());
			for (const Action& a : queue)
				fprintf(f, "  action %s phase %d tries %d\n", a.label, a.phase, a.tries);
			fprintf(f, "fight %s auto %d\n", fightTarget ? fightTarget->GetInstanceName().ToChar() : "-", autoFight ? 1 : 0);
			int n = 0;
			for (zCVob* vob : things)
				if (Alive(vob) && n < 60)
				{
					zVEC3 q = vob->GetPositionWorld();
					char nm[96];
					NameOf(vob, nm, sizeof nm);
					const char* label;
					const char* icon;
					DefaultVerb(hero, vob, label, icon);
					float sx = -1.0f, sy = -1.0f;
					Project(q, sx, sy);
					fprintf(f, "thing %s \"%s\" %s at %.0f %.0f %.0f screen %.0f %.0f distance %.0f\n", vob->GetObjectName().ToChar(), nm, label,
						q[0], q[1], q[2], sx, sy, (q - hero->GetPositionWorld()).Length());
					n++;
				}
			fclose(f);
		}

		static void BridgeFrame(oCNpc* hero)
		{
			if (bridgeClick)
			{
				// a click given by the bridge last frame: as if the button went down
				bridgeClick = false;
			}
			float now = RealNow();
			if (now - bridgeTime < 0.3f)
				return;
			bridgeTime = now;
			FILE* f = nullptr;
			if (fopen_s(&f, "Sims.cmd", "r") || !f)
				return;
			char line[256];
			while (fgets(line, sizeof line, f))
			{
				char cmd[32] = "", a1[128] = "";
				float x = 0.0f, y = 0.0f, z = 0.0f;
				if (sscanf_s(line, "%31s", cmd, (unsigned)sizeof cmd) != 1)
					continue;
				Log("bridge: %s", line);
				if (!strcmp(cmd, "pointer") && sscanf_s(line, "%*s %f %f", &x, &y) == 2)
				{
					pointerX = x * screenW;
					pointerY = y * screenH;
				}
				else if (!strcmp(cmd, "click"))
				{
					sscanf_s(line, "%*s %127s", a1, (unsigned)sizeof a1);
					Pick p = PickAt(hero, pointerX, pointerY);
					hover = p;
					if (!strcmp(a1, "right"))
						OpenPie(hero, p, !p.vob);
					else if (pieOpen)
					{
						int i = HoveredOption();
						if (i >= 0)
							ChooseOption(hero, i);
						else
							ClosePie();
					}
					else if (p.vob)
						OpenPie(hero, p, false);
					else if (p.ground)
					{
						Action a;
						a.act = A_GO;
						a.pos = p.point;
						Enqueue(hero, a, false);
					}
				}
				else if (!strcmp(cmd, "pie") && sscanf_s(line, "%*s %f", &x) == 1)
					ChooseOption(hero, (int)x);
				else if (!strcmp(cmd, "button") && sscanf_s(line, "%*s %127s", a1, (unsigned)sizeof a1) == 1)
					PressButton(hero, a1);
				else if (!strcmp(cmd, "go") && sscanf_s(line, "%*s %f %f %f", &x, &y, &z) == 3)
				{
					Action a;
					a.act = A_GO;
					a.pos = zVEC3(x, y, z);
					Enqueue(hero, a, false);
				}
				else if ((!strcmp(cmd, "use") || !strcmp(cmd, "npc")) && sscanf_s(line, "%*s %127s", a1, (unsigned)sizeof a1) == 1)
				{
					int option = 0;
					sscanf_s(line, "%*s %*s %d", &option);
					zCVob* vob = FindNamed(hero, a1);
					if (!vob)
						Log("bridge: %s not found", a1);
					else
					{
						Pick p;
						p.vob = vob;
						OpenPie(hero, p, false);
						if (pieOpen)
							ChooseOption(hero, option);
					}
				}
				else if (!strcmp(cmd, "beam") && sscanf_s(line, "%*s %127s", a1, (unsigned)sizeof a1) == 1)
				{
					hero->BeamTo(zSTRING(a1));
					CenterOnHero(hero);
				}
				else if (!strcmp(cmd, "auto") && sscanf_s(line, "%*s %f", &x) == 1)
					autoFight = x != 0.0f;
				else if (!strcmp(cmd, "cam") && sscanf_s(line, "%*s %f %f %f", &x, &y, &z) == 3)
				{
					camYaw = x;
					camPitch = y;
					wantDistance = z;
				}
			}
			fclose(f);
			DeleteFileA("Sims.cmd");
			WriteState(hero);
		}

		// F8: Gothic's own controls and back
		static void ToggleControls()
		{
			simsMode = !simsMode;
			ClosePie();
			Log("controls: %s", simsMode ? "The Sims'" : "Gothic's");
		}
	}

	// --- hooks --------------------------------------------------------------------------------------------

	// The hero's AI each frame: his actions and fights, then Gothic's own with the keys answered by
	// the plugin (Sims::OverrideKey).
	void __fastcall Sims_oCAIHuman_DoAI(oCAIHuman* self, void* vtable, zCVob* vob, int& vobDeleted);
	auto Hook_Sims_oCAIHuman_DoAI = Union::CreateHook(SIGNATURE_OF(&oCAIHuman::DoAI), &Sims_oCAIHuman_DoAI, Union::HookType::Hook_Detours);
	void __fastcall Sims_oCAIHuman_DoAI(oCAIHuman* self, void* vtable, zCVob* vob, int& vobDeleted)
	{
		oCNpc* hero = oCNpc::player;
		if (hero && self == hero->human_ai)
		{
			Sims::LoadSettings();
			Sims::InstallCrashLog();
			static bool started = false;
			if (!started)
			{
				started = true;
				Sims::Log("Sims plugin (%s)", Sims::GOTHIC2 ? "Gothic 2" : "Gothic 1");
			}
			if (Sims::Active(hero))
			{
				Sims::step = "actions";
				Sims::UpdateActions(hero);
				Sims::step = "route";
				Sims::UpdateWalk(hero);
				Sims::UpdateFastAnims(hero);
				Sims::step = "fight";
				Sims::UpdateFight(hero);
				Sims::step = "Gothic's AI (hero)";
				Sims::overrideInput = true;
				Hook_Sims_oCAIHuman_DoAI(self, vtable, vob, vobDeleted);
				Sims::overrideInput = false;
				Sims::actionKeyEdge = false;
				for (Sims::HeldKey& h : Sims::held)
					h.edge = false;
				Sims::step = "Gothic's frame";
				return;
			}
		}
		Hook_Sims_oCAIHuman_DoAI(self, vtable, vob, vobDeleted);
	}

	float __fastcall Sims_zCInput_Win32_GetState(zCInput_Win32* self, void* vtable, unsigned short key);
	auto Hook_Sims_zCInput_Win32_GetState = Union::CreateHook(SIGNATURE_OF(&zCInput_Win32::GetState), &Sims_zCInput_Win32_GetState, Union::HookType::Hook_Detours);
	float __fastcall Sims_zCInput_Win32_GetState(zCInput_Win32* self, void* vtable, unsigned short key)
	{
		float state;
		int toggled;
		return Sims::OverrideKey(key, state, toggled) ? state : Hook_Sims_zCInput_Win32_GetState(self, vtable, key);
	}

	int __fastcall Sims_zCInput_Win32_GetToggled(zCInput_Win32* self, void* vtable, unsigned short key);
	auto Hook_Sims_zCInput_Win32_GetToggled = Union::CreateHook(SIGNATURE_OF(&zCInput_Win32::GetToggled), &Sims_zCInput_Win32_GetToggled, Union::HookType::Hook_Detours);
	int __fastcall Sims_zCInput_Win32_GetToggled(zCInput_Win32* self, void* vtable, unsigned short key)
	{
		float state;
		int toggled;
		return Sims::OverrideKey(key, state, toggled) ? toggled : Hook_Sims_zCInput_Win32_GetToggled(self, vtable, key);
	}

	// Gothic turns the hero with the mouse and its turn keys here; as a Sim the mouse is the pointer's.
	void __fastcall Sims_oCAIHuman_PC_Turnings(oCAIHuman* self, void* vtable, int force);
	auto Hook_Sims_oCAIHuman_PC_Turnings = Union::CreateHook(SIGNATURE_OF(&oCAIHuman::PC_Turnings), &Sims_oCAIHuman_PC_Turnings, Union::HookType::Hook_Detours);
	void __fastcall Sims_oCAIHuman_PC_Turnings(oCAIHuman* self, void* vtable, int force)
	{
		oCNpc* hero = oCNpc::player;
		if (hero && self == hero->human_ai && Sims::Active(hero) && !hero->GetInteractMob())
			return;
		Hook_Sims_oCAIHuman_PC_Turnings(self, vtable, force);
	}

	void __fastcall Sims_zCAICamera_DoAI(zCAICamera* self, void* vtable, zCVob* vob, int& vobDeleted);
	auto Hook_Sims_zCAICamera_DoAI = Union::CreateHook(SIGNATURE_OF(&zCAICamera::DoAI), &Sims_zCAICamera_DoAI, Union::HookType::Hook_Detours);
	void __fastcall Sims_zCAICamera_DoAI(zCAICamera* self, void* vtable, zCVob* vob, int& vobDeleted)
	{
		Hook_Sims_zCAICamera_DoAI(self, vtable, vob, vobDeleted);
		Sims::LoadSettings();
		Sims::step = "camera";
		Sims::UpdateCamera(self);
		Sims::step = "Gothic's frame";
	}

	// Gothic's focus while an action gives him Gothic's action key: the action's target.
#if ENGINE == Engine_G2A
	void __fastcall Sims_oCNpc_CollectFocusVob(oCNpc* self, void* vtable, int force);
	auto Hook_Sims_oCNpc_CollectFocusVob = Union::CreateHook(SIGNATURE_OF(&oCNpc::CollectFocusVob), &Sims_oCNpc_CollectFocusVob, Union::HookType::Hook_Detours);
	void __fastcall Sims_oCNpc_CollectFocusVob(oCNpc* self, void* vtable, int force)
	{
		Hook_Sims_oCNpc_CollectFocusVob(self, vtable, force);
		if (self == oCNpc::player && Sims::forcedFocus && Sims::Alive(Sims::forcedFocus))
			self->SetFocusVob(Sims::forcedFocus);
	}
#else
	void __fastcall Sims_oCNpc_CollectFocusVob(oCNpc* self, void* vtable);
	auto Hook_Sims_oCNpc_CollectFocusVob = Union::CreateHook(SIGNATURE_OF(&oCNpc::CollectFocusVob), &Sims_oCNpc_CollectFocusVob, Union::HookType::Hook_Detours);
	void __fastcall Sims_oCNpc_CollectFocusVob(oCNpc* self, void* vtable)
	{
		Hook_Sims_oCNpc_CollectFocusVob(self, vtable);
		if (self == oCNpc::player && Sims::forcedFocus && Sims::Alive(Sims::forcedFocus))
			self->SetFocusVob(Sims::forcedFocus);
	}
#endif

	// The UI each frame the screen is drawn: under Gothic's own views (its inventory, dialogues), the
	// pointer over them. The renderer's alpha from the textures (the Minecraft mashup's note: Gothic 2's
	// camera can leave it constant).
	void __fastcall Sims_zCView_DrawItems(zCView* self, void* vtable);
	auto Hook_Sims_zCView_DrawItems = Union::CreateHook(SIGNATURE_OF(&zCView::DrawItems), &Sims_zCView_DrawItems, Union::HookType::Hook_Detours);
	void __fastcall Sims_zCView_DrawItems(zCView* self, void* vtable)
	{
		if (self != screen || !zrenderer || !ogame)
		{
			Hook_Sims_zCView_DrawItems(self, vtable);
			return;
		}
		Sims::LoadSettings();
		zTRnd_AlphaBlendSource source = zrenderer->GetAlphaBlendSource(), material = zRND_ALPHA_SOURCE_MATERIAL;
		float factor = zrenderer->GetAlphaBlendFactor(), one = 1.0f;
		zrenderer->SetAlphaBlendSource(material);
		zrenderer->SetAlphaBlendFactor(one);
		Sims::step = "frame";
		Sims::Frame();
		Sims::step = "ui";
		Sims::DrawUi();
		Sims::step = "Gothic's drawing";
		Hook_Sims_zCView_DrawItems(self, vtable);
		zrenderer->SetAlphaBlendSource(material);
		zrenderer->SetAlphaBlendFactor(one);
		Sims::step = "item screen";
		Sims::DrawXferOnTop();
		Sims::step = "pointer";
		Sims::DrawPointerOnTop();
		Sims::DrawGothicControlsHint();
		zrenderer->SetAlphaBlendSource(source);
		zrenderer->SetAlphaBlendFactor(factor);
		Sims::step = "Gothic's frame";
	}

	// Keys: F8 switches the controls; Esc closes the pie menu; the camera's keys aren't Gothic's
	// while he is played as a Sim (no screen of Gothic's open).
	int __fastcall Sims_oCGame_HandleEvent(oCGame* self, void* vtable, int key);
	auto Hook_Sims_oCGame_HandleEvent = Union::CreateHook(SIGNATURE_OF(&oCGame::HandleEvent), &Sims_oCGame_HandleEvent, Union::HookType::Hook_Detours);
	int __fastcall Sims_oCGame_HandleEvent(oCGame* self, void* vtable, int key)
	{
		Sims::LoadSettings();
		oCNpc* hero = oCNpc::player;
		if (key == KEY_F8 && Sims::settings.controls)
		{
			Sims::ToggleControls();
			return TRUE;
		}
		if (Sims::Active(hero))
		{
			if (key == KEY_ESCAPE && Sims::pieOpen)
			{
				Sims::ClosePie();
				return TRUE;
			}
			bool screens = Sims::InDialog() || Sims::ContainerOpen(hero) || hero->GetInteractMob();
			// The Sims' time keys: Space pauses and goes on, 1, 2 and 3 its speeds (Gothic's weapon keys
			// there are the HUD's buttons while he is played as a Sim; 4 to 0 stay Gothic's spells)
			if (!Sims::InDialog() && !Sims::ContainerOpen(hero))
				switch (key)
				{
				case KEY_SPACE: case KEY_1: case KEY_2: case KEY_3:
					return TRUE; // (read in Sims::Frame: Gothic may not pass keys on while it is paused)
				default:
					break;
				}
			if (!screens)
				switch (key)
				{
				case KEY_W: case KEY_A: case KEY_S: case KEY_D: case KEY_Q: case KEY_E: case KEY_Z: case KEY_X: case KEY_HOME:
				case KEY_UPARROW: case KEY_DOWNARROW: case KEY_LEFTARROW: case KEY_RIGHTARROW:
					return TRUE;
				default:
					break;
				}
		}
		return Hook_Sims_oCGame_HandleEvent(self, vtable, key);
	}

	namespace Sims
	{
		// one of Gothic's logical keys pressed (the first key bound to it), as the player would
		static void PressGothicKey(int logical)
		{
			if (!zinput || !ogame)
				return;
			zCArray<unsigned short> keys;
			zinput->GetBinding((unsigned short)logical, keys);
			if (keys.GetNum() == 0)
			{
				Log("key %d: nothing bound to it", logical);
				return;
			}
			Hook_Sims_oCGame_HandleEvent(ogame, nullptr, keys[0]);
		}

		// a key of the keyboard pressed for Gothic (a spell's number key)
		static void PressRawKey(int key)
		{
			if (ogame)
				Hook_Sims_oCGame_HandleEvent(ogame, nullptr, key);
		}
	}

	// Gothic's health, mana and focus bars give way to the hero's panel.
	void __fastcall Sims_oCGame_UpdatePlayerStatus(oCGame* self, void* vtable);
	auto Hook_Sims_oCGame_UpdatePlayerStatus = Union::CreateHook(SIGNATURE_OF(&oCGame::UpdatePlayerStatus), &Sims_oCGame_UpdatePlayerStatus, Union::HookType::Hook_Detours);
	void __fastcall Sims_oCGame_UpdatePlayerStatus(oCGame* self, void* vtable)
	{
		Hook_Sims_oCGame_UpdatePlayerStatus(self, vtable);
		oCNpc* hero = oCNpc::player;
		if (!screen || !Sims::uiReady || !Sims::Active(hero))
			return;
		for (zCView* bar : { (zCView*)self->hpBar, (zCView*)self->manaBar, (zCView*)self->focusBar })
			if (bar)
				screen->RemoveItem(bar);
	}

	// Who hits the hero (auto fight answers them).
	void __fastcall Sims_oCNpc_OnDamage_Hit(oCNpc* self, void* vtable, oCNpc::oSDamageDescriptor& desc);
	auto Hook_Sims_oCNpc_OnDamage_Hit = Union::CreateHook(SIGNATURE_OF(&oCNpc::OnDamage_Hit), &Sims_oCNpc_OnDamage_Hit, Union::HookType::Hook_Detours);
	void __fastcall Sims_oCNpc_OnDamage_Hit(oCNpc* self, void* vtable, oCNpc::oSDamageDescriptor& desc)
	{
		if (self && self == oCNpc::player && desc.pNpcAttacker && desc.pNpcAttacker != self)
		{
			Sims::lastAttacker = desc.pNpcAttacker;
			Sims::lastHitTime = Sims::Now();
		}
		Hook_Sims_oCNpc_OnDamage_Hit(self, vtable, desc);
	}

	// Before any world goes (a load, a new game, another level) the plugin lets go of its references.
	void __fastcall Sims_oCGame_LoadGame(oCGame* self, void* vtable, int slot, zSTRING const& level);
	auto Hook_Sims_oCGame_LoadGame = Union::CreateHook(SIGNATURE_OF(&oCGame::LoadGame), &Sims_oCGame_LoadGame, Union::HookType::Hook_Detours);
	void __fastcall Sims_oCGame_LoadGame(oCGame* self, void* vtable, int slot, zSTRING const& level)
	{
		Sims::ForgetWorld();
		Hook_Sims_oCGame_LoadGame(self, vtable, slot, level);
	}

	void __fastcall Sims_oCGame_LoadSavegame(oCGame* self, void* vtable, int slot, int loadGlobals);
	auto Hook_Sims_oCGame_LoadSavegame = Union::CreateHook(SIGNATURE_OF(&oCGame::LoadSavegame), &Sims_oCGame_LoadSavegame, Union::HookType::Hook_Detours);
	void __fastcall Sims_oCGame_LoadSavegame(oCGame* self, void* vtable, int slot, int loadGlobals)
	{
		Sims::ForgetWorld();
		Hook_Sims_oCGame_LoadSavegame(self, vtable, slot, loadGlobals);
	}

	void __fastcall Sims_oCGame_ChangeLevel(oCGame* self, void* vtable, zSTRING const& world, zSTRING const& start);
	auto Hook_Sims_oCGame_ChangeLevel = Union::CreateHook(SIGNATURE_OF(&oCGame::ChangeLevel), &Sims_oCGame_ChangeLevel, Union::HookType::Hook_Detours);
	void __fastcall Sims_oCGame_ChangeLevel(oCGame* self, void* vtable, zSTRING const& world, zSTRING const& start)
	{
		Sims::ForgetWorld();
		Hook_Sims_oCGame_ChangeLevel(self, vtable, world, start);
	}
}
