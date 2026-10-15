// Minecraft's Steve as the hero of Gothic 1 (GothicMod.exe 1.08k_mod) or Gothic 2 Night of the Raven
// (Gothic2.exe 2.6). make_steve.py gives the hero Steve's blocky body playing Gothic's animations
// (STEVE.MDS); this plugin gives him Minecraft's camera.
//
// Camera (Minecraft's GameRenderer / Camera): first person at his eyes, F5 switches to the camera
// behind him and then in front of him, looking back at him (Minecraft's 4 blocks away), as F5
// does in Minecraft. In first person his body is unseen and the eyes are in the middle of his
// head; Minecraft's hand is drawn instead (see UpdateHand). Being hurt tilts the view
// (GameRenderer.bobHurt: up to 14 degrees for 10 ticks, about the way the blow came from); walking
// bobs it (bobView: sideways, up, a roll of 3 and a nod of 5 degrees). The mouse turns Steve and the view together (Minecraft's look: yaw turns the
// body, pitch tilts the view only, up to 90 degrees); Gothic's own turning is off meanwhile. The
// field of view is Minecraft's (70 degrees vertical by default). In dialogues, while he uses
// something (a bench, a ladder, a chest), and when he is down, Gothic's own camera takes over.
// The eyes are at Minecraft's 1.62 m (1.27 m sneaking), and the view bobs as he walks
// (GameRenderer.bobView); sprinting widens the view by 15 % (AbstractClientPlayer.getFieldOfViewModifier).
//
// Movement (physics=1): Minecraft's, on land, in 20 ticks a second as LivingEntity.travel does it,
// in Minecraft's units (blocks of 1 m, ticks of 50 ms): W/S/A/D push him relative to where he looks
// (the input normalised, times 0.3 while sneaking) by his speed 0.1 a tick (1.3 times sprinting)
// on the ground, 0.02 (0.026) in the air; then he moves by his speed, which loses to friction 0.546
// a tick on the ground (a block's 0.6 times 0.91) and 0.91 in the air. Gravity takes 0.08 a tick
// and the air 2 % of his falling speed. Space jumps with 0.42 up (sprinting also 0.2 forward), again
// at once on landing while held, 10 ticks after the last jump at the earliest (1.25 blocks high); only
// a ledge of Gothic's own world higher than that (1.35 m) that Gothic can climb
// (oCAniCtrl_Human::CanJumpLedge) gets Gothic's climb, never a block. Left Ctrl, or W tapped twice,
// sprints (only forwards, not sneaking; running into a wall ends it); Left Shift sneaks, and
// sneaking keeps him from walking off an edge (Player.maybeBackOffFromEdge). Gothic still does
// the collision: on the ground the move goes in as the model's root motion (zCModel::vobTrans,
// which zCVob::DoFrameActivity applies with Gothic's walls, steps up to 60 cm, Minecraft's 0.6
// block, and floor), in the air as the speed of Gothic's physics body. Gothic's fall check
// (zCAIPlayer::CheckPhysics) is off for him meanwhile: it would end a jump at once, as he leaves
// the floor from 0 cm above it. He falls from an edge higher than a step (60 cm) and lands when
// falling with the floor at his feet, with Minecraft's fall damage (a point of its 20 health for
// each block past 3, as that share of his). Gothic's own movement keys do nothing meanwhile.
// Ladders are Minecraft's (LivingEntity.handleOnClimbable): touching one of Gothic's ladders he
// climbs at 0.2 a tick while he pushes into it (W facing it) or holds Space, slides down no faster
// than 0.15 a tick, holds on sneaking, moves along it no faster than 0.15, and takes no fall damage;
// Gothic's own ladder use is left out.
// Mouse: the left button is Minecraft's attack: the arm swings and whoever the crosshair points at
// within 3 m takes his hit (his strength, plus a drawn melee weapon's damage) times Minecraft's
// cooldown share (attack speed 4 with the fist, 1.6 a one-hander, 1.0 a two-hander). The right
// button uses (Minecraft.startUseItem): on what Gothic has in focus under the crosshair (someone
// to talk to, a body, an item, a chest, a door, a bench) it is Gothic's action, whatever the hand
// holds (a drawn weapon is put away first; Gothic 1 interacts on the action key with a forward
// press, both given for half a second); else what the chosen slot holds: a block goes down, an
// egg hatches, the bow draws; with one of Gothic's slots it is Gothic's action. A bow, crossbow or
// spell of Gothic's shoots on the left button as Gothic does. In water and in Gothic's
// interactions Gothic moves him.
// His pose follows Minecraft's HumanoidModel.setupAnim: legs and arms swing with his speed (cos of
// the walked distance times 0.6662, the legs 1.4 times), the arms hang loose with the idle bob,
// the head looks where the camera looks, sneaking bends him forward (0.5 rad); set on Gothic's
// pose after zCModel::AdvanceAnis, while he stands or moves (Gothic's actions keep their own);
// holding a block or an item of Minecraft's his right arm is half raised (ArmPose.ITEM) and the
// thing turns with the arm alone, drawing the bow both arms point where he looks (BOW_AND_ARROW).
//
// Steve's body: Gothic draws characters as progressive meshes that thin out with distance, which
// would take the corners off his boxes: his soft skins are kept in full. Armour keeps its
// protection but not its look (his body stays Steve's): the armour's visual_change is skipped
// while it goes on. The hero's overlays (weapon skills, the sprint potion, acrobatics, the torch)
// are Gothic's HUMANS_*.MDS, made for Gothic's skeleton; on Steve their STEVE_*.MDS versions
// (make_steve.py) go on in their place.
//
// Compile run: the installer starts the game once with -mccompile and -zautoconvertdata. On the
// menu's first frame Steve's model, his body and every overlay are loaded, so Gothic compiles them,
// and the game ends. The players' own starts then go without -zautoconvertdata, with which Gothic
// tries to rebuild some of its own monsters from sources it doesn't ship and stops where a
// skeleton appears.
//
// Settings, read from Gothic.ini section [MINECRAFT] at start (defaults in brackets):
//   controls=1        Minecraft's camera (0: Gothic's own)
//   physics=1         Minecraft's movement (0: Gothic's)
//   firstPerson=1     start in first person (0: behind him)
//   mouseSpeed=0.25   camera degrees per mouse unit; invertMouseY=0
//   fov=70            vertical field of view (degrees)
//   hud=1             Minecraft's HUD in place of Gothic's bars; guiScale=0 its scale (0: Minecraft's
//                     automatic one), vignette=1 the darkening towards the screen's edges
//   hunger=1          Minecraft's hunger (the food bar, healing from a full one, starving)
//   mobs=1            Gothic's zombies and skeletons look like Minecraft's
//   debugLog=0        1: write Minecraft.log in the Gothic folder

#include <algorithm>
#include <climits>
#include <unordered_map>
#include <vector>

namespace GOTHIC_NAMESPACE
{
	namespace Minecraft
	{
		// This file is compiled once per engine: Gothic 1 and Gothic 2 Night of the Raven.
		static constexpr bool GOTHIC2 = ENGINE == Engine_G2A;

		struct Settings
		{
			bool controls = true;
			bool physics = true;
			bool firstPerson = true;
			float mouseSpeed = 0.25f;
			bool invertMouseY = false;
			float fov = 70.0f;
			bool debugLog = false;
			bool hud = true, hunger = true, vignette = true, mobs = true;
			int guiScale = 0;
		};

		static Settings settings;
		static bool settingsLoaded = false;
		static const float PI = 3.14159265f;
		static const float PIXEL = 0.9375f * 100.0f / 16.0f; // cm a skin pixel (make_steve.py)

		static void LoadSettings()
		{
			if (settingsLoaded || !zoptions)
				return;
			settingsLoaded = true;
			zSTRING s = "MINECRAFT";
			settings.controls = zoptions->ReadBool(s, "controls", 1) != 0;
			settings.physics = zoptions->ReadBool(s, "physics", 1) != 0;
			settings.firstPerson = zoptions->ReadBool(s, "firstPerson", 1) != 0;
			settings.mouseSpeed = zoptions->ReadReal(s, "mouseSpeed", settings.mouseSpeed);
			settings.invertMouseY = zoptions->ReadBool(s, "invertMouseY", 0) != 0;
			settings.fov = zoptions->ReadReal(s, "fov", settings.fov);
			settings.debugLog = zoptions->ReadBool(s, "debugLog", 0) != 0;
			settings.hud = zoptions->ReadBool(s, "hud", 1) != 0;
			settings.hunger = zoptions->ReadBool(s, "hunger", 1) != 0;
			settings.vignette = zoptions->ReadBool(s, "vignette", 1) != 0;
			settings.mobs = zoptions->ReadBool(s, "mobs", 1) != 0;
			settings.guiScale = zoptions->ReadInt(s, "guiScale", 0);
		}

		static void Log(const char* format, ...)
		{
			if (!settings.debugLog)
				return;
			FILE* file = nullptr;
			if (fopen_s(&file, "Minecraft.log", "a") || !file)
				return;
			va_list args;
			va_start(args, format);
			fprintf(file, "%8.2f  ", ztimer ? ztimer->totalTimeFloat / 1000.0f : 0.0f);
			vfprintf(file, format, args);
			fputc('\n', file);
			va_end(args);
			fclose(file);
		}

		// Crash diagnostics: the plugin's step, and every serious exception (an access violation and
		// the like) with where it happened and the plugin's return addresses on the stack, logged before
		// Gothic's or Union's handlers end the game.
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
				DWORD size = 0;
				IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + ((IMAGE_DOS_HEADER*)base)->e_lfanew);
				size = nt->OptionalHeader.SizeOfImage;
				for (int i = 0; i < 2048; i++)
				{
					DWORD value = 0;
					if (IsBadReadPtr(stack + i, 4))
						break;
					value = stack[i];
					if (value > (DWORD)base && value < (DWORD)base + size)
						Log("  plugin return address on the stack: Minecraft.dll+%X", value - (DWORD)base);
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

		static zVEC3 Flat(const zVEC3& v)
		{
			zVEC3 out(v[0], 0.0f, v[2]);
			float len = out.Length();
			return len > 0.0001f ? out * (1.0f / len) : zVEC3(0.0f, 0.0f, 1.0f);
		}

		// Steve's model: make_steve.py's STEVE.MDS as the base of the model.
		static bool IsSteve(zCModel* model)
		{
			if (!model || model->modelProtoList.GetNum() == 0 || !model->modelProtoList[0])
				return false;
			zSTRING name = model->modelProtoList[0]->modelProtoName;
			name.Upper();
			return name == "STEVE";
		}

		static bool IsSteve(oCNpc* npc)
		{
			return npc && npc->GetVisual() && IsSteve(npc->GetModel());
		}

		// --- Steve's body ----------------------------------------------------------------------------
		static void FullDetail(oCNpc* hero)
		{
			static zCModel* done = nullptr;
			zCModel* model = hero->GetModel();
			if (!model || model == done || model->meshSoftSkinList.GetNum() == 0)
				return;
			done = model;
			for (int i = 0; i < model->meshSoftSkinList.GetNum(); i++)
				if (zCMeshSoftSkin* skin = model->meshSoftSkinList[i])
				{
					skin->lodParams.zDisplace2 = 1e12f;
					skin->lodParams.minVerts = 1 << 20;
				}
			Log("full detail for %d soft skin(s)", model->meshSoftSkinList.GetNum());
		}

		// Gothic thins a compiled mesh out with distance (its progressive mesh's LOD), which takes the
		// corners off Minecraft's boxes and lets small ones vanish: every mesh of the plugin's in full.
		static void KeepDetail(zCVisual* visual)
		{
			if (zCProgMeshProto* mesh = zDYNAMIC_CAST<zCProgMeshProto>(visual))
			{
				mesh->lodParams.zDisplace2 = 1e12f;
				mesh->lodParams.minVerts = 1 << 20;
			}
		}

		// HUMANS_<x> -> STEVE_<x> (the same ending, .MDS or none) on Steve's model; false for other
		// models and names. `known` is false where make_steve.py made none (another character's).
		static bool SteveOverlay(zCModel* model, const zSTRING& name, zSTRING& steve, bool& known)
		{
			if (!IsSteve(model))
				return false;
			char upper[MAX_PATH];
			strncpy_s(upper, name.ToChar(), _TRUNCATE);
			_strupr_s(upper);
			if (strncmp(upper, "HUMANS_", 7) != 0)
				return false;
			char mapped[MAX_PATH], base[MAX_PATH], path[MAX_PATH], compiled[MAX_PATH];
			sprintf_s(mapped, "STEVE_%s", upper + 7);
			steve = zSTRING(mapped);
			strncpy_s(base, upper + 7, _TRUNCATE);
			if (char* dot = strchr(base, '.'))
				*dot = 0;
			// Gothic runs in its own folder (not system)
			sprintf_s(path, "_work\\DATA\\ANIMS\\STEVE_%s.MDS", base);
			sprintf_s(compiled, "_work\\DATA\\ANIMS\\_COMPILED\\STEVE_%s.MDH", base);
			known = GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES || GetFileAttributesA(compiled) != INVALID_FILE_ATTRIBUTES;
			return true;
		}

		static float Now() { return ztimer ? ztimer->totalTimeFloat / 1000.0f : 0.0f; }

		// --- input -------------------------------------------------------------------------------
		static bool GameHasFocus()
		{
			DWORD process = 0;
			HWND window = GetForegroundWindow();
			return window && GetWindowThreadProcessId(window, &process) && process == GetCurrentProcessId();
		}

		static bool KeyDown(int key) { return zinput && zinput->KeyPressed(key) != 0; }

		// Gothic's own button query can stay 0 while it holds the mouse (the Witcher mashup)
		static bool MouseDown(bool left) { return (GetAsyncKeyState(left ? VK_LBUTTON : VK_RBUTTON) & 0x8000) && GameHasFocus(); }

		struct Key
		{
			bool held = false, pressed = false;
			void Update(bool down)
			{
				pressed = down && !held;
				held = down;
			}
		};

		static Key keyForward, keyBack, keyLeft, keyRight, keyJump, keySneak, keySprint, keyAttack, keyUse;

		// --- Minecraft's movement (see the header) -------------------------------------------------
		static float yaw = 0.0f, pitch = 0.0f; // radians; yaw 0 looks along +z, pitch > 0 down
		static bool physicsOn = false;      // Minecraft's movement drives him this frame
		static bool overrideInput = false;  // inside his DoAI: Gothic's movement keys answered here
		static bool ledgeJump = false;      // Gothic's jump climbs a ledge: its movement until he stands
		static bool askLedge = false;       // Gothic gets its jump key this frame
		static float ledgeTime = 0.0f;
		static bool onLadder = false, ladderAhead = false; // touching one of Gothic's ladders, facing it
		static int useTicks = -1;           // ticks the bow is drawn (LivingEntity.getTicksUsingItem), -1: not drawn
		static float useUntil = -1.0f;      // Gothic's interaction: its action and forward keys until then
		static float putAwayUntil = -1.0f;  // putting a weapon away first, for the interaction
		static bool useEdge = false;        // the interaction's first frame: the action key's press
		static bool onGround = true, sprinting = false, sneaking = false, doubleTap = false, jumped = false;
		static bool airborne = false;       // in the air by Minecraft's movement: Gothic's fall check is off
		static float fallDistance = 0.0f;   // blocks fallen (Entity.fallDistance)
		static int swingTicks = -1;         // ticks into an attack's arm swing (6 a swing), -1: none
		static float attackAnim = 0.0f, attackAnimOld = 0.0f;
		static int attackStrength = 100;    // ticks since the last attack (Player.attackStrengthTicker)
		static int foodLevel = 20;          // Minecraft's hunger (FoodData): 20 full, sprinting needs more than 6
		static float saturation = 5.0f, exhaustion = 0.0f;
		static int wheelSteps = 0;          // mouse wheel steps for the hotbar (the HUD)
		static bool inventoryOpen = false;  // the creative inventory (E) is open
		static float cursorX = 0.0f, cursorY = 0.0f; // its pointer, in GUI pixels
		enum { EGG_ZOMBIE, EGG_SKELETON, EGG_CREEPER, EGG_DRAGON, EGGS };
		static bool BreakTarget(oCNpc* hero, float npcDistance); // (the blocks, further down)
		static void InventoryInput(oCNpc* hero);
		static void UseSelected(oCNpc* hero);
		static bool SelectedIsGothic();
		static void DrawInventory(oCNpc* hero);
		static bool GothicLedge(oCNpc* hero);
		static void UpdateLadder(oCNpc* hero);
		static void RightClick(oCNpc* hero);
		static void ReleaseBow(oCNpc* hero, bool shoot);
		static bool HoldingBow(oCNpc* hero);
		static bool McInHand(oCNpc* hero);
		static void Ignite(oCNpc* hero);
		static void UpdateTnt(oCNpc* hero);
		static void UpdateDragon(oCNpc* hero);
		static void DrawBossBar();
		static bool DragonMelee(oCNpc* hero, float npcDistance, float damage);
		static int DragonPartOnRay(const zVEC3& from, const zVEC3& ray, float& t);
		static void DragonHurt(oCNpc* hero, int part, float damage, bool arrow);
		static void DragonExplosion(oCNpc* hero, const zVEC3& at, float radius);
		static void PourLava(oCNpc* hero);
		static void FillBucket(oCNpc* hero);
		static void PhysicsTicks(oCNpc* hero);
		static bool HeroBurning(oCNpc* hero);
		static bool EyesInLava(oCNpc* hero);
		static zCTexture* FireOverlayFrame();
		static bool heroInLava = false, heroDeepLava = false; // (EntityTicks)
		static float gui = 2.0f; // screen pixels a GUI pixel (the HUD's scale)
		static int guiW = 320, guiH = 240;
		static float velX = 0.0f, velY = 0.0f, velZ = 0.0f;    // blocks a tick, after the last tick
		static float moveX = 0.0f, moveY = 0.0f, moveZ = 0.0f; // the last tick's move, blocks
		static float tickClock = 0.0f;  // seconds into the current tick
		static int jumpDelay = 0, tapTicks = 0;
		static float swingPos = 0.0f, swingPosOld = 0.0f, swingAmount = 0.0f, swingAmountOld = 0.0f;
		static float bob = 0.0f, bobOld = 0.0f, walkDist = 0.0f, walkDistOld = 0.0f;
		static float fovMod = 1.0f, fovModOld = 1.0f;
		static zVEC3 groundMove(0.0f, 0.0f, 0.0f); // cm he moves this frame on the ground (root motion)
		static zVEC3 lastPos(0.0f, 0.0f, 0.0f), lastWanted(0.0f, 0.0f, 0.0f);
		static int blockedSide = 0, blockedUp = 0;
		static float airLogUntil = -1.0f;
		static const float TICK = 0.05f, BLOCK = 100.0f;

		static float Lerp(float a, float b, float t) { return a + (b - a) * t; }
		static float TickShare() { return tickClock / TICK; } // Minecraft's partialTick

		// One of Minecraft's ticks of his movement (LocalPlayer.aiStep, LivingEntity.jumpFromGround,
		// LivingEntity.travel, WalkAnimationState.update, Player.aiStep's bob). True if he jumped.
		static bool Tick(oCNpc* hero)
		{
			swingPosOld = swingPos;
			swingAmountOld = swingAmount;
			attackAnimOld = attackAnim; // LivingEntity.updateSwingTime: 6 ticks a swing
			if (swingTicks >= 0 && ++swingTicks >= 6)
				swingTicks = -1;
			attackAnim = swingTicks >= 0 ? swingTicks / 6.0f : 0.0f;
			attackStrength++;
			bobOld = bob;
			walkDistOld = walkDist;
			fovModOld = fovMod;
			if (jumpDelay > 0)
				jumpDelay--;
			if (tapTicks > 0)
				tapTicks--;
			if (useTicks >= 0)
				useTicks++;
			float forward = (keyForward.held ? 1.0f : 0.0f) - (keyBack.held ? 1.0f : 0.0f);
			float side = (keyRight.held ? 1.0f : 0.0f) - (keyLeft.held ? 1.0f : 0.0f);
			bool wasSneaking = sneaking;
			sneaking = keySneak.held;
			if (sneaking != wasSneaking)
				Log("sneak %s", sneaking ? "on" : "off");
			if (!sprinting && forward > 0.0f && !sneaking && useTicks < 0 && foodLevel > 6 && (keySprint.held || doubleTap))
			{
				sprinting = true;
				Log("sprint on");
			}
			doubleTap = false;
			if (sprinting && (forward <= 0.0f || sneaking || useTicks >= 0))
			{
				sprinting = false;
				Log("sprint off");
			}
			float len = sqrtf(forward * forward + side * side);
			if (len > 1.0f)
			{
				forward /= len;
				side /= len;
			}
			if (sneaking)
			{
				forward *= 0.3f;
				side *= 0.3f;
			}
			if (useTicks >= 0) // drawing the bow: a fifth (LocalPlayer.modifyInput)
			{
				forward *= 0.2f;
				side *= 0.2f;
			}
			float fx = sinf(yaw), fz = cosf(yaw), rx = cosf(yaw), rz = -sinf(yaw);
			bool ground = onGround; // the tick's travel as he stood at its start (a jump's too)
			bool jump = false;
			if (heroDeepLava && keyJump.held) // LivingEntity.jumpInLiquid
			{
				velY += 0.04f;
				if (onGround)
				{
					onGround = false;
					jump = true;
				}
			}
			else if (keyJump.held && onGround && jumpDelay == 0)
			{
				oCAniCtrl_Human* ani = hero->GetAnictrl();
				if (ani && ani->CanJumpLedge() && GothicLedge(hero))
				{
					askLedge = true;
					Log("jump: a ledge Gothic climbs");
				}
				else
				{
					velY = 0.42f;
					if (sprinting)
					{
						velX += fx * 0.2f;
						velZ += fz * 0.2f;
					}
					onGround = false;
					jumpDelay = 10;
					jump = true;
					exhaustion += sprinting ? 0.2f : 0.05f; // Player.jumpFromGround
				}
			}
			float speed = 0.1f * (sprinting ? 1.3f : 1.0f);
			float accel = ground ? speed : (sprinting ? 0.026f : 0.02f);
			float friction = ground ? 0.6f * 0.91f : 0.91f;
			if (heroInLava) // LivingEntity.travelInLava: 0.02 a tick, half of it kept
			{
				accel = 0.02f;
				friction = 0.5f;
			}
			velX += (fx * forward + rx * side) * accel;
			velZ += (fz * forward + rz * side) * accel;
			if (onLadder) // LivingEntity.handleOnClimbable
			{
				velX = velX < -0.15f ? -0.15f : (velX > 0.15f ? 0.15f : velX);
				velZ = velZ < -0.15f ? -0.15f : (velZ > 0.15f ? 0.15f : velZ);
				velY = velY < -0.15f ? -0.15f : velY;
				if (sneaking && velY < 0.0f)
					velY = 0.0f;
				fallDistance = 0.0f;
			}
			moveX = velX;
			moveY = onGround ? 0.0f : velY;
			moveZ = velZ;
			velX *= friction;
			velZ *= friction;
			if (onLadder && ((keyForward.held && ladderAhead) || keyJump.held)) // pushing into it, or jumping: up
			{
				if (onGround)
				{
					onGround = false;
					jump = true;
				}
				velY = 0.2f;
			}
			velY = onGround ? 0.0f : (heroInLava ? velY * 0.5f - 0.02f : (velY - 0.08f) * 0.98f);
			if (!onGround && moveY < 0.0f)
				fallDistance -= moveY;
			float moved = sqrtf(moveX * moveX + moveZ * moveZ);
			float target = moved * 4.0f > 1.0f ? 1.0f : moved * 4.0f;
			swingAmount += (target - swingAmount) * 0.4f;
			swingPos += swingAmount;
			bob += ((onGround ? (moved < 0.1f ? moved : 0.1f) : 0.0f) - bob) * 0.4f;
			walkDist += moved * 0.6f;
			if (sprinting && ground)
				exhaustion += moved * 0.1f; // Player.checkMovementStatistics: 0.1 a sprinted block
			float zoom = 1.0f;
			if (useTicks >= 0) // AbstractClientPlayer.getFieldOfViewModifier: the drawn bow narrows the view
			{
				float f = useTicks / 20.0f;
				f = f > 1.0f ? 1.0f : f * f;
				zoom = 1.0f - f * 0.15f;
			}
			fovMod += ((sprinting ? 1.15f : 1.0f) * zoom - fovMod) * 0.5f;
			return jump;
		}

		// --- camera ----------------------------------------------------------------------------------
		enum View { FIRST_PERSON, BEHIND, IN_FRONT };
		static int view = FIRST_PERSON;
		static bool viewSet = false;
		static bool cameraActive = false; // the camera is Minecraft's this frame
		static float gothicFovX = 0.0f, gothicFovY = 0.0f;
		static const float THIRD_PERSON = 400.0f; // Minecraft's 4 blocks

		static bool CameraModeAllowed(zCAICamera* ai)
		{
			if (!ai || !ai->camVob)
				return false;
			zSTRING mode = ai->curcammode;
			mode.Upper();
			return mode == "CAMMODNORMAL" || mode == "CAMMODRUN" || mode == "CAMMODJUMP" || mode == "CAMMODJUMPUP"
				|| mode == "CAMMODFALL" || mode == "CAMMODSWIM" || mode == "CAMMODDIVE" || mode == "CAMMODLOOKBACK"
				|| mode == "CAMMODMELEE" || mode == "CAMMODMELEEMULT" || mode == "CAMMODRANGED" || mode == "CAMMODRANGEDSHORT"
				|| mode == "CAMMODMAGIC" || mode == "CAMMODLOOK";
		}

		static bool ControlsActive(oCNpc* hero)
		{
			return settings.controls && hero && !hero->IsDead() && !hero->IsUnconscious() && !hero->GetInteractMob()
				&& CameraModeAllowed(zCAICamera::current) && zCAICamera::current->target == hero && IsSteve(hero);
		}

		static zVEC3 Forward() { return zVEC3(sinf(yaw), 0.0f, cosf(yaw)); }

		static zVEC3 Look()
		{
			return zVEC3(sinf(yaw) * cosf(pitch), -sinf(pitch), cosf(yaw) * cosf(pitch));
		}

		static void SetFov(bool minecraft)
		{
			zCCamera* camera = zCCamera::activeCam;
			if (!camera)
				return;
			if (gothicFovY <= 0.0f)
				camera->GetFOV(gothicFovX, gothicFovY);
			if (gothicFovX <= 0.0f || gothicFovY <= 0.0f)
				return;
			if (!minecraft || settings.fov <= 1.0f)
			{
				camera->SetFOV(gothicFovX, gothicFovY);
				return;
			}
			float aspect = tanf(gothicFovX * PI / 360.0f) / tanf(gothicFovY * PI / 360.0f);
			float fovY = settings.fov * Lerp(fovModOld, fovMod, TickShare());
			fovY = fovY > 150.0f ? 150.0f : fovY;
			float fovX = 2.0f * atanf(tanf(fovY * PI / 360.0f) * aspect) * 180.0f / PI;
			camera->SetFOV(fovX, fovY);
		}

		// His body in first person: unseen (Minecraft draws none of it there). Another hero (a load, a
		// change of body) gets his old one shown again if it is still there.
		static oCNpc* hiddenHero = nullptr;

		static void ShowBody(oCNpc* hero, bool show)
		{
			if (hiddenHero && hiddenHero != hero)
			{
				oCWorld* game = ogame ? ogame->GetGameWorld() : nullptr;
				for (zCListSort<oCNpc>* node = game && game->voblist_npcs ? game->voblist_npcs->next : nullptr; node; node = node->next)
					if (node->data == hiddenHero)
						hiddenHero->showVisual = TRUE;
				hiddenHero = nullptr;
			}
			if (!hero)
				return;
			if ((hero->showVisual != 0) != show)
			{
				hero->showVisual = show ? TRUE : FALSE;
				Log("body: %s", show ? "seen" : "unseen (first person)");
			}
			hiddenHero = show ? nullptr : hero;
		}

		// The camera's near clip: Gothic's 35 cm (zCCamera's own, 1 cm only with a w-buffer) would cut
		// the hand in first person, where it is 5 cm; Gothic's again in the other views.
		static zCCamera* nearCamera = nullptr;
		static float gothicNear = -1.0f;
		static const float FP_NEAR = 5.0f;

		static void SetNearClip(bool firstPerson)
		{
			zCCamera* camera = zCCamera::activeCam;
			if (!camera)
				return;
			if (camera != nearCamera)
			{
				nearCamera = camera;
				gothicNear = -1.0f;
			}
			if (firstPerson)
			{
				if (gothicNear < 0.0f)
				{
					gothicNear = camera->nearClipZ;
					Log("camera: near clip %.0f cm -> %.0f cm (first person)", gothicNear, gothicNear < FP_NEAR ? gothicNear : FP_NEAR);
				}
				camera->nearClipZ = gothicNear < FP_NEAR ? gothicNear : FP_NEAR;
			}
			else if (gothicNear >= 0.0f)
			{
				camera->nearClipZ = gothicNear;
				gothicNear = -1.0f;
			}
		}

		// v turned by `degrees` about the unit `axis` (the right-handed formula)
		static zVEC3 Turned(const zVEC3& v, const zVEC3& axis, float degrees)
		{
			float a = degrees * PI / 180.0f, c = cosf(a), s = sinf(a);
			zVEC3 cross(axis[1] * v[2] - axis[2] * v[1], axis[2] * v[0] - axis[0] * v[2], axis[0] * v[1] - axis[1] * v[0]);
			float d = axis[0] * v[0] + axis[1] * v[1] + axis[2] * v[2];
			return v * c + cross * s + axis * (d * (1.0f - c));
		}

		// LivingEntity.hurtTime (10 ticks) and hurtDir: from whom he fights, else none (0)
		static float hurtStart = -100.0f, hurtDir = 0.0f;
		static int camHealth = -1;

		static void UpdateCamera(zCAICamera* ai)
		{
			LoadSettings();
			oCNpc* hero = oCNpc::player;
			bool active = hero && ai && ai->camVob && ControlsActive(hero);
			if (!active)
			{
				inventoryOpen = false;
				ShowBody(hero, true);
				if (cameraActive)
				{
					SetNearClip(false);
					SetFov(false);
					Log("camera: Gothic's");
				}
				cameraActive = false;
				camHealth = -1;
				return;
			}
			if (!viewSet)
			{
				view = settings.firstPerson ? FIRST_PERSON : BEHIND;
				viewSet = true;
			}
			if (!cameraActive) // take over from Gothic's camera: look where Steve faces
			{
				zVEC3 at = Flat(hero->GetAtVectorWorld());
				yaw = atan2f(at[0], at[2]);
				pitch = 0.0f;
				cameraActive = true;
				Log("camera: Minecraft's, %s", view == FIRST_PERSON ? "first person" : (view == BEHIND ? "behind" : "in front"));
			}
			float dx = 0.0f, dy = 0.0f, dz = 0.0f;
			zinput->GetMousePos(dx, dy, dz);
			if (inventoryOpen)
			{
				cursorX += dx * 0.5f;
				cursorY += dy * 0.5f;
				cursorX = cursorX < 0.0f ? 0.0f : (cursorX > guiW ? (float)guiW : cursorX);
				cursorY = cursorY < 0.0f ? 0.0f : (cursorY > guiH ? (float)guiH : cursorY);
				dx = dy = dz = 0.0f;
			}
			if (dz != 0.0f)
				wheelSteps += dz > 0.0f ? 1 : -1;
			yaw += dx * settings.mouseSpeed * PI / 180.0f;
			pitch += (settings.invertMouseY ? -dy : dy) * settings.mouseSpeed * PI / 180.0f;
			float limit = 89.5f * PI / 180.0f;
			pitch = pitch < -limit ? -limit : (pitch > limit ? limit : pitch);

			// the eyes: Minecraft's eye height above his feet (1.62 m, 1.27 m sneaking) while his
			// movement is Minecraft's, else the middle of his head (make_steve.py: the head node sits
			// on his neck, the head 8 pixels up from it)
			zVEC3 eye = hero->GetPositionWorld() + zVEC3(0.0f, 28.0f * PIXEL - 12.0f * PIXEL, 0.0f);
			if (physicsOn && hero->human_ai)
				eye[1] = hero->human_ai->feetY + (sneaking ? 127.0f : 162.0f);
			else if (view != FIRST_PERSON) // (in first person his model is unseen, its nodes perhaps not placed)
			{
				if (zCModel* model = hero->GetModel())
					if (zCModelNodeInst* head = model->SearchNode(zSTRING("BIP01 HEAD")))
						eye = model->GetNodePositionWorld(head) + zVEC3(0.0f, 4.0f * PIXEL, 0.0f);
			}
			zVEC3 look = Look();
			bool first = view == FIRST_PERSON;
			ShowBody(hero, !first);
			SetNearClip(first);
			zVEC3 position = eye;
			zVEC3 heading = look;
			// what bobHurt and bobView do to Minecraft's view, done to the camera in its own frame
			// (Gothic's axes: Minecraft's view space with z the other way): a view turn R(a, t) is the
			// camera's turn about the axis with z flipped by the same t; a view shift the camera's the
			// other way
			zVEC3 shift(0.0f, 0.0f, 0.0f);
			float nod = 0.0f, roll = 0.0f, hurtTilt = 0.0f;
			int health = hero->attribute[NPC_ATR_HITPOINTS];
			if (camHealth >= 0 && health < camHealth)
			{
				hurtStart = Now();
				hurtDir = 0.0f;
				oCNpc* enemy = hero->enemy;
				if (enemy && enemy != hero && (enemy->GetPositionWorld() - hero->GetPositionWorld()).Length() < 1500.0f)
				{
					zVEC3 to = enemy->GetPositionWorld() - hero->GetPositionWorld();
					zVEC3 side(cosf(yaw), 0.0f, -sinf(yaw));
					hurtDir = 90.0f + atan2f(to.Dot(side), to.Dot(Forward())) * 180.0f / PI;
				}
			}
			camHealth = health;
			float hurt = 10.0f - (Now() - hurtStart) / TICK;
			if (hurt > 0.0f)
			{
				float h = hurt / 10.0f;
				hurtTilt = -sinf(h * h * h * h * PI) * 14.0f;
			}
			if (physicsOn) // bobView, in blocks
			{
				float share = TickShare();
				float walked = -Lerp(walkDistOld, walkDist, share);
				float h = Lerp(bobOld, bob, share);
				shift = zVEC3(-sinf(walked * PI) * h * 0.5f, fabsf(cosf(walked * PI) * h), 0.0f) * BLOCK;
				roll = sinf(walked * PI) * h * 3.0f;
				nod = fabsf(cosf(walked * PI - 0.2f) * h) * 5.0f;
			}
			if (!first)
			{
				zVEC3 ray = (view == BEHIND ? look * -1.0f : look) * THIRD_PERSON;
				position = eye + ray;
				zCWorld* world = hero->GetHomeWorld();
				int flags = zTRACERAY_STAT_POLY | zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER;
				if (world && world->TraceRayNearestHit(eye, ray, hero, flags) && world->traceRayReport.foundHit)
				{
					float dist = (world->traceRayReport.foundIntersection - eye).Length() - 10.0f;
					position = eye + ray * ((dist > 10.0f ? dist : 10.0f) / THIRD_PERSON);
				}
				if (view == IN_FRONT)
					heading = look * -1.0f;
			}
			zVEC3 at0 = heading;
			zVEC3 right0(at0[2], 0.0f, -at0[0]); // up x at
			right0.Normalize();
			zVEC3 up0(at0[1] * right0[2] - at0[2] * right0[1], at0[2] * right0[0] - at0[0] * right0[2], at0[0] * right0[1] - at0[1] * right0[0]);
			zVEC3 axes[3] = { zVEC3(1.0f, 0.0f, 0.0f), zVEC3(0.0f, 1.0f, 0.0f), zVEC3(0.0f, 0.0f, 1.0f) };
			float hd = hurtDir * PI / 180.0f;
			zVEC3 hurtAxis(-sinf(hd), 0.0f, -cosf(hd));
			for (zVEC3& v : axes)
			{
				if (hurtTilt != 0.0f)
					v = Turned(v, hurtAxis, hurtTilt);
				if (roll != 0.0f)
					v = Turned(v, zVEC3(0.0f, 0.0f, -1.0f), roll);
				if (nod != 0.0f)
					v = Turned(v, zVEC3(1.0f, 0.0f, 0.0f), nod);
			}
			auto world = [&](const zVEC3& v) { return right0 * v[0] + up0 * v[1] + at0 * v[2]; };
			zMAT4 trafo = zMAT4::GetIdentity();
			trafo.SetRightVector(world(axes[0]));
			trafo.SetUpVector(world(axes[1]));
			trafo.SetAtVector(world(axes[2]));
			trafo.SetTranslation(position + world(shift));
			ai->camVob->SetTrafoObjToWorld(trafo);
			SetFov(true);
		}

		static void NextView()
		{
			view = (view + 1) % 3;
			Log("F5: %s", view == FIRST_PERSON ? "first person" : (view == BEHIND ? "behind" : "in front"));
		}

		// --- Minecraft's movement, each frame ------------------------------------------------------
		static int BodyState(oCNpc* hero) { return hero->GetBodyState() & BS_ONLY_STATE; }

		static void ReadKeys()
		{
			bool focus = GameHasFocus();
			bool forward = focus && KeyDown(KEY_W);
			if (forward && !keyForward.held) // W tapped twice within 7 ticks sprints
			{
				if (tapTicks > 0)
					doubleTap = true;
				tapTicks = 7;
			}
			keyForward.Update(forward);
			keyBack.Update(focus && KeyDown(KEY_S));
			keyLeft.Update(focus && KeyDown(KEY_A));
			keyRight.Update(focus && KeyDown(KEY_D));
			keyJump.Update(focus && KeyDown(KEY_SPACE));
			keySneak.Update(focus && KeyDown(KEY_LSHIFT));
			keySprint.Update(focus && KeyDown(KEY_LCONTROL));
			keyAttack.Update(MouseDown(true));
			keyUse.Update(MouseDown(false));
		}

		// Sneaking on the ground he doesn't walk off an edge (Player.maybeBackOffFromEdge): a move that
		// would leave no floor within a step (60 cm) under his feet loses that part.
		static bool FloorUnder(oCNpc* hero, const zVEC3& at, float feet)
		{
			zCWorld* world = hero->GetHomeWorld();
			if (!world)
				return true;
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER;
			zVEC3 from(at[0], feet + 30.0f, at[2]);
			return world->TraceRayNearestHit(from, zVEC3(0.0f, -90.0f, 0.0f), hero, flags) && world->traceRayReport.foundHit;
		}

		static zVEC3 EdgeSafe(oCNpc* hero, zVEC3 move)
		{
			if (!hero->human_ai)
				return move;
			zVEC3 at = hero->GetPositionWorld();
			float feet = hero->human_ai->feetY;
			if (FloorUnder(hero, at + move, feet))
				return move;
			zVEC3 x(move[0], 0.0f, 0.0f), z(0.0f, 0.0f, move[2]);
			bool keepX = FloorUnder(hero, at + x, feet), keepZ = FloorUnder(hero, at + z, feet);
			if (!keepX)
				velX = 0.0f;
			if (!keepZ)
				velZ = 0.0f;
			return (keepX ? x : zVEC3(0.0f, 0.0f, 0.0f)) + (keepZ ? z : zVEC3(0.0f, 0.0f, 0.0f));
		}

		static void StopMovement()
		{
			velX = velY = velZ = moveX = moveY = moveZ = 0.0f;
			sprinting = false;
			groundMove = zVEC3(0.0f, 0.0f, 0.0f);
		}

		// The water's depth at his feet (0 on dry land): in water Gothic moves him (wading, swimming).
		static float WaterDepth(oCNpc* hero)
		{
			zCCollisionObject* collision = hero->GetCollisionObject();
			if (!collision || !hero->human_ai)
				return 0.0f;
			float depth = static_cast<zCCollObjectCharacter*>(collision)->m_oSpatialState.m_fWaterY - hero->human_ai->feetY;
			return depth > 0.0f && depth < 10000.0f ? depth : 0.0f;
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

		static zVEC3 Eyes(oCNpc* hero)
		{
			zVEC3 eye = hero->GetPositionWorld();
			if (hero->human_ai)
				eye[1] = hero->human_ai->feetY + (sneaking ? 127.0f : 162.0f);
			return eye;
		}

		// Who his crosshair points at within `reach` cm from his eyes (GameRenderer.pick), not behind a
		// wall: the nearest living character's box (taken in to 70 % across, as the GTA mashup's aim).
		static oCNpc* Target(oCNpc* hero, float reach, zVEC3& point)
		{
			zVEC3 from = Eyes(hero), dir = Look();
			float wall = reach;
			zCWorld* world = hero->GetHomeWorld();
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER;
			if (world && world->TraceRayNearestHit(from, dir * reach, hero, flags) && world->traceRayReport.foundHit)
				wall = (world->traceRayReport.foundIntersection - from).Length();
			oCWorld* game = ogame ? ogame->GetGameWorld() : nullptr;
			oCNpc* best = nullptr;
			float bestDistance = wall;
			for (zCListSort<oCNpc>* node = game && game->voblist_npcs ? game->voblist_npcs->next : nullptr; node; node = node->next)
			{
				oCNpc* npc = node->data;
				if (!npc || npc == hero || npc->IsDead())
					continue;
				zTBBox3D box = npc->bbox3D;
				zVEC3 centre = (box.mins + box.maxs) * 0.5f;
				for (int i = 0; i < 3; i += 2)
				{
					box.mins[i] = centre[i] + (box.mins[i] - centre[i]) * 0.7f;
					box.maxs[i] = centre[i] + (box.maxs[i] - centre[i]) * 0.7f;
				}
				float distance = RayBox(from, dir, box);
				if (distance >= 0.0f && distance < bestDistance)
				{
					bestDistance = distance;
					best = npc;
				}
			}
			point = from + dir * bestDistance;
			return best;
		}

		// A left click: Minecraft's attack (Player.attack). The arm swings; whoever the crosshair points
		// at within 3 m (Minecraft's 3 blocks) takes the hit Gothic deals with what he holds (his
		// strength, plus a drawn melee weapon's damage), times Minecraft's cooldown share: 0.2 + 0.8 x
		// charge squared, the charge filling in 20 / attack speed ticks (4 for the fist, 1.6 a one-hander,
		// 1.0 a two-hander). Gothic then plays the hit and the victim's reaction.
		static void Attack(oCNpc* hero)
		{
			swingTicks = 0;
			exhaustion += 0.1f; // Player.attack
			int mode = hero->GetWeaponMode();
			oCItem* weapon = (mode == NPC_WEAPON_1HS || mode == NPC_WEAPON_2HS) ? hero->GetWeapon() : nullptr;
			float speed = mode == NPC_WEAPON_1HS ? 1.6f : (mode == NPC_WEAPON_2HS ? 1.0f : 4.0f);
			float charge = (attackStrength + 0.5f) / (20.0f / speed);
			charge = charge > 1.0f ? 1.0f : charge;
			attackStrength = 0;
			zVEC3 point;
			oCNpc* target = Target(hero, 300.0f, point);
			if (BreakTarget(hero, target ? (point - Eyes(hero)).Length() : 1e30f))
				return;
			float hitDamage = ((float)hero->attribute[NPC_ATR_STRENGTH] + (weapon ? (float)weapon->damageTotal : 0.0f)) * (0.2f + charge * charge * 0.8f);
			if (DragonMelee(hero, target ? (point - Eyes(hero)).Length() : 1e30f, hitDamage))
				return;
			if (!target)
			{
				Log("attack: a swing at nothing");
				return;
			}
			float damage = (float)hero->attribute[NPC_ATR_STRENGTH] + (weapon ? (float)weapon->damageTotal : 0.0f);
			damage *= 0.2f + charge * charge * 0.8f;
			int type = weapon && weapon->damageTypes ? (int)weapon->damageTypes : oEDamageType_Blunt;
			Log("attack: hitting %s for %.0f", target->GetInstanceName().ToChar(), damage);
			step = "attack: Gothic's damage";
			target->OnDamage(hero, hero, damage, type, point);
			step = "movement";
			Log("attack: hit %s for %.0f (charge %.2f; health %d/%d)", target->GetInstanceName().ToChar(), damage, charge,
				target->attribute[NPC_ATR_HITPOINTS], target->attribute[NPC_ATR_HITPOINTSMAX]);
		}

		// Landing: Minecraft's fall damage (LivingEntity.calculateFallDamage: a point of its 20 health
		// for each block past 3), as that share of his health.
		static void Land(oCNpc* hero)
		{
			airborne = false;
			hero->SetPhysicsEnabled(FALSE);
			velY = 0.0f;
			int points = (int)ceilf(fallDistance - 3.0f);
			Log("landed after %.1f blocks", fallDistance);
			if (points > 0)
			{
				float damage = points / 20.0f * hero->attribute[NPC_ATR_HITPOINTSMAX];
				hero->OnDamage(hero, nullptr, damage + hero->protection[oEDamageIndex_Fall], oEDamageType_Fall, hero->GetPositionWorld());
				Log("fall damage: %d of Minecraft's 20 -> %.0f", points, damage);
			}
			fallDistance = 0.0f;
		}

		// An explosion's push (Entity.push): his speed (blocks a tick) gets it; pushed up, he leaves the ground.
		static void Knock(oCNpc* hero, const zVEC3& push)
		{
			if (!physicsOn)
				return;
			velX += push[0];
			velY += push[1];
			velZ += push[2];
			if (push[1] > 0.0f && !airborne)
			{
				airborne = true;
				onGround = false;
				fallDistance = 0.0f;
				hero->SetPhysicsEnabled(TRUE);
				airLogUntil = Now() + 1.5f;
			}
			Log("pushed: %.2f %.2f %.2f blocks a tick", push[0], push[1], push[2]);
		}

		// Before his DoAI: Minecraft's ticks for the time passed, then the move for this frame.
		static void UpdateMovement(oCNpc* hero)
		{
			ReadKeys();
			if (inventoryOpen) // its clicks; he stands still meanwhile
			{
				InventoryInput(hero);
				for (Key* k : { &keyForward, &keyBack, &keyLeft, &keyRight, &keyJump, &keySneak, &keySprint, &keyAttack, &keyUse })
					k->Update(false);
			}
			askLedge = false;
			int state = BodyState(hero);
			bool groundState = state == BS_STAND || state == BS_WALK || state == BS_RUN || state == BS_SPRINT || state == BS_SNEAK;
			if (ledgeJump) // Gothic climbs: its movement until he stands again
			{
				if (groundState && !hero->physicsEnabled && Now() - ledgeTime > 0.3f)
				{
					ledgeJump = false;
					Log("jump: Gothic's climb done");
				}
				else
				{
					physicsOn = false;
					return;
				}
			}
			float depth = WaterDepth(hero);
			bool on = settings.physics && cameraActive && ControlsActive(hero) && (groundState || airborne) && depth < 40.0f
				&& hero->human_ai;
			if (on != physicsOn)
				Log("movement: %s (body state %d, water %.0f cm)", on ? "Minecraft's" : "Gothic's", state, depth);
			physicsOn = on;
			if (!on)
			{
				if (airborne) // Gothic's fall check takes the flight over
				{
					airborne = false;
					Log("in the air: Gothic's fall from here");
				}
				if (useTicks >= 0)
					ReleaseBow(hero, false);
				StopMovement();
				return;
			}
			float dt = ztimer->frameTimeFloat / 1000.0f;
			dt = dt > 0.1f ? 0.1f : dt;
			zVEC3 pos = hero->GetPositionWorld();
			float above = hero->human_ai->aboveFloor;

			// Entity.move: a wall takes the speed into it, a ceiling the speed up (he moved less than
			// a third of what he was moved two frames running)
			zVEC3 moved = pos - lastPos;
			float wantSide = sqrtf(lastWanted[0] * lastWanted[0] + lastWanted[2] * lastWanted[2]);
			float gotSide = sqrtf(moved[0] * moved[0] + moved[2] * moved[2]);
			blockedSide = wantSide > 1.0f && gotSide < wantSide * 0.3f ? blockedSide + 1 : 0;
			if (blockedSide >= 2 && (velX != 0.0f || velZ != 0.0f))
			{
				velX = velZ = 0.0f;
				if (sprinting)
					Log("sprint off: against a wall");
				sprinting = false;
			}
			blockedUp = lastWanted[1] > 1.0f && moved[1] < lastWanted[1] * 0.3f ? blockedUp + 1 : 0;
			if (blockedUp >= 2 && velY > 0.0f)
			{
				velY = 0.0f;
				Log("head against the ceiling");
			}

			// On the ground Gothic's floor follows him down up to a step (60 cm, its zMV_STEP_HEIGHT);
			// from a higher edge he falls. He lands falling with the floor at his feet.
			if (!airborne && above > 60.0f)
			{
				airborne = true;
				fallDistance = 0.0f;
				hero->SetPhysicsEnabled(TRUE);
				airLogUntil = Now() + 1.5f;
				Log("off an edge: falling (%.0f cm above the floor)", above);
			}
			else if (airborne && velY <= 0.0f && above <= 3.0f)
				Land(hero);
			onGround = !airborne;

			int mode = hero->GetWeaponMode();
			static int lastMode = -1, lastState = -1;
			if (mode != lastMode || state != lastState)
			{
				Log("weapon mode %d, body state %d", mode, state);
				lastMode = mode;
				lastState = state;
			}
			UpdateLadder(hero);
			if (useTicks >= 0 && (!keyUse.held || !HoldingBow(hero))) // let go: the arrow flies (BowItem.releaseUsing)
				ReleaseBow(hero, keyUse.held == false);
			if (keyAttack.pressed && useTicks < 0 && mode != NPC_WEAPON_BOW && mode != NPC_WEAPON_CBOW && mode != NPC_WEAPON_MAG)
				Attack(hero);
			if (keyUse.pressed)
				RightClick(hero);
			if (putAwayUntil > 0.0f && (mode == NPC_WEAPON_NONE || Now() > putAwayUntil)) // the weapon away: the interaction
			{
				if (mode == NPC_WEAPON_NONE)
				{
					useUntil = Now() + 0.5f;
					useEdge = true;
				}
				putAwayUntil = -1.0f;
			}

			tickClock += dt;
			int ticks = 0;
			jumped = false;
			while (tickClock >= TICK && ticks < 4)
			{
				jumped |= Tick(hero);
				tickClock -= TICK;
				ticks++;
			}
			if (tickClock >= TICK)
				tickClock = 0.0f;
			if (askLedge)
			{
				ledgeJump = true;
				ledgeTime = Now();
				StopMovement();
				physicsOn = false;
				return;
			}
			float rate = BLOCK / TICK; // blocks a tick -> cm a second
			zVEC3 velocity(moveX * rate, moveY * rate, moveZ * rate);
			if (jumped)
			{
				airborne = true;
				fallDistance = 0.0f;
				onGround = false;
				airLogUntil = Now() + 1.5f;
				Log("%s: %.0f cm/s up, %.0f cm/s along%s", onLadder ? "ladder" : "jump", velocity[1],
					sqrtf(velocity[0] * velocity[0] + velocity[2] * velocity[2]), sprinting ? " (sprinting)" : "");
			}
			if (onGround)
			{
				groundMove = zVEC3(velocity[0] * dt, 0.0f, velocity[2] * dt);
				if (sneaking)
					groundMove = EdgeSafe(hero, groundMove);
				lastWanted = groundMove;
			}
			else
			{
				groundMove = zVEC3(0.0f, 0.0f, 0.0f);
				if (!hero->physicsEnabled)
					hero->SetPhysicsEnabled(TRUE);
				if (zCRigidBody* body = hero->GetRigidBody())
					body->SetVelocity(velocity);
				lastWanted = velocity * dt;
			}
			static float speedLog = 0.0f, speedLogMoved = 0.0f, speedLogWanted = 0.0f, speedLogTime = 0.0f;
			speedLogMoved += sqrtf(moved[0] * moved[0] + moved[2] * moved[2]);
			speedLogWanted += sqrtf(velocity[0] * velocity[0] + velocity[2] * velocity[2]) * dt;
			speedLogTime += dt;
			if (Now() - speedLog > 1.0f && speedLogTime > 0.0f)
			{
				if (speedLogWanted > 1.0f)
					Log("speed: asked %.0f cm/s, moved %.0f cm/s, %s%s%s", speedLogWanted / speedLogTime, speedLogMoved / speedLogTime,
						onGround ? "on the ground" : "in the air", sprinting ? ", sprinting" : "", sneaking ? ", sneaking" : "");
				speedLog = Now();
				speedLogMoved = speedLogWanted = speedLogTime = 0.0f;
			}
			lastPos = pos;
			if (Now() < airLogUntil)
				Log("air: body state %d, physics %d, %.0f cm above the floor, feet %.0f, up %.0f cm/s",
					state, (int)hero->physicsEnabled, above, hero->human_ai->feetY, velocity[1]);
		}

		// Gothic's movement keys while Minecraft's movement is on: nothing but its interaction (action
		// and forward for half a second, RightClick) without a weapon, and the jump at a ledge. The left
		// button is Minecraft's attack (Attack); Gothic's own attacks don't start.
		static bool OverrideKey(unsigned short key, float& state, int& toggled)
		{
			if (!overrideInput)
				return false;
			auto set = [&](bool on, bool pressed) { state = on ? 1.0f : 0.0f; toggled = pressed ? 1 : 0; return true; };
			oCNpc* hero = oCNpc::player;
			int mode = hero ? hero->GetWeaponMode() : NPC_WEAPON_NONE;
			bool armed = mode != NPC_WEAPON_NONE;
			bool ranged = mode == NPC_WEAPON_BOW || mode == NPC_WEAPON_CBOW || mode == NPC_WEAPON_MAG;
			switch (key)
			{
			case GAME_ACTION:
				if (ranged) // shooting and casting stay Gothic's, on the left button
					return set(keyAttack.held, keyAttack.pressed);
				return set(!armed && Now() < useUntil, !armed && useEdge); // (Ctrl sprints)
			case GAME_SMOVE: return set(askLedge, askLedge);
			case GAME_UP:
				if (ranged) // Gothic 1 shoots on the action with the forward key
					return set(keyAttack.held, keyAttack.pressed);
				{
					bool interact = !armed && Now() < useUntil && hero && BodyState(hero) == BS_STAND;
					return set(interact, interact);
				}
			case GAME_DOWN: case GAME_LEFT: case GAME_RIGHT: case GAME_STRAFELEFT: case GAME_STRAFERIGHT:
			case GAME_SLOW: case GAME_SNEAK: case GAME_SMOVE2: case GAME_LOOK: case GAME_LOOK_FP: case GAME_ACTION2:
			case GAME_PARADE: case GAME_ACTIONLEFT: case GAME_ACTIONRIGHT:
				return set(false, false);
			default:
				return false;
			}
		}

		// --- Minecraft's pose (HumanoidModel.setupAnim) ----------------------------------------------
		struct M3
		{
			float m[3][3];
		};

		static M3 Mul(const M3& a, const M3& b)
		{
			M3 r;
			for (int i = 0; i < 3; i++)
				for (int j = 0; j < 3; j++)
					r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j];
			return r;
		}

		static M3 Rotation(const zMAT4& t)
		{
			M3 r;
			for (int i = 0; i < 3; i++)
				for (int j = 0; j < 3; j++)
					r.m[i][j] = t.v[i][j];
			return r;
		}

		// rotation by `angle` about the unit axis `a` (column vectors)
		static M3 AxisAngle(const float a[3], float angle)
		{
			float c = cosf(angle), s = sinf(angle), t = 1.0f - c;
			M3 r;
			r.m[0][0] = c + t * a[0] * a[0];        r.m[0][1] = t * a[0] * a[1] - s * a[2]; r.m[0][2] = t * a[0] * a[2] + s * a[1];
			r.m[1][0] = t * a[0] * a[1] + s * a[2]; r.m[1][1] = c + t * a[1] * a[1];        r.m[1][2] = t * a[1] * a[2] - s * a[0];
			r.m[2][0] = t * a[0] * a[2] - s * a[1]; r.m[2][1] = t * a[1] * a[2] + s * a[0]; r.m[2][2] = c + t * a[2] * a[2];
			return r;
		}

		// a node's bind pose rotation in the model's frame (its prototype's chain)
		static M3 BindWorld(zCModelNode* node)
		{
			if (!node)
			{
				M3 one = { { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } } };
				return one;
			}
			return Mul(BindWorld(node->parentNode), Rotation(node->trafo));
		}

