// GTA San Andreas on-foot controls, camera and HUD for CJ in Gothic 1 (GothicMod.exe 1.08k_mod).
//
// Movement follows GTA's CTaskSimplePlayerOnFoot::PlayerControlZelda (gta-reversed,
// Tasks/TaskTypes/TaskSimplePlayerOnFoot.cpp): W/A/S/D give a direction relative to the
// camera, CJ turns towards it at his GTA heading-change rate and runs forward. Gothic still
// plays the animations (CJ's own GTA ones), moves him by their root motion and handles
// collision, ledges and water. Holding Space blends CJ from his run into his GTA sprint
// (CJ_SPRINT.MDS) while he gathers speed. The mouse orbits a GTA-style follow camera, also with
// fists up. Dialogue, inventory and interaction modes keep Gothic's own controls and camera.
//
// Keys (GTA SA defaults): W/A/S/D move, mouse camera, Space sprint, Left Shift jump, Left Alt walk,
// C crouch/sneak, F use/talk/take, Q/E or the mouse wheel switch to the pistol and back. Unarmed,
// the left mouse button raises CJ's fists and punches (click again during a punch for GTA's combo)
// and the right one blocks; the fists come down after a few seconds without fighting. With the
// pistol the right button aims and the left fires. Gothic's own movement and combat keys do
// nothing in GTA mode (OverrideKey); its menu keys (inventory, status, log, map) stay.
//
// Pistol (make_cj_pistol.py turns Gothic's light crossbow into it and its bolts into 9 mm rounds):
// it is drawn as Gothic's crossbow, so NPCs see the drawn weapon as usual. Aiming and firing are
// GTA's (data/weapon.dat PISTOL): the right mouse button brings up GTA's over-the-shoulder camera
// and crosshair, CJ faces the camera, walks and strafes with GTA's gun walk and points his arm at
// the crosshair (nine baked poses blended by Gothic, in place of GTA's arm IK). A shot hits
// whatever is under the crosshair up to 35 m away for 25 of a ped's 100 health, plays GTA's
// pistol sound, kicks CJ's hand up as colt45_fire does, shows GTA's muzzle flash and alerts
// nearby NPCs; 17 rounds a clip, then CJ reloads (colt45_reload). The rounds are the ammunition
// items in the inventory. A round that hits the world kicks up dust (and sparks off stone and
// metal) where it lands, with Gothic's sound of metal hitting that ground, and a splash on water;
// each shot lights up its surroundings for a moment.
//
// Rocket launcher (make_cj_pistol.py makes Gothic's second crossbow GTA's rocket launcher, and an
// unused alchemy ingredient its rockets): drawn as a crossbow too, carried with GTA's rocket stance,
// walk and run (CJ_ROCKET.MDS overlay; heavy: no sprinting or jumping with it). The right button
// aims it in first person through GTA's rocket crosshair, the left fires (aimed or straight
// ahead): GTA's RocketFire, the missile flying with a fire and smoke trail until it hits something,
// then the explosion: a fireball some 5 m across with a column of smoke (Gothic's fire effects made
// bigger), a few small fires (spray instead on water), a flash of light; it throws people off their
// feet within rocketRadius, kills them close by, throws the bike, shakes the camera and is heard far
// away. Unaimed, the rocket flies straight ahead from the shoulder. CJ runs with it at his usual run
// speed.
//
// Purple dildo (make_cj_pistol.py makes Jackal's unused sword GTA's dildo): drawn as Gothic's
// one-hander, held and swung with GTA's dildo stance, block and three-hit combo (clicks as with the
// fists, melee.dat DILDO damage). Not GTA's doing: while it is out, CJ's enemies near him (hostile
// to him, or fighting him) run away from him with Gothic's own flee states, and anyone he hits
// with it runs too.
//
// Q/E or the mouse wheel cycle the weapons CJ has, as GTA does: fists, pistol, rocket launcher,
// dildo.
//
// Radar (CHud::DrawRadar): GTA's round radar in the bottom left corner, a map of Gothic's world
// (make_cj_radar.py) turned with the camera, CJ's arrow, north and hostile NPCs as red blips.
//
// Clock: with gtaClock on, Gothic's day runs at GTA's pace (a game minute every real second).
//
// Wasted: when CJ dies the camera does what San Andreas's does (CCam::ProcessPedsDeadBaby): from
// straight above his chest it looks down on him and rises, sliding to his side and swaying across.
//
// Motorbike (make_cj_bike.py, see CJ::Bike): GTA's bike stands next to CJ; F gets him on and off,
// W/S throttle and brake, A/D steer, Space rear brake, Up/Down lean.
//
// Jump (CTaskSimpleJump::Launch): Gothic starts the jump on Left Shift; as soon as the hero is
// airborne he gets GTA's launch (only for jumps the player asked for, not Gothic's own hops down
// steps and slopes): 8.5 force units up on his 70 kg, and forward 0.10 (standing/walking),
// 0.17 (running) or 0.22 (sprinting) metres per 1/50 s tick (the last two times runSpeed and
// sprintSpeed), kept until he lands. Ledge climbs stay Gothic's.
//
// CJ has animations for fists, his guns and the dildo only: a click always raises his fists (even
// with a sword equipped), and drawing a sword, bow, other crossbow or spell is blocked.
//
// Tuning, read from Gothic.ini section [CJMOVEMENT] at start:
//   turnRate=450        CJ's turn speed in degrees per second (GTA: 9 deg per 1/50 s tick)
//   mouseSpeed=0.25     camera degrees per mouse unit
//   invertMouseY=0
//   camDistance=300     follow distance in cm
//   camHeight=50        height of the point the camera looks at, above CJ's origin, in cm
//   camSmooth=0.08      seconds the camera takes to catch up with CJ (0 = no smoothing), so his
//                       steps, lunges and punches do not jerk the view
//   gtaHud=1            draw the GTA San Andreas HUD instead of Gothic's bars
//   fistDamage=1        CJ's punches deal GTA's damage (melee.dat UNARMED_1: 6, 9, 15, moving 5 of a
//                       ped's 100 health) as that share of the target's health, times this
//   runSpeed=0.75       CJ's run and sprint speed as a share of GTA's (run_player 5.7 m/s,
//   sprintSpeed=0.7     sprint_civi 9.7 m/s; Gothic's hero runs 3.45 m/s); the animations play
//                       slower to match, so his feet stay planted
//   sprintAccel=0.8     seconds from running to full sprint while Space is held
//   sprintDecel=0.5     seconds from full sprint back to running after Space is let go
//   gunDamage=1         a pistol hit takes GTA's 25 of a ped's 100 health as that share of the
//                       target's health, times this
//   aimDistance=150     camera distance (cm) while aiming
//   aimSide=40          ... how far it moves to the right of CJ (cm)
//   aimHeight=75        ... height of the point it looks at (cm, like camHeight)
//   startPistol=1       give CJ the pistol and 51 rounds if he has no pistol (on a new game or load)
//   startRocket=1       give CJ the rocket launcher and 10 rockets if he has none (on a new game or load)
//   rocketRadius=500    how far (cm) a rocket's blast hurts (GTA's EXPLOSION_ROCKET: 1000); it kills
//                       within some 2/5 of that, behind cover half as much
//   startDildo=1        give CJ the dildo if he has none (on a new game or load)
//   fleeRadius=2000     how near (cm) an enemy must be to run from CJ's drawn dildo
//   slopeWalk=60        steepest ground (degrees) CJ walks on without Gothic sliding him down
//   slopeSlide=70       ... and where Gothic's steeper slide starts
//   maxFrameTime=40     with GTA controls on, the longest step (ms) one frame may take (Gothic: 100);
//                       on a stutter the game slows down instead of CJ leaping off a steep slope
//   radar=1             draw GTA's radar (with gtaHud)
//   radarRange=18000    world distance (cm) from the radar's centre to its rim (GTA on foot: 180 m)
//   radarX0, radarZ0, radarX1, radarZ1   the world rectangle (cm) of the radar map, written by
//                       make_cj_radar.py
//   bike=1              put GTA's motorbike next to CJ (make_cj_bike.py)
//   bikeCamDistance=450 the camera's distance behind the bike (cm)
//   bikeGrip=1          the tyres' grip: 1 = GTA's (fTractionMultiplier on GTA's surface grip)
//   bikeSpeed=0.6       the bike's top speed and pull as a share of GTA's (as runSpeed for running)
//   wastedCam=1         San Andreas's camera when CJ dies
//   wastedHeight=1500   how high (cm above CJ) it rises before it stops (GTA fades out instead)
//   gtaClock=1          Gothic's world time runs at GTA's pace: an hour a minute instead of an hour
//                       every 250 s (the sky and the NPCs' daily routines run faster with it)
//   lightFlashes=1      shots and explosions light up their surroundings for a moment (GTA's
//                       CPointLights gun flash and explosion light)
//   bikeLight=1         the bike's headlight lights the way at night while CJ rides it
//   crashDamage=1       CJ is hurt when a crash throws him off the bike (0 = never)
//
// Fists: CJ always has GTA's three-punch combo (Gothic would limit it by fighting talent).
//   debugLog=1          write jump, sprint and weapon events to CJMovement.log (Gothic folder)

namespace GOTHIC_NAMESPACE
{
	namespace CJ
	{
		struct Settings
		{
			float turnRate = 450.0f;
			float mouseSpeed = 0.25f;
			bool invertMouseY = false;
			float camDistance = 300.0f;
			float camHeight = 50.0f;
			float camSmooth = 0.08f;
			bool gtaHud = true;
			float fistDamage = 1.0f;
			float runSpeed = 0.75f;
			float sprintSpeed = 0.7f;
			float sprintAccel = 0.8f;
			float sprintDecel = 0.5f;
			float gunDamage = 1.0f;
			float aimDistance = 150.0f;
			float aimSide = 40.0f;
			float aimHeight = 75.0f;
			bool startPistol = true;
			bool startRocket = true;
			float rocketRadius = 500.0f;
			bool startDildo = true;
			float fleeRadius = 2000.0f;
			float slopeWalk = 60.0f;
			float slopeSlide = 70.0f;
			float maxFrameTime = 40.0f;
			bool radar = true;
			float radarRange = 18000.0f;
			float radarX0 = 0.0f, radarZ0 = 0.0f, radarX1 = 0.0f, radarZ1 = 0.0f;
			bool gtaClock = true;
			bool wastedCam = true;
			float wastedHeight = 1500.0f;
			bool bike = true;
			float bikeCamDistance = 450.0f;
			float bikeGrip = 1.0f;
			float bikeSpeed = 0.6f;
			bool lightFlashes = true;
			bool bikeLight = true;
			float crashDamage = 1.0f;
			bool debugLog = true;
		};

		static Settings settings;
		static bool settingsLoaded = false;

		static bool overrideInput = false; // true while the hero's AI reads its controls in GTA mode
		static bool heroAI = false;        // true while the hero's AI runs at all
		static bool moving = false;        // W/A/S/D held this frame
		static bool punch = false, punchPressed = false; // left mouse button with fists up (or the dildo out)
		static bool block = false, blockPressed = false; // right mouse button with fists up (or the dildo out)
		static float comboTime = 0.0f;     // seconds Gothic's attack key stays held after a click
		static float punchPulse = 0.0f;    // seconds "forward" is reported for a click
		static bool punchToggle = false;   // this frame Gothic should see a fresh press
		// Gothic chains a punch when the forward key (W) itself is tapped during the punch's combo
		// window, so a click during a punch becomes a short virtual W tap timed into the next
		// window. Windows of s_FistAttack in seconds, from make_cj_model.py's DEF_WINDOW tags
		// (frames 4-11, 19-31, 40-54 at 25 fps).
		static const float COMBO_WINDOWS[3][2] = { { 0.16f, 0.44f }, { 0.76f, 1.24f }, { 1.60f, 2.16f } };
		static float attackStart = -1.0f;  // game time the current attack began
		static float tapFrom = -1.0f, tapUntil = -1.0f;
		static bool virtualW = false, virtualWEdge = false;
		static bool meleeAttack = false;   // CJ is throwing a punch, or swinging the dildo
		static float jumpKeyTime = -1.0f;  // when Left Shift was last pressed (s, game time)
		static bool jumpKeyHeld = false, jumpKeyPressed = false;
		static bool gtaControls = false;   // GTA controls active (last hero AI frame)
		static bool useKey = false, useKeyPressed = false;     // F
		static float useUntil = -1.0f;     // F's interaction window (s, game time)
		static bool useHeld = false, useForward = false;       // what F gives Gothic this frame
		static bool walkKey = false, walkKeyPressed = false;   // Left Alt
		static bool sneakKey = false, sneakKeyPressed = false; // C
		static const float FIST_IDLE = 3.0f; // seconds without fighting before the fists come down
		static float lastFight = -1.0f;    // last click or punch with fists up (s, game time)
		static float fistsLowered = -1.0f; // when CJ was last told to lower his fists
		static zCModelPrototype* sprintProto = nullptr; // CJ_SPRINT.MDS, loaded once and kept
		static zCModelPrototype* sprintBase = nullptr;  // the CJ.MDS prototype it was loaded for
		static zCModel* sprintModel = nullptr;          // the hero model the sprint state belongs to
		static float sprintBlend = 0.0f;                // 0 running .. 1 full sprint
		static int lastState = -1;
		static bool jumping = false, launched = false;
		static float jumpStart = 0.0f, launchTime = 0.0f;
		static zVEC3 jumpVelocity;
		static zVEC3 jumpLastPos, jumpLastVel;          // where CJ was and the velocity he was given last frame
		static int jumpFrames = 0, jumpBlocked = 0;     // frames of flight, of them in a row against a wall
		static int jumpCeiling = 0;                     // ... and in a row against a ceiling
		static bool sprintLock = false;                 // Space still held from the bike's handbrake
		static bool cameraActive = false;
		static zVEC3 cameraForward;        // horizontal, unit length
		static float cameraPitch = 20.0f;  // degrees, positive looks down on CJ
		static zVEC3 cameraFocus;          // smoothed point the camera looks at
		static float mouseWheel = 0.0f;    // this frame's wheel movement (read with the camera)
		static float recentFrame = 0.0f;   // the longest frame (ms, before maxFrameTime) of the last second
		static float recentFrameAt = -1.0f;

		// Pistol
		static const int CLIP = 17;                       // weapon.dat PISTOL ammoClip
		static const float SHOT_INTERVAL = 11.0f / 30.0f; // colt45_fire loop, frames 6-17 at 30 fps
		static const float RELOAD_TIME = 0.933f;          // colt45_reload
		static const float GUN_RANGE = 3500.0f;           // weapon.dat weaponRange 35 m
		static const float GUN_DAMAGE = 25.0f;            // weapon.dat damage, of a ped's 100 health
		static const float AIM_YAW = 60.0f, AIM_PITCH = 55.0f; // make_cj_model.py's pose grid
		static const float AIM_GAIN = 0.5f;               // share of the barrel's error corrected per frame
		static float aimX = 0.5f, aimY = 0.5f;            // the pose blend (combine X/Y)
		static const float CROSSHAIR_X = 0.53f, CROSSHAIR_Y = 0.4f; // share of the screen
		// The guns (make_cj_pistol.py), Gothic's crossbows, each with its ammunition and clip.
		enum { GUN_NONE = -1, GUN_PISTOL, GUN_ROCKET, GUNS };
		static const char* const GUN_ITEMS[GUNS] = { "ITRW_CROSSBOW_01", "ITRW_CROSSBOW_02" };
		static const char* const GUN_AMMO[GUNS] = { "ITAMBOLT", "ITMI_ALCHEMY_SYRIANOIL_01" };
		static const int GUN_CLIP[GUNS] = { CLIP, 1 };    // weapon.dat ammoClip of PISTOL and RLAUNCHER
		static const char* const GUN_NAMES[GUNS] = { "pistol", "rocket launcher" };
		static int gunIndex[GUNS] = { -1, -1 }, gunAmmo[GUNS] = { -1, -1 }; // their script instances
		static int clips[GUNS] = { -1, -1 }; // rounds in each gun, -1: fill it from the ammunition
		static bool gunsGiven = false;       // the start weapons checked for this game (NewGame)
		static int pendingGun = -2;          // the gun to draw once the weapon in hand is away (-1 none)
		static float pendingSince = 0.0f;
		static bool switchKeyHeld = false;
		// The purple dildo (make_cj_pistol.py turns Jackal's unused sword into it): drawn in Gothic's
		// one-handed mode, swung with the fists' click controls. In the weapon cycle it comes after
		// the guns (GTA's slot 10, the gifts).
		static const int WEAPON_DILDO = GUNS;
		static const char* const DILDO_ITEM = "JACKALSSWORD";
		static int dildoIndex = -1;
		// s_1hAttack's combo windows (make_cj_model.py's DEF_WINDOW tags: frames 9-24, 37-53, 65-83 at 25 fps)
		static const float DILDO_WINDOWS[3][2] = { { 0.36f, 0.96f }, { 1.48f, 2.12f }, { 2.60f, 3.32f } };
		static const float DILDO_DAMAGE[3] = { 18.0f, 18.0f, 25.0f }; // melee.dat DILDO combo, of a ped's 100 health
		static const float DILDO_MOVING_DAMAGE = 10.0f;               // ... its moving attack
		static zCSoundFX* dildoSound = nullptr;
		// Rocket launcher: weapon.dat RLAUNCHER (and ROCKET for the missile), CExplosion's EXPLOSION_ROCKET
		static const float ROCKET_INTERVAL = 0.97f;       // rocket.ifp RocketFire: the shot and the reload
		static const float ROCKET_SPEED = 5000.0f;        // cm/s: a GTA rocket covers some 50 m a second
		static const float ROCKET_LIFE = 6.0f;            // seconds before it blows up in the air
		static oCNpc* blastTarget = nullptr;              // the one being hurt by a blast (OnDamage_Hit hook)
		static float blastDamage = 0.0f;
		static zVEC3 blastDir;
		static bool blastLethal = false;                  // a rocket's blast kills; a bike's knock knocks out
		static float shake = 0.0f;                        // camera shake from explosions, 0..1
		static bool heavy = false;                        // the rocket launcher is out: no sprint, no jump
		static float fpBlend = 0.0f;                      // 0 third person .. 1 the launcher's first-person aim
		static bool aiming = false;
		static float aimBlend = 0.0f;        // 0 follow camera .. 1 aiming camera
		static float aimHoldUntil = -1.0f;   // a click without the right button aims this long
		static float nextShot = 0.0f, reloadUntil = -1.0f;
		static float lastShot = -1.0f;       // when the last round was fired (s, game time)
		static zCVisual* flashVisual = nullptr; // CJ_GUNFLASH.3DS, loaded once and kept
		static bool aimUp = false, aimDown = false, aimLeft = false, aimRight = false; // W/S/A/D while aiming
		static zVEC3 aimOrigin, aimDir;      // the crosshair's ray, from the camera
		static bool aimRayValid = false;
		static oCNpc* gunTarget = nullptr;   // NPC being shot (OnDamage_Hit hook)

		static const char* const SPRINT_OVERLAY = "CJ_SPRINT.MDS";
		static const char* const RUN_CYCLES[3] = { "S_RUNL", "S_CBOWRUNL", "S_1HRUNL" }; // what CJ_SPRINT.MDS replaces
		static const float PI = 3.14159265f;

		static void LoadSettings()
		{
			if (settingsLoaded || !zoptions)
				return;
			settingsLoaded = true;
			zSTRING section = "CJMOVEMENT";
			settings.turnRate = zoptions->ReadReal(section, "turnRate", settings.turnRate);
			settings.mouseSpeed = zoptions->ReadReal(section, "mouseSpeed", settings.mouseSpeed);
			settings.invertMouseY = zoptions->ReadBool(section, "invertMouseY", 0) != 0;
			settings.camDistance = zoptions->ReadReal(section, "camDistance", settings.camDistance);
			settings.camHeight = zoptions->ReadReal(section, "camHeight", settings.camHeight);
			settings.camSmooth = zoptions->ReadReal(section, "camSmooth", settings.camSmooth);
			settings.gtaHud = zoptions->ReadBool(section, "gtaHud", 1) != 0;
			settings.fistDamage = zoptions->ReadReal(section, "fistDamage", settings.fistDamage);
			settings.runSpeed = zoptions->ReadReal(section, "runSpeed", settings.runSpeed);
			settings.sprintSpeed = zoptions->ReadReal(section, "sprintSpeed", settings.sprintSpeed);
			settings.sprintAccel = zoptions->ReadReal(section, "sprintAccel", settings.sprintAccel);
			settings.sprintDecel = zoptions->ReadReal(section, "sprintDecel", settings.sprintDecel);
			settings.gunDamage = zoptions->ReadReal(section, "gunDamage", settings.gunDamage);
			settings.aimDistance = zoptions->ReadReal(section, "aimDistance", settings.aimDistance);
			settings.aimSide = zoptions->ReadReal(section, "aimSide", settings.aimSide);
			settings.aimHeight = zoptions->ReadReal(section, "aimHeight", settings.aimHeight);
			settings.startPistol = zoptions->ReadBool(section, "startPistol", 1) != 0;
			settings.startRocket = zoptions->ReadBool(section, "startRocket", 1) != 0;
			settings.rocketRadius = zoptions->ReadReal(section, "rocketRadius", settings.rocketRadius);
			settings.startDildo = zoptions->ReadBool(section, "startDildo", 1) != 0;
			settings.fleeRadius = zoptions->ReadReal(section, "fleeRadius", settings.fleeRadius);
			settings.slopeWalk = zoptions->ReadReal(section, "slopeWalk", settings.slopeWalk);
			settings.slopeSlide = zoptions->ReadReal(section, "slopeSlide", settings.slopeSlide);
			settings.maxFrameTime = zoptions->ReadReal(section, "maxFrameTime", settings.maxFrameTime);
			settings.radar = zoptions->ReadBool(section, "radar", 1) != 0;
			settings.radarRange = zoptions->ReadReal(section, "radarRange", settings.radarRange);
			settings.radarX0 = zoptions->ReadReal(section, "radarX0", settings.radarX0);
			settings.radarZ0 = zoptions->ReadReal(section, "radarZ0", settings.radarZ0);
			settings.radarX1 = zoptions->ReadReal(section, "radarX1", settings.radarX1);
			settings.radarZ1 = zoptions->ReadReal(section, "radarZ1", settings.radarZ1);
			settings.gtaClock = zoptions->ReadBool(section, "gtaClock", 1) != 0;
			settings.wastedCam = zoptions->ReadBool(section, "wastedCam", 1) != 0;
			settings.wastedHeight = zoptions->ReadReal(section, "wastedHeight", settings.wastedHeight);
			settings.bike = zoptions->ReadBool(section, "bike", 1) != 0;
			settings.bikeCamDistance = zoptions->ReadReal(section, "bikeCamDistance", settings.bikeCamDistance);
			settings.bikeGrip = zoptions->ReadReal(section, "bikeGrip", settings.bikeGrip);
			settings.bikeSpeed = zoptions->ReadReal(section, "bikeSpeed", settings.bikeSpeed);
			settings.lightFlashes = zoptions->ReadBool(section, "lightFlashes", 1) != 0;
			settings.bikeLight = zoptions->ReadBool(section, "bikeLight", 1) != 0;
			settings.crashDamage = zoptions->ReadReal(section, "crashDamage", settings.crashDamage);
			settings.debugLog = zoptions->ReadBool(section, "debugLog", 1) != 0;
		}

		static void Log(const char* format, ...)
		{
			if (!settings.debugLog)
				return;
			FILE* file = nullptr;
			if (fopen_s(&file, "CJMovement.log", "a") || !file)
				return;
			va_list args;
			va_start(args, format);
			fprintf(file, "%8.2f  ", ztimer ? ztimer->totalTimeFloat / 1000.0f : 0.0f);
			vfprintf(file, format, args);
			fputc('\n', file);
			va_end(args);
			fclose(file);
		}

		static zVEC3 Cross(const zVEC3& a, const zVEC3& b)
		{
			return zVEC3(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
		}

		static zVEC3 Flat(const zVEC3& v)
		{
			zVEC3 out(v[0], 0.0f, v[2]);
			float len = out.Length();
			return len > 0.0001f ? out * (1.0f / len) : zVEC3(0.0f, 0.0f, 1.0f);
		}

		// Horizontal right-hand side of a horizontal forward vector, in the engine's own
		// handedness (taken from the hero's at/right vectors, so no convention is assumed).
		static zVEC3 RightOf(const zVEC3& forward, oCNpc* hero)
		{
			zVEC3 up(0.0f, 1.0f, 0.0f);
			zVEC3 cross = Cross(forward, up);
			zVEC3 heroCross = Cross(Flat(hero->GetAtVectorWorld()), up);
			float sign = heroCross.Dot(Flat(hero->GetRightVectorWorld())) >= 0.0f ? 1.0f : -1.0f;
			return Flat(cross * sign);
		}

		static bool KeyDown(int key)
		{
			return zinput && zinput->KeyPressed(key) != 0;
		}

		// Gothic's own button query can stay 0 while it holds the mouse through DirectInput, so
		// the Windows button state counts too (only while Gothic is the active window).
		static bool GameHasFocus()
		{
			DWORD process = 0;
			HWND window = GetForegroundWindow();
			return window && GetWindowThreadProcessId(window, &process) && process == GetCurrentProcessId();
		}

		static bool MouseDown(bool left, const char** source = nullptr)
		{
			if ((GetAsyncKeyState(left ? VK_LBUTTON : VK_RBUTTON) & 0x8000) && GameHasFocus())
			{
				if (source)
					*source = "Windows";
				return true;
			}
			return false;
		}

		static zSTRING CameraMode()
		{
			zCAICamera* ai = zCAICamera::current;
			zSTRING mode = ai ? ai->curcammode : zSTRING("(no camera)");
			mode.Upper();
			return mode;
		}

		static int BodyState(oCNpc* hero)
		{
			return hero->GetBodyState() & BS_ONLY_STATE;
		}

		static bool CameraModeAllowed()
		{
			zCAICamera* ai = zCAICamera::current;
			if (!ai || !ai->camVob)
				return false;
			zSTRING mode = ai->curcammode;
			mode.Upper();
			return mode == "CAMMODNORMAL" || mode == "CAMMODRUN" || mode == "CAMMODJUMP" || mode == "CAMMODJUMPUP"
				|| mode == "CAMMODFALL" || mode == "CAMMODSWIM" || mode == "CAMMODDIVE" || mode == "CAMMODLOOKBACK"
				|| mode == "CAMMODMELEE" || mode == "CAMMODRANGED" || mode == "CAMMODRANGEDSHORT";
		}

		// The guns', their ammunition's and the dildo's script instances (make_cj_pistol.py).
		static void FindGuns()
		{
			if (gunIndex[0] < 0 && parser)
			{
				for (int i = 0; i < GUNS; i++)
				{
					gunIndex[i] = parser->GetIndex(GUN_ITEMS[i]);
					gunAmmo[i] = parser->GetIndex(GUN_AMMO[i]);
				}
				dildoIndex = parser->GetIndex(DILDO_ITEM);
			}
		}

		static oCItem* GunItem(oCNpc* hero, int gun)
		{
			FindGuns();
			return gun >= 0 && gunIndex[gun] >= 0 ? hero->IsInInv(gunIndex[gun], 1) : nullptr;
		}

		static int AmmoCount(oCNpc* hero, int gun)
		{
			FindGuns();
			return gun >= 0 && gunAmmo[gun] >= 0 ? hero->inventory2.GetAmount(gunAmmo[gun]) : 0;
		}

		// Which gun CJ has out (Gothic's crossbow mode with that crossbow), GUN_NONE for none.
		static int DrawnGun(oCNpc* hero)
		{
			if (hero->GetWeaponMode() != NPC_WEAPON_CBOW)
				return GUN_NONE;
			oCItem* ranged = hero->GetWeapon();
			if (!ranged)
				ranged = hero->GetEquippedRangedWeapon();
			FindGuns();
			for (int i = 0; i < GUNS; i++)
				if (ranged && ranged->GetInstance() == gunIndex[i])
					return i;
			return GUN_NONE;
		}

		static bool PistolDrawn(oCNpc* hero) { return DrawnGun(hero) == GUN_PISTOL; }
		static bool GunDrawn(oCNpc* hero) { return DrawnGun(hero) != GUN_NONE; }

		static oCItem* DildoItem(oCNpc* hero)
		{
			FindGuns();
			return dildoIndex >= 0 ? hero->IsInInv(dildoIndex, 1) : nullptr;
		}

		// Whether CJ has the dildo out (Gothic's one-handed mode with it in his hand).
		static bool DildoDrawn(oCNpc* hero)
		{
			if (hero->GetWeaponMode() != NPC_WEAPON_1HS)
				return false;
			FindGuns();
			oCItem* weapon = hero->GetWeapon();
			if (!weapon)
				weapon = hero->GetEquippedMeleeWeapon();
			return weapon && dildoIndex >= 0 && weapon->GetInstance() == dildoIndex;
		}

		// The weapon CJ has out in GTA's cycle: GUN_NONE (fists or nothing), a gun or WEAPON_DILDO.
		static int CurrentWeapon(oCNpc* hero)
		{
			return DildoDrawn(hero) ? WEAPON_DILDO : DrawnGun(hero);
		}

		static const char* WeaponName(int weapon)
		{
			return weapon == WEAPON_DILDO ? "dildo" : (weapon >= 0 ? GUN_NAMES[weapon] : "unarmed");
		}

		// The GTA camera follows CJ on foot, with fists up, a gun or the dildo out; dialogue,
		// inventory, interaction and death cameras stay Gothic's.
		static bool CameraActive(oCNpc* hero)
		{
			return hero && !hero->IsDead() && !hero->IsUnconscious() && !hero->GetInteractMob()
				&& (hero->GetWeaponMode() <= NPC_WEAPON_FIST || GunDrawn(hero) || DildoDrawn(hero)) && CameraModeAllowed();
		}

		// GTA controls: as the camera, while CJ stands, moves, swims, jumps or fights with fists.
		static bool ControlsActive(oCNpc* hero)
		{
			if (!CameraActive(hero))
				return false;
			switch (BodyState(hero))
			{
			case BS_STAND: case BS_WALK: case BS_SNEAK: case BS_RUN: case BS_SPRINT: case BS_SWIM:
			case BS_DIVE: case BS_JUMP: case BS_FALL: case BS_HIT: case BS_PARADE:
				return true;
			default:
				return false;
			}
		}

		// Sprint as in GTA, where holding sprint blends run_player into sprint_civi and CJ gathers
		// speed: sprintBlend ramps up over sprintAccel seconds (down over sprintDecel) and the two
		// cycles cross-fade in step (both start on the left foot). The sprint cycle is the s_RunL
		// (and s_CBowRunL, with the pistol out) of the CJ_SPRINT.MDS overlay. Gothic's overlay
		// switch (zCModel::Apply/RemoveModelProtoOverlay) stops every animation the model plays,
		// which made CJ snap into the sprint, so the overlay goes on and off the model's prototype
		// list directly here.
		// Gothic moves CJ by the animations' root motion; both cycles play at the rate that gives
		// the wanted speed, so his feet stay planted.
		static zCModelAni* ProtoAni(zCModelPrototype* proto, int id)
		{
			return proto && id >= 0 && id < proto->protoAnis.GetNum() ? proto->protoAnis[id] : nullptr;
		}

		static bool SprintApplied(zCModel* model)
		{
			return sprintProto && model->modelProtoList.IsInList(sprintProto);
		}

		// What Apply/RemoveModelProtoOverlay do apart from stopping the animations.
		static void SetSprintOverlay(zCModel* model, bool on)
		{
			if (on == SprintApplied(model))
				return;
			if (on)
			{
				model->modelProtoList.InsertEnd(sprintProto);
				sprintProto->refCtr++;
			}
			else
			{
				model->modelProtoList.RemoveOrder(sprintProto);
				sprintProto->Release();
			}
		}

		// The overlay is loaded once for CJ's model and kept (not for other models the hero may
		// get, such as an animal after a transformation spell).
		static bool LoadSprint(zCModel* model)
		{
			zCModelPrototype* base = model->modelProtoList.GetNum() > 0 ? model->modelProtoList[0] : nullptr;
			if (!base)
				return false;
			if (base == sprintBase)
				return sprintProto != nullptr;
			zSTRING name = base->modelProtoName;
			name.Upper();
			if (!(name == "CJ"))
				return false;
			sprintBase = base;
			sprintProto = zCModelPrototype::Load(zSTRING(SPRINT_OVERLAY), base);
			Log("sprint: %s %s", SPRINT_OVERLAY, sprintProto ? "loaded" : "failed to load");
			return sprintProto != nullptr;
		}

		// Earlier versions applied the overlay through Gothic, which keeps it in savegames.
		static void ClearSavedSprintOverlay(oCNpc* hero)
		{
			for (int i = 0; i < hero->activeOverlays.GetNum(); i++)
			{
				zSTRING overlay = hero->activeOverlays[i];
				overlay.Upper();
				if (overlay == SPRINT_OVERLAY)
				{
					Log("sprint: removing %s saved on the hero", SPRINT_OVERLAY);
					hero->RemoveOverlay(zSTRING(SPRINT_OVERLAY));
					return;
				}
			}
		}

		// Swaps the playing cycle for `to` (the other one of s_RunL) with a cross-fade over
		// `seconds`, at the same point of the stride. Nothing to do if CJ is not in the cycle:
		// Gothic starts the right one when he gets there.
		static void CrossFade(zCModel* model, zCModelAni* to, float seconds)
		{
			zCModelAniActive* from = nullptr;
			for (int i = 0; i < model->numActiveAnis; i++)
			{
				zCModelAniActive* active = model->aniChannels[i];
				if (active->protoAni != to && active->protoAni->aniID == to->aniID && !active->isFadingOut)
					from = active;
			}
			if (!from)
				return;
			seconds = seconds > 0.1f ? seconds : 0.1f;
			float progress = from->GetProgressPercent();
			from->blendOutOverride = -1.0f / seconds; // StartAni fades `from` out at this rate
			float blendIn = to->blendInSpeed;         // and `to` in at the prototype's rate
			to->blendInSpeed = 1.0f / seconds;
			model->StartAni(to, zCModel::zMDL_STARTANI_DEFAULT);
			to->blendInSpeed = blendIn;
			zCModelAniActive* started = model->GetActiveAni(to);
			if (!started)
				return;
			started->SetProgressPercent(progress);
			if (to->rootNodeIndex >= 0) // StartAni took the root position at frame 0; no jump in root motion
				started->lastPos = started->thisPos = to->GetTrans((int)started->actFrame, to->rootNodeIndex);
		}

		// Plays both cycles at one cadence (so their feet stay in step through the cross-fade) that
		// moves CJ at the speed between running and sprinting that sprintBlend asks for.
		static void UpdateSprintSpeed(zCModel* model, zCModelAni* run, zCModelAni* sprint)
		{
			// root travel per second and per loop of each cycle at its own frame rate
			float runVelocity = run->fpsRate > 0.0f ? run->GetAniVelocity() * run->fpsRateSource / run->fpsRate : 0.0f;
			float sprintVelocity = sprint->fpsRate > 0.0f ? sprint->GetAniVelocity() * sprint->fpsRateSource / sprint->fpsRate : 0.0f;
			if (runVelocity <= 0.0f || sprintVelocity <= 0.0f || run->numFrames <= 0 || sprint->numFrames <= 0)
				return;
			float runStride = runVelocity * run->numFrames / run->fpsRateSource;
			float sprintStride = sprintVelocity * sprint->numFrames / sprint->fpsRateSource;

			// share of the sprint cycle in CJ's root motion
			float runWeight = 0.0f, sprintWeight = 0.0f;
			zCModelNodeInst* root = model->nodeList.GetNum() > 0 ? model->nodeList[0] : nullptr;
			for (int i = 0; root && i < root->numNodeAnis; i++)
			{
				zCModelAniActive* active = root->nodeAniList[i].modelAni;
				if (active && active->protoAni == run)
					runWeight += root->nodeAniList[i].weight;
				else if (active && active->protoAni == sprint)
					sprintWeight += root->nodeAniList[i].weight;
			}
			float share = runWeight + sprintWeight > 0.001f ? sprintWeight / (runWeight + sprintWeight) : (SprintApplied(model) ? 1.0f : 0.0f);

			float blend = sprintBlend * sprintBlend * (3.0f - 2.0f * sprintBlend); // eases in and out
			float speed = runVelocity * settings.runSpeed + (sprintVelocity * settings.sprintSpeed - runVelocity * settings.runSpeed) * blend;
			float cadence = speed / (runStride + (sprintStride - runStride) * share); // loops per second
			run->fpsRate = cadence * run->numFrames > 1.0f ? cadence * run->numFrames : 1.0f;
			sprint->fpsRate = cadence * sprint->numFrames > 1.0f ? cadence * sprint->numFrames : 1.0f;
		}

		// want: Space held while CJ runs. In the air CJ keeps the speed he jumped with.
		static void UpdateSprint(oCNpc* hero, bool want, bool moving, bool airborne)
		{
			zCModel* model = hero->GetModel();
			if (!model || !LoadSprint(model))
				return;
			if (model != sprintModel) // new hero model (game loaded, level change)
			{
				sprintModel = model;
				sprintBlend = 0.0f;
				ClearSavedSprintOverlay(hero);
			}
			// the run cycles the overlay replaces: unarmed (and fists), with a gun and with the dildo out
			zCModelAni* runs[3], * sprints[3];
			for (int i = 0; i < 3; i++)
			{
				int id = model->GetAniIDFromAniName(RUN_CYCLES[i]);
				runs[i] = ProtoAni(model->modelProtoList[0], id);
				sprints[i] = ProtoAni(sprintProto, id);
			}
			if (!airborne)
			{
				bool applied = SprintApplied(model);
				if (want && !applied)
				{
					SetSprintOverlay(model, true);
					for (zCModelAni* sprint : sprints)
						if (sprint)
							CrossFade(model, sprint, settings.sprintAccel * (1.0f - sprintBlend));
					Log("sprint on (from %.2f)", sprintBlend);
				}
				else if (!want && applied)
				{
					SetSprintOverlay(model, false);
					for (zCModelAni* run : runs)
						if (run)
							CrossFade(model, run, settings.sprintDecel * sprintBlend);
					Log("sprint off (from %.2f)", sprintBlend);
				}
				float dt = ztimer->frameTimeFloat / 1000.0f;
				if (!moving)
					sprintBlend = 0.0f; // stopping loses the momentum
				else if (want)
					sprintBlend += dt / (settings.sprintAccel > 0.01f ? settings.sprintAccel : 0.01f);
				else
					sprintBlend -= dt / (settings.sprintDecel > 0.01f ? settings.sprintDecel : 0.01f);
				sprintBlend = sprintBlend < 0.0f ? 0.0f : (sprintBlend > 1.0f ? 1.0f : sprintBlend);
			}
			for (int i = 0; i < 3; i++)
				if (runs[i] && sprints[i])
					UpdateSprintSpeed(model, runs[i], sprints[i]);
		}

		// Back to running at once, e.g. when a dialogue takes over.
		static void StopSprint(oCNpc* hero)
		{
			sprintBlend = 0.0f;
			zCModel* model = hero->GetModel();
			if (!model || model != sprintModel || !sprintProto)
				return;
			if (SprintApplied(model))
			{
				SetSprintOverlay(model, false);
				Log("sprint off (controls inactive)");
			}
			for (const char* name : RUN_CYCLES)
				if (zCModelAni* run = ProtoAni(model->modelProtoList[0], model->GetAniIDFromAniName(name)))
					run->fpsRate = run->fpsRateSource * settings.runSpeed;
		}

		// CTaskSimpleJump: the launch animation (JUMP_launch / JUMP_launch_R, 0.2 s) plays on the
		// ground, then Launch() gives the jump speed and gravity (0.008 m per 1/50 s tick squared)
		// takes over. GTA speeds are metres per tick: x 5000 gives cm/s, x 250000 cm/s^2.
		static const float LAUNCH_DELAY = 0.2f, GTA_GRAVITY = 0.008f * 250000.0f;

		static float Now() { return ztimer->totalTimeFloat / 1000.0f; }

		static void StartJump(oCNpc* hero)
		{
			float forward = 0.10f;               // standing or walking: no run animation playing
			if (moving && !KeyDown(KEY_LALT))    // lerp(0.17, 0.22) by how far the sprint is blended in
				forward = 0.17f * settings.runSpeed + (0.22f * settings.sprintSpeed - 0.17f * settings.runSpeed) * sprintBlend;
			jumpVelocity = Flat(hero->GetAtVectorWorld()) * (forward * 5000.0f);
			jumpVelocity[1] = 8.5f / 70.0f * 5000.0f; // ApplyMoveForce(0, 0, 8.5) on CJ's 70 kg
			jumping = true;
			launched = false;
			jumpStart = Now();
			Log("jump start: forward %.0f cm/s, up %.0f cm/s, physics %d", forward * 5000.0f, jumpVelocity[1], (int)hero->physicsEnabled);
		}

		// Gothic starts the jump on Left Shift (its jump key). After GTA's launch delay CJ flies
		// GTA's arc: constant forward speed, launch speed up, GTA gravity, set every frame (so
		// Gothic's own gravity does not add up) until Gothic reports the landing.
		static void UpdateJump(oCNpc* hero)
		{
			int state = BodyState(hero);
			bool airborne = state == BS_JUMP || state == BS_FALL;
			bool wasGrounded = lastState == BS_STAND || lastState == BS_WALK || lastState == BS_RUN
				|| lastState == BS_SPRINT || lastState == BS_SNEAK;
			if (state != lastState)
				Log("body state %d -> %d", lastState, state);
			if (state == BS_FALL && lastState != BS_FALL && hero->human_ai) // why Gothic lets CJ fall
			{
				float floorY = 0.0f, lastFloorY = 0.0f, slope = -1.0f;
				if (zCCollisionObject* collision = hero->GetCollisionObject())
				{
					zCCollObjectCharacter::zTSpatialState& spatial = static_cast<zCCollObjectCharacter*>(collision)->m_oSpatialState;
					floorY = spatial.m_fFloorY;
					lastFloorY = spatial.m_fLastFloorY;
					if (spatial.m_poFloorPoly)
					{
						float up = spatial.m_poFloorPoly->polyPlane.normal[1];
						slope = acosf(up > 1.0f ? 1.0f : (up < -1.0f ? -1.0f : up)) * 180.0f / PI;
					}
				}
				zVEC3 at = hero->GetPositionWorld();
				Log("fall: %.0f cm above the floor (Gothic falls from %.0f), physics %d, at %.0f %.0f %.0f, floor %.0f (before %.0f), "
					"slope %.0f deg, longest frame %.0f ms, weapon mode %d", hero->human_ai->aboveFloor, hero->human_ai->config.zMV_STEP_HEIGHT,
					(int)hero->physicsEnabled, at[0], at[1], at[2], floorY, lastFloorY, slope, recentFrame, hero->GetWeaponMode());
			}
			float now = Now();
			bool asked = jumpKeyTime >= 0.0f && now - jumpKeyTime < 0.5f;
			if (!jumping && state == BS_JUMP && wasGrounded && CameraActive(hero) && !asked)
				Log("jump: Gothic's own (no Left Shift), left as it is");
			if (!jumping && state == BS_JUMP && wasGrounded && CameraActive(hero) && asked)
			{
				jumpKeyTime = -1.0f;
				oCAniCtrl_Human* ani = hero->GetAnictrl();
				if (ani && ani->CanJumpLedge())
					Log("jump: ledge in front, Gothic climbs");
				else
					StartJump(hero);
			}
			lastState = state;
			if (!jumping)
				return;

			zCRigidBody* body = hero->GetRigidBody();
			if (!body || !airborne || now - jumpStart > 3.0f)
			{
				Log("jump end: state %d, launched %d, in the air %.2f s", state, (int)launched, launched ? now - launchTime : 0.0f);
				jumping = false;
				return;
			}
			if (!launched)
			{
				if (now - jumpStart < LAUNCH_DELAY)
					return;
				if (!hero->physicsEnabled)
				{
					hero->SetPhysicsEnabled(TRUE); // Gothic keeps a jump animation-driven; GTA's flight needs physics
					Log("jump: enabled physics");
				}
				launched = true;
				launchTime = now;
				jumpFrames = jumpBlocked = jumpCeiling = 0;
				Log("jump launch: velocity %.0f %.0f %.0f", jumpVelocity[0], jumpVelocity[1], jumpVelocity[2]);
			}
			// GTA's ped stops against what it flies into (CPhysical's collision response takes the
			// speed into the wall away). Gothic's collision keeps CJ out of the wall but the velocity set
			// here every frame would keep pushing him into it: a wall that held him back two frames in a
			// row takes his forward speed, a ceiling his speed up (two frames: one long frame alone does
			// not look like a wall).
			zVEC3 pos = hero->GetPositionWorld();
			float dt = ztimer->frameTimeFloat / 1000.0f;
			if (jumpFrames >= 2 && dt > 0.0f)
			{
				zVEC3 moved = pos - jumpLastPos;
				float want = sqrtf(jumpLastVel[0] * jumpLastVel[0] + jumpLastVel[2] * jumpLastVel[2]) * dt;
				float got = sqrtf(moved[0] * moved[0] + moved[2] * moved[2]);
				jumpBlocked = want > 2.0f && got < want * 0.3f ? jumpBlocked + 1 : 0;
				if (jumpBlocked >= 2 && (jumpVelocity[0] != 0.0f || jumpVelocity[2] != 0.0f))
				{
					jumpVelocity[0] = jumpVelocity[2] = 0.0f;
					Log("jump: against a wall, forward speed gone");
				}
				jumpCeiling = jumpLastVel[1] * dt > 2.0f && moved[1] < jumpLastVel[1] * dt * 0.3f ? jumpCeiling + 1 : 0;
				if (jumpCeiling >= 2)
				{
					jumpVelocity[1] = GTA_GRAVITY * (now - launchTime); // up speed now 0: he falls from here
					jumpCeiling = 0;
					Log("jump: head against the ceiling");
				}
			}
			zVEC3 velocity = jumpVelocity;
			velocity[1] -= GTA_GRAVITY * (now - launchTime);
			body->SetVelocity(velocity);
			jumpLastPos = pos;
			jumpLastVel = velocity;
			jumpFrames++;
		}

		// --- Pistol ------------------------------------------------------------------------------
		static zCSoundFX* shotSound = nullptr;
		static zCSoundFX* reloadSound = nullptr;

		// Sounds from CJ_Weapons.vdf (make_cj_pistol.py), played at CJ.
		static void PlaySfx(zCSoundFX*& sound, const char* file, zCVob* source)
		{
			if (!zsound)
				return;
			if (!sound)
				sound = zsound->LoadSoundFX(zSTRING(file));
			if (sound)
				zsound->PlaySound3D(sound, source, zSND_SLOT_NONE, nullptr);
		}

		// Sounds by file name, loaded once and kept (a missing file stays null: no sound).
		static zCSoundFX* Sound(const char* file)
		{
			struct Entry { const char* file; zCSoundFX* fx; };
			static Entry cache[24];
			static int count = 0;
			for (int i = 0; i < count; i++)
				if (!strcmp(cache[i].file, file))
					return cache[i].fx;
			zCSoundFX* fx = zsound ? zsound->LoadSoundFX(zSTRING(file)) : nullptr;
			if (count < 24)
				cache[count++] = { file, fx };
			return fx;
		}

		// One sound at a vob, `volume` 0..1, heard up to `radius` cm.
		static void PlayAt(const char* file, zCVob* source, float volume = 1.0f, float radius = 3000.0f)
		{
			zCSoundFX* fx = Sound(file);
			if (!fx || !source || !zsound)
				return;
			zCSoundSystem::zTSound3DParams params;
			params.SetDefaults();
			params.volume = volume;
			params.radius = radius;
			zsound->PlaySound3D(fx, source, zSND_SLOT_NONE, &params);
		}

		// A looping sound at a vob that fades towards the volume asked for each frame; asked for
		// another file, it changes over to that one.
		struct Loop
		{
			const char* file = nullptr;
			int handle = 0;
			float volume = 0.0f;

			void Update(zCVob* source, const char* name, float want, float pitch, float dt)
			{
				if (!zsound || !source)
					return;
				if (name != file)
				{
					Stop();
					file = name;
				}
				volume += (want - volume) * (dt / 0.1f < 1.0f ? dt / 0.1f : 1.0f);
				zCSoundFX* fx = file ? Sound(file) : nullptr;
				if (!fx || (want <= 0.0f && volume < 0.02f))
				{
					if (handle)
						zsound->StopSound(handle);
					handle = 0;
					return;
				}
				zCSoundSystem::zTSound3DParams params;
				params.SetDefaults();
				params.loopType = zCSoundSystem::zSND_LOOPING_ENABLED;
				params.volume = volume;
				params.radius = 3000.0f;
				params.pitchOffset = 12.0f * log2f(pitch > 0.1f ? pitch : 0.1f);
				if (!handle || !zsound->IsSoundActive(handle))
					handle = zsound->PlaySound3D(fx, source, zSND_SLOT_NONE, &params);
				else
					zsound->UpdateSound3D(handle, &params);
			}

			void Stop()
			{
				if (zsound && handle)
					zsound->StopSound(handle);
				handle = 0;
				volume = 0.0f;
			}
		};

		// The dildo scares CJ's enemies off (the plugin's own idea, not GTA's): Gothic's flee states,
		// ZS_Flee for people (they run with HUMANS_FLEE.MDS and stay hostile to CJ afterwards) and
		// ZS_MM_Flee for monsters (they run until 20 m away), with CJ as the one to flee from: the
		// parser's OTHER, which StartAIState keeps for the state, and the enemy, which Gothic's flee
		// movement runs from. What the NPC was doing (its attack) is dropped. Only for NPCs hostile to
		// CJ or fighting him (ZS_Flee would make anyone else hostile), unless `hit`: whoever the dildo
		// hits runs whatever they think of him.
		static int fleeState = -1, monsterFleeState = -1;

		static bool Scare(oCNpc* npc, oCNpc* hero, bool hit)
		{
			if (!parser || !npc || npc == hero || npc->IsDead() || npc->IsUnconscious() || npc->IsAPlayer())
				return false;
			if (fleeState < 0)
			{
				fleeState = parser->GetIndex(zSTRING("ZS_FLEE"));
				monsterFleeState = parser->GetIndex(zSTRING("ZS_MM_FLEE"));
			}
			bool monster = npc->IsMonster() != 0;
			int state = monster ? monsterFleeState : fleeState;
			if (state < 0 || npc->state.IsInState(state))
				return false;
			if (!hit && npc->GetAttitude(hero) != NPC_ATT_HOSTILE && npc->enemy != hero)
				return false;
			zCPar_Symbol* self = parser->GetSymbol(zSTRING("SELF"));
			zCPar_Symbol* other = parser->GetSymbol(zSTRING("OTHER"));
			void* oldSelf = self ? self->GetInstanceAdr() : nullptr;
			void* oldOther = other ? other->GetInstanceAdr() : nullptr;
			parser->SetInstance(zSTRING("SELF"), npc);
			parser->SetInstance(zSTRING("OTHER"), hero);
			npc->ClearEM();
			npc->SetEnemy(hero);
			npc->state.StartAIState(zSTRING(monster ? "ZS_MM_FLEE" : "ZS_FLEE"), TRUE, 0, 0.0f, FALSE);
			parser->SetInstance(zSTRING("SELF"), oldSelf);
			parser->SetInstance(zSTRING("OTHER"), oldOther);
			Log("dildo: %s runs away (%s, %.0f m)", npc->GetInstanceName().ToChar(), hit ? "hit" : "scared",
				(npc->GetPositionWorld() - hero->GetPositionWorld()).Length() / 100.0f);
			return true;
		}

		// While the dildo is out, every half second: the enemies within fleeRadius run.
		static void ScareEnemies(oCNpc* hero, bool drawn)
		{
			static float next = 0.0f;
			if (!drawn || Now() < next)
				return;
			next = Now() + 0.5f;
			oCWorld* game = ogame ? ogame->GetGameWorld() : nullptr;
			zVEC3 at = hero->GetPositionWorld();
			for (zCListSort<oCNpc>* node = game && game->voblist_npcs ? game->voblist_npcs->next : nullptr; node; node = node->next)
				if (node->data && node->data != hero && (node->data->GetPositionWorld() - at).Length() < settings.fleeRadius)
					Scare(node->data, hero, false);
		}

		// Whether Gothic still has a weapon message for CJ (drawing or putting away) to run.
		static bool WeaponBusy(oCNpc* hero)
		{
			zCEventManager* em = hero->GetEM();
			for (int i = 0; em && i < em->GetNumMessages(); i++)
				if (zDYNAMIC_CAST<oCMsgWeapon>(em->GetEventMessage(i)))
					return true;
			return false;
		}

		// Whether CJ carries the rocket launcher: in his hand, or in Gothic's ranged slot. Gothic takes a
		// drawn weapon out of the slot (GetEquippedRangedWeapon is empty while it is in the hand), so
		// both count; keyed on the slot alone the stance came off the moment the launcher was drawn.
		static bool RocketCarried(oCNpc* hero)
		{
			FindGuns();
			oCItem* inHand = hero->GetWeaponMode() == NPC_WEAPON_CBOW ? hero->GetWeapon() : nullptr;
			oCItem* equipped = hero->GetEquippedRangedWeapon();
			int rocket = gunIndex[GUN_ROCKET];
			return rocket >= 0 && ((inHand && inHand->GetInstance() == rocket) || (equipped && equipped->GetInstance() == rocket));
		}

		// The rocket launcher's stance, walk and run (CJ_ROCKET.MDS) while CJ carries it (RocketCarried).
		// Like the sprint overlay it goes on and off the model's prototype list directly:
		// Gothic's ApplyOverlay stops every animation the model plays, which broke the launcher's draw
		// (Gothic drew it again and again) and CJ's moves with it. An overlay an earlier version applied
		// the Gothic way (kept in savegames) is removed.
		static zCModelPrototype* rocketProto = nullptr;
		static zCModelPrototype* rocketBase = nullptr;

		static void SetRocketOverlay(oCNpc* hero, bool on)
		{
			zCModel* model = hero->GetModel();
			if (!model || model->modelProtoList.GetNum() == 0)
				return;
			for (int i = 0; i < hero->activeOverlays.GetNum(); i++)
			{
				zSTRING overlay = hero->activeOverlays[i];
				overlay.Upper();
				if (overlay == "CJ_ROCKET.MDS")
				{
					Log("rocket: removing CJ_ROCKET.MDS applied the Gothic way");
					hero->RemoveOverlay(zSTRING("CJ_ROCKET.MDS"));
					break;
				}
			}
			zCModelPrototype* base = model->modelProtoList[0];
			if (base != rocketBase)
			{
				zSTRING name = base->modelProtoName;
				name.Upper();
				rocketBase = base;
				rocketProto = name == "CJ" ? zCModelPrototype::Load(zSTRING("CJ_ROCKET.MDS"), base) : nullptr;
				Log("rocket: CJ_ROCKET.MDS %s", rocketProto ? "loaded" : "not loaded");
				// GTA's run_rocket at CJ's run speed (runSpeed), as his other runs (UpdateSprint)
				if (zCModelAni* run = ProtoAni(rocketProto, model->GetAniIDFromAniName(zSTRING("S_CBOWRUNL"))))
					run->fpsRate = run->fpsRateSource * settings.runSpeed;
			}
			if (!rocketProto || on == (model->modelProtoList.IsInList(rocketProto) != 0))
				return;
			if (on)
			{
				model->modelProtoList.InsertEnd(rocketProto);
				rocketProto->refCtr++;
			}
			else
			{
				model->modelProtoList.RemoveOrder(rocketProto);
				rocketProto->Release();
			}
			Log("rocket: stance overlay %s", on ? "on" : "off");
		}

		// Q/E or the wheel: GTA's weapon cycle through what CJ has, fists, pistol, rocket launcher,
		// dildo (`step` 1 forward, -1 back). Drawing goes through Gothic's weapon messages: the weapon in
		// hand is put away first and the next one drawn once it is (UpdatePendingGun); the draw
		// animation's DEF_FIGHTMODE event switches the mode, and Gothic lets the NPCs around see the
		// drawn weapon. One switch at a time: while one runs, further presses wait (queued weapon
		// messages would otherwise draw and put away weapons long after).
		static void SwitchWeapon(oCNpc* hero, int step)
		{
			if (pendingGun != -2 || WeaponBusy(hero))
			{
				int message = -1; // which weapon message Gothic still has to run (oCMsgWeapon::TWeaponSubType)
				zCEventManager* em = hero->GetEM();
				for (int i = 0; em && i < em->GetNumMessages() && message < 0; i++)
					if (oCMsgWeapon* weapon = zDYNAMIC_CAST<oCMsgWeapon>(em->GetEventMessage(i)))
						message = weapon->subType;
				Log("weapon switch: still switching (to %s, weapon message %d), press ignored",
					pendingGun != -2 ? WeaponName(pendingGun) : "-", message);
				return;
			}
			int owned[GUNS + 2], count = 0;
			owned[count++] = GUN_NONE;
			for (int i = 0; i < GUNS; i++)
				if (GunItem(hero, i))
					owned[count++] = i;
			if (DildoItem(hero))
				owned[count++] = WEAPON_DILDO;
			int current = CurrentWeapon(hero), at = 0;
			for (int i = 0; i < count; i++)
				if (owned[i] == current)
					at = i;
			int next = owned[((at + step) % count + count) % count];
			if (next == current)
				return;
			if (hero->GetWeaponMode() != NPC_WEAPON_NONE)
				hero->GetEM()->OnMessage(new oCMsgWeapon(oCMsgWeapon::EV_REMOVEWEAPON, NPC_WEAPON_NONE, 0), hero);
			pendingGun = next;
			pendingSince = Now();
			Log("weapon switch: %s -> %s", WeaponName(current), WeaponName(next));
		}

		// A new game or a loaded savegame (oCGame::LoadGame / LoadSavegame hooks): the start weapons are
		// checked again for the hero it brings, and nothing of the last game's weapons is pending.
		static void NewGame()
		{
			gunsGiven = false;
			pendingGun = -2;
			reloadUntil = aimHoldUntil = -1.0f;
			for (int& clip : clips)
				clip = -1;
			Log("game: new game or load");
		}

		// The weapon to draw once CJ's hands are free: equipped (Gothic's ranged and melee slots hold
		// one each) and drawn.
		static void UpdatePendingGun(oCNpc* hero)
		{
			if (pendingGun == -2)
				return;
			if (Now() - pendingSince > 3.0f)
			{
				pendingGun = -2;
				return;
			}
			if (hero->GetWeaponMode() != NPC_WEAPON_NONE || pendingGun < 0)
			{
				if (pendingGun < 0 && hero->GetWeaponMode() == NPC_WEAPON_NONE)
					pendingGun = -2;
				return;
			}
			if (WeaponBusy(hero))
				return; // the putting away has not finished
			if (pendingGun == WEAPON_DILDO)
			{
				// the dildo in the melee slot (a sword there could not be swung by CJ anyway)
				oCItem* dildo = DildoItem(hero);
				oCItem* equipped = hero->GetEquippedMeleeWeapon();
				if (dildo && equipped && equipped != dildo)
					hero->UnequipItem(equipped);
				if (dildo && hero->GetEquippedMeleeWeapon() != dildo)
					hero->EquipItem(dildo);
				if (dildo)
				{
					equipped = hero->GetEquippedMeleeWeapon();
					hero->GetEM()->OnMessage(new oCMsgWeapon(oCMsgWeapon::EV_DRAWWEAPON, NPC_WEAPON_1HS, 0), hero);
					Log("weapon switch: drawing the dildo (equipped: %s)", equipped ? equipped->GetInstanceName().ToChar() : "nothing");
				}
				pendingGun = -2;
				return;
			}
			oCItem* gun = GunItem(hero, pendingGun);
			if (gun)
			{
				// one ranged weapon equipped at a time: the other gun off first
				oCItem* equipped = hero->GetEquippedRangedWeapon();
				if (equipped && equipped != gun)
					hero->UnequipItem(equipped);
				if (hero->GetEquippedRangedWeapon() != gun)
					hero->EquipItem(gun);
				equipped = hero->GetEquippedRangedWeapon();
				SetRocketOverlay(hero, RocketCarried(hero));
				hero->GetEM()->OnMessage(new oCMsgWeapon(oCMsgWeapon::EV_DRAWWEAPON, NPC_WEAPON_CBOW, 0), hero);
				Log("weapon switch: drawing the %s (equipped: %s, %d rounds)", GUN_NAMES[pendingGun],
					equipped ? equipped->GetInstanceName().ToChar() : "nothing", AmmoCount(hero, pendingGun));
			}
			pendingGun = -2;
		}

		// Distance along a unit ray to where it enters a box, or -1 if it misses.
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

		// Water along a ray. Gothic's ray tests skip water polygons unless asked to test them
		// (zTRACERAY_POLY_TEST_WATER), and the flags used elsewhere here also skip see-through
		// polygons, which water may count as: this ray asks for water and skips nothing else, and it
		// reports water only when that is the first polygon it meets.
		static bool WaterHit(zCWorld* world, const zVEC3& from, const zVEC3& ray, zCVob* ignore, zVEC3& point)
		{
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_POLY_TEST_WATER | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER;
			if (!world || !world->TraceRayNearestHit(from, ray, ignore, flags) || !world->traceRayReport.foundHit)
				return false;
			zCPolygon* poly = world->traceRayReport.foundPoly;
			if (!poly || !poly->material || poly->material->matGroup != zMAT_GROUP_WATER)
				return false;
			point = world->traceRayReport.foundIntersection;
			return true;
		}

		// What is under the crosshair (the ray UpdateCamera last aimed): the first wall or object, or
		// the first character in front of it, up to the pistol's range. Gothic's ray test does not
		// report characters, so the ray is checked against each NPC's box (narrowed to the body),
		// beyond CJ himself. Returns the NPC hit, if any; `surface` tells where a round that hits no
		// NPC lands on the world (or an object in it).
		struct Surface
		{
			bool hit = false;
			zVEC3 normal;
			int group = zMAT_GROUP_UNDEF; // the polygon's material group (stone, wood, ...)
		};

		static oCNpc* AimTrace(oCNpc* hero, zVEC3& point, Surface* surface = nullptr)
		{
			float range = GUN_RANGE + settings.camDistance;
			point = aimOrigin + aimDir * range;
			if (surface)
				surface->hit = false;
			zCWorld* world = hero->GetHomeWorld();
			if (!aimRayValid || !world)
				return nullptr;
			float wall = range;
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER;
			if (world->TraceRayNearestHit(aimOrigin, aimDir * range, hero, flags) && world->traceRayReport.foundHit)
			{
				const zTTraceRayReport& report = world->traceRayReport;
				point = report.foundIntersection;
				wall = (point - aimOrigin).Length();
				if (surface)
				{
					surface->hit = true;
					surface->normal = report.foundPolyNormal;
					surface->group = report.foundPoly && report.foundPoly->material ? report.foundPoly->material->matGroup : zMAT_GROUP_UNDEF;
				}
			}
			oCWorld* game = ogame ? ogame->GetGameWorld() : nullptr;
			float nearest = (hero->GetPositionWorld() - aimOrigin).Dot(aimDir); // nothing between camera and CJ
			oCNpc* best = nullptr;
			float bestDistance = wall;
			for (zCListSort<oCNpc>* node = game && game->voblist_npcs ? game->voblist_npcs->next : nullptr; node; node = node->next)
			{
				oCNpc* npc = node->data;
				if (!npc || npc == hero || npc->IsDead())
					continue;
				zTBBox3D box = npc->bbox3D;
				zVEC3 centre = (box.mins + box.maxs) * 0.5f;
				for (int i = 0; i < 3; i += 2) // the box reaches the arms' widest pose
				{
					box.mins[i] = centre[i] + (box.mins[i] - centre[i]) * 0.7f;
					box.maxs[i] = centre[i] + (box.maxs[i] - centre[i]) * 0.7f;
				}
				float distance = RayBox(aimOrigin, aimDir, box);
				if (distance > nearest && distance < bestDistance)
				{
					bestDistance = distance;
					best = npc;
				}
			}
			if (best)
			{
				point = aimOrigin + aimDir * bestDistance;
				if (surface)
					surface->hit = false;
				return best;
			}
			// a round stops in water (GTA: CWeapon::DoBulletImpact's water splash); a lake's bed is no hit
			zVEC3 water;
			if (surface && WaterHit(world, aimOrigin, point - aimOrigin, hero, water) && (water - aimOrigin).Length() > nearest)
			{
				point = water;
				surface->hit = true;
				surface->normal = zVEC3(0.0f, 1.0f, 0.0f);
				surface->group = zMAT_GROUP_WATER;
			}
			return nullptr;
		}

		// Bullet impacts (CWeapon::DoBulletImpact -> FxManager AddBulletImpact): a puff of dust where a
		// round hits the world, sparks too off stone and metal, and the sound of a metal item hitting
		// that ground (as Gothic plays for things that fall). Gothic's emitters (PFX_DUST,
		// PFX_METALSPARKS) run on and on, so each one stops after a short burst; the effect then dies
		// with its last particle and Gothic removes it from the world.
		struct Burst
		{
			zCParticleFX* effect = nullptr;
			float stop = 0.0f;
		};
		static Burst bursts[48];

		// A copy of one of Gothic's emitters with its particles `size` times bigger and spread `spread`
		// times wider (the shape they start from, their speed), made once and kept.
		static zCParticleEmitter* Scaled(const char* name, float size, float spread)
		{
			struct Entry { const char* name; float size, spread; zCParticleEmitter* emitter; };
			static Entry cache[16];
			static int count = 0;
			for (int i = 0; i < count; i++)
				if (cache[i].name == name && cache[i].size == size && cache[i].spread == spread)
					return cache[i].emitter;
			zCParticleEmitter* proto = zCParticleFX::SearchParticleEmitter(zSTRING(name));
			if (!proto || count >= 16)
				return proto;
			zCParticleEmitter* emitter = new zCParticleEmitter(*proto);
			emitter->visSizeStart[0] *= size;
			emitter->visSizeStart[1] *= size;
			emitter->shpCircleSphereRadius *= spread;
			emitter->shpLineBoxDim = emitter->shpLineBoxDim * spread;
			emitter->shpOffsetVec = emitter->shpOffsetVec * spread;
			emitter->velAvg *= spread;
			emitter->velVar *= spread;
			cache[count++] = { name, size, spread, emitter };
			return emitter;
		}

		static zCVob* Emit(zCWorld* world, const char* emitter, const zVEC3& point, const zVEC3& normal, float seconds,
			float size = 1.0f, float spread = 1.0f)
		{
			Burst* slot = nullptr;
			for (Burst& b : bursts)
				if (!b.effect)
				{
					slot = &b;
					break;
				}
			if (!slot)
				return nullptr;
			// the emitters fly along their vob's up axis: the surface's normal
			zVEC3 up = normal;
			up.Normalize();
			zVEC3 at = Cross(fabsf(up[1]) < 0.9f ? zVEC3(0.0f, 1.0f, 0.0f) : zVEC3(0.0f, 0.0f, 1.0f), up);
			at.Normalize();
			zMAT4 trafo = zMAT4::GetIdentity();
			trafo.SetRightVector(Cross(up, at));
			trafo.SetUpVector(up);
			trafo.SetAtVector(at);
			trafo.SetTranslation(point + up * 3.0f);

			zCParticleFX* effect = new zCParticleFX();
			zCVob* vob = new zCVob();
			vob->dontWriteIntoArchive = TRUE;
			vob->SetCollDetStat(FALSE);
			vob->SetCollDetDyn(FALSE);
			vob->SetTrafoObjToWorld(trafo);
			vob->SetVisual(effect);
			zCParticleEmitter* scaled = size != 1.0f || spread != 1.0f ? Scaled(emitter, size, spread) : nullptr;
			if (scaled)
				effect->SetAndStartEmitter(scaled, FALSE);
			else
				effect->SetAndStartEmitter(zSTRING(emitter), FALSE);
			world->AddVob(vob);
			vob->SetCollDetStat(FALSE); // (again: no collision with what it shows)
			vob->SetCollDetDyn(FALSE);
			vob->Release(); // the world keeps it; the burst keeps the effect until it stops
			slot->effect = effect;
			slot->stop = Now() + seconds;
			return vob;
		}

		// Water thrown up where something hits or crosses it (Gothic's splash, the one its swimmers
		// make), `size` times as big.
		static zCVob* Splash(zCWorld* world, const zVEC3& point, float size, float seconds = 0.12f)
		{
			return Emit(world, "PFX_WATERSPLASH", point, zVEC3(0.0f, 1.0f, 0.0f), seconds, size, size);
		}

		// Light flashes (GTA's CPointLights::AddLight for gunfire and explosions): a dynamic light of
		// Gothic's, `range` cm, fading from `color` to nothing over `seconds`.
		struct Flash
		{
			zCVobLight* light = nullptr;
			float start = 0.0f, life = 0.0f;
			zCOLOR color;
		};
		static Flash flashes[6];

		static zCVobLight* NewLight(zCWorld* world, const zVEC3& pos, float range, const zCOLOR& color)
		{
			zCVobLight* light = new zCVobLight();
			light->dontWriteIntoArchive = TRUE;
			light->SetCollDetStat(FALSE);
			light->SetCollDetDyn(FALSE);
			light->lightData.isStatic = FALSE;
			light->lightData.isTurnedOn = TRUE;
			light->lightData.lightColor = color;
			light->SetRange(range, TRUE);
			light->SetPositionWorld(pos);
			world->AddVob(light);
			light->SetCollDetStat(FALSE); // (again: no collision with what it shows)
			light->SetCollDetDyn(FALSE);
			return light;
		}

		static void FreeLight(zCVobLight*& light)
		{
			if (!light)
				return;
			if (zCWorld* world = light->GetHomeWorld())
				world->RemoveVob(light);
			light->Release();
			light = nullptr;
		}

		static zCOLOR Color(int r, int g, int b)
		{
			zCOLOR c;
			c.r = (unsigned char)r;
			c.g = (unsigned char)g;
			c.b = (unsigned char)b;
			c.alpha = 255;
			return c;
		}

		static void LightFlash(zCWorld* world, const zVEC3& pos, float range, const zCOLOR& color, float seconds)
		{
			if (!settings.lightFlashes || !world || seconds <= 0.0f)
				return;
			Flash* slot = nullptr;
			for (Flash& f : flashes)
				if (!f.light)
				{
					slot = &f;
					break;
				}
			if (!slot)
				return;
			slot->light = NewLight(world, pos, range, color);
			slot->start = Now();
			slot->life = seconds;
			slot->color = color;
		}

		// every frame: the flashes fade and go
		static void UpdateFlashes()
		{
			float now = ztimer ? Now() : 0.0f;
			for (Flash& f : flashes)
			{
				if (!f.light)
					continue;
				float t = (now - f.start) / f.life;
				if (t >= 1.0f || t < 0.0f || !f.light->GetHomeWorld()) // (or the game time went back: a load)
				{
					FreeLight(f.light);
					continue;
				}
				float k = (1.0f - t) * (1.0f - t);
				f.light->lightData.lightColor = Color((int)(f.color.r * k), (int)(f.color.g * k), (int)(f.color.b * k));
			}
		}

		static void Impact(oCNpc* hero, const zVEC3& point, const Surface& surface)
		{
			zCWorld* world = hero->GetHomeWorld();
			if (!world || !surface.hit)
				return;
			zCVob* source = nullptr;
			if (surface.group == zMAT_GROUP_WATER)
				source = Splash(world, point, 1.0f);
			else
				source = Emit(world, "PFX_DUST", point, surface.normal, 0.1f);
			if (surface.group == zMAT_GROUP_STONE || surface.group == zMAT_GROUP_METAL)
				Emit(world, "PFX_METALSPARKS", point, surface.normal, 0.15f);
			if (source && zsndMan) // 2: the round's material, MAT_METAL
				zsndMan->StartHitSound(source, zCSoundManager::zSND_MAN_MEDIUM_ITEM, zCSoundManager::zSND_MAN_MEDIUM_LEVEL,
					2, surface.group, nullptr);
		}

		// every frame: ends the bursts that have run their time
		static void UpdateBursts()
		{
			float now = ztimer ? Now() : 0.0f;
			for (Burst& b : bursts)
				if (b.effect && (now >= b.stop || now < b.stop - 5.0f)) // (or the game time went back: a load)
				{
					b.effect->StopEmitterOutput();
					b.effect->Release();
					b.effect = nullptr;
				}
		}

		// --- Rocket launcher -------------------------------------------------------------------------
		static void TakeRound(oCNpc* hero, int gun);
		static zCSoundFX* launchSound = nullptr;
		static zCSoundFX* blastSound = nullptr;

		// A sound loud and far (radius 100 m), as GTA's explosions are.
		static void PlayLoud(zCSoundFX*& sound, const char* file, zCVob* source)
		{
			if (!zsound || !source)
				return;
			if (!sound)
				sound = zsound->LoadSoundFX(zSTRING(file));
			if (!sound)
				return;
			zCSoundSystem::zTSound3DParams params;
			params.SetDefaults();
			params.radius = 10000.0f;
			params.volume = 1.0f;
			zsound->PlaySound3D(sound, source, zSND_SLOT_NONE, &params);
		}

		// A particle effect that moves (the missile's trail): the effect and its vob, both kept.
		struct Trail
		{
			zCParticleFX* effect = nullptr;
			zCVob* vob = nullptr;

			void Start(zCWorld* world, const char* emitter, const zVEC3& pos)
			{
				effect = new zCParticleFX();
				vob = new zCVob();
				vob->dontWriteIntoArchive = TRUE;
				vob->SetCollDetStat(FALSE);
				vob->SetCollDetDyn(FALSE);
				vob->SetPositionWorld(pos);
				vob->SetVisual(effect);
				effect->SetAndStartEmitter(zSTRING(emitter), FALSE);
				world->AddVob(vob);
				vob->SetCollDetStat(FALSE); // (again: no collision with what it shows)
				vob->SetCollDetDyn(FALSE);
			}

			void Move(const zVEC3& pos)
			{
				if (vob)
					vob->SetPositionWorld(pos);
			}

			// it stops; the effect dies with its last particle and Gothic removes it from the world
			void Stop()
			{
				if (effect)
				{
					effect->StopEmitterOutput();
					effect->Release();
				}
				if (vob)
					vob->Release();
				effect = nullptr;
				vob = nullptr;
			}
		};

		struct Missile
		{
			bool live = false;
			zCWorld* world = nullptr;
			zCVob* vob = nullptr;
			Trail flame, smoke;
			zVEC3 pos, dir;
			float age = 0.0f;
		};
		static Missile missiles[8];

		static zMAT4 Facing(const zVEC3& dir, const zVEC3& pos)
		{
			zVEC3 ref = fabsf(dir[1]) < 0.95f ? zVEC3(0.0f, 1.0f, 0.0f) : zVEC3(1.0f, 0.0f, 0.0f);
			zVEC3 right = Cross(ref, dir);
			right.Normalize();
			zMAT4 trafo = zMAT4::GetIdentity();
			trafo.SetAtVector(dir);
			trafo.SetRightVector(right);
			trafo.SetUpVector(Cross(dir, right));
			trafo.SetTranslation(pos);
			return trafo;
		}

		// GTA's rocket explosion (CExplosion::AddExplosion EXPLOSION_ROCKET): a fireball some 5 m across
		// (Gothic's fireball impact, firestorm blast and flash made bigger), a column of smoke, a few
		// small fires up to 4 m around (GTA's 0 to 3, burning a few seconds); its sound heard far
		// away; within rocketRadius everyone hurt, the more the closer (killed within some 2/5 of it,
		// half behind a wall) and thrown away from it (Gothic's fly damage); the camera shakes; NPCs
		// hear it.
		namespace Bike { static void Blast(oCNpc* hero, const zVEC3& point, float radius); }

		static void Explode(oCNpc* hero, zCWorld* world, const zVEC3& point)
		{
			zVEC3 up(0.0f, 1.0f, 0.0f);
			zCVob* source = Emit(world, "MFX_FIREBALL_COLLIDE4", point, up, 0.3f, 3.0f, 2.0f);
			Emit(world, "MFX_FIRESTORM_INVESTBLAST4", point + up * 80.0f, up, 0.35f, 8.0f, 12.0f);
			Emit(world, "MFX_FIREBALL_INVEST_BLAST", point + up * 60.0f, up, 0.3f, 5.0f, 5.0f);
			Emit(world, "FIRE_SMOKE", point + up * 50.0f, up, 2.5f, 5.0f, 4.0f);
			LightFlash(world, point + up * 100.0f, 1500.0f, Color(255, 170, 80), 1.0f); // CExplosion's light
			// on water a column of spray instead of fires (GTA: CExplosion's water splash)
			zVEC3 water;
			bool onWater = WaterHit(world, point + up * 150.0f, up * -300.0f, hero, water);
			if (onWater)
				Splash(world, water, 5.0f, 0.4f);
			int fires = onWater ? 0 : (int)(Now() * 1000.0f) % 4;
			for (int i = 0; i < fires; i++)
			{
				float a = Now() * 7.3f + i * 2.1f, r = 150.0f + 250.0f * fabsf(sinf(a * 1.7f));
				zVEC3 at = point + zVEC3(cosf(a) * r, 0.0f, sinf(a) * r);
				zVEC3 from = at + up * 300.0f; // the ground there (GTA: CWorld::FindGroundZFor3DCoord)
				if (world->TraceRayNearestHit(from, up * -1300.0f, hero, zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER)
					&& world->traceRayReport.foundHit && fabsf(world->traceRayReport.foundIntersection[1] - point[1]) < 1000.0f)
				{
					zVEC3 ground = world->traceRayReport.foundIntersection;
					if (!WaterHit(world, from, ground - from, hero, water)) // no fire burns under water
						Emit(world, "FIRE_MEDIUM", ground, up, 4.0f + 4.0f * fabsf(sinf(a)));
				}
			}
			PlayLoud(blastSound, "CJ_ROCKET_EXPLOSION.WAV", source ? source : hero);
			Bike::Blast(hero, point, settings.rocketRadius > 50.0f ? settings.rocketRadius : 50.0f); // (CJ thrown off first)

			oCWorld* game = ogame ? ogame->GetGameWorld() : nullptr;
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER;
			float radius = settings.rocketRadius > 50.0f ? settings.rocketRadius : 50.0f;
			auto blast = [&](oCNpc* npc)
			{
				if (!npc || npc->IsDead())
					return;
				zVEC3 centre = (npc->bbox3D.mins + npc->bbox3D.maxs) * 0.5f;
				zVEC3 away = centre - point;
				float d = away.Length();
				float share = (radius - d) / (radius * 0.6f);
				if (share <= 0.0f)
					return;
				share = (share > 1.0f ? 1.0f : share) * 1.5f;
				zVEC3 from = point + up * 30.0f;
				if (world->TraceRayNearestHit(from, centre - from, npc, flags) && world->traceRayReport.foundHit)
					share *= 0.5f; // behind cover
				blastTarget = npc;
				blastDamage = share * npc->attribute[NPC_ATR_HITPOINTSMAX];
				blastDir = Flat(away) + up * 0.6f;
				blastDir.Normalize();
				blastLethal = true;
				bool lethal = blastDamage >= npc->attribute[NPC_ATR_HITPOINTS];
				if (npc == hero)
				{
					if (source)
						npc->OnDamage(source, nullptr, blastDamage, oEDamageType_Blunt | oEDamageType_Fly, point);
				}
				else
					npc->OnDamage(hero, hero, blastDamage, oEDamageType_Blunt | oEDamageType_Fly, point);
				blastTarget = nullptr;
				blastLethal = false;
				// Gothic knocks a human beaten down by blunt damage out instead of killing him (and the
				// log showed guards taking blast after blast that way): a blast that takes all his
				// health kills, as GTA's does. Scripted immortals (NPC_FLAG_IMMORTAL) lose no health
				// and stay as they are.
				bool knockedOut = !npc->IsDead() && (npc->IsUnconscious() || npc->attribute[NPC_ATR_HITPOINTS] <= 1);
				if (npc != hero && lethal && knockedOut && !npc->HasFlag(NPC_FLAG_IMMORTAL))
				{
					npc->attribute[NPC_ATR_HITPOINTS] = 0;
					npc->DoDie(hero);
					Log("rocket: %s killed (Gothic had knocked him out)", npc->GetInstanceName().ToChar());
				}
				Log("rocket: %s %.0f m from the blast, %.0f damage", npc->GetInstanceName().ToChar(), d / 100.0f, share * npc->attribute[NPC_ATR_HITPOINTSMAX]);
			};
			for (zCListSort<oCNpc>* node = game && game->voblist_npcs ? game->voblist_npcs->next : nullptr; node; node = node->next)
				if (node->data != hero)
					blast(node->data);
			blast(hero);
			if (zCCamera::activeCam && zCCamera::activeCam->connectedVob)
			{
				float d = (zCCamera::activeCam->connectedVob->GetPositionWorld() - point).Length();
				float s = 1.0f - d / 3000.0f;
				shake = s > shake ? s : shake;
			}
			oCNpc::AssessFightSound_S(hero, point, nullptr);
		}

		static void EndMissile(Missile& m)
		{
			m.flame.Stop();
			m.smoke.Stop();
			if (m.vob)
			{
				if (zCWorld* world = m.vob->GetHomeWorld())
					world->RemoveVob(m.vob);
				m.vob->Release();
			}
			m.vob = nullptr;
			m.live = false;
		}

		// A rocket from the launcher's muzzle (weapon.dat RLAUNCHER fire offset 0.42 0 0.05 in the
		// hand's frame) towards what is under the crosshair; `level` (fired without aiming): straight
		// ahead the way CJ faces, level, as GTA fires it from the shoulder. (Towards the crosshair the
		// follow camera's downward look put the rocket into the ground a few metres in front of CJ.)
		static void FireRocket(oCNpc* hero, const zVEC3& point, bool level = false)
		{
			zCWorld* world = hero->GetHomeWorld();
			zCModel* model = hero->GetModel();
			if (!world || !model)
				return;
			Missile* m = nullptr;
			for (Missile& slot : missiles)
				if (!slot.live)
				{
					m = &slot;
					break;
				}
			if (!m)
				return;
			zVEC3 muzzle = aimOrigin + aimDir * 60.0f;
			if (zCModelNodeInst* hand = model->SearchNode(zSTRING("ZS_RIGHTHAND")))
			{
				zMAT4 trafo = hero->GetTrafoModelNodeToWorld(hand);
				muzzle = trafo.GetTranslation() + trafo.GetRightVector() * 42.0f + trafo.GetUpVector() * 5.0f;
			}
			if (fpBlend > 0.5f) // aimed in first person: from just below the eye, as the view is GTA's
				muzzle = aimOrigin + aimDir * 40.0f + zVEC3(0.0f, -15.0f, 0.0f);
			zVEC3 dir = point - muzzle;
			if (dir.Length() < 100.0f)
				dir = aimDir;
			if (level)
				dir = Flat(hero->GetAtVectorWorld());
			dir.Normalize();
			TakeRound(hero, GUN_ROCKET);
			LightFlash(world, muzzle, 400.0f, Color(255, 190, 110), 0.15f);
			m->live = true;
			m->world = world;
			m->pos = muzzle;
			m->dir = dir;
			m->age = 0.0f;
			m->vob = new zCVob();
			m->vob->dontWriteIntoArchive = TRUE;
			m->vob->SetCollDetStat(FALSE);
			m->vob->SetCollDetDyn(FALSE);
			m->vob->SetVisual(zSTRING("CJ_MISSILE.3DS"));
			m->vob->SetTrafoObjToWorld(Facing(dir, muzzle));
			world->AddVob(m->vob);
			m->vob->SetCollDetStat(FALSE); // (again: no collision with what it shows)
			m->vob->SetCollDetDyn(FALSE);
			m->flame.Start(world, "MFX_FIREBALL_INIT", muzzle);
			m->smoke.Start(world, "FIRE_SMOKE", muzzle);
			PlayLoud(launchSound, "CJ_ROCKET_LAUNCH.WAV", hero);
			model->StartAni(zSTRING("T_CJ_ROCKETFIRE"), zCModel::zMDL_STARTANI_DEFAULT);
			oCNpc::AssessFightSound_S(hero, hero->GetPositionWorld(), nullptr);
			Log("rocket: fired (%d left)", AmmoCount(hero, GUN_ROCKET));
		}

		// Every frame: the missiles fly on and blow up on the first wall, object or person in their
		// way (or after ROCKET_LIFE); the camera shake fades.
		static void UpdateMissiles()
		{
			float dt = ztimer ? ztimer->frameTimeFloat / 1000.0f : 0.0f;
			shake = shake > 0.0f ? shake - dt * 1.5f : 0.0f;
			oCNpc* hero = oCNpc::player;
			for (Missile& m : missiles)
			{
				if (!m.live)
					continue;
				if (!hero || hero->GetHomeWorld() != m.world)
				{
					EndMissile(m);
					continue;
				}
				float step = ROCKET_SPEED * dt, hit = -1.0f;
				int flags = zTRACERAY_STAT_POLY | zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER;
				if (step > 0.0f && m.world->TraceRayNearestHit(m.pos, m.dir * step, hero, flags) && m.world->traceRayReport.foundHit)
					hit = (m.world->traceRayReport.foundIntersection - m.pos).Length();
				oCWorld* game = ogame ? ogame->GetGameWorld() : nullptr;
				for (zCListSort<oCNpc>* node = game && game->voblist_npcs ? game->voblist_npcs->next : nullptr; node; node = node->next)
				{
					oCNpc* npc = node->data;
					if (!npc || npc == hero || npc->IsDead())
						continue;
					zTBBox3D box = npc->bbox3D; // narrowed to the body, as for the pistol (AimTrace)
					zVEC3 centre = (box.mins + box.maxs) * 0.5f;
					for (int i = 0; i < 3; i += 2)
					{
						box.mins[i] = centre[i] + (box.mins[i] - centre[i]) * 0.7f;
						box.maxs[i] = centre[i] + (box.maxs[i] - centre[i]) * 0.7f;
					}
					float d = RayBox(m.pos, m.dir, box);
					if (d >= 0.0f && d <= step && (hit < 0.0f || d < hit))
						hit = d;
				}
				zVEC3 water; // it blows up on water, not on the bed below it
				if (step > 0.0f && WaterHit(m.world, m.pos, m.dir * step, hero, water))
				{
					float d = (water - m.pos).Length();
					hit = hit < 0.0f || d < hit ? d : hit;
				}
				m.age += dt;
				if (hit >= 0.0f || m.age > ROCKET_LIFE)
				{
					zVEC3 at = m.pos + m.dir * (hit >= 0.0f ? (hit > 20.0f ? hit - 20.0f : 0.0f) : step);
					Log("rocket: %s after %.1f s", hit >= 0.0f ? "hit" : "blew up in the air", m.age);
					zCWorld* world = m.world;
					EndMissile(m);
					Explode(hero, world, at);
					continue;
				}
				m.pos += m.dir * step;
				m.vob->SetTrafoObjToWorld(Facing(m.dir, m.pos));
				m.flame.Move(m.pos - m.dir * 45.0f);
				m.smoke.Move(m.pos - m.dir * 70.0f);
			}
		}

		// colt45_fire's kick, from its frames 6-10 (30 fps): the barrel rises 11 degrees in two frames
		// (67 ms), stays near there a frame and is level again at 133 ms. As combine Y (the poses span
		// 2 * AIM_PITCH degrees).
		static float Recoil(float now)
		{
			float t = lastShot >= 0.0f ? now - lastShot : 1.0f;
			float kick = t < 0.067f ? t / 0.067f : t < 0.1f ? 1.0f - (t - 0.067f) / 0.033f * 0.13f
				: t < 0.133f ? 0.87f * (0.133f - t) / 0.033f : 0.0f;
			return kick * 11.0f / (2.0f * AIM_PITCH);
		}

		// GTA's muzzle flash for a moment on each shot (CPed::SetGunFlashAlpha), drawn additively, in
		// the hand slot make_cj_model.py adds for it.
		static const float FLASH_TIME = 0.06f;

		static void ShowFlash(zCModel* model, bool on)
		{
			zCModelNodeInst* node = model ? model->SearchNode(zSTRING("ZS_GUNFLASH")) : nullptr;
			if (!node)
				return;
			if (on && !flashVisual)
			{
				flashVisual = zCVisual::LoadVisual(zSTRING("CJ_GUNFLASH.3DS"));
				if (zCProgMeshProto* mesh = zDYNAMIC_CAST<zCProgMeshProto>(flashVisual))
					for (int i = 0; i < mesh->numSubMeshes; i++)
						if (mesh->subMeshList[i].material)
							mesh->subMeshList[i].material->rndAlphaBlendFunc = zRND_ALPHA_FUNC_ADD;
			}
			if (on != (node->nodeVisual != nullptr))
				model->SetNodeVisual(node, on ? flashVisual : nullptr, FALSE);
		}

		// One round of a gun from the inventory stack (the last one leaves it).
		static void TakeRound(oCNpc* hero, int gun)
		{
			FindGuns();
			if (oCItem* rounds = gunAmmo[gun] >= 0 ? hero->IsInInv(gunAmmo[gun], 1) : nullptr)
			{
				if (rounds->amount > 1)
					rounds->amount--;
				else
					hero->RemoveFromInv(rounds, 1);
			}
		}

		// One pistol round: GTA's sound, the hit.
		static void Shoot(oCNpc* hero, oCNpc* target, const zVEC3& point, const Surface& surface)
		{
			TakeRound(hero, GUN_PISTOL);
			PlaySfx(shotSound, "CJ_PISTOL_SHOT.WAV", hero);
			lastShot = Now();
			ShowFlash(hero->GetModel(), true);
			if (zCModel* model = hero->GetModel()) // the flash lights up what is around (GTA: 3 m)
			{
				zCModelNodeInst* node = model->SearchNode(zSTRING("ZS_GUNFLASH"));
				zVEC3 muzzle = node ? model->GetNodePositionWorld(node) : hero->GetPositionWorld() + zVEC3(0.0f, 60.0f, 0.0f);
				LightFlash(hero->GetHomeWorld(), muzzle, 300.0f, Color(255, 220, 140), 0.08f);
			}
			oCNpc::AssessFightSound_S(hero, hero->GetPositionWorld(), target);
			if (!target)
			{
				Impact(hero, point, surface);
				Log("shot: missed (hit the world %d, material group %d)", (int)surface.hit, surface.group);
				return;
			}
			// GTA's 25 of a ped's 100 health, as that share of the target's (OnDamage_Hit hook);
			// Gothic then plays the hit, blood and reactions, and a ranged hit kills.
			float damage = GUN_DAMAGE / 100.0f * target->attribute[NPC_ATR_HITPOINTSMAX] * settings.gunDamage;
			gunTarget = target;
			target->OnDamage(hero, hero, damage, oEDamageType_Point, point);
			gunTarget = nullptr;
			Log("shot: hit %s (health %d/%d)", target->GetInstanceName().ToChar(),
				target->attribute[NPC_ATR_HITPOINTS], target->attribute[NPC_ATR_HITPOINTSMAX]);
		}

		// The gun walk's share of each bone. A diagonal, as GTA blends its forward and sideways
		// animations: Gothic shares each bone out among its animations by weights it keeps summing to
		// 1 (zCModelNodeInst::CalcWeights), so both get, fixed, half of what the animations above them
		// (aim, reload) leave, and Gothic's moves underneath get none until the gun walk fades out.
		// A single one that is left over from a diagonal (Gothic gave it only part when the other
		// faded out) gets all of it again once it has faded in.
		static void BlendGunMoves(zCModel* model, zCModelAniActive* first, zCModelAniActive* second)
		{
			zCModelAniActive* moves[2] = { first, second };
			int count = second ? 2 : 1;
			int a = first->protoAni->layer, b = second ? second->protoAni->layer : a;
			int low = a < b ? a : b, high = a > b ? a : b;
			for (int n = 0; n < model->nodeList.GetNum(); n++)
			{
				zCModelNodeInst* node = model->nodeList[n];
				int found[2] = { -1, -1 };
				float above = 0.0f;
				for (int i = 0; i < node->numNodeAnis; i++)
				{
					zCModelAniActive* ani = node->nodeAniList[i].modelAni;
					if (ani == moves[0])
						found[0] = i;
					else if (count == 2 && ani == moves[1])
						found[1] = i;
					else if (ani && ani->protoAni->layer > high)
						above += node->nodeAniList[i].weight;
				}
				if (found[0] < 0 || (count == 2 && found[1] < 0))
					continue;
				if (count == 1 && node->nodeAniList[found[0]].blendState != zCModelNodeInst::zMDL_BLEND_STATE_CONST)
					continue; // still fading in: Gothic's own blend
				float share = (above < 1.0f ? 1.0f - above : 0.0f) / count;
				for (int i = 0; i < node->numNodeAnis; i++)
				{
					zCModelNodeInst::zTNodeAni& entry = node->nodeAniList[i];
					if (i == found[0] || i == found[1])
					{
						entry.weight = share;
						entry.weightSpeed = 0.0f;
						entry.blendState = zCModelNodeInst::zMDL_BLEND_STATE_CONST;
					}
					else if (entry.modelAni && entry.modelAni->protoAni->layer < low)
					{
						entry.weight = 0.0f;
						entry.weightSpeed = 0.0f;
					}
				}
			}
		}

		static void StartReload(oCNpc* hero, zCModel* model)
		{
			reloadUntil = Now() + RELOAD_TIME;
			model->StartAni(zSTRING("T_CJ_PISTOLRELOAD"), zCModel::zMDL_STARTANI_DEFAULT);
			PlaySfx(reloadSound, "CJ_PISTOL_RELOAD.WAV", hero);
			Log("reload (%d rounds)", AmmoCount(hero, GUN_PISTOL));
		}

		// Aiming (right button, or a short while after a click) and firing (left button), every
		// frame on the ground with the pistol out. CJ's arm follows the crosshair through the nine
		// poses of t_CJ_PistolAim: combine X/Y from the crosshair point's direction seen from his
		// right shoulder.
		static void UpdateGun(oCNpc* hero, bool grounded)
		{
			float now = Now(), dt = ztimer->frameTimeFloat / 1000.0f;
			zCModel* model = hero->GetModel();
			int gun = model ? DrawnGun(hero) : GUN_NONE;
			bool drawn = gun != GUN_NONE, rocket = gun == GUN_ROCKET;
			int& clip = clips[drawn ? gun : GUN_PISTOL];
			int clipSize = GUN_CLIP[drawn ? gun : GUN_PISTOL];
			int ammo = AmmoCount(hero, drawn ? gun : GUN_PISTOL);
			if (clip < 0 || clip > ammo)
				clip = ammo < clipSize ? ammo : clipSize;
			bool reloading = reloadUntil >= 0.0f;
			if (reloading && (now >= reloadUntil || !drawn))
			{
				reloadUntil = -1.0f;
				reloading = false;
				clip = ammo < clipSize ? ammo : clipSize;
			}
			// the rocket launcher's stance, walk and run while it is the equipped ranged weapon (heavy
			// while it is out: GTA's CJ neither sprints nor jumps with it)
			heavy = rocket;
			oCItem* equipped = hero->GetEquippedRangedWeapon();
			SetRocketOverlay(hero, RocketCarried(hero));
			static int lastGun = GUN_NONE;
			static float drawnAt = 0.0f;
			if (gun != lastGun)
			{
				drawnAt = now;
				oCItem* inHand = hero->GetWeapon();
				Log("drawn: %s (in hand %s, equipped %s)", gun >= 0 ? GUN_NAMES[gun] : "no gun",
					inHand ? inHand->GetInstanceName().ToChar() : "nothing", equipped ? equipped->GetInstanceName().ToChar() : "nothing");
				lastGun = gun;
			}
			bool lmb = drawn && MouseDown(true), rmb = drawn && MouseDown(false);
			if (lmb && !rocket)
				aimHoldUntil = now + 0.6f; // the launcher fires from the shoulder without aiming
			bool wasAiming = aiming;
			aiming = drawn && grounded && (rmb || (!rocket && now < aimHoldUntil));
			if (aiming != wasAiming)
				Log("aim %s", aiming ? "on" : "off");
			aimUp = aiming && KeyDown(KEY_W);
			aimDown = aiming && KeyDown(KEY_S);
			aimLeft = aiming && KeyDown(KEY_A);
			aimRight = aiming && KeyDown(KEY_D);
			if (lastShot >= 0.0f && now - lastShot > FLASH_TIME)
				ShowFlash(model, false);
			if (!model)
				return;

			int aimId = model->GetAniIDFromAniName("T_CJ_PISTOLAIM");
			zCModelAni* aimAni = aimId >= 0 ? model->GetAniFromAniID(aimId) : nullptr;
			bool posing = aiming && !rocket && !reloading && aimAni;
			bool playing = aimAni && model->IsAniActive(aimAni);
			if (posing && !playing)
				model->StartAni(aimAni, zCModel::zMDL_STARTANI_DEFAULT);
			else if (!posing && playing)
				model->FadeOutAni(aimAni);

			// GTA's gun walk while aiming: W/S forward and back (layer 13), A/D sideways (layer 14),
			// both at once on a diagonal (BlendGunMoves). Their root motion moves CJ; Gothic's own
			// moves get no keys meanwhile (OverrideKey), so its crossbow mode's hops never start.
			static const char* const GUN_MOVES[4] = { "T_CJ_GUNMOVEF", "T_CJ_GUNMOVEB", "T_CJ_GUNMOVEL", "T_CJ_GUNMOVER" };
			int along = aimUp ? 0 : aimDown ? 1 : -1, side = aimLeft ? 2 : aimRight ? 3 : -1;
			zCModelAniActive* playing2[2] = { nullptr, nullptr };
			for (int i = 0; i < 4; i++)
			{
				zCModelAni* ani = model->GetAniFromAniID(model->GetAniIDFromAniName(GUN_MOVES[i]));
				if (!ani)
					continue;
				bool wanted = i == along || i == side;
				zCModelAniActive* active = model->GetActiveAni(ani);
				if (wanted && (!active || active->isFadingOut))
					model->StartAni(ani, zCModel::zMDL_STARTANI_DEFAULT);
				else if (!wanted && active && !active->isFadingOut)
					model->FadeOutAni(ani);
				if (wanted)
					playing2[i < 2 ? 0 : 1] = model->GetActiveAni(ani);
			}
			if (playing2[0] || playing2[1])
				BlendGunMoves(model, playing2[0] ? playing2[0] : playing2[1], playing2[0] ? playing2[1] : nullptr);
			// nothing fires while the draw still swings the gun up (Gothic switches the mode halfway
			// through it; its weapon message stays queued for a while after, so that is no guide)
			bool ready = now - drawnAt > 0.35f;
			if (rocket && lmb && !aiming && now >= nextShot && clip > 0 && cameraActive && ready)
			{
				hero->SetHeadingAtWorld(cameraForward); // fired from the shoulder, where the camera looks
				zVEC3 point;
				AimTrace(hero, point);
				FireRocket(hero, point, true);
				nextShot = now + ROCKET_INTERVAL;
				return;
			}
			if (!aiming)
				return;

			zVEC3 point;
			Surface surface;
			oCNpc* target = AimTrace(hero, point, &surface);
			if (posing)
			{
				// GTA points the arm with an IK chain whatever the moves underneath do to the torso
				// (its gun walk turns CJ up to 50 degrees). Here the blend of the nine poses is steered
				// each frame by how far the gun's barrel (ZS_RIGHTHAND's X axis: colt45.dff lies along
				// the hand's X) points off the crosshair point; it starts from the direction seen from
				// the shoulder.
				zVEC3 at = Flat(hero->GetAtVectorWorld()), right = RightOf(at, hero);
				auto direction = [&](const zVEC3& d, float& yaw, float& pitch)
				{
					yaw = atan2f(d.Dot(right), d.Dot(at)) * 180.0f / PI;
					pitch = atan2f(d[1], sqrtf(d[0] * d[0] + d[2] * d[2])) * 180.0f / PI;
				};
				zCModelNodeInst* hand = model->SearchNode(zSTRING("ZS_RIGHTHAND"));
				float wantYaw, wantPitch;
				bool steered = false;
				if (playing && hand)
				{
					zMAT4 trafo = hero->GetTrafoModelNodeToWorld(hand);
					zVEC3 barrel = trafo.GetRightVector();
					zVEC3 toPoint = point - trafo.GetTranslation();
					float gunYaw, gunPitch;
					direction(toPoint, wantYaw, wantPitch);
					direction(barrel, gunYaw, gunPitch);
					float yawError = wantYaw - gunYaw, pitchError = wantPitch - gunPitch;
					yawError -= yawError > 180.0f ? 360.0f : (yawError < -180.0f ? -360.0f : 0.0f);
					static float lastAimLog = -1.0f;
					if (now - lastAimLog > 2.0f)
					{
						lastAimLog = now;
						Log("aim: barrel off by %.0f / %.0f degrees, pose blend %.2f %.2f", yawError, pitchError, aimX, aimY);
					}
					if (Recoil(now) > 0.0f)
						steered = true; // the kick moves the barrel: hold the blend until it is over
					else if (fabsf(yawError) < 100.0f) // else the barrel is not where it should be: open loop
					{
						aimX += AIM_GAIN * yawError / (2.0f * AIM_YAW);
						aimY -= AIM_GAIN * pitchError / (2.0f * AIM_PITCH);
						steered = true;
					}
				}
				if (!steered)
				{
					zCModelNodeInst* shoulder = model->SearchNode(zSTRING("BIP01 R UPPERARM"));
					direction(point - (shoulder ? model->GetNodePositionWorld(shoulder) : hero->GetPositionWorld()), wantYaw, wantPitch);
					aimX = 0.5f + wantYaw / (2.0f * AIM_YAW);
					aimY = 0.5f - wantPitch / (2.0f * AIM_PITCH);
				}
				aimX = aimX < 0.0f ? 0.0f : (aimX > 1.0f ? 1.0f : aimX);
				aimY = aimY < 0.0f ? 0.0f : (aimY > 1.0f ? 1.0f : aimY);
				float kicked = aimY - Recoil(now);
				model->SetCombineAniXY(aimId, aimX, kicked < 0.0f ? 0.0f : kicked);
			}
			if (!lmb || reloading || now < nextShot || !ready)
				return;
			if (rocket) // one rocket a load: RocketFire is the shot and the reload
			{
				if (clip > 0)
				{
					FireRocket(hero, point);
					nextShot = now + ROCKET_INTERVAL;
				}
				return;
			}
			if (clip > 0)
			{
				Shoot(hero, target, point, surface);
				clip--;
				nextShot = now + SHOT_INTERVAL;
				if (clip == 0 && AmmoCount(hero, GUN_PISTOL) > 0)
					StartReload(hero, model); // GTA reloads as soon as the clip is empty
			}
			else if (ammo > 0)
				StartReload(hero, model);
		}

		// CJ's eyes are two small separate pieces of his head mesh (head.dff). Gothic's progressive
		// meshes drop such pieces first as the camera moves away (zCProgMeshProto::
		// CalcLODRenderVertPerc: full detail only within zDisplace2, squared cm), so his eyes vanished
		// beyond arm's length. His skin meshes always render in full.
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
			Log("full detail for %d skin mesh(es)", model->meshSoftSkinList.GetNum());
		}