		struct Limb
		{
			zCModelNodeInst* node = nullptr;
			M3 bind;              // its own bind rotation in its parent's frame
			float side[3], up[3], ahead[3]; // the model's x (right), y and z (forward) axes in its parent's frame
		};

		enum { LEG_R, LEG_L, ARM_R, ARM_L, HEAD, BODY, LIMBS };
		static const char* const LIMB_NODES[LIMBS] = { "BIP01 R THIGH", "BIP01 L THIGH", "BIP01 R UPPERARM", "BIP01 L UPPERARM",
			"BIP01 HEAD", "BIP01 SPINE1" };
		static Limb limbs[LIMBS];
		static zCModel* limbsModel = nullptr;
		static bool limbsFound = false;
		static zCModelNodeInst* rightHand = nullptr; // what he holds hangs on it

		static bool FindLimbs(zCModel* model)
		{
			if (model == limbsModel)
				return limbsFound;
			limbsModel = model;
			limbsFound = true;
			rightHand = model->SearchNode(zSTRING("BIP01 R HAND"));
			for (int i = 0; i < LIMBS; i++)
			{
				Limb& l = limbs[i];
				l.node = model->SearchNode(zSTRING(LIMB_NODES[i]));
				if (!l.node || !l.node->protoNode)
				{
					limbsFound = false;
					Log("pose: %s not found", LIMB_NODES[i]);
					continue;
				}
				l.bind = Rotation(l.node->protoNode->trafo);
				M3 parent = BindWorld(l.node->protoNode->parentNode);
				for (int k = 0; k < 3; k++)
				{
					l.side[k] = parent.m[0][k];  // parent^T (1, 0, 0)
					l.up[k] = parent.m[1][k];    // parent^T (0, 1, 0)
					l.ahead[k] = parent.m[2][k]; // parent^T (0, 0, 1)
				}
			}
			return limbsFound;
		}

		// Minecraft's part rotation (ModelPart: z, then y, then x, about the model's axes) on its bind pose
		static void SetLimb(int i, float xRot, float zRot = 0.0f, float yRot = 0.0f)
		{
			Limb& l = limbs[i];
			M3 r = Mul(AxisAngle(l.side, xRot), l.bind);
			if (yRot != 0.0f)
				r = Mul(AxisAngle(l.up, yRot), r);
			if (zRot != 0.0f)
				r = Mul(AxisAngle(l.ahead, zRot), r);
			for (int a = 0; a < 3; a++)
				for (int b = 0; b < 3; b++)
					l.node->trafo.v[a][b] = r.m[a][b];
		}

		// Gothic's pose is Minecraft's only while he stands or moves: Gothic's layer-1 animations are
		// its stands, walks and runs, falls and jumps (an action, a hit or an item keeps its own).
		static bool Neutral(zCModel* model)
		{
			for (int i = 0; i < model->numActiveAnis; i++)
			{
				zCModelAniActive* active = model->aniChannels[i];
				if (!active || !active->protoAni || active->protoAni->layer != 1)
					continue;
				char name[64];
				strncpy_s(name, active->protoAni->aniName.ToChar(), _TRUNCATE);
				_strupr_s(name);
				size_t n = strlen(name);
				auto ends = [&](const char* tail) { size_t k = strlen(tail); return n >= k && strcmp(name + n - k, tail) == 0; };
				bool still = name[0] == 'S' && (ends("RUN") || ends("WALK") || ends("SNEAK") || ends("RUNL") || ends("WALKL"));
				bool air = strstr(name, "FALL") || strstr(name, "JUMP");
				if (!still && !air)
					return false;
			}
			return true;
		}

		// After zCModel::AdvanceAnis on his model: the frame's ground move as its root motion, and
		// Minecraft's pose over Gothic's.
		static void AfterAnis(oCNpc* hero, zCModel* model)
		{
			if (onGround)
			{
				zVEC3 right = hero->GetRightVectorWorld(), at = hero->GetAtVectorWorld();
				model->vobTrans = zVEC3(groundMove.Dot(right), 0.0f, groundMove.Dot(at));
			}
			if (!FindLimbs(model) || !Neutral(model))
				return;
			float share = TickShare();
			float amount = Lerp(swingAmountOld, swingAmount, share);
			float walked = swingPos - swingAmount * (1.0f - share);
			float leg = cosf(walked * 0.6662f) * 1.4f * amount;
			SetLimb(LEG_R, leg);
			SetLimb(LEG_L, -leg);
			float lean = sneaking ? 0.5f : 0.0f;
			// the attack's swing (HumanoidModel.setupAttackAnimation; the arms hang on the body here, so
			// they turn with it already)
			float attack = Lerp(attackAnimOld, attackAnim, share);
			float bodyYaw = 0.0f, swingX = 0.0f, swingZ = 0.0f;
			if (attack > 0.0f)
			{
				bodyYaw = sinf(sqrtf(attack) * 2.0f * PI) * 0.2f;
				float f = 1.0f - attack;
				f *= f;
				f *= f;
				f = 1.0f - f;
				swingX = -(sinf(f * PI) * 1.2f + sinf(attack * PI) * -(pitch - 0.7f) * 0.75f);
				swingZ = sinf(attack * PI) * -0.4f;
			}
			if (sneaking || bodyYaw != 0.0f)
				SetLimb(BODY, lean, 0.0f, bodyYaw);
			int mode = hero->GetWeaponMode();
			bool mcHand = McInHand(hero);
			if (mcHand && rightHand && rightHand->protoNode) // the thing turns with the arm alone
				rightHand->trafo = rightHand->protoNode->trafo;
			if (mode == NPC_WEAPON_NONE || mode == NPC_WEAPON_FIST || mode == NPC_WEAPON_1HS || mode == NPC_WEAPON_2HS)
			{
				float age = Now() * 20.0f;
				float arm = cosf(walked * 0.6662f + PI) * amount;
				float bobX = sinf(age * 0.067f) * 0.05f, bobZ = cosf(age * 0.09f) * 0.05f + 0.05f;
				float crouch = sneaking ? 0.4f - lean : 0.0f; // Minecraft's arms lean 0.4, the body 0.5
				if (useTicks >= 0 && mode == NPC_WEAPON_NONE) // HumanoidModel.ArmPose.BOW_AND_ARROW
				{
					SetLimb(ARM_R, -PI / 2.0f + pitch + bobX + crouch, bobZ, -0.1f);
					SetLimb(ARM_L, -PI / 2.0f + pitch - bobX + crouch, -bobZ, 0.5f);
				}
				else
				{
					float right = arm, left = -arm;
					if (mode == NPC_WEAPON_1HS || mode == NPC_WEAPON_2HS || mcHand) // HumanoidModel.ArmPose.ITEM
						right = right * 0.5f - PI / 10.0f;
					SetLimb(ARM_R, right + bobX + crouch + swingX, bobZ + swingZ, bodyYaw * 2.0f);
					SetLimb(ARM_L, left - bobX + crouch + bodyYaw, -bobZ);
				}
			}
			SetLimb(HEAD, pitch - lean, 0.0f, -bodyYaw);
		}

		// --- Minecraft's hunger (FoodData) -------------------------------------------------------------
		// Sprinting (0.1 a block), jumping (0.05, sprinting 0.2), attacking (0.1), swimming (0.01 a
		// block) and being hurt (0.1) tire him; every 4 of it take a point of saturation, or of food
		// when there is none. A full bar (20) with saturation heals him fast (Minecraft's health a
		// point every 10 ticks per 6 saturation), 18 or more slowly (a point every 80 ticks, tiring
		// him by 6), an empty one starves him down to a point (Minecraft's normal difficulty).
		// Minecraft's health is his health's twentieth. Food Gothic's way (apples, bread, meat, beer)
		// fills the bar as Minecraft's matching food does (FoodData.eat). Kept by the plugin, not in
		// the savegame: full again on every new game and load.
		static float hungerClock = 0.0f;
		static int foodTimer = 0, lastHealthPoints = -1;
		static float healCarry = 0.0f;
		static zVEC3 lastSwimPos(0.0f, 0.0f, 0.0f);

		static void ResetHunger()
		{
			foodLevel = 20;
			saturation = 5.0f;
			exhaustion = 0.0f;
			foodTimer = 0;
			healCarry = 0.0f;
			lastHealthPoints = -1;
		}

		static float McHealth(oCNpc* hero)
		{
			int max = hero->attribute[NPC_ATR_HITPOINTSMAX];
			return max > 0 ? 20.0f * hero->attribute[NPC_ATR_HITPOINTS] / max : 0.0f;
		}

		// heal (or hurt) by Minecraft's health points, as that share of his
		static void ChangeHealth(oCNpc* hero, float points)
		{
			int max = hero->attribute[NPC_ATR_HITPOINTSMAX];
			healCarry += points / 20.0f * max;
			int whole = (int)healCarry;
			if (whole == 0)
				return;
			healCarry -= (float)whole;
			int health = hero->attribute[NPC_ATR_HITPOINTS] + whole;
			hero->attribute[NPC_ATR_HITPOINTS] = health > max ? max : (health < 1 ? 1 : health);
		}

		static void FoodTick(oCNpc* hero)
		{
			int health = hero->attribute[NPC_ATR_HITPOINTS];
			if (lastHealthPoints >= 0 && health < lastHealthPoints)
				exhaustion += 0.1f; // hurt
			int state = BodyState(hero);
			zVEC3 pos = hero->GetPositionWorld();
			if (state == BS_SWIM || state == BS_DIVE)
				exhaustion += (pos - lastSwimPos).Length() / BLOCK * 0.01f;
			lastSwimPos = pos;
			if (exhaustion > 4.0f)
			{
				exhaustion -= 4.0f;
				if (saturation > 0.0f)
					saturation = saturation > 1.0f ? saturation - 1.0f : 0.0f;
				else if (foodLevel > 0)
					foodLevel--;
			}
			bool hurt = health < hero->attribute[NPC_ATR_HITPOINTSMAX];
			if (saturation > 0.0f && hurt && foodLevel >= 20)
			{
				if (++foodTimer >= 10)
				{
					float f = saturation < 6.0f ? saturation : 6.0f;
					ChangeHealth(hero, f / 6.0f);
					exhaustion += f;
					foodTimer = 0;
				}
			}
			else if (foodLevel >= 18 && hurt)
			{
				if (++foodTimer >= 80)
				{
					ChangeHealth(hero, 1.0f);
					exhaustion += 6.0f;
					foodTimer = 0;
				}
			}
			else if (foodLevel <= 0)
			{
				if (++foodTimer >= 80)
				{
					if (McHealth(hero) > 1.0f)
						ChangeHealth(hero, -1.0f);
					foodTimer = 0;
				}
			}
			else
				foodTimer = 0;
			lastHealthPoints = hero->attribute[NPC_ATR_HITPOINTS];
		}

		static void UpdateHunger(oCNpc* hero)
		{
			if (!settings.hunger || hero->IsDead())
				return;
			float dt = ztimer->frameTimeFloat / 1000.0f;
			hungerClock += dt > 0.25f ? 0.25f : dt;
			while (hungerClock >= TICK)
			{
				FoodTick(hero);
				hungerClock -= TICK;
			}
		}

		// Minecraft's food (Foods.java: nutrition, saturation modifier) for Gothic's, by its name
		static void FoodValue(oCItem* item, int& nutrition, float& modifier)
		{
			char name[96];
			strncpy_s(name, item->GetInstanceName().ToChar(), _TRUNCATE);
			_strupr_s(name);
			struct { const char* part; int nutrition; float modifier; } table[] = {
				{ "APPLE", 4, 0.3f }, { "BREAD", 5, 0.6f }, { "LOAF", 5, 0.6f }, { "MUTTONRAW", 3, 0.3f }, { "RAW", 3, 0.3f },
				{ "MUTTON", 8, 0.8f }, { "MEAT", 8, 0.8f }, { "HAM", 8, 0.8f }, { "BACON", 8, 0.8f }, { "SAUSAGE", 6, 0.6f },
				{ "FISH", 5, 0.6f }, { "SOUP", 6, 0.6f }, { "STEW", 6, 0.6f }, { "RAGOUT", 6, 0.6f }, { "RICE", 5, 0.6f },
				{ "CHEESE", 4, 0.3f }, { "HONEY", 6, 0.1f }, { "BERR", 2, 0.1f }, { "MUSHROOM", 2, 0.1f }, { "PILZ", 2, 0.1f },
				{ "BEER", 1, 0.1f }, { "WINE", 1, 0.1f }, { "BOOZE", 1, 0.1f }, { "RUM", 1, 0.1f }, { "MILK", 1, 0.1f },
				{ "WATER", 1, 0.1f } };
			nutrition = 2;
			modifier = 0.3f;
			for (auto& t : table)
				if (strstr(name, t.part))
				{
					nutrition = t.nutrition;
					modifier = t.modifier;
					return;
				}
		}

		static void Eat(oCItem* item)
		{
			int nutrition;
			float modifier;
			FoodValue(item, nutrition, modifier);
			foodLevel = foodLevel + nutrition > 20 ? 20 : foodLevel + nutrition;
			saturation += nutrition * modifier * 2.0f;
			saturation = saturation > foodLevel ? (float)foodLevel : saturation;
			Log("ate %s: food %d, saturation %.1f", item->GetInstanceName().ToChar(), foodLevel, saturation);
		}

		// --- Minecraft's HUD (Hud.java, ContextualBar, ExperienceBar) ---------------------------------
		// make_mc_hud.py's atlas (MC_HUD.TGA) and its table (MC_HUD.TXT), drawn with Gothic's renderer
		// before the screen's views (zCView::DrawItems), pixel for pixel (no filtering), at Minecraft's
		// GUI scale: the largest whole number that leaves the screen at least 320 x 240 GUI pixels
		// (Window.calculateScale; guiScale sets a highest one), laid out on whole GUI pixels as
		// Minecraft's (integer halves). The blocks' icons (MC_ICONS.TGA) are drawn for that scale (1 to
		// 4), so each screen pixel shows the texel Minecraft's 3D block item would. Hidden in dialogues.
		//  - the vignette (darker at night), in first person the crosshair and the attack indicator
		//    under it (the charge filling; full when charged and looking at someone within reach);
		//  - the hotbar: Gothic's number keys 1 to 9 as its slots: 1 the melee weapon, 2 the ranged
		//    one, 3 to 9 the spells of those keys, shown as Minecraft's items (a sword or an axe of the
		//    tier its damage reaches, bow, crossbow, enchanted book for a rune, paper for a scroll,
		//    with its count); the selected slot is the drawn weapon or spell, else the last chosen one.
		//    The mouse wheel goes through the slots as Minecraft's does (Gothic's key for each);
		//  - his health as ten hearts (shaking at 4 points or less, blinking after a hit), armour (his
		//    protection against edges, 5 a point), food, air while diving (Gothic's dive time), the
		//    experience bar (Gothic's experience between its levels' thresholds, 500 x level apart)
		//    and his level in green, the item's name for two seconds after the slot changes, and the
		//    speed effect while Gothic's sprint (the speed potion) is on.
		// Gothic's health, breath and enemy bars go; its mana bar stays while a spell is drawn.
		struct Sprite
		{
			char name[40];
			float x, y, w, h;
		};
		struct Glyph
		{
			float x, y, w, h, advance, top, scale;
			bool ok;
		};
		static Sprite sprites[160];
		static int spriteCount = 0;
		struct BlockType
		{
			char id[32], sound[16], name[64];
			float hardness, resistance; // Minecraft's (resistance: an explosion's rays lose it)
			zCVisual* hand;
		};
		static BlockType blockTypes[32];
		static int blockTypeCount = 0;
		static char eggIds[EGGS][16] = { "zombie", "skeleton", "creeper", "ender_dragon" };
		static char eggNames[EGGS][64] = { "Zombie", "Skeleton", "Creeper", "Ender Dragon" };
		enum { ITEM_BOW, ITEM_ARROW, ITEM_FLINT, ITEM_LAVA, ITEM_BUCKET, MC_ITEMS }; // Minecraft's items (make_mc_items.py)
		static char itemIds[MC_ITEMS][24] = { "bow", "arrow", "flint_and_steel", "lava_bucket", "bucket" };
		static char itemNames[MC_ITEMS][64] = { "Bow", "Arrow", "Flint and Steel", "Lava Bucket", "Bucket" };
		// the blocks' icons (MC_ICONS.TGA), each drawn for the HUD's scales 1 to 4
		struct Icon
		{
			char id[32];
			int scale;
			float x, y, w, h;
		};
		static Icon icons[128];
		static int iconCount = 0;
		static float iconsW = 1.0f, iconsH = 1.0f;
		static zCTexture* iconsTexture = nullptr;
		// Point sampling picks a texel near a screen pixel's corner or its centre (by the renderer);
		// a fifth of a texel in, either lands inside a pixel's 4 x 4 texels at the HUD's scales.
		static const float NUDGE = 0.2f;
		static char inventoryTitle[64] = "Building Blocks";
		static char bossName[64] = "Ender Dragon";
		static Glyph glyphTable[256];
		static float spaceAdvance[256];
		static float atlasW = 1.0f, atlasH = 1.0f, atlasScale = 4.0f;
		static zCTexture* atlasTexture = nullptr;
		static zCTexture* vignetteTexture = nullptr;
		static bool hudLoaded = false, hudReady = false;

		static zCOLOR Argb(int a, int r, int g, int b)
		{
			zCOLOR c;
			c.r = (unsigned char)r;
			c.g = (unsigned char)g;
			c.b = (unsigned char)b;
			c.alpha = (unsigned char)a;
			return c;
		}

		// A texture of the plugin's, loaded and kept loaded: Gothic's resource manager drops a texture
		// unused for a few seconds and shows its default (stone) one while it comes back, which would
		// flash on an animation's frames (lava, fire, the explosion's puffs).
		static zCTexture* LoadTexture(const char* name)
		{
			zSTRING file = name;
			zCTexture* texture = zCTexture::Load(file, 0);
			if (texture && zresMan)
				zresMan->CacheIn(texture, -1.0f);
			if (texture)
				texture->cacheOutLock = TRUE;
			return texture;
		}

		static void LoadHud()
		{
			if (hudLoaded)
				return;
			hudLoaded = true;
			FILE* file = nullptr;
			if (fopen_s(&file, "_work\\DATA\\TEXTURES\\_COMPILED\\MC_HUD.TXT", "r") || !file)
			{
				Log("hud: MC_HUD.TXT missing (make_mc_hud.py writes it)");
				return;
			}
			char line[256];
			while (fgets(line, sizeof line, file))
			{
				char kind[16] = "";
				if (sscanf_s(line, "%15s", kind, (unsigned)sizeof kind) != 1)
					continue;
				if (!strcmp(kind, "atlas"))
					sscanf_s(line, "%*s %f %f %f", &atlasW, &atlasH, &atlasScale);
				else if (!strcmp(kind, "sprite") && spriteCount < 160)
				{
					Sprite& sp = sprites[spriteCount];
					if (sscanf_s(line, "%*s %39s %f %f %f %f", sp.name, (unsigned)sizeof sp.name, &sp.x, &sp.y, &sp.w, &sp.h) == 5)
						spriteCount++;
				}
				else if (!strcmp(kind, "glyph"))
				{
					int byte = -1;
					Glyph g = {};
					if (sscanf_s(line, "%*s %d %f %f %f %f %f %f %f", &byte, &g.x, &g.y, &g.w, &g.h, &g.advance, &g.top, &g.scale) == 8
						&& byte >= 0 && byte < 256)
					{
						g.ok = true;
						glyphTable[byte] = g;
					}
				}
				else if (!strcmp(kind, "block") && blockTypeCount < 32)
				{
					BlockType& b = blockTypes[blockTypeCount];
					int index = -1;
					if (sscanf_s(line, "%*s %d %31s %f %f %15s %63[^\r\n]", &index, b.id, (unsigned)sizeof b.id, &b.hardness, &b.resistance,
						b.sound, (unsigned)sizeof b.sound, b.name, (unsigned)sizeof b.name) == 6 && index == blockTypeCount)
					{
						b.hand = nullptr;
						blockTypeCount++;
					}
				}
				else if (!strcmp(kind, "egg"))
				{
					int index = -1;
					char id[16], name[64];
					if (sscanf_s(line, "%*s %d %15s %63[^\r\n]", &index, id, (unsigned)sizeof id, name, (unsigned)sizeof name) == 3
						&& index >= 0 && index < EGGS)
					{
						strcpy_s(eggIds[index], id);
						strcpy_s(eggNames[index], name);
					}
				}
				else if (!strcmp(kind, "item"))
				{
					int index = -1;
					char id[24], name[64];
					if (sscanf_s(line, "%*s %d %23s %63[^\r\n]", &index, id, (unsigned)sizeof id, name, (unsigned)sizeof name) == 3
						&& index >= 0 && index < MC_ITEMS)
					{
						strcpy_s(itemIds[index], id);
						strcpy_s(itemNames[index], name);
					}
				}
				else if (!strcmp(kind, "icons"))
					sscanf_s(line, "%*s %f %f", &iconsW, &iconsH);
				else if (!strcmp(kind, "icon") && iconCount < 128)
				{
					Icon& ic = icons[iconCount];
					if (sscanf_s(line, "%*s %31s %d %f %f %f %f", ic.id, (unsigned)sizeof ic.id, &ic.scale, &ic.x, &ic.y, &ic.w, &ic.h) == 6)
						iconCount++;
				}
				else if (!strcmp(kind, "title"))
					sscanf_s(line, "%*s %63[^\r\n]", inventoryTitle, (unsigned)sizeof inventoryTitle);
				else if (!strcmp(kind, "boss"))
					sscanf_s(line, "%*s %63[^\r\n]", bossName, (unsigned)sizeof bossName);
				else if (!strcmp(kind, "space"))
				{
					int byte = -1;
					float advance = 0.0f;
					if (sscanf_s(line, "%*s %d %f", &byte, &advance) == 2 && byte >= 0 && byte < 256)
						spaceAdvance[byte] = advance;
				}
			}
			fclose(file);
			atlasTexture = LoadTexture("MC_HUD.TGA");
			vignetteTexture = LoadTexture("MC_VIGNETTE.TGA");
			iconsTexture = LoadTexture("MC_ICONS.TGA");
			hudReady = atlasTexture && spriteCount > 0;
			Log("hud: %d sprites, atlas %.0f x %.0f %s, vignette %s, %d icons %s", spriteCount, atlasW, atlasH,
				atlasTexture ? "loaded" : "missing", vignetteTexture ? "loaded" : "missing", iconCount, iconsTexture ? "loaded" : "missing");
		}