		// --- Motorbike ---------------------------------------------------------------------------
		// GTA San Andreas' motorbike (make_cj_bike.py: the Sanchez unless told otherwise; its points,
		// handling.cfg numbers and the get on/off paths come from system\CJBike.ini). It stands next
		// to CJ when he enters a world; F next to it gets him on from the side he stands on (GTA's
		// jump-on animation, his centre carried along its path), F again gets him off: to the left
		// (the right if a wall is in the way), or at speed he bails out and the bike falls over.
		//
		// Riding, with GTA's keys: W throttle, S brake and then reverse, A/D steer, Space the handbrake
		// (locks the rear wheel: it slides), Up/Down arrows (or Num8/Num2) lean forward/back: the
		// rider's pose, wheelies (back on the throttle) and stoppies (forward on the brake), and in
		// the air the bike's pitch. GTA's own bike code (CBike::ProcessControl, ProcessBikeWheel,
		// ProcessRiderAnims) is not reverse-engineered in gta-reversed; this uses what is:
		//  - drive: cTransmission::CalculateDriveAcceleration over the gears of InitGearRatios, the
		//    numbers converted as cHandlingDataMgr::ConvertDataToGameUnits does; fBrakeDeceleration;
		//    air resistance as CPhysical::ApplyAirResistance with fDragMult / 2000; GTA's gravity
		//    along the slope, so hills slow the bike and it rolls back down them; bikeSpeed scales it;
		//  - tyres: a rigid body like GTA's vehicles (its own velocity and turning, mass and turn
		//    mass from handling.cfg) with GTA's tyre (CVehicle::ProcessWheel) on each wheel: grip
		//    from fTractionMultiplier, fTractionBias and GTA's surface grip, less across the wheel
		//    (see Tyre), so a spinning or locked wheel (fTractionLoss) lets the bike slide; steering
		//    up to fSteeringLock, less with speed (fSpeedSteer); the bike leans as far as its turn
		//    needs, up to fMaxLean;
		//  - ground: rays find the ground under both wheels; the bike follows it unless the ground
		//    falls away faster than GTA's gravity pulls it down by more than the suspension reaches
		//    (fSuspensionLowerLimit), so crests and ramps throw it into the air, where it flies until
		//    the ground meets it again;
		//  - walls and steps over 35 cm stop it (hit fast, CJ is thrown off and hurt), people in its
		//    way are thrown down and hurt by its speed; a rocket's blast throws it;
		//  - water holds it back the deeper it goes, and water up to the seat drowns its engine and
		//    throws CJ in (it no longer rides across lakes); in water it sinks slowly instead of
		//    falling;
		//  - what GTA shows and plays around it: dust behind a spinning or skidding wheel and at speed
		//    on loose ground, spray through water, GTA's skid sound for the ground, its crash sounds,
		//    the suspension dipping with a thud on landing, the headlight at night.
		// While CJ is on the bike Gothic's AI for him rests: the plugin sets him on the seat (his
		// collision off), plays his GTA rider animations and the engine, and the camera follows the bike.
		namespace Bike
		{
			struct Path
			{
				int frames = 0;
				zVEC3 points[64];

				float Length() const { return frames > 1 ? (frames - 1) / 25.0f : 0.0f; }

				zVEC3 At(float t) const
				{
					float f = t * 25.0f;
					int i = (int)f;
					if (frames <= 0)
						return zVEC3(0.0f, 0.0f, 0.0f);
					if (i < 0 || f < 0.0f)
						return points[0];
					if (i >= frames - 1)
						return points[frames - 1];
					float a = f - i;
					return points[i] * (1.0f - a) + points[i + 1] * a;
				}
			};

			enum { MOUNT_LEFT, MOUNT_RIGHT, DISMOUNT_LEFT, DISMOUNT_RIGHT };

			// CJBike.ini, in the bike's own axes as Gothic has them (x right, y up, z forward), cm
			struct Data
			{
				bool tried = false, valid = false;
				zVEC3 seat, pivot, axis, hubFront, hubRear, boxMin, boxMax;
				float radiusFront = 34.0f, radiusRear = 32.0f;
				int gears = 5;
				char drive = 'R';
				float maxVelocity = 190.0f, engineAccel = 50.0f, engineInertia = 5.0f, brakeDecel = 14.0f;
				float steeringLock = 35.0f, dragMult = 5.0f, maxLean = 45.0f, speedSteer = 0.6f, enginePitch = 1.0f;
				float mass = 500.0f, turnMass = 200.0f, tractionMult = 1.0f, tractionLoss = 0.9f, tractionBias = 0.5f;
				float fullAnimLean = 43.0f, wheelieAng = 45.0f, stoppieAng = -35.0f;
				float suspensionDroop = 16.0f; // cm, -fSuspensionLowerLimit
				Path paths[4];
			};
			static Data data;

			// cTransmission (gta-reversed cTransmission.cpp), speeds in m per 1/50 s tick
			struct Transmission
			{
				int gears = 5;
				float maxVelocity = 0.0f, maxFlatVelocity = 0.0f, maxReverse = -0.05f, accel = 0.0f, inertia = 1.0f, brake = 0.0f;
				float gearMax[8] = {}, changeUp[8] = {}, changeDown[8] = {};

				// tHandlingData::InitFromData (accel x 0.4), ConvertDataToGameUnits, InitGearRatios
				void Init()
				{
					gears = data.gears < 1 ? 1 : (data.gears > 6 ? 6 : data.gears);
					inertia = data.engineInertia > 0.01f ? data.engineInertia : 0.01f;
					accel = data.engineAccel * 0.4f / 2500.0f;
					brake = data.brakeDecel / 2500.0f;
					float limit = accel / 6.0f, v = data.maxVelocity * 0.277778f / 50.0f;
					while (v > 0.0f)
					{
						v -= 0.01f;
						if (data.dragMult >= 0.01f)
						{
							if (v * v * (data.dragMult / 2.0f) / 1000.0f <= limit)
								break;
							continue;
						}
						float recip = 1.0f / (v * v * data.dragMult + 1.0f);
						if ((recip - 1.0f) * -v <= limit)
							break;
					}
					maxVelocity = v * 1.2f;
					maxFlatVelocity = v;
					maxReverse = -0.05f; // two-wheelers
					accel /= data.drive == '4' ? 4.0f : 2.0f;
					float half = 0.5f * maxVelocity / gears, span = maxVelocity - half;
					for (int i = 1; i <= gears; i++)
					{
						gearMax[i] = i * span / gears + half;
						float difference = gearMax[i] - gearMax[i - 1];
						if (i >= gears)
							changeUp[i] = maxVelocity;
						else
						{
							changeDown[i + 1] = 0.42f * difference + gearMax[i - 1];
							changeUp[i] = 0.6667f * difference + gearMax[i - 1];
						}
					}
					gearMax[0] = maxReverse;
					changeUp[0] = -0.01f;
					changeDown[0] = maxReverse;
					changeDown[1] = -0.01f;
					// bikeSpeed: the gears' speeds and the pull scaled alike (as runSpeed scales CJ's run)
					float scale = settings.bikeSpeed > 0.1f ? settings.bikeSpeed : 0.1f;
					maxVelocity *= scale;
					maxFlatVelocity *= scale;
					accel *= scale;
					for (int i = 1; i <= gears; i++)
					{
						gearMax[i] *= scale;
						changeUp[i] *= scale;
						if (i > 1)
							changeDown[i] *= scale;
					}
				}

				// CalculateDriveAcceleration: the speed change this step (timeStep in ticks)
				float Drive(float gas, int& gear, float velocity, float& ratio, float& smooth, bool onGround, float timeStep)
				{
					bool first = true;
					for (int pass = 0; pass < 16 && velocity >= maxReverse && velocity <= maxVelocity; pass++)
					{
						gear = gear < 0 ? 0 : (gear > gears ? gears : gear);
						bool go = false, lower = false;
						if (velocity > changeUp[gear])
						{
							if (gear == 0 && gas <= 0.0f)
								go = true;
							else
								gear++;
						}
						else
						{
							if (velocity >= changeDown[gear] || gear == 0 || (gear == 1 && gas >= 0.0f))
								go = true;
							lower = true;
						}
						if (go)
						{
							float mult = 4.5f; // reverse
							if (gears == 1)
								mult = 1.0f;
							else if (gear >= 1)
							{
								float g = 1.0f - (gear - 1.0f) / (gears - 1.0f);
								mult = g * g * 3.0f + 1.0f;
							}
							float result = mult * accel * 0.4f * gas * timeStep;
							if (first && onGround)
							{
								float change = maxVelocity / gears / 3.0f, now, range;
								if (gear == 0)
								{
									now = change - velocity;
									range = change - changeDown[0];
								}
								else if (gear == 1)
								{
									now = change + velocity;
									range = change + changeUp[1];
								}
								else
								{
									now = velocity - changeDown[gear];
									range = changeUp[gear] - changeDown[gear];
								}
								float r = range != 0.0f ? now / range : 0.0f;
								float a = 1.0f - (r - ratio) * inertia;
								a = a < 0.1f ? 0.1f : (a > 1.0f ? 1.0f : a);
								ratio = r;
								smooth = a * 0.15f + 0.85f * smooth;
								result *= smooth;
							}
							else if (first)
							{
								ratio += fabsf(gas) / inertia * timeStep * 0.1f;
								ratio = ratio > 1.0f ? 1.0f : ratio;
								smooth = 0.1f;
							}
							float top = gearMax[gear], over;
							if (top >= 0.0f || velocity >= top)
							{
								if (top <= 0.0f || velocity <= top)
									return result;
								over = velocity - top;
							}
							else
								over = top - velocity;
							return result * (1.0f - (over / 0.05f < 1.0f ? over / 0.05f : 1.0f));
						}
						if (lower)
							gear--;
						first = false;
					}
					return 0.0f;
				}

				// the engine's revs within the gear, 0..1 (CAEVehicleAudioEntity's RealRevsRatio)
				float Revs(int gear, float speed) const
				{
					if (gear <= 0 || gear > gears)
						return 0.0f;
					float lo = changeDown[gear], hi = gearMax[gear];
					float r = hi > lo ? (fabsf(speed) - lo) / (hi - lo) : 0.0f;
					return r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r);
				}
			};
			static Transmission transmission;

			enum Phase { PARKED, APPROACH, MOUNTING, RIDING, DISMOUNTING };
			enum WheelState { WHEEL_NORMAL, WHEEL_SPINNING, WHEEL_SKIDDING, WHEEL_FIXED }; // GTA's tWheelState

			struct State
			{
				zCWorld* world = nullptr;
				oCNpc* hero = nullptr;
				zCVob* vobs[4] = {};       // chassis, front (forks, bars, mudguard), front wheel, rear wheel
				zVEC3 pos;                 // the bike's origin
				float yaw = 0.0f;          // heading: 0 along +z, turning right adds
				float pitch = 0.0f, roll = 0.0f; // radians: nose up, lean right
				float speed = 0.0f;        // cm/s along the heading (negative backwards)
				zVEC3 vel = zVEC3(0.0f, 0.0f, 0.0f); // m per tick over the ground (it can slide sideways)
				float omega = 0.0f;        // radians per tick it turns, right positive
				float wheelie = 0.0f;      // radians: front wheel up about the rear hub (+), rear up about the front (-)
				int wheelState[2] = { 0, 0 }; // front, rear (Tyre)
				int groundFront = zMAT_GROUP_UNDEF, groundRear = zMAT_GROUP_UNDEF;
				float accelLean = 0.0f;    // the rider's body pushed back by acceleration, forward by braking
				float airTime = 0.0f;      // seconds in the air
				float vy = 0.0f;           // cm/s up
				bool airborne = false;
				float steerInput = 0.0f, steer = 0.0f, leanInput = 0.0f, gas = 0.0f, brake = 0.0f;
				float spinFront = 0.0f, spinRear = 0.0f;
				int gear = 1;
				float revsRatio = 0.0f, revsSmooth = 1.0f; // the transmission's inertia state
				float fallen = 0.0f;       // the roll it falls over to without a rider
				bool riderless = false;    // CJ bailed out: no throttle, falls over when it stops
				Phase phase = PARKED;
				int path = MOUNT_LEFT;
				float phaseStart = 0.0f;
				zVEC3 approachFrom;
				float approachYaw = 0.0f;
				zVEC3 rootLocal;           // CJ's root node from his vob, in his own axes, standing
				zVEC3 loggedOffset;        // the rider's root offset last written to the log
				bool collStat = true, collDyn = true;
				float lastOff = -10.0f;    // when CJ last got off (F held on does not get him on again)
				float water = -1e30f;      // the water's surface at the bike (none: -1e30)
				float depth = 0.0f;        // how deep the bike stands in it (cm, wheels' bottom to surface)
				bool drowned = false;      // in water up to the seat: the engine is dead, the rider swims
				float squash = 0.0f;       // cm the suspension is pressed in by a landing
				float nextDust = 0.0f, nextSplash = 0.0f; // when the wheels next throw up dust / water
			};
			static State bike;

			struct Basis { zVEC3 right, up, at; };

			static Basis Orientation(float yaw, float pitch, float roll)
			{
				float sy = sinf(yaw), cy = cosf(yaw);
				zVEC3 at0(sy, 0.0f, cy), right0(cy, 0.0f, -sy), up0(0.0f, 1.0f, 0.0f);
				Basis b;
				b.at = at0 * cosf(pitch) + up0 * sinf(pitch);
				zVEC3 up = up0 * cosf(pitch) - at0 * sinf(pitch);
				b.up = up * cosf(roll) + right0 * sinf(roll);
				b.right = right0 * cosf(roll) - up * sinf(roll);
				return b;
			}

			static zVEC3 ToWorld(const Basis& b, const zVEC3& local)
			{
				return b.right * local[0] + b.up * local[1] + b.at * local[2];
			}

			// Rodrigues: v turned by `angle` about the unit `axis`
			static zVEC3 Rotate(const zVEC3& v, const zVEC3& axis, float angle)
			{
				float c = cosf(angle), s = sinf(angle);
				return v * c + Cross(axis, v) * s + axis * (axis.Dot(v) * (1.0f - c));
			}

			static zMAT4 Trafo(const Basis& b, const zVEC3& pos)
			{
				zMAT4 m = zMAT4::GetIdentity();
				m.SetRightVector(b.right);
				m.SetUpVector(b.up);
				m.SetAtVector(b.at);
				m.SetTranslation(pos);
				return m;
			}

			static float Yaw(const zVEC3& forward) { return atan2f(forward[0], forward[2]); }

			static bool Busy() { return bike.phase != PARKED; }

			// --- CJBike.ini ----------------------------------------------------------------------
			static bool ReadIni(const char* section, const char* key, char* out, int size)
			{
				static char path[MAX_PATH] = "";
				if (!path[0])
				{
					GetModuleFileNameA(nullptr, path, MAX_PATH);
					char* slash = strrchr(path, '\\');
					if (slash)
						strcpy_s(slash + 1, MAX_PATH - (slash + 1 - path), "CJBike.ini");
				}
				return GetPrivateProfileStringA(section, key, "", out, size, path) > 0;
			}

			static float ReadFloat(const char* section, const char* key, float fallback)
			{
				char buf[64];
				return ReadIni(section, key, buf, sizeof(buf)) ? (float)atof(buf) : fallback;
			}

			// GTA metres (x right, y forward, z up) -> Gothic cm (x right, y up, z forward)
			static zVEC3 FromGta(float x, float y, float z) { return zVEC3(x * 100.0f, z * 100.0f, y * 100.0f); }

			static zVEC3 ReadPoint(const char* key)
			{
				char buf[128];
				float x = 0.0f, y = 0.0f, z = 0.0f;
				if (ReadIni("BIKE", key, buf, sizeof(buf)))
					sscanf_s(buf, "%f,%f,%f", &x, &y, &z);
				return FromGta(x, y, z);
			}

			static void ReadPath(const char* key, Path& path)
			{
				static char buf[16384];
				path.frames = 0;
				if (!ReadIni("PATHS", key, buf, sizeof(buf)))
					return;
				char* next = nullptr;
				char* token = strtok_s(buf, ";", &next); // the frame count
				for (token = strtok_s(nullptr, ";", &next); token && path.frames < 64; token = strtok_s(nullptr, ";", &next))
				{
					float x = 0.0f, y = 0.0f, z = 0.0f;
					if (sscanf_s(token, "%f,%f,%f", &x, &y, &z) == 3)
						path.points[path.frames++] = FromGta(x, y, z);
				}
			}

			static bool Load()
			{
				if (data.tried)
					return data.valid;
				data.tried = true;
				char name[64];
				if (!ReadIni("BIKE", "name", name, sizeof(name)))
				{
					Log("bike: no system\\CJBike.ini (make_cj_bike.py writes it)");
					return false;
				}
				data.seat = ReadPoint("seat");
				data.pivot = ReadPoint("pivot");
				data.axis = ReadPoint("steerAxis");
				data.axis.Normalize();
				data.hubFront = ReadPoint("hubFront");
				data.hubRear = ReadPoint("hubRear");
				data.boxMin = ReadPoint("boxMin");
				data.boxMax = ReadPoint("boxMax");
				data.radiusFront = ReadFloat("BIKE", "radiusFront", 0.34f) * 100.0f;
				data.radiusRear = ReadFloat("BIKE", "radiusRear", 0.32f) * 100.0f;
				data.gears = (int)ReadFloat("HANDLING", "gears", 5.0f);
				data.maxVelocity = ReadFloat("HANDLING", "maxVelocity", data.maxVelocity);
				data.engineAccel = ReadFloat("HANDLING", "engineAccel", data.engineAccel);
				data.engineInertia = ReadFloat("HANDLING", "engineInertia", data.engineInertia);
				data.brakeDecel = ReadFloat("HANDLING", "brakeDecel", data.brakeDecel);
				data.steeringLock = ReadFloat("HANDLING", "steeringLock", data.steeringLock);
				data.dragMult = ReadFloat("HANDLING", "dragMult", data.dragMult);
				char drive[8];
				if (ReadIni("HANDLING", "driveType", drive, sizeof(drive)))
					data.drive = drive[0];
				data.mass = ReadFloat("HANDLING", "mass", data.mass);
				data.turnMass = ReadFloat("HANDLING", "turnMass", data.turnMass);
				data.tractionMult = ReadFloat("HANDLING", "tractionMult", data.tractionMult);
				data.tractionLoss = ReadFloat("HANDLING", "tractionLoss", data.tractionLoss);
				data.tractionBias = ReadFloat("HANDLING", "tractionBias", data.tractionBias);
				data.fullAnimLean = ReadFloat("BIKEHANDLING", "fullAnimLean", data.fullAnimLean);
				data.suspensionDroop = fabsf(ReadFloat("HANDLING", "suspensionLower", -0.16f)) * 100.0f;
				data.wheelieAng = ReadFloat("BIKEHANDLING", "wheelieAng", data.wheelieAng);
				data.stoppieAng = ReadFloat("BIKEHANDLING", "stoppieAng", data.stoppieAng);
				data.maxLean = ReadFloat("BIKEHANDLING", "maxLean", data.maxLean);
				data.speedSteer = ReadFloat("BIKEHANDLING", "speedSteer", data.speedSteer);
				data.enginePitch = ReadFloat("SOUND", "enginePitch", data.enginePitch);
				static const char* const PATHS[4] = { "MountL", "MountR", "DismountL", "DismountR" };
				data.valid = true;
				for (int i = 0; i < 4; i++)
				{
					ReadPath(PATHS[i], data.paths[i]);
					data.valid = data.valid && data.paths[i].frames > 1;
				}
				transmission.Init();
				Log("bike: %s, wheelbase %.0f cm, wheels %.0f / %.0f cm, top speed %.0f km/h on the flat, gears to %.0f %.0f %.0f %.0f %.0f km/h%s",
					name, data.hubFront[2] - data.hubRear[2], data.radiusFront, data.radiusRear, transmission.maxFlatVelocity * 180.0f,
					transmission.gearMax[1] * 180.0f, transmission.gearMax[2] * 180.0f, transmission.gearMax[3] * 180.0f,
					transmission.gearMax[4] * 180.0f, transmission.gearMax[5] * 180.0f, data.valid ? "" : " (paths missing)");
				return data.valid;
			}

			// --- the bike in the world -----------------------------------------------------------
			static const char* const PART_VISUALS[4] = { "CJ_BIKE_CHASSIS.3DS", "CJ_BIKE_FRONT.3DS", "CJ_BIKE_WHEELF.3DS", "CJ_BIKE_WHEELR.3DS" };

			static void RemoveVobs()
			{
				for (zCVob*& vob : bike.vobs)
				{
					if (!vob)
						continue;
					if (zCWorld* world = vob->GetHomeWorld())
						world->RemoveVob(vob);
					vob->Release();
					vob = nullptr;
				}
			}

			static int TraceFlags()
			{
				return zTRACERAY_STAT_POLY | zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER;
			}

			// The ground under a point: a ray from `above` cm over it down to `below` under it. Water is
			// no ground: under it the ray finds the bed (WaterAt tells how deep it is). Earlier the ray
			// asked for water polygons, so the bike could ride across a lake.
			static bool Ground(zCWorld* world, const zVEC3& point, float above, float below, float& y, int* group = nullptr)
			{
				zVEC3 from = point + zVEC3(0.0f, above, 0.0f);
				float reach = above + below;
				for (int pass = 0; pass < 2; pass++)
				{
					if (reach <= 0.0f || !world->TraceRayNearestHit(from, zVEC3(0.0f, -reach, 0.0f), bike.hero, TraceFlags())
						|| !world->traceRayReport.foundHit)
						return false;
					zCPolygon* poly = world->traceRayReport.foundPoly;
					int hitGroup = poly && poly->material ? poly->material->matGroup : zMAT_GROUP_UNDEF;
					float hitY = world->traceRayReport.foundIntersection[1];
					if (hitGroup == zMAT_GROUP_WATER && pass == 0) // the surface after all: on through it
					{
						reach -= from[1] - hitY + 1.0f;
						from[1] = hitY - 1.0f;
						continue;
					}
					y = hitY;
					if (group)
						*group = hitGroup;
					return true;
				}
				return false;
			}

			// The water's surface over (or under) a point, -1e30 if there is none.
			static float WaterAt(zCWorld* world, const zVEC3& point)
			{
				zVEC3 surface;
				return WaterHit(world, point + zVEC3(0.0f, 400.0f, 0.0f), zVEC3(0.0f, -900.0f, 0.0f), bike.hero, surface) ? surface[1] : -1e30f;
			}

			// The bike as drawn: its pose on the ground turned up about the rear hub in a wheelie (down
			// about the front hub in a stoppie), and leaning (or lying) about where its tyres touch.
			static void Pose(Basis& b, zVEC3& origin)
			{
				float pitch = bike.pitch + bike.wheelie;
				Basis ground = Orientation(bike.yaw, bike.pitch, 0.0f), up = Orientation(bike.yaw, pitch, 0.0f);
				origin = bike.pos;
				if (bike.wheelie != 0.0f)
				{
					const zVEC3& hub = bike.wheelie > 0.0f ? data.hubRear : data.hubFront;
					origin += ToWorld(ground, hub) - ToWorld(up, hub);
				}
				b = Orientation(bike.yaw, pitch, bike.roll + bike.fallen);
				float height = data.radiusRear - data.hubRear[1]; // the origin above the ground
				origin += (b.up - up.up) * height;
				origin -= b.up * bike.squash; // the suspension pressed in (the wheels stay down: PlaceParts)
			}