		static const Sprite* FindSprite(const char* name)
		{
			for (int i = 0; i < spriteCount; i++)
				if (!strcmp(sprites[i].name, name))
					return &sprites[i];
			return nullptr;
		}

		// GUI pixels -> a textured rectangle on the screen
		static void Quad(zCTexture* texture, float x, float y, float w, float h, float u0, float v0, float u1, float v1, const zCOLOR& color)
		{
			if (!texture || w <= 0.0f || h <= 0.0f)
				return;
			float px[4] = { x, x + w, x + w, x }, py[4] = { y, y, y + h, y + h };
			float pu[4] = { u0, u1, u1, u0 }, pv[4] = { v0, v0, v1, v1 };
			zTRndSimpleVertex v[4];
			for (int i = 0; i < 4; i++)
			{
				v[i].pos = zVEC2(px[i] * gui, py[i] * gui);
				v[i].z = 1.0f;
				v[i].uv = zVEC2(pu[i], pv[i]);
				v[i].color = color;
			}
			zrenderer->DrawPolySimple(texture, v, 4);
		}

		// a sprite at its own size at GUI (x, y): `share` of its width from the left (the bars)
		static void Blit(const char* name, float x, float y, float share = 1.0f, const zCOLOR& color = Argb(255, 255, 255, 255))
		{
			const Sprite* sp = FindSprite(name);
			if (!sp)
				return;
			float u = sp->w * share;
			Quad(atlasTexture, x, y, sp->w / atlasScale * share, sp->h / atlasScale, (sp->x + NUDGE) / atlasW, (sp->y + NUDGE) / atlasH,
				(sp->x + u + NUDGE) / atlasW, (sp->y + sp->h + NUDGE) / atlasH, color);
		}

		// a block's icon at GUI (x, y), 16 x 16: the one drawn for the HUD's scale (the nearest there is)
		static void DrawBlockIcon(const char* id, float x, float y)
		{
			int want = gui > 4.0f ? 4 : (int)gui;
			const Icon* best = nullptr;
			for (int i = 0; i < iconCount; i++)
				if (!strcmp(icons[i].id, id) && (!best || abs(icons[i].scale - want) < abs(best->scale - want)))
					best = &icons[i];
			if (best && iconsTexture)
				Quad(iconsTexture, x, y, 16.0f, 16.0f, (best->x + NUDGE) / iconsW, (best->y + NUDGE) / iconsH, (best->x + best->w + NUDGE) / iconsW,
					(best->y + best->h + NUDGE) / iconsH, Argb(255, 255, 255, 255));
		}

		static float TextWidth(const char* text)
		{
			float width = 0.0f;
			for (const unsigned char* c = (const unsigned char*)text; *c; c++)
				width += glyphTable[*c].ok ? glyphTable[*c].advance : (spaceAdvance[*c] > 0.0f ? spaceAdvance[*c] : 4.0f);
			return width;
		}

		static void Glyphs(const char* text, float x, float y, const zCOLOR& color)
		{
			for (const unsigned char* c = (const unsigned char*)text; *c; c++)
			{
				const Glyph& g = glyphTable[*c];
				if (!g.ok)
				{
					x += spaceAdvance[*c] > 0.0f ? spaceAdvance[*c] : 4.0f;
					continue;
				}
				Quad(atlasTexture, x, y + g.top, g.w / atlasScale * g.scale, g.h / atlasScale * g.scale, (g.x + NUDGE) / atlasW,
					(g.y + NUDGE) / atlasH, (g.x + g.w + NUDGE) / atlasW, (g.y + g.h + NUDGE) / atlasH, color);
				x += g.advance;
			}
		}

		// Font.drawInBatch: the shadow one pixel right and down, a quarter as bright
		static void Text(const char* text, float x, float y, const zCOLOR& color, bool shadow = true)
		{
			if (shadow)
				Glyphs(text, x + 1.0f, y + 1.0f, Argb(color.alpha, color.r / 4, color.g / 4, color.b / 4));
			Glyphs(text, x, y, color);
		}

		// --- the hotbar: nine slots, each one of Gothic's number keys (1 its melee weapon, 2 its ranged
		// one, 3 to 9 the spell of that key), a block, a spawn egg, the bow or the arrow. The creative
		// inventory (E) puts any of them into any slot; the slots are kept in Gothic.ini ([MINECRAFT] hotbar).
		enum { ENTRY_NONE, ENTRY_GOTHIC, ENTRY_BLOCK, ENTRY_EGG, ENTRY_ITEM };
		struct Entry
		{
			int kind, value;
		};
		static bool Same(const Entry& a, const Entry& b) { return a.kind == b.kind && a.value == b.value; }
		static Entry hotbar[9] = { { ENTRY_GOTHIC, 0 }, { ENTRY_GOTHIC, 1 }, { ENTRY_NONE, 0 }, { ENTRY_NONE, 0 }, { ENTRY_NONE, 0 },
			{ ENTRY_NONE, 0 }, { ENTRY_NONE, 0 }, { ENTRY_NONE, 0 }, { ENTRY_NONE, 0 } }; // (as a new world's: empty)
		static bool hotbarLoaded = false;
		static int selectedSlot = 0;
		static const int KEYS[9] = { KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9 };

		static void LoadHotbar()
		{
			if (hotbarLoaded || !zoptions)
				return;
			hotbarLoaded = true;
			zSTRING text = zoptions->ReadString(zSTRING("MINECRAFT"), "hotbar", "");
			const char* p = text.ToChar();
			for (int i = 0; i < 9 && p && *p; i++)
			{
				while (*p == ' ')
					p++;
				char kind = *p;
				int value = atoi(p + 1);
				if (kind == 'g' && value >= 0 && value < 9)
					hotbar[i] = { ENTRY_GOTHIC, value };
				else if (kind == 'b' && value >= 0 && value < blockTypeCount)
					hotbar[i] = { ENTRY_BLOCK, value };
				else if (kind == 'e' && value >= 0 && value < EGGS)
					hotbar[i] = { ENTRY_EGG, value };
				else if (kind == 'i' && value >= 0 && value < MC_ITEMS)
					hotbar[i] = { ENTRY_ITEM, value };
				else if (kind == 'n')
					hotbar[i] = { ENTRY_NONE, 0 };
				while (*p && *p != ' ')
					p++;
			}
		}

		static void SaveHotbar()
		{
			char text[96] = "";
			for (int i = 0; i < 9; i++)
			{
				char one[8];
				sprintf_s(one, "%s%c%d", i ? " " : "", "ngbei"[hotbar[i].kind], hotbar[i].value);
				strcat_s(text, one);
			}
			if (zoptions)
				zoptions->WriteString(zSTRING("MINECRAFT"), "hotbar", zSTRING(text), TRUE);
		}

		// What Gothic's number key `key` (0 for 1, ...) holds: the melee weapon, the ranged one, a spell.
		static oCItem* GothicItem(oCNpc* hero, int key)
		{
			if (key == 0)
				return hero->GetEquippedMeleeWeapon();
			if (key == 1)
				return hero->GetEquippedRangedWeapon();
			oCMag_Book* book = hero->GetSpellBook();
			return book ? book->GetSpellItemByKey(key + 1) : nullptr;
		}

		// the number key (0 for 1, ...) of what he holds: the drawn weapon or spell, -1 with nothing drawn
		static int HeldKey(oCNpc* hero)
		{
			int mode = hero->GetWeaponMode();
			if (mode == NPC_WEAPON_FIST || mode == NPC_WEAPON_1HS || mode == NPC_WEAPON_2HS || mode == NPC_WEAPON_DAG)
				return 0;
			if (mode == NPC_WEAPON_BOW || mode == NPC_WEAPON_CBOW)
				return 1;
			if (mode == NPC_WEAPON_MAG)
				if (oCMag_Book* book = hero->GetSpellBook())
				{
					int nr = book->GetSelectedSpellNr(); // (GetSpellItem checks only the upper end)
					oCItem* item = nr >= 0 ? book->GetSpellItem(nr) : nullptr;
					int key = item ? book->GetKeyByItem(item) : -1;
					if (key >= 3 && key <= 9)
						return key - 1;
				}
			return -1;
		}

		// the slot of what he holds (the selected one if it has it), -1 if no slot has it
		static int HeldSlot(oCNpc* hero)
		{
			int key = HeldKey(hero);
			if (key < 0)
				return -1;
			if (hotbar[selectedSlot].kind == ENTRY_GOTHIC && hotbar[selectedSlot].value == key)
				return selectedSlot;
			for (int i = 0; i < 9; i++)
				if (hotbar[i].kind == ENTRY_GOTHIC && hotbar[i].value == key)
					return i;
			return -1;
		}

		static oCItem* EntryItem(oCNpc* hero, const Entry& e) { return e.kind == ENTRY_GOTHIC ? GothicItem(hero, e.value) : nullptr; }

		static void EntryName(oCNpc* hero, const Entry& e, char* out, size_t size)
		{
			out[0] = 0;
			if (e.kind == ENTRY_GOTHIC)
			{
				if (oCItem* item = GothicItem(hero, e.value))
					strncpy_s(out, size, item->name.ToChar(), _TRUNCATE);
			}
			else if (e.kind == ENTRY_BLOCK && e.value < blockTypeCount)
				strncpy_s(out, size, blockTypes[e.value].name, _TRUNCATE);
			else if (e.kind == ENTRY_EGG && e.value < EGGS)
				strncpy_s(out, size, eggNames[e.value], _TRUNCATE);
			else if (e.kind == ENTRY_ITEM && e.value < MC_ITEMS)
				strncpy_s(out, size, itemNames[e.value], _TRUNCATE);
		}

		// Minecraft's item for one of Gothic's (by kind, a weapon's tier by its damage)
		static const char* Icon(oCItem* item, zCOLOR& tint, bool& overlay)
		{
			overlay = false;
			char name[96];
			strncpy_s(name, item->GetInstanceName().ToChar(), _TRUNCATE);
			_strupr_s(name);
			int kind = item->mainflag, damage = item->damageTotal;
			if (kind & 2) // ITEM_KAT_NF
			{
				static const char* const SWORDS[] = { "item/wooden_sword", "item/stone_sword", "item/iron_sword", "item/diamond_sword", "item/netherite_sword" };
				static const char* const AXES[] = { "item/wooden_axe", "item/stone_axe", "item/iron_axe", "item/diamond_axe", "item/netherite_axe" };
				int tier = damage < 20 ? 0 : (damage < 35 ? 1 : (damage < 55 ? 2 : (damage < 80 ? 3 : 4)));
				bool twoHanded = (item->flags & ((1 << 16) | (1 << 17))) != 0; // ITEM_2HD_SWD, ITEM_2HD_AXE
				bool axe = (item->flags & ((1 << 15) | (1 << 17))) != 0;         // ITEM_AXE, ITEM_2HD_AXE
				return (twoHanded || axe) ? AXES[tier] : SWORDS[tier];
			}
			if (kind & 4) // ITEM_KAT_FF
				return (item->flags & (1 << 20)) ? "item/crossbow_standby" : "item/bow";
			if (kind & 8)
				return "item/arrow";
			if (kind & 512) // ITEM_KAT_RUNE: a rune, or a scroll
				return strncmp(name, "ITSC", 4) == 0 ? "item/paper" : "item/enchanted_book";
			if (kind & 128) // ITEM_KAT_POTIONS: the bottle and its colour
			{
				overlay = true;
				tint = strstr(name, "MANA") ? Argb(255, 56, 93, 198) : (strstr(name, "HEAL") || strstr(name, "HEALTH") ? Argb(255, 248, 36, 35)
					: Argb(255, 124, 175, 198));
				return "item/potion";
			}
			if (kind & 32) // ITEM_KAT_FOOD
			{
				struct { const char* part; const char* icon; } table[] = {
					{ "APPLE", "item/apple" }, { "BREAD", "item/bread" }, { "LOAF", "item/bread" }, { "RAW", "item/beef" },
					{ "MUTTON", "item/cooked_beef" }, { "MEAT", "item/cooked_beef" }, { "HAM", "item/cooked_beef" }, { "BACON", "item/cooked_beef" },
					{ "SAUSAGE", "item/cooked_beef" }, { "FISH", "item/cooked_cod" }, { "SOUP", "item/mushroom_stew" }, { "STEW", "item/rabbit_stew" },
					{ "RAGOUT", "item/rabbit_stew" }, { "RICE", "item/mushroom_stew" }, { "HONEY", "item/honey_bottle" }, { "BERR", "item/sweet_berries" },
					{ "MUSHROOM", "item/red_mushroom" }, { "PILZ", "item/red_mushroom" }, { "CHEESE", "item/cookie" } };
				for (auto& t : table)
					if (strstr(name, t.part))
						return t.icon;
				return "item/bread";
			}
			if (kind & 256)
				return "item/torch";
			return "item/stick";
		}

		static void DrawItem(oCItem* item, float x, float y)
		{
			zCOLOR tint = Argb(255, 255, 255, 255);
			bool overlay = false;
			const char* icon = Icon(item, tint, overlay);
			if (overlay) // the potion's liquid in its colour, the bottle over it
				Blit("item/potion_overlay", x, y, 1.0f, tint);
			Blit(icon, x, y);
			if (item->amount > 1) // ItemRenderer's count: white, right-aligned at the slot's bottom
			{
				char count[16];
				sprintf_s(count, "%d", item->amount);
				Text(count, x + 19.0f - 2.0f - TextWidth(count), y + 6.0f + 3.0f, Argb(255, 255, 255, 255));
			}
		}

		static void DrawEntry(oCNpc* hero, const Entry& e, float x, float y)
		{
			char sprite[64];
			if (e.kind == ENTRY_GOTHIC)
			{
				if (oCItem* item = GothicItem(hero, e.value))
					DrawItem(item, x, y);
			}
			else if (e.kind == ENTRY_BLOCK && e.value < blockTypeCount)
				DrawBlockIcon(blockTypes[e.value].id, x, y);
			else if (e.kind == ENTRY_EGG && e.value < EGGS)
			{
				sprintf_s(sprite, "item/%s_spawn_egg", eggIds[e.value]);
				Blit(sprite, x, y);
			}
			else if (e.kind == ENTRY_ITEM && e.value < MC_ITEMS)
			{
				sprintf_s(sprite, "item/%s", itemIds[e.value]);
				Blit(sprite, x, y);
			}
		}

		// The wheel and the number keys choose a slot. A slot with Gothic's key presses that key (not
		// when he holds it already, which would put it away); a block, an egg or an empty Gothic key
		// put away what he holds (Minecraft's hand holds the slot's thing only).
		static int PressGothicKey(int key); // Gothic's own key handling (oCGame::HandleEvent), after its hook
		static int toolHighlight = 0;
		static char toolName[96] = "";
		static Entry toolEntry = { ENTRY_NONE, 0 };
		static oCItem* toolItem = nullptr;

		static void ShowToolName(oCNpc* hero)
		{
			toolEntry = hotbar[selectedSlot];
			toolItem = EntryItem(hero, toolEntry);
			EntryName(hero, toolEntry, toolName, sizeof toolName);
			toolHighlight = toolName[0] ? 40 : 0;
		}

		static void ChooseSlot(oCNpc* hero, int slot)
		{
			slot = (slot % 9 + 9) % 9;
			selectedSlot = slot;
			ShowToolName(hero);
			if (!ogame)
				return;
			const Entry& e = hotbar[slot];
			int held = HeldKey(hero);
			step = "hotbar: Gothic's key";
			if (e.kind == ENTRY_GOTHIC && GothicItem(hero, e.value))
			{
				Log("slot %d: Gothic's key %d (%s), held %d", slot + 1, e.value + 1, toolName, held + 1);
				if (held != e.value)
					PressGothicKey(KEYS[e.value]);
			}
			else
			{
				Log("slot %d: %s, held %d%s", slot + 1, toolName[0] ? toolName : "empty", held + 1, held >= 0 ? ": put away" : "");
				if (held >= 0)
					PressGothicKey(KEYS[held]);
			}
			step = "hud";
		}

		// --- per tick: blinking hearts, the item name's timer ---
		static int hudTicks = 0;
		static float hudClock = 0.0f;
		static int lastHealth = -1, displayHealth = 0, healthBlinkTime = 0;
		static float lastHealthTime = 0.0f;
		static unsigned int seed = 1;
		static int Random(int n)
		{
			seed = seed * 1103515245u + 12345u;
			return (int)((seed >> 16) % (unsigned int)n);
		}

		static void HudTick(oCNpc* hero)
		{
			hudTicks++;
			if (toolHighlight > 0)
				toolHighlight--;
			const Entry& e = hotbar[selectedSlot];
			if (!Same(e, toolEntry) || EntryItem(hero, e) != toolItem) // what the selected slot holds changed
				ShowToolName(hero);
		}

		static void DrawHearts(oCNpc* hero)
		{
			int current = (int)ceilf(McHealth(hero));
			if (lastHealth >= 0 && current < lastHealth)
				healthBlinkTime = hudTicks + 20, lastHealthTime = Now();
			else if (lastHealth >= 0 && current > lastHealth)
				healthBlinkTime = hudTicks + 10, lastHealthTime = Now();
			if (Now() - lastHealthTime > 1.0f)
			{
				displayHealth = current;
				lastHealthTime = Now();
			}
			lastHealth = current;
			bool blink = healthBlinkTime > hudTicks && (healthBlinkTime - hudTicks) / 3 % 2 == 1;
			seed = (unsigned int)hudTicks * 312871u;
			float left = (float)(guiW / 2 - 91), right = (float)(guiW / 2 + 91), base = guiH - 39.0f;
			// armour
			int armor = (int)(hero->protection[oEDamageIndex_Edge] / 5.0f + 0.5f);
			armor = armor > 20 ? 20 : (armor < 0 ? 0 : armor);
			if (armor > 0)
				for (int i = 0; i < 10; i++)
					Blit(i * 2 + 1 < armor ? "armor_full" : (i * 2 + 1 == armor ? "armor_half" : "armor_empty"), left + i * 8, base - 10.0f);
			// hearts, from the last
			for (int i = 9; i >= 0; i--)
			{
				float x = left + i * 8, y = base;
				if (current <= 4)
					y += (float)Random(2);
				Blit(blink ? "heart/container_blinking" : "heart/container", x, y);
				int halves = i * 2;
				if (blink && halves < displayHealth)
					Blit(halves + 1 == displayHealth ? "heart/half_blinking" : "heart/full_blinking", x, y);
				if (halves < current)
					Blit(halves + 1 == current ? "heart/half" : "heart/full", x, y);
			}
			// food
			for (int i = 0; i < 10; i++)
			{
				float y = base;
				if (saturation <= 0.0f && hudTicks % (foodLevel * 3 + 1) == 0)
					y += (float)(Random(3) - 1);
				float x = right - i * 8 - 9.0f;
				Blit("food_empty", x, y);
				if (i * 2 + 1 < foodLevel)
					Blit("food_full", x, y);
				if (i * 2 + 1 == foodLevel)
					Blit("food_half", x, y);
			}
			// air while diving (Gothic's dive time as Minecraft's 300 ticks)
			bool diving = BodyState(hero) == BS_DIVE;
			if (hero->divetime > 0.0f && (diving || hero->divectr < hero->divetime))
			{
				int max = 300, air = (int)(300.0f * hero->divectr / hero->divetime);
				air = air < 0 ? 0 : (air > max ? max : air);
				auto bubble = [&](int offset) { return (int)ceilf((air + offset) * 10.0f / max); };
				int full = bubble(-2), popping = bubble(0), empty = 10 - bubble(air != 0 && diving ? 1 : 0);
				for (int b = 1; b <= 10; b++)
				{
					float x = right - (b - 1) * 8 - 9.0f, y = base - 10.0f;
					if (b <= full)
						Blit("air", x, y);
					else if (full != popping && b == popping && diving)
						Blit("air_bursting", x, y);
					else if (b > 10 - empty)
						Blit("air_empty", x, y + (empty == 10 && hudTicks % 2 == 0 ? (float)Random(2) : 0.0f));
				}
			}
		}

		static void DrawExperience(oCNpc* hero)
		{
			int level = hero->level;
			float next = (float)hero->experience_points_next_level, step = 500.0f * (level + 1);
			float progress = step > 0.0f ? ((float)hero->experience_points - (next - step)) / step : 0.0f;
			progress = progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
			float left = (float)((guiW - 182) / 2), top = guiH - 24.0f - 5.0f;
			Blit("experience_bar_background", left, top);
			int filled = (int)(progress * 183.0f);
			if (filled > 0)
				Blit("experience_bar_progress", left, top, filled / 182.0f);
			if (level > 0) // ContextualBar.extractExperienceLevel: green on a black outline
			{
				char text[16];
				sprintf_s(text, "%d", level);
				float x = floorf((guiW - TextWidth(text)) / 2.0f), y = guiH - 24.0f - 9.0f - 2.0f;
				zCOLOR black = Argb(255, 0, 0, 0);
				Text(text, x + 1.0f, y, black, false);
				Text(text, x - 1.0f, y, black, false);
				Text(text, x, y + 1.0f, black, false);
				Text(text, x, y - 1.0f, black, false);
				Text(text, x, y, Argb(255, 0x80, 0xFF, 0x20), false);
			}
		}

		static void DrawHotbar(oCNpc* hero)
		{
			float centre = (float)(guiW / 2);
			int held = HeldSlot(hero);
			if (held >= 0)
				selectedSlot = held;
			Blit("hotbar", centre - 91.0f, guiH - 22.0f);
			Blit("hotbar_selection", centre - 91.0f - 1.0f + selectedSlot * 20.0f, guiH - 22.0f - 1.0f);
			for (int i = 0; i < 9; i++)
				DrawEntry(hero, hotbar[i], centre - 90.0f + i * 20.0f + 2.0f, guiH - 16.0f - 3.0f);
			if (toolHighlight > 0 && toolName[0]) // Hud.extractSelectedItemName
			{
				int alpha = (int)(toolHighlight * 256.0f / 10.0f);
				alpha = alpha > 255 ? 255 : alpha;
				float width = TextWidth(toolName);
				Text(toolName, floorf((guiW - width) / 2.0f), guiH - 59.0f, Argb(alpha, 255, 255, 255));
			}
		}

		static void DrawCrosshair(oCNpc* hero)
		{
			if (view != FIRST_PERSON)
				return;
			Blit("crosshair", (float)((guiW - 15) / 2), (float)((guiH - 15) / 2));
			int mode = hero->GetWeaponMode();
			if (mode == NPC_WEAPON_BOW || mode == NPC_WEAPON_CBOW || mode == NPC_WEAPON_MAG)
				return;
			float speed = mode == NPC_WEAPON_1HS ? 1.6f : (mode == NPC_WEAPON_2HS ? 1.0f : 4.0f);
			float delay = 20.0f / speed;
			float charge = (attackStrength + TickShare()) / delay;
			charge = charge > 1.0f ? 1.0f : charge;
			float x = (float)(guiW / 2 - 8), y = (float)(guiH / 2 - 7 + 16);
			zVEC3 point;
			if (charge >= 1.0f && delay > 5.0f && Target(hero, 300.0f, point))
				Blit("crosshair_attack_indicator_full", x, y);
			else if (charge < 1.0f)
			{
				Blit("crosshair_attack_indicator_background", x, y);
				Blit("crosshair_attack_indicator_progress", x, y, (int)(charge * 17.0f) / 16.0f);
			}
		}

		static bool HasOverlay(zCModel* model, const char* name)
		{
			for (int i = 1; model && i < model->modelProtoList.GetNum(); i++)
				if (zCModelPrototype* proto = model->modelProtoList[i])
				{
					zSTRING n = proto->modelProtoName;
					n.Upper();
					if (n == name)
						return true;
				}
			return false;
		}

		static void DrawEffects(oCNpc* hero)
		{
			if (!HasOverlay(hero->GetModel(), "STEVE_SPRINT")) // Gothic's speed potion
				return;
			float x = guiW - 25.0f, y = 1.0f;
			Blit("effect_background", x, y);
			Blit("effect/speed", x + 3.0f, y + 3.0f);
		}

		// Hud.extractVignette: darker towards the edges, more at night (Gothic's hour)
		static void DrawVignette()
		{
			if (!settings.vignette || !vignetteTexture)
				return;
			int hour = 12, minute = 0;
			if (oCWorldTimer* timer = ogame->GetWorldTimer())
				timer->GetTime(hour, minute);
			float h = hour + minute / 60.0f;
			float night = h < 5.0f || h > 21.0f ? 1.0f : (h < 7.0f ? (7.0f - h) / 2.0f : (h > 19.0f ? (h - 19.0f) / 2.0f : 0.0f));
			int alpha = (int)(255.0f * (0.25f + 0.35f * night));
			Quad(vignetteTexture, 0.0f, 0.0f, (float)guiW, (float)guiH, 0.0f, 0.0f, 1.0f, 1.0f, Argb(alpha, 0, 0, 0));
		}

		static bool HudVisible(oCNpc* hero)
		{
			if (!settings.hud || !hero || !ogame || !screen || !zrenderer || !ogame->GetShowPlayerStatus() || !IsSteve(hero) || hero->IsDead())
				return false;
			if (zCAICamera* ai = zCAICamera::current)
			{
				zSTRING mode = ai->curcammode;
				mode.Upper();
				if (mode == "CAMMODDIALOG")
					return false;
			}
			return true;
		}

		static void Fill(float x, float y, float w, float h, const zCOLOR& color); // (the inventory's)

		static void DrawHud()
		{
			oCNpc* hero = oCNpc::player;
			if (!HudVisible(hero))
				return;
			LoadHud();
			if (!hudReady)
				return;
			int width = zrenderer->vid_xdim, height = zrenderer->vid_ydim;
			if (width <= 0 || height <= 0)
				return;
			int scale = 1, most = settings.guiScale > 0 ? settings.guiScale : 100000;
			while (scale != most && scale < width && scale < height && width / (scale + 1) >= 320 && height / (scale + 1) >= 240)
				scale++;
			gui = (float)scale;
			guiW = (width + scale - 1) / scale;
			guiH = (height + scale - 1) / scale;
			hudClock += ztimer->frameTimeFloat / 1000.0f;
			for (int n = 0; hudClock >= TICK && n < 5; n++, hudClock -= TICK)
				HudTick(hero);
			if (hudClock >= TICK)
				hudClock = 0.0f;
			if (wheelSteps != 0) // Minecraft: scrolling up goes to the slot on the left
			{
				ChooseSlot(hero, selectedSlot + (wheelSteps > 0 ? -1 : 1));
				wheelSteps = 0;
			}

			// the 2D state zCView::Blit draws in (as the GTA mashup's radar), no filtering
			int zWrite = zrenderer->GetZBufferWriteEnabled();
			zTRnd_ZBufferCmp zCompare = zrenderer->GetZBufferCompare();
			zTRnd_AlphaBlendFunc blend = zrenderer->GetAlphaBlendFunc();
			int bilerp = zrenderer->GetBilerpFilterEnabled(), wrap = zrenderer->GetTextureWrapEnabled(), fog = zrenderer->GetFog();
			zTRnd_ZBufferCmp always = zRND_ZBUFFER_CMP_ALWAYS;
			zTRnd_AlphaBlendFunc alphaBlend = zRND_ALPHA_FUNC_BLEND;
			zrenderer->SetViewport(0, 0, width, height);
			zrenderer->SetZBufferWriteEnabled(FALSE);
			zrenderer->SetZBufferCompare(always);
			zrenderer->SetAlphaBlendFunc(alphaBlend);
			zrenderer->SetTextureWrapEnabled(FALSE);
			zrenderer->SetFog(FALSE);
			zrenderer->SetBilerpFilterEnabled(TRUE);
			DrawVignette();
			zrenderer->SetBilerpFilterEnabled(FALSE);
			if (view == FIRST_PERSON && HeroBurning(hero))
				if (zCTexture* flame = FireOverlayFrame()) // two of fire_1's flames over the screen's bottom
				{
					float w = guiW * 0.62f, h = guiH * 0.7f, y = guiH - h * 0.72f;
					Quad(flame, -guiW * 0.06f, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, Argb(230, 255, 255, 255));
					Quad(flame, guiW * 0.44f, y, w, h, 1.0f, 0.0f, 0.0f, 1.0f, Argb(230, 255, 255, 255));
				}
			if (view == FIRST_PERSON && EyesInLava(hero)) // FogRenderer: lava's (0.6, 0.1, 0), seen 0.25 blocks
				Fill(0.0f, 0.0f, (float)guiW, (float)guiH, Argb(235, 153, 26, 0));
			DrawCrosshair(hero);
			DrawHotbar(hero);
			DrawHearts(hero);
			DrawExperience(hero);
			DrawEffects(hero);
			DrawBossBar();
			if (inventoryOpen)
				DrawInventory(hero);
			zrenderer->SetZBufferWriteEnabled(zWrite);
			zrenderer->SetZBufferCompare(zCompare);
			zrenderer->SetAlphaBlendFunc(blend);
			zrenderer->SetBilerpFilterEnabled(bilerp);
			zrenderer->SetTextureWrapEnabled(wrap);
			zrenderer->SetFog(fog);
		}

		// --- Minecraft's blocks in Gothic's world -------------------------------------------------------
		// make_mc_blocks.py's cubes of a metre (MC_BLK_<ID>.3DS) on a grid of metres from the world's
		// origin, as objects of their own (vobs named MCBLOCK) with Gothic's collision, so he walks into
		// them and stands on them; they are saved with the game and found again by name after a load.
		// The right button with a block puts it against the face or the ground the crosshair points at
		// within 4.5 m (Minecraft's reach), where it doesn't take his own place; the left one breaks the
		// block it points at (at once, as in creative mode). Minecraft's sounds for both
		// (make_mc_sounds.py). Gothic's world itself stays as it is.
		static std::unordered_map<long long, zCVob*> blocks;
		static std::unordered_map<long long, int> blockKinds;
		static zCWorld* blocksWorld = nullptr;
		static const float CELL = 100.0f, REACH = 450.0f;

		static long long CellKey(int x, int y, int z)
		{
			return ((long long)(x & 0x1FFFFF) << 42) | ((long long)(y & 0x1FFFFF) << 21) | (long long)(z & 0x1FFFFF);
		}

		static void CellOf(const zVEC3& p, int& x, int& y, int& z)
		{
			x = (int)floorf(p[0] / CELL);
			y = (int)floorf(p[1] / CELL);
			z = (int)floorf(p[2] / CELL);
		}

		static zVEC3 CellCentre(int x, int y, int z) { return zVEC3((x + 0.5f) * CELL, (y + 0.5f) * CELL, (z + 0.5f) * CELL); }

		static void BlockVisual(int kind, char* out, size_t size, bool hand)
		{
			char id[32];
			strcpy_s(id, blockTypes[kind].id);
			_strupr_s(id);
			sprintf_s(out, size, "MC_BLK_%s%s.3DS", id, hand ? "_HAND" : "");
		}

		static void PlaySound(const char* name, zCVob* source)
		{
			if (!zsound || !source)
				return;
			zCSoundFX* fx = zsound->LoadSoundFX(zSTRING(name));
			if (!fx)
				return;
			zCSoundSystem::zTSound3DParams params;
			params.SetDefaults();
			params.radius = 2500.0f;
			zsound->PlaySound3D(fx, source, zSND_SLOT_NONE, &params);
		}

		// Minecraft's SoundType: dig/<kind>1-4 (glass breaking: random/glass1-3)
		static void BlockSound(int kind, zCVob* source, bool breaking)
		{
			char name[48];
			const char* sound = blockTypes[kind].sound;
			if (breaking && !strcmp(sound, "glass"))
				sprintf_s(name, "MC_GLASS%d.WAV", 1 + rand() % 3);
			else
			{
				char upper[16];
				strcpy_s(upper, strcmp(sound, "glass") ? sound : "stone");
				_strupr_s(upper);
				sprintf_s(name, "MC_DIG_%s%d.WAV", upper, 1 + rand() % 4);
			}
			PlaySound(name, source);
		}

		// the world changed (a load, another level): the blocks it holds, found by their name
		static std::unordered_map<long long, char> gothicSolid; // (Minecraft's grid over Gothic's world, further down)

		static void FindBlocks(zCWorld* world)
		{
			blocks.clear();
			blockKinds.clear();
			gothicSolid.clear();
			blocksWorld = world;
			if (!world)
				return;
			zCArray<zCVob*> found;
			world->SearchVobListByName(zSTRING("MCBLOCK"), found);
			for (int i = 0; i < found.GetNum(); i++)
			{
				zCVob* vob = found[i];
				zCVisual* visual = vob ? vob->GetVisual() : nullptr;
				if (!visual)
					continue;
				zSTRING name = visual->GetVisualName();
				name.Upper();
				for (int kind = 0; kind < blockTypeCount; kind++)
				{
					char want[64];
					BlockVisual(kind, want, sizeof want, false);
					if (name == want)
					{
						int x, y, z;
						CellOf(vob->GetPositionWorld(), x, y, z);
						blocks[CellKey(x, y, z)] = vob;
						KeepDetail(visual);
						blockKinds[CellKey(x, y, z)] = kind;
						break;
					}
				}
			}
			Log("blocks: %d in this world", (int)blocks.size());
		}

		struct Pick
		{
			bool hit = false, block = false;
			int x = 0, y = 0, z = 0;       // the block's cell
			zVEC3 point, normal;           // where the ray hit, the face's or ground's normal
			float distance = 0.0f;
		};

		// What the crosshair points at within `reach`: Gothic's world (its polygons) or a block, the
		// nearer (the blocks walked through on their grid: Amanatides and Woo's traversal).
		static Pick PickBlock(oCNpc* hero, float reach)
		{
			Pick best;
			best.distance = reach;
			zVEC3 from = Eyes(hero), dir = Look();
			zCWorld* world = hero->GetHomeWorld();
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE | zTRACERAY_POLY_NORMAL | zTRACERAY_POLY_IGNORE_TRANSP;
			if (world && world->TraceRayNearestHit(from, dir * reach, hero, flags) && world->traceRayReport.foundHit)
			{
				best.hit = true;
				best.point = world->traceRayReport.foundIntersection;
				best.normal = world->traceRayReport.foundPolyNormal;
				best.distance = (best.point - from).Length();
			}
			int x, y, z;
			CellOf(from, x, y, z);
			int step[3];
			float next[3], delta[3];
			int cell[3] = { x, y, z };
			for (int i = 0; i < 3; i++)
			{
				step[i] = dir[i] > 0.0f ? 1 : -1;
				float edge = (cell[i] + (dir[i] > 0.0f ? 1 : 0)) * CELL;
				delta[i] = fabsf(dir[i]) > 1e-6f ? CELL / fabsf(dir[i]) : 1e30f;
				next[i] = fabsf(dir[i]) > 1e-6f ? (edge - from[i]) / dir[i] : 1e30f;
			}
			for (int n = 0; n < 32; n++)
			{
				int axis = next[0] < next[1] ? (next[0] < next[2] ? 0 : 2) : (next[1] < next[2] ? 1 : 2);
				float t = next[axis];
				if (t > best.distance)
					break;
				cell[axis] += step[axis];
				next[axis] += delta[axis];
				if (blocks.count(CellKey(cell[0], cell[1], cell[2])))
				{
					best.hit = best.block = true;
					best.x = cell[0];
					best.y = cell[1];
					best.z = cell[2];
					best.distance = t;
					best.point = from + dir * t;
					best.normal = zVEC3(axis == 0 ? (float)-step[0] : 0.0f, axis == 1 ? (float)-step[1] : 0.0f, axis == 2 ? (float)-step[2] : 0.0f);
					break;
				}
			}
			return best;
		}

		static void CellChanged(int x, int y, int z);
		static void CellFromKey(long long key, int& x, int& y, int& z);
		static bool GothicSolid(int x, int y, int z);
		static void RemoveFire(long long key);
		static void RemoveLava(long long key);
		static bool TargetCell(oCNpc* hero, int& x, int& y, int& z, Pick& p);
		struct FallCheck
		{
			long long key;
			int ticks;
		};
		static std::vector<FallCheck> fallChecks;

		// a cell he (or anyone) stands in takes no block (Minecraft's placement check)
		static bool CellFree(oCNpc* hero, int x, int y, int z)
		{
			if (blocks.count(CellKey(x, y, z)))
				return false;
			zVEC3 lo(x * CELL, y * CELL, z * CELL), hi = lo + zVEC3(CELL, CELL, CELL);
			oCWorld* game = ogame ? ogame->GetGameWorld() : nullptr;
			for (zCListSort<oCNpc>* node = game && game->voblist_npcs ? game->voblist_npcs->next : nullptr; node; node = node->next)
			{
				oCNpc* npc = node->data;
				if (!npc || npc->IsDead())
					continue;
				zTBBox3D box = npc->bbox3D;
				zVEC3 centre = (box.mins + box.maxs) * 0.5f;
				for (int i = 0; i < 3; i += 2) // a body is narrower than its box (the arms)
				{
					box.mins[i] = centre[i] + (box.mins[i] - centre[i]) * 0.5f;
					box.maxs[i] = centre[i] + (box.maxs[i] - centre[i]) * 0.5f;
				}
				if (box.mins[0] < hi[0] && box.maxs[0] > lo[0] && box.mins[1] < hi[1] && box.maxs[1] > lo[1]
					&& box.mins[2] < hi[2] && box.maxs[2] > lo[2])
					return false;
			}
			return true;
		}

		// a block in a cell (it takes the place of fire or lava there)
		static zCVob* PutBlock(int kind, int x, int y, int z)
		{
			oCWorld* world = ogame ? ogame->GetGameWorld() : nullptr;
			if (!world)
				return nullptr;
			long long key = CellKey(x, y, z);
			RemoveFire(key);
			RemoveLava(key);
			char visual[64];
			BlockVisual(kind, visual, sizeof visual, false);
			zCVob* vob = new zCVob();
			vob->SetVobName(zSTRING("MCBLOCK"));
			vob->SetVisual(zSTRING(visual));
			KeepDetail(vob->GetVisual());
			vob->SetPositionWorld(CellCentre(x, y, z));
			world->AddVob(vob);
			vob->SetCollDetStat(TRUE); // after AddVob, which sets it as it likes
			vob->SetCollDetDyn(TRUE);
			blocks[key] = vob;
			blockKinds[key] = kind;
			vob->Release(); // the world keeps it
			CellChanged(x, y, z);
			fallChecks.push_back({ key, 2 }); // FallingBlock.onPlace
			return vob;
		}