			// The wheels and steering as GTA's CBike::PreRender sets them: the front assembly turns
			// about the fork's axis (from the front hub up to the steering pivot), the wheels spin.
			static void PlaceParts()
			{
				if (!bike.vobs[0])
					return;
				Basis b;
				zVEC3 origin;
				Pose(b, origin);
				bike.vobs[0]->SetTrafoObjToWorld(Trafo(b, origin));
				Basis steer;
				steer.right = ToWorld(b, Rotate(zVEC3(1.0f, 0.0f, 0.0f), data.axis, bike.steer));
				steer.up = ToWorld(b, Rotate(zVEC3(0.0f, 1.0f, 0.0f), data.axis, bike.steer));
				steer.at = ToWorld(b, Rotate(zVEC3(0.0f, 0.0f, 1.0f), data.axis, bike.steer));
				zVEC3 pivot = origin + ToWorld(b, data.pivot);
				if (bike.vobs[1])
					bike.vobs[1]->SetTrafoObjToWorld(Trafo(steer, pivot));
				auto spun = [](const Basis& base, float angle)
				{
					zVEC3 axis(1.0f, 0.0f, 0.0f);
					Basis w;
					w.right = ToWorld(base, axis);
					w.up = ToWorld(base, Rotate(zVEC3(0.0f, 1.0f, 0.0f), axis, angle));
					w.at = ToWorld(base, Rotate(zVEC3(0.0f, 0.0f, 1.0f), axis, angle));
					return w;
				};
				zVEC3 sprung = b.up * bike.squash; // the wheels where the ground holds them
				if (bike.vobs[2])
				{
					zVEC3 hub = pivot + ToWorld(steer, data.hubFront - data.pivot) + sprung;
					bike.vobs[2]->SetTrafoObjToWorld(Trafo(spun(steer, bike.spinFront), hub));
				}
				if (bike.vobs[3])
					bike.vobs[3]->SetTrafoObjToWorld(Trafo(spun(b, bike.spinRear), origin + ToWorld(b, data.hubRear) + sprung));
			}

			// Sets the bike on the ground at its x/z: height and pitch from both wheels.
			static void Settle(zCWorld* world)
			{
				Basis b = Orientation(bike.yaw, 0.0f, 0.0f);
				float front = 0.0f, rear = 0.0f;
				zVEC3 f = bike.pos + ToWorld(b, data.hubFront), r = bike.pos + ToWorld(b, data.hubRear);
				bool hasFront = Ground(world, f, 200.0f, 500.0f, front), hasRear = Ground(world, r, 200.0f, 500.0f, rear);
				if (!hasFront && !hasRear)
					return;
				front = hasFront ? front : rear;
				rear = hasRear ? rear : front;
				float base = data.hubFront[2] - data.hubRear[2];
				bike.pitch = atan2f(front + data.radiusFront - rear - data.radiusRear, base) - atan2f(data.hubFront[1] - data.hubRear[1], base);
				bike.pos[1] = rear + data.radiusRear - (data.hubRear[1] * cosf(bike.pitch) + data.hubRear[2] * sinf(bike.pitch));
				bike.vy = 0.0f;
				bike.airborne = false;
			}

			static void StopEffects();
			static void Headlight(bool on);

			// Puts the bike on CJ's right, facing where he faces.
			static void Spawn(oCNpc* hero, zCWorld* world)
			{
				RemoveVobs();
				StopEffects();
				bike = State();
				bike.world = world;
				bike.hero = hero;
				zVEC3 at = Flat(hero->GetAtVectorWorld());
				bike.yaw = Yaw(at);
				bike.pos = hero->GetPositionWorld() + Orientation(bike.yaw, 0.0f, 0.0f).right * 180.0f + at * 60.0f;
				Settle(world);
				for (int i = 0; i < 4; i++)
				{
					zCVob* vob = new zCVob();
					vob->dontWriteIntoArchive = TRUE;
					vob->SetCollDetStat(FALSE);
					vob->SetCollDetDyn(FALSE);
					vob->SetVisual(zSTRING(PART_VISUALS[i]));
					world->AddVob(vob);
					vob->SetCollDetStat(FALSE); // (again: no collision with what it shows)
					vob->SetCollDetDyn(FALSE);
					bike.vobs[i] = vob; // the plugin keeps a reference
				}
				PlaceParts();
				Log("bike: placed next to CJ at %.0f %.0f %.0f", bike.pos[0], bike.pos[1], bike.pos[2]);
			}

			// --- engine sound --------------------------------------------------------------------
			// GTA's player engine (CAEVehicleAudioEntity::ProcessPlayerVehicleEngine) plays the idle
			// loop standing, the revs under throttle with the pitch rising through each gear
			// (Rev: 0.9 .. 1.5), and the cruise loop rolling off the throttle (Crz: 0.925 .. 1.25).
			static const char* const ENGINE_SOUNDS[3] = { "CJ_BIKE_IDLE.WAV", "CJ_BIKE_REV.WAV", "CJ_BIKE_CRUISE.WAV" };
			static zCSoundFX* engineFx[3] = {};
			static int engineHandle[3] = {};
			static float engineVolume[3] = {};

			static void Engine(bool on, float dt)
			{
				if (!zsound || !bike.vobs[0])
					return;
				float speedRatio = transmission.maxVelocity > 0.0f ? fabsf(bike.speed / 5000.0f) / transmission.maxVelocity : 0.0f;
				speedRatio = speedRatio > 1.0f ? 1.0f : speedRatio;
				float revs = bike.wheelState[1] == WHEEL_SPINNING ? 1.0f : transmission.Revs(bike.gear, bike.speed / 5000.0f);
				bool moving = fabsf(bike.speed) > 100.0f;
				float want[3] = { !moving && bike.gas <= 0.0f ? 0.7f : 0.0f, bike.gas > 0.0f ? 0.9f : 0.0f, moving && bike.gas <= 0.0f ? 0.7f : 0.0f };
				float pitch[3] = { 0.85f + 0.15f * speedRatio, 0.9f + 0.6f * (bike.airborne ? 1.0f : revs), 0.925f + 0.325f * speedRatio };
				for (int i = 0; i < 3; i++)
				{
					if (!on)
					{
						if (engineHandle[i])
							zsound->StopSound(engineHandle[i]);
						engineHandle[i] = 0;
						engineVolume[i] = 0.0f;
						continue;
					}
					engineVolume[i] += (want[i] - engineVolume[i]) * (dt / 0.15f < 1.0f ? dt / 0.15f : 1.0f); // cross-fade
					if (!engineFx[i])
						engineFx[i] = zsound->LoadSoundFX(zSTRING(ENGINE_SOUNDS[i]));
					if (!engineFx[i])
						continue;
					zCSoundSystem::zTSound3DParams params;
					params.SetDefaults();
					params.loopType = zCSoundSystem::zSND_LOOPING_ENABLED;
					params.volume = engineVolume[i];
					params.radius = 4000.0f;
					params.pitchOffset = 12.0f * log2f(pitch[i] * data.enginePitch);
					if (!engineHandle[i] || !zsound->IsSoundActive(engineHandle[i]))
						engineHandle[i] = zsound->PlaySound3D(engineFx[i], bike.vobs[0], zSND_SLOT_NONE, &params);
					else
						zsound->UpdateSound3D(engineHandle[i], &params);
				}
			}

			// --- physics --------------------------------------------------------------------------
			struct Hit { oCNpc* npc; float time; };
			static Hit hits[8];

			// People the bike runs into: knocked down, hurt by its speed (as a GTA car hit: at 10 m/s
			// 40 % of their health, at 20 m/s more than all of it).
			static void HitPeople(oCNpc* hero, const Basis& b)
			{
				float speed = fabsf(bike.speed) / 100.0f; // m/s
				if (speed < 3.0f)
					return;
				oCWorld* game = ogame ? ogame->GetGameWorld() : nullptr;
				zVEC3 front = bike.pos + b.at * (bike.speed > 0.0f ? data.boxMax[2] - 30.0f : data.boxMin[2] + 30.0f);
				float now = Now();
				for (zCListSort<oCNpc>* node = game && game->voblist_npcs ? game->voblist_npcs->next : nullptr; node; node = node->next)
				{
					oCNpc* npc = node->data;
					if (!npc || npc == hero || npc->IsDead())
						continue;
					zVEC3 d = npc->GetPositionWorld() - front;
					if (fabsf(d[1]) > 150.0f || d[0] * d[0] + d[2] * d[2] > 70.0f * 70.0f)
						continue;
					Hit* slot = &hits[0];
					bool recent = false;
					for (Hit& h : hits)
					{
						if (h.npc == npc)
						{
							slot = &h;
							recent = now - h.time < 1.0f;
							break;
						}
						if (h.time < slot->time)
							slot = &h;
					}
					if (recent)
						continue;
					slot->npc = npc;
					slot->time = now;
					float share = speed * speed * 0.004f;
					share = share > 1.5f ? 1.5f : share;
					// knocked down and thrown the way the bike goes, through the blast's damage path
					// (OnDamage_Hit hook: blunt on top of the protection, Gothic's fly damage); people
					// are knocked out by it rather than killed, as by any blunt hit in Gothic
					zVEC3 dir = Flat(bike.vel) + zVEC3(0.0f, 0.5f, 0.0f);
					dir.Normalize();
					blastTarget = npc;
					blastDamage = share * npc->attribute[NPC_ATR_HITPOINTSMAX];
					blastDir = dir;
					blastLethal = false;
					npc->OnDamage(hero, hero, blastDamage, oEDamageType_Blunt | oEDamageType_Fly, npc->GetPositionWorld());
					blastTarget = nullptr;
					PlayAt("CJ_BIKE_HITPED.WAV", npc); // GTA's COLCARPED
					bike.speed *= 0.75f;
					bike.vel = bike.vel * 0.75f;
					Log("bike: ran into %s at %.0f km/h (%.0f damage)", npc->GetInstanceName().ToChar(), speed * 3.6f, share * npc->attribute[NPC_ATR_HITPOINTSMAX]);
				}
			}

			static void Eject(oCNpc* hero, const char* why, float carry = 0.7f, float hurt = 0.0f);
			static Loop skidLoop, waterLoop;
			static const char* const CRASH_SOUNDS[5] = { "CJ_BIKE_CRASH1.WAV", "CJ_BIKE_CRASH2.WAV", "CJ_BIKE_CRASH3.WAV",
				"CJ_BIKE_CRASH4.WAV", "CJ_BIKE_CRASH5.WAV" };

			// GTA's bike crash (SND_GENRL_COLLISIONS_BIKE_CRASH1-5), one of the five, at the bike
			static void CrashSound(float volume)
			{
				static int next = 0;
				next = (next + 1 + (int)(Now() * 1000.0f) % 3) % 5;
				PlayAt(CRASH_SOUNDS[next], bike.vobs[0], volume > 1.0f ? 1.0f : volume, 4000.0f);
			}

			// GTA's tyre for one wheel (CVehicle::ProcessWheel, gta-reversed Vehicle.cpp; GTA's
			// CVehicle::ProcessBikeWheel is not reverse-engineered): in m per 1/50 s tick, the change it
			// makes to its contact point's speed along the wheel (fwd) and across it (right). Within
			// the adhesion the wheel cancels its share of the sideways slip and drives or brakes; past
			// it the wheel spins (too much thrust) or skids, and a wheel that skidded last time has
			// fTractionLoss of its grip, so a slide carries on until the speeds settle again.
			// For the bike the grip across the wheel is SIDE_GRIP of GTA's (along it, GTA's own), so it
			// corners about as hard as its fMaxLean holds (some 1.2 g on dirt) without spinning its rear
			// wheel at every start; a spinning, skidding or locked wheel keeps only that lower grip in
			// any direction, so a handbrake turn slides instead of stopping the bike dead.
			static void Tyre(const zVEC3& wheelFwd, const zVEC3& wheelRight, const zVEC3& contactSpeed, int wheelsOnGround,
				float thrust, float brake, float adhesion, float ts, int& state, float& fwdOut, float& rightOut)
			{
				const float TRAC_FRAC_LIMIT = 0.3f, ALREADY_SPINNING_LOSS = 0.2f; // WS_TRAC_FRAC_LIMIT, WS_ALREADY_SPINNING_LOSS
				const float SIDE_GRIP = 0.3f;
				float contactFwd = wheelFwd.Dot(contactSpeed), contactRight = wheelRight.Dot(contactSpeed);
				bool braking = brake != 0.0f, driving = !braking && thrust != 0.0f, alreadySkidding = false;
				adhesion *= ts;
				fwdOut = rightOut = 0.0f;
				if (adhesion <= 0.0f)
					return;
				if (state != WHEEL_NORMAL)
				{
					alreadySkidding = true;
					adhesion *= data.tractionLoss;
					if (state == WHEEL_SPINNING)
						adhesion *= 1.0f - fabsf(bike.gas) * ALREADY_SPINNING_LOSS;
				}
				state = WHEEL_NORMAL;
				float side = adhesion * SIDE_GRIP;
				float fwd = 0.0f, right = contactRight != 0.0f ? -contactRight / wheelsOnGround : 0.0f;
				if (driving)
				{
					fwd = thrust;
					right = right < -side ? -side : (right > side ? side : right);
				}
				else if (contactFwd != 0.0f)
				{
					fwd = -contactFwd / wheelsOnGround;
					if (!braking && fabsf(bike.gas) < 0.01f) // rolling: a bike's wheel friction (0.9 * 0.6 / (mass + 200))
						brake = 0.9f * 0.6f / (data.mass + 200.0f) * ts;
					if (brake > adhesion)
					{
						if (fabsf(contactFwd) > 0.005f)
							state = WHEEL_FIXED; // locked: it slides
					}
					else
						fwd = fwd < -brake ? -brake : (fwd > brake ? brake : fwd);
				}
				float grip = (fwd / adhesion) * (fwd / adhesion) + (right / side) * (right / side); // within the grip: <= 1
				if (grip > 1.0f || state == WHEEL_FIXED)
				{
					if (state != WHEEL_FIXED)
						state = driving && TRAC_FRAC_LIMIT * adhesion < fabsf(fwd) ? WHEEL_SPINNING : WHEEL_SKIDDING;
					float loss = data.tractionLoss;
					if (alreadySkidding)
						loss = 1.0f;
					else if (state == WHEEL_SPINNING)
						loss *= 1.0f - fabsf(bike.gas) * ALREADY_SPINNING_LOSS;
					float l = sqrtf(fwd * fwd + right * right), limit = side * loss;
					if (l > limit)
					{
						fwd *= limit / l;
						right *= limit / l;
					}
				}
				fwdOut = fwd;
				rightOut = right;
			}

			// GTA's adhesive limits of a tyre on each kind of ground (data\surface.dat, rubber against
			// hard 3.6, road 4.5, loose 3.2, sand 3.0, wet 2.8): Gothic's stone, metal and wood count as
			// hard ground, water as wet, the rest (earth, grass, unnamed) as loose.
			static float Adhesive(int group)
			{
				switch (group)
				{
				case zMAT_GROUP_STONE: case zMAT_GROUP_METAL: case zMAT_GROUP_WOOD: return 3.6f;
				case zMAT_GROUP_WATER: return 2.8f;
				default: return 3.2f;
				}
			}

			// A tyre's speed change at `r` (m from the centre) acting on the bike as CPhysical's
			// ApplyMoveForce and ApplyTurnForce take it: all of it on the speed, and the turn its
			// impulse (with GetMass, the bike's mass as felt at that point) gives about the vertical.
			static void Push(const zVEC3& r, const zVEC3& change)
			{
				float speed = change.Length();
				if (speed <= 0.0f)
					return;
				zVEC3 dir = change * (1.0f / speed);
				bike.vel += change;
				float arm = r[2] * dir[0] - r[0] * dir[2]; // (r x dir).y
				float mass = 1.0f / (arm * arm / data.turnMass + 1.0f / data.mass);
				bike.omega += arm * speed * mass / data.turnMass;
			}

			// One step of the bike as a rigid body like GTA's vehicles: its speed over the ground
			// (vel, m per tick), its turning (omega, radians per tick) and GTA's tyre on each wheel on
			// the ground. `rider`: CJ's inputs count.
			static void Step(oCNpc* hero, float dt, bool rider)
			{
				zCWorld* world = bike.world;
				if (!world || dt <= 0.0f)
					return;
				float ts = dt * 50.0f; // GTA ticks
				Basis flat = Orientation(bike.yaw, 0.0f, 0.0f);
				float v = bike.vel.Dot(flat.at); // m per tick along the bike
				// CBike::ProcessControlInputs (GTA III/VC): gas or brake by the way the bike moves
				float pedal = rider && !bike.drowned ? (KeyDown(KEY_W) ? 1.0f : 0.0f) - (KeyDown(KEY_S) ? 1.0f : 0.0f) : 0.0f;
				if (fabsf(v) < 0.01f || (v > 0.0f) == (pedal >= 0.0f) || pedal == 0.0f)
				{
					bike.gas = pedal;
					bike.brake = 0.0f;
				}
				else
				{
					bike.gas = 0.0f;
					bike.brake = fabsf(pedal);
				}
				bool handbrake = rider && KeyDown(KEY_SPACE);
				if (!rider)
					bike.brake = bike.riderless ? 0.3f : 1.0f;
				float steerTarget = rider ? (KeyDown(KEY_D) ? 1.0f : 0.0f) - (KeyDown(KEY_A) ? 1.0f : 0.0f) : 0.0f;
				float k = 0.2f * ts < 1.0f ? 0.2f * ts : 1.0f;
				bike.steerInput += (steerTarget - bike.steerInput) * k;
				float leanTarget = rider ? ((KeyDown(KEY_UP) || KeyDown(KEY_NUMPAD8)) ? 1.0f : 0.0f) - ((KeyDown(KEY_DOWN) || KeyDown(KEY_NUMPAD2)) ? 1.0f : 0.0f) : 0.0f;
				bike.leanInput += (leanTarget - bike.leanInput) * k;
				float speedRatio = transmission.maxFlatVelocity > 0.0f ? fabsf(v) / transmission.maxFlatVelocity : 0.0f;
				speedRatio = speedRatio > 1.0f ? 1.0f : speedRatio;
				bike.steer = bike.steerInput * data.steeringLock * PI / 180.0f * (1.0f - data.speedSteer * speedRatio);

				// wheelies (leaning back on the throttle; a little on a full-throttle start) and stoppies
				// (leaning forward on the brake), up to most of fWheelieAng / fStoppieAng
				float wheelieTarget = 0.0f;
				if (!bike.airborne && bike.gas > 0.0f && v > -0.01f)
				{
					if (bike.leanInput < -0.1f)
						wheelieTarget = -bike.leanInput * bike.gas * data.wheelieAng * 0.8f * PI / 180.0f;
					else if (bike.gear <= 1 && bike.gas > 0.9f && bike.leanInput < 0.1f && v * 50.0f < 4.0f)
						wheelieTarget = 0.06f; // a full-throttle start lifts the front a little (the tyre still steers)
				}
				else if (!bike.airborne && bike.brake > 0.5f && bike.leanInput > 0.1f && v * 50.0f > 3.0f)
					wheelieTarget = -bike.leanInput * fabsf(data.stoppieAng) * 0.8f * PI / 180.0f;
				float rate = (fabsf(wheelieTarget) > fabsf(bike.wheelie) ? 1.2f : 2.0f) * dt;
				bike.wheelie += wheelieTarget > bike.wheelie ? (wheelieTarget - bike.wheelie < rate ? wheelieTarget - bike.wheelie : rate)
					: -(bike.wheelie - wheelieTarget < rate ? bike.wheelie - wheelieTarget : rate);

				// the tyres
				const float LIFTED = 0.1f; // radians: past it the front (or rear) wheel is off the ground
				bool frontDown = !bike.airborne && bike.wheelie < LIFTED, rearDown = !bike.airborne && bike.wheelie > -LIFTED;
				int onGround = (frontDown ? 1 : 0) + (rearDown ? 1 : 0);
				float thrust = transmission.Drive(bike.gas, bike.gear, v, bike.revsRatio, bike.revsSmooth, rearDown, ts);
				if (onGround > 0)
				{
					// GTA's grip (0.004 * fTractionMultiplier, CAutomobile::ProcessControl) shared by the two
					// wheels; bikeGrip scales it
					float traction = data.tractionMult / 250.0f * 0.5f * settings.bikeGrip;
					zVEC3 rF = flat.at * (data.hubFront[2] / 100.0f), rR = flat.at * (data.hubRear[2] / 100.0f);
					zVEC3 spin(0.0f, bike.omega, 0.0f);
					zVEC3 cF = bike.vel + Cross(spin, rF), cR = bike.vel + Cross(spin, rR);
					float s = sinf(bike.steer), c = cosf(bike.steer);
					zVEC3 fwdF = flat.at * c + flat.right * s, rightF = flat.right * c - flat.at * s;
					float brake = transmission.brake * bike.brake * ts * 0.5f; // fBrakeDeceleration between the wheels (bias 0.5)
					float fwd = 0.0f, right = 0.0f;
					zVEC3 changeF, changeR;
					if (frontDown)
					{
						Tyre(fwdF, rightF, cF, onGround, 0.0f, brake, Adhesive(bike.groundFront) * traction * 2.0f * data.tractionBias,
							ts, bike.wheelState[0], fwd, right);
						changeF = fwdF * fwd + rightF * right;
					}
					if (rearDown)
					{
						Tyre(flat.at, flat.right, cR, onGround, thrust, handbrake ? 20000.0f : brake, // the handbrake locks it
							Adhesive(bike.groundRear) * traction * (2.0f - 2.0f * data.tractionBias), ts, bike.wheelState[1], fwd, right);
						changeR = flat.at * fwd + flat.right * right;
					}
					if (frontDown)
						Push(rF, changeF);
					if (rearDown)
						Push(rR, changeR);
					// GTA's gravity along the slope: hills slow the bike and it rolls back down them
					bike.vel += flat.at * (-0.008f * sinf(bike.pitch) * ts);
					// in a wheelie the rider steers by leaning (GTA's fWheelieSteer)
					if (!frontDown)
					{
						float want = bike.steerInput * 0.6f / 50.0f; // up to 0.6 radians a second
						bike.omega += (want - bike.omega) * (0.1f * ts < 1.0f ? 0.1f * ts : 1.0f);
					}
				}
				float drag = bike.vel.Length() * data.dragMult / 2000.0f; // CPhysical::ApplyAirResistance
				bike.vel = bike.vel * powf(1.0f - (drag < 0.5f ? drag : 0.5f), ts);
				bike.omega *= powf(0.99f, ts);
				if (!rider && onGround == 2 && bike.vel.Length() < 0.0004f && fabsf(bike.omega) < 0.0004f)
					bike.vel = zVEC3(0.0f, 0.0f, 0.0f), bike.omega = 0.0f; // standing still
				bike.yaw += bike.omega * ts;
				float before = v;
				flat = Orientation(bike.yaw, 0.0f, 0.0f);
				v = bike.vel.Dot(flat.at);
				bike.speed = v * 5000.0f;
				float accel = dt > 0.0f ? (v - before) * 50.0f / dt : 0.0f; // m/s^2 along the bike
				bike.accelLean += ((accel < -12.0f ? 1.0f : accel > 12.0f ? -1.0f : -accel / 12.0f) * 0.4f - bike.accelLean) * (1.0f - expf(-dt * 6.0f));

				// the bike leans into its turn as far as the turn needs (up to fMaxLean)
				float maxLean = data.maxLean * PI / 180.0f;
				float lean = bike.airborne ? bike.roll : atanf(bike.omega * 50.0f * v * 50.0f / 9.81f);
				lean = lean > maxLean ? maxLean : (lean < -maxLean ? -maxLean : lean);
				if (fabsf(bike.speed) < 150.0f)
					lean *= fabsf(bike.speed) / 150.0f;
				bike.roll += (lean - bike.roll) * (1.0f - expf(-dt * 8.0f));
				bike.spinFront += bike.speed * dt / data.radiusFront;
				bike.spinRear += (bike.speed + (bike.wheelState[1] == WHEEL_SPINNING ? 1500.0f : 0.0f)) * dt / data.radiusRear;

				// walls: rays ahead (or behind) at two heights
				zVEC3 move = bike.vel * (5000.0f * dt);
				float moved = move.Length();
				if (moved > 0.01f)
				{
					zVEC3 dir = move * (1.0f / moved);
					float reach = dir.Dot(flat.at) >= 0.0f ? data.boxMax[2] : -data.boxMin[2];
					for (float h : { 0.0f, 45.0f })
					{
						zVEC3 from = bike.pos + zVEC3(0.0f, h, 0.0f);
						if (world->TraceRayNearestHit(from, dir * (reach + moved), hero, TraceFlags())
							&& world->traceRayReport.foundHit && world->traceRayReport.foundPolyNormal[1] < 0.6f)
						{
							float impact = moved / dt;
							zVEC3 at = world->traceRayReport.foundIntersection, normal = world->traceRayReport.foundPolyNormal;
							zCPolygon* poly = world->traceRayReport.foundPoly;
							int group = poly && poly->material ? poly->material->matGroup : zMAT_GROUP_UNDEF;
							if (impact > 300.0f) // (resting against it is no news)
								Log("bike: hit a wall at %.0f km/h", impact * 0.036f);
							if (impact > 500.0f) // GTA's crash, dust off the wall, sparks off stone and metal
							{
								CrashSound(impact / 2000.0f);
								Emit(world, "PFX_DUST", at, normal, 0.08f, 1.5f, 1.0f);
								if (group == zMAT_GROUP_STONE || group == zMAT_GROUP_METAL)
									Emit(world, "PFX_METALSPARKS", at, normal, 0.12f);
							}
							if (rider && impact > 1500.0f)
								Eject(hero, "crash", 0.3f, impact / 100.0f); // over the bars, before the bike rebounds
							bike.vel = bike.vel * -0.15f;
							bike.omega *= 0.5f;
							bike.speed = bike.vel.Dot(flat.at) * 5000.0f;
							move = zVEC3(0.0f, 0.0f, 0.0f);
							break;
						}
					}
				}
				zVEC3 oldPos = bike.pos;
				bike.pos += move;

				// ground under the wheels
				float base = data.hubFront[2] - data.hubRear[2];
				Basis b = Orientation(bike.yaw, bike.pitch, 0.0f);
				zVEC3 hubF = bike.pos + ToWorld(b, data.hubFront), hubR = bike.pos + ToWorld(b, data.hubRear);
				float groundF = 0.0f, groundR = 0.0f;
				bool hasF = Ground(world, hubF, 50.0f, 400.0f, groundF, &bike.groundFront), hasR = Ground(world, hubR, 50.0f, 400.0f, groundR, &bike.groundRear);
				// water: how deep the bike stands in it (its wheels' bottom under the surface); in it the
				// bike sinks slowly instead of falling (GTA's buoyancy, fPercentSubmerged)
				float height = data.radiusRear - data.hubRear[1]; // the origin above the wheels' bottom
				float lastDepth = bike.depth;
				bike.water = WaterAt(world, (hubF + hubR) * 0.5f);
				bike.depth = bike.water - (bike.pos[1] - height);
				bool wet = bike.depth > 0.0f;
				if (wet && lastDepth <= 0.0f && bike.airborne && bike.vy < -300.0f) // it falls into the water
					Splash(world, zVEC3(bike.pos[0], bike.water, bike.pos[2]), 4.0f, 0.3f);
				float gravity = wet ? GTA_GRAVITY * 0.2f : GTA_GRAVITY;
				float ballistic = bike.pos[1] + bike.vy * dt - 0.5f * gravity * dt * dt;
				float droop = data.suspensionDroop; // how far the wheels reach down (fSuspensionLowerLimit)
				bool grounded = false;
				if (hasF || hasR)
				{
					groundF = hasF ? groundF : groundR + (data.radiusRear - data.radiusFront);
					groundR = hasR ? groundR : groundF + (data.radiusFront - data.radiusRear);
					float wantF = groundF + data.radiusFront, wantR = groundR + data.radiusRear;
					float pitch = atan2f(wantF - wantR, base) - atan2f(data.hubFront[1] - data.hubRear[1], base);
					float y = wantR - (data.hubRear[1] * cosf(pitch) + data.hubRear[2] * sinf(pitch));
					bool step = !bike.airborne && wantF - hubF[1] > 35.0f && bike.speed > 0.0f;
					bool steep = !bike.airborne && pitch > 50.0f * PI / 180.0f;
					if (step || steep)
					{
						bike.pos = oldPos; // a step or a slope the bike cannot climb: like a wall
						bike.vel = bike.vel * -0.15f;
						bike.speed = bike.vel.Dot(flat.at) * 5000.0f;
					}
					else if (y >= ballistic - droop)
					{
						if (bike.airborne)
						{
							float fall = -bike.vy;
							if (bike.airTime > 0.25f)
								Log("bike: landed at %.0f km/h after %.1f s in the air, falling %.1f m/s", fabsf(bike.speed) * 0.036f,
									bike.airTime, fall / 100.0f);
							if (rider && bike.vy < -1800.0f && bike.airTime > 0.6f)
								Eject(hero, "hard landing", 0.7f, fall / 100.0f);
							if (bike.airTime > 0.25f && fall > 300.0f)
							{
								// the suspension takes the landing: the bike dips and comes up again, a
								// thud (GTA's BOUNCE_SUSPENSION), dust (or water) from under the wheels
								float dip = fall / 1500.0f * droop;
								bike.squash = dip > droop ? droop : dip;
								PlayAt("CJ_BIKE_BOUNCE.WAV", bike.vobs[0], fall / 1200.0f > 1.0f ? 1.0f : fall / 1200.0f);
								zVEC3 wheels[2] = { zVEC3(hubF[0], groundF, hubF[2]), zVEC3(hubR[0], groundR, hubR[2]) };
								for (const zVEC3& w : wheels)
								{
									if (wet)
										Splash(world, zVEC3(w[0], bike.water, w[2]), 2.0f);
									else
										Emit(world, "ZMODELLANDDUST", w, zVEC3(0.0f, 1.0f, 0.0f), 0.1f, 1.5f, 1.5f);
								}
							}
							bike.vy = 0.0f; // the suspension takes the landing
						}
						else
						{
							float measured = (y - oldPos[1]) / dt; // following the ground, smoothed over bumps
							measured = measured > 2000.0f ? 2000.0f : (measured < -2000.0f ? -2000.0f : measured);
							bike.vy = 0.5f * bike.vy + 0.5f * measured;
						}
						bike.airTime = 0.0f;
						bike.pos[1] = y;
						bike.pitch += (pitch - bike.pitch) * (1.0f - expf(-dt * 20.0f));
						bike.airborne = false;
						grounded = true;
					}
				}
				if (!grounded)
				{
					if (!bike.airborne)
					{
						bike.pitch += bike.wheelie; // a wheelie leaves the ground as it is
						bike.wheelie = 0.0f;
					}
					bike.airTime += dt;
					if (bike.airTime > 0.25f && bike.airTime - dt <= 0.25f)
						Log("bike: in the air at %.0f km/h", fabsf(bike.speed) * 0.036f);
					bike.airborne = true;
					bike.pos[1] = ballistic;
					bike.vy -= gravity * dt;
					if (wet) // the water holds it back: it sinks slowly
					{
						bike.vy = bike.vy < -300.0f ? -300.0f : bike.vy;
						bike.vy *= expf(-dt * 2.0f);
					}
					bike.pitch += bike.leanInput * -1.2f * dt; // lean forward: nose down
					if (bike.pos[1] < -100000.0f) // fell out of the world
						bike.vel = zVEC3(0.0f, 0.0f, 0.0f), bike.vy = 0.0f;
				}
				if (rider && !bike.airborne)
					HitPeople(hero, Orientation(bike.yaw, bike.pitch, 0.0f));
				bike.squash *= expf(-dt * 7.0f); // the suspension comes back up

				// water (GTA: a bike in water): it holds the bike back the deeper it goes, and water up to
				// the seat drowns the engine and throws the rider in, where Gothic has him swim
				float seat = data.seat[1] + height; // the seat over the wheels' bottom
				if (wet)
				{
					float k = 0.03f * (bike.depth / seat < 1.5f ? bike.depth / seat : 1.5f); // of its speed a tick
					bike.vel = bike.vel * powf(1.0f - k, ts);
					bike.omega *= powf(1.0f - k, ts);
					bike.speed = bike.vel.Dot(flat.at) * 5000.0f;
					if (!bike.drowned && bike.depth > seat * 0.8f)
					{
						bike.drowned = true;
						bike.riderless = true;
						Splash(world, zVEC3(bike.pos[0], bike.water, bike.pos[2]), 3.0f, 0.3f);
						Log("bike: drowned in %.0f cm of water", bike.depth);
						if (rider)
							Eject(hero, "into the water", 0.3f);
					}
				}

				// what the wheels throw up (GTA's wheel particles): dust behind a wheel that spins or
				// skids, a trail of it at speed on loose ground (earth, grass), water at speed through
				// water; and GTA's skid sound for the ground it is on (tarmac for Gothic's stone, wood and
				// metal, grass for loose ground, gravel for wet)
				float now = Now(), ground = fabsf(bike.speed);
				Basis level = Orientation(bike.yaw, bike.pitch, 0.0f);
				zVEC3 rear = bike.pos + ToWorld(level, data.hubRear) - zVEC3(0.0f, data.radiusRear, 0.0f);
				zVEC3 front = bike.pos + ToWorld(level, data.hubFront) - zVEC3(0.0f, data.radiusFront, 0.0f);
				bool hard = bike.groundRear == zMAT_GROUP_STONE || bike.groundRear == zMAT_GROUP_METAL || bike.groundRear == zMAT_GROUP_WOOD;
				bool spinning = !bike.airborne && rearDown && bike.wheelState[1] == WHEEL_SPINNING;
				bool rearSkid = !bike.airborne && rearDown && bike.wheelState[1] != WHEEL_NORMAL && bike.wheelState[1] != WHEEL_SPINNING && ground > 150.0f;
				bool frontSkid = !bike.airborne && frontDown && bike.wheelState[0] != WHEEL_NORMAL && ground > 150.0f;
				bool trail = !bike.airborne && !hard && bike.groundRear != zMAT_GROUP_WATER && ground > 700.0f;
				zVEC3 up(0.0f, 1.0f, 0.0f);
				if (!wet && (spinning || rearSkid || frontSkid || trail) && now >= bike.nextDust)
				{
					bool skids = spinning || rearSkid || frontSkid;
					bike.nextDust = now + (skids ? 0.06f : 0.1f);
					if (spinning || rearSkid || trail)
						Emit(world, "PFX_DUST", rear, up, 0.04f, skids || ground > 1500.0f ? 1.6f : 1.0f, 1.0f);
					if (frontSkid)
						Emit(world, "PFX_DUST", front, up, 0.04f, 1.6f, 1.0f);
				}
				if (wet && !bike.airborne && ground > 250.0f && now >= bike.nextSplash)
				{
					bike.nextSplash = now + 0.08f;
					float size = ground > 1500.0f ? 2.0f : 1.3f;
					Splash(world, zVEC3(front[0], bike.water, front[2]), size);
					if (ground > 600.0f)
						Splash(world, zVEC3(rear[0], bike.water, rear[2]), size);
				}
				const char* skidFile = wet || bike.groundRear == zMAT_GROUP_WATER ? "CJ_BIKE_SKID_GRAVEL.WAV"
					: hard ? "CJ_BIKE_SKID.WAV" : "CJ_BIKE_SKID_GRASS.WAV";
				bool skidding = (spinning || rearSkid || frontSkid) && bike.depth < 20.0f;
				skidLoop.Update(bike.vobs[0], skidFile, skidding ? 0.8f : 0.0f, 0.9f + 0.3f * (ground / 3000.0f < 1.0f ? ground / 3000.0f : 1.0f), dt);
				waterLoop.Update(bike.vobs[0], "CJ_BIKE_WATER.WAV", wet && ground > 100.0f ? (ground / 1500.0f < 1.0f ? ground / 1500.0f : 1.0f) : 0.0f, 1.0f, dt);

				// without a rider it stops and falls over (GTA's bikes lie on their side), with a crash
				if (bike.riderless && fabsf(bike.speed) < 50.0f && !bike.airborne && bike.fallen == 0.0f)
				{
					bike.fallen = 0.0001f;
					if (!wet)
						CrashSound(0.4f);
				}
				if (bike.fallen != 0.0f)
					bike.fallen += (1.45f - bike.fallen) * (1.0f - expf(-dt * 6.0f));
			}