		// On a block's side, or on Gothic's ground: the cell out of the ground on Minecraft's grid
		// (TargetCell), so a block stands on the grid's ground, not sunk into it.
		static void PlaceBlock(oCNpc* hero, int kind)
		{
			int x, y, z;
			Pick p;
			if (!TargetCell(hero, x, y, z, p) || !CellFree(hero, x, y, z))
				return;
			zCVob* vob = PutBlock(kind, x, y, z);
			if (!vob)
				return;
			BlockSound(kind, vob, false);
			swingTicks = 0;
			Log("block: %s at %d %d %d", blockTypes[kind].id, x, y, z);
		}

		static void RemoveBlock(long long key, bool sound)
		{
			auto it = blocks.find(key);
			if (it == blocks.end())
				return;
			zCVob* vob = it->second;
			int kind = blockKinds[key];
			blocks.erase(it);
			blockKinds.erase(key);
			if (sound)
				BlockSound(kind, vob, true);
			if (zCWorld* world = vob->GetHomeWorld())
				world->RemoveVob(vob);
			int x, y, z;
			CellFromKey(key, x, y, z);
			CellChanged(x, y, z);
		}

		// A left click at a block nearer than whoever the crosshair points at breaks it.
		static bool BreakTarget(oCNpc* hero, float npcDistance)
		{
			Pick p = PickBlock(hero, REACH);
			if (!p.block || p.distance >= npcDistance)
				return false;
			RemoveBlock(CellKey(p.x, p.y, p.z), true);
			swingTicks = 0;
			return true;
		}

		// the block, the bow (as far as it is drawn: Minecraft's bow_pulling_0 to 2 from 0.65 and 0.9 of
		// 20 ticks), the arrow, flint and steel or the egg he holds in his right hand while its slot is
		// chosen and no weapon is out (seen behind or in front of him)
		static zCVisual* heldVisual = nullptr;
		static const char* const ITEM_VISUALS[] = { "MC_ITEM_BOW.3DS", "MC_ITEM_BOW_PULLING_0.3DS", "MC_ITEM_BOW_PULLING_1.3DS",
			"MC_ITEM_BOW_PULLING_2.3DS", "MC_ITEM_ARROW.3DS", "MC_ITEM_FLINT_AND_STEEL.3DS", "MC_ITEM_ZOMBIE_SPAWN_EGG.3DS",
			"MC_ITEM_SKELETON_SPAWN_EGG.3DS", "MC_ITEM_CREEPER_SPAWN_EGG.3DS", "MC_ITEM_ENDER_DRAGON_SPAWN_EGG.3DS", "MC_ITEM_LAVA_BUCKET.3DS",
			"MC_ITEM_BUCKET.3DS" };
		static zCVisual* itemVisuals[12] = {};

		static zCVisual* ItemVisual(int i)
		{
			if (!itemVisuals[i])
			{
				itemVisuals[i] = zCVisual::LoadVisual(zSTRING(ITEM_VISUALS[i])); // kept
				KeepDetail(itemVisuals[i]);
			}
			return itemVisuals[i];
		}

		static void UpdateHeldBlock(oCNpc* hero)
		{
			zCModel* model = hero->GetModel();
			zCModelNodeInst* node = model ? model->SearchNode(zSTRING("ZS_RIGHTHAND")) : nullptr;
			if (!node)
				return;
			const Entry& e = hotbar[selectedSlot];
			zCVisual* want = nullptr;
			if (e.kind == ENTRY_BLOCK && e.value < blockTypeCount && hero->GetWeaponMode() == NPC_WEAPON_NONE)
			{
				BlockType& b = blockTypes[e.value];
				if (!b.hand)
				{
					char visual[64];
					BlockVisual(e.value, visual, sizeof visual, true);
					b.hand = zCVisual::LoadVisual(zSTRING(visual)); // kept
					KeepDetail(b.hand);
				}
				want = b.hand;
			}
			else if (e.kind == ENTRY_ITEM && hero->GetWeaponMode() == NPC_WEAPON_NONE)
			{
				int pull = useTicks < 0 ? 0 : (useTicks >= 18 ? 3 : (useTicks >= 13 ? 2 : 1));
				static const int VISUAL[MC_ITEMS] = { 0, 4, 5, 10, 11 }; // the bow's by its pull
				want = ItemVisual(e.value == ITEM_BOW ? pull : VISUAL[e.value]);
			}
			else if (e.kind == ENTRY_EGG && e.value < EGGS && hero->GetWeaponMode() == NPC_WEAPON_NONE)
				want = ItemVisual(6 + e.value);
			if (node->nodeVisual == want || (!want && node->nodeVisual != heldVisual))
				return;
			model->SetNodeVisual(node, want, FALSE);
			heldVisual = want;
		}

		// --- Minecraft's mobs ---------------------------------------------------------------------------
		// Gothic's zombies and skeletons look like Minecraft's (mobs=1): their scripts give them the
		// bodies ZOM_BODY and SKE_BODY (oCNpc::SetAdditionalVisuals); in their place go Steve's skeleton
		// and animations (STEVE.MDS: oCNpc::mds_name, from which Gothic builds the model) with
		// make_steve.py's zombie or skeleton body, and the zombie gets its raised arms (STEVE_ZOMBIE.MDS).
		// The spawn eggs bring Gothic's zombie and skeleton, and the creeper: Gothic's zombie (its AI
		// walks at him) with make_steve.py's creeper (MCCREEPER.MDS). A creeper within 3 blocks of him
		// swells for 30 ticks (Creeper: swell, 7 blocks away it shrinks back) and explodes with
		// Minecraft's power 3: everyone within 6 blocks takes ((1 - d / 6)^2 + (1 - d / 6)) / 2 x 42 + 1 of
		// Minecraft's 20 health (Explosion: d the distance in blocks; as that share of theirs) and is
		// thrown, the blocks within 3 go (but obsidian; Explode), with Gothic's fire and smoke and
		// Minecraft's bang. It swells as Minecraft draws it (CreeperRenderer.scale).
		enum { MOB_NONE, MOB_ZOMBIE, MOB_SKELETON, MOB_CREEPER };
		static bool creeperPending = false;

		static int MobKind(oCNpc* npc, const zSTRING& body)
		{
			if (creeperPending)
				return MOB_CREEPER;
			if (!settings.mobs || !npc || npc == oCNpc::player)
				return MOB_NONE;
			char name[64], model[64];
			strncpy_s(name, body.ToChar(), _TRUNCATE);
			_strupr_s(name);
			strncpy_s(model, npc->mds_name.ToChar(), _TRUNCATE);
			_strupr_s(model);
			if (!strncmp(name, "SKE_BODY", 8) && strstr(model, "HUMANS"))
				return MOB_SKELETON;
			if (!strncmp(name, "ZOM_BODY", 8))
				return MOB_ZOMBIE;
			return MOB_NONE;
		}

		struct Effect
		{
			zCParticleFX* effect = nullptr;
			float stop = 0.0f;
		};
		static Effect effects[16];

		static void Emit(zCWorld* world, const char* emitter, const zVEC3& point, float seconds)
		{
			Effect* slot = nullptr;
			for (Effect& e : effects)
				if (!e.effect)
				{
					slot = &e;
					break;
				}
			if (!slot || !world)
				return;
			zCParticleFX* effect = new zCParticleFX();
			zCVob* vob = new zCVob();
			vob->dontWriteIntoArchive = TRUE;
			vob->SetCollDetStat(FALSE);
			vob->SetCollDetDyn(FALSE);
			vob->SetPositionWorld(point);
			vob->SetVisual(effect);
			effect->SetAndStartEmitter(zSTRING(emitter), FALSE);
			world->AddVob(vob);
			vob->SetCollDetStat(FALSE);
			vob->SetCollDetDyn(FALSE);
			vob->Release(); // the world keeps it; the effect is kept until it stops (the GTA mashup's bursts)
			slot->effect = effect;
			slot->stop = Now() + seconds;
		}

		static void UpdateEffects()
		{
			float now = Now();
			for (Effect& e : effects)
				if (e.effect && (now >= e.stop || now < e.stop - 10.0f))
				{
					e.effect->StopEmitterOutput();
					e.effect->Release();
					e.effect = nullptr;
				}
		}

		static void SpawnDragon(oCNpc* hero, const zVEC3& at);

		static void SpawnMob(oCNpc* hero, int egg)
		{
			Pick p = PickBlock(hero, REACH);
			if (!p.hit || !parser || !ogame)
				return;
			zVEC3 spot = p.block ? CellCentre(p.x + (int)p.normal[0], p.y + (int)p.normal[1], p.z + (int)p.normal[2]) : p.point;
			spot[1] += 10.0f;
			if (egg == EGG_DRAGON)
			{
				SpawnDragon(hero, spot);
				swingTicks = 0;
				return;
			}
			const char* instance = egg == EGG_SKELETON ? "SKELETON" : (GOTHIC2 ? "ZOMBIE01" : "ZOMBIE");
			int index = parser->GetIndex(zSTRING(instance));
			oCWorld* world = ogame->GetGameWorld();
			if (index < 0 || !world)
			{
				Log("egg: no %s in the scripts", instance);
				return;
			}
			creeperPending = egg == EGG_CREEPER;
			oCNpc* npc = static_cast<oCNpc*>(world->CreateVob(zVOB_TYPE_NSC, index));
			creeperPending = false;
			if (!npc)
				return;
			npc->SetPositionWorld(spot);
			world->AddVob(npc);
			npc->SetCollDetStat(FALSE);
			npc->SetCollDetDyn(FALSE);
			npc->SearchNpcPosition(spot);
			npc->SetCollDetStat(TRUE);
			npc->SetCollDetDyn(TRUE);
			npc->Enable(spot);
			if (egg == EGG_CREEPER)
				npc->name[0] = zSTRING("Creeper");
			npc->Release(); // the world keeps it
			swingTicks = 0;
			Log("egg: %s (%s) at %.0f %.0f %.0f", eggIds[egg], instance, spot[0], spot[1], spot[2]);
		}

		struct Fuse
		{
			oCNpc* npc;
			int swell, swellOld;
		};
		static Fuse fuses[24];
		static float creeperClock = 0.0f;

		static bool IsCreeper(oCNpc* npc)
		{
			zCModel* model = npc->GetVisual() ? npc->GetModel() : nullptr;
			if (!model || model->modelProtoList.GetNum() == 0 || !model->modelProtoList[0])
				return false;
			zSTRING name = model->modelProtoList[0]->modelProtoName;
			name.Upper();
			return name == "MCCREEPER";
		}

		static Fuse* FuseOf(oCNpc* npc)
		{
			Fuse* free = nullptr;
			for (Fuse& f : fuses)
			{
				if (f.npc == npc)
					return &f;
				if (!f.npc && !free)
					free = &f;
			}
			if (free)
				*free = { npc, 0, 0 };
			return free;
		}

		static void Explode(oCNpc* hero, const zVEC3& at, float power, oCNpc* source); // (with TNT, further down)

		static void UpdateCreepers(oCNpc* hero)
		{
			float dt = ztimer->frameTimeFloat / 1000.0f;
			creeperClock += dt > 0.25f ? 0.25f : dt;
			int ticks = 0;
			while (creeperClock >= TICK)
			{
				creeperClock -= TICK;
				ticks++;
			}
			for (Fuse& f : fuses) // gone from the world (or a load): forgotten
				if (f.npc)
				{
					bool there = false;
					oCWorld* game = ogame ? ogame->GetGameWorld() : nullptr;
					for (zCListSort<oCNpc>* node = game && game->voblist_npcs ? game->voblist_npcs->next : nullptr; node && !there; node = node->next)
						there = node->data == f.npc;
					if (!there || f.npc->IsDead())
						f.npc = nullptr;
				}
			oCWorld* game = ogame ? ogame->GetGameWorld() : nullptr;
			oCNpc* explode = nullptr;
			for (zCListSort<oCNpc>* node = game && game->voblist_npcs ? game->voblist_npcs->next : nullptr; node; node = node->next)
			{
				oCNpc* npc = node->data;
				if (!npc || npc == hero || npc->IsDead() || !IsCreeper(npc))
					continue;
				Fuse* f = FuseOf(npc);
				if (!f)
					continue;
				float d = (npc->GetPositionWorld() - hero->GetPositionWorld()).Length() / CELL;
				for (int t = 0; t < ticks; t++)
				{
					f->swellOld = f->swell;
					int dir = d < 3.0f || (f->swell > 0 && d < 7.0f) ? 1 : -1;
					if (dir > 0 && f->swell == 0)
						PlaySound("MC_FUSE.WAV", npc);
					f->swell = f->swell + dir < 0 ? 0 : f->swell + dir;
					if (f->swell >= 30)
						explode = npc;
				}
				// CreeperRenderer.scale: wider and a little taller as it swells, trembling
				float s = (f->swellOld + (f->swell - f->swellOld) * (creeperClock / TICK)) / 28.0f;
				float g = 1.0f + sinf(s * 100.0f) * s * 0.01f;
				s = s < 0.0f ? 0.0f : (s > 1.0f ? 1.0f : s);
				s *= s;
				s *= s;
				if (zCModel* model = npc->GetModel())
					model->SetModelScale(zVEC3((1.0f + s * 0.4f) * g, (1.0f + s * 0.1f) / g, (1.0f + s * 0.4f) * g));
			}
			if (explode)
			{
				for (Fuse& f : fuses)
					if (f.npc == explode)
						f.npc = nullptr;
				Explode(hero, explode->GetPositionWorld(), 3.0f, explode);
				if (oCSpawnManager* spawn = ogame->GetSpawnManager())
					spawn->DeleteNpc(explode);
			}
		}

		// --- the creative inventory (E) ---------------------------------------------------------------
		// CreativeModeInventoryScreen's panel (tab_items.png): a grid of 9 x 5 with Gothic's nine number
		// keys, the blocks and the spawn eggs, his hotbar under it. The mouse moves a pointer (the view
		// stands still); a left click puts what is under it into the chosen slot, a number key into that
		// slot, a click on the hotbar chooses that slot. E or Esc closes it.
		static int hoveredEntry = -1, hoveredSlot = -1;

		// a plain rectangle (the atlas's white pixel, tinted)
		static void Fill(float x, float y, float w, float h, const zCOLOR& color)
		{
			if (const Sprite* white = FindSprite("white"))
				Quad(atlasTexture, x, y, w, h, (white->x + 1) / atlasW, (white->y + 1) / atlasH, (white->x + 2) / atlasW, (white->y + 2) / atlasH, color);
		}

		static int InventoryEntries(Entry* out)
		{
			int n = 0;
			for (int k = 0; k < 9; k++)
				out[n++] = { ENTRY_GOTHIC, k };
			for (int b = 0; b < blockTypeCount && n < 45; b++)
				out[n++] = { ENTRY_BLOCK, b };
			for (int e = 0; e < EGGS && n < 45; e++)
				out[n++] = { ENTRY_EGG, e };
			for (int i = 0; i < MC_ITEMS && n < 45; i++)
				out[n++] = { ENTRY_ITEM, i };
			return n;
		}

		static void OpenInventory(bool open)
		{
			if (open == inventoryOpen)
				return;
			inventoryOpen = open;
			cursorX = guiW / 2.0f;
			cursorY = guiH / 2.0f;
			hoveredEntry = hoveredSlot = -1;
			Log("inventory %s", open ? "open" : "closed");
		}

		// a click in the open inventory (before his movement: the keys go to it)
		static void InventoryInput(oCNpc* hero)
		{
			if (!keyAttack.pressed)
				return;
			Entry entries[45];
			int count = InventoryEntries(entries);
			if (hoveredEntry >= 0 && hoveredEntry < count)
			{
				hotbar[selectedSlot] = entries[hoveredEntry];
				SaveHotbar();
				ShowToolName(hero);
			}
			else if (hoveredSlot >= 0)
				ChooseSlot(hero, hoveredSlot);
		}

		static void DrawInventory(oCNpc* hero)
		{
			Fill(0.0f, 0.0f, (float)guiW, (float)guiH, Argb(0xC0, 0x10, 0x10, 0x10)); // the dimmed screen
			float x0 = (float)((guiW - 195) / 2), y0 = (float)((guiH - 136) / 2);
			Blit("creative/tab_items", x0, y0);
			Text(inventoryTitle, x0 + 8.0f, y0 + 6.0f, Argb(255, 0x40, 0x40, 0x40), false);
			Entry entries[45];
			int count = InventoryEntries(entries);
			hoveredEntry = hoveredSlot = -1;
			auto hover = [&](float x, float y) { return cursorX >= x - 1 && cursorX < x + 17 && cursorY >= y - 1 && cursorY < y + 17; };
			auto highlight = [&](float x, float y) { Fill(x, y, 16.0f, 16.0f, Argb(0x80, 255, 255, 255)); };
			for (int i = 0; i < count; i++)
			{
				float x = x0 + 9.0f + (i % 9) * 18.0f, y = y0 + 18.0f + (i / 9) * 18.0f;
				DrawEntry(hero, entries[i], x, y);
				if (entries[i].kind == ENTRY_GOTHIC) // Gothic's keys: the key's number
				{
					char key[4];
					sprintf_s(key, "%d", entries[i].value + 1);
					Text(key, x, y, Argb(255, 255, 255, 160));
				}
				if (hover(x, y))
				{
					hoveredEntry = i;
					highlight(x, y);
				}
			}
			for (int i = 0; i < 9; i++)
			{
				float x = x0 + 9.0f + i * 18.0f, y = y0 + 112.0f;
				DrawEntry(hero, hotbar[i], x, y);
				if (i == selectedSlot)
					Blit("hotbar_selection", x - 4.0f, y - 4.0f);
				if (hover(x, y))
				{
					hoveredSlot = i;
					highlight(x, y);
				}
			}
			// the name of what is under the pointer, in a tooltip's dark box
			char name[96] = "";
			if (hoveredEntry >= 0)
			{
				EntryName(hero, entries[hoveredEntry], name, sizeof name);
				if (!name[0] && entries[hoveredEntry].kind == ENTRY_GOTHIC)
					sprintf_s(name, "Gothic %d", entries[hoveredEntry].value + 1);
			}
			else if (hoveredSlot >= 0)
				EntryName(hero, hotbar[hoveredSlot], name, sizeof name);
			if (name[0]) // GuiGraphics.renderTooltip's dark box
			{
				float w = TextWidth(name), x = cursorX + 12.0f, y = cursorY - 12.0f;
				Fill(x - 3.0f, y - 3.0f, w + 6.0f, 14.0f, Argb(0xF0, 0x10, 0x00, 0x10));
				Text(name, x, y, Argb(255, 255, 255, 255));
			}
			Fill(cursorX - 1.0f, cursorY - 1.0f, 3.0f, 3.0f, Argb(255, 0, 0, 0)); // the pointer
			Fill(cursorX - 0.5f, cursorY - 0.5f, 2.0f, 2.0f, Argb(255, 255, 255, 255));
		}

		static void UpdateArrows(oCNpc* hero);

		// Each frame of his AI: the blocks of this world, the block in his hand, the creepers, the
		// arrows, the effects.
		static void UpdateWorld(oCNpc* hero)
		{
			LoadHud();
			LoadHotbar();
			zCWorld* world = hero->GetHomeWorld();
			if (world != blocksWorld)
				FindBlocks(world);
			UpdateHeldBlock(hero);
			UpdateCreepers(hero);
			UpdateArrows(hero);
			UpdateTnt(hero);
			PhysicsTicks(hero);
			UpdateDragon(hero);
			UpdateEffects();
		}

		// The right button with a block or an egg chosen (Gothic's action stays for Gothic's slots).
		static void UseSelected(oCNpc* hero)
		{
			const Entry& e = hotbar[selectedSlot];
			if (e.kind == ENTRY_BLOCK && e.value < blockTypeCount)
				PlaceBlock(hero, e.value);
			else if (e.kind == ENTRY_EGG && e.value < EGGS)
				SpawnMob(hero, e.value);
		}

		static bool SelectedIsGothic() { return hotbar[selectedSlot].kind == ENTRY_GOTHIC || hotbar[selectedSlot].kind == ENTRY_NONE; }

		// a block or one of Minecraft's items in his hand (no weapon of Gothic's out)
		static bool McInHand(oCNpc* hero)
		{
			int kind = hotbar[selectedSlot].kind;
			return (kind == ENTRY_BLOCK || kind == ENTRY_ITEM || kind == ENTRY_EGG) && hero->GetWeaponMode() == NPC_WEAPON_NONE;
		}

		static bool HoldingBow(oCNpc* hero)
		{
			return hotbar[selectedSlot].kind == ENTRY_ITEM && hotbar[selectedSlot].value == ITEM_BOW && hero->GetWeaponMode() == NPC_WEAPON_NONE;
		}

		// --- Minecraft's ladders ------------------------------------------------------------------------
		// Gothic's ladders (oCMobLadder) are climbed Minecraft's way (Tick, handleOnClimbable): he is on
		// one while his middle is within 40 cm of its box and his feet between its foot and its top.
		static std::vector<zCVob*> ladders;
		static zCWorld* laddersWorld = nullptr;

		static void UpdateLadder(oCNpc* hero)
		{
			bool was = onLadder;
			onLadder = ladderAhead = false;
			zCWorld* world = hero->GetHomeWorld();
			if (!world || !hero->human_ai)
				return;
			if (world != laddersWorld)
			{
				ladders.clear();
				laddersWorld = world;
				zCArray<zCVob*> found;
				world->SearchVobListByClass(oCMobLadder::classDef, found, nullptr);
				for (int i = 0; i < found.GetNum(); i++)
					if (found[i])
						ladders.push_back(found[i]);
				Log("ladders: %d in this world", (int)ladders.size());
			}
			zVEC3 pos = hero->GetPositionWorld();
			float feet = hero->human_ai->feetY;
			for (zCVob* ladder : ladders)
			{
				const zTBBox3D& b = ladder->bbox3D;
				float dx = b.mins[0] - pos[0] > 0.0f ? b.mins[0] - pos[0] : (pos[0] - b.maxs[0] > 0.0f ? pos[0] - b.maxs[0] : 0.0f);
				float dz = b.mins[2] - pos[2] > 0.0f ? b.mins[2] - pos[2] : (pos[2] - b.maxs[2] > 0.0f ? pos[2] - b.maxs[2] : 0.0f);
				if (dx * dx + dz * dz > 40.0f * 40.0f || feet < b.mins[1] - 30.0f || feet > b.maxs[1] - 10.0f)
					continue;
				onLadder = true;
				zVEC3 to((b.mins[0] + b.maxs[0]) * 0.5f - pos[0], 0.0f, (b.mins[2] + b.maxs[2]) * 0.5f - pos[2]);
				float len = to.Length();
				ladderAhead = len < 25.0f || Forward().Dot(to * (1.0f / len)) > 0.0f;
				break;
			}
			if (onLadder != was)
				Log("ladder: %s", onLadder ? "on" : "off");
		}

		// Gothic's climb for a ledge Gothic's CanJumpLedge found (zCAIPlayer's ledge info): only higher
		// than Minecraft's jump reaches (1.25 blocks; 1.35 m), and not a block's.
		static bool GothicLedge(oCNpc* hero)
		{
			zCAIPlayer* ai = hero->human_ai;
			zCAIPlayer::zTLedgeInfo* ledge = ai ? ai->GetLedgeInfo() : nullptr;
			if (!ledge)
				return false;
			float height = ledge->point[1] - ai->feetY;
			bool block = false;
			for (int i = -1; i <= 1 && !block; i++)
				for (int j = -1; j <= 1 && !block; j++)
				{
					int x, y, z;
					CellOf(ledge->point + zVEC3(i * 30.0f, -10.0f, j * 30.0f), x, y, z);
					block = blocks.count(CellKey(x, y, z)) != 0;
				}
			Log("jump: a ledge %.0f cm up%s", height, block ? ", a block's" : "");
			return height > 135.0f && !block;
		}

		// --- the right button (Minecraft.startUseItem) -------------------------------------------------
		// What Gothic has in focus, if it is something to use (someone to talk to who isn't a beast, a
		// body, an item, a chest, a door, a bench; not a ladder) and the crosshair is on it within 4 m.
		static zCVob* InteractTarget(oCNpc* hero)
		{
			zCVob* focus = hero->GetFocusVob();
			if (!focus || focus == hero || zDYNAMIC_CAST<oCMobLadder>(focus))
				return nullptr;
			if (oCNpc* npc = zDYNAMIC_CAST<oCNpc>(focus))
			{
				if (!npc->IsDead() && npc->IsMonster() && !npc->IsOrc())
					return nullptr;
			}
			else if (!zDYNAMIC_CAST<oCItem>(focus) && !zDYNAMIC_CAST<oCMobInter>(focus))
				return nullptr;
			zTBBox3D box = focus->bbox3D;
			box.mins -= zVEC3(20.0f, 20.0f, 20.0f);
			box.maxs += zVEC3(20.0f, 20.0f, 20.0f);
			float distance = RayBox(Eyes(hero), Look(), box);
			return distance >= 0.0f && distance <= 400.0f ? focus : nullptr;
		}

		// Gothic's action (with the forward key, as Gothic 1 interacts) for half a second; a drawn weapon
		// goes away first (its number key), when there is something to use.
		static void Interact(oCNpc* hero, zCVob* target)
		{
			if (hero->GetWeaponMode() != NPC_WEAPON_NONE)
			{
				int held = HeldKey(hero);
				if (!target || held < 0)
					return;
				step = "use: putting the weapon away";
				PressGothicKey(KEYS[held]);
				step = "movement";
				putAwayUntil = Now() + 2.0f;
				Log("use: %s, the weapon away first", target->GetObjectName().ToChar());
				return;
			}
			useUntil = Now() + 0.5f;
			useEdge = true;
			Log("use: Gothic's action on %s", target ? target->GetObjectName().ToChar() : "its focus");
		}

		static void RightClick(oCNpc* hero)
		{
			zCVob* target = InteractTarget(hero);
			if (target || SelectedIsGothic())
				Interact(hero, target);
			else if (HoldingBow(hero))
			{
				useTicks = 0;
				Log("bow: drawing");
			}
			else if (hotbar[selectedSlot].kind == ENTRY_ITEM && hotbar[selectedSlot].value == ITEM_FLINT)
				Ignite(hero);
			else if (hotbar[selectedSlot].kind == ENTRY_ITEM && hotbar[selectedSlot].value == ITEM_LAVA)
				PourLava(hero);
			else if (hotbar[selectedSlot].kind == ENTRY_ITEM && hotbar[selectedSlot].value == ITEM_BUCKET)
				FillBucket(hero);
			else
				UseSelected(hero);
		}

		// --- Minecraft's bow and arrows ----------------------------------------------------------------
		// The bow (BowItem): the right button held draws it (Tick: walking at a fifth, the view narrowing
		// up to 15 %; its drawn stages in his hand; both arms up where he looks); let go, it shoots at
		// its power (t^2 + 2t) / 3, t the seconds drawn, at most 1 (a full draw takes a second), none
		// under 0.1, with Minecraft's sound. The arrow (AbstractArrow) leaves his eyes 10 cm down at 3
		// blocks a tick times the power where the crosshair points (a little scattered, plus his own
		// movement); each tick it moves, then loses 1 % of its speed and falls 0.05 faster. What it
		// meets first on the way: Gothic's world or a block, where it sticks for a minute (Minecraft's
		// 1200 ticks; until its block goes), or someone, who takes ceil(2 x its speed) of Minecraft's 20
		// health (a full draw's critical shot up to half of it plus 2 more) as that share of theirs,
		// with Gothic's hurt and reaction to him. Creative: it needs no arrows. Arrows aren't saved.
		struct Arrow
		{
			zCVob* vob = nullptr; // a reference of the plugin's own: the object stays valid till released
			zVEC3 pos, last, vel; // cm (now, a tick ago); blocks a tick
			bool stuck = false, crit = false;
			long long block = -1; // the block it sticks in, -1 Gothic's world
			float until = 0.0f;
			int age = 0;          // ticks flown: shown from the second (it leaves from between his eyes)
		};
		static Arrow arrows[32];
		static float arrowClock = 0.0f;
		static int nextArrow = 0;

		static void RemoveArrow(Arrow& a)
		{
			if (!a.vob)
				return;
			if (zCWorld* world = a.vob->GetHomeWorld())
				world->RemoveVob(a.vob);
			a.vob->Release();
			a.vob = nullptr;
		}

		// its object at its place between the last tick's and this one's, along its flight
		static void PlaceArrow(Arrow& a, float share)
		{
			float len = a.vel.Length();
			if (len > 0.0001f)
			{
				zVEC3 dir = a.vel * (1.0f / len);
				if (fabsf(dir[1]) > 0.999f) // straight up or down: a heading needs some way along
					dir = zVEC3(0.01f, dir[1] > 0.0f ? 1.0f : -1.0f, 0.0f);
				a.vob->SetHeadingAtWorld(dir);
			}
			a.vob->SetPositionWorld(a.stuck ? a.pos : a.last + (a.pos - a.last) * share);
		}

		static void ShootArrow(oCNpc* hero, float power)
		{
			oCWorld* world = ogame ? ogame->GetGameWorld() : nullptr;
			if (!world)
				return;
			Arrow* slot = nullptr;
			for (int i = 0; i < 32 && !slot; i++)
				if (!arrows[(nextArrow + i) % 32].vob)
					slot = &arrows[(nextArrow + i) % 32];
			if (!slot) // all taken: the oldest goes
			{
				slot = &arrows[nextArrow % 32];
				RemoveArrow(*slot);
			}
			nextArrow = (int)(slot - arrows) + 1;
			auto scatter = [] { return 0.0172275f * (rand() / (float)RAND_MAX - rand() / (float)RAND_MAX); }; // RandomSource.triangle
			zVEC3 look = Look();
			zVEC3 vel(look[0] + scatter(), look[1] + scatter(), look[2] + scatter());
			vel = vel * (power * 3.0f) + zVEC3(moveX, onGround ? 0.0f : moveY, moveZ); // Projectile.shootFromRotation
			zVEC3 at = Eyes(hero) - zVEC3(0.0f, 10.0f, 0.0f);
			zCVob* vob = new zCVob();
			vob->dontWriteIntoArchive = TRUE;
			vob->SetCollDetStat(FALSE);
			vob->SetCollDetDyn(FALSE);
			vob->SetVisual(zSTRING("MC_ARROW.3DS"));
			KeepDetail(vob->GetVisual());
			vob->SetPositionWorld(at);
			world->AddVob(vob);
			vob->SetCollDetStat(FALSE);
			vob->SetCollDetDyn(FALSE);
			vob->showVisual = FALSE;
			*slot = Arrow();
			slot->vob = vob; // (the plugin keeps its reference)
			slot->pos = slot->last = at;
			slot->vel = vel;
			slot->crit = power >= 1.0f;
			slot->until = Now() + 60.0f;
			PlaceArrow(*slot, 0.0f);
		}

		static void ArrowTick(oCNpc* hero, Arrow& a)
		{
			if (a.stuck)
			{
				if (Now() > a.until || (a.block >= 0 && !blocks.count(a.block)))
					RemoveArrow(a);
				return;
			}
			if (Now() > a.until)
			{
				RemoveArrow(a);
				return;
			}
			if (++a.age >= 2)
				a.vob->showVisual = TRUE;
			zVEC3 from = a.pos, move = a.vel * BLOCK;
			float hit = 2.0f; // the share of this tick's move to what it meets
			zCVob* hitVob = nullptr;
			oCNpc* victim = nullptr;
			zCWorld* world = a.vob->GetHomeWorld();
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_VOB_IGNORE_CHARACTER | zTRACERAY_VOB_IGNORE_NO_CD_DYN;
			if (world && world->TraceRayNearestHit(from, move, a.vob, flags) && world->traceRayReport.foundHit)
			{
				hit = (world->traceRayReport.foundIntersection - from).Length() / move.Length();
				hitVob = world->traceRayReport.foundVob;
			}
			oCWorld* game = ogame ? ogame->GetGameWorld() : nullptr;
			for (zCListSort<oCNpc>* node = game && game->voblist_npcs ? game->voblist_npcs->next : nullptr; node; node = node->next)
			{
				oCNpc* npc = node->data;
				if (!npc || npc == hero || npc->IsDead())
					continue;
				zTBBox3D box = npc->bbox3D;
				zVEC3 centre = (box.mins + box.maxs) * 0.5f;
				for (int i = 0; i < 3; i += 2) // a body is narrower than its box (the arms)
				{
					box.mins[i] = centre[i] + (box.mins[i] - centre[i]) * 0.6f;
					box.maxs[i] = centre[i] + (box.maxs[i] - centre[i]) * 0.6f;
				}
				float t = RayBox(from, move, box);
				if (t >= 0.0f && t < hit)
				{
					hit = t;
					victim = npc;
				}
			}
			float dragonT;
			int dragonPart = DragonPartOnRay(from, move, dragonT);
			if (dragonPart >= 0 && dragonT < hit)
			{
				int damage = (int)ceilf(a.vel.Length() * 2.0f);
				if (a.crit)
					damage += rand() % (damage / 2 + 2);
				DragonHurt(hero, dragonPart, (float)damage, true);
				RemoveArrow(a);
				return;
			}
			if (hit <= 1.0f)
			{
				zVEC3 point = from + move * hit;
				char sound[24];
				sprintf_s(sound, "MC_BOWHIT%d.WAV", 1 + rand() % 4);
				if (victim) // AbstractArrow.onHitEntity
				{
					int damage = (int)ceilf(a.vel.Length() * 2.0f);
					if (a.crit)
						damage += rand() % (damage / 2 + 2);
					float points = damage / 20.0f * victim->attribute[NPC_ATR_HITPOINTSMAX];
					PlaySound(sound, victim);
					step = "arrow: Gothic's damage";
					victim->OnDamage(hero, hero, points + victim->protection[oEDamageIndex_Point], oEDamageType_Point, point);
					step = "world";
					Log("arrow: hits %s for %d of Minecraft's 20 (%.0f; health %d/%d)", victim->GetInstanceName().ToChar(), damage, points,
						victim->attribute[NPC_ATR_HITPOINTS], victim->attribute[NPC_ATR_HITPOINTSMAX]);
					RemoveArrow(a);
					return;
				}
				// AbstractArrow.onHitBlock: it sticks, 0.05 blocks back along its flight
				a.pos = a.last = point - move * (5.0f / move.Length());
				a.stuck = true;
				a.until = Now() + 60.0f;
				if (hitVob)
				{
					int x, y, z;
					CellOf(hitVob->GetPositionWorld(), x, y, z);
					auto it = blocks.find(CellKey(x, y, z));
					if (it != blocks.end() && it->second == hitVob)
						a.block = it->first;
				}
				PlaySound(sound, a.vob);
				a.vob->showVisual = TRUE;
				PlaceArrow(a, 1.0f);
				return;
			}
			a.last = a.pos;
			a.pos = from + move;
			a.vel = a.vel * 0.99f;
			a.vel[1] -= 0.05f;
		}

		static void UpdateArrows(oCNpc* hero)
		{
			float dt = ztimer->frameTimeFloat / 1000.0f;
			arrowClock += dt > 0.25f ? 0.25f : dt;
			while (arrowClock >= TICK)
			{
				arrowClock -= TICK;
				for (Arrow& a : arrows)
					if (a.vob)
						ArrowTick(hero, a);
			}
			for (Arrow& a : arrows)
				if (a.vob)
					PlaceArrow(a, arrowClock / TICK);
		}

		// BowItem.getPowerForTime
		static float BowPower(int ticks)
		{
			float f = ticks / 20.0f;
			f = (f * f + f * 2.0f) / 3.0f;
			return f > 1.0f ? 1.0f : f;
		}

		static void ReleaseBow(oCNpc* hero, bool shoot)
		{
			int ticks = useTicks;
			useTicks = -1;
			float power = BowPower(ticks);
			if (!shoot || power < 0.1f)
			{
				Log("bow: let down (%d ticks)", ticks);
				return;
			}
			ShootArrow(hero, power);
			PlaySound("MC_BOW.WAV", hero);
			Log("bow: shot at power %.2f (%d ticks)%s", power, ticks, power >= 1.0f ? ", critical" : "");
		}

		// --- Minecraft's grid over Gothic's world -------------------------------------------------------
		// Minecraft's physics (fire, lava, falling sand, where a block goes on the ground, an explosion's
		// rays) works on cells of a metre, the blocks' grid. Gothic's own world is put on that grid, unseen:
		// a cell is solid when its middle is inside Gothic's static world (in the ground, a rock, a wall):
		// looking straight up from it, the first of Gothic's polygons faces up (the inside of a surface).
		// Asked for where needed and kept for the world.

		static void CellFromKey(long long key, int& x, int& y, int& z)
		{
			auto field = [](long long v) { int i = (int)(v & 0x1FFFFF); return i & 0x100000 ? i - 0x200000 : i; };
			x = field(key >> 42);
			y = field(key >> 21);
			z = field(key);
		}

		static bool GothicSolid(int x, int y, int z)
		{
			long long key = CellKey(x, y, z);
			auto it = gothicSolid.find(key);
			if (it != gothicSolid.end())
				return it->second != 0;
			bool solid = false;
			if (zCWorld* world = blocksWorld)
			{
				int flags = zTRACERAY_STAT_POLY | zTRACERAY_POLY_2SIDED | zTRACERAY_POLY_NORMAL | zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_VOB_IGNORE;
				if (world->TraceRayNearestHit(CellCentre(x, y, z), zVEC3(0.0f, 100.0f * BLOCK, 0.0f), (zCVob*)nullptr, flags)
					&& world->traceRayReport.foundHit)
					solid = world->traceRayReport.foundPolyNormal[1] > 0.0f;
			}
			gothicSolid[key] = solid ? 1 : 0;
			return solid;
		}

		// a full cell for Minecraft's physics: a block or Gothic's world
		static bool Solid(int x, int y, int z) { return blocks.count(CellKey(x, y, z)) != 0 || GothicSolid(x, y, z); }

		// --- Minecraft's fire (FireBlock), lava (LavaFluid, FlowingFluid), falling sand (FallingBlockEntity) ---
		// Fire: a block of it (MC_FIRE.3DS: Minecraft's floor fire, its animation, full bright) where flint
		// and steel lights a cell's side, or lava or fire spreads. It stays while a solid cell is under it or
		// something flammable next to it; every 30 to 39 ticks it ages (0 to 15), and with nothing flammable
		// next to it goes out older than 3 (on the ground: seconds), at 15 by a chance of 1 in 4; it burns
		// the flammable blocks next to it (planks, logs, leaves, wool, bookshelves, TNT, by Minecraft's odds;
		// a burnt one may catch fire, TNT is lit) and lights empty cells near flammable ones, up to 4 above.
		// Lava: a source from the lava bucket (the bucket takes one back) flows as Minecraft's in the
		// Overworld: every 30 ticks a cell takes the most of its four neighbours less 2 (so 3 blocks from a
		// source), a cell with lava above is full and falling; it falls first, and sideways goes the ways
		// that lead down soonest (looking 2 cells ahead); a block put in it takes its place. Now and then
		// (Minecraft's random ticks) it lights flammable things near it. Drawn at its level (n / 9 of a
		// cell), animated, full bright. Sand falls (gravity 0.04 a tick) when nothing solid is under it.
		// Whoever stands in fire takes a point of Minecraft's 20 (in lava 4) at most every 10 ticks and burns
		// (8 seconds, lava 15): a point a second, water puts it out; burning they show Minecraft's flames,
		// Steve in first person at the bottom of the screen; in lava Steve moves as in Minecraft's (0.02 a
		// tick, half his speed kept, falling slowly, Space swims up 0.04).
		struct FireCell
		{
			zCVob* vob;
			int age;
			float due;
		};
		static std::unordered_map<long long, FireCell> fireCells;
		struct LavaCell
		{
			zCVob* vob;
			int amount;          // 1 to 8 (a source 8)
			bool falling, source;
			float due;           // its next tick (< 0: none)
			int mesh;            // MC_LAVA_<mesh> it shows
		};
		static std::unordered_map<long long, LavaCell> lavaCells;
		struct Falling
		{
			zCVob* vob = nullptr; // the plugin's reference
			int kind = 0;
			zVEC3 pos, last;      // the bottom's middle (cm)
			float vy = 0.0f;
			int time = 0;
		};
		static Falling fallingBlocks[32];
		struct Burning
		{
			int fireTicks = 0;        // Entity.remainingFireTicks
			float hurtUntil = 0.0f;   // hurt again from then (Minecraft's 10 ticks)
			zCVob* flames = nullptr;  // the plugin's reference
			bool seen = false;
		};
		static std::unordered_map<oCNpc*, Burning> burning;
		static zCVisual* fireVisual = nullptr;
		static zCVisual* lavaVisuals[10] = {};
		static zCTexture* fireFrames[2][32] = {};
		static zCTexture* lavaStill[20] = {};
		static zCTexture* lavaFlow[32] = {};
		static float physicsClock = 0.0f;
		static const int LAVA_DELAY = 30;

		static void Flammability(int x, int y, int z, int& ignite, int& burn)
		{
			ignite = burn = 0;
			auto it = blockKinds.find(CellKey(x, y, z));
			if (it == blockKinds.end())
				return;
			static const struct { const char* id; int ignite, burn; } TABLE[] = { { "oak_planks", 5, 20 }, { "oak_log", 5, 5 },
				{ "oak_leaves", 30, 60 }, { "white_wool", 30, 60 }, { "bookshelf", 30, 20 }, { "tnt", 15, 100 } };
			for (const auto& t : TABLE)
				if (!strcmp(blockTypes[it->second].id, t.id))
				{
					ignite = t.ignite;
					burn = t.burn;
					return;
				}
		}

		static int IgniteOddsOf(int x, int y, int z)
		{
			int ignite, burn;
			Flammability(x, y, z, ignite, burn);
			return ignite;
		}

		static bool Air(int x, int y, int z)
		{
			long long key = CellKey(x, y, z);
			return !blocks.count(key) && !fireCells.count(key) && !lavaCells.count(key) && !GothicSolid(x, y, z);
		}