			// --- CJ on the bike --------------------------------------------------------------------
			// Where Gothic draws CJ's root node (GTA's ped centre) from his vob, in his own axes. It
			// depends on what plays: Gothic draws the root at the animation's root height less a root
			// height it keeps for the playing animations, which the rider's blend of poses (an
			// aniComb, no frames of its own) leaves at nothing, so on the bike the root is drawn about
			// a metre higher than standing. Read every frame, from the pose Gothic last computed.
			static zVEC3 RootOffset(oCNpc* hero)
			{
				static zCModel* known = nullptr;
				static zCModelNodeInst* root = nullptr;
				zCModel* model = hero->GetModel();
				if (model != known)
				{
					known = model;
					root = model ? model->SearchNode(zSTRING("BIP01")) : nullptr;
				}
				if (!root)
					return zVEC3(0.0f, 0.0f, 0.0f);
				zVEC3 d = model->GetNodePositionWorld(root) - hero->GetPositionWorld();
				return zVEC3(d.Dot(hero->GetRightVectorWorld()), d.Dot(hero->GetUpVectorWorld()), d.Dot(hero->GetAtVectorWorld()));
			}

			// His vob so that his root node is at `centre`, turned as `b`.
			static void PlaceRider(oCNpc* hero, const zVEC3& centre, const Basis& b)
			{
				zVEC3 offset = RootOffset(hero);
				if ((offset - bike.loggedOffset).Length() > 15.0f)
				{
					bike.loggedOffset = offset;
					Log("bike: CJ's root is drawn %.0f %.0f %.0f cm from his origin (phase %d)", offset[0], offset[1], offset[2], (int)bike.phase);
				}
				hero->SetTrafoObjToWorld(Trafo(b, centre - ToWorld(b, offset)));
			}

			static void Measure(oCNpc* hero)
			{
				bike.rootLocal = bike.loggedOffset = RootOffset(hero);
				Log("bike: CJ's root %.0f %.0f %.0f cm from his origin, standing", bike.rootLocal[0], bike.rootLocal[1], bike.rootLocal[2]);
			}

			static void SetCollision(oCNpc* hero, bool on)
			{
				if (!on)
				{
					bike.collStat = hero->collDetectionStatic != 0;
					bike.collDyn = hero->collDetectionDynamic != 0;
					hero->SetPhysicsEnabled(FALSE);
				}
				hero->SetCollDetStat(on ? bike.collStat : FALSE);
				hero->SetCollDetDyn(on ? bike.collDyn : FALSE);
			}

			static zCModelAni* Ani(zCModel* model, const char* name)
			{
				int id = model ? model->GetAniIDFromAniName(zSTRING(name)) : -1;
				return id >= 0 ? model->GetAniFromAniID(id) : nullptr;
			}

			static void StartAni(zCModel* model, const char* name)
			{
				if (zCModelAni* ani = Ani(model, name))
					model->StartAni(ani, zCModel::zMDL_STARTANI_DEFAULT);
				else
					Log("bike: CJ.MDS has no %s (run make_cj_model.py)", name);
			}

			// F next to the bike (UpdateHero): CJ walks the last bit to its side and jumps on.
			static bool TryMount(oCNpc* hero)
			{
				if (!settings.bike || !Load() || bike.phase != PARKED || !bike.vobs[0] || bike.world != hero->GetHomeWorld()
					|| Now() - bike.lastOff < 0.6f)
					return false;
				Basis b = Orientation(bike.yaw, bike.pitch, 0.0f);
				zVEC3 d = hero->GetPositionWorld() - (bike.pos + ToWorld(b, data.seat));
				d[1] = 0.0f;
				int state = BodyState(hero);
				if (d.Length() > 220.0f || fabsf(bike.speed) > 100.0f || bike.airborne
					|| !(state == BS_STAND || state == BS_WALK || state == BS_RUN || state == BS_SPRINT || state == BS_SNEAK))
					return false;
				if (bike.drowned) // a bike under water stays dead; out of it, its engine starts again
				{
					if (bike.depth > (data.seat[1] + data.radiusRear - data.hubRear[1]) * 0.4f)
					{
						Log("bike: drowned, it does not start");
						return false;
					}
					bike.drowned = false;
				}
				if (hero->GetWeaponMode() != NPC_WEAPON_NONE)
					hero->SetWeaponMode(NPC_WEAPON_NONE); // GTA puts the weapon away at once
				StopSprint(hero);
				ShowFlash(hero->GetModel(), false);
				aiming = false;
				bool left = d.Dot(b.right) < 0.0f;
				bike.path = left ? MOUNT_LEFT : MOUNT_RIGHT;
				bike.fallen = 0.0f; // a bike lying on its side is stood up
				bike.riderless = false;
				bike.speed = bike.omega = bike.wheelie = 0.0f;
				bike.vel = zVEC3(0.0f, 0.0f, 0.0f);
				Measure(hero);
				SetCollision(hero, false);
				bike.approachFrom = hero->GetPositionWorld();
				bike.approachYaw = Yaw(Flat(hero->GetAtVectorWorld()));
				bike.phase = APPROACH;
				bike.phaseStart = Now();
				Log("bike: getting on from the %s", left ? "left" : "right");
				return true;
			}

			// CJ standing again with his root at `centre`: upright, his collision back, Gothic's stand
			// animation in place of the rider's (Gothic's AI does not start it by itself).
			static void Finish(oCNpc* hero, const zVEC3& centre)
			{
				Basis upright = Orientation(bike.yaw, 0.0f, 0.0f);
				hero->SetTrafoObjToWorld(Trafo(upright, centre - ToWorld(upright, bike.rootLocal)));
				SetCollision(hero, true);
				if (zCModel* model = hero->GetModel())
				{
					for (const char* name : { "S_CJ_BIKESTILL", "S_CJ_BIKEPUSHES" })
						if (zCModelAni* ani = Ani(model, name))
							if (model->IsAniActive(ani))
								model->FadeOutAni(ani);
					StartAni(model, "S_RUN");
				}
				bike.phase = PARKED;
				bike.lastOff = Now();
				bike.gas = bike.brake = bike.steerInput = bike.leanInput = 0.0f;
				Engine(false, 0.0f);
			}

			// CJ thrown off (bailing out at speed, a crash, a hard landing, a blast, deep water): beside
			// the bike, flying on with part of its speed; Gothic takes over his fall. The bike carries on
			// without him. `carry`: the share of the bike's speed CJ flies on with; `hurt`: how hard (m/s)
			// he hits what threw him off. GTA hurts a rider knocked off at speed; its numbers
			// (CEventKnockOffBike) are not reverse-engineered, so this is the plugin's own: nothing up to
			// 6 m/s, then a share of his health growing to 60 % at 27 m/s (some 100 km/h), times
			// crashDamage.
			static void Eject(oCNpc* hero, const char* why, float carry, float hurt)
			{
				if (bike.phase == PARKED)
					return;
				Basis b;
				zVEC3 origin;
				Pose(b, origin);
				zVEC3 centre = origin + ToWorld(b, data.seat) - Orientation(bike.yaw, 0.0f, 0.0f).right * 60.0f;
				zVEC3 velocity = bike.vel * (5000.0f * carry) + zVEC3(0.0f, 300.0f, 0.0f);
				Finish(hero, centre);
				bike.riderless = true;
				sprintLock = true; // Space (the handbrake) held on does not make him sprint off
				hero->SetPhysicsEnabled(TRUE);
				if (zCRigidBody* body = hero->GetRigidBody())
					body->SetVelocity(velocity);
				float share = (hurt - 6.0f) / 35.0f;
				float damage = 0.0f;
				if (share > 0.0f && settings.crashDamage > 0.0f && bike.vobs[0])
				{
					share = (share > 0.6f ? 0.6f : share) * settings.crashDamage;
					damage = share * hero->attribute[NPC_ATR_HITPOINTSMAX];
					hero->OnDamage(bike.vobs[0], nullptr, damage + hero->protection[oEDamageIndex_Blunt], oEDamageType_Blunt, hero->GetPositionWorld());
				}
				Log("bike: CJ thrown off (%s) at %.0f km/h, %.0f damage", why, fabsf(bike.speed) * 0.036f, damage);
			}

			// F on the bike: off to the left (the right if a wall is in the way); bail out at speed
			// (hurt by a third of the speed: he jumps, he is not thrown).
			static void Dismount(oCNpc* hero)
			{
				if (fabsf(bike.speed) > 400.0f || bike.airborne)
				{
					Eject(hero, "bailed out", 0.7f, fabsf(bike.speed) / 100.0f / 3.0f);
					return;
				}
				Basis b = Orientation(bike.yaw, bike.pitch, 0.0f);
				zVEC3 seat = bike.pos + ToWorld(b, data.seat);
				bool left = true;
				if (bike.world && bike.world->TraceRayNearestHit(seat, b.right * -110.0f, hero, TraceFlags()) && bike.world->traceRayReport.foundHit)
					left = false;
				bike.path = left ? DISMOUNT_LEFT : DISMOUNT_RIGHT;
				bike.speed = bike.omega = bike.wheelie = 0.0f;
				bike.vel = zVEC3(0.0f, 0.0f, 0.0f);
				bike.roll = 0.0f;
				bike.steerInput = 0.0f;
				bike.phase = DISMOUNTING;
				bike.phaseStart = Now();
				StartAni(hero->GetModel(), left ? "T_CJ_BIKEDISMOUNTL" : "T_CJ_BIKEDISMOUNTR");
				Engine(false, 0.0f);
				Log("bike: getting off to the %s", left ? "left" : "right");
			}

			// GTA's rider animation (CBike::ProcessRiderAnims): the nine poses blended by how far the
			// bike leans (X: its lean against fFullAnimLean, so his knee comes out in hard turns) and
			// by his lean forward or back (Y: the lean keys, the pull of accelerating and braking, back
			// in a wheelie); one foot down while stopped; pushing the bike back with his feet in reverse.
			static void Layer2(zCModel* model, zCModelAni* ani, bool on)
			{
				if (!ani)
					return;
				bool playing = model->IsAniActive(ani);
				if (on && !playing)
					model->StartAni(ani, zCModel::zMDL_STARTANI_DEFAULT);
				else if (!on && playing)
					model->FadeOutAni(ani);
			}

			static void RiderAnims(zCModel* model)
			{
				if (!model)
					return;
				zCModelAni* ride = Ani(model, "S_CJ_BIKERIDE");
				if (ride && !model->IsAniActive(ride))
					model->StartAni(ride, zCModel::zMDL_STARTANI_DEFAULT);
				float side = bike.roll / (data.fullAnimLean * PI / 180.0f);
				float lean = bike.leanInput + bike.accelLean - (bike.wheelie > 0.0f ? bike.wheelie * 2.0f : 0.0f);
				side = side < -1.0f ? -1.0f : (side > 1.0f ? 1.0f : side);
				lean = lean < -1.0f ? -1.0f : (lean > 1.0f ? 1.0f : lean);
				if (ride)
					model->SetCombineAniXY(ride->aniID, 0.5f + side * 0.5f, 0.5f - lean * 0.5f);
				bool reversing = bike.speed < -30.0f && bike.gas < 0.0f && !bike.airborne;
				bool stopped = !reversing && fabsf(bike.speed) < 60.0f && !bike.airborne && bike.gas <= 0.0f;
				Layer2(model, Ani(model, "S_CJ_BIKEPUSHES"), reversing);
				Layer2(model, Ani(model, "S_CJ_BIKESTILL"), stopped);
			}

			static bool useHeldOnBike = false;

			// Every frame of CJ on the bike, in place of Gothic's AI for him.
			static void UpdateRider(oCNpc* hero)
			{
				float now = Now(), dt = ztimer->frameTimeFloat / 1000.0f;
				dt = dt > 0.05f ? 0.05f : dt;
				if (hero->IsDead() || hero->IsUnconscious())
				{
					Eject(hero, "down");
					return;
				}
				zCModel* model = hero->GetModel();
				float t = now - bike.phaseStart;
				const Path& path = data.paths[bike.path];
				Basis level = Orientation(bike.yaw, bike.pitch, 0.0f);
				switch (bike.phase)
				{
				case APPROACH:
				{
					// to where GTA's jump-on starts: the seat less the animation's whole path
					zVEC3 start = bike.pos + ToWorld(level, data.seat - (path.points[path.frames - 1] - path.points[0]));
					float a = t / 0.3f > 1.0f ? 1.0f : t / 0.3f;
					float turn = bike.yaw - bike.approachYaw;
					turn -= turn > PI ? 2.0f * PI : (turn < -PI ? -2.0f * PI : 0.0f);
					Basis upright = Orientation(bike.approachYaw + turn * a, 0.0f, 0.0f);
					zVEC3 from = bike.approachFrom + ToWorld(upright, bike.rootLocal);
					PlaceRider(hero, from + (start - from) * a, upright);
					if (a >= 1.0f)
					{
						bike.phase = MOUNTING;
						bike.phaseStart = now;
						StartAni(model, bike.path == MOUNT_LEFT ? "T_CJ_BIKEMOUNTL" : "T_CJ_BIKEMOUNTR");
					}
					break;
				}
				case MOUNTING:
					PlaceRider(hero, bike.pos + ToWorld(level, data.seat - (path.points[path.frames - 1] - path.At(t))), level);
					if (t >= path.Length())
					{
						bike.phase = RIDING;
						bike.gear = 1;
						useHeldOnBike = KeyDown(KEY_F);
						Log("bike: riding");
					}
					break;
				case RIDING:
				{
					bool f = KeyDown(KEY_F);
					bool pressed = f && !useHeldOnBike;
					useHeldOnBike = f;
					if (pressed)
					{
						Dismount(hero);
						if (bike.phase != RIDING)
							break;
					}
					Step(hero, dt, true);
					if (bike.phase != RIDING) // thrown off
						break;
					RiderAnims(model);
					Basis b;
					zVEC3 origin;
					Pose(b, origin);
					PlaceRider(hero, origin + ToWorld(b, data.seat), b);
					Engine(true, dt);
					break;
				}
				case DISMOUNTING:
					PlaceRider(hero, bike.pos + ToWorld(level, data.seat + (path.At(t) - path.points[0])), level);
					if (t >= path.Length())
					{
						Finish(hero, bike.pos + ToWorld(level, data.seat + (path.points[path.frames - 1] - path.points[0])));
						Log("bike: off");
					}
					break;
				default:
					break;
				}
				PlaceParts();
				Headlight(bike.phase == RIDING && !bike.drowned);
			}

			// A rocket's blast (CJ::Explode) throws the bike away from it and up, as GTA's explosions
			// throw vehicles: within the blast's radius, harder the closer; its rider is thrown off
			// (the blast hurts him itself), and without him it falls over where it lands.
			static void Blast(oCNpc* hero, const zVEC3& point, float radius)
			{
				if (!bike.vobs[0] || bike.world != hero->GetHomeWorld())
					return;
				zVEC3 away = bike.pos - point;
				float d = away.Length();
				if (d >= radius)
					return;
				float k = 1.0f - d / radius;
				if (bike.phase != PARKED)
					Eject(hero, "blast", 0.3f);
				bike.riderless = true;
				bike.fallen = 0.0f;
				bike.vel += Flat(away) * (k * 1500.0f / 5000.0f); // up to 15 m/s (m per tick)
				bike.omega += (away.Dot(Orientation(bike.yaw, 0.0f, 0.0f).right) > 0.0f ? 1.0f : -1.0f) * k * 0.08f;
				bike.vy = bike.vy > k * 900.0f ? bike.vy : k * 900.0f;
				bike.pos[1] += 25.0f; // above the suspension's reach: in the air
				bike.airborne = true;
				CrashSound(k + 0.3f);
				Log("bike: thrown by a blast %.0f m away", d / 100.0f);
			}

			// GTA's headlight at night (from 20:00 to 6:00) while CJ rides: Gothic's dynamic lights
			// are points, so the light hangs a few metres ahead of the bike over the ground it shows.
			static zCVobLight* headlight = nullptr;

			static void Headlight(bool on)
			{
				int day = 0, hour = 12, minute = 0;
				if (on && ogame)
					ogame->GetTime(day, hour, minute);
				if (!on || !settings.bikeLight || !bike.world || (hour >= 6 && hour < 20))
				{
					FreeLight(headlight);
					return;
				}
				Basis b;
				zVEC3 origin;
				Pose(b, origin);
				zVEC3 at = origin + Flat(b.at) * 350.0f + zVEC3(0.0f, 80.0f, 0.0f);
				if (headlight && headlight->GetHomeWorld() != bike.world)
					FreeLight(headlight);
				if (!headlight)
					headlight = NewLight(bike.world, at, 900.0f, Color(255, 244, 214));
				else
					headlight->SetPositionWorld(at);
			}

			static void StopEffects()
			{
				FreeLight(headlight);
				skidLoop.Stop();
				waterLoop.Stop();
			}

			// Every frame of the hero's AI: the bike in CJ's world (put next to him when he enters one),
			// rolling on by itself while he is not on it.
			static void Update(oCNpc* hero)
			{
				zCWorld* world = hero->GetHomeWorld();
				if (!settings.bike || !world || !Load())
					return;
				if (world != bike.world || hero != bike.hero || !bike.vobs[0])
				{
					if (bike.phase != PARKED) // a game loaded while CJ rode: stand him up again
					{
						bike.phase = PARKED;
						Engine(false, 0.0f);
					}
					if (!hero->collDetectionStatic)
					{
						hero->SetCollDetStat(TRUE);
						hero->SetCollDetDyn(TRUE);
						hero->SetHeadingAtWorld(Flat(hero->GetAtVectorWorld()));
					}
					Spawn(hero, world);
					return;
				}
				if (bike.phase == PARKED)
				{
					float dt = ztimer->frameTimeFloat / 1000.0f;
					Step(hero, dt > 0.05f ? 0.05f : dt, false);
					PlaceParts();
					Headlight(false);
				}
			}

			// GTA's radar shows more around a fast vehicle: 180 m up to 350 m between 15 and 45 m/s
			// (CRadar::DrawMap, RADAR_MIN/MAX_SPEED 0.3 and 0.9 m per tick).
			static float RadarScale()
			{
				if (bike.phase != RIDING)
					return 1.0f;
				float v = fabsf(bike.speed) / 5000.0f;
				float t = (v - 0.3f) / 0.6f;
				t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
				return 1.0f + t * (350.0f / 180.0f - 1.0f);
			}
		}

		// PlayerControlZelda: desired heading = input direction relative to the camera; CJ
		// turns towards it at most turnRate degrees per second (CPed::UpdatePosition).
		static void UpdateHero(oCNpc* hero)
		{
			float up = (KeyDown(KEY_W) ? 1.0f : 0.0f) - (KeyDown(KEY_S) ? 1.0f : 0.0f);
			float right = (KeyDown(KEY_D) ? 1.0f : 0.0f) - (KeyDown(KEY_A) ? 1.0f : 0.0f);
			moving = up != 0.0f || right != 0.0f;

			bool jumpKey = KeyDown(KEY_LSHIFT);
			jumpKeyPressed = jumpKey && !jumpKeyHeld;
			if (jumpKeyPressed)
				jumpKeyTime = ztimer->totalTimeFloat / 1000.0f;
			jumpKeyHeld = jumpKey;
			bool key = KeyDown(KEY_F);
			useKeyPressed = key && !useKey;
			useKey = key;
			if (useKeyPressed && Bike::TryMount(hero)) // GTA's F gets into the vehicle before anything else
			{
				moving = false;
				useHeld = useForward = false;
				useUntil = -1.0f;
				return;
			}
			// Gothic interacts (talk, take, open) on its action key held plus a forward press while
			// the hero stands (oCAIHuman::PC_ActionMove -> StandActions): F stops CJ and gives Gothic
			// action and forward presses for a moment, until he stands and the interaction starts.
			if (useKeyPressed)
			{
				useUntil = Now() + 0.5f;
				zCVob* focus = hero->GetFocusVob();
				Log("use (F): focus %s, body state %d, weapon mode %d", focus ? focus->GetObjectName().ToChar() : "none",
					BodyState(hero), hero->GetWeaponMode());
			}
			bool interacting = Now() < useUntil;
			if (interacting)
				moving = false;
			useHeld = useKey || interacting;
			useForward = interacting && BodyState(hero) == BS_STAND;
			key = KeyDown(KEY_LALT);
			walkKeyPressed = key && !walkKey;
			walkKey = key;
			key = KeyDown(KEY_C);
			sneakKeyPressed = key && !sneakKey;
			sneakKey = key;

			if (!gunsGiven)
			{
				gunsGiven = true;
				FindGuns();
				static const int GIVE[GUNS] = { 3 * CLIP, 10 };
				for (int i = 0; i < GUNS; i++)
					if ((i == GUN_PISTOL ? settings.startPistol : settings.startRocket) && gunIndex[i] >= 0 && gunAmmo[i] >= 0 && !GunItem(hero, i))
					{
						hero->CreateItems(gunIndex[i], 1);
						hero->CreateItems(gunAmmo[i], GIVE[i]);
						Log("start weapons: gave CJ the %s and %d rounds", GUN_NAMES[i], GIVE[i]);
					}
				if (settings.startDildo && dildoIndex >= 0 && !DildoItem(hero))
				{
					hero->CreateItems(dildoIndex, 1);
					Log("start weapons: gave CJ the dildo");
				}
			}
			UpdatePendingGun(hero);

			// GTA has no fists-up key: unarmed, a click raises CJ's fists at once (as the scripts'
			// Npc_SetToFistMode does) and the left one punches straight away. FIST_IDLE seconds without
			// a click, or sprinting off, lowers them again (Fight2Idle).
			int stateNow = BodyState(hero);
			bool onGround = stateNow != BS_JUMP && stateNow != BS_FALL && stateNow != BS_SWIM && stateNow != BS_DIVE;
			bool lmbNow = MouseDown(true), rmbNow = MouseDown(false);
			if (hero->GetWeaponMode() == NPC_WEAPON_NONE && (lmbNow || rmbNow) && !punch && !block && onGround)
			{
				hero->SetToFistMode();
				lastFight = Now();
				Log("fists up (%s click)", lmbNow ? "left" : "right");
			}

			// Q/E or the wheel cycle the weapons (not mid-punch, aim or reload): E and the wheel down
			// forward, Q and the wheel up back
			static float lastSwitch = -1.0f;
			bool keyQ = KeyDown(KEY_Q), keyE = KeyDown(KEY_E), switchKey = keyQ || keyE;
			int cycle = switchKey && !switchKeyHeld ? (keyE ? 1 : -1) : (mouseWheel < 0.0f ? 1 : (mouseWheel > 0.0f ? -1 : 0));
			switchKeyHeld = switchKey;
			mouseWheel = 0.0f;
			if (cycle != 0 && Now() - lastSwitch > 0.4f && BodyState(hero) != BS_HIT && !aiming && reloadUntil < 0.0f)
			{
				lastSwitch = Now();
				SwitchWeapon(hero, cycle);
			}

			// Gothic attacks while its action key is held and "forward" is pressed; each further
			// press inside a punch's combo window chains the next punch. A click holds action for
			// comboTime and presses forward for 0.1 s; CJ stands while he punches, as in GTA. The
			// dildo swings the same way, through Gothic's one-handed attack and its own combo windows.
			bool fists = hero->GetWeaponMode() == NPC_WEAPON_FIST;
			bool dildo = DildoDrawn(hero);
			bool melee = fists || dildo;
			const char* lmbSource = "";
			const char* rmbSource = "";
			bool lmb = melee && MouseDown(true, &lmbSource);
			bool rmb = melee && MouseDown(false, &rmbSource);
			punchPressed = lmb && !punch;
			blockPressed = rmb && !block;
			punch = lmb;
			block = rmb;
			if (blockPressed)
				Log("block click (%s button, body state %d)", rmbSource, BodyState(hero));
			// The first click starts a punch through Gothic's attack controls; a click during a punch
			// schedules a virtual W tap (what holding the attack key and tapping W does by hand).
			float now = Now(), dt = ztimer->frameTimeFloat / 1000.0f;
			bool attacking = BodyState(hero) == BS_HIT;
			punchToggle = false;
			if (punchPulse > 0.0f)
				punchPulse -= dt;
			if (attacking && attackStart < 0.0f)
				attackStart = now;
			if (!attacking)
				attackStart = tapFrom = tapUntil = -1.0f;
			if (punchPressed)
			{
				comboTime = 0.9f;
				Log("punch click (%s button, body state %d)", lmbSource, BodyState(hero));
				if (!attacking)
				{
					punchPulse = 0.1f;
					punchToggle = true;
				}
				else if (now > tapUntil) // one extra punch per click
				{
					float elapsed = now - attackStart;
					const float (*windows)[2] = dildo ? DILDO_WINDOWS : COMBO_WINDOWS;
					for (int w = 0; w < 3; w++)
					{
						const float* window = windows[w];
						if (elapsed > window[1] - 0.05f)
							continue;
						tapFrom = attackStart + window[0] + 0.02f;
						tapFrom = tapFrom > now ? tapFrom : now;
						tapUntil = tapFrom + 0.12f;
						float end = attackStart + window[1] - 0.01f;
						tapUntil = tapUntil < end ? tapUntil : end;
						Log("combo: W tap at %.2f-%.2f s into the punch", tapFrom - attackStart, tapUntil - attackStart);
						break;
					}
				}
			}
			bool tap = tapFrom >= 0.0f && now >= tapFrom && now <= tapUntil;
			virtualWEdge = tap && !virtualW;
			virtualW = tap;
			meleeAttack = melee && attacking;
			if (attacking && comboTime < 0.3f)
				comboTime = 0.3f; // keep Gothic's attack key held through the punch
			comboTime = melee && comboTime > 0.0f ? comboTime - ztimer->frameTimeFloat / 1000.0f : 0.0f;
			if (comboTime > 0.0f)
				moving = false;
			if (melee && (lmb || rmb || attacking || comboTime > 0.0f))
				lastFight = now;
			ScareEnemies(hero, dildo);
			bool space = KeyDown(KEY_SPACE);
			if (sprintLock && !space)
				sprintLock = false;
			bool sprintKey = space && !sprintLock;
			bool sprintOff = moving && sprintKey;
			if (fists && !lmb && !rmb && !attacking && comboTime <= 0.0f && (now - lastFight > FIST_IDLE || sprintOff)
				&& now - fistsLowered > 1.0f)
			{
				fistsLowered = now;
				hero->GetEM()->OnMessage(new oCMsgWeapon(oCMsgWeapon::EV_REMOVEWEAPON, NPC_WEAPON_NONE, 0), hero);
				Log("fists down (%s)", sprintOff ? "sprinting" : "no fighting");
			}

			int state = BodyState(hero);
			bool grounded = state != BS_JUMP && state != BS_FALL;
			bool swimming = state == BS_SWIM || state == BS_DIVE;
			// Gothic's side and back steps (the strafes while aiming) count as jumps; only a real
			// jump or fall puts the pistol down
			UpdateGun(hero, !swimming && !jumping && state != BS_FALL);
			bool onFoot = !fists && moving && !swimming && !aiming;
			UpdateSprint(hero, onFoot && !heavy && sprintKey, onFoot, !grounded);
			if (aiming)
			{
				// GTA's aiming: CJ faces where the camera looks and walks or strafes with W/A/S/D
				// (OverrideKey hands them to Gothic as its own walk, back and strafe keys)
				moving = false;
				if (cameraActive)
					hero->SetHeadingAtWorld(cameraForward);
				return;
			}
			if (!moving || !grounded)
				return;

			zVEC3 forward = cameraActive ? cameraForward : Flat(zCAICamera::current->camVob->GetAtVectorWorld());
			zVEC3 want = forward * up + RightOf(forward, hero) * right;
			want = Flat(want);
			zVEC3 at = Flat(hero->GetAtVectorWorld());

			float dot = at.Dot(want);
			dot = dot > 1.0f ? 1.0f : (dot < -1.0f ? -1.0f : dot);
			float angle = acosf(dot);
			float step = settings.turnRate * PI / 180.0f * ztimer->frameTimeFloat / 1000.0f;
			zVEC3 heading = want;
			if (angle > step)
			{
				bool behind = angle > PI - 0.01f; // straight behind: turn over CJ's right side
				zVEC3 to = behind ? RightOf(at, hero) : want;
				float a = behind ? PI / 2.0f : angle;
				float t = step / a;
				heading = Flat(at * (sinf((1.0f - t) * a) / sinf(a)) + to * (sinf(t * a) / sinf(a)));
			}
			hero->SetHeadingAtWorld(heading);
		}

		// San Andreas's wasted camera, CCam::ProcessPedsDeadBaby (MODE_PED_DEAD_BABY; not reversed in
		// gta-reversed, read from gta-sa.exe: 0x526660 in the Steam build). It starts 2 m straight above
		// CJ's chest (BONE_SPINE1) looking down, the top of the screen the way he faced, and rises
		// 0.04 m a 1/50 s tick (2 m/s) while nothing is above it (a 0.3 m sphere 0.2 m up). In the
		// first second it slides out to 2 m on his right (2 sin(t / 0.6 s) t / 1 s, while the way there
		// is free), and over the first two seconds the view sways across him: straight down plus
		// -0.35 (t / 2 s) cos(t / 0.6 s) of his right side. GTA fades out after a few seconds; Gothic
		// keeps the dead hero on screen, so the camera stops wastedHeight above him.
		struct WastedCamera
		{
			bool on = false;
			float start = 0.0f;
			zVEC3 forward, right, pos;
		};
		static WastedCamera wastedCamera;

		static bool UpdateWastedCamera(oCNpc* hero, zCVob* camVob)
		{
			if (!settings.wastedCam || !hero->IsDead())
			{
				wastedCamera.on = false;
				return false;
			}
			zCWorld* world = hero->GetHomeWorld();
			if (!world)
				return false;
			zCModel* model = hero->GetModel();
			zCModelNodeInst* spine = model ? model->SearchNode(zSTRING("BIP01 SPINE1")) : nullptr;
			zVEC3 target = spine ? model->GetNodePositionWorld(spine) : hero->GetPositionWorld();
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER;
			auto blocked = [&](const zVEC3& from, const zVEC3& ray)
			{
				return world->TraceRayNearestHit(from, ray, hero, flags) && world->traceRayReport.foundHit;
			};
			zVEC3 up(0.0f, 1.0f, 0.0f);
			WastedCamera& cam = wastedCamera;
			if (!cam.on)
			{
				cam.on = true;
				cam.start = Now();
				cam.forward = Flat(hero->GetAtVectorWorld());
				cam.right = RightOf(cam.forward, hero);
				float height = 200.0f; // (below a low ceiling: under it)
				if (blocked(target, up * 230.0f))
				{
					height = world->traceRayReport.foundIntersection[1] - target[1] - 30.0f;
					height = height > 20.0f ? height : 20.0f;
				}
				cam.pos = target + up * height;
				Log("wasted: camera %.0f cm above CJ, looking down", height);
			}
			float dt = ztimer->frameTimeFloat / 1000.0f, t = Now() - cam.start;
			// rising while nothing is above it
			float top = target[1] + settings.wastedHeight;
			if (cam.pos[1] < top && !blocked(cam.pos, up * 50.0f))
			{
				cam.pos[1] += 200.0f * dt;
				cam.pos[1] = cam.pos[1] < top ? cam.pos[1] : top;
			}
			// out to his right side
			float t1 = t < 1.0f ? t : 1.0f;
			zVEC3 to = zVEC3(target[0], cam.pos[1], target[2]) + cam.right * (200.0f * sinf(t1 / 0.6f) * t1);
			zVEC3 step = to - cam.pos;
			step[1] = 0.0f;
			float length = step.Length();
			if (length > 0.1f && !blocked(cam.pos, step * ((length + 30.0f) / length)))
				cam.pos = to;
			// looking down, swaying across him
			float t2 = t < 2.0f ? t : 2.0f;
			zVEC3 look = zVEC3(0.0f, -1.0f, 0.0f) + cam.right * (-0.35f * (t2 / 2.0f) * cosf(t2 / 0.6f));
			look.Normalize();
			zVEC3 screenUp = cam.forward - look * cam.forward.Dot(look);
			screenUp.Normalize();
			zMAT4 trafo = zMAT4::GetIdentity();
			trafo.SetAtVector(look);
			trafo.SetUpVector(screenUp);
			trafo.SetRightVector(Cross(screenUp, look));
			trafo.SetTranslation(cam.pos);
			camVob->SetTrafoObjToWorld(trafo);
			if (!hero->showVisual)
				hero->showVisual = TRUE;
			return true;
		}