		static const int SIDES[6][3] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };

		static bool FlammableNear(int x, int y, int z) // FireBlock.isValidFireLocation
		{
			for (const auto& d : SIDES)
				if (IgniteOddsOf(x + d[0], y + d[1], z + d[2]) > 0)
					return true;
			return false;
		}

		static bool FireCanStay(int x, int y, int z) { return Solid(x, y - 1, z) || FlammableNear(x, y, z); }

		static zVEC3 CellBottom(int x, int y, int z) { return zVEC3((x + 0.5f) * CELL, y * CELL, (z + 0.5f) * CELL); }

		// where lava or fire is drawn: on Gothic's ground when that is under the cell and lower than the
		// grid's step (a slope: up to half a cell), so it doesn't hang over it
		static zVEC3 OnGround(int x, int y, int z)
		{
			zVEC3 at = CellBottom(x, y, z);
			if (!GothicSolid(x, y - 1, z) || !blocksWorld)
				return at;
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_VOB_IGNORE;
			if (blocksWorld->TraceRayNearestHit(at + zVEC3(0.0f, CELL * 0.5f, 0.0f), zVEC3(0.0f, -CELL, 0.0f), (zCVob*)nullptr, flags)
				&& blocksWorld->traceRayReport.foundHit && blocksWorld->traceRayReport.foundIntersection[1] < at[1])
				at[1] = blocksWorld->traceRayReport.foundIntersection[1];
			return at;
		}

		// an object for fire, lava or a falling block: no collision, not saved, the plugin's reference
		static zCVob* EffectVob(zCVisual* visual, const zVEC3& at)
		{
			oCWorld* world = ogame ? ogame->GetGameWorld() : nullptr;
			if (!world || !visual)
				return nullptr;
			KeepDetail(visual);
			zCVob* vob = new zCVob();
			vob->dontWriteIntoArchive = TRUE;
			vob->SetCollDetStat(FALSE);
			vob->SetCollDetDyn(FALSE);
			vob->SetVisual(visual);
			vob->SetPositionWorld(at);
			world->AddVob(vob);
			vob->SetCollDetStat(FALSE);
			vob->SetCollDetDyn(FALSE);
			return vob;
		}

		static void DropVob(zCVob*& vob)
		{
			if (!vob)
				return;
			if (zCWorld* world = vob->GetHomeWorld())
				world->RemoveVob(vob);
			vob->Release();
			vob = nullptr;
		}

		static void CellChanged(int x, int y, int z);

		static void RemoveFire(long long key)
		{
			auto it = fireCells.find(key);
			if (it == fireCells.end())
				return;
			DropVob(it->second.vob);
			fireCells.erase(it);
		}

		static void SetFire(int x, int y, int z, int age)
		{
			long long key = CellKey(x, y, z);
			if (!Air(x, y, z) || !FireCanStay(x, y, z))
				return;
			if (!fireVisual)
				fireVisual = zCVisual::LoadVisual(zSTRING("MC_FIRE.3DS")); // kept
			FireCell cell;
			cell.vob = EffectVob(fireVisual, OnGround(x, y, z));
			cell.age = age;
			cell.due = Now() + (30 + rand() % 10) * TICK;
			fireCells[key] = cell;
		}

		static void RemoveLava(long long key)
		{
			auto it = lavaCells.find(key);
			if (it == lavaCells.end())
				return;
			DropVob(it->second.vob);
			lavaCells.erase(it);
		}

		static void ScheduleLava(long long key, int ticks)
		{
			auto it = lavaCells.find(key);
			if (it != lavaCells.end() && it->second.due < 0.0f)
				it->second.due = Now() + ticks * TICK;
		}

		// lava's look: its level, full with lava above it (FlowingFluid.getHeight)
		static void ShowLava(int x, int y, int z)
		{
			auto it = lavaCells.find(CellKey(x, y, z));
			if (it == lavaCells.end())
				return;
			LavaCell& c = it->second;
			int mesh = c.falling || lavaCells.count(CellKey(x, y + 1, z)) ? 9 : c.amount;
			if (mesh == c.mesh && c.vob)
				return;
			if (!lavaVisuals[mesh])
			{
				char name[24];
				sprintf_s(name, "MC_LAVA_%d.3DS", mesh);
				lavaVisuals[mesh] = zCVisual::LoadVisual(zSTRING(name)); // kept
			}
			if (!c.vob)
				c.vob = EffectVob(lavaVisuals[mesh], OnGround(x, y, z));
			else
				c.vob->SetVisual(lavaVisuals[mesh]);
			c.mesh = mesh;
		}

		static void SetLava(int x, int y, int z, int amount, bool falling, bool source)
		{
			long long key = CellKey(x, y, z);
			RemoveFire(key);
			auto it = lavaCells.find(key);
			if (it == lavaCells.end())
				it = lavaCells.emplace(key, LavaCell{ nullptr, 0, false, false, -1.0f, -1 }).first;
			it->second.amount = amount;
			it->second.falling = falling;
			it->second.source = source;
			it->second.due = -1.0f;
			ScheduleLava(key, LAVA_DELAY); // onPlace
			ShowLava(x, y, z);
			ShowLava(x, y - 1, z);
			CellChanged(x, y, z);
		}

		static bool LavaSource(int x, int y, int z)
		{
			auto it = lavaCells.find(CellKey(x, y, z));
			return it != lavaCells.end() && it->second.source;
		}

		// FlowingFluid.canMaybePassThrough: not solid, not a source of it
		static bool LavaCanEnter(int x, int y, int z) { return !Solid(x, y, z) && !LavaSource(x, y, z); }

		// FlowingFluid.getNewLiquid: amount (0: none), falling
		static int NewLava(int x, int y, int z, bool& falling)
		{
			falling = false;
			if (lavaCells.count(CellKey(x, y + 1, z)))
			{
				falling = true;
				return 8;
			}
			int highest = 0;
			for (int d = 0; d < 6; d++)
				if (SIDES[d][1] == 0)
				{
					auto it = lavaCells.find(CellKey(x + SIDES[d][0], y, z + SIDES[d][2]));
					if (it != lavaCells.end() && it->second.amount > highest)
						highest = it->second.amount;
				}
			int amount = highest - 2; // LavaFluid.getDropOff
			return amount > 0 ? amount : 0;
		}

		// SpreadContext.isHole: lava can go down from there
		static bool LavaHole(int x, int y, int z) { return !Solid(x, y - 1, z); }

		static int SlopeDistance(int x, int y, int z, int pass, int fromDir) // FlowingFluid.getSlopeDistance
		{
			int lowest = 1000;
			for (int d = 0; d < 6; d++)
			{
				if (SIDES[d][1] != 0 || d == fromDir)
					continue;
				int tx = x + SIDES[d][0], tz = z + SIDES[d][2];
				if (!LavaCanEnter(tx, y, tz))
					continue;
				if (LavaHole(tx, y, tz))
					return pass;
				if (pass < 2) // LavaFluid.getSlopeFindDistance
				{
					int v = SlopeDistance(tx, y, tz, pass + 1, d ^ 1);
					lowest = v < lowest ? v : lowest;
				}
			}
			return lowest;
		}

		static void SpreadLavaTo(int x, int y, int z, int amount, bool falling)
		{
			if (amount <= 0)
				return;
			auto it = lavaCells.find(CellKey(x, y, z));
			if (it != lavaCells.end()) // lava isn't replaced by lava (LavaFluid.canBeReplacedWith)
				return;
			SetLava(x, y, z, amount, falling, false);
		}

		static void SpreadLavaSides(int x, int y, int z, const LavaCell& c)
		{
			int neighbour = c.falling ? 7 : c.amount - 2;
			if (neighbour <= 0)
				return;
			int lowest = 1000, ways[4], wayCount = 0, amounts[4];
			bool falls[4];
			for (int d = 0; d < 6; d++) // FlowingFluid.getSpread
			{
				if (SIDES[d][1] != 0)
					continue;
				int tx = x + SIDES[d][0], tz = z + SIDES[d][2];
				if (!LavaCanEnter(tx, y, tz))
					continue;
				bool falling;
				int amount = NewLava(tx, y, tz, falling);
				int distance = LavaHole(tx, y, tz) ? 0 : SlopeDistance(tx, y, tz, 1, d ^ 1);
				if (distance < lowest)
					wayCount = 0;
				if (distance <= lowest)
				{
					if (!lavaCells.count(CellKey(tx, y, tz)) && wayCount < 4)
					{
						ways[wayCount] = d;
						amounts[wayCount] = amount;
						falls[wayCount++] = falling;
					}
					lowest = distance;
				}
			}
			for (int i = 0; i < wayCount; i++)
				SpreadLavaTo(x + SIDES[ways[i]][0], y, z + SIDES[ways[i]][2], amounts[i], falls[i]);
		}

		static void LavaTick(int x, int y, int z)
		{
			long long key = CellKey(x, y, z);
			auto it = lavaCells.find(key);
			if (it == lavaCells.end())
				return;
			it->second.due = -1.0f;
			if (!it->second.source) // FlowingFluid.tick
			{
				bool falling;
				int amount = NewLava(x, y, z, falling);
				if (amount <= 0)
				{
					RemoveLava(key);
					ShowLava(x, y - 1, z);
					CellChanged(x, y, z);
					return;
				}
				if (amount != it->second.amount || falling != it->second.falling)
				{
					int delay = LAVA_DELAY; // LavaFluid.getSpreadDelay: rising, mostly 4 times later
					if (!falling && !it->second.falling && amount > it->second.amount && rand() % 4 != 0)
						delay *= 4;
					it->second.amount = amount;
					it->second.falling = falling;
					ShowLava(x, y, z);
					it->second.due = Now() + delay * TICK;
					CellChanged(x, y, z);
				}
			}
			LavaCell c = it->second;
			// FlowingFluid.spread: down first
			if (LavaCanEnter(x, y - 1, z))
			{
				if (!lavaCells.count(CellKey(x, y - 1, z)))
				{
					SpreadLavaTo(x, y - 1, z, 8, true);
					int sources = 0;
					for (int d = 0; d < 6; d++)
						if (SIDES[d][1] == 0 && LavaSource(x + SIDES[d][0], y, z + SIDES[d][2]))
							sources++;
					if (sources >= 3)
						SpreadLavaSides(x, y, z, c);
					return;
				}
			}
			if (c.source || Solid(x, y - 1, z)) // (not a hole under it)
				SpreadLavaSides(x, y, z, c);
		}

		// LavaFluid.randomTick: fire near it, where something flammable is
		static void LavaRandomTick(int x, int y, int z)
		{
			int passes = rand() % 3;
			if (passes > 0)
			{
				int tx = x, ty = y, tz = z;
				for (int pass = 0; pass < passes; pass++)
				{
					tx += rand() % 3 - 1;
					ty += 1;
					tz += rand() % 3 - 1;
					if (Air(tx, ty, tz))
					{
						if (FlammableNear(tx, ty, tz))
						{
							SetFire(tx, ty, tz, 0);
							return;
						}
					}
					else if (Solid(tx, ty, tz))
						return;
				}
			}
			else
				for (int i = 0; i < 3; i++)
				{
					int tx = x + rand() % 3 - 1, tz = z + rand() % 3 - 1;
					if (Air(tx, y + 1, tz) && IgniteOddsOf(tx, y, tz) > 0)
						SetFire(tx, y + 1, tz, 0);
				}
		}

		static void PrimeTnt(oCNpc* hero, long long key, int fuse);

		// FireBlock.checkBurnOut
		static void BurnOut(oCNpc* hero, int x, int y, int z, int chance, int age)
		{
			int ignite, burn;
			Flammability(x, y, z, ignite, burn);
			if (rand() % chance >= burn)
				return;
			long long key = CellKey(x, y, z);
			bool tnt = !strcmp(blockTypes[blockKinds[key]].id, "tnt");
			if (tnt)
			{
				PrimeTnt(hero, key, 80);
				return;
			}
			RemoveBlock(key, false);
			if (rand() % (age + 10) < 5)
				SetFire(x, y, z, age + rand() % 5 / 4 > 15 ? 15 : age + rand() % 5 / 4);
		}

		static void FireTick(oCNpc* hero, int x, int y, int z)
		{
			long long key = CellKey(x, y, z);
			auto it = fireCells.find(key);
			if (it == fireCells.end())
				return;
			it->second.due = Now() + (30 + rand() % 10) * TICK;
			if (!FireCanStay(x, y, z))
			{
				RemoveFire(key);
				return;
			}
			int age = it->second.age;
			int newAge = age + rand() % 3 / 2;
			it->second.age = newAge > 15 ? 15 : newAge;
			if (!FlammableNear(x, y, z))
			{
				if (!Solid(x, y - 1, z) || age > 3)
					RemoveFire(key);
				return;
			}
			if (age == 15 && rand() % 4 == 0 && IgniteOddsOf(x, y - 1, z) == 0)
			{
				RemoveFire(key);
				return;
			}
			BurnOut(hero, x + 1, y, z, 300, age);
			BurnOut(hero, x - 1, y, z, 300, age);
			BurnOut(hero, x, y - 1, z, 250, age);
			BurnOut(hero, x, y + 1, z, 250, age);
			BurnOut(hero, x, y, z + 1, 300, age);
			BurnOut(hero, x, y, z - 1, 300, age);
			for (int xx = -1; xx <= 1; xx++)
				for (int zz = -1; zz <= 1; zz++)
					for (int yy = -1; yy <= 4; yy++)
					{
						if (!xx && !yy && !zz)
							continue;
						int tx = x + xx, ty = y + yy, tz = z + zz;
						if (!Air(tx, ty, tz))
							continue;
						int odds = 0; // FireBlock.getIgniteOdds: the most of its neighbours'
						for (const auto& d : SIDES)
						{
							int o = IgniteOddsOf(tx + d[0], ty + d[1], tz + d[2]);
							odds = o > odds ? o : odds;
						}
						if (odds <= 0)
							continue;
						int rate = 100 + (yy > 1 ? (yy - 1) * 100 : 0);
						int chance = (odds + 40 + 2 * 7) / (age + 30); // Minecraft's normal difficulty
						if (chance > 0 && rand() % rate <= chance)
						{
							int spread = age + rand() % 5 / 4;
							SetFire(tx, ty, tz, spread > 15 ? 15 : spread);
						}
					}
		}

		static void CheckFall(int x, int y, int z);

		// A cell changed (a block, fire or lava came or went): lava near it looks again (its next tick),
		// a block above it may fall.
		static void CellChanged(int x, int y, int z)
		{
			ScheduleLava(CellKey(x, y, z), LAVA_DELAY);
			for (const auto& d : SIDES)
				ScheduleLava(CellKey(x + d[0], y + d[1], z + d[2]), LAVA_DELAY);
			fallChecks.push_back({ CellKey(x, y + 1, z), 2 });
		}

		static bool Gravity(int kind) { return kind >= 0 && kind < blockTypeCount && !strcmp(blockTypes[kind].id, "sand"); }

		static void CheckFall(int x, int y, int z) // FallingBlock.tick
		{
			long long key = CellKey(x, y, z);
			auto it = blockKinds.find(key);
			if (it == blockKinds.end() || !Gravity(it->second) || Solid(x, y - 1, z))
				return;
			Falling* f = nullptr;
			for (Falling& g : fallingBlocks)
				if (!g.vob && !f)
					f = &g;
			if (!f)
				return;
			int kind = it->second;
			BlockType& b = blockTypes[kind];
			char name[64];
			BlockVisual(kind, name, sizeof name, false);
			zCVisual* visual = zCVisual::LoadVisual(zSTRING(name));
			RemoveBlock(key, false);
			f->vob = EffectVob(visual, CellCentre(x, y, z));
			if (visual)
				visual->Release(); // (the object keeps it)
			f->kind = kind;
			f->pos = f->last = CellBottom(x, y, z);
			f->vy = 0.0f;
			f->time = 0;
			Log("falling: %s from %d %d %d", b.id, x, y, z);
		}

		static void FallTick(Falling& f)
		{
			f.last = f.pos;
			f.vy -= 0.04f;
			float bottom = f.pos[1] + f.vy * BLOCK;
			int x, y, z;
			CellOf(f.pos + zVEC3(0.0f, 1.0f, 0.0f), x, y, z);
			int lowest = (int)floorf((bottom - 1.0f) / CELL);
			for (int cy = y - 1; cy >= lowest; cy--) // what it lands on, on its way down
				if (Solid(x, cy, z))
				{
					DropVob(f.vob);
					if (!blocks.count(CellKey(x, cy + 1, z)))
						PutBlock(f.kind, x, cy + 1, z);
					return;
				}
			f.pos[1] = bottom;
			f.vy *= 0.98f;
			if (++f.time > 600)
				DropVob(f.vob);
		}

		// whoever stands in fire or lava (Entity.baseTick, BaseFireBlock.entityInside, Entity.lavaHurt)
		static void McHurt(oCNpc* npc, float points, int type, int protection)
		{
			float damage = points / 20.0f * npc->attribute[NPC_ATR_HITPOINTSMAX];
			if (npc == oCNpc::player && damage < npc->attribute[NPC_ATR_HITPOINTS]) // Steve: Minecraft's hurt, no stumble
				npc->attribute[NPC_ATR_HITPOINTS] -= (int)ceilf(damage);
			else
				npc->OnDamage(npc, nullptr, damage + npc->protection[protection], type, npc->GetPositionWorld());
		}

		static void EntityTicks(oCNpc* hero)
		{
			oCWorld* game = ogame ? ogame->GetGameWorld() : nullptr;
			for (auto& b : burning)
				b.second.seen = false;
			float now = Now();
			heroInLava = heroDeepLava = false;
			for (zCListSort<oCNpc>* node = game && game->voblist_npcs ? game->voblist_npcs->next : nullptr; node; node = node->next)
			{
				oCNpc* npc = node->data;
				if (!npc || npc->IsDead())
					continue;
				zTBBox3D box = npc->bbox3D;
				float feet = npc == hero && npc->human_ai ? npc->human_ai->feetY : box.mins[1];
				zVEC3 pos = npc->GetPositionWorld();
				int x, y, z;
				CellOf(zVEC3(pos[0], feet + 5.0f, pos[2]), x, y, z);
				bool inFire = fireCells.count(CellKey(x, y, z)) || fireCells.count(CellKey(x, y + 1, z));
				bool inLava = false;
				for (int dy = 0; dy <= 1 && !inLava; dy++)
				{
					auto it = lavaCells.find(CellKey(x, y + dy, z));
					if (it != lavaCells.end())
					{
						float top = (y + dy) * CELL + (it->second.mesh >= 9 ? 1.0f : it->second.amount / 9.0f) * CELL;
						inLava = feet < top;
						if (npc == hero)
						{
							heroInLava = inLava;
							heroDeepLava = top - feet > 0.4f * BLOCK;
						}
					}
				}
				auto bit = burning.find(npc);
				int state = BodyState(npc);
				bool wet = state == BS_SWIM || state == BS_DIVE || (npc == hero && WaterDepth(hero) > 0.0f);
				if (!inFire && !inLava && bit == burning.end())
					continue;
				Burning& b = burning[npc];
				b.seen = true;
				if (wet)
					b.fireTicks = 0;
				if ((inFire || inLava) && now >= b.hurtUntil)
				{
					b.hurtUntil = now + 10 * TICK;
					McHurt(npc, inLava ? 4.0f : 1.0f, oEDamageType_Fire, oEDamageIndex_Fire);
					if (inLava)
						PlaySound("MC_FIZZ.WAV", npc);
					Log("%s: %s takes %d of Minecraft's 20", inLava ? "lava" : "fire", npc->GetInstanceName().ToChar(), inLava ? 4 : 1);
				}
				if (!wet && (inFire || inLava))
				{
					int ticks = inLava ? 300 : 160;
					b.fireTicks = b.fireTicks > ticks ? b.fireTicks : ticks;
				}
				if (b.fireTicks > 0 && --b.fireTicks % 20 == 0)
					McHurt(npc, 1.0f, oEDamageType_Fire, oEDamageIndex_Fire);
			}
			for (auto it = burning.begin(); it != burning.end();)
				if (!it->second.seen || it->second.fireTicks <= 0)
				{
					DropVob(it->second.flames);
					it = burning.erase(it);
				}
				else
					++it;
		}

		// lava, fire and the flames on whoever burns glow (Minecraft's light 15): Gothic's light for them is
		// one white ambient light (the hook on zCRenderLightContainer::CollectLights)
		static bool FullBright(zCVob* vob)
		{
			zCVisual* visual = vob->GetVisual();
			if (!visual)
				return false;
			if (visual == fireVisual)
				return true;
			for (zCVisual* v : lavaVisuals)
				if (v && v == visual)
					return true;
			return false;
		}

		static bool HeroBurning(oCNpc* hero)
		{
			auto it = burning.find(hero);
			return it != burning.end() && it->second.fireTicks > 0;
		}

		static zCTexture* FireOverlayFrame() { return fireFrames[1][0] ? fireFrames[1][(int)(Now() * 20.0f) % 32] : nullptr; }

		static bool EyesInLava(oCNpc* hero)
		{
			zVEC3 eye = Eyes(hero);
			int x, y, z;
			CellOf(eye, x, y, z);
			auto it = lavaCells.find(CellKey(x, y, z));
			return it != lavaCells.end() && eye[1] < y * CELL + (it->second.mesh >= 9 ? 1.0f : it->second.amount / 9.0f) * CELL;
		}

		// animated textures: one frame for all (fire 20 a second, lava's still there and back every 2
		// ticks, its flow every 3), and full bright light on fire, lava and the flames on whoever burns
		static void AnimateEffects(oCNpc* hero)
		{
			// the frames make_mc_blocks.py wrote (as many as the player's Minecraft has: its lava_flow has
			// 16 in some versions, 32 in others)
			static int fireCount = 0, stillCount = 0, flowCount = 0;
			if (!fireCount && !stillCount && !flowCount)
			{
				auto load = [](const char* prefix, zCTexture** frames, int most)
				{
					int count = 0;
					for (int i = 0; i < most; i++)
					{
						char name[40], path[MAX_PATH];
						sprintf_s(name, "%s%d", prefix, i);
						sprintf_s(path, "_work\\DATA\\TEXTURES\\_COMPILED\\%s-C.TEX", name);
						if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)
							break;
						strcat_s(name, ".TGA");
						frames[count++] = LoadTexture(name);
					}
					return count;
				};
				fireCount = load("MC_FIRE0_", fireFrames[0], 32);
				int fire1 = load("MC_FIRE1_", fireFrames[1], 32);
				fireCount = fire1 < fireCount ? fire1 : fireCount;
				stillCount = load("MC_LAVA_STILL_", lavaStill, 20);
				flowCount = load("MC_LAVA_FLOW_", lavaFlow, 32);
				Log("animations: fire %d frames, lava %d still and %d flowing", fireCount, stillCount, flowCount);
			}
			int ticks = (int)(Now() * 20.0f);
			int fire = fireCount ? ticks % fireCount : 0, flow = flowCount ? ticks / 3 % flowCount : 0;
			int bounce = stillCount > 1 ? 2 * stillCount - 2 : 1, still = ticks / 2 % bounce; // there and back
			still = still < stillCount ? still : bounce - still;
			auto frames = [&](zCVisual* visual)
			{
				zCProgMeshProto* mesh = zDYNAMIC_CAST<zCProgMeshProto>(visual);
				for (int i = 0; mesh && i < mesh->numSubMeshes; i++)
					if (zCMaterial* m = mesh->subMeshList[i].material)
					{
						const char* name = m->GetName().ToChar();
						zCTexture* frame = !strncmp(name, "MC_FIRE0", 8) ? fireFrames[0][fire] : !strncmp(name, "MC_FIRE1", 8) ? fireFrames[1][fire]
							: !strncmp(name, "MC_LAVA_STILL", 13) ? lavaStill[still] : !strncmp(name, "MC_LAVA_FLOW", 12) ? lavaFlow[flow] : nullptr;
						if (frame && m->texture != frame)
							m->SetTexture(frame);
						if (!strncmp(name, "MC_FIRE", 7))
							m->rndAlphaBlendFunc = zRND_ALPHA_FUNC_TEST;
					}
			};
			if (!fireVisual)
				fireVisual = zCVisual::LoadVisual(zSTRING("MC_FIRE.3DS")); // kept
			frames(fireVisual);
			for (zCVisual* v : lavaVisuals)
				if (v)
					frames(v);
			zCOLOR bright = Argb(255, 255, 255, 255);
			auto light = [&](zCVob* vob)
			{
				if (vob)
				{
					vob->lightColorStat = bright;
					vob->lightColorStatDirty = FALSE;
				}
			};
			for (auto& f : fireCells)
				light(f.second.vob);
			for (auto& l : lavaCells)
				light(l.second.vob);
			for (auto& b : burning) // the flames on whoever burns (not on Steve in first person: the screen's)
			{
				oCNpc* npc = b.first;
				bool show = b.second.fireTicks > 0 && !(npc == hero && view == FIRST_PERSON && cameraActive);
				if (show && !b.second.flames)
					b.second.flames = EffectVob(fireVisual, npc->GetPositionWorld());
				if (!show)
					DropVob(b.second.flames);
				if (b.second.flames)
				{
					zVEC3 pos = npc->GetPositionWorld();
					b.second.flames->SetPositionWorld(zVEC3(pos[0], npc->bbox3D.mins[1], pos[2]));
					light(b.second.flames);
				}
			}
		}

		// Each world tick: the scheduled fire and lava ticks, lava's random ticks (Minecraft's 3 a section
		// of 4096 cells a tick), falling blocks, whoever stands in fire or lava.
		static void PhysicsTicks(oCNpc* hero)
		{
			float dt = ztimer->frameTimeFloat / 1000.0f;
			physicsClock += dt > 0.25f ? 0.25f : dt;
			while (physicsClock >= TICK)
			{
				physicsClock -= TICK;
				float now = Now();
				std::vector<long long> due;
				for (auto& f : fireCells)
					if (f.second.due <= now)
						due.push_back(f.first);
				for (long long key : due)
				{
					int x, y, z;
					CellFromKey(key, x, y, z);
					FireTick(hero, x, y, z);
				}
				due.clear();
				std::vector<long long> random;
				for (auto& l : lavaCells)
				{
					if (l.second.due >= 0.0f && l.second.due <= now)
						due.push_back(l.first);
					if (rand() % 4096 < 3)
						random.push_back(l.first);
				}
				for (long long key : due)
				{
					int x, y, z;
					CellFromKey(key, x, y, z);
					LavaTick(x, y, z);
				}
				for (long long key : random)
				{
					int x, y, z;
					CellFromKey(key, x, y, z);
					if (lavaCells.count(key))
						LavaRandomTick(x, y, z);
				}
				std::vector<FallCheck> checks;
				checks.swap(fallChecks);
				for (FallCheck& c : checks)
					if (--c.ticks <= 0)
					{
						int x, y, z;
						CellFromKey(c.key, x, y, z);
						CheckFall(x, y, z);
					}
					else
						fallChecks.push_back(c);
				for (Falling& f : fallingBlocks)
					if (f.vob)
						FallTick(f);
				EntityTicks(hero);
			}
			for (Falling& f : fallingBlocks)
				if (f.vob)
					f.vob->SetPositionWorld(f.last + (f.pos - f.last) * (physicsClock / TICK) + zVEC3(0.0f, CELL * 0.5f, 0.0f));
			AnimateEffects(hero);
		}

		// where a thing put against the crosshair's cell side goes (a block, fire, lava): the cell next
		// to the block or to the spot of Gothic's world it points at, out of Gothic's solid cells
		static bool TargetCell(oCNpc* hero, int& x, int& y, int& z, Pick& p)
		{
			p = PickBlock(hero, REACH);
			if (!p.hit)
				return false;
			if (p.block)
			{
				x = p.x + (int)p.normal[0];
				y = p.y + (int)p.normal[1];
				z = p.z + (int)p.normal[2];
				return true;
			}
			CellOf(p.point + p.normal * 1.0f, x, y, z);
			int axis = fabsf(p.normal[1]) >= fabsf(p.normal[0]) && fabsf(p.normal[1]) >= fabsf(p.normal[2]) ? 1
				: (fabsf(p.normal[0]) >= fabsf(p.normal[2]) ? 0 : 2);
			int step = p.normal[axis] >= 0.0f ? 1 : -1;
			for (int i = 0; i < 2 && GothicSolid(x, y, z); i++) // out of the ground on the grid
			{
				x += axis == 0 ? step : 0;
				y += axis == 1 ? step : 0;
				z += axis == 2 ? step : 0;
			}
			return !GothicSolid(x, y, z);
		}

		static void Ignite(oCNpc* hero)
		{
			int x, y, z;
			Pick p;
			if (!TargetCell(hero, x, y, z, p))
				return;
			swingTicks = 0;
			PlaySound("MC_IGNITE.WAV", hero);
			if (p.block)
			{
				long long key = CellKey(p.x, p.y, p.z);
				auto kind = blockKinds.find(key);
				if (kind != blockKinds.end() && !strcmp(blockTypes[kind->second].id, "tnt"))
				{
					PrimeTnt(hero, key, 80);
					return;
				}
			}
			SetFire(x, y, z, 0);
			Log("flint and steel: fire at %d %d %d%s", x, y, z, fireCells.count(CellKey(x, y, z)) ? "" : " (it can't burn there)");
		}

		// the lava bucket puts a source where a block would go; the bucket takes a source back (Minecraft's
		// pick through fluids: the first source on the crosshair's way)
		static void PourLava(oCNpc* hero)
		{
			int x, y, z;
			Pick p;
			if (!TargetCell(hero, x, y, z, p) || Solid(x, y, z))
				return;
			SetLava(x, y, z, 8, false, true);
			char sound[32];
			sprintf_s(sound, "MC_BUCKET_EMPTY_LAVA%d.WAV", 1 + rand() % 3);
			PlaySound(sound, hero);
			Log("lava bucket: a source at %d %d %d", x, y, z);
		}

		static void FillBucket(oCNpc* hero)
		{
			zVEC3 from = Eyes(hero), dir = Look();
			for (float t = 0.0f; t < REACH; t += 10.0f)
			{
				int x, y, z;
				CellOf(from + dir * t, x, y, z);
				if (LavaSource(x, y, z))
				{
					RemoveLava(CellKey(x, y, z));
					ShowLava(x, y - 1, z);
					CellChanged(x, y, z);
					char sound[32];
					sprintf_s(sound, "MC_BUCKET_FILL_LAVA%d.WAV", 1 + rand() % 3);
					PlaySound(sound, hero);
					Log("bucket: lava taken at %d %d %d", x, y, z);
					return;
				}
				if (Solid(x, y, z))
					return;
			}
		}

		// --- TNT and flint and steel ---------------------------------------------------------------------
		// Flint and steel (FlintAndSteelItem) on TNT lights it (TntBlock.prime: the block becomes primed
		// TNT, with Minecraft's fuse sound); on anything else it lights a fire on the face's side (Gothic's
		// fire for 8 seconds, a point of Minecraft's 20 every half second to whoever stands in it,
		// BaseFireBlock.entityInside), with Minecraft's sound. Primed TNT (PrimedTnt) hops 0.2 blocks a
		// tick up and a little aside, falls 0.04 a tick faster losing 2 % a tick (on the floor 30 % aside),
		// flashes white every 5 ticks (TntMinecartRenderer) and after 80 ticks explodes with power 4
		// (Explode); TNT an explosion reaches is lit with 10 to 29 ticks to go. Neither is saved.
		struct Tnt
		{
			zCVob* vob = nullptr; // the plugin's reference
			zVEC3 pos, last, vel; // the bottom's middle (cm), a tick ago; blocks a tick
			int fuse = 0;
			bool flash = false;
		};
		static Tnt tnts[16];
		static float tntClock = 0.0f;
		static zCVisual* tntVisuals[2] = {};

		static zCVisual* TntVisual(bool flash)
		{
			if (!tntVisuals[flash])
			{
				tntVisuals[flash] = zCVisual::LoadVisual(zSTRING(flash ? "MC_TNT_FLASH.3DS" : "MC_BLK_TNT.3DS")); // kept
				KeepDetail(tntVisuals[flash]);
			}
			return tntVisuals[flash];
		}

		static void RemoveTnt(Tnt& t)
		{
			if (!t.vob)
				return;
			if (zCWorld* world = t.vob->GetHomeWorld())
				world->RemoveVob(t.vob);
			t.vob->Release();
			t.vob = nullptr;
		}

		static void PrimeTnt(oCNpc* hero, long long key, int fuse)
		{
			auto it = blocks.find(key);
			oCWorld* world = ogame ? ogame->GetGameWorld() : nullptr;
			if (it == blocks.end() || !world)
				return;
			Tnt* slot = nullptr;
			for (Tnt& t : tnts)
				if (!t.vob && !slot)
					slot = &t;
			if (!slot)
				return;
			zVEC3 centre = it->second->GetPositionWorld();
			RemoveBlock(key, false);
			zCVob* vob = new zCVob();
			vob->dontWriteIntoArchive = TRUE;
			vob->SetCollDetStat(FALSE);
			vob->SetCollDetDyn(FALSE);
			vob->SetVisual(TntVisual(false));
			vob->SetPositionWorld(centre);
			world->AddVob(vob);
			vob->SetCollDetStat(FALSE);
			vob->SetCollDetDyn(FALSE);
			float turn = rand() / (float)RAND_MAX * 2.0f * PI;
			*slot = Tnt();
			slot->vob = vob;
			slot->pos = slot->last = centre - zVEC3(0.0f, CELL * 0.5f, 0.0f);
			slot->vel = zVEC3(-sinf(turn) * 0.02f, 0.2f, -cosf(turn) * 0.02f);
			slot->fuse = fuse;
			PlaySound("MC_FUSE.WAV", vob);
			Log("tnt: lit, %d ticks", fuse);
		}

		// --- Minecraft's explosion (ServerExplosion; the client's ClientPacketListener.handleExplosion) ---
		// Of `power` (a creeper's 3, TNT's 4) at `at`:
		//  - the blocks: rays from it to the 16 x 16 x 16 points on a cube's surface, each starting with
		//    power x (0.7 to 1.3) and losing 0.225 every 0.3 blocks and (resistance + 0.3) x 0.3 in a block
		//    (the table's blast resistance); a block a ray still has power in goes (TNT is lit, 10 to 29
		//    ticks to go). Gothic's own world stays.
		//  - everyone within 2 x power blocks (d of that as a share) is seen by it from some of their box
		//    (getSeenPercent: a grid of points through it, those with no wall or block in between), and
		//    with impact = (1 - d) x that share takes (impact^2 + impact) / 2 x 7 x 2 power + 1 of
		//    Minecraft's 20 health and is pushed by impact blocks a tick away from it (from the eyes; lit
		//    TNT from its foot, so a pile of it flies apart). Gothic's characters take it Gothic's way, the
		//    push as Gothic's throw when it is strong; Steve without Gothic's hurt (unless it kills him),
		//    pushed by Minecraft's movement.
		//  - Minecraft's bang where it is (heard 64 blocks away, its pitch 0.7 in the WAV), and its big puff
		//    (HugeExplosionSeedParticle: for 8 ticks 6 puffs a tick within 4 blocks, each HugeExplosionParticle
		//    6 to 9 ticks through explosion_0 to 15, 4 blocks across shrinking to 2 as the seed ages, grey).
		struct Puff
		{
			zCVob* vob = nullptr;   // the plugin's references
			zCDecal* decal = nullptr;
			int age = 0, lifetime = 0;
			bool live = false;
		};
		static Puff puffs[160];
		struct Seed
		{
			zVEC3 at;
			int age = -1;
		};
		static Seed seeds[8];
		static zCTexture* puffFrames[16] = {};
		static float puffClock = 0.0f;
		static zCVob* boomVobs[4] = {};
		static int nextBoom = 0;

		static float Random01() { return rand() / (float)RAND_MAX; }

		static void SpawnPuff(zCWorld* world, const zVEC3& at, float size, const zCOLOR* tint = nullptr)
		{
			Puff* p = nullptr;
			for (Puff& q : puffs)
				if (!q.live)
				{
					p = &q;
					break;
				}
			if (!p || !world)
				return;
			if (!puffFrames[0])
				for (int i = 0; i < 16; i++)
				{
					char name[32];
					sprintf_s(name, "MC_EXPLOSION_%d.TGA", i);
					puffFrames[i] = LoadTexture(name);
				}
			if (!p->vob)
			{
				p->decal = new zCDecal();
				p->decal->decalMaterial->rndAlphaBlendFunc = zRND_ALPHA_FUNC_BLEND;
				p->vob = new zCVob();
				p->vob->dontWriteIntoArchive = TRUE;
				p->vob->visualCamAlign = (zTVisualCamAlign)zVISUAL_CAMALIGN_FULL;
				p->vob->SetCollDetStat(FALSE);
				p->vob->SetCollDetDyn(FALSE);
				p->vob->SetVisual(p->decal);
			}
			if (p->vob->GetHomeWorld() != world)
			{
				if (zCWorld* old = p->vob->GetHomeWorld())
					old->RemoveVob(p->vob);
				world->AddVob(p->vob);
				p->vob->SetCollDetStat(FALSE);
				p->vob->SetCollDetDyn(FALSE);
			}
			int grey = (int)((Random01() * 0.6f + 0.4f) * 255.0f);
			p->decal->decalMaterial->color = tint ? *tint : Argb(255, grey, grey, grey);
			p->decal->SetDecalDim(size * BLOCK, size * BLOCK);
			p->decal->SetTexture(puffFrames[0]);
			p->vob->SetPositionWorld(at);
			p->vob->showVisual = TRUE;
			p->age = 0;
			p->lifetime = 6 + rand() % 4;
			p->live = true;
		}

		static void UpdatePuffs(zCWorld* world)
		{
			float dt = ztimer->frameTimeFloat / 1000.0f;
			puffClock += dt > 0.25f ? 0.25f : dt;
			while (puffClock >= TICK)
			{
				puffClock -= TICK;
				for (Puff& p : puffs)
					if (p.live)
					{
						if (++p.age >= p.lifetime)
						{
							p.live = false;
							p.vob->showVisual = FALSE;
						}
						else
							p.decal->SetTexture(puffFrames[p.age * 15 / p.lifetime]);
					}
				for (Seed& s : seeds)
					if (s.age >= 0)
					{
						for (int i = 0; i < 6; i++)
						{
							zVEC3 at = s.at + zVEC3(Random01() - Random01(), Random01() - Random01(), Random01() - Random01()) * (4.0f * BLOCK);
							SpawnPuff(world, at, 2.0f * (1.0f - s.age / 8.0f * 0.5f));
						}
						if (++s.age >= 8)
							s.age = -1;
					}
			}
		}

		// How much of a box sees `centre` (ServerExplosion.getSeenPercent), Gothic's world and blocks in the way
		static float SeenPercent(zCWorld* world, const zVEC3& centre, const zTBBox3D& box)
		{
			if (!world)
				return 1.0f;
			float step[3];
			for (int i = 0; i < 3; i++)
			{
				step[i] = 1.0f / ((box.maxs[i] - box.mins[i]) / BLOCK * 2.0f + 1.0f);
				step[i] = step[i] < 0.25f ? 0.25f : step[i]; // at most 5 points along
			}
			float xOffset = (1.0f - floorf(1.0f / step[0]) * step[0]) / 2.0f, zOffset = (1.0f - floorf(1.0f / step[2]) * step[2]) / 2.0f;
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_VOB_IGNORE_CHARACTER | zTRACERAY_VOB_IGNORE_NO_CD_DYN;
			int hits = 0, count = 0;
			for (float xx = 0.0f; xx <= 1.0f; xx += step[0])
				for (float yy = 0.0f; yy <= 1.0f; yy += step[1])
					for (float zz = 0.0f; zz <= 1.0f; zz += step[2])
					{
						zVEC3 from(Lerp(box.mins[0], box.maxs[0], xx) + xOffset * BLOCK, Lerp(box.mins[1], box.maxs[1], yy),
							Lerp(box.mins[2], box.maxs[2], zz) + zOffset * BLOCK);
						if (!(world->TraceRayNearestHit(from, centre - from, (zCVob*)nullptr, flags) && world->traceRayReport.foundHit))
							hits++;
						count++;
					}
			return count ? (float)hits / count : 0.0f;
		}

		// the blocks the explosion's rays break
		static void ExplodedBlocks(const zVEC3& at, float power, std::vector<long long>& out)
		{
			float cx = at[0] / CELL, cy = at[1] / CELL, cz = at[2] / CELL;
			for (int xx = 0; xx < 16; xx++)
				for (int yy = 0; yy < 16; yy++)
					for (int zz = 0; zz < 16; zz++)
					{
						if (xx != 0 && xx != 15 && yy != 0 && yy != 15 && zz != 0 && zz != 15)
							continue;
						float dx = xx / 15.0f * 2.0f - 1.0f, dy = yy / 15.0f * 2.0f - 1.0f, dz = zz / 15.0f * 2.0f - 1.0f;
						float d = sqrtf(dx * dx + dy * dy + dz * dz);
						dx /= d;
						dy /= d;
						dz /= d;
						float remaining = power * (0.7f + Random01() * 0.6f);
						float x = cx, y = cy, z = cz;
						while (remaining > 0.0f)
						{
							int bx = (int)floorf(x), by = (int)floorf(y), bz = (int)floorf(z);
							long long key = CellKey(bx, by, bz);
							auto it = blockKinds.find(key);
							if (it != blockKinds.end())
							{
								remaining -= (blockTypes[it->second].resistance + 0.3f) * 0.3f;
								if (remaining > 0.0f)
									out.push_back(key);
							}
							else if (lavaCells.count(key)) // lava's resistance 100
								remaining -= (100.0f + 0.3f) * 0.3f;
							else if (GothicSolid(bx, by, bz)) // Gothic's ground as stone, never broken
								remaining -= (6.0f + 0.3f) * 0.3f;
							else if (fireCells.count(key))
								RemoveFire(key);
							x += dx * 0.3f;
							y += dy * 0.3f;
							z += dz * 0.3f;
							remaining -= 0.22500001f;
						}
					}
			std::sort(out.begin(), out.end());
			out.erase(std::unique(out.begin(), out.end()), out.end());
		}

		static void Explode(oCNpc* hero, const zVEC3& at, float power, oCNpc* source)
		{
			zCWorld* world = hero->GetHomeWorld();
			if (!world)
				return;
			Log("explosion: power %.0f at %.0f %.0f %.0f", power, at[0], at[1], at[2]);
			// the bang, where it is
			zCVob*& boom = boomVobs[nextBoom++ % 4];
			if (!boom)
			{
				boom = new zCVob();
				boom->dontWriteIntoArchive = TRUE;
				boom->SetCollDetStat(FALSE);
				boom->SetCollDetDyn(FALSE);
			}
			if (boom->GetHomeWorld() != world)
			{
				if (zCWorld* old = boom->GetHomeWorld())
					old->RemoveVob(boom);
				world->AddVob(boom);
				boom->SetCollDetStat(FALSE);
				boom->SetCollDetDyn(FALSE);
			}
			boom->SetPositionWorld(at);
			if (zsound)
			{
				char bang[24];
				sprintf_s(bang, "MC_EXPLODE%d.WAV", 1 + rand() % 4);
				if (zCSoundFX* fx = zsound->LoadSoundFX(zSTRING(bang)))
				{
					zCSoundSystem::zTSound3DParams params;
					params.SetDefaults();
					params.radius = 64.0f * BLOCK;
					zsound->PlaySound3D(fx, boom, zSND_SLOT_NONE, &params);
				}
			}
			for (Seed& s : seeds)
				if (s.age < 0)
				{
					s.at = at;
					s.age = 0;
					break;
				}
			// the blocks
			std::vector<long long> broken;
			ExplodedBlocks(at, power, broken);
			int lit = 0;
			for (long long key : broken)
			{
				auto it = blockKinds.find(key);
				if (it == blockKinds.end())
					continue;
				if (!strcmp(blockTypes[it->second].id, "tnt"))
				{
					PrimeTnt(hero, key, 10 + rand() % 20);
					lit++;
				}
				else
					RemoveBlock(key, false);
			}
			// whoever is near: the characters, Steve among them, and lit TNT
			float radius = power * 2.0f;
			oCWorld* game = ogame ? ogame->GetGameWorld() : nullptr;
			int hurt = 0;
			for (zCListSort<oCNpc>* node = game && game->voblist_npcs ? game->voblist_npcs->next : nullptr; node; node = node->next)
			{
				oCNpc* npc = node->data;
				if (!npc || npc == source || npc->IsDead())
					continue;
				zTBBox3D box = npc->bbox3D;
				zVEC3 centre = (box.mins + box.maxs) * 0.5f;
				for (int i = 0; i < 3; i += 2) // a body is narrower than its box (the arms)
				{
					box.mins[i] = centre[i] + (box.mins[i] - centre[i]) * 0.6f;
					box.maxs[i] = centre[i] + (box.maxs[i] - centre[i]) * 0.6f;
				}
				bool steve = npc == hero && npc->human_ai;
				float feet = steve ? npc->human_ai->feetY : box.mins[1];
				zVEC3 foot(centre[0], feet, centre[2]);
				float d = (foot - at).Length() / BLOCK / radius;
				if (d > 1.0f)
					continue;
				zVEC3 eye(centre[0], steve ? feet + (sneaking ? 127.0f : 162.0f) : feet + (box.maxs[1] - feet) * 0.85f, centre[2]);
				zVEC3 dir = eye - at;
				float len = dir.Length();
				dir = len > 0.001f ? dir * (1.0f / len) : zVEC3(0.0f, 1.0f, 0.0f);
				float seen = SeenPercent(world, at, box);
				float impact = (1.0f - d) * seen;
				float points = (impact * impact + impact) / 2.0f * 7.0f * radius + 1.0f;
				float damage = points / 20.0f * npc->attribute[NPC_ATR_HITPOINTSMAX];
				hurt++;
				if (steve) // Minecraft's: no stumble, the push his movement's
				{
					if (damage < npc->attribute[NPC_ATR_HITPOINTS])
						npc->attribute[NPC_ATR_HITPOINTS] -= (int)ceilf(damage);
					else
						npc->OnDamage(boom, source, damage + npc->protection[oEDamageIndex_Blunt], oEDamageType_Blunt, at);
					Knock(npc, dir * impact);
				}
				else
				{
					int type = oEDamageType_Blunt | (impact > 0.4f ? oEDamageType_Fly : 0);
					npc->OnDamage(boom, source ? source : hero, damage + npc->protection[oEDamageIndex_Blunt], type, at);
				}
				Log("explosion: %s at %.1f of %.0f blocks, seen %.0f %% -> %.1f of Minecraft's 20 (%.0f), pushed %.2f", npc->GetInstanceName().ToChar(),
					d * radius, radius, seen * 100.0f, points, damage, impact);
			}
			for (Tnt& t : tnts)
				if (t.vob)
				{
					float d = (t.pos - at).Length() / BLOCK / radius;
					if (d > 1.0f)
						continue;
					zTBBox3D box;
					box.mins = t.pos - zVEC3(49.0f, 0.0f, 49.0f);
					box.maxs = t.pos + zVEC3(49.0f, 98.0f, 49.0f);
					zVEC3 dir = t.pos - at;
					float len = dir.Length();
					dir = len > 0.001f ? dir * (1.0f / len) : zVEC3(0.0f, 1.0f, 0.0f);
					t.vel += dir * ((1.0f - d) * SeenPercent(world, at, box));
				}
			DragonExplosion(hero, at, radius);
			Log("explosion: %d blocks gone, %d TNT lit, %d hurt", (int)broken.size() - lit, lit, hurt);
		}

		// what lit TNT meets on its way (Gothic's world or a block)
		static bool TntHit(Tnt& t, const zVEC3& from, const zVEC3& ray, zVEC3& point)
		{
			zCWorld* world = t.vob->GetHomeWorld();
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_VOB_IGNORE_CHARACTER | zTRACERAY_VOB_IGNORE_NO_CD_DYN;
			if (!world || !world->TraceRayNearestHit(from, ray, t.vob, flags) || !world->traceRayReport.foundHit)
				return false;
			point = world->traceRayReport.foundIntersection;
			return true;
		}

		// Entity.move for its box (0.98 blocks): up or down first, then along. Down is looked for from its
		// top, so TNT lit in a block sunk into Gothic's ground (blocks sit on a grid of metres) comes out
		// on top of the ground instead of falling through it.
		static void TntTick(oCNpc* hero, Tnt& t)
		{
			t.last = t.pos;
			t.vel[1] -= 0.04f;
			zVEC3 move = t.vel * BLOCK, hit;
			const float SIZE = CELL * 0.98f;
			bool ground = false;
			if (move[1] < 0.0f && TntHit(t, t.pos + zVEC3(0.0f, SIZE, 0.0f), zVEC3(0.0f, move[1] - SIZE, 0.0f), hit))
			{
				move[1] = hit[1] - t.pos[1];
				ground = true;
			}
			else if (move[1] > 0.0f && TntHit(t, t.pos + zVEC3(0.0f, SIZE * 0.5f, 0.0f), zVEC3(0.0f, move[1] + SIZE * 0.5f, 0.0f), hit))
			{
				move[1] = hit[1] - SIZE - t.pos[1];
				move[1] = move[1] < 0.0f ? 0.0f : move[1];
				t.vel[1] = 0.0f;
			}
			zVEC3 along(move[0], 0.0f, move[2]);
			float len = along.Length();
			if (len > 0.01f)
			{
				zVEC3 dir = along * (1.0f / len), from = t.pos + zVEC3(0.0f, SIZE * 0.5f + move[1], 0.0f);
				if (TntHit(t, from, dir * (len + SIZE * 0.5f), hit))
				{
					float room = (hit - from).Length() - SIZE * 0.5f;
					room = room < 0.0f ? 0.0f : room;
					move[0] = dir[0] * room;
					move[2] = dir[2] * room;
					t.vel[0] = t.vel[2] = 0.0f;
				}
			}
			t.pos += move;
			t.vel = t.vel * 0.98f;
			if (ground)
			{
				t.vel[0] *= 0.7f;
				t.vel[1] = 0.0f;
				t.vel[2] *= 0.7f;
			}
			if (--t.fuse <= 0)
			{
				zVEC3 at = t.pos + zVEC3(0.0f, 0.0625f * BLOCK, 0.0f);
				RemoveTnt(t);
				Explode(hero, at, 4.0f, nullptr);
				return;
			}
			bool flash = t.fuse / 5 % 2 == 0;
			if (flash != t.flash)
			{
				t.flash = flash;
				t.vob->SetVisual(TntVisual(flash));
			}
		}

		static void UpdateTnt(oCNpc* hero)
		{
			UpdatePuffs(hero->GetHomeWorld());
			float dt = ztimer->frameTimeFloat / 1000.0f;
			tntClock += dt > 0.25f ? 0.25f : dt;
			while (tntClock >= TICK)
			{
				tntClock -= TICK;
				for (Tnt& t : tnts)
					if (t.vob)
						TntTick(hero, t);
			}
			for (Tnt& t : tnts)
				if (t.vob)
					t.vob->SetPositionWorld(t.last + (t.pos - t.last) * (tntClock / TICK) + zVEC3(0.0f, CELL * 0.5f, 0.0f));
		}

		// --- first person: Minecraft's hand (FirstPersonHandsAndItemsRenderer) ---------------------------
		// In first person Minecraft draws nothing of him but the hand: his bare right arm, or what it holds
		// (a block, an egg, the bow, the arrow, flint and steel; for Gothic's drawn weapon or spell the
		// item the hotbar shows for it), make_mc_items.py's MC_FP_* meshes as one object put in front of
		// the camera each frame after it: the sway behind the turning view (xBob, yBob follow the view
		// halfway a tick; the hand turns by a tenth of what the view is ahead of them), the attack's swing
		// (renderPlayerArm; swingArm for an item), the bow's draw, and the hand's dip when what it holds
		// changes (FirstPersonHandsAndItems.tick: down to nothing and up again, 0.4 a tick; after an attack
		// up with the cooldown cubed). Minecraft's view space (blocks, -z ahead) is the camera's frame with
		// z the other way; the meshes are a quarter of the size and distance (FP_UNIT), and the camera's
		// near clip is 5 cm meanwhile (Gothic's 35 cm would cut the hand). The block he points at gets
		// Minecraft's black outline in every view.
		static const float FP_UNIT = 25.0f;
		static zCVob* handVob = nullptr; // the plugin's reference
		struct HandVisual
		{
			char name[48];
			zCVisual* visual;
		};
		static HandVisual handVisuals[64];
		static int handVisualCount = 0;
		static char handShown[48] = "", handShownBase[40] = "";
		static float handHeight = 0.0f, handHeightOld = 0.0f, handClock = 0.0f;
		static float xBob = 0.0f, xBobOld = 0.0f, yBob = 0.0f, yBobOld = 0.0f;
		static bool bobsSet = false;

		static zCVisual* HandVisualNamed(const char* name)
		{
			for (int i = 0; i < handVisualCount; i++)
				if (!strcmp(handVisuals[i].name, name))
					return handVisuals[i].visual;
			if (handVisualCount >= 64)
				return nullptr;
			HandVisual& h = handVisuals[handVisualCount++];
			strcpy_s(h.name, name);
			h.visual = zCVisual::LoadVisual(zSTRING(name)); // kept
			KeepDetail(h.visual);
			if (!h.visual)
				Log("hand: no %s", name);
			return h.visual;
		}

		// what the hand shows (its mesh) and what that is (the bow's drawn stages are all the bow)
		static void HandThing(oCNpc* hero, char* mesh, size_t meshSize, char* base, size_t baseSize)
		{
			char id[40] = "ARM", stage[40] = "";
			int mode = hero->GetWeaponMode();
			const Entry& e = hotbar[selectedSlot];
			if (mode == NPC_WEAPON_NONE)
			{
				if (e.kind == ENTRY_BLOCK && e.value < blockTypeCount)
					sprintf_s(id, "BLK_%s", blockTypes[e.value].id);
				else if (e.kind == ENTRY_EGG && e.value < EGGS)
					sprintf_s(id, "%s_SPAWN_EGG", eggIds[e.value]);
				else if (e.kind == ENTRY_ITEM && e.value < MC_ITEMS)
				{
					strcpy_s(id, itemIds[e.value]);
					if (e.value == ITEM_BOW && useTicks >= 0)
						sprintf_s(stage, "BOW_PULLING_%d", useTicks >= 18 ? 2 : (useTicks >= 13 ? 1 : 0));
				}
			}
			else if (mode != NPC_WEAPON_FIST)
			{
				oCItem* item = nullptr;
				if (mode == NPC_WEAPON_MAG)
				{
					if (oCMag_Book* book = hero->GetSpellBook())
					{
						int nr = book->GetSelectedSpellNr();
						item = nr >= 0 ? book->GetSpellItem(nr) : nullptr;
					}
				}
				else
				{
					item = hero->GetWeapon();
					if (!item)
						item = mode == NPC_WEAPON_BOW || mode == NPC_WEAPON_CBOW ? hero->GetEquippedRangedWeapon() : hero->GetEquippedMeleeWeapon();
				}
				if (item)
				{
					zCOLOR tint;
					bool overlay;
					const char* icon = Icon(item, tint, overlay);
					if (!strncmp(icon, "item/", 5))
						strcpy_s(id, icon + 5);
				}
			}
			_strupr_s(id);
			_strupr_s(stage);
			strcpy_s(base, baseSize, id);
			sprintf_s(mesh, meshSize, "MC_FP_%s.3DS", stage[0] ? stage : id);
		}

		// Minecraft's pose stack in its view space (blocks, -z ahead): each step after the last, as
		// PoseStack.translate and rotateDegrees (x, y, z)
		struct HandPose
		{
			float r[3][3] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
			float t[3] = { 0.0f, 0.0f, 0.0f };

			HandPose& Move(float x, float y, float z)
			{
				for (int i = 0; i < 3; i++)
					t[i] += r[i][0] * x + r[i][1] * y + r[i][2] * z;
				return *this;
			}

			HandPose& Turn(int axis, float degrees)
			{
				float a = degrees * PI / 180.0f, c = cosf(a), s = sinf(a);
				float m[3][3] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
				int i = (axis + 1) % 3, j = (axis + 2) % 3;
				m[i][i] = c;
				m[i][j] = -s;
				m[j][i] = s;
				m[j][j] = c;
				float out[3][3];
				for (int a2 = 0; a2 < 3; a2++)
					for (int b = 0; b < 3; b++)
						out[a2][b] = r[a2][0] * m[0][b] + r[a2][1] * m[1][b] + r[a2][2] * m[2][b];
				memcpy(r, out, sizeof r);
				return *this;
			}
		};

		// LevelRenderer's outline of the block he points at: black, a hair outside its faces
		static void DrawOutline(oCNpc* hero)
		{
			if (!zlineCache || !HudVisible(hero) || inventoryOpen)
				return;
			Pick p = PickBlock(hero, REACH);
			if (!p.block)
				return;
			float x0 = p.x * CELL - 0.2f, y0 = p.y * CELL - 0.2f, z0 = p.z * CELL - 0.2f, s = CELL + 0.4f;
			zVEC3 c[8];
			for (int i = 0; i < 8; i++)
				c[i] = zVEC3(x0 + (i & 1 ? s : 0.0f), y0 + (i & 2 ? s : 0.0f), z0 + (i & 4 ? s : 0.0f));
			static const int EDGES[12][2] = { { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 }, { 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 },
				{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };
			for (const auto& e : EDGES)
				zlineCache->Line3D(c[e[0]], c[e[1]], Argb(255, 0, 0, 0), TRUE);
		}

		static void UpdateHand(zCAICamera* ai)
		{
			oCNpc* hero = oCNpc::player;
			if (cameraActive && hero)
				DrawOutline(hero);
			bool show = cameraActive && view == FIRST_PERSON && hero && ai && ai->camVob && !hero->IsDead();
			if (!show)
			{
				if (handVob && handVob->showVisual)
					handVob->showVisual = FALSE;
				bobsSet = false;
				return;
			}
			oCWorld* world = ogame ? ogame->GetGameWorld() : nullptr;
			if (!world)
				return;
			if (!handVob || handVob->GetHomeWorld() != world)
			{
				if (handVob)
				{
					if (zCWorld* old = handVob->GetHomeWorld())
						old->RemoveVob(handVob);
					handVob->Release();
				}
				handVob = new zCVob();
				handVob->dontWriteIntoArchive = TRUE;
				handVob->SetCollDetStat(FALSE);
				handVob->SetCollDetDyn(FALSE);
				handVob->SetVobName(zSTRING("MCHAND"));
				world->AddVob(handVob);
				handVob->SetCollDetStat(FALSE);
				handVob->SetCollDetDyn(FALSE);
			}
			float pitchDeg = pitch * 180.0f / PI, yawDeg = yaw * 180.0f / PI;
			if (!bobsSet)
			{
				xBob = xBobOld = pitchDeg;
				yBob = yBobOld = yawDeg;
				bobsSet = true;
			}
			char mesh[48], base[40];
			HandThing(hero, mesh, sizeof mesh, base, sizeof base);
			if (!HandVisualNamed(mesh)) // nothing of Minecraft's for it: the bare hand
			{
				strcpy_s(mesh, "MC_FP_ARM.3DS");
				strcpy_s(base, "ARM");
			}
			float dt = ztimer->frameTimeFloat / 1000.0f;
			handClock += dt > 0.25f ? 0.25f : dt;
			while (handClock >= TICK)
			{
				handClock -= TICK;
				handHeightOld = handHeight;
				xBobOld = xBob;
				yBobOld = yBob;
				xBob += (pitchDeg - xBob) * 0.5f;
				yBob += (yawDeg - yBob) * 0.5f;
				int mode = hero->GetWeaponMode();
				float speed = mode == NPC_WEAPON_1HS ? 1.6f : (mode == NPC_WEAPON_2HS ? 1.0f : 4.0f);
				float charge = (attackStrength + 1.0f) / (20.0f / speed);
				charge = charge > 1.0f ? 1.0f : charge;
				float target = strcmp(base, handShownBase) ? 0.0f : charge * charge * charge;
				float change = target - handHeight;
				handHeight += change < -0.4f ? -0.4f : (change > 0.4f ? 0.4f : change);
				if (handHeight < 0.1f && strcmp(base, handShownBase))
				{
					strcpy_s(handShownBase, base);
					strcpy_s(handShown, mesh);
				}
			}
			if (!strcmp(base, handShownBase)) // the same thing: its look at once (the bow's stages)
				strcpy_s(handShown, mesh);
			zCVisual* visual = handShown[0] ? HandVisualNamed(handShown) : nullptr;
			if (!visual)
			{
				if (handVob->showVisual)
					handVob->showVisual = FALSE;
				return;
			}
			if (handVob->GetVisual() != visual)
				handVob->SetVisual(visual);

			float share = handClock / TICK;
			float low = 1.0f - Lerp(handHeightOld, handHeight, share); // inverseArmHeight
			float a = attackAnim - attackAnimOld; // LivingEntity.getAttackAnim
			a = attackAnimOld + (a < 0.0f ? a + 1.0f : a) * TickShare();
			float sq = sqrtf(a);
			HandPose p;
			p.Turn(0, (pitchDeg - Lerp(xBobOld, xBob, share)) * 0.1f).Turn(1, (yawDeg - Lerp(yBobOld, yBob, share)) * 0.1f);
			if (!strcmp(handShownBase, "ARM")) // renderPlayerArm
			{
				p.Move(-0.3f * sinf(sq * PI) + 0.64f, 0.4f * sinf(sq * 2.0f * PI) - 0.6f - 0.6f * low, -0.4f * sinf(a * PI) - 0.72f);
				p.Turn(1, 45.0f).Turn(1, sinf(sq * PI) * 70.0f).Turn(2, sinf(a * a * PI) * -20.0f);
			}
			else
			{
				p.Move(0.56f, -0.52f - 0.6f * low, -0.72f); // applyItemArmTransform
				if (useTicks >= 0 && !strcmp(handShownBase, "BOW")) // ItemUseAnimation.BOW
				{
					p.Move(-0.2785682f, 0.18344387f, 0.15731531f).Turn(0, -13.935f).Turn(1, 35.3f).Turn(2, -9.785f);
					float held = useTicks + TickShare() - 1.0f;
					held = held < 0.0f ? 0.0f : held;
					float power = held / 20.0f;
					power = (power * power + power * 2.0f) / 3.0f;
					power = power > 1.0f ? 1.0f : power;
					if (power > 0.1f)
						p.Move(0.0f, sinf((held - 0.1f) * 1.3f) * (power - 0.1f) * 0.004f, 0.0f);
					p.Move(0.0f, 0.0f, power * 0.04f).Turn(1, -45.0f);
				}
				else // swingArm
				{
					p.Move(-0.4f * sinf(sq * PI), 0.2f * sinf(sq * 2.0f * PI), -0.2f * sinf(a * PI));
					p.Turn(1, 45.0f + sinf(a * a * PI) * -20.0f).Turn(2, sinf(sq * PI) * -20.0f).Turn(0, sinf(sq * PI) * -80.0f).Turn(1, -45.0f);
				}
			}
			// Minecraft's view space -> the camera's frame (z the other way) -> the world
			float r[3][3];
			memcpy(r, p.r, sizeof r);
			r[0][2] = -r[0][2];
			r[1][2] = -r[1][2];
			r[2][0] = -r[2][0];
			r[2][1] = -r[2][1];
			zCVob* cam = ai->camVob;
			zVEC3 right = cam->GetRightVectorWorld(), up = cam->GetUpVectorWorld(), at = cam->GetAtVectorWorld();
			auto world3 = [&](float x, float y, float z) { return right * x + up * y + at * z; };
			zMAT4 m = zMAT4::GetIdentity();
			m.SetRightVector(world3(r[0][0], r[1][0], r[2][0]));
			m.SetUpVector(world3(r[0][1], r[1][1], r[2][1]));
			m.SetAtVector(world3(r[0][2], r[1][2], r[2][2]));
			m.SetTranslation(cam->GetPositionWorld() + world3(p.t[0] * FP_UNIT, p.t[1] * FP_UNIT, -p.t[2] * FP_UNIT));
			handVob->SetTrafoObjToWorld(m);
			if (!handVob->showVisual)
				handVob->showVisual = TRUE;
		}

		// --- the Ender Dragon (EnderDragon, its phases, EnderDragonModel, EnderDragonRenderer) ----------
		// Its spawn egg hatches it where it is used, and that spot on Gothic's ground is its fight's middle
		// (Minecraft's podium). Minecraft's summoned dragon only hovers; this one flies its End fight's phases
		// around that spot, as with no crystals left:
		//  - holding pattern: node to node of the inner rings (radii 40 and 20 blocks, 15 and 5 above Gothic's
		//    ground; Minecraft's node graph and A*), each target up to 20 blocks higher, now and then the
		//    other way round; at a path's end it lands (1 in 3) or strafes Steve (at least 1 in 2);
		//  - strafing: at him, spitting a fireball when it faces him within 64 blocks with him in sight; the
		//    fireball (0.1 a tick faster each tick, keeping 95 %) leaves its breath where it hits: a cloud 3
		//    blocks across growing to 7 over 30 seconds, 6 of Minecraft's 20 a second to whoever is in it;
		//  - landing on the spot, sitting: looking for him (5 s), roaring (2 s), breathing on the ground before
		//    it (a cloud 5 blocks across, 10 s), four times, taking off; charging him when he is far;
		//  - dying: it flies to the spot, then rises 0.1 a tick for 10 seconds among explosions and is gone.
		// Its flight (EnderDragon.aiStep): toward the phase's target, turning by yRotA, climbing as fast as
		// its fly speed allows, faster along its heading the better it faces the target (0.06 a tick), keeping
		// 80-95 % across and 91 % up and down. It flies through Gothic's world (noPhysics), breaking the blocks
		// its head, neck and body go through (but obsidian, where it slows). Its wings throw whoever is
		// under them (4 blocks / d^2 away, 0.2 up; 5 of Minecraft's 20 while flying), its head and neck hurt
		// 10. Minecraft's 200 health: Steve's melee hits hurt it by Gothic's damage / 10 (Minecraft's scale),
		// arrows and explosions by Minecraft's; away from the head a quarter, but at least 1; sitting, arrows
		// don't hurt it and 25 % of its health lost makes it take off. Its pink boss bar shows its health.
		// Drawn as EnderDragonModel.setupAnim and EnderDragonRenderer place its 23 parts (make_mc_dragon.py):
		// the neck and tail follow its flight history (the last 64 ticks' height and heading), the wings flap
		// (flapTime: 0.2 / (10 x speed + 1) x 2^climb a tick), the jaw opens and closes; the wings' flaps,
		// its growl (every 10-20 s), its hurt and its death are Minecraft's sounds. One dragon at a time.
		enum { DP_HOLDING, DP_STRAFE, DP_APPROACH, DP_LANDING, DP_SCANNING, DP_ATTACKING, DP_FLAMING, DP_TAKEOFF, DP_CHARGING, DP_DYING };
		static const char* const DRAGON_PHASES[] = { "holding pattern", "strafing", "landing approach", "landing", "sitting: scanning",
			"sitting: roaring", "sitting: breathing", "taking off", "charging", "dying" };
		enum { DV_HEAD, DV_JAW, DV_NECK, DV_BODY = DV_NECK + 5, DV_WING_L, DV_TIP_L, DV_WING_R, DV_TIP_R, DV_LEGS, DV_TAIL = DV_LEGS + 12,
			DRAGON_VOBS = DV_TAIL + 12 };
		enum { DPART_HEAD, DPART_NECK, DPART_BODY, DPART_TAIL1, DPART_TAIL2, DPART_TAIL3, DPART_WING1, DPART_WING2, DRAGON_PARTS };
		struct DragonNode
		{
			int x, y, z;
			float g, h, f;
			int from;
			bool closed, open;
		};
		struct Dragon
		{
			bool alive = false;
			float x, y, z, xo, yo, zo;     // blocks (Gothic's world / 100)
			float vx, vy, vz;              // blocks a tick
			float yRot, yRotA;             // degrees, Minecraft's (its head the other way to its flight)
			float flapTime, oFlapTime;
			float health;
			int phase;
			float histY[64], histYRot[64];
			int histHead;
			float ox, oy, oz;              // the fight's middle (its podium on the ground)
			DragonNode nodes[24];
			int path[34][3];
			int pathLen, pathNext;
			bool hasTarget;
			float tx, ty, tz;
			bool clockwise;
			int fireballCharge, phaseTicks, flameCount, timeSinceCharge, deathTime, growlTime;
			bool firstTick, inWall;
			float sittingDamage;
			float px[DRAGON_PARTS], py[DRAGON_PARTS], pz[DRAGON_PARTS]; // its parts' hit boxes (EnderDragonPart): foot middle
			zCVob* vobs[DRAGON_VOBS];      // the plugin's references
		};
		static Dragon dragon;
		static const float PART_W[DRAGON_PARTS] = { 1.0f, 3.0f, 5.0f, 2.0f, 2.0f, 2.0f, 4.0f, 4.0f };
		static const float PART_H[DRAGON_PARTS] = { 1.0f, 3.0f, 3.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f };
		static float dragonClock = 0.0f;
		static std::unordered_map<oCNpc*, float> dragonHitUntil;

		struct Fireball
		{
			zCVob* vob = nullptr;      // the plugin's reference (a picture facing the camera)
			zCDecal* decal = nullptr;
			zVEC3 pos, last, vel;      // cm; blocks a tick
			int life = 0;
		};
		static Fireball fireballs[4];
		struct Breath
		{
			float x, y, z, radius, grow;  // blocks
			int age = -1, duration = 0;
			float nextHurt = 0.0f;
		};
		static Breath breaths[8];
		static std::unordered_map<oCNpc*, float> breathHitUntil;

		static float WrapDegrees(float d)
		{
			d = fmodf(d, 360.0f);
			if (d >= 180.0f)
				d -= 360.0f;
			if (d < -180.0f)
				d += 360.0f;
			return d;
		}

		static float GroundY(float bx, float bz, float fromY) // blocks: Gothic's ground under a spot
		{
			zCWorld* world = blocksWorld;
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_VOB_IGNORE;
			if (world && world->TraceRayNearestHit(zVEC3(bx * BLOCK, (fromY + 300.0f) * BLOCK, bz * BLOCK), zVEC3(0.0f, -600.0f * BLOCK, 0.0f),
				(zCVob*)nullptr, flags) && world->traceRayReport.foundHit)
				return world->traceRayReport.foundIntersection[1] / BLOCK;
			return fromY;
		}

		// EnderDragon.findClosestNode: its 24 nodes around the middle (here only the inner 12: no crystals)
		static void MakeDragonNodes()
		{
			for (int i = 0; i < 24; i++)
			{
				int adjust = 5;
				float r, a;
				if (i < 12)
				{
					r = 60.0f;
					a = 2.0f * (-PI + PI / 12.0f * i);
				}
				else if (i < 20)
				{
					r = 40.0f;
					a = 2.0f * (-PI + PI / 8.0f * (i - 12));
					adjust += 10;
				}
				else
				{
					r = 20.0f;
					a = 2.0f * (-PI + PI / 4.0f * (i - 20));
				}
				DragonNode& n = dragon.nodes[i];
				n.x = (int)floorf(dragon.ox + r * cosf(a));
				n.z = (int)floorf(dragon.oz + r * sinf(a));
				float ground = GroundY((float)n.x, (float)n.z, dragon.oy);
				n.y = (int)floorf((ground > dragon.oy ? ground : dragon.oy) + adjust);
				if (n.y < dragon.oy + 10.0f)
					n.y = (int)(dragon.oy + 10.0f);
			}
		}

		static const int NODE_ADJACENCY[24] = { 6146, 8197, 8202, 16404, 32808, 32848, 65696, 131392, 131712, 263424, 526848, 525313,
			1581057, 3166214, 2138120, 6373424, 4358208, 12910976, 9044480, 9706496, 15216640, 13688832, 11763712, 8257536 };

		static int ClosestDragonNode(float x, float y, float z)
		{
			float closest = 10000.0f;
			int index = 12;
			int fx = (int)floorf(x), fy = (int)floorf(y), fz = (int)floorf(z);
			for (int i = 12; i < 24; i++)
			{
				const DragonNode& n = dragon.nodes[i];
				float d = (float)((n.x - fx) * (n.x - fx) + (n.y - fy) * (n.y - fy) + (n.z - fz) * (n.z - fz));
				if (d < closest)
				{
					closest = d;
					index = i;
				}
			}
			return index;
		}

		static float NodeDistance(const DragonNode& a, const DragonNode& b)
		{
			float dx = (float)(a.x - b.x), dy = (float)(a.y - b.y), dz = (float)(a.z - b.z);
			return sqrtf(dx * dx + dy * dy + dz * dz);
		}

		// EnderDragon.findPath (A* over the nodes' graph), its end `final` (or none: final[0] == INT_MIN)
		static bool FindDragonPath(int start, int end, const int* final)
		{
			for (DragonNode& n : dragon.nodes)
			{
				n.closed = n.open = false;
				n.g = n.h = n.f = 0.0f;
				n.from = -1;
			}
			DragonNode& to = dragon.nodes[end];
			dragon.nodes[start].h = NodeDistance(dragon.nodes[start], to);
			dragon.nodes[start].f = dragon.nodes[start].h;
			dragon.nodes[start].open = true;
			int closest = start, reached = -1;
			while (true)
			{
				int current = -1;
				for (int i = 12; i < 24; i++)
					if (dragon.nodes[i].open && (current < 0 || dragon.nodes[i].f < dragon.nodes[current].f))
						current = i;
				if (current < 0)
					break;
				if (current == end)
				{
					reached = end;
					break;
				}
				DragonNode& c = dragon.nodes[current];
				if (NodeDistance(c, to) < NodeDistance(dragon.nodes[closest], to))
					closest = current;
				c.open = false;
				c.closed = true;
				for (int i = 12; i < 24; i++)
				{
					if (!(NODE_ADJACENCY[current] & (1 << i)) || dragon.nodes[i].closed)
						continue;
					DragonNode& a = dragon.nodes[i];
					float g = c.g + NodeDistance(c, a);
					if (!a.open || g < a.g)
					{
						a.from = current;
						a.g = g;
						a.h = NodeDistance(a, to);
						a.f = a.g + a.h;
						a.open = true;
					}
				}
			}
			if (reached < 0)
			{
				if (closest == start)
				{
					dragon.pathLen = 0;
					return false;
				}
				reached = closest;
			}
			int chain[24], count = 0;
			for (int i = reached; i >= 0 && count < 24; i = dragon.nodes[i].from)
				chain[count++] = i;
			dragon.pathLen = 0;
			for (int i = count - 1; i >= 0; i--)
			{
				const DragonNode& n = dragon.nodes[chain[i]];
				dragon.path[dragon.pathLen][0] = n.x;
				dragon.path[dragon.pathLen][1] = n.y;
				dragon.path[dragon.pathLen++][2] = n.z;
			}
			if (final && final[0] != INT_MIN)
			{
				dragon.path[dragon.pathLen][0] = final[0];
				dragon.path[dragon.pathLen][1] = final[1];
				dragon.path[dragon.pathLen++][2] = final[2];
			}
			dragon.pathNext = 0;
			return true;
		}

		static bool PathDone() { return dragon.pathNext >= dragon.pathLen; }

		static void NextPathNode() // navigateToNextPathNode: the node, up to 20 blocks higher
		{
			if (dragon.pathLen == 0 || PathDone())
				return;
			const int* n = dragon.path[dragon.pathNext++];
			dragon.tx = (float)n[0];
			dragon.tz = (float)n[2];
			dragon.ty = n[1] + Random01() * 20.0f;
			dragon.hasTarget = true;
		}

		static void SetDragonPhase(int phase)
		{
			if (phase == dragon.phase)
				return;
			dragon.phase = phase;
			dragon.hasTarget = false;
			dragon.pathLen = dragon.pathNext = 0;
			dragon.fireballCharge = dragon.phaseTicks = dragon.timeSinceCharge = 0;
			dragon.firstTick = true;
			if (phase == DP_FLAMING)
				dragon.flameCount++;
			Log("dragon: %s", DRAGON_PHASES[phase]);
		}

		static bool DragonSitting() { return dragon.phase == DP_SCANNING || dragon.phase == DP_ATTACKING || dragon.phase == DP_FLAMING; }

		static float DragonFlySpeed()
		{
			return dragon.phase == DP_LANDING ? 1.5f : (dragon.phase == DP_CHARGING || dragon.phase == DP_DYING ? 3.0f : 0.6f);
		}

		static float DragonTurnSpeed()
		{
			float rotSpeed = sqrtf(dragon.vx * dragon.vx + dragon.vz * dragon.vz) + 1.0f;
			float dist = rotSpeed < 40.0f ? rotSpeed : 40.0f;
			return dragon.phase == DP_LANDING ? dist / rotSpeed : 0.7f / dist / rotSpeed;
		}

		static void DragonHolding(int& node) // the next node round the inner ring
		{
			int current = ClosestDragonNode(dragon.x, dragon.y, dragon.z);
			node = current;
			if (rand() % 8 == 0)
			{
				dragon.clockwise = !dragon.clockwise;
				node += 6;
			}
			node += dragon.clockwise ? 1 : -1;
			node -= 12;
			node &= 7;
			node += 12;
			static const int NONE[3] = { INT_MIN, 0, 0 };
			if (FindDragonPath(current, node, NONE))
				dragon.pathNext++;
		}

		static void DragonStrafe(oCNpc* hero);

		static void DragonHoldingNewTarget(oCNpc* hero)
		{
			if (dragon.pathLen && PathDone())
			{
				if (rand() % 3 == 0)
				{
					SetDragonPhase(DP_APPROACH);
					return;
				}
				zVEC3 h = hero->GetPositionWorld() * (1.0f / BLOCK);
				float d = ((h[0] - dragon.ox) * (h[0] - dragon.ox) + (h[1] - dragon.oy) * (h[1] - dragon.oy) + (h[2] - dragon.oz) * (h[2] - dragon.oz)) / 512.0f;
				if (rand() % (int)(d + 2.0f) == 0 || rand() % 2 == 0)
				{
					DragonStrafe(hero);
					return;
				}
			}
			if (!dragon.pathLen || PathDone())
			{
				int node;
				DragonHolding(node);
			}
			NextPathNode();
		}

		static void DragonStrafe(oCNpc* hero) // DragonStrafePlayerPhase.setTarget
		{
			SetDragonPhase(DP_STRAFE);
			zVEC3 h = hero->GetPositionWorld() * (1.0f / BLOCK);
			int current = ClosestDragonNode(dragon.x, dragon.y, dragon.z), node = ClosestDragonNode(h[0], h[1], h[2]);
			float xd = floorf(h[0]) - dragon.x, zd = floorf(h[2]) - dragon.z;
			float ho = 0.4f + sqrtf(xd * xd + zd * zd) / 80.0f - 1.0f;
			ho = ho < 10.0f ? ho : 10.0f;
			int final[3] = { (int)floorf(h[0]), (int)floorf(h[1] + ho), (int)floorf(h[2]) };
			if (FindDragonPath(current, node, final))
			{
				dragon.pathNext++;
				NextPathNode();
			}
		}

		static zVEC3 DragonPartCentre(int part)
		{
			return zVEC3(dragon.px[part], dragon.py[part] + PART_H[part] * 0.5f, dragon.pz[part]) * BLOCK;
		}

		// Entity.getViewVector of its heading (and pitch), Minecraft's way round
		static zVEC3 DragonLook(float xRot)
		{
			float y = dragon.yRot * PI / 180.0f, x = xRot * PI / 180.0f;
			return zVEC3(-sinf(y) * cosf(x), -sinf(x), cosf(y) * cosf(x));
		}

		static void ShootDragonFireball(oCNpc* hero)
		{
			Fireball* f = nullptr;
			for (Fireball& g : fireballs)
				if (!g.vob && !f)
					f = &g;
			oCWorld* world = ogame ? ogame->GetGameWorld() : nullptr;
			if (!f || !world)
				return;
			zVEC3 look = DragonLook(0.0f);
			zVEC3 start(dragon.px[DPART_HEAD] - look[0], dragon.py[DPART_HEAD] + 0.5f + 0.5f, dragon.pz[DPART_HEAD] - look[2]);
			zVEC3 target = hero->GetPositionWorld() * (1.0f / BLOCK);
			zVEC3 dir = target - start;
			float len = dir.Length();
			dir = len > 0.001f ? dir * (1.0f / len) : zVEC3(0.0f, -1.0f, 0.0f);
			f->decal = new zCDecal();
			if (zCTexture* texture = LoadTexture("MC_DRAGON_FIREBALL.TGA"))
				f->decal->SetTexture(texture);
			f->decal->SetDecalDim(50.0f, 50.0f);
			f->decal->decalMaterial->rndAlphaBlendFunc = zRND_ALPHA_FUNC_TEST;
			f->vob = new zCVob();
			f->vob->dontWriteIntoArchive = TRUE;
			f->vob->visualCamAlign = (zTVisualCamAlign)zVISUAL_CAMALIGN_FULL;
			f->vob->SetCollDetStat(FALSE);
			f->vob->SetCollDetDyn(FALSE);
			f->vob->SetVisual(f->decal);
			f->vob->SetPositionWorld(start * BLOCK);
			world->AddVob(f->vob);
			f->vob->SetCollDetStat(FALSE);
			f->vob->SetCollDetDyn(FALSE);
			f->pos = f->last = start * BLOCK;
			f->vel = dir * 0.1f; // AbstractHurtingProjectile: its acceleration power
			f->life = 0;
			PlaySound("MC_DRAGON_SHOOT.WAV", dragon.vobs[DV_HEAD] ? dragon.vobs[DV_HEAD] : hero);
			Log("dragon: a fireball");
		}

		static void Breathe(float x, float y, float z, float radius, int duration, float grow)
		{
			for (Breath& b : breaths)
				if (b.age < 0)
				{
					b.x = x;
					b.y = y;
					b.z = z;
					b.radius = radius;
					b.grow = grow;
					b.age = 0;
					b.duration = duration;
					return;
				}
		}

		static bool DragonSees(oCNpc* hero) // the head sees him: nothing of Gothic's world or a block between
		{
			zCWorld* world = blocksWorld;
			if (!world)
				return true;
			zVEC3 from = DragonPartCentre(DPART_HEAD), to = Eyes(hero);
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_VOB_IGNORE_CHARACTER | zTRACERAY_VOB_IGNORE_NO_CD_DYN;
			return !(world->TraceRayNearestHit(from, to - from, (zCVob*)nullptr, flags) && world->traceRayReport.foundHit);
		}

		// DragonPhaseInstance.doServerTick for its phase
		static void DragonPhaseTick(oCNpc* hero)
		{
			zVEC3 h = hero->GetPositionWorld() * (1.0f / BLOCK);
			float distToTarget = dragon.hasTarget ? (dragon.tx - dragon.x) * (dragon.tx - dragon.x) + (dragon.ty - dragon.y) * (dragon.ty - dragon.y)
				+ (dragon.tz - dragon.z) * (dragon.tz - dragon.z) : 0.0f;
			switch (dragon.phase)
			{
			case DP_HOLDING:
				if (distToTarget < 100.0f || distToTarget > 22500.0f)
					DragonHoldingNewTarget(hero);
				break;
			case DP_STRAFE:
			{
				if (PathDone())
				{
					float xd = h[0] - dragon.x, zd = h[2] - dragon.z;
					float ho = 0.4f + sqrtf(xd * xd + zd * zd) / 80.0f - 1.0f;
					dragon.tx = h[0];
					dragon.ty = h[1] + (ho < 10.0f ? ho : 10.0f);
					dragon.tz = h[2];
					dragon.hasTarget = true;
				}
				distToTarget = (dragon.tx - dragon.x) * (dragon.tx - dragon.x) + (dragon.ty - dragon.y) * (dragon.ty - dragon.y)
					+ (dragon.tz - dragon.z) * (dragon.tz - dragon.z);
				if (distToTarget < 100.0f || distToTarget > 22500.0f)
				{
					if (!dragon.pathLen || PathDone())
					{
						int node;
						DragonHolding(node);
					}
					NextPathNode();
				}
				float dx = h[0] - dragon.x, dy = h[1] - dragon.y, dz = h[2] - dragon.z;
				if (dx * dx + dy * dy + dz * dz < 4096.0f && DragonSees(hero))
				{
					dragon.fireballCharge++;
					zVEC3 aim(dx, 0.0f, dz), dir(sinf(dragon.yRot * PI / 180.0f), 0.0f, -cosf(dragon.yRot * PI / 180.0f));
					float len = aim.Length();
					float dot = len > 0.001f ? aim.Dot(dir) / len : 1.0f;
					float angle = acosf(dot < -1.0f ? -1.0f : (dot > 1.0f ? 1.0f : dot)) * 180.0f / PI + 0.5f;
					if (dragon.fireballCharge >= 5 && angle < 10.0f)
					{
						ShootDragonFireball(hero);
						SetDragonPhase(DP_HOLDING);
					}
				}
				else if (dragon.fireballCharge > 0)
					dragon.fireballCharge--;
				break;
			}
			case DP_APPROACH:
				if (distToTarget < 100.0f || distToTarget > 22500.0f)
				{
					if (!dragon.pathLen || PathDone())
					{
						int current = ClosestDragonNode(dragon.x, dragon.y, dragon.z);
						zVEC3 aim(h[0] - dragon.ox, 0.0f, h[2] - dragon.oz);
						float len = aim.Length();
						aim = len > 0.001f ? aim * (1.0f / len) : zVEC3(1.0f, 0.0f, 0.0f);
						int node = ClosestDragonNode(dragon.ox - aim[0] * 40.0f, dragon.oy + 40.0f, dragon.oz - aim[2] * 40.0f);
						int final[3] = { (int)floorf(dragon.ox), (int)floorf(dragon.oy), (int)floorf(dragon.oz) };
						if (FindDragonPath(current, node, final))
							dragon.pathNext++;
					}
					NextPathNode();
					if (dragon.pathLen && PathDone())
						SetDragonPhase(DP_LANDING);
				}
				break;
			case DP_LANDING:
				dragon.tx = floorf(dragon.ox) + 0.5f;
				dragon.ty = dragon.oy;
				dragon.tz = floorf(dragon.oz) + 0.5f;
				dragon.hasTarget = true;
				if (distToTarget < 1.0f && dragon.phaseTicks++ > 0)
				{
					dragon.flameCount = 0;
					SetDragonPhase(DP_SCANNING);
				}
				break;
			case DP_SCANNING:
			{
				dragon.phaseTicks++;
				float dx = h[0] - dragon.x, dz = h[2] - dragon.z;
				bool inRange = dx * dx + dz * dz + (h[1] - dragon.y) * (h[1] - dragon.y) < 400.0f && fabsf(h[1] - dragon.y) <= 10.0f;
				if (inRange)
				{
					if (dragon.phaseTicks > 25)
						SetDragonPhase(DP_ATTACKING);
					else
					{
						zVEC3 aim(dx, 0.0f, dz), dir(sinf(dragon.yRot * PI / 180.0f), 0.0f, -cosf(dragon.yRot * PI / 180.0f));
						float len = aim.Length();
						float dot = len > 0.001f ? aim.Dot(dir) / len : 1.0f;
						float angle = acosf(dot < -1.0f ? -1.0f : (dot > 1.0f ? 1.0f : dot)) * 180.0f / PI + 0.5f;
						if (angle > 10.0f)
						{
							float ax = h[0] - dragon.px[DPART_HEAD], az = h[2] - dragon.pz[DPART_HEAD];
							float delta = WrapDegrees(180.0f - atan2f(ax, az) * 180.0f / PI - dragon.yRot);
							delta = delta < -100.0f ? -100.0f : (delta > 100.0f ? 100.0f : delta);
							dragon.yRotA *= 0.8f;
							float dist = sqrtf(ax * ax + az * az) + 1.0f, rotSpeed = dist;
							dist = dist > 40.0f ? 40.0f : dist;
							dragon.yRotA += delta * (0.7f / dist / rotSpeed);
							dragon.yRot += dragon.yRotA;
						}
					}
				}
				else if (dragon.phaseTicks >= 100)
				{
					float d = (h[0] - dragon.x) * (h[0] - dragon.x) + (h[1] - dragon.y) * (h[1] - dragon.y) + (h[2] - dragon.z) * (h[2] - dragon.z);
					SetDragonPhase(DP_TAKEOFF);
					if (d < 150.0f * 150.0f)
					{
						SetDragonPhase(DP_CHARGING);
						dragon.tx = h[0];
						dragon.ty = h[1];
						dragon.tz = h[2];
						dragon.hasTarget = true;
					}
				}
				break;
			}
			case DP_ATTACKING:
				if (dragon.phaseTicks == 0 || dragon.phaseTicks == 20)
				{
					char growl[32];
					sprintf_s(growl, "MC_DRAGON_GROWL%d.WAV", 1 + rand() % 4);
					PlaySound(growl, dragon.vobs[DV_HEAD] ? dragon.vobs[DV_HEAD] : hero);
				}
				if (dragon.phaseTicks++ >= 40)
					SetDragonPhase(DP_FLAMING);
				break;
			case DP_FLAMING:
				dragon.phaseTicks++;
				if (dragon.phaseTicks >= 200)
					SetDragonPhase(dragon.flameCount >= 4 ? DP_TAKEOFF : DP_SCANNING);
				else if (dragon.phaseTicks == 10) // a cloud of breath on the ground before its head
				{
					zVEC3 look(dragon.px[DPART_HEAD] - dragon.x, 0.0f, dragon.pz[DPART_HEAD] - dragon.z);
					float len = look.Length();
					look = len > 0.001f ? look * (1.0f / len) : zVEC3(0.0f, 0.0f, 1.0f);
					float bx = dragon.px[DPART_HEAD] + look[0] * 2.5f, bz = dragon.pz[DPART_HEAD] + look[2] * 2.5f;
					float by = GroundY(bx, bz, dragon.py[DPART_HEAD]);
					Breathe(bx, floorf(by) + 1.0f, bz, 5.0f, 200, 0.0f);
				}
				break;
			case DP_TAKEOFF:
				if (!dragon.firstTick && dragon.pathLen)
				{
					float dx = dragon.x - dragon.ox, dz = dragon.z - dragon.oz, dy = dragon.y - dragon.oy;
					if (dx * dx + dy * dy + dz * dz > 100.0f)
						SetDragonPhase(DP_HOLDING);
				}
				else
				{
					dragon.firstTick = false;
					int current = ClosestDragonNode(dragon.x, dragon.y, dragon.z);
					zVEC3 look = DragonLook(-45.0f);
					int node = ClosestDragonNode(dragon.ox - look[0] * 40.0f, dragon.oy + 40.0f, dragon.oz - look[2] * 40.0f);
					node -= 12;
					node &= 7;
					node += 12;
					static const int NONE[3] = { INT_MIN, 0, 0 };
					if (FindDragonPath(current, node, NONE))
					{
						dragon.pathNext++;
						NextPathNode();
					}
				}
				break;
			case DP_CHARGING:
				if (dragon.timeSinceCharge > 0 && dragon.timeSinceCharge++ >= 10)
					SetDragonPhase(DP_HOLDING);
				else if (distToTarget < 100.0f || distToTarget > 22500.0f)
					dragon.timeSinceCharge++;
				break;
			case DP_DYING:
				dragon.tx = floorf(dragon.ox) + 0.5f;
				dragon.ty = dragon.oy;
				dragon.tz = floorf(dragon.oz) + 0.5f;
				dragon.hasTarget = true;
				dragon.health = distToTarget < 100.0f || distToTarget > 22500.0f ? 0.0f : 1.0f;
				break;
			}
		}

		static void DragonHurt(oCNpc* hero, int part, float damage, bool arrow)
		{
			if (!dragon.alive || dragon.phase == DP_DYING || dragon.health <= 0.0f)
				return;
			if (arrow && DragonSitting()) // AbstractDragonSittingPhase.onHurt
				damage = 0.0f;
			if (part != DPART_HEAD && part != DPART_NECK)
				damage = damage / 4.0f + (damage < 1.0f ? damage : 1.0f);
			if (damage < 0.01f)
				return;
			dragon.health -= damage;
			char hit[32];
			sprintf_s(hit, "MC_DRAGON_HIT%d.WAV", 1 + rand() % 4);
			PlaySound(hit, dragon.vobs[DV_BODY] ? dragon.vobs[DV_BODY] : hero);
			Log("dragon: hurt %.1f (part %d), %.0f of 200", damage, part, dragon.health);
			if (DragonSitting())
			{
				dragon.sittingDamage += damage;
				if (dragon.sittingDamage > 50.0f)
				{
					dragon.sittingDamage = 0.0f;
					SetDragonPhase(DP_TAKEOFF);
				}
			}
			if (dragon.health <= 0.0f)
			{
				if (!DragonSitting()) // handleKillingBlow
				{
					dragon.health = 1.0f;
					SetDragonPhase(DP_DYING);
				}
				else
					dragon.health = 0.0f;
			}
		}

		// the first of its parts on a ray (t: the share of `ray`), -1: none
		static int DragonPartOnRay(const zVEC3& from, const zVEC3& ray, float& t)
		{
			int best = -1;
			t = 2.0f;
			if (!dragon.alive)
				return -1;
			for (int i = 0; i < DRAGON_PARTS; i++)
			{
				zTBBox3D box;
				box.mins = zVEC3(dragon.px[i] - PART_W[i] * 0.5f, dragon.py[i], dragon.pz[i] - PART_W[i] * 0.5f) * BLOCK;
				box.maxs = zVEC3(dragon.px[i] + PART_W[i] * 0.5f, dragon.py[i] + PART_H[i], dragon.pz[i] + PART_W[i] * 0.5f) * BLOCK;
				float d = RayBox(from, ray, box);
				if (d >= 0.0f && d < t)
				{
					t = d;
					best = i;
				}
			}
			return t <= 1.0f ? best : -1;
		}

		// whoever its wings, head and neck reach (EnderDragon.knockBack, hurt)
		static void DragonTouches(oCNpc* hero)
		{
			oCWorld* game = ogame ? ogame->GetGameWorld() : nullptr;
			float now = Now();
			float bx = dragon.px[DPART_BODY], bz = dragon.pz[DPART_BODY];
			for (zCListSort<oCNpc>* node = game && game->voblist_npcs ? game->voblist_npcs->next : nullptr; node; node = node->next)
			{
				oCNpc* npc = node->data;
				if (!npc || npc->IsDead())
					continue;
				zVEC3 p = npc->GetPositionWorld() * (1.0f / BLOCK);
				float feet = (npc == hero && npc->human_ai ? npc->human_ai->feetY : npc->bbox3D.mins[1]) / BLOCK;
				float top = npc->bbox3D.maxs[1] / BLOCK;
				auto inside = [&](int part, float grow, float growY, float down)
				{
					float w = PART_W[part] * 0.5f + grow;
					return fabsf(p[0] - dragon.px[part]) < w + 0.3f && fabsf(p[2] - dragon.pz[part]) < w + 0.3f
						&& top > dragon.py[part] - growY - down && feet < dragon.py[part] + PART_H[part] + growY - down;
				};
				bool wing = inside(DPART_WING1, 4.0f, 2.0f, 2.0f) || inside(DPART_WING2, 4.0f, 2.0f, 2.0f);
				bool head = inside(DPART_HEAD, 1.0f, 1.0f, 0.0f) || inside(DPART_NECK, 1.0f, 1.0f, 0.0f);
				if (!wing && !head)
					continue;
				float& until = dragonHitUntil[npc];
				if (wing)
				{
					float xd = p[0] - bx, zd = p[2] - bz;
					float dd = xd * xd + zd * zd > 0.1f ? xd * xd + zd * zd : 0.1f;
					zVEC3 push(xd / dd * 4.0f, 0.2f, zd / dd * 4.0f);
					if (npc == hero)
						Knock(hero, push);
					if (!DragonSitting() && now >= until)
					{
						until = now + 10 * TICK;
						if (npc == hero)
							McHurt(npc, 5.0f, oEDamageType_Blunt, oEDamageIndex_Blunt);
						else
						{
							float damage = 5.0f / 20.0f * npc->attribute[NPC_ATR_HITPOINTSMAX];
							npc->OnDamage(dragon.vobs[DV_BODY] ? dragon.vobs[DV_BODY] : npc, nullptr, damage + npc->protection[oEDamageIndex_Blunt],
								oEDamageType_Blunt | oEDamageType_Fly, npc->GetPositionWorld());
						}
						Log("dragon: its wing throws %s", npc->GetInstanceName().ToChar());
					}
				}
				if (head && now >= until)
				{
					until = now + 10 * TICK;
					McHurt(npc, 10.0f, oEDamageType_Edge, oEDamageIndex_Edge);
					Log("dragon: its head hits %s", npc->GetInstanceName().ToChar());
				}
			}
		}

		// EnderDragon.checkWalls: the blocks in its head's, neck's and body's boxes go (obsidian stops it)
		static bool DragonWalls(int part)
		{
			bool wall = false;
			int x0 = (int)floorf(dragon.px[part] - PART_W[part] * 0.5f), x1 = (int)floorf(dragon.px[part] + PART_W[part] * 0.5f);
			int y0 = (int)floorf(dragon.py[part]), y1 = (int)floorf(dragon.py[part] + PART_H[part]);
			int z0 = (int)floorf(dragon.pz[part] - PART_W[part] * 0.5f), z1 = (int)floorf(dragon.pz[part] + PART_W[part] * 0.5f);
			for (int x = x0; x <= x1; x++)
				for (int y = y0; y <= y1; y++)
					for (int z = z0; z <= z1; z++)
					{
						long long key = CellKey(x, y, z);
						auto it = blockKinds.find(key);
						if (it == blockKinds.end())
							continue;
						if (!strcmp(blockTypes[it->second].id, "obsidian"))
							wall = true;
						else
							RemoveBlock(key, true);
					}
			return wall;
		}

		static void DragonTick(oCNpc* hero)
		{
			Dragon& d = dragon;
			d.xo = d.x;
			d.yo = d.y;
			d.zo = d.z;
			d.oFlapTime = d.flapTime;
			if (d.health <= 0.0f) // EnderDragon.tickDeath: up, explosions, gone
			{
				d.deathTime++;
				if (d.deathTime == 1)
					PlaySound("MC_DRAGON_END.WAV", hero);
				zCWorld* world = hero->GetHomeWorld();
				zVEC3 at(d.x + (Random01() - 0.5f) * 8.0f, d.y + 2.0f + (Random01() - 0.5f) * 4.0f, d.z + (Random01() - 0.5f) * 8.0f);
				if (d.deathTime >= 180)
				{
					for (Seed& s : seeds)
						if (s.age < 0)
						{
							s.at = at * BLOCK;
							s.age = 0;
							break;
						}
				}
				else
					SpawnPuff(world, at * BLOCK, 2.0f);
				d.y += 0.1f;
				for (int i = 0; i < DRAGON_PARTS; i++)
					d.py[i] += 0.1f;
				if (d.deathTime >= 200)
				{
					for (zCVob*& vob : d.vobs)
						DropVob(vob);
					d.alive = false;
					Log("dragon: dead");
				}
				return;
			}
			float horizontal = sqrtf(d.vx * d.vx + d.vz * d.vz);
			float flapSpeed = 0.2f / (horizontal * 10.0f + 1.0f) * powf(2.0f, d.vy);
			d.flapTime += DragonSitting() ? 0.1f : (d.inWall ? flapSpeed * 0.5f : flapSpeed);
			d.yRot = WrapDegrees(d.yRot);
			d.histHead = (d.histHead + 1) & 63; // DragonFlightHistory.record
			d.histY[d.histHead] = d.y;
			d.histYRot[d.histHead] = d.yRot;
			int phase = d.phase;
			DragonPhaseTick(hero);
			if (d.phase != phase)
				DragonPhaseTick(hero);
			if (d.hasTarget && !DragonSitting())
			{
				float xdd = d.tx - d.x, ydd = d.ty - d.y, zdd = d.tz - d.z;
				float distToTarget = xdd * xdd + ydd * ydd + zdd * zdd;
				float max = DragonFlySpeed();
				float horizontalDist = sqrtf(xdd * xdd + zdd * zdd);
				if (horizontalDist > 0.0f)
				{
					ydd /= horizontalDist;
					ydd = ydd < -max ? -max : (ydd > max ? max : ydd);
				}
				d.vy += ydd * 0.01f;
				d.yRot = WrapDegrees(d.yRot);
				zVEC3 aim(xdd, ydd, zdd), dir(sinf(d.yRot * PI / 180.0f), d.vy, -cosf(d.yRot * PI / 180.0f));
				float al = sqrtf(xdd * xdd + (d.ty - d.y) * (d.ty - d.y) + zdd * zdd);
				aim = al > 0.0f ? zVEC3(xdd, d.ty - d.y, zdd) * (1.0f / al) : zVEC3(0.0f, 0.0f, 0.0f);
				float dl = dir.Length();
				dir = dl > 0.0f ? dir * (1.0f / dl) : dir;
				float dot = (aim.Dot(dir) + 0.5f) / 1.5f;
				dot = dot > 0.0f ? dot : 0.0f;
				if (fabsf(xdd) > 1e-5f || fabsf(zdd) > 1e-5f)
				{
					float yRotD = WrapDegrees(180.0f - atan2f(xdd, zdd) * 180.0f / PI - d.yRot);
					yRotD = yRotD < -50.0f ? -50.0f : (yRotD > 50.0f ? 50.0f : yRotD);
					d.yRotA *= 0.8f;
					d.yRotA += yRotD * DragonTurnSpeed();
					d.yRot += d.yRotA * 0.1f;
				}
				float span = 2.0f / (distToTarget + 1.0f);
				// moveRelative(0.06 x ..., (0, 0, -1)): along its heading
				float accel = 0.06f * (dot * span + (1.0f - span));
				float ry = d.yRot * PI / 180.0f;
				d.vx += sinf(ry) * accel;
				d.vz += -cosf(ry) * accel;
				float k = d.inWall ? 0.8f : 1.0f;
				d.x += d.vx * k;
				d.y += d.vy * k;
				d.z += d.vz * k;
				zVEC3 actual(d.vx, d.vy, d.vz);
				float alen = actual.Length();
				actual = alen > 0.0f ? actual * (1.0f / alen) : actual;
				float slide = 0.8f + 0.15f * (actual.Dot(dir) + 1.0f) / 2.0f;
				d.vx *= slide;
				d.vy *= 0.91f;
				d.vz *= slide;
			}
			// its parts (EnderDragon.aiStep's tickPart)
			auto sample = [&](int delay, float& y, float& yRot) { int i = (d.histHead - delay) & 63; y = d.histY[i]; yRot = d.histYRot[i]; };
			float y5, r5, y10, r10, y0, r0;
			sample(5, y5, r5);
			sample(10, y10, r10);
			sample(0, y0, r0);
			float tilt = (y5 - y10) * 10.0f * PI / 180.0f, ccTilt = cosf(tilt), ssTilt = sinf(tilt);
			float rot1 = d.yRot * PI / 180.0f, ss1 = sinf(rot1), cc1 = cosf(rot1);
			auto place = [&](int part, float x, float y, float z) { d.px[part] = d.x + x; d.py[part] = d.y + y; d.pz[part] = d.z + z; };
			place(DPART_BODY, ss1 * 0.5f, 0.0f, -cc1 * 0.5f);
			place(DPART_WING1, cc1 * 4.5f, 2.0f, ss1 * 4.5f);
			place(DPART_WING2, cc1 * -4.5f, 2.0f, ss1 * -4.5f);
			float ss2 = sinf(rot1 - d.yRotA * 0.01f), cc2 = cosf(rot1 - d.yRotA * 0.01f);
			float yOffset = DragonSitting() ? -1.0f : y5 - y0;
			place(DPART_HEAD, ss2 * 6.5f * ccTilt, yOffset + ssTilt * 6.5f, -cc2 * 6.5f * ccTilt);
			place(DPART_NECK, ss2 * 5.5f * ccTilt, yOffset + ssTilt * 5.5f, -cc2 * 5.5f * ccTilt);
			for (int i = 0; i < 3; i++)
			{
				float py0, pr0;
				sample(12 + i * 2, py0, pr0);
				float rot = rot1 + WrapDegrees(pr0 - r5) * PI / 180.0f;
				float ss = sinf(rot), cc = cosf(rot), dd = (i + 1) * 2.0f;
				place(DPART_TAIL1 + i, -(ss1 * 1.5f + ss * dd) * ccTilt, py0 - y5 - (dd + 1.5f) * ssTilt + 1.5f, (cc1 * 1.5f + cc * dd) * ccTilt);
			}
			DragonTouches(hero);
			d.inWall = DragonWalls(DPART_HEAD) | DragonWalls(DPART_NECK) | DragonWalls(DPART_BODY);
			// Gothic's ground isn't Minecraft's: the dragon can't eat through it, so it keeps 3 blocks over it
			// (and over the ground 10 ticks ahead), but where it lands
			float podium = sqrtf((d.x - d.ox) * (d.x - d.ox) + (d.z - d.oz) * (d.z - d.oz));
			if (!DragonSitting() && !((d.phase == DP_LANDING || d.phase == DP_DYING) && podium < 12.0f))
			{
				float ground = GroundY(d.x, d.z, d.y), ahead = GroundY(d.x + d.vx * 10.0f, d.z + d.vz * 10.0f, d.y);
				float floor = (ground > ahead ? ground : ahead) + 3.0f;
				if (d.y < floor)
				{
					float up = floor - d.y;
					d.y += up < 1.0f ? up : 1.0f;
					d.vy = d.vy < 0.0f ? 0.0f : d.vy;
				}
			}
			static int logTicks = 0;
			if (++logTicks % 40 == 0)
				Log("dragon: at %.0f %.0f %.0f (ground %.0f), %s, target %.0f %.0f %.0f, speed %.2f, health %.0f", d.x, d.y, d.z,
					GroundY(d.x, d.z, d.y), DRAGON_PHASES[d.phase], d.tx, d.ty, d.tz, sqrtf(d.vx * d.vx + d.vy * d.vy + d.vz * d.vz), d.health);
			// its wings' flaps, its growl
			if (cosf(d.oFlapTime * 2.0f * PI) <= -0.3f && cosf(d.flapTime * 2.0f * PI) >= -0.3f)
			{
				char wings[32];
				sprintf_s(wings, "MC_DRAGON_WINGS%d.WAV", 1 + rand() % 6);
				if (d.vobs[DV_BODY] && zsound)
					if (zCSoundFX* fx = zsound->LoadSoundFX(zSTRING(wings)))
					{
						zCSoundSystem::zTSound3DParams params;
						params.SetDefaults();
						params.radius = 80.0f * BLOCK;
						zsound->PlaySound3D(fx, d.vobs[DV_BODY], zSND_SLOT_NONE, &params);
					}
			}
			if (!DragonSitting() && --d.growlTime < 0)
			{
				char growl[32];
				sprintf_s(growl, "MC_DRAGON_GROWL%d.WAV", 1 + rand() % 4);
				PlaySound(growl, d.vobs[DV_HEAD] ? d.vobs[DV_HEAD] : hero);
				d.growlTime = 200 + rand() % 200;
			}
		}

		static void BreathTick(oCNpc* hero)
		{
			oCWorld* game = ogame ? ogame->GetGameWorld() : nullptr;
			zCWorld* world = hero->GetHomeWorld();
			float now = Now();
			for (Breath& b : breaths)
			{
				if (b.age < 0)
					continue;
				if (++b.age >= b.duration)
				{
					b.age = -1;
					continue;
				}
				b.radius += b.grow;
				static const zCOLOR PURPLE = Argb(255, 200, 40, 220);
				for (int i = 0; i < 3; i++) // its purple
				{
					float a = Random01() * 2.0f * PI, r = sqrtf(Random01()) * b.radius;
					SpawnPuff(world, zVEC3(b.x + cosf(a) * r, b.y + 0.1f + Random01() * 0.4f, b.z + sinf(a) * r) * BLOCK, 0.35f, &PURPLE);
				}
				for (zCListSort<oCNpc>* node = game && game->voblist_npcs ? game->voblist_npcs->next : nullptr; node; node = node->next)
				{
					oCNpc* npc = node->data;
					if (!npc || npc->IsDead())
						continue;
					zVEC3 p = npc->GetPositionWorld() * (1.0f / BLOCK);
					float feet = (npc == hero && npc->human_ai ? npc->human_ai->feetY : npc->bbox3D.mins[1]) / BLOCK;
					float dx = p[0] - b.x, dz = p[2] - b.z;
					if (dx * dx + dz * dz > b.radius * b.radius || feet > b.y + 0.5f || npc->bbox3D.maxs[1] / BLOCK < b.y)
						continue;
					float& until = breathHitUntil[npc];
					if (now < until)
						continue;
					until = now + 20 * TICK; // AreaEffectCloud's reapplication
					McHurt(npc, 6.0f, oEDamageType_Magic, oEDamageIndex_Magic); // instant damage
					Log("dragon's breath: %s takes 6 of Minecraft's 20", npc->GetInstanceName().ToChar());
				}
			}
		}

		static void FireballTick(oCNpc* hero, Fireball& f)
		{
			f.last = f.pos;
			zVEC3 move = f.vel * BLOCK;
			float hit = 2.0f;
			zCWorld* world = f.vob->GetHomeWorld();
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_VOB_IGNORE_CHARACTER | zTRACERAY_VOB_IGNORE_NO_CD_DYN;
			if (world && world->TraceRayNearestHit(f.pos, move, f.vob, flags) && world->traceRayReport.foundHit)
				hit = (world->traceRayReport.foundIntersection - f.pos).Length() / (move.Length() > 0.001f ? move.Length() : 1.0f);
			oCWorld* game = ogame ? ogame->GetGameWorld() : nullptr;
			for (zCListSort<oCNpc>* node = game && game->voblist_npcs ? game->voblist_npcs->next : nullptr; node; node = node->next)
			{
				oCNpc* npc = node->data;
				if (!npc || npc->IsDead())
					continue;
				float t = RayBox(f.pos, move, npc->bbox3D);
				if (t >= 0.0f && t < hit)
					hit = t;
			}
			if (hit <= 1.0f || ++f.life > 600) // DragonFireball.onHit: its breath where it hit
			{
				zVEC3 at = (f.pos + move * (hit <= 1.0f ? hit : 0.0f)) * (1.0f / BLOCK);
				if (hit <= 1.0f)
				{
					Breathe(at[0], at[1], at[2], 3.0f, 600, (7.0f - 3.0f) / 600.0f);
					char bang[24];
					sprintf_s(bang, "MC_EXPLODE%d.WAV", 1 + rand() % 4);
					PlaySound(bang, f.vob);
					Log("dragon: its fireball breaks at %.0f %.0f %.0f", at[0], at[1], at[2]);
				}
				DropVob(f.vob);
				if (f.decal)
				{
					f.decal->Release();
					f.decal = nullptr;
				}
				return;
			}
			f.pos += move;
			float len = f.vel.Length();
			if (len > 0.0f)
				f.vel = (f.vel + f.vel * (0.1f / len)) * 0.95f; // AbstractHurtingProjectile: accelerating, 95 % kept
		}

		static zCVisual* dragonVisuals[14] = {};

		static void SpawnDragon(oCNpc* hero, const zVEC3& at)
		{
			if (dragon.alive)
			{
				Log("dragon: one is flying already");
				return;
			}
			oCWorld* world = ogame ? ogame->GetGameWorld() : nullptr;
			if (!world)
				return;
			static const char* const NAMES[14] = { "HEAD", "JAW", "SPINE", "BODY", "WING_L", "WINGTIP_L", "WING_R", "WINGTIP_R", "FRONTLEG",
				"FRONTLEGTIP", "FRONTFOOT", "HINDLEG", "HINDLEGTIP", "HINDFOOT" };
			for (int i = 0; i < 14; i++)
				if (!dragonVisuals[i])
				{
					char name[40];
					sprintf_s(name, "MC_DRAGON_%s.3DS", NAMES[i]);
					dragonVisuals[i] = zCVisual::LoadVisual(zSTRING(name)); // kept
					if (zCProgMeshProto* mesh = zDYNAMIC_CAST<zCProgMeshProto>(dragonVisuals[i])) // the wings' skin: see-through
						for (int m = 0; m < mesh->numSubMeshes; m++)
							if (zCMaterial* mat = mesh->subMeshList[m].material)
								if (!strcmp(mat->GetName().ToChar(), "MC_DRAGON_A"))
									mat->rndAlphaBlendFunc = zRND_ALPHA_FUNC_TEST;
				}
			Dragon& d = dragon;
			for (zCVob*& vob : d.vobs)
				vob = nullptr;
			int visual[DRAGON_VOBS];
			visual[DV_HEAD] = 0;
			visual[DV_JAW] = 1;
			for (int i = 0; i < 5; i++)
				visual[DV_NECK + i] = 2;
			visual[DV_BODY] = 3;
			visual[DV_WING_L] = 4;
			visual[DV_TIP_L] = 5;
			visual[DV_WING_R] = 6;
			visual[DV_TIP_R] = 7;
			for (int leg = 0; leg < 4; leg++)
				for (int j = 0; j < 3; j++)
					visual[DV_LEGS + leg * 3 + j] = (leg % 2 == 0 ? 8 : 11) + j;
			for (int i = 0; i < 12; i++)
				visual[DV_TAIL + i] = 2;
			zVEC3 b = at * (1.0f / BLOCK);
			for (int i = 0; i < DRAGON_VOBS; i++)
				d.vobs[i] = EffectVob(dragonVisuals[visual[i]], at);
			d.alive = true;
			d.ox = b[0];
			d.oy = GroundY(b[0], b[2], b[1]);
			d.oz = b[2];
			d.x = d.xo = b[0];
			d.y = d.yo = b[1] + 4.0f;
			d.z = d.zo = b[2];
			d.vx = d.vy = d.vz = 0.0f;
			d.yRot = (float)(rand() % 360) - 180.0f;
			d.yRotA = 0.0f;
			d.flapTime = d.oFlapTime = 0.0f;
			d.health = 200.0f;
			d.phase = -1;
			for (int i = 0; i < 64; i++) // record: the first sample fills the history
			{
				d.histY[i] = d.y;
				d.histYRot[i] = d.yRot;
			}
			d.histHead = 0;
			d.pathLen = d.pathNext = 0;
			d.hasTarget = false;
			d.clockwise = false;
			d.flameCount = 0;
			d.sittingDamage = 0.0f;
			d.deathTime = 0;
			d.growlTime = 100;
			d.inWall = false;
			for (int i = 0; i < DRAGON_PARTS; i++)
			{
				d.px[i] = d.x;
				d.py[i] = d.y;
				d.pz[i] = d.z;
			}
			MakeDragonNodes();
			for (int i = 12; i < 24; i++)
				Log("dragon: node %d at %d %d %d", i, d.nodes[i].x, d.nodes[i].y, d.nodes[i].z);
			SetDragonPhase(DP_HOLDING);
			char growl[32];
			sprintf_s(growl, "MC_DRAGON_GROWL%d.WAV", 1 + rand() % 4);
			PlaySound(growl, d.vobs[DV_HEAD]);
			Log("dragon: hatched at %.0f %.0f %.0f (its ground %.0f)", d.x, d.y, d.z, d.oy);
			if (zCCamera* camera = zCCamera::activeCam) // how far Gothic draws things (a dragon farther is gone)
				Log("view: far clip %.0f cm, objects' %d cm; the world's objects' %.0f cm (x %.2f), its tree's %.0f cm", camera->farClipZ,
					camera->vobFarClipZ, world->vobFarClipZ, world->vobFarClipZScalability, world->bspTree.vobFarClipZ);
		}

		// EnderDragonModel.setupAnim and EnderDragonRenderer: every part's place in the world, each frame
		static void PlaceDragon(float share)
		{
			Dragon& d = dragon;
			auto hist = [&](int delay, float& y, float& yRot)
			{
				int i = (d.histHead - delay) & 63, o = (d.histHead - delay - 1) & 63;
				y = Lerp(d.histY[o], d.histY[i], share);
				yRot = d.histYRot[o] + WrapDegrees(d.histYRot[i] - d.histYRot[o]) * share;
			};
			float flap = Lerp(d.oFlapTime, d.flapTime, share) * 2.0f * PI;
			float x = Lerp(d.xo, d.x, share), y = Lerp(d.yo, d.y, share), z = Lerp(d.zo, d.z, share);
			float y7, yr7, y5, yr5, y10, yr10;
			hist(7, y7, yr7);
			hist(5, y5, yr5);
			hist(10, y10, yr10);
			// the renderer: at its place, turned by its heading 7 ticks ago, tilted by its climb
			HandPose world;
			world.Move(x, y, z).Turn(1, -yr7).Turn(0, (y5 - y10) * 10.0f).Move(0.0f, 0.0f, 1.0f).Turn(2, 180.0f).Move(0.0f, -1.501f, 0.0f);
			// the model's root
			float bounce = sinf(flap - 1.0f) + 1.0f;
			bounce = (bounce * bounce + bounce * 2.0f) * 0.05f;
			HandPose root = world;
			root.Move(0.0f, (bounce - 2.0f), -48.0f / 16.0f).Turn(0, bounce * 2.0f);
			auto part = [&](const HandPose& parent, float px, float py, float pz, float xr, float yr, float zr)
			{
				HandPose p = parent;
				p.Move(px / 16.0f, py / 16.0f, pz / 16.0f).Turn(2, zr * 180.0f / PI).Turn(1, yr * 180.0f / PI).Turn(0, xr * 180.0f / PI);
				return p;
			};
			auto set = [&](int index, const HandPose& p)
			{
				zCVob* vob = d.vobs[index];
				if (!vob)
					return;
				zMAT4 m = zMAT4::GetIdentity();
				m.SetRightVector(zVEC3(p.r[0][0], p.r[1][0], p.r[2][0]));
				m.SetUpVector(zVEC3(p.r[0][1], p.r[1][1], p.r[2][1]));
				m.SetAtVector(zVEC3(p.r[0][2], p.r[1][2], p.r[2][2]));
				m.SetTranslation(zVEC3(p.t[0], p.t[1], p.t[2]) * BLOCK);
				vob->SetTrafoObjToWorld(m);
			};
			float sy6, syr6;
			hist(6, sy6, syr6);
			float r5y, r5r, r10y, r10r;
			hist(5, r5y, r5r);
			hist(10, r10y, r10r);
			float rot2 = WrapDegrees(r5r - r10r);
			float rot = WrapDegrees(r5r + rot2 / 2.0f);
			bool landing = d.phase == DP_LANDING || d.phase == DP_TAKEOFF, sitting = DragonSitting();
			float egg = sqrtf((d.x - d.ox) * (d.x - d.ox) + (d.y - d.oy) * (d.y - d.oy) + (d.z - d.oz) * (d.z - d.oz));
			auto headOffset = [&](int partIndex, float bodyY, float partY) // getHeadPartYOffset
			{
				if (landing)
					return partIndex / (egg / 4.0f > 1.0f ? egg / 4.0f : 1.0f);
				if (sitting)
					return (float)partIndex;
				if (partIndex == 6)
					return 0.0f;
				return partY - bodyY;
			};
			float xx = 0.0f, yy = 20.0f, zz = -12.0f;
			for (int i = 0; i < 5; i++)
			{
				float py, pr;
				hist(5 - i, py, pr);
				float yRot = WrapDegrees(pr - syr6) * PI / 180.0f * 1.5f;
				float xRot = cosf(i * 0.45f + flap) * 0.15f + headOffset(i, sy6, py) * PI / 180.0f * 1.5f * 5.0f;
				float zRot = -WrapDegrees(pr - rot) * PI / 180.0f * 1.5f;
				set(DV_NECK + i, part(root, xx, yy, zz, xRot, yRot, zRot));
				xx -= sinf(yRot) * cosf(xRot) * 10.0f;
				yy += sinf(xRot) * 10.0f;
				zz -= cosf(yRot) * cosf(xRot) * 10.0f;
			}
			float cy, cr;
			hist(0, cy, cr);
			HandPose head = part(root, xx, yy, zz, WrapDegrees(headOffset(6, sy6, cy)) * PI / 180.0f * 1.5f * 5.0f,
				WrapDegrees(cr - syr6) * PI / 180.0f, -WrapDegrees(cr - rot) * PI / 180.0f);
			set(DV_HEAD, head);
			set(DV_JAW, part(head, 0.0f, 4.0f, -8.0f, (sinf(flap) + 1.0f) * 0.2f, 0.0f, 0.0f));
			HandPose body = part(root, 0.0f, 20.0f, 8.0f, 0.0f, 0.0f, -rot2 * 1.5f * PI / 180.0f);
			set(DV_BODY, body);
			float wingX = 0.125f - cosf(flap) * 0.2f, wingZ = -(sinf(flap) + 0.125f) * 0.8f, tipZ = (sinf(flap + 2.0f) + 0.5f) * 0.75f;
			HandPose wingL = part(body, 12.0f, -15.0f, -6.0f, wingX, -0.25f, wingZ);
			set(DV_WING_L, wingL);
			set(DV_TIP_L, part(wingL, 56.0f, 0.0f, 0.0f, 0.0f, 0.0f, tipZ));
			HandPose wingR = part(body, -12.0f, -15.0f, -6.0f, wingX, 0.25f, -wingZ);
			set(DV_WING_R, wingR);
			set(DV_TIP_R, part(wingR, -56.0f, 0.0f, 0.0f, 0.0f, 0.0f, -tipZ));
			for (int side = 0; side < 2; side++) // poseLimbs
			{
				float s = side == 0 ? 1.0f : -1.0f;
				HandPose front = part(body, 12.0f * s, 0.0f, -6.0f, 1.3f + bounce * 0.1f, 0.0f, 0.0f);
				HandPose frontTip = part(front, 0.0f, 20.0f, -1.0f, -0.5f - bounce * 0.1f, 0.0f, 0.0f);
				HandPose frontFoot = part(frontTip, 0.0f, 23.0f, 0.0f, 0.75f + bounce * 0.1f, 0.0f, 0.0f);
				HandPose hind = part(body, 16.0f * s, -4.0f, 34.0f, 1.0f + bounce * 0.1f, 0.0f, 0.0f);
				HandPose hindTip = part(hind, 0.0f, 32.0f, -4.0f, 0.5f + bounce * 0.1f, 0.0f, 0.0f);
				HandPose hindFoot = part(hindTip, 0.0f, 31.0f, 4.0f, 0.75f + bounce * 0.1f, 0.0f, 0.0f);
				int base = DV_LEGS + side * 6;
				set(base, front);
				set(base + 1, frontTip);
				set(base + 2, frontFoot);
				set(base + 3, hind);
				set(base + 4, hindTip);
				set(base + 5, hindFoot);
			}
			float tailX = 0.0f;
			xx = 0.0f;
			yy = 10.0f;
			zz = 60.0f;
			float sy11, syr11;
			hist(11, sy11, syr11);
			for (int i = 0; i < 12; i++)
			{
				float py, pr;
				hist(12 + i, py, pr);
				tailX += sinf(i * 0.45f + flap) * 0.05f;
				float yRot = (WrapDegrees(pr - syr11) * 1.5f + 180.0f) * PI / 180.0f;
				float xRot = tailX + (py - sy11) * PI / 180.0f * 1.5f * 5.0f;
				float zRot = WrapDegrees(pr - rot) * PI / 180.0f * 1.5f;
				set(DV_TAIL + i, part(root, xx, yy, zz, xRot, yRot, zRot));
				yy += sinf(xRot) * 10.0f;
				zz -= cosf(yRot) * cosf(xRot) * 10.0f;
				xx -= sinf(yRot) * cosf(xRot) * 10.0f;
			}
		}

		static void UpdateDragon(oCNpc* hero)
		{
			float dt = ztimer->frameTimeFloat / 1000.0f;
			dragonClock += dt > 0.25f ? 0.25f : dt;
			while (dragonClock >= TICK)
			{
				dragonClock -= TICK;
				if (dragon.alive)
					DragonTick(hero);
				for (Fireball& f : fireballs)
					if (f.vob)
						FireballTick(hero, f);
				BreathTick(hero);
			}
			float share = dragonClock / TICK;
			if (dragon.alive)
				PlaceDragon(share);
			for (Fireball& f : fireballs)
				if (f.vob)
					f.vob->SetPositionWorld(f.last + (f.pos - f.last) * share);
		}

		// BossHealthOverlay: its pink bar and name at the top
		static void DrawBossBar()
		{
			if (!dragon.alive)
				return;
			float x = (float)(guiW / 2 - 91), y = 12.0f;
			Blit("boss_bar/pink_background", x, y);
			float share = dragon.health / 200.0f;
			share = share < 0.0f ? 0.0f : (share > 1.0f ? 1.0f : share);
			if (share > 0.0f)
				Blit("boss_bar/pink_progress", x, y, share);
			Text(bossName, floorf((guiW - TextWidth(bossName)) / 2.0f), y - 9.0f, Argb(255, 255, 255, 255));
		}

		static bool DragonMelee(oCNpc* hero, float npcDistance, float damage)
		{
			float t;
			zVEC3 ray = Look() * 300.0f;
			int part = DragonPartOnRay(Eyes(hero), ray, t);
			if (part < 0 || t * 300.0f >= npcDistance)
				return false;
			DragonHurt(hero, part, damage / 10.0f, false);
			return true;
		}

		static void DragonExplosion(oCNpc* hero, const zVEC3& at, float radius)
		{
			if (!dragon.alive)
				return;
			float d = (DragonPartCentre(DPART_BODY) - at).Length() / BLOCK / radius;
			if (d > 1.0f)
				return;
			float impact = 1.0f - d;
			DragonHurt(hero, DPART_BODY, (impact * impact + impact) / 2.0f * 7.0f * radius + 1.0f, false);
		}

		static void ForgetDragon()
		{
			for (zCVob*& vob : dragon.vobs)
				DropVob(vob);
			dragon.alive = false;
			for (Fireball& f : fireballs)
			{
				DropVob(f.vob);
				if (f.decal)
				{
					f.decal->Release();
					f.decal = nullptr;
				}
			}
			for (Breath& b : breaths)
				b.age = -1;
			dragonHitUntil.clear();
			breathHitUntil.clear();
		}

		// Before a world goes (a load, another level): the arrows, lit TNT and the hand out of it, and the
		// plugin's lists of its blocks and ladders forgotten (found again in the next one).
		static void ForgetWorld()
		{
			for (Arrow& a : arrows)
				RemoveArrow(a);
			for (Tnt& t : tnts)
				RemoveTnt(t);
			for (auto& f : fireCells)
				DropVob(f.second.vob);
			fireCells.clear();
			for (auto& l : lavaCells)
				DropVob(l.second.vob);
			lavaCells.clear();
			for (Falling& f : fallingBlocks)
				DropVob(f.vob);
			for (auto& b : burning)
				DropVob(b.second.flames);
			burning.clear();
			fallChecks.clear();
			gothicSolid.clear();
			for (Puff& q : puffs)
				if (q.vob)
				{
					if (zCWorld* world = q.vob->GetHomeWorld())
						world->RemoveVob(q.vob);
					q.vob->Release();
					q.decal->Release();
					q.vob = nullptr;
					q.decal = nullptr;
					q.live = false;
				}
			for (Seed& seed : seeds)
				seed.age = -1;
			for (zCVob*& boom : boomVobs)
				if (boom)
				{
					if (zCWorld* world = boom->GetHomeWorld())
						world->RemoveVob(boom);
					boom->Release();
					boom = nullptr;
				}
			if (handVob)
			{
				if (zCWorld* world = handVob->GetHomeWorld())
					world->RemoveVob(handVob);
				handVob->Release();
				handVob = nullptr;
			}
			hiddenHero = nullptr;
			camHealth = -1;
			ForgetDragon();
			for (Effect& e : effects)
				if (e.effect)
				{
					e.effect->StopEmitterOutput();
					e.effect->Release();
					e.effect = nullptr;
				}
			useTicks = -1;
			useUntil = putAwayUntil = -1.0f;
			blocks.clear();
			blockKinds.clear();
			blocksWorld = nullptr;
			ladders.clear();
			laddersWorld = nullptr;
			Log("world: forgotten (a load or another level)");
		}

		// --- compile run -----------------------------------------------------------------------------
		static bool CompileRun()
		{
			static int run = -1;
			if (run < 0)
			{
				char line[4096];
				strncpy_s(line, GetCommandLineA(), _TRUNCATE);
				_strlwr_s(line);
				run = strstr(line, "-mccompile") != nullptr;
			}
			return run == 1;
		}

		// Gothic's loader under structured exception handling (no C++ objects in here).
		static zCModelPrototype* GuardedLoad(const zSTRING& name, zCModelPrototype* base, DWORD& code, void*& where)
		{
			__try
			{
				return zCModelPrototype::Load(name, base);
			}
			__except (code = GetExceptionCode(), where = GetExceptionInformation()->ExceptionRecord->ExceptionAddress,
			          EXCEPTION_EXECUTE_HANDLER)
			{
				return nullptr;
			}
		}

		static void Compile()
		{
			static bool done = false;
			if (done)
				return;
			done = true;
			LoadSettings();
			Log("compile run: Steve's model, body and overlays");
			DWORD code = 0;
			void* where = nullptr;
			zCModelPrototype* base = GuardedLoad(zSTRING("STEVE.MDS"), nullptr, code, where);
			Log("compile run: STEVE.MDS %s (exception %08X at %p)", base ? "loaded" : "not loaded", code, where);
			if (base)
			{
				WIN32_FIND_DATAA found;
				HANDLE find = FindFirstFileA("_work\\DATA\\ANIMS\\STEVE_*.MDS", &found);
				int overlays = 0;
				if (find != INVALID_HANDLE_VALUE)
				{
					do
					{
						zCModelPrototype* proto = GuardedLoad(zSTRING(found.cFileName), base, code, where);
						Log("compile run: %s %s (exception %08X at %p)", found.cFileName, proto ? "loaded" : "not loaded", code, where);
						if (proto)
						{
							proto->Release();
							overlays++;
						}
					} while (FindNextFileA(find, &found));
					FindClose(find);
				}
				Log("compile run: %d overlays", overlays);
				// the mobs' bodies: a model's registerMesh compiles only the first; Gothic converts a body
				// when a model puts it on (as an armour the first time it is worn)
				// (the model and the prototype stay: freeing them here stopped Gothic, the prototype's count
				// not down to zero; the process ends right after)
				zCModel* model = new zCModel(base);
				for (const char* body : { "MC_ZOMBIE_BODY", "MC_SKELETON_BODY" })
					Log("compile run: %s %s", body, model->ApplyMeshLib(zSTRING(body)) ? "compiled" : "not compiled");
			}
			zCModelPrototype* creeper = GuardedLoad(zSTRING("MCCREEPER.MDS"), nullptr, code, where);
			Log("compile run: MCCREEPER.MDS %s (exception %08X at %p)", creeper ? "loaded" : "not loaded", code, where);
			if (creeper)
				creeper->Release();
			// every mesh of make_mc_blocks.py and make_mc_items.py (blocks, items, the hand)
			WIN32_FIND_DATAA mesh;
			HANDLE meshes = FindFirstFileA("_work\\DATA\\MESHES\\MC\\*.3DS", &mesh);
			int count = 0;
			if (meshes != INVALID_HANDLE_VALUE)
			{
				do
				{
					if (zCVisual* v = zCVisual::LoadVisual(zSTRING(mesh.cFileName)))
					{
						v->Release();
						count++;
					}
				} while (FindNextFileA(meshes, &mesh));
				FindClose(meshes);
			}
			Log("compile run: %d meshes of MESHES\\MC", count);
			Log("compile run: done, quitting");
			// Gothic's own exit from its menu in a background window takes minutes (the GTA mashup's
			// compile run); what it compiled is written and closed, so the process ends here
			TerminateProcess(GetCurrentProcess(), 0);
		}
	}

	// In the compile run Gothic's warnings, faults and fatal errors go into the log: a fatal one
	// ends the game with exit code 0 and no message.
	int __fastcall Mc_zERROR_Report(zERROR* self, void* vtable, zERROR_TYPE type, int id, const zSTRING& text, signed char level,
		unsigned int flag, int line, char* file, char* function);
	auto Hook_Mc_zERROR_Report = Union::CreateHook(
		SIGNATURE_OF(static_cast<int (zERROR::*)(zERROR_TYPE, int, const zSTRING&, signed char, unsigned int, int, char*, char*)>(&zERROR::Report)),
		&Mc_zERROR_Report, Union::HookType::Hook_Detours);
	int __fastcall Mc_zERROR_Report(zERROR* self, void* vtable, zERROR_TYPE type, int id, const zSTRING& text, signed char level,
		unsigned int flag, int line, char* file, char* function)
	{
		if (Minecraft::CompileRun() ? type >= zERR_TYPE_WARN : type >= zERR_TYPE_FAULT)
			Minecraft::Log("%sGothic %s: %s (plugin step: %s)", Minecraft::CompileRun() ? "compile run: " : "",
				type == zERR_TYPE_FATAL ? "FATAL" : type == zERR_TYPE_FAULT ? "fault" : "warning", text.ToChar(), Minecraft::step);
		return Hook_Mc_zERROR_Report(self, vtable, type, id, text, level, flag, line, file, function);
	}

	// The compile run starts on the first frame Gothic draws (its menu).
	void __fastcall Mc_zCView_DrawItems(zCView* self, void* vtable);
	auto Hook_Mc_zCView_DrawItems = Union::CreateHook(SIGNATURE_OF(&zCView::DrawItems), &Mc_zCView_DrawItems, Union::HookType::Hook_Detours);
	void __fastcall Mc_zCView_DrawItems(zCView* self, void* vtable)
	{
		if (self == screen && Minecraft::CompileRun())
			Minecraft::Compile();
		if (self == screen)
		{
			Minecraft::step = "hud";
			Minecraft::DrawHud();
			Minecraft::step = "Gothic's drawing";
		}
		Hook_Mc_zCView_DrawItems(self, vtable);
	}

	// Gothic's overlays on Steve: their STEVE_* versions (see the header).
	int __fastcall Mc_zCModel_ApplyModelProtoOverlay(zCModel* self, void* vtable, zSTRING const& name);
	auto Hook_Mc_zCModel_ApplyModelProtoOverlay = Union::CreateHook(
		SIGNATURE_OF(static_cast<int (zCModel::*)(zSTRING const&)>(&zCModel::ApplyModelProtoOverlay)),
		&Mc_zCModel_ApplyModelProtoOverlay, Union::HookType::Hook_Detours);
	int __fastcall Mc_zCModel_ApplyModelProtoOverlay(zCModel* self, void* vtable, zSTRING const& name)
	{
		zSTRING steve;
		bool known = false;
		if (!Minecraft::SteveOverlay(self, name, steve, known))
			return Hook_Mc_zCModel_ApplyModelProtoOverlay(self, vtable, name);
		Minecraft::Log("overlay %s: %s", name.ToChar(), known ? steve.ToChar() : "none for Steve, left off");
		return known ? Hook_Mc_zCModel_ApplyModelProtoOverlay(self, vtable, steve) : FALSE;
	}

	void __fastcall Mc_zCModel_RemoveModelProtoOverlay(zCModel* self, void* vtable, zSTRING const& name);
	auto Hook_Mc_zCModel_RemoveModelProtoOverlay = Union::CreateHook(
		SIGNATURE_OF(static_cast<void (zCModel::*)(zSTRING const&)>(&zCModel::RemoveModelProtoOverlay)),
		&Mc_zCModel_RemoveModelProtoOverlay, Union::HookType::Hook_Detours);
	void __fastcall Mc_zCModel_RemoveModelProtoOverlay(zCModel* self, void* vtable, zSTRING const& name)
	{
		zSTRING steve;
		bool known = false;
		if (!Minecraft::SteveOverlay(self, name, steve, known))
			return Hook_Mc_zCModel_RemoveModelProtoOverlay(self, vtable, name);
		if (known)
			Hook_Mc_zCModel_RemoveModelProtoOverlay(self, vtable, steve);
	}

	// Minecraft's look for Gothic's zombies and skeletons, and the creeper (see Minecraft::MobKind).
	void __fastcall Mc_oCNpc_SetAdditionalVisuals(oCNpc* self, void* vtable, zSTRING& body, int bodyTex, int bodyColor, zSTRING& head,
		int headTex, int teethTex, int armor);
	auto Hook_Mc_oCNpc_SetAdditionalVisuals = Union::CreateHook(SIGNATURE_OF(&oCNpc::SetAdditionalVisuals), &Mc_oCNpc_SetAdditionalVisuals,
		Union::HookType::Hook_Detours);
	void __fastcall Mc_oCNpc_SetAdditionalVisuals(oCNpc* self, void* vtable, zSTRING& body, int bodyTex, int bodyColor, zSTRING& head,
		int headTex, int teethTex, int armor)
	{
		Minecraft::LoadSettings();
		int kind = Minecraft::MobKind(self, body);
		if (kind == Minecraft::MOB_NONE)
			return Hook_Mc_oCNpc_SetAdditionalVisuals(self, vtable, body, bodyTex, bodyColor, head, headTex, teethTex, armor);
		static const char* const BODIES[] = { "", "MC_ZOMBIE_BODY", "MC_SKELETON_BODY", "MC_CREEPER_BODY" };
		self->mds_name = zSTRING(kind == Minecraft::MOB_CREEPER ? "MCCREEPER.MDS" : "STEVE.MDS");
		zSTRING mobBody(BODIES[kind]), noHead("");
		Hook_Mc_oCNpc_SetAdditionalVisuals(self, vtable, mobBody, 0, 0, noHead, 0, 0, armor);
		if (kind == Minecraft::MOB_ZOMBIE)
			self->ApplyOverlay(zSTRING("STEVE_ZOMBIE.MDS"));
		Minecraft::Log("mob: %s as Minecraft's %s", self->GetInstanceName().ToChar(), BODIES[kind]);
	}

	// Armour on Steve: its protection, not its look.
	void __fastcall Mc_oCNpc_EquipArmor(oCNpc* self, void* vtable, oCItem* item);
	auto Hook_Mc_oCNpc_EquipArmor = Union::CreateHook(SIGNATURE_OF(&oCNpc::EquipArmor), &Mc_oCNpc_EquipArmor, Union::HookType::Hook_Detours);
	void __fastcall Mc_oCNpc_EquipArmor(oCNpc* self, void* vtable, oCItem* item)
	{
		if (!item || !Minecraft::IsSteve(self))
			return Hook_Mc_oCNpc_EquipArmor(self, vtable, item);
		Minecraft::Log("armour %s on: Steve keeps his look", item->GetInstanceName().ToChar());
		zSTRING look = item->visual_change;
		item->visual_change = "";
		Hook_Mc_oCNpc_EquipArmor(self, vtable, item);
		item->visual_change = look;
	}

	// The hero faces where the camera looks while Minecraft's camera is on.
	void __fastcall Mc_oCAIHuman_DoAI(oCAIHuman* self, void* vtable, zCVob* vob, int& vobDeleted);
	auto Hook_Mc_oCAIHuman_DoAI = Union::CreateHook(SIGNATURE_OF(&oCAIHuman::DoAI), &Mc_oCAIHuman_DoAI, Union::HookType::Hook_Detours);
	void __fastcall Mc_oCAIHuman_DoAI(oCAIHuman* self, void* vtable, zCVob* vob, int& vobDeleted)
	{
		oCNpc* hero = oCNpc::player;
		if (hero && self == hero->human_ai)
		{
			Minecraft::LoadSettings();
			Minecraft::InstallCrashLog();
			static bool started = false;
			if (!started)
			{
				started = true;
				Minecraft::Log("Minecraft plugin (%s), hero %s", Minecraft::GOTHIC2 ? "Gothic 2" : "Gothic 1",
					Minecraft::IsSteve(hero) ? "Steve" : "not Steve");
			}
			if (Minecraft::IsSteve(hero))
				Minecraft::FullDetail(hero);
			if (Minecraft::cameraActive && Minecraft::ControlsActive(hero))
				hero->SetHeadingAtWorld(Minecraft::Forward());
			Minecraft::step = "movement";
			Minecraft::UpdateMovement(hero);
			Minecraft::step = "hunger";
			Minecraft::UpdateHunger(hero);
			Minecraft::step = "world";
			Minecraft::UpdateWorld(hero);
			Minecraft::step = "Gothic's AI (hero)";
			if (Minecraft::physicsOn)
			{
				Minecraft::overrideInput = true;
				Hook_Mc_oCAIHuman_DoAI(self, vtable, vob, vobDeleted);
				Minecraft::overrideInput = false;
				Minecraft::useEdge = false;
				return;
			}
			Minecraft::useEdge = false;
		}
		Hook_Mc_oCAIHuman_DoAI(self, vtable, vob, vobDeleted);
	}

	// While Minecraft's movement is on, Gothic's movement keys reach the hero's AI only as
	// Minecraft::OverrideKey answers them.
	float __fastcall Mc_zCInput_Win32_GetState(zCInput_Win32* self, void* vtable, unsigned short key);
	auto Hook_Mc_zCInput_Win32_GetState = Union::CreateHook(SIGNATURE_OF(&zCInput_Win32::GetState), &Mc_zCInput_Win32_GetState, Union::HookType::Hook_Detours);
	float __fastcall Mc_zCInput_Win32_GetState(zCInput_Win32* self, void* vtable, unsigned short key)
	{
		float state;
		int toggled;
		return Minecraft::OverrideKey(key, state, toggled) ? state : Hook_Mc_zCInput_Win32_GetState(self, vtable, key);
	}

	int __fastcall Mc_zCInput_Win32_GetToggled(zCInput_Win32* self, void* vtable, unsigned short key);
	auto Hook_Mc_zCInput_Win32_GetToggled = Union::CreateHook(SIGNATURE_OF(&zCInput_Win32::GetToggled), &Mc_zCInput_Win32_GetToggled, Union::HookType::Hook_Detours);
	int __fastcall Mc_zCInput_Win32_GetToggled(zCInput_Win32* self, void* vtable, unsigned short key)
	{
		float state;
		int toggled;
		return Minecraft::OverrideKey(key, state, toggled) ? toggled : Hook_Mc_zCInput_Win32_GetToggled(self, vtable, key);
	}

	// His model's animations advanced: Minecraft's move as the root motion, Minecraft's pose.
	void __fastcall Mc_zCModel_AdvanceAnis(zCModel* self, void* vtable);
	auto Hook_Mc_zCModel_AdvanceAnis = Union::CreateHook(SIGNATURE_OF(&zCModel::AdvanceAnis), &Mc_zCModel_AdvanceAnis, Union::HookType::Hook_Detours);
	void __fastcall Mc_zCModel_AdvanceAnis(zCModel* self, void* vtable)
	{
		Hook_Mc_zCModel_AdvanceAnis(self, vtable);
		oCNpc* hero = oCNpc::player;
		if (Minecraft::physicsOn && hero && hero->GetVisual() == static_cast<zCVisual*>(self))
		{
			const char* was = Minecraft::step;
			Minecraft::step = "pose";
			Minecraft::AfterAnis(hero, self);
			Minecraft::step = was;
		}
	}

	// Gothic's fall check (starting a fall from 60 cm above the floor, landing) is off for the hero
	// while his movement is Minecraft's: the plugin's jumps and falls start and land there.
	void __fastcall Mc_zCAIPlayer_CheckPhysics(zCAIPlayer* self, void* vtable);
	auto Hook_Mc_zCAIPlayer_CheckPhysics = Union::CreateHook(SIGNATURE_OF(&zCAIPlayer::CheckPhysics), &Mc_zCAIPlayer_CheckPhysics, Union::HookType::Hook_Detours);
	void __fastcall Mc_zCAIPlayer_CheckPhysics(zCAIPlayer* self, void* vtable)
	{
		oCNpc* hero = oCNpc::player;
		if (Minecraft::physicsOn && hero && self == static_cast<zCAIPlayer*>(hero->human_ai))
			return;
		Hook_Mc_zCAIPlayer_CheckPhysics(self, vtable);
	}

	// Gothic turns the hero with the mouse and its turn keys here; with Minecraft's camera the mouse does.
	void __fastcall Mc_oCAIHuman_PC_Turnings(oCAIHuman* self, void* vtable, int force);
	auto Hook_Mc_oCAIHuman_PC_Turnings = Union::CreateHook(SIGNATURE_OF(&oCAIHuman::PC_Turnings), &Mc_oCAIHuman_PC_Turnings, Union::HookType::Hook_Detours);
	void __fastcall Mc_oCAIHuman_PC_Turnings(oCAIHuman* self, void* vtable, int force)
	{
		oCNpc* hero = oCNpc::player;
		if (hero && self == hero->human_ai && Minecraft::cameraActive && Minecraft::ControlsActive(hero))
			return;
		Hook_Mc_oCAIHuman_PC_Turnings(self, vtable, force);
	}

	void __fastcall Mc_zCAICamera_DoAI(zCAICamera* self, void* vtable, zCVob* vob, int& vobDeleted);
	auto Hook_Mc_zCAICamera_DoAI = Union::CreateHook(SIGNATURE_OF(&zCAICamera::DoAI), &Mc_zCAICamera_DoAI, Union::HookType::Hook_Detours);
	void __fastcall Mc_zCAICamera_DoAI(zCAICamera* self, void* vtable, zCVob* vob, int& vobDeleted)
	{
		Hook_Mc_zCAICamera_DoAI(self, vtable, vob, vobDeleted);
		Minecraft::step = "camera";
		Minecraft::UpdateCamera(self);
		Minecraft::step = "hand";
		Minecraft::UpdateHand(self);
		Minecraft::step = "Gothic's frame";
	}

	// Lava and fire full bright (Minecraft::FullBright): the lights Gothic collects for them give way to
	// one white ambient light, so their sides glow as their tops.
	void __fastcall Mc_zCRenderLightContainer_CollectLights(zCRenderLightContainer* self, const zTRenderContext& context);
	auto Hook_Mc_zCRenderLightContainer_CollectLights = Union::CreateHook(SIGNATURE_OF(&zCRenderLightContainer::CollectLights),
		&Mc_zCRenderLightContainer_CollectLights, Union::HookType::Hook_Detours);
	void __fastcall Mc_zCRenderLightContainer_CollectLights(zCRenderLightContainer* self, const zTRenderContext& context)
	{
		Hook_Mc_zCRenderLightContainer_CollectLights(self, context);
		if (context.vob && Minecraft::FullBright(context.vob))
		{
			self->numLights = 1;
			zCRenderLight& light = self->lightList[0];
			light.lightType = zLIGHT_TYPE_AMBIENT;
			light.colorDiffuse = zVEC3(255.0f, 255.0f, 255.0f);
		}
	}

	// F5 switches the camera while Minecraft's is on (Gothic 2's quicksave key then does nothing).
	int __fastcall Mc_oCGame_HandleEvent(oCGame* self, void* vtable, int key);
	auto Hook_Mc_oCGame_HandleEvent = Union::CreateHook(SIGNATURE_OF(&oCGame::HandleEvent), &Mc_oCGame_HandleEvent, Union::HookType::Hook_Detours);
	int __fastcall Mc_oCGame_HandleEvent(oCGame* self, void* vtable, int key)
	{
		if (key == KEY_F5 && Minecraft::cameraActive)
		{
			Minecraft::NextView();
			return TRUE;
		}
		oCNpc* hero = oCNpc::player;
		if (key == KEY_E && Minecraft::cameraActive) // the creative inventory
		{
			Minecraft::OpenInventory(!Minecraft::inventoryOpen);
			return TRUE;
		}
		if (key == KEY_ESCAPE && Minecraft::inventoryOpen)
		{
			Minecraft::OpenInventory(false);
			return TRUE;
		}
		if (key >= KEY_1 && key <= KEY_9 && hero && Minecraft::cameraActive) // a hotbar slot
		{
			int slot = key - KEY_1;
			Minecraft::Entry entries[45];
			int count = Minecraft::InventoryEntries(entries);
			if (Minecraft::inventoryOpen && Minecraft::hoveredEntry >= 0 && Minecraft::hoveredEntry < count)
			{
				Minecraft::hotbar[slot] = entries[Minecraft::hoveredEntry];
				Minecraft::SaveHotbar();
				Minecraft::ShowToolName(hero);
			}
			else if (!Minecraft::inventoryOpen)
				Minecraft::ChooseSlot(hero, slot);
			Minecraft::Log("key %d: slot %d (%s)", slot + 1, slot + 1, Minecraft::toolName);
			return TRUE;
		}
		const char* was = Minecraft::step;
		Minecraft::step = "Gothic's key handling";
		int result = Hook_Mc_oCGame_HandleEvent(self, vtable, key);
		Minecraft::step = was;
		return result;
	}

	namespace Minecraft
	{
		static int PressGothicKey(int key) { return Hook_Mc_oCGame_HandleEvent(ogame, nullptr, key); }
	}

	// Gothic's health, breath and enemy bars give way to Minecraft's HUD; its mana bar stays while a
	// spell is drawn.
	void __fastcall Mc_oCGame_UpdatePlayerStatus(oCGame* self, void* vtable);
	auto Hook_Mc_oCGame_UpdatePlayerStatus = Union::CreateHook(SIGNATURE_OF(&oCGame::UpdatePlayerStatus), &Mc_oCGame_UpdatePlayerStatus, Union::HookType::Hook_Detours);
	void __fastcall Mc_oCGame_UpdatePlayerStatus(oCGame* self, void* vtable)
	{
		Hook_Mc_oCGame_UpdatePlayerStatus(self, vtable);
		oCNpc* hero = oCNpc::player;
		if (!Minecraft::settings.hud || !screen || !Minecraft::IsSteve(hero))
			return;
		bool spell = hero && hero->GetWeaponMode() == NPC_WEAPON_MAG;
		for (zCView* bar : { (zCView*)self->hpBar, (zCView*)self->swimBar, (zCView*)self->focusBar, spell ? nullptr : (zCView*)self->manaBar })
			if (bar)
				screen->RemoveItem(bar);
	}

	// Food fills Minecraft's food bar (oCNpc::UseItem: Gothic's use of an item, from the inventory
	// through oCNpc::Equip, and drinking).
	int __fastcall Mc_oCNpc_UseItem(oCNpc* self, void* vtable, oCItem* item);
	auto Hook_Mc_oCNpc_UseItem = Union::CreateHook(SIGNATURE_OF(&oCNpc::UseItem), &Mc_oCNpc_UseItem, Union::HookType::Hook_Detours);
	int __fastcall Mc_oCNpc_UseItem(oCNpc* self, void* vtable, oCItem* item)
	{
		if (self && self == oCNpc::player && item && (item->mainflag & 32) && Minecraft::settings.hunger) // ITEM_KAT_FOOD
			Minecraft::Eat(item);
		return Hook_Mc_oCNpc_UseItem(self, vtable, item);
	}

	// A new game or a loaded one: hunger full again. Before any world goes (a load, another level) the
	// plugin lets go of its objects in it (Minecraft::ForgetWorld).
	void __fastcall Mc_oCGame_LoadGame(oCGame* self, void* vtable, int slot, zSTRING const& level);
	auto Hook_Mc_oCGame_LoadGame = Union::CreateHook(SIGNATURE_OF(&oCGame::LoadGame), &Mc_oCGame_LoadGame, Union::HookType::Hook_Detours);
	void __fastcall Mc_oCGame_LoadGame(oCGame* self, void* vtable, int slot, zSTRING const& level)
	{
		Minecraft::ResetHunger();
		Minecraft::ForgetWorld();
		Hook_Mc_oCGame_LoadGame(self, vtable, slot, level);
	}

	void __fastcall Mc_oCGame_LoadSavegame(oCGame* self, void* vtable, int slot, int loadGlobals);
	auto Hook_Mc_oCGame_LoadSavegame = Union::CreateHook(SIGNATURE_OF(&oCGame::LoadSavegame), &Mc_oCGame_LoadSavegame, Union::HookType::Hook_Detours);
	void __fastcall Mc_oCGame_LoadSavegame(oCGame* self, void* vtable, int slot, int loadGlobals)
	{
		Minecraft::ResetHunger();
		Minecraft::ForgetWorld();
		Hook_Mc_oCGame_LoadSavegame(self, vtable, slot, loadGlobals);
	}

	void __fastcall Mc_oCGame_ChangeLevel(oCGame* self, void* vtable, zSTRING const& world, zSTRING const& start);
	auto Hook_Mc_oCGame_ChangeLevel = Union::CreateHook(SIGNATURE_OF(&oCGame::ChangeLevel), &Mc_oCGame_ChangeLevel, Union::HookType::Hook_Detours);
	void __fastcall Mc_oCGame_ChangeLevel(oCGame* self, void* vtable, zSTRING const& world, zSTRING const& start)
	{
		Minecraft::ForgetWorld();
		Hook_Mc_oCGame_ChangeLevel(self, vtable, world, start);
	}
}