		// GTA follow camera: the mouse orbits it around CJ; it looks at a point above him and
		// is pulled in front of walls.
		static void UpdateCamera(zCAICamera* ai)
		{
			oCNpc* hero = oCNpc::player;
			zCVob* camVob = ai ? ai->camVob : nullptr;
			if (hero && camVob && UpdateWastedCamera(hero, camVob))
			{
				cameraActive = aimRayValid = false;
				aimBlend = fpBlend = 0.0f;
				return;
			}
			bool active = hero && camVob && ai->target == hero && CameraActive(hero);
			if (!active)
			{
				cameraActive = false;
				aimRayValid = false;
				aimBlend = fpBlend = 0.0f;
				if (hero && !hero->showVisual)
					hero->showVisual = TRUE;
				return;
			}
			// While aiming the camera eases over CJ's right shoulder (GTA's MODE_AIMWEAPON); with the
			// rocket launcher into his eyes instead (MODE_ROCKETLAUNCHER: first person, CJ unseen)
			float dt = ztimer->frameTimeFloat / 1000.0f;
			bool rocket = DrawnGun(hero) == GUN_ROCKET;
			aimBlend += (aiming && !rocket ? dt : -dt) / 0.15f;
			aimBlend = aimBlend < 0.0f ? 0.0f : (aimBlend > 1.0f ? 1.0f : aimBlend);
			fpBlend += (aiming && rocket ? dt : -dt) / 0.15f;
			fpBlend = fpBlend < 0.0f ? 0.0f : (fpBlend > 1.0f ? 1.0f : fpBlend);
			bool seen = fpBlend < 0.5f;
			if ((hero->showVisual != 0) != seen)
				hero->showVisual = seen ? TRUE : FALSE;
			float height = settings.camHeight + (settings.aimHeight - settings.camHeight) * aimBlend;
			float distance = settings.camDistance + (settings.aimDistance - settings.camDistance) * aimBlend;
			zVEC3 target = hero->GetPositionWorld() + zVEC3(0.0f, height, 0.0f);
			bool riding = Bike::Busy(); // GTA's bike camera: further back, looking at the rider
			if (riding)
			{
				distance = settings.bikeCamDistance;
				target = Bike::bike.pos + zVEC3(0.0f, 90.0f, 0.0f);
			}
			if (!cameraActive) // take over from wherever Gothic's camera is
			{
				cameraForward = Flat(camVob->GetAtVectorWorld());
				cameraPitch = 20.0f;
				cameraFocus = target;
				cameraActive = true;
			}
			// Follow CJ with a short lag (exponential, time constant camSmooth); jump straight to him
			// after a teleport or level change.
			float smooth = riding ? 0.02f : settings.camSmooth;
			if (smooth <= 0.0f || (target - cameraFocus).Length() > 400.0f)
				cameraFocus = target;
			else
				cameraFocus = cameraFocus + (target - cameraFocus) * (1.0f - expf(-dt / smooth));

			float dx = 0.0f, dy = 0.0f, dz = 0.0f;
			zinput->GetMousePos(dx, dy, dz);
			mouseWheel = dz;
			float yaw = dx * settings.mouseSpeed * PI / 180.0f;
			zVEC3 right = RightOf(cameraForward, hero);
			cameraForward = Flat(cameraForward * cosf(yaw) + right * sinf(yaw));
			cameraPitch += (settings.invertMouseY ? -dy : dy) * settings.mouseSpeed;
			cameraPitch = cameraPitch < -40.0f ? -40.0f : (cameraPitch > 70.0f ? 70.0f : cameraPitch);
			// On the bike the camera swings back behind it while it moves and the mouse rests (GTA's
			// vehicle camera returns behind the vehicle after free looking).
			static float lastLook = -10.0f;
			if (dx != 0.0f || dy != 0.0f)
				lastLook = Now();
			if (riding && Bike::bike.phase == Bike::RIDING && fabsf(Bike::bike.speed) > 150.0f && Now() - lastLook > 1.0f)
			{
				float k = 1.0f - expf(-dt / 0.35f);
				zVEC3 behind = Bike::Orientation(Bike::bike.yaw, 0.0f, 0.0f).at;
				cameraForward = Flat(cameraForward + (behind - cameraForward) * k);
				cameraPitch += (12.0f - cameraPitch) * k;
			}

			float pitch = cameraPitch * PI / 180.0f;
			zVEC3 look = cameraForward * cosf(pitch) + zVEC3(0.0f, -sinf(pitch), 0.0f);
			zVEC3 focus = cameraFocus + RightOf(cameraForward, hero) * (settings.aimSide * aimBlend);
			zVEC3 ray = look * -distance;
			zVEC3 position = focus + ray;

			zCWorld* world = hero->GetHomeWorld();
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER;
			if (world && world->TraceRayNearestHit(focus, ray, hero, flags) && world->traceRayReport.foundHit)
			{
				zVEC3 hit = world->traceRayReport.foundIntersection;
				float dist = (hit - focus).Length() - 15.0f;
				position = focus + look * -(dist > 30.0f ? dist : 30.0f);
			}
			if (fpBlend > 0.0f)
			{
				zCModel* model = hero->GetModel();
				zCModelNodeInst* head = model ? model->SearchNode(zSTRING("BIP01 HEAD")) : nullptr;
				zVEC3 eye = (head ? model->GetNodePositionWorld(head) : hero->GetPositionWorld() + zVEC3(0.0f, 70.0f, 0.0f)) + cameraForward * 12.0f;
				position = position + (eye - position) * fpBlend;
			}
			if (shake > 0.0f) // an explosion: the view shakes (GTA's camera shake)
			{
				float n = Now() * 60.0f;
				position += zVEC3(sinf(n * 1.3f), sinf(n * 1.7f + 1.0f), sinf(n * 1.1f + 2.0f)) * (12.0f * shake);
			}
			camVob->SetPositionWorld(position);
			camVob->SetHeadingAtWorld(look);

			// The crosshair's ray: GTA draws it at 53 % across and 40 % down the screen
			// (CCamera::m_f3rdPersonCHairMultX/Y), through the camera's field of view; the rocket
			// launcher's in the middle.
			float fovX = 90.0f, fovY = 67.5f;
			if (zCCamera::activeCam)
				zCCamera::activeCam->GetFOV(fovX, fovY);
			float tx = tanf(fovX * PI / 360.0f), ty = tanf(fovY * PI / 360.0f);
			float crossX = rocket ? 0.5f : CROSSHAIR_X, crossY = rocket ? 0.5f : CROSSHAIR_Y;
			aimOrigin = position;
			aimDir = look + camVob->GetRightVectorWorld() * ((2.0f * crossX - 1.0f) * tx)
				+ camVob->GetUpVectorWorld() * ((1.0f - 2.0f * crossY) * ty);
			aimDir.Normalize();
			aimRayValid = true;
		}
	}

	// GTA keeps a ped on the ground it moves over (its collision line reaches 1 m below the ped's
	// centre, CTempColModels ms_colModelPed1) and lets him walk steep ground. In Gothic, with GTA
	// controls on, the hero's movement AI is set up for that here, where it hands its settings to
	// the collision object (zCAIPlayer::PropagateCollObjectStates) every frame:
	//  - height correction at once, as for NPCs: for the player Gothic smooths it
	//    (m_eDoHeightCorrectionSmooth), which left CJ's faster run behind the floor on slopes;
	//  - steeper walkable ground: past zMV_MAX_GROUND_ANGLE_WALK zCAIPlayer::CheckFloorSliding
	//    turns physics on and slides the hero, which at CJ's speed threw him off the crest of
	//    every steep path. The angles (radians, compared with acos of the floor normal) are raised
	//    to slopeWalk / slopeSlide degrees, never lowered.
	void __fastcall zCAIPlayer_PropagateCollObjectStates(zCAIPlayer* self, void* vtable, int state);
	auto Hook_zCAIPlayer_PropagateCollObjectStates = Union::CreateHook(reinterpret_cast<void*>(0x004FE610),
		&zCAIPlayer_PropagateCollObjectStates, Union::HookType::Hook_Detours);
	void __fastcall zCAIPlayer_PropagateCollObjectStates(zCAIPlayer* self, void* vtable, int state)
	{
		bool hero = CJ::gtaControls && oCNpc::player && self->vob == oCNpc::player;
		if (hero)
		{
			zCAIPlayer::zTConfig& config = self->config;
			static bool logged = false;
			if (!logged)
			{
				logged = true;
				CJ::Log("Gothic's slopes for the hero: walk %.0f, slide %.0f / %.0f degrees, step %.0f cm",
					config.zMV_MAX_GROUND_ANGLE_WALK * 180.0f / CJ::PI, config.zMV_MAX_GROUND_ANGLE_SLIDE * 180.0f / CJ::PI,
					config.zMV_MAX_GROUND_ANGLE_SLIDE2 * 180.0f / CJ::PI, config.zMV_STEP_HEIGHT);
			}
			float walk = CJ::settings.slopeWalk * CJ::PI / 180.0f, slide = CJ::settings.slopeSlide * CJ::PI / 180.0f;
			config.zMV_MAX_GROUND_ANGLE_WALK = config.zMV_MAX_GROUND_ANGLE_WALK > walk ? config.zMV_MAX_GROUND_ANGLE_WALK : walk;
			config.zMV_MAX_GROUND_ANGLE_SLIDE = config.zMV_MAX_GROUND_ANGLE_SLIDE > slide ? config.zMV_MAX_GROUND_ANGLE_SLIDE : slide;
			config.zMV_MAX_GROUND_ANGLE_SLIDE2 = config.zMV_MAX_GROUND_ANGLE_SLIDE2 > slide ? config.zMV_MAX_GROUND_ANGLE_SLIDE2 : slide;
		}
		Hook_zCAIPlayer_PropagateCollObjectStates(self, vtable, state);
		if (hero)
			if (zCCollisionObject* collision = self->vob->GetCollisionObject()) // a character's, as Gothic set it just now
				static_cast<zCCollObjectCharacter*>(collision)->m_oConfig.m_eDoHeightCorrectionSmooth = 0;
	}

	// Gothic lets a frame last up to 100 ms. On a stutter (Gothic loading NPCs or textures as CJ
	// nears a camp, a new weapon's model on its first draw) CJ, at GTA speed, moves 40-70 cm in that
	// one step; down a steep slope the floor is then more than Gothic's 60 cm step below him and
	// Gothic drops him into a fall. Its own hero, slower and kept off ground steeper than 45 degrees,
	// never gets there. With GTA controls on, a frame counts as at most maxFrameTime ms: the game
	// slows down through the stutter instead of CJ (or the bike) leaping. The longest recent frame
	// goes into the fall log.
	void __fastcall zCTimer_ResetTimer(zCTimer* self, void* vtable);
	auto Hook_zCTimer_ResetTimer = Union::CreateHook(reinterpret_cast<void*>(0x005CE550), &zCTimer_ResetTimer, Union::HookType::Hook_Detours);
	void __fastcall zCTimer_ResetTimer(zCTimer* self, void* vtable)
	{
		Hook_zCTimer_ResetTimer(self, vtable);
		if (self != ztimer)
			return;
		float raw = self->frameTimeFloat, now = self->totalTimeFloat / 1000.0f;
		if (raw > CJ::recentFrame || now - CJ::recentFrameAt > 1.0f)
		{
			CJ::recentFrame = raw;
			CJ::recentFrameAt = now;
		}
		float cap = CJ::settings.maxFrameTime;
		if (!CJ::gtaControls || cap < 1.0f || raw <= cap)
			return;
		unsigned long capped = (unsigned long)cap;
		self->totalTime -= self->frameTime - capped;
		self->totalTimeFloat -= raw - (float)capped;
		self->frameTime = capped;
		self->frameTimeFloat = (float)capped;
	}

	// A new game, or a savegame loaded: see CJ::NewGame.
	void __fastcall oCGame_LoadGame(oCGame* self, void* vtable, int slot, zSTRING const& level);
	auto Hook_oCGame_LoadGame = Union::CreateHook(reinterpret_cast<void*>(0x0063C070), &oCGame_LoadGame, Union::HookType::Hook_Detours);
	void __fastcall oCGame_LoadGame(oCGame* self, void* vtable, int slot, zSTRING const& level)
	{
		CJ::NewGame();
		Hook_oCGame_LoadGame(self, vtable, slot, level);
	}

	void __fastcall oCGame_LoadSavegame(oCGame* self, void* vtable, int slot, int loadGlobals);
	auto Hook_oCGame_LoadSavegame = Union::CreateHook(reinterpret_cast<void*>(0x0063C2A0), &oCGame_LoadSavegame, Union::HookType::Hook_Detours);
	void __fastcall oCGame_LoadSavegame(oCGame* self, void* vtable, int slot, int loadGlobals)
	{
		CJ::NewGame();
		Hook_oCGame_LoadSavegame(self, vtable, slot, loadGlobals);
	}

	// The hero's AI reads its controls inside DoAI; overrides apply only during that call.
	void __fastcall oCAIHuman_DoAI(oCAIHuman* self, void* vtable, zCVob* vob, int& vobDeleted);
	auto Hook_oCAIHuman_DoAI = Union::CreateHook(reinterpret_cast<void*>(0x00615A50), &oCAIHuman_DoAI, Union::HookType::Hook_Detours);
	void __fastcall oCAIHuman_DoAI(oCAIHuman* self, void* vtable, zCVob* vob, int& vobDeleted)
	{
		oCNpc* hero = oCNpc::player;
		if (!hero || self != hero->human_ai)
			return Hook_oCAIHuman_DoAI(self, vtable, vob, vobDeleted);

		CJ::LoadSettings();
		CJ::FullDetail(hero);
		CJ::Bike::Update(hero);
		if (CJ::Bike::Busy()) // CJ on the bike: the plugin moves him, Gothic's AI for him rests
		{
			CJ::gtaControls = true;
			CJ::moving = CJ::aiming = false;
			CJ::Bike::UpdateRider(hero);
			return;
		}
		int weaponMode = hero->GetWeaponMode();
		bool dildo = CJ::DildoDrawn(hero);
		if (weaponMode >= NPC_WEAPON_DAG && !CJ::GunDrawn(hero) && !dildo) // no sword, bow, other crossbow or magic animations
		{
			CJ::Log("weapon mode %d blocked (CJ has fists, his guns and the dildo only)", weaponMode);
			hero->SetWeaponMode(NPC_WEAPON_NONE);
		}
		CJ::UpdateJump(hero);
		if ((weaponMode == NPC_WEAPON_FIST || dildo) && hero->GetAnictrl())
			hero->GetAnictrl()->comboMax = 3; // FightA_1, FightA_2, FightA_3 (DILDO_1, DILDO_2, DILDO_3)
		CJ::heroAI = true;
		static int lastWeaponMode = -1, lastActive = -1;
		int active = CJ::ControlsActive(hero) ? 1 : 0;
		CJ::gtaControls = active != 0;
		if (weaponMode != lastWeaponMode || active != lastActive)
		{
			CJ::Log("weapon mode %d, GTA controls %d (camera mode %s, body state %d)", weaponMode, active,
				CJ::CameraMode().ToChar(), CJ::BodyState(hero));
			lastWeaponMode = weaponMode;
			lastActive = active;
		}
		if (!active)
		{
			CJ::moving = CJ::punch = CJ::block = CJ::punchPressed = CJ::blockPressed = false;
			CJ::comboTime = 0.0f;
			CJ::punchPulse = 0.0f;
			CJ::punchToggle = CJ::virtualW = CJ::virtualWEdge = CJ::meleeAttack = false;
			CJ::attackStart = CJ::tapFrom = CJ::tapUntil = -1.0f;
			CJ::aiming = CJ::aimUp = CJ::aimDown = CJ::aimLeft = CJ::aimRight = false;
			CJ::aimHoldUntil = -1.0f;
			CJ::ShowFlash(hero->GetModel(), false);
			CJ::StopSprint(hero);
			Hook_oCAIHuman_DoAI(self, vtable, vob, vobDeleted);
			CJ::heroAI = false;
			return;
		}
		CJ::UpdateHero(hero);
		if (CJ::Bike::Busy()) // F got CJ onto the bike
		{
			CJ::heroAI = false;
			return;
		}
		CJ::overrideInput = true;
		Hook_oCAIHuman_DoAI(self, vtable, vob, vobDeleted);
		CJ::overrideInput = false;
		CJ::heroAI = false;

	}

	// In GTA mode only GTA's controls reach the hero: every movement and combat key Gothic asks for
	// is answered from them, so Gothic's own bindings (arrow keys, Ctrl, Delete/PgDn strafes, Alt
	// and Shift moves, X for weapons, its look keys) do nothing. Menus, the inventory (Tab), status
	// (B), log and map keep Gothic's keys.
	//   W/A/S/D  Gothic's "forward" while any is held (CJ already faces the way to go); nothing while
	//            aiming, when UpdateGun walks CJ with GTA's gun moves
	//   mouse    with fists up the left button is Gothic's attack (action + forward), the right one
	//            its parry (action + back); fists come up by themselves (UpdateHero)
	//   F        Gothic's action unarmed (with the forward press it needs): talk, take, open, use.
	//            Gothic.ini binds F to its action key too (keyAction=1d002100, F taken off
	//            keyLookFP), for where Gothic reads the key itself
	//   Left Shift jump, Left Alt walk, C sneak
	static bool OverrideKey(unsigned short key, float& state, int& toggled)
	{
		auto set = [&](bool on, bool pressed) { state = on ? 1.0f : 0.0f; toggled = pressed ? 1 : 0; return true; };
		// During a punch Gothic checks its combo while the animation plays, outside the hero's AI:
		// there, only left clicks count (the virtual W tap, the attack key held through the combo).
		if (CJ::meleeAttack && key == GAME_UP)
			return set(CJ::virtualW || CJ::punchPulse > 0.0f, CJ::virtualWEdge || CJ::punchToggle);
		if (CJ::meleeAttack && key == GAME_ACTION && CJ::comboTime > 0.0f)
			return set(true, false);
		// Gothic's weapon, look and other combat keys never act in GTA mode, wherever Gothic reads them
		if (CJ::gtaControls)
		{
			switch (key)
			{
			case GAME_WEAPON: case GAME_LOOK: case GAME_LOOK_FP: case GAME_LOCK_TARGET: case GAME_PARADE:
			case GAME_ACTIONLEFT: case GAME_ACTIONRIGHT: case GAME_ACTION2: case GAME_SMOVE2: case GAME_SHIFT:
			case GAME_LAME_POTION: case GAME_LAME_HEAL:
				return set(false, false);
			}
		}
		if (!CJ::overrideInput)
			return false;
		bool pistol = oCNpc::player && oCNpc::player->GetWeaponMode() == NPC_WEAPON_CBOW;
		switch (key)
		{
		case GAME_UP:
			if (CJ::useForward)
				return set(false, true);
			if (CJ::aiming)
				return set(false, false);
			if (CJ::comboTime > 0.0f)
				return set(CJ::punchPulse > 0.0f, CJ::punchToggle);
			state = CJ::moving ? 1.0f : 0.0f;
			toggled = -1; // Gothic's own toggle, if CJ moves
			return true;
		case GAME_DOWN:
			return set(!CJ::aiming && CJ::block, !CJ::aiming && CJ::blockPressed);
		case GAME_LEFT: case GAME_RIGHT: case GAME_STRAFELEFT: case GAME_STRAFERIGHT:
			return set(false, false);
		case GAME_ACTION:
			if (CJ::aiming || pistol)
				return set(false, false);
			if (CJ::comboTime > 0.0f || CJ::punch || CJ::block)
				return set(true, CJ::punchToggle || CJ::blockPressed);
			return set(CJ::useHeld, CJ::useKeyPressed);
		case GAME_SLOW:
			return set(CJ::walkKey && !CJ::aiming, CJ::walkKeyPressed && !CJ::aiming);
		case GAME_SMOVE:
			return set(CJ::jumpKeyHeld && !CJ::aiming && !CJ::heavy, CJ::jumpKeyPressed && !CJ::aiming && !CJ::heavy);
		case GAME_SNEAK:
			return set(CJ::sneakKey && !CJ::aiming, CJ::sneakKeyPressed && !CJ::aiming);
		default:
			return false;
		}
	}

	float __fastcall zCInput_Win32_GetState(zCInput_Win32* self, void* vtable, unsigned short key);
	auto Hook_zCInput_Win32_GetState = Union::CreateHook(reinterpret_cast<void*>(0x004C8310), &zCInput_Win32_GetState, Union::HookType::Hook_Detours);
	float __fastcall zCInput_Win32_GetState(zCInput_Win32* self, void* vtable, unsigned short key)
	{
		float state;
		int toggled;
		return OverrideKey(key, state, toggled) ? state : Hook_zCInput_Win32_GetState(self, vtable, key);
	}

	int __fastcall zCInput_Win32_GetToggled(zCInput_Win32* self, void* vtable, unsigned short key);
	auto Hook_zCInput_Win32_GetToggled = Union::CreateHook(reinterpret_cast<void*>(0x004C8630), &zCInput_Win32_GetToggled, Union::HookType::Hook_Detours);
	int __fastcall zCInput_Win32_GetToggled(zCInput_Win32* self, void* vtable, unsigned short key)
	{
		float state;
		int toggled;
		if (!OverrideKey(key, state, toggled))
			return Hook_zCInput_Win32_GetToggled(self, vtable, key);
		if (toggled >= 0)
			return toggled;
		return state != 0.0f ? Hook_zCInput_Win32_GetToggled(self, vtable, key) : 0;
	}

	// During a punch the forward keys (W, or Home) and the left mouse button report the click
	// combo instead of the hardware, wherever Gothic reads them from.
	static bool ForwardKey(int key)
	{
		return CJ::meleeAttack && (key == KEY_W || key == KEY_HOME);
	}

	int __fastcall zCInput_Win32_KeyPressed(zCInput_Win32* self, void* vtable, int key);
	auto Hook_zCInput_Win32_KeyPressed = Union::CreateHook(reinterpret_cast<void*>(0x004C87A0), &zCInput_Win32_KeyPressed, Union::HookType::Hook_Detours);
	int __fastcall zCInput_Win32_KeyPressed(zCInput_Win32* self, void* vtable, int key)
	{
		if (ForwardKey(key))
			return CJ::virtualW ? 1 : 0;
		return Hook_zCInput_Win32_KeyPressed(self, vtable, key);
	}

	int __fastcall zCInput_Win32_KeyToggled(zCInput_Win32* self, void* vtable, int key);
	auto Hook_zCInput_Win32_KeyToggled = Union::CreateHook(reinterpret_cast<void*>(0x004C87B0), &zCInput_Win32_KeyToggled, Union::HookType::Hook_Detours);
	int __fastcall zCInput_Win32_KeyToggled(zCInput_Win32* self, void* vtable, int key)
	{
		if (ForwardKey(key))
			return CJ::virtualWEdge ? 1 : 0;
		return Hook_zCInput_Win32_KeyToggled(self, vtable, key);
	}

	int __fastcall zCInput_Win32_GetMouseButtonPressedLeft(zCInput_Win32* self, void* vtable);
	auto Hook_zCInput_Win32_GetMouseButtonPressedLeft = Union::CreateHook(reinterpret_cast<void*>(0x004C8C30), &zCInput_Win32_GetMouseButtonPressedLeft, Union::HookType::Hook_Detours);
	int __fastcall zCInput_Win32_GetMouseButtonPressedLeft(zCInput_Win32* self, void* vtable)
	{
		if (CJ::meleeAttack && CJ::comboTime > 0.0f)
			return 1;
		return Hook_zCInput_Win32_GetMouseButtonPressedLeft(self, vtable);
	}

	// Gothic turns the hero with the mouse and A/D here; with GTA controls CJ turns towards
	// the camera-relative direction instead.
	void __fastcall oCAIHuman_PC_Turnings(oCAIHuman* self, void* vtable, int force);
	auto Hook_oCAIHuman_PC_Turnings = Union::CreateHook(reinterpret_cast<void*>(0x00614800), &oCAIHuman_PC_Turnings, Union::HookType::Hook_Detours);
	void __fastcall oCAIHuman_PC_Turnings(oCAIHuman* self, void* vtable, int force)
	{
		if (!CJ::overrideInput)
			Hook_oCAIHuman_PC_Turnings(self, vtable, force);
	}

	void __fastcall zCAICamera_DoAI(zCAICamera* self, void* vtable, zCVob* vob, int& vobDeleted);
	auto Hook_zCAICamera_DoAI = Union::CreateHook(reinterpret_cast<void*>(0x0049D6A0), &zCAICamera_DoAI, Union::HookType::Hook_Detours);
	void __fastcall zCAICamera_DoAI(zCAICamera* self, void* vtable, zCVob* vob, int& vobDeleted)
	{
		Hook_zCAICamera_DoAI(self, vtable, vob, vobDeleted);
		CJ::UpdateCamera(self);
	}

	// GTA San Andreas HUD (CHud, gta-reversed Hud.cpp) in GTA's 640x448 HUD coordinates, scaled
	// to Gothic's 8192x8192 virtual screen like SCREEN_STRETCH_X/Y. Gothic health -> health bar,
	// ore -> money (counter rolling like GTA's), world time -> clock, armour protection -> armour
	// bar, mana (or breath while diving) -> the light blue bar, weapon icon and ammo, the aiming
	// crosshair, and "wasted" when CJ dies. Textures come from make_cj_hud.py.
	namespace CJ::Hud
	{
		// zCOLOR is stored b, g, r, alpha; filled in directly (no engine call while the DLL loads)
		static zCOLOR Rgb(int r, int g, int b)
		{
			zCOLOR c;
			c.r = (unsigned char)r;
			c.g = (unsigned char)g;
			c.b = (unsigned char)b;
			c.alpha = 255;
			return c;
		}

		// HudColours.cpp
		static const zCOLOR RED = Rgb(180, 25, 29), GREEN = Rgb(54, 104, 44), LIGHT_GRAY = Rgb(225, 225, 225),
			LIGHT_BLUE = Rgb(172, 203, 241), BLACK = Rgb(0, 0, 0), WHITE = Rgb(255, 255, 255);
		static const float SX = 8192.0f / 640.0f, SY = 8192.0f / 448.0f;

		struct View
		{
			zCView* view = nullptr;
			zSTRING texture;
			bool shown = false;

			void Set(const char* tex, const zCOLOR& color, float x, float y, float w, float h, int alpha = 255)
			{
				if (!view)
				{
					view = new zCView(0, 0, 8192, 8192);
					view->SetAlphaBlendFunc(zRND_ALPHA_FUNC_BLEND);
				}
				if (!(texture == tex))
				{
					texture = tex;
					view->InsertBack(texture);
				}
				view->SetColor(color);
				view->SetTransparency(alpha);
				view->SetPos(int(x * SX), int(y * SY));
				view->SetSize(int(w * SX + 0.5f) > 0 ? int(w * SX + 0.5f) : 1, int(h * SY + 0.5f));
				Show(true);
			}

			void Show(bool on)
			{
				if (!view || on == shown || !screen)
					return;
				shown = on;
				if (on)
					screen->InsertItem(view);
				else
					screen->RemoveItem(view);
			}
		};

		struct Bar
		{
			View border, back, fill;

			// CSprite2d::DrawBarChart: filled part in the colour, the rest at half brightness,
			// a 2 unit black border.
			void Set(float x, float y, float w, float h, float percent, const zCOLOR& color)
			{
				percent = percent < 0.0f ? 0.0f : (percent > 100.0f ? 100.0f : percent);
				zCOLOR dark = Rgb(color.r / 2, color.g / 2, color.b / 2);
				border.Set("CJ_HUD_WHITE.TGA", BLACK, x, y, w, h);
				back.Set("CJ_HUD_WHITE.TGA", dark, x + 2.0f, y + 2.0f, w - 4.0f, h - 4.0f);
				float filled = (w - 4.0f) * percent / 100.0f;
				if (filled >= 0.5f)
					fill.Set("CJ_HUD_WHITE.TGA", color, x + 2.0f, y + 2.0f, filled, h - 4.0f);
				else
					fill.Show(false);
			}

			void Show(bool on) { border.Show(on); back.Show(on); fill.Show(on); }
		};

		struct Text
		{
			View glyphs[9];

			// Pricedown, not proportional (fonts.dat UNPROP 20), scale 0.55 x 1.1, right aligned:
			// each character advances 20 * 0.55 units; the 32x40 glyph cell is drawn 17.6 x 22 and
			// sits at (16, 12) in its 64x64 texture.
			void Set(const char* text, const zCOLOR& color, float right, float y)
			{
				int n = (int)strlen(text);
				float advance = 20.0f * 0.55f, cellW = 32.0f * 0.55f, cellH = 20.0f * 1.1f;
				float x = right - advance * n;
				for (int i = 0; i < 9; i++)
				{
					if (i >= n)
					{
						glyphs[i].Show(false);
						continue;
					}
					char c = text[i];
					char name[32];
					if (c == '$')
						strcpy_s(name, "CJ_HUD_GLYPH_DOLLAR.TGA");
					else if (c == ':')
						strcpy_s(name, "CJ_HUD_GLYPH_COLON.TGA");
					else
						sprintf_s(name, "CJ_HUD_GLYPH_%c.TGA", c);
					glyphs[i].Set(name, color, x + i * advance - 16.0f * cellW / 32.0f, y - 12.0f * cellH / 40.0f,
						64.0f * cellW / 32.0f, 64.0f * cellH / 40.0f);
				}
			}

			void Show(bool on) { for (View& g : glyphs) g.Show(on); }
		};

		// CHud::DrawAmmo: the subtitles font at 0.3 x 0.7, proportional (fonts.dat font 1 widths of
		// the digits and '-', plus the edge of 1), centred.
		struct AmmoText
		{
			View glyphs[12];

			static float Advance(char c)
			{
				static const int DIGITS[10] = { 23, 15, 21, 21, 21, 21, 21, 21, 20, 21 };
				return ((c == '-' ? 14 : DIGITS[c - '0']) + 1) * 0.3f;
			}

			void Set(const char* text, const zCOLOR& color, float centre, float y)
			{
				int n = (int)strlen(text);
				float cellW = 32.0f * 0.3f, cellH = 20.0f * 0.7f, width = 0.0f;
				for (int i = 0; i < n; i++)
					width += Advance(text[i]);
				float x = centre - width / 2.0f;
				for (int i = 0; i < 12; i++)
				{
					if (i >= n)
					{
						glyphs[i].Show(false);
						continue;
					}
					char name[32];
					if (text[i] == '-')
						strcpy_s(name, "CJ_HUD_GLYPH_SUB_DASH.TGA");
					else
						sprintf_s(name, "CJ_HUD_GLYPH_SUB_%c.TGA", text[i]);
					glyphs[i].Set(name, color, x - 16.0f * cellW / 32.0f, y - 12.0f * cellH / 40.0f,
						64.0f * cellW / 32.0f, 64.0f * cellH / 40.0f);
					x += Advance(text[i]);
				}
			}

			void Show(bool on) { for (View& g : glyphs) g.Show(on); }
		};

		// CHud::DrawBustedWastedMessage: "wasted" (GXT DEAD) in FONT_GOTHIC (font2) at 2.1 x 2.1,
		// proportional (fonts.dat font 0 widths plus the edge of 3), centred, its bottom 30 units
		// above the screen's middle, light grey fading in by 0.4 alpha a millisecond.
		struct Wasted
		{
			View glyphs[6];
			float since = -1.0f;

			void Update(bool dead, float now)
			{
				if (!dead)
				{
					since = -1.0f;
					for (View& g : glyphs)
						g.Show(false);
					return;
				}
				if (since < 0.0f)
					since = now;
				float alpha = (now - since) * 1000.0f * 0.4f;
				static const char TEXT[] = "wasted";
				static const int PROP[6] = { 18, 11, 12, 8, 10, 11 }; // w a s t e d
				float scale = 2.1f, cellW = 32.0f * scale, cellH = 20.0f * scale, width = 0.0f;
				for (int p : PROP)
					width += (p + 3) * scale;
				float x = 320.0f - width / 2.0f, y = 224.0f - 30.0f - cellH;
				for (int i = 0; i < 6; i++)
				{
					char name[32];
					sprintf_s(name, "CJ_HUD_GLYPH_GOTH_%c.TGA", TEXT[i] - 'a' + 'A');
					glyphs[i].Set(name, LIGHT_GRAY, x - 16.0f * cellW / 32.0f, y - 12.0f * cellH / 40.0f,
						64.0f * cellW / 32.0f, 64.0f * cellH / 40.0f, alpha > 255.0f ? 255 : (int)alpha);
					x += (PROP[i] + 3) * scale;
				}
			}
		};

		static Bar health, armour, blue;
		static Text clock, money;
		static Wasted wasted;
		static int shownMoney = -1;       // the money counter, rolling towards the ore count
		static float moneyTicks = 0.0f;
		static AmmoText ammo;
		static View fist, crosshair[4], rocketSite[4];
		static int oreIndex = -1;
		static int frame = 0;

		static void Show(bool on)
		{
			health.Show(on); armour.Show(on); blue.Show(on); clock.Show(on); money.Show(on); fist.Show(on); ammo.Show(on);
			for (View& c : crosshair)
				c.Show(on);
			for (View& c : rocketSite)
				c.Show(on);
		}

		// CHud::DrawCrossHairs for an aimed gun: four quarters of sitem16 around the crosshair point,
		// each GetWeaponRadiusOnScreen * 32 units square (the pistol: 0.5 / accuracy 1.0 * 15 / range 35).
		static void Crosshair(bool on)
		{
			if (!on)
			{
				for (View& c : crosshair)
					c.Show(false);
				return;
			}
			float cx = 640.0f * CROSSHAIR_X, cy = 448.0f * CROSSHAIR_Y, s = 64.0f * (0.5f / 1.0f * 15.0f / 35.0f) / 2.0f;
			crosshair[0].Set("CJ_HUD_SITEM16_TL.TGA", WHITE, cx - s, cy - s, s, s);
			crosshair[1].Set("CJ_HUD_SITEM16_TR.TGA", WHITE, cx, cy - s, s, s);
			crosshair[2].Set("CJ_HUD_SITEM16_BL.TGA", WHITE, cx - s, cy, s, s);
			crosshair[3].Set("CJ_HUD_SITEM16_BR.TGA", WHITE, cx, cy, s, s);
		}

		// The rocket launcher's first-person crosshair (CHud::DrawCrossHairs, MODE_ROCKETLAUNCHER):
		// siterocket in the four corners around the middle of the screen, each 24 units square and
		// 20 units off the middle.
		static void RocketCrosshair(bool on)
		{
			static const char* const CORNERS[4] = { "TL", "TR", "BL", "BR" };
			for (int i = 0; i < 4; i++)
			{
				if (!on)
				{
					rocketSite[i].Show(false);
					continue;
				}
				char name[40];
				sprintf_s(name, "CJ_HUD_SITEROCKET_%s.TGA", CORNERS[i]);
				float x = 320.0f + (i % 2 ? 20.0f : -44.0f), y = 224.0f + (i / 2 ? 20.0f : -44.0f);
				rocketSite[i].Set(name, WHITE, x, y, 24.0f, 24.0f);
			}
		}

		static bool Visible(oCNpc* hero)
		{
			if (!settings.gtaHud || !hero || !ogame || !screen || !ogame->GetShowPlayerStatus())
				return false;
			zCAICamera* ai = zCAICamera::current;
			if (ai)
			{
				zSTRING mode = ai->curcammode;
				mode.Upper();
				if (mode == "CAMMODDIALOG")
					return false;
			}
			return true;
		}

		static void Update()
		{
			LoadSettings();
			oCNpc* hero = oCNpc::player;
			wasted.Update(settings.gtaHud && screen && hero && hero->IsDead(), Now());
			if (!Visible(hero))
			{
				Show(false);
				return;
			}
			frame++;

			int day = 0, hour = 0, minute = 0;
			ogame->GetTime(day, hour, minute);
			char text[16];
			sprintf_s(text, "%02d:%02d", hour, minute);
			clock.Set(text, LIGHT_GRAY, 640.0f - 32.0f, 22.0f);

			if (oreIndex < 0 && parser)
				oreIndex = parser->GetIndex("ITMINUGGET");
			int ore = oreIndex >= 0 ? hero->inventory2.GetAmount(oreIndex) : 0;
			// The counter rolls towards the amount, as GTA's does (CPlayerInfo::Process in GTA III and
			// VC, re3; SA keeps it): per 1/30 s tick by 12345, 1234, 123, 42 or 1 by how far off it is.
			if (shownMoney < 0)
				shownMoney = ore;
			moneyTicks += ztimer->frameTimeFloat / 1000.0f * 30.0f;
			for (; moneyTicks >= 1.0f && shownMoney != ore; moneyTicks -= 1.0f)
			{
				int diff = ore > shownMoney ? ore - shownMoney : shownMoney - ore;
				int step = diff > 100000 ? 12345 : diff > 10000 ? 1234 : diff > 1000 ? 123 : diff > 50 ? 42 : 1;
				shownMoney += ore > shownMoney ? step : -step;
			}
			if (moneyTicks > 1.0f)
				moneyTicks = 1.0f;
			sprintf_s(text, "$%08d", shownMoney > 99999999 ? 99999999 : shownMoney);
			money.Set(text, GREEN, 640.0f - 32.0f, 89.0f);

			// CHud::DrawWeapon: the weapon icon, and for a gun the rounds outside / in the clip under it
			float iconX = 640.0f - 32.0f - 0.17343046f * 640.0f;
			int gun = DrawnGun(hero), weapon = CurrentWeapon(hero);
			static const char* const ICONS[GUNS + 1] = { "CJ_HUD_COLT45.TGA", "CJ_HUD_ROCKETLA.TGA", "CJ_HUD_DILDO.TGA" };
			fist.Set(weapon >= 0 ? ICONS[weapon] : "CJ_HUD_FIST.TGA", WHITE, iconX, 20.0f, 47.0f, 58.0f);
			if (gun >= 0)
			{
				int total = AmmoCount(hero, gun), inClip = clips[gun] < 0 ? 0 : (clips[gun] > total ? total : clips[gun]);
				sprintf_s(text, "%d-%d", total - inClip, inClip);
				ammo.Set(text, LIGHT_BLUE, iconX + 47.0f / 2.0f, 43.0f + 20.0f);
			}
			else
				ammo.Show(false);
			Crosshair(aiming && aimBlend > 0.5f);
			RocketCrosshair(aiming && fpBlend > 0.5f);

			int hp = hero->attribute[NPC_ATR_HITPOINTS], hpMax = hero->attribute[NPC_ATR_HITPOINTSMAX];
			bool blink = hp * 10 < hpMax && (frame / 8) % 2; // CHud::RenderHealthBar flashes below 10 %
			if (blink)
				health.Show(false);
			else
				health.Set(640.0f - 141.0f, 77.0f, 109.0f, 9.0f, hpMax > 0 ? 100.0f * hp / hpMax : 0.0f, RED);

			int protection = hero->protection[2]; // oEDamageIndex_Edge
			if (protection > 1)
				armour.Set(640.0f - 94.0f, 48.0f, 62.0f, 9.0f, (float)protection, LIGHT_GRAY);
			else
				armour.Show(false);

			bool diving = (hero->GetBodyState() & BS_ONLY_STATE) == BS_DIVE;
			int mana = hero->attribute[NPC_ATR_MANA], manaMax = hero->attribute[NPC_ATR_MANAMAX];
			if (diving && hero->divetime > 0.0f)
				blue.Set(640.0f - 94.0f, 62.0f, 62.0f, 9.0f, 100.0f * hero->divectr / hero->divetime, LIGHT_BLUE);
			else if (manaMax > 0)
				blue.Set(640.0f - 94.0f, 62.0f, 62.0f, 9.0f, 100.0f * mana / manaMax, LIGHT_BLUE);
			else
				blue.Show(false);
		}
	}

	// GTA's radar (CHud::DrawRadar, CRadar in gta-reversed Radar.cpp), in GTA's 640x448 HUD units
	// stretched to the screen like the rest of the HUD:
	//  - the map: an ellipse (posX 40, 104 from the bottom, 94 x 76 units) of the radar map
	//    texture (make_cj_radar.py), centred on CJ and turned with the camera so that up on the
	//    radar is where the camera looks; radarRange from the centre to the rim (GTA on foot:
	//    180 m). Other worlds than WORLD.ZEN (the mines, the orc temple) get no map, as GTA's
	//    interiors;
	//  - the rim: hud.txd radardisc mirrored into the four corners, 4 units beyond the map, black;
	//  - blips (CRadar::ShowRadarTraceWithHeight): hostile NPCs, and any NPC fighting CJ, as red
	//    squares with a black edge, triangles pointing up or down when they are more than 2 m above
	//    or 4 m below CJ;
	//  - north (radar_north) on the rim, CJ's arrow (radar_centre) in the middle, turned to where
	//    he faces (CRadar::DrawRotatingRadarSprite, 8 units each side).
	// Drawn with Gothic's renderer before the screen's views (zCView::DrawItems), so Gothic's
	// windows (inventory, documents) cover it.
	namespace CJ::Radar
	{
		static const float CX = 87.0f, CY = 382.0f, RX = 47.0f, RY = 38.0f, RIM = 4.0f, SPRITE = 8.0f;
		static const int SEGMENTS = 48;
		static float pxX = 1.0f, pxY = 1.0f; // pixels per HUD unit

		struct Texture
		{
			const char* name;
			zCTexture* texture;
			bool tried;

			zCTexture* Get()
			{
				if (!tried)
				{
					tried = true;
					zSTRING file = name;
					texture = zCTexture::Load(file, 0);
					if (texture && zresMan)
						zresMan->CacheIn(texture, -1.0f);
					Log("radar: texture %s %s", name, texture ? "loaded" : "missing");
				}
				return texture;
			}
		};
		static Texture mapTexture = { "CJ_RADAR.TGA" }, discTexture = { "CJ_HUD_RADARDISC.TGA" },
			centreTexture = { "CJ_HUD_RADAR_CENTRE.TGA" }, northTexture = { "CJ_HUD_RADAR_NORTH.TGA" },
			whiteTexture = { "CJ_HUD_WHITE.TGA" };

		// A triangle fan (Gothic's DrawPolySimple), in pixels; at most 12 corners.
		static void Fan(zCTexture* texture, const float* xy, const float* uv, int n, const zCOLOR& color)
		{
			zTRndSimpleVertex v[12];
			for (int i = 0; i < n && i < 12; i++)
			{
				v[i].pos = zVEC2(xy[2 * i], xy[2 * i + 1]);
				v[i].z = 1.0f;
				v[i].uv = zVEC2(uv[2 * i], uv[2 * i + 1]);
				v[i].color = color;
			}
			if (texture)
				zrenderer->DrawPolySimple(texture, v, n < 12 ? n : 12);
		}

		// HUD units: a rectangle (u0 > u1 or v0 > v1 mirror the texture)
		static void Rect(zCTexture* texture, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, const zCOLOR& color)
		{
			float xy[8] = { x0 * pxX, y0 * pxY, x1 * pxX, y0 * pxY, x1 * pxX, y1 * pxY, x0 * pxX, y1 * pxY };
			float uv[8] = { u0, v0, u1, v0, u1, v1, u0, v1 };
			Fan(texture, xy, uv, 4, color);
		}

		// HUD units: a sprite SPRITE units each side of (x, y), turned clockwise by `angle` (radians)
		static void Sprite(zCTexture* texture, float x, float y, float angle, const zCOLOR& color)
		{
			static const float CORNERS[4][2] = { { -1.0f, -1.0f }, { 1.0f, -1.0f }, { 1.0f, 1.0f }, { -1.0f, 1.0f } };
			float c = cosf(angle), s = sinf(angle), xy[8], uv[8];
			for (int i = 0; i < 4; i++)
			{
				float ox = CORNERS[i][0] * SPRITE, oy = CORNERS[i][1] * SPRITE;
				xy[2 * i] = (x + ox * c - oy * s) * pxX;
				xy[2 * i + 1] = (y + ox * s + oy * c) * pxY;
				uv[2 * i] = CORNERS[i][0] > 0.0f ? 1.0f : 0.0f;
				uv[2 * i + 1] = CORNERS[i][1] > 0.0f ? 1.0f : 0.0f;
			}
			Fan(texture, xy, uv, 4, color);
		}

		// CRadar::ShowRadarTraceWithHeight: `size` (SCREEN_STRETCH_X units, so square on screen)
		// each side of the point; 0 level, 1 above (triangle up), -1 below (triangle down).
		static void Blip(float x, float y, float size, int height, const zCOLOR& color)
		{
			zCTexture* white = whiteTexture.Get();
			float uv[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
			x *= pxX;
			y *= pxY;
			for (int pass = 0; pass < 2; pass++)
			{
				float h = (size + (pass == 0 ? 1.0f : 0.0f)) * pxX; // black edge first
				const zCOLOR& c = pass == 0 ? Hud::BLACK : color;
				if (height == 0)
				{
					float xy[8] = { x - h, y - h, x + h, y - h, x + h, y + h, x - h, y + h };
					Fan(white, xy, uv, 4, c);
				}
				else
				{
					float tip = height > 0 ? -h : h, edge = height > 0 ? h : -h;
					float xy[6] = { x, y + tip, x + h, y + edge, x - h, y + edge };
					Fan(white, xy, uv, 3, c);
				}
			}
		}

		// Only WORLD.ZEN has a map.
		static bool MapWorld(oCNpc* hero)
		{
			static zCWorld* known = nullptr;
			static bool mapped = false;
			zCWorld* world = hero->GetHomeWorld();
			if (world != known)
			{
				known = world;
				oCWorld* game = zDYNAMIC_CAST<oCWorld>(world);
				zSTRING file = game ? game->GetWorldFilename() : zSTRING("");
				file.Upper();
				const char* name = file.ToChar();
				size_t n = strlen(name);
				mapped = n >= 9 && strcmp(name + n - 9, "WORLD.ZEN") == 0 && (n == 9 || name[n - 10] == '\\' || name[n - 10] == '/');
				Log("radar: world %s, %s", name, mapped ? "map" : "no map");
			}
			return mapped && settings.radarX1 > settings.radarX0 && settings.radarZ1 > settings.radarZ0;
		}

		static void Draw()
		{
			oCNpc* hero = oCNpc::player;
			if (!settings.radar || !zrenderer || !Hud::Visible(hero) || hero->IsDead() || !hero->GetHomeWorld())
				return;
			int width = zrenderer->vid_xdim, height = zrenderer->vid_ydim;
			if (width <= 0 || height <= 0)
				return;
			pxX = width / 640.0f;
			pxY = height / 448.0f;

			zCAICamera* ai = zCAICamera::current;
			zVEC3 forward = cameraActive ? cameraForward
				: Flat(ai && ai->camVob ? ai->camVob->GetAtVectorWorld() : hero->GetAtVectorWorld());
			zVEC3 right = RightOf(forward, hero);
			zVEC3 centre = hero->GetPositionWorld();
			float range = (settings.radarRange > 500.0f ? settings.radarRange : 500.0f) * Bike::RadarScale();

			// the 2D state zCView::Blit draws in, restored afterwards
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
			zrenderer->SetBilerpFilterEnabled(TRUE);
			zrenderer->SetTextureWrapEnabled(FALSE);
			zrenderer->SetFog(FALSE);

			// the map: fans from the centre, each vertex at the world point it shows
			bool map = MapWorld(hero);
			zCTexture* mapTex = map ? mapTexture.Get() : nullptr;
			float spanX = settings.radarX1 - settings.radarX0, spanZ = settings.radarZ1 - settings.radarZ0;
			auto vertex = [&](float rx, float ry, float* xy, float* uv)
			{
				xy[0] = (CX + rx * RX) * pxX;
				xy[1] = (CY + ry * RY) * pxY;
				zVEC3 world = centre + right * (rx * range) - forward * (ry * range);
				uv[0] = map ? (world[0] - settings.radarX0) / spanX : 0.5f;
				uv[1] = map ? (settings.radarZ1 - world[2]) / spanZ : 0.5f;
			};
			const int PER_FAN = 8;
			for (int first = 0; first < SEGMENTS; first += PER_FAN)
			{
				float xy[2 * (PER_FAN + 2)], uv[2 * (PER_FAN + 2)];
				vertex(0.0f, 0.0f, xy, uv);
				for (int i = 0; i <= PER_FAN; i++)
				{
					float a = 2.0f * PI * (first + i) / SEGMENTS;
					vertex(cosf(a), sinf(a), xy + 2 * (i + 1), uv + 2 * (i + 1));
				}
				if (mapTex)
					Fan(mapTex, xy, uv, PER_FAN + 2, Hud::WHITE);
				else // no map here: a dark disc, as GTA's radar indoors
				{
					zCOLOR dark = Hud::BLACK;
					dark.alpha = 140;
					Fan(whiteTexture.Get(), xy, uv, PER_FAN + 2, dark);
				}
			}

			// the rim
			zCTexture* disc = discTexture.Get();
			float l = CX - RX - RIM, r = CX + RX + RIM, t = CY - RY - RIM, b = CY + RY + RIM;
			Rect(disc, l, t, CX, CY, 0.0f, 0.0f, 1.0f, 1.0f, Hud::BLACK);
			Rect(disc, CX, t, r, CY, 1.0f, 0.0f, 0.0f, 1.0f, Hud::BLACK);
			Rect(disc, l, CY, CX, b, 0.0f, 1.0f, 1.0f, 0.0f, Hud::BLACK);
			Rect(disc, CX, CY, r, b, 1.0f, 1.0f, 0.0f, 0.0f, Hud::BLACK);

			// hostile NPCs inside the radar's range
			oCWorld* game = ogame ? ogame->GetGameWorld() : nullptr;
			for (zCListSort<oCNpc>* node = game && game->voblist_npcs ? game->voblist_npcs->next : nullptr; node; node = node->next)
			{
				oCNpc* npc = node->data;
				if (!npc || npc == hero || npc->IsDead() || !npc->GetHomeWorld())
					continue;
				zVEC3 offset = npc->GetPositionWorld() - centre;
				float rx = offset.Dot(right) / range, ry = -offset.Dot(forward) / range;
				if (rx * rx + ry * ry > 1.0f)
					continue;
				bool fighting = npc->enemy == hero && npc->GetWeaponMode() != NPC_WEAPON_NONE;
				if (!fighting && npc->GetAttitude(hero) != NPC_ATT_HOSTILE)
					continue;
				int above = offset[1] > 200.0f ? 1 : (offset[1] < -400.0f ? -1 : 0);
				Blip(CX + rx * RX, CY + ry * RY, 2.0f, above, Hud::RED);
			}

			// north on the rim, CJ's arrow in the middle
			float nx = zVEC3(0.0f, 0.0f, 1.0f).Dot(right), ny = -zVEC3(0.0f, 0.0f, 1.0f).Dot(forward);
			float length = sqrtf(nx * nx + ny * ny);
			if (length > 0.001f)
				Sprite(northTexture.Get(), CX + nx / length * RX, CY + ny / length * RY, 0.0f, Hud::WHITE);
			zVEC3 at = Flat(hero->GetAtVectorWorld());
			Sprite(centreTexture.Get(), CX, CY, atan2f(at.Dot(right), at.Dot(forward)), Hud::WHITE);

			zrenderer->SetZBufferWriteEnabled(zWrite);
			zrenderer->SetZBufferCompare(zCompare);
			zrenderer->SetAlphaBlendFunc(blend);
			zrenderer->SetBilerpFilterEnabled(bilerp);
			zrenderer->SetTextureWrapEnabled(wrap);
			zrenderer->SetFog(fog);
		}
	}

	void __fastcall zCView_DrawItems(zCView* self, void* vtable);
	auto Hook_zCView_DrawItems = Union::CreateHook(reinterpret_cast<void*>(0x006FCBD0), &zCView_DrawItems, Union::HookType::Hook_Detours);
	void __fastcall zCView_DrawItems(zCView* self, void* vtable)
	{
		if (self == screen)
			CJ::Radar::Draw();
		Hook_zCView_DrawItems(self, vtable);
	}

	// GTA's clock: a game minute every real second (CClock::ms_nMillisecondsPerGameMinute 1000).
	// Gothic's world timer adds the frame time to the time of day, an hour being 250000 ms; with
	// gtaClock it adds 250000 / 60000 times as much.
	void __fastcall oCWorldTimer_Timer(oCWorldTimer* self, void* vtable);
	auto Hook_oCWorldTimer_Timer = Union::CreateHook(reinterpret_cast<void*>(0x006D7B40), &oCWorldTimer_Timer, Union::HookType::Hook_Detours);
	void __fastcall oCWorldTimer_Timer(oCWorldTimer* self, void* vtable)
	{
		CJ::LoadSettings();
		if (CJ::settings.gtaClock && ztimer)
			self->worldTime += ztimer->frameTimeFloat * (250000.0f / 60000.0f - 1.0f);
		Hook_oCWorldTimer_Timer(self, vtable);
	}

	// Gothic's own health, mana and breath bars give way to the GTA HUD.
	void __fastcall oCGame_UpdatePlayerStatus(oCGame* self, void* vtable);
	auto Hook_oCGame_UpdatePlayerStatus = Union::CreateHook(reinterpret_cast<void*>(0x00638F90), &oCGame_UpdatePlayerStatus, Union::HookType::Hook_Detours);
	void __fastcall oCGame_UpdatePlayerStatus(oCGame* self, void* vtable)
	{
		Hook_oCGame_UpdatePlayerStatus(self, vtable);
		if (CJ::settings.gtaHud && screen)
			for (zCView* bar : { (zCView*)self->hpBar, (zCView*)self->manaBar, (zCView*)self->swimBar })
				if (bar)
					screen->RemoveItem(bar);
	}

	void __fastcall oCGame_Render(oCGame* self, void* vtable);
	auto Hook_oCGame_Render = Union::CreateHook(reinterpret_cast<void*>(0x0063DBE0), &oCGame_Render, Union::HookType::Hook_Detours);
	void __fastcall oCGame_Render(oCGame* self, void* vtable)
	{
		CJ::Hud::Update();
		CJ::UpdateBursts();
		CJ::UpdateFlashes();
		CJ::UpdateMissiles();
		Hook_oCGame_Render(self, vtable);
	}

	// Pistol rounds deal GTA's damage (see CJ::Shoot), and so do CJ's punches: melee.dat UNARMED_1
	// gives 6, 9 and 15 for the combo's hits and 5 for the moving punch, out of a ped's 100 health,
	// and the dildo's (DILDO: 18, 18, 25, moving 10). As that share of the target's health, on top
	// of its blunt protection so the amount gets through; Gothic then applies it as usual (knock-out,
	// death, reactions). Whoever the dildo hits hears GTA's dildo hit and runs from CJ.
	void __fastcall oCNpc_OnDamage_Hit(oCNpc* self, void* vtable, oCNpc::oSDamageDescriptor& desc);
	auto Hook_oCNpc_OnDamage_Hit = Union::CreateHook(reinterpret_cast<void*>(0x00731410), &oCNpc_OnDamage_Hit, Union::HookType::Hook_Detours);
	void __fastcall oCNpc_OnDamage_Hit(oCNpc* self, void* vtable, oCNpc::oSDamageDescriptor& desc)
	{
		oCNpc* hero = oCNpc::player;
		if (self == CJ::blastTarget)
		{
			// a rocket's blast (CJ::Explode) or the bike running someone down (CJ::Bike::HitPeople):
			// its damage on top of the blunt protection, and Gothic's fly damage throwing the victim
			// away from it
			int damage = (int)(CJ::blastDamage + 0.5f);
			damage = damage < 1 ? 1 : damage;
			int blunt = self->protection[oEDamageIndex_Blunt], fly = self->protection[oEDamageIndex_Fly];
			for (unsigned long& d : desc.aryDamage)
				d = 0;
			desc.aryDamage[oEDamageIndex_Blunt] = damage + blunt;
			desc.aryDamage[oEDamageIndex_Fly] = fly + 1;
			desc.fDamageTotal = (float)(damage + blunt + fly + 1);
			desc.enuModeDamage = oEDamageType_Blunt | oEDamageType_Fly;
			desc.vecDirectionFly = CJ::blastDir;
			desc.dwFieldsValid |= oCNpc::oEDamageDescFlag_Damage | oCNpc::oEDamageDescFlag_DamageType | oCNpc::oEDamageDescFlag_FlyDirection;
			if (CJ::blastLethal) // a rocket is no melee hit: Gothic's knock-out rule is for those
			{
				desc.enuModeWeapon = oETypeWeapon_Range;
				desc.dwFieldsValid |= oCNpc::oEDamageDescFlag_WeaponType;
			}
		}
		else if (hero && self == CJ::gunTarget && desc.pNpcAttacker == hero)
		{
			// a pistol round (CJ::Shoot): GTA's 25 of 100 health as that share of the target's, on top
			// of its protection against missiles, as point damage
			int health = self->attribute[NPC_ATR_HITPOINTSMAX];
			int damage = (int)(CJ::GUN_DAMAGE / 100.0f * health * CJ::settings.gunDamage + 0.5f);
			damage = damage < 1 ? 1 : damage;
			int protection = self->protection[oEDamageIndex_Point];
			for (unsigned long& d : desc.aryDamage)
				d = 0;
			desc.aryDamage[oEDamageIndex_Point] = damage + protection;
			desc.fDamageTotal = (float)(damage + protection);
			desc.enuModeDamage = oEDamageType_Point;
			desc.dwFieldsValid |= oCNpc::oEDamageDescFlag_Damage;
			CJ::Log("pistol hit on %s: %d of %d health (+%d protection)", self->GetInstanceName().ToChar(), damage, health, protection);
		}
		else if (hero && desc.pNpcAttacker == hero && self != hero && (hero->GetWeaponMode() == NPC_WEAPON_FIST || CJ::DildoDrawn(hero)))
		{
			bool dildo = hero->GetWeaponMode() != NPC_WEAPON_FIST;
			oCAniCtrl_Human* ani = hero->GetAnictrl();
			bool moving = ani && ani->IsInCombo() == 0 && CJ::moving;
			static const float COMBO[3] = { 6.0f, 9.0f, 15.0f };
			int hit = ani ? ani->comboNr : 0;
			hit = hit < 0 ? 0 : (hit > 2 ? 2 : hit);
			float gta = dildo ? (moving ? CJ::DILDO_MOVING_DAMAGE : CJ::DILDO_DAMAGE[hit]) : (moving ? 5.0f : COMBO[hit]);
			int health = self->attribute[NPC_ATR_HITPOINTSMAX];
			int damage = (int)(gta / 100.0f * health * CJ::settings.fistDamage + 0.5f);
			damage = damage < 1 ? 1 : damage;
			int protection = self->protection[oEDamageIndex_Blunt];
			CJ::Log("%s hit %d on %s: Gothic damage %lu, GTA %.0f%% of %d health = %d (+%d protection)", dildo ? "dildo" : "punch",
				hit + 1, self->GetInstanceName().ToChar(), desc.aryDamage[oEDamageIndex_Blunt], gta, health, damage, protection);
			if (dildo) // all of it blunt (Gothic would add the wielder's strength to the weapon's)
			{
				for (unsigned long& d : desc.aryDamage)
					d = 0;
				desc.enuModeDamage = oEDamageType_Blunt;
				desc.dwFieldsValid |= oCNpc::oEDamageDescFlag_DamageType;
			}
			desc.aryDamage[oEDamageIndex_Blunt] = damage + protection;
			desc.fDamageTotal = (float)(damage + protection);
			desc.dwFieldsValid |= oCNpc::oEDamageDescFlag_Damage;
			Hook_oCNpc_OnDamage_Hit(self, vtable, desc);
			if (dildo)
			{
				CJ::PlaySfx(CJ::dildoSound, "CJ_DILDO_HIT.WAV", self);
				CJ::Scare(self, hero, true); // after Gothic's own reaction to the hit, which it replaces
			}
			return;
		}
		Hook_oCNpc_OnDamage_Hit(self, vtable, desc);
	}
}
