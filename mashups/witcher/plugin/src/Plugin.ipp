// The Witcher 3's Geralt as Gothic 1's hero (GothicMod.exe 1.08k_mod). make_geralt_model.py and
// make_geralt_anims.py give the hero Geralt's body and his own animations (GERALT.MDS); this
// plugin gives him The Witcher 3's controls, camera, movement and combat. Numbers come from the
// game's scripts and animation events (research/combat.md, research/controls_hud.md in the repo
// folder), times are 1:1, damage is a share of the victim's health (the Witcher's 3500-health
// scale has no Gothic counterpart).
//
// Full detail: Gothic draws characters as progressive meshes and thins them out with distance
// (zCProgMeshProto::CalcLODRenderVertPerc keeps full detail only within lodParams.zDisplace2,
// squared cm, and drops vertices down to minVerts beyond, scaled by modelDetail in Gothic.ini).
// Geralt is fifteen soft skins (Gothic renders one only below 2048 positions), and each thinned
// on its own, tearing and smearing him; every soft skin of the hero's model is kept in full.
//
// Controls (The Witcher 3's PC defaults): W/A/S/D move relative to the camera, Geralt turning
// towards the way to go (no strafing outside the dodges yet); the mouse turns the camera; Left
// Ctrl toggles walking; Left Shift held sprints (GERALT_SPRINT.MDS, The Witcher 3's sprint cycle,
// eased in over sprintAccel); Space jumps (Gothic's jump, Geralt's animations); E talks, takes,
// opens and uses (Gothic's action); 1 / 2 draw the sword (steel / silver: Gothic's one-handed
// weapon until the Witcher's swords are items), C sheathes it; a left click with no sword out
// draws it. Gothic's own movement, weapon, look and number keys do nothing while these are on;
// its menu keys (inventory, status, log, map) stay.
//
// Sword fighting (Gothic's one-handed mode; Gothic's own swing combo is not used):
// - Left click: a fast swing, Shift + left click: a strong one, each chosen and played by the
//   plugin as the game's combat.ws picks them (see Swing): by the distance to the target (normal,
//   extra-distance, far and far-extra gap closers, run-in swings while sprinting), alternating
//   stances; he slides onto the target and turns to it during the wind-up. The hit lands at the
//   end of the swing's PreAttack event (fast 0.39 s, far 0.69 s, strong 0.69 s, strong far
//   0.79 s) on everyone within swingReach in a 110-degree cone. A press during an action is kept
//   (the game's one-action buffer) and fires at the action's ActionBlend (fast 0.60 s, far
//   0.90 s, strong 1.10 s, strong far 1.20 s).
// - Alt (or a direction key tapped twice within 0.2 s): a dodge, back, left or right Gothic's own
//   jump back and side steps (Geralt's dodges in GERALT.MDS), forward played here; Space: a roll
//   (6.4 m). For 0.6 s of a dodge (0.8 s of a roll) melee hits miss him, unless he dodged
//   straight at the attacker (within 30 degrees).
// - Right button held: his guard. Melee hits from in front (within 90 degrees) are parried; if
//   the button went down within counterWindow seconds before the hit, Geralt counters instead:
//   he kicks and the attacker falls.
// - Damage of his hits: fastDamage / strongDamage of the victim's maximum health (the game's strong
//   swing does 1.93 times a fast one), +-10 %, a 5 % critical chance (+50 % from behind) for +25 %.
//   The game's steel sword is for people (guilds below Gothic's GIL_HUMANS separator) and the silver
//   one for monsters; until his silver sword is in, wrongSword sets the share against the other kind
//   (1 = full).
//
// Signs: Q casts the chosen one (3 Aard, 4 Yrden, 5 Igni, 6 Quen, 7 Axii), with the game's
// casting animations (with or without the sword out), the effect at its cast_throw event:
// - Aard: a 60-degree cone of 7 m: knocks down (signKnockdown chance) or staggers those in it.
// - Igni: a 60-degree cone of 5 m: igniDamage of their health as fire, igniBurn chance of burning
//   for 3 s (2.7 % of maximum health a second).
// - Quen: a shield for 30 s that takes the whole of the next hit.
// - Yrden: a circle of 2.6 m for 10 s around where he cast it; enemies in it move at 0.74 speed.
// - Axii: the enemy he faces within 15 m stands dazed for 5 s (Gothic's magic sleep state).
// Every sign takes the full stamina bar (100); stamina comes back at 10 a second while enemies are
// near and 100 a second otherwise (the game's numbers at level 1).
//
// Gothic armour would replace Geralt's body with a Gothic armour mesh: the hero cannot put
// armour on.
//
// Tuning, read from Gothic.ini section [WITCHER] at start (defaults in brackets):
//   controls=1          The Witcher 3's controls, camera and combat (0: Gothic's own)
//   turnRate=720        degrees a second Geralt turns towards the way to go standing or walking,
//                       turnRateRun=450 running, turnRateSprint=300 sprinting
//   gameTurns=1         the game's own turns (Witcher_turns.txt from make_geralt_anims.py): a start
//                       towards the side or back pivots him into it, a reversal while running,
//                       walking or sprinting is the game's 180-degree turn
//   camAutoCenter=1     while he moves, the camera swings in behind him camAutoDelay=5 s after the
//                       mouse last turned it (camAutoDelayPitch=3 s for the pitch, back to level),
//                       over camAutoTime=1.2 s (the game's AutoCameraCenter)
//   mouseSpeed=0.25     camera degrees per mouse unit; invertMouseY=0
//   camDistance=185     exploration camera: distance (cm, the game's modern camera 1.85 m),
//   camSide=55          its offset to the right, camBack=60 further back, camHeight=130 the
//                       point it orbits above Geralt's feet
//   combatDistance=150  the camera with an enemy near and the sword out: distance, combatSide=74,
//   combatBack=38, combatUp=15, combatHeight=115
//   sprintAccel=0.5     seconds from running to full sprint; sprintDecel=0.4 back
//   slopeWalk=60        steepest ground (degrees) Geralt walks without Gothic sliding him down,
//   slopeSlide=70       and where its slide starts
//   maxFrameTime=40     the longest step (ms) one frame may take while the controls are on
//   fastDamage=0.15     share of the victim's maximum health a fast swing takes; strongDamage=0.29
//   swingReach=160      reach (cm) of a swing past the victim's edge (the game's slash_short 1.6 m)
//   swingStop=60        how far (cm) from the target's edge a swing's slide stops
//   swingStep=50        how far (cm) a swing at no one moves him; swingTurn=900 degrees a second
//                       he turns onto the target during the wind-up
//   wrongSword=1        damage share with the wrong sword for the victim (see above)
//   killHumans=0        1: people he beats down die (The Witcher's way) instead of Gothic's
//                       knock-out (which its story counts on)
//   counterWindow=0.45  seconds before a hit in which a guard press counters it
//   signKnockdown=0.5   Aard's knock-down chance; igniDamage=0.2, igniBurn=0.5
//   startSword=1        give Geralt a one-handed sword if he has no melee weapon
//   debugLog=1          write the plugin's events to Witcher.log (Gothic folder)
//   face=1              his face moves (make_geralt_model.py's face bones): talking while he speaks,
//                       blinking, a grimace when hit, the expressions scripts set
//   voice=1             Geralt's own lines now and then (make_geralt_voice.py): a fight starting,
//                       a kill, badly hurt, exploring; voiceSubtitles=1, voiceVolume=1,
//   voiceGap=8          seconds at least between two, voiceIdleMin=90 / voiceIdleMax=240 seconds
//                       between his comments while exploring

namespace GOTHIC_NAMESPACE
{
	namespace Witcher
	{
		struct Settings
		{
			bool controls = true;
			float turnRate = 720.0f, turnRateRun = 450.0f, turnRateSprint = 300.0f;
			bool gameTurns = true;
			bool camAutoCenter = true;
			float camAutoDelay = 5.0f, camAutoDelayPitch = 3.0f, camAutoTime = 1.2f;
			float mouseSpeed = 0.25f;
			bool invertMouseY = false;
			float camDistance = 185.0f, camSide = 55.0f, camBack = 60.0f, camHeight = 130.0f;
			float combatDistance = 150.0f, combatSide = 74.0f, combatBack = 38.0f, combatUp = 15.0f, combatHeight = 115.0f;
			float sprintAccel = 0.5f, sprintDecel = 0.4f;
			float slopeWalk = 60.0f, slopeSlide = 70.0f;
			float maxFrameTime = 40.0f;
			float fastDamage = 0.15f, strongDamage = 0.29f;
			float swingReach = 160.0f, swingStop = 60.0f, swingStep = 50.0f, swingTurn = 900.0f;
			float wrongSword = 1.0f;
			bool killHumans = false;
			float counterWindow = 0.45f;
			float signKnockdown = 0.5f, igniDamage = 0.2f, igniBurn = 0.5f;
			bool startSword = true;
			bool debugLog = true;
			bool voice = true, voiceSubtitles = true;
			bool face = true;
			float voiceVolume = 1.0f, voiceGap = 8.0f, voiceIdleMin = 90.0f, voiceIdleMax = 240.0f;
		};

		static Settings settings;
		static bool settingsLoaded = false;
		static bool started = false; // the first frame has been logged
		static const float PI = 3.14159265f;

		static void LoadSettings()
		{
			if (settingsLoaded || !zoptions)
				return;
			settingsLoaded = true;
			zSTRING s = "WITCHER";
			settings.controls = zoptions->ReadBool(s, "controls", 1) != 0;
			settings.turnRate = zoptions->ReadReal(s, "turnRate", settings.turnRate);
			settings.turnRateRun = zoptions->ReadReal(s, "turnRateRun", settings.turnRateRun);
			settings.turnRateSprint = zoptions->ReadReal(s, "turnRateSprint", settings.turnRateSprint);
			settings.gameTurns = zoptions->ReadBool(s, "gameTurns", 1) != 0;
			settings.camAutoCenter = zoptions->ReadBool(s, "camAutoCenter", 1) != 0;
			settings.camAutoDelay = zoptions->ReadReal(s, "camAutoDelay", settings.camAutoDelay);
			settings.camAutoDelayPitch = zoptions->ReadReal(s, "camAutoDelayPitch", settings.camAutoDelayPitch);
			settings.camAutoTime = zoptions->ReadReal(s, "camAutoTime", settings.camAutoTime);
			settings.mouseSpeed = zoptions->ReadReal(s, "mouseSpeed", settings.mouseSpeed);
			settings.invertMouseY = zoptions->ReadBool(s, "invertMouseY", 0) != 0;
			settings.camDistance = zoptions->ReadReal(s, "camDistance", settings.camDistance);
			settings.camSide = zoptions->ReadReal(s, "camSide", settings.camSide);
			settings.camBack = zoptions->ReadReal(s, "camBack", settings.camBack);
			settings.camHeight = zoptions->ReadReal(s, "camHeight", settings.camHeight);
			settings.combatDistance = zoptions->ReadReal(s, "combatDistance", settings.combatDistance);
			settings.combatSide = zoptions->ReadReal(s, "combatSide", settings.combatSide);
			settings.combatBack = zoptions->ReadReal(s, "combatBack", settings.combatBack);
			settings.combatUp = zoptions->ReadReal(s, "combatUp", settings.combatUp);
			settings.combatHeight = zoptions->ReadReal(s, "combatHeight", settings.combatHeight);
			settings.sprintAccel = zoptions->ReadReal(s, "sprintAccel", settings.sprintAccel);
			settings.sprintDecel = zoptions->ReadReal(s, "sprintDecel", settings.sprintDecel);
			settings.slopeWalk = zoptions->ReadReal(s, "slopeWalk", settings.slopeWalk);
			settings.slopeSlide = zoptions->ReadReal(s, "slopeSlide", settings.slopeSlide);
			settings.maxFrameTime = zoptions->ReadReal(s, "maxFrameTime", settings.maxFrameTime);
			settings.fastDamage = zoptions->ReadReal(s, "fastDamage", settings.fastDamage);
			settings.strongDamage = zoptions->ReadReal(s, "strongDamage", settings.strongDamage);
			settings.swingReach = zoptions->ReadReal(s, "swingReach", settings.swingReach);
			settings.swingStop = zoptions->ReadReal(s, "swingStop", settings.swingStop);
			settings.swingStep = zoptions->ReadReal(s, "swingStep", settings.swingStep);
			settings.swingTurn = zoptions->ReadReal(s, "swingTurn", settings.swingTurn);
			settings.wrongSword = zoptions->ReadReal(s, "wrongSword", settings.wrongSword);
			settings.killHumans = zoptions->ReadBool(s, "killHumans", 0) != 0;
			settings.counterWindow = zoptions->ReadReal(s, "counterWindow", settings.counterWindow);
			settings.signKnockdown = zoptions->ReadReal(s, "signKnockdown", settings.signKnockdown);
			settings.igniDamage = zoptions->ReadReal(s, "igniDamage", settings.igniDamage);
			settings.igniBurn = zoptions->ReadReal(s, "igniBurn", settings.igniBurn);
			settings.startSword = zoptions->ReadBool(s, "startSword", 1) != 0;
			settings.debugLog = zoptions->ReadBool(s, "debugLog", 1) != 0;
			settings.voice = zoptions->ReadBool(s, "voice", 1) != 0;
			settings.face = zoptions->ReadBool(s, "face", 1) != 0;
			settings.voiceSubtitles = zoptions->ReadBool(s, "voiceSubtitles", 1) != 0;
			settings.voiceVolume = zoptions->ReadReal(s, "voiceVolume", settings.voiceVolume);
			settings.voiceGap = zoptions->ReadReal(s, "voiceGap", settings.voiceGap);
			settings.voiceIdleMin = zoptions->ReadReal(s, "voiceIdleMin", settings.voiceIdleMin);
			settings.voiceIdleMax = zoptions->ReadReal(s, "voiceIdleMax", settings.voiceIdleMax);
		}

		static void Log(const char* format, ...)
		{
			if (!settings.debugLog)
				return;
			FILE* file = nullptr;
			if (fopen_s(&file, "Witcher.log", "a") || !file)
				return;
			va_list args;
			va_start(args, format);
			fprintf(file, "%8.2f  ", ztimer ? ztimer->totalTimeFloat / 1000.0f : 0.0f);
			vfprintf(file, format, args);
			fputc('\n', file);
			va_end(args);
			fclose(file);
		}

		static float Now() { return ztimer ? ztimer->totalTimeFloat / 1000.0f : 0.0f; }
		static float FrameSeconds() { return ztimer ? ztimer->frameTimeFloat / 1000.0f : 0.0f; }

		static float Random() { return (float)rand() / (float)RAND_MAX; }

		// --- vectors (Gothic: Y up, centimetres) --------------------------------------------------
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

		// Horizontal right-hand side of a horizontal forward vector, in the engine's own handedness
		// (taken from the hero's at/right vectors, as the GTA mashup's CJMovement does).
		static zVEC3 RightOf(const zVEC3& forward, oCNpc* hero)
		{
			zVEC3 up(0.0f, 1.0f, 0.0f);
			zVEC3 cross = Cross(forward, up);
			zVEC3 heroCross = Cross(Flat(hero->GetAtVectorWorld()), up);
			float sign = heroCross.Dot(Flat(hero->GetRightVectorWorld())) >= 0.0f ? 1.0f : -1.0f;
			return Flat(cross * sign);
		}

		// Angle (degrees) between a horizontal direction and the way from `from` to `to`.
		static float AngleTo(const zVEC3& dir, const zVEC3& from, const zVEC3& to)
		{
			float dot = Flat(dir).Dot(Flat(to - from));
			dot = dot > 1.0f ? 1.0f : (dot < -1.0f ? -1.0f : dot);
			return acosf(dot) * 180.0f / PI;
		}

		// --- input ---------------------------------------------------------------------------------
		static bool KeyDown(int key)
		{
			return zinput && zinput->KeyPressed(key) != 0;
		}

		// Gothic's own button query can stay 0 while it holds the mouse through DirectInput, so
		// the Windows button state is read (only while Gothic is the active window).
		static bool GameHasFocus()
		{
			DWORD process = 0;
			HWND window = GetForegroundWindow();
			return window && GetWindowThreadProcessId(window, &process) && process == GetCurrentProcessId();
		}

		static bool MouseDown(bool left)
		{
			return (GetAsyncKeyState(left ? VK_LBUTTON : VK_RBUTTON) & 0x8000) && GameHasFocus();
		}

		// A key's state this frame and whether it went down this frame.
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

		static Key keyW, keyA, keyS, keyD, keyCtrl, keyShift, keyAlt, keySpace, keyE, keyQ, keyC, keyZ;
		static Key key1, key2, key3, key4, key5, key6, key7, mouseLeft, mouseRight;
		static float lastTap[4] = { -10.0f, -10.0f, -10.0f, -10.0f }; // W, A, S, D: double taps

		static void ReadKeys()
		{
			keyW.Update(KeyDown(KEY_W));
			keyA.Update(KeyDown(KEY_A));
			keyS.Update(KeyDown(KEY_S));
			keyD.Update(KeyDown(KEY_D));
			keyCtrl.Update(KeyDown(KEY_LCONTROL));
			keyShift.Update(KeyDown(KEY_LSHIFT));
			keyAlt.Update(KeyDown(KEY_LMENU));
			keySpace.Update(KeyDown(KEY_SPACE));
			keyE.Update(KeyDown(KEY_E));
			keyQ.Update(KeyDown(KEY_Q));
			keyC.Update(KeyDown(KEY_C));
			keyZ.Update(KeyDown(KEY_Z));
			key1.Update(KeyDown(KEY_1));
			key2.Update(KeyDown(KEY_2));
			key3.Update(KeyDown(KEY_3));
			key4.Update(KeyDown(KEY_4));
			key5.Update(KeyDown(KEY_5));
			key6.Update(KeyDown(KEY_6));
			key7.Update(KeyDown(KEY_7));
			mouseLeft.Update(MouseDown(true));
			mouseRight.Update(MouseDown(false));
		}

		// --- state ---------------------------------------------------------------------------------
		static bool w3Controls = false;    // our controls drive the hero (last hero AI frame)
		static bool overrideInput = false; // true while the hero's AI reads its controls
		static bool moving = false;        // a direction key held this frame
		static bool walking = false;       // Left Ctrl's toggle
		// what the plugin hands Gothic as its own keys this frame
		static bool gForward = false, gForwardEdge = false, gAction = false, gActionEdge = false;
		static bool gBack = false, gBackEdge = false, gStrafeL = false, gStrafeLEdge = false;
		static bool gStrafeR = false, gStrafeREdge = false, gJump = false, gJumpEdge = false;
		static float useUntil = -1.0f;     // E's interaction window
		// Geralt's own actions the plugin plays (they take over the body until they end)
		static float actionUntil = -1.0f;  // when the current one may be followed by another
		static float actionEnd = -1.0f;    // when it is over
		static const char* actionName = "";
		static float evadeUntil = -1.0f;   // melee misses him until then (a dodge or roll)
		static zVEC3 evadeDir;             // which way he dodged
		static bool guarding = false;
		static float guardPressed = -10.0f;
		// sword swings (t_W3_<family><n><stance>, see Swing): the one-slot buffer, the stance the next
		// one starts from, the place in the string, the swing playing (its target, hit time, slide)
		enum { ATTACK_NONE, ATTACK_FAST, ATTACK_STRONG };
		static int queuedAttack = ATTACK_NONE;
		static float queuedAt = -10.0f;
		static bool stanceLeft = false;
		static int stringIndex = 0;
		static bool swinging = false, swingStrong = false, swingHitDone = true, swingCutLogged = false;
		static float swingStart = -1.0f, swingHitAt = -1.0f, swingScale = 1.0f;
		static oCNpc* swingTarget = nullptr;
		// signs and stamina
		enum { SIGN_AARD, SIGN_YRDEN, SIGN_IGNI, SIGN_QUEN, SIGN_AXII, SIGNS };
		static const char* const SIGN_NAMES[SIGNS] = { "Aard", "Yrden", "Igni", "Quen", "Axii" };
		static int sign = SIGN_AARD;
		static float stamina = 100.0f;
		static float signThrowAt = -1.0f;
		static int signCasting = -1;
		static float quenUntil = -1.0f, yrdenUntil = -1.0f;
		static zVEC3 yrdenAt;
		static bool startSwordGiven = false;
		// damage being applied by the plugin (the OnDamage_Hit hook sets it exactly)
		static oCNpc* plugTarget = nullptr;
		static float plugDamage = 0.0f;
		static int plugType = 0;
		static zVEC3 plugFly;
		static bool plugFlyOn = false;
		// burning (Igni) and dazed (Axii) people
		struct Burning { oCNpc* npc = nullptr; float until = 0.0f, next = 0.0f; };
		static Burning burning[16];
		struct Slowed { oCNpc* npc = nullptr; };
		static Slowed slowedNpcs[16];
		// camera
		static bool cameraActive = false;
		static zVEC3 cameraForward;
		static float cameraPitch = 10.0f; // degrees, positive looks down (the game's -10)
		static zVEC3 cameraFocus;
		static float combatBlend = 0.0f;  // 0 exploration .. 1 combat camera
		static oCNpc* lockTarget = nullptr;
		static float mouseMovedX = -100.0f, mouseMovedY = -100.0f; // when the mouse last turned it
		// the game's turns (Witcher_turns.txt): the one playing, where it began facing, its scale
		struct TurnAni
		{
			char name[32];
			int frames;
			float deg[64]; // his turn at each frame from the first (left positive)
		};
		static TurnAni turnAnis[24];
		static int turnAniCount = 0;
		static bool turnsLoaded = false;
		static const TurnAni* turnPlaying = nullptr;
		static int turnAniID = -1;
		static zVEC3 turnFrom;
		static float turnScale = 1.0f, turnEnded = -10.0f;
		static bool wasMoving = false;
		// sprint (GERALT_SPRINT.MDS)
		static const char* const SPRINT_OVERLAY = "GERALT_SPRINT.MDS";
		static const char* const RUN_CYCLES[2] = { "S_RUNL", "S_1HRUNL" };
		static zCModelPrototype* sprintProto = nullptr, * sprintBase = nullptr;
		static zCModel* sprintModel = nullptr;
		static float sprintBlend = 0.0f;
		// effects
		struct Burst { zCParticleFX* effect = nullptr; float stop = 0.0f; };
		static Burst bursts[32];

		static int BodyState(oCNpc* npc)
		{
			return npc->GetBodyState() & BS_ONLY_STATE;
		}

		static bool SwordDrawn(oCNpc* hero)
		{
			return hero->GetWeaponMode() == NPC_WEAPON_1HS;
		}

		// The Witcher's split of foes: steel for people, silver for monsters (Gothic's guilds below
		// the GIL_HUMANS separator are its people; orcs, being people too, take steel).
		static bool IsMonster(oCNpc* npc)
		{
			int guild = npc->GetTrueGuild();
			return guild > NPC_GIL_HUMANS && guild < NPC_GIL_ORCS;
		}

		static bool Hostile(oCNpc* npc, oCNpc* hero)
		{
			return npc && npc != hero && !npc->IsDead() && !npc->IsUnconscious()
				&& (npc->GetAttitude(hero) == NPC_ATT_HOSTILE || npc->enemy == hero);
		}

		// The NPCs within `range` cm of `around`. The distance is checked before anything else is
		// asked of an NPC: queries such as GetModel() make Gothic load far NPCs' models, and with
		// -zautoconvertdata it then tries to recompile them from sources the game doesn't ship
		// (a skeleton's "ASC not found: SKE_SPAWN_A02" ended the game).
		template <typename F> static void ForNpcsNear(const zVEC3& around, float range, F f)
		{
			oCWorld* world = ogame ? ogame->GetGameWorld() : nullptr;
			for (zCListSort<oCNpc>* node = world && world->voblist_npcs ? world->voblist_npcs->next : nullptr; node; node = node->next)
			{
				oCNpc* npc = node->data;
				if (npc && npc->GetHomeWorld() && (npc->GetPositionWorld() - around).Length() <= range)
					f(npc);
			}
		}

		// A model the NPC already has (never one Gothic would have to load).
		static zCModel* LoadedModel(oCNpc* npc)
		{
			return zDYNAMIC_CAST<zCModel>(npc->GetVisual());
		}

		// The nearest hostile within `range` cm whose direction is within `cone` degrees of `dir`.
		static oCNpc* NearestFoe(oCNpc* hero, const zVEC3& dir, float range, float cone)
		{
			oCNpc* best = nullptr;
			float bestScore = 1e30f;
			zVEC3 at = hero->GetPositionWorld();
			ForNpcsNear(at, range, [&](oCNpc* npc)
			{
				if (!Hostile(npc, hero))
					return;
				float d = (npc->GetPositionWorld() - at).Length();
				float angle = AngleTo(dir, at, npc->GetPositionWorld());
				if (angle > cone)
					return;
				float score = d + angle * 3.0f; // the game weighs angle and distance
				if (score < bestScore)
				{
					bestScore = score;
					best = npc;
				}
			});
			return best;
		}

		static int HostilesNear(oCNpc* hero, float range)
		{
			int n = 0;
			zVEC3 at = hero->GetPositionWorld();
			ForNpcsNear(at, range, [&](oCNpc* npc) { if (Hostile(npc, hero)) n++; });
			return n;
		}

		// --- the model -----------------------------------------------------------------------------
		// Gothic keeps animation names in upper case and compares them as they are.
		static int AniID(zCModel* model, const char* name)
		{
			if (!model)
				return -1;
			zSTRING upper(name);
			upper.Upper();
			return model->GetAniIDFromAniName(upper);
		}

		static bool StartAni(oCNpc* hero, const char* name)
		{
			zCModel* model = hero->GetModel();
			int id = AniID(model, name);
			if (id < 0)
			{
				Log("animation %s missing", name);
				return false;
			}
			model->StartAni(id, 0);
			return true;
		}

		static bool AniActive(oCNpc* hero, const char* name)
		{
			zCModel* model = hero->GetModel();
			int id = AniID(model, name);
			zCModelAni* ani = id >= 0 ? model->GetAniFromAniID(id) : nullptr;
			return ani && model->IsAniActive(ani);
		}

		// Whether an animation of that name plays, from the model itself or an overlay (the sprint's
		// s_RunL), and is not on its way out.
		static bool Playing(oCNpc* hero, const char* name)
		{
			zCModel* model = hero->GetModel();
			int id = AniID(model, name);
			for (int i = 0; model && id >= 0 && i < model->numActiveAnis; i++)
				if (zCModelAniActive* a = model->aniChannels[i])
					if (a->protoAni && a->protoAni->aniID == id && !a->isFadingOut)
						return true;
			return false;
		}

		// A horizontal direction turned from `from` towards `to` by the share t of the angle between
		// them (straight behind: by way of his right).
		static zVEC3 TurnToward(const zVEC3& from, const zVEC3& to, float t, oCNpc* hero)
		{
			float dot = from.Dot(to);
			dot = dot > 1.0f ? 1.0f : (dot < -1.0f ? -1.0f : dot);
			float angle = acosf(dot);
			if (angle < 0.001f)
				return to;
			bool behind = angle > PI - 0.01f;
			zVEC3 target = behind ? RightOf(from, hero) : to;
			float a = behind ? PI / 2.0f : angle, step = t * angle;
			step = step > a ? a : step;
			return Flat(from * (sinf(a - step) / sinf(a)) + target * (sinf(step) / sinf(a)));
		}

		// Every soft skin of the hero's model at full detail (see the header), once per model:
		// a new game or a loaded savegame gives the hero a new one.
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

		// --- effects and sound ---------------------------------------------------------------------
		static zCVob* Emit(const char* emitter, const zVEC3& point, float seconds)
		{
			zCWorld* world = oCNpc::player ? oCNpc::player->GetHomeWorld() : nullptr;
			Burst* slot = nullptr;
			for (Burst& b : bursts)
				if (!b.effect)
				{
					slot = &b;
					break;
				}
			if (!slot || !world)
				return nullptr;
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
			vob->Release(); // the world keeps it; the burst keeps the effect until it stops
			slot->effect = effect;
			slot->stop = Now() + seconds;
			return vob;
		}

		static void UpdateBursts()
		{
			float now = Now();
			for (Burst& b : bursts)
				if (b.effect && (now >= b.stop || now < b.stop - 30.0f)) // (or the game time went back: a load)
				{
					b.effect->StopEmitterOutput();
					b.effect->Release();
					b.effect = nullptr;
				}
		}

		// --- Geralt's voice ------------------------------------------------------------------------
		// make_geralt_voice.py puts his own lines from The Witcher 3 (the game's Polish or English
		// dub) into Data\Geralt_Voice.vdf and lists them in Witcher_voice.txt (Gothic folder):
		// category, sound file, seconds, subtitle, tab-separated. He says one now and then as the
		// game has him do: when a fight starts, after a kill, badly hurt, and out exploring.
		struct VoiceLine
		{
			char category[16] = "";
			char file[48] = "";
			float seconds = 2.0f;
			char text[256] = "";
			zCSoundFX* fx = nullptr;
		};
		static VoiceLine* voiceLines = nullptr;
		static int voiceCount = 0;
		static bool voiceLoaded = false;
		static float voiceUntil = 0.0f, voiceNextIdle = -1.0f, voiceHurtReady = 0.0f;
		static bool voiceFight = false;
		static int voiceRecent[6] = { -1, -1, -1, -1, -1, -1 };
		static float voiceLastHealth = -1.0f;

		static void LoadVoice()
		{
			if (voiceLoaded)
				return;
			voiceLoaded = true;
			FILE* file = nullptr;
			if (fopen_s(&file, "Witcher_voice.txt", "r") || !file)
			{
				Log("voice: Witcher_voice.txt not found (run make_geralt_voice.py)");
				return;
			}
			static char line[512];
			int capacity = 0;
			while (fgets(line, sizeof(line), file))
			{
				line[strcspn(line, "\r\n")] = 0;
				if (!line[0] || line[0] == '#')
					continue;
				char* fields[4] = {};
				int n = 0;
				for (char* p = line; p && n < 4;)
				{
					fields[n++] = p;
					char* tab = n < 4 ? strchr(p, '\t') : nullptr;
					if (tab)
						*tab = 0;
					p = tab ? tab + 1 : nullptr;
				}
				if (n < 3)
					continue;
				if (voiceCount == capacity)
				{
					capacity = capacity ? capacity * 2 : 64;
					VoiceLine* grown = new VoiceLine[capacity];
					for (int i = 0; i < voiceCount; i++)
						grown[i] = voiceLines[i];
					delete[] voiceLines;
					voiceLines = grown;
				}
				VoiceLine& v = voiceLines[voiceCount++];
				strncpy_s(v.category, fields[0], _TRUNCATE);
				strncpy_s(v.file, fields[1], _TRUNCATE);
				v.seconds = (float)atof(fields[2]);
				if (fields[3])
					strncpy_s(v.text, fields[3], _TRUNCATE);
			}
			fclose(file);
			Log("voice: %d lines", voiceCount);
		}

		// One of his lines of `category`, with `chance` (0..1), unless he is still speaking (his
		// own line, or Gothic's dialogue voice) or spoke less than voiceGap seconds ago. Against
		// someone (against: 0 a person, 1 a monster) the lines the game keeps for people only
		// (category_human) or monsters only (category_monster) count too.
		static void Say(oCNpc* hero, const char* category, float chance, int against = -1)
		{
			float now = Now();
			if (!settings.voice || !zsound || now < voiceUntil + settings.voiceGap || hero->IsVoiceActive() || Random() > chance)
				return;
			LoadVoice();
			char own[32] = "";
			if (against >= 0)
				sprintf_s(own, "%s_%s", category, against ? "monster" : "human");
			int candidates = 0, pick = -1;
			for (int i = 0; i < voiceCount; i++)
			{
				if (strcmp(voiceLines[i].category, category) && (!own[0] || strcmp(voiceLines[i].category, own)))
					continue;
				bool recent = false;
				for (int r : voiceRecent)
					recent |= r == i;
				if (recent)
					continue;
				if (rand() % ++candidates == 0)  // a uniform pick in one pass
					pick = i;
			}
			if (pick < 0)
				return;
			VoiceLine& v = voiceLines[pick];
			if (!v.fx)
				v.fx = zsound->LoadSoundFX(zSTRING(v.file));
			if (!v.fx)
				return;
			zCSoundSystem::zTSound3DParams params;
			params.SetDefaults();
			params.volume = settings.voiceVolume;
			params.radius = 2500.0f;
			zsound->PlaySound3D(v.fx, hero, zSND_SLOT_NONE, &params);
			for (int r = 5; r > 0; r--)
				voiceRecent[r] = voiceRecent[r - 1];
			voiceRecent[0] = pick;
			voiceUntil = now + v.seconds;
			if (settings.voiceSubtitles && v.text[0] && ogame && ogame->array_view[0])
			{
				static zCOLOR white(255, 255, 255, 255);
				ogame->array_view[0]->PrintTimedCX(6900, zSTRING(v.text), (v.seconds + 0.5f) * 1000.0f, &white);
			}
			Log("voice %s: %s", category, v.file);
		}

		// Each hero frame: a fight starting, low health, and his comments while exploring.
		static void UpdateVoice(oCNpc* hero, int hostiles)
		{
			float now = Now();
			if (voiceNextIdle < 0.0f)
				voiceNextIdle = now + settings.voiceIdleMin;
			bool fight = hostiles > 0;
			if (fight && !voiceFight)
			{
				oCNpc* foe = NearestFoe(hero, hero->GetAtVectorWorld(), 1500.0f, 180.0f);
				Say(hero, "combat", 0.6f, foe ? (IsMonster(foe) ? 1 : 0) : -1);
			}
			voiceFight = fight;
			float health = (float)hero->attribute[NPC_ATR_HITPOINTS] / (float)(hero->attribute[NPC_ATR_HITPOINTSMAX] > 0 ? hero->attribute[NPC_ATR_HITPOINTSMAX] : 1);
			if (voiceLastHealth >= 0.0f && health < voiceLastHealth && health < 0.35f && now >= voiceHurtReady)
			{
				voiceHurtReady = now + 25.0f;
				Say(hero, "hurt", 0.7f);
			}
			voiceLastHealth = health;
			if (!fight && now >= voiceNextIdle)
			{
				Say(hero, "explore", 1.0f);
				voiceNextIdle = now + settings.voiceIdleMin + Random() * (settings.voiceIdleMax - settings.voiceIdleMin);
			}
		}

		// --- damage --------------------------------------------------------------------------------
		// `share` of the victim's maximum health, of `type` (Gothic's damage type flags), through
		// Gothic's own damage (hit reactions, death, knock-out); `fly` throws him that way.
		static void Hurt(oCNpc* hero, oCNpc* npc, float share, int type, const zVEC3* fly = nullptr)
		{
			if (!npc || npc->IsDead())
				return;
			plugTarget = npc;
			plugDamage = share * npc->attribute[NPC_ATR_HITPOINTSMAX];
			plugType = type;
			plugFlyOn = fly != nullptr;
			if (fly)
				plugFly = *fly;
			bool lethal = plugDamage >= npc->attribute[NPC_ATR_HITPOINTS];
			bool standing = !npc->IsUnconscious();
			npc->OnDamage(hero, hero, plugDamage > 1.0f ? plugDamage : 1.0f, type, npc->GetPositionWorld());
			plugTarget = nullptr;
			plugFlyOn = false;
			if (settings.killHumans && lethal && !npc->IsDead() && !npc->HasFlag(NPC_FLAG_IMMORTAL)
				&& (npc->IsUnconscious() || npc->attribute[NPC_ATR_HITPOINTS] <= 1))
			{
				npc->attribute[NPC_ATR_HITPOINTS] = 0;
				npc->DoDie(hero);
			}
			if (hero == oCNpc::player && standing && (npc->IsDead() || npc->IsUnconscious()))
				Say(hero, "kill", 0.45f, IsMonster(npc) ? 1 : 0);
		}

		// The share of a sword hit: base, the right or wrong sword, +-10 %, critical hits.
		static float SwordShare(oCNpc* hero, oCNpc* victim, float base)
		{
			bool silver = false; // TODO: Geralt's silver sword item
			float share = base * (IsMonster(victim) == silver ? 1.0f : settings.wrongSword);
			share *= 0.9f + 0.2f * Random();
			bool behind = AngleTo(victim->GetAtVectorWorld(), victim->GetPositionWorld(), hero->GetPositionWorld()) > 90.0f;
			if (Random() < (behind ? 0.55f : 0.05f))
			{
				share *= 1.25f;
				Log("critical hit on %s", victim->GetInstanceName().ToChar());
			}
			return share;
		}

		// --- sprint (port of the GTA mashup's, with The Witcher's speeds: the cycles' own) -----------
		static zCModelAni* ProtoAni(zCModelPrototype* proto, int id)
		{
			return proto && id >= 0 && id < proto->protoAnis.GetNum() ? proto->protoAnis[id] : nullptr;
		}

		static bool SprintApplied(zCModel* model)
		{
			return sprintProto && model->modelProtoList.IsInList(sprintProto);
		}

		// What Apply/RemoveModelProtoOverlay do apart from stopping every animation (which would
		// snap him into the sprint and, during a draw, loop it: gothic2beyond docs/09).
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

		static bool LoadSprint(zCModel* model)
		{
			zCModelPrototype* base = model->modelProtoList.GetNum() > 0 ? model->modelProtoList[0] : nullptr;
			if (!base)
				return false;
			if (base == sprintBase)
				return sprintProto != nullptr;
			zSTRING name = base->modelProtoName;
			name.Upper();
			if (!(name == "GERALT"))
				return false;
			sprintBase = base;
			sprintProto = zCModelPrototype::Load(zSTRING(SPRINT_OVERLAY), base);
			Log("sprint: %s %s", SPRINT_OVERLAY, sprintProto ? "loaded" : "failed to load");
			return sprintProto != nullptr;
		}

		// Swaps the playing cycle for `to` with a cross-fade over `seconds`, at the same point of
		// the stride.
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
			from->blendOutOverride = -1.0f / seconds;
			float blendIn = to->blendInSpeed;
			to->blendInSpeed = 1.0f / seconds;
			model->StartAni(to, zCModel::zMDL_STARTANI_DEFAULT);
			to->blendInSpeed = blendIn;
			zCModelAniActive* started = model->GetActiveAni(to);
			if (!started)
				return;
			started->SetProgressPercent(progress);
			if (to->rootNodeIndex >= 0) // no jump in the root motion
				started->lastPos = started->thisPos = to->GetTrans((int)started->actFrame, to->rootNodeIndex);
		}

		// Both cycles at one cadence (feet in step through the cross-fade) that moves him at the
		// speed between the run's and the sprint's that sprintBlend asks for.
		static void UpdateSprintSpeed(zCModel* model, zCModelAni* run, zCModelAni* sprint)
		{
			float runVelocity = run->fpsRate > 0.0f ? run->GetAniVelocity() * run->fpsRateSource / run->fpsRate : 0.0f;
			float sprintVelocity = sprint->fpsRate > 0.0f ? sprint->GetAniVelocity() * sprint->fpsRateSource / sprint->fpsRate : 0.0f;
			if (runVelocity <= 0.0f || sprintVelocity <= 0.0f || run->numFrames <= 0 || sprint->numFrames <= 0)
				return;
			float runStride = runVelocity * run->numFrames / run->fpsRateSource;
			float sprintStride = sprintVelocity * sprint->numFrames / sprint->fpsRateSource;
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
			float blend = sprintBlend * sprintBlend * (3.0f - 2.0f * sprintBlend);
			float speed = runVelocity + (sprintVelocity - runVelocity) * blend;
			float cadence = speed / (runStride + (sprintStride - runStride) * share);
			run->fpsRate = cadence * run->numFrames > 1.0f ? cadence * run->numFrames : 1.0f;
			sprint->fpsRate = cadence * sprint->numFrames > 1.0f ? cadence * sprint->numFrames : 1.0f;
		}

		static void UpdateSprint(oCNpc* hero, bool want, bool airborne)
		{
			zCModel* model = hero->GetModel();
			if (!model || !LoadSprint(model))
				return;
			if (model != sprintModel)
			{
				sprintModel = model;
				sprintBlend = 0.0f;
			}
			zCModelAni* runs[2], * sprints[2];
			for (int i = 0; i < 2; i++)
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
					for (zCModelAni* s : sprints)
						if (s)
							CrossFade(model, s, settings.sprintAccel * (1.0f - sprintBlend));
					Log("sprint on");
				}
				else if (!want && applied)
				{
					SetSprintOverlay(model, false);
					for (zCModelAni* r : runs)
						if (r)
							CrossFade(model, r, settings.sprintDecel * sprintBlend);
					Log("sprint off");
				}
				float dt = FrameSeconds();
				if (!moving)
					sprintBlend = 0.0f;
				else if (want)
					sprintBlend += dt / (settings.sprintAccel > 0.01f ? settings.sprintAccel : 0.01f);
				else
					sprintBlend -= dt / (settings.sprintDecel > 0.01f ? settings.sprintDecel : 0.01f);
				sprintBlend = sprintBlend < 0.0f ? 0.0f : (sprintBlend > 1.0f ? 1.0f : sprintBlend);
			}
			for (int i = 0; i < 2; i++)
				if (runs[i] && sprints[i])
					UpdateSprintSpeed(model, runs[i], sprints[i]);
		}

		static void StopSprint(oCNpc* hero)
		{
			sprintBlend = 0.0f;
			zCModel* model = hero->GetModel();
			if (!model || model != sprintModel || !sprintProto)
				return;
			if (SprintApplied(model))
				SetSprintOverlay(model, false);
			for (const char* name : RUN_CYCLES)
				if (zCModelAni* run = ProtoAni(model->modelProtoList[0], model->GetAniIDFromAniName(name)))
					run->fpsRate = run->fpsRateSource;
		}

		// --- per game ------------------------------------------------------------------------------
		static void NewGame()
		{
			LoadSettings();
			startSwordGiven = false;
			lockTarget = nullptr;
			actionUntil = actionEnd = evadeUntil = quenUntil = yrdenUntil = -1.0f;
			signCasting = -1;
			stamina = 100.0f;
			for (Burning& b : burning)
				b.npc = nullptr;
			for (Slowed& sl : slowedNpcs)
				sl.npc = nullptr;
			Log("new game or savegame loaded");
		}

		static void GiveStartSword(oCNpc* hero)
		{
			if (startSwordGiven || !parser)
				return;
			startSwordGiven = true;
			if (!settings.startSword || hero->GetEquippedMeleeWeapon())
				return;
			int index = parser->GetIndex(zSTRING("ITMW_1H_SWORD_01"));
			if (index < 0)
				return;
			oCItem* item = hero->IsInInv(index, 1);
			if (!item)
			{
				hero->CreateItems(index, 1);
				item = hero->IsInInv(index, 1);
			}
			if (item)
			{
				hero->EquipItem(item);
				Log("start sword: Geralt has a one-handed sword (until his own swords are in)");
			}
		}

		// --- actions -------------------------------------------------------------------------------
		static bool Busy()
		{
			return Now() < actionUntil;
		}

		static void Begin(oCNpc* hero, const char* name, float next, float end)
		{
			if (!StartAni(hero, name))
				return;
			swinging = false; // whatever comes next ends a swing (Swing sets it again for its own)
			actionName = name;
			actionUntil = Now() + next;
			actionEnd = Now() + end;
			Log("action %s", name);
		}

		static void FaceFoe(oCNpc* hero, float range, float cone)
		{
			if (oCNpc* foe = lockTarget ? lockTarget : NearestFoe(hero, hero->GetAtVectorWorld(), range, cone))
				hero->SetHeadingAtWorld(Flat(foe->GetPositionWorld() - hero->GetPositionWorld()));
		}

		// dir: -1 none, 0 forward, 1 back, 2 left, 3 right (relative to Geralt)
		static void Dodge(oCNpc* hero, int dir)
		{
			zVEC3 at = Flat(hero->GetAtVectorWorld()), right = RightOf(at, hero);
			if (dir < 0)
				dir = 1;
			evadeDir = dir == 0 ? at : (dir == 1 ? at * -1.0f : (dir == 2 ? right * -1.0f : right));
			evadeUntil = Now() + 0.6f;
			if (dir == 0)
				Begin(hero, "t_W3_DodgeF", 0.65f, 1.5f);
			else
			{
				// back and to the side: Gothic's own jump back and side steps (GERALT.MDS gives them
				// Geralt's dodges), so Gothic moves him with its collision
				gBack = gBackEdge = dir == 1;
				gStrafeL = gStrafeLEdge = dir == 2;
				gStrafeR = gStrafeREdge = dir == 3;
				swinging = false;
				actionUntil = Now() + 0.65f;
				actionEnd = Now() + 1.5f;
				actionName = dir == 1 ? "dodge back" : (dir == 2 ? "dodge left" : "dodge right");
				Log("action %s", actionName);
			}
		}

		static void Roll(oCNpc* hero, const zVEC3& want, bool hasDir)
		{
			zVEC3 at = Flat(hero->GetAtVectorWorld());
			bool back = hasDir && at.Dot(want) < -0.7f;
			if (hasDir && !back)
				hero->SetHeadingAtWorld(want); // the game turns him into the roll
			evadeDir = back ? at * -1.0f : (hasDir ? want : at);
			evadeUntil = Now() + 0.8f;
			Begin(hero, back ? "t_W3_RollB" : "t_W3_RollF", 0.9f, 1.33f);
		}

		// The game's turns: his 180-degree turns while running, walking or sprinting and his starts
		// towards the side or back (t_W3_...Turn<L/R>, t_W3_...Start<L/R/BL/BR>). make_geralt_anims.py
		// takes their turn out of the animation (Gothic would not turn his heading with it) and
		// lists it per frame in Witcher_turns.txt; while one plays, his heading follows that turn
		// from where it began, scaled so he ends facing the way to go.
		static void LoadTurns()
		{
			if (turnsLoaded)
				return;
			turnsLoaded = true;
			FILE* file = nullptr;
			if (fopen_s(&file, "Witcher_turns.txt", "r") || !file)
			{
				Log("turns: Witcher_turns.txt not found (run make_geralt_anims.py)");
				return;
			}
			static char line[2048];
			while (fgets(line, sizeof(line), file) && turnAniCount < 24)
			{
				if (line[0] == '#' || line[0] == '\r' || line[0] == '\n')
					continue;
				TurnAni& t = turnAnis[turnAniCount];
				char* context = nullptr;
				char* name = strtok_s(line, "\t", &context);
				char* count = name ? strtok_s(nullptr, "\t", &context) : nullptr;
				if (!name || !count)
					continue;
				strncpy_s(t.name, name, _TRUNCATE);
				t.frames = 0;
				for (char* v = strtok_s(nullptr, " \t\r\n", &context); v && t.frames < 64; v = strtok_s(nullptr, " \t\r\n", &context))
					t.deg[t.frames++] = (float)atof(v);
				if (t.frames >= 2)
					turnAniCount++;
			}
			fclose(file);
			Log("turns: %d of the game's turning animations", turnAniCount);
		}

		// want: degrees to the way to go, left positive
		static bool StartTurn(oCNpc* hero, const char* name, float want)
		{
			const TurnAni* turn = nullptr;
			for (int i = 0; i < turnAniCount && !turn; i++)
				if (!_stricmp(turnAnis[i].name, name))
					turn = &turnAnis[i];
			zCModel* model = hero->GetModel();
			int id = AniID(model, name);
			if (!turn || id < 0 || fabsf(turn->deg[turn->frames - 1]) < 1.0f)
				return false;
			float seconds = (turn->frames - 1) / 25.0f;
			Begin(hero, name, seconds, seconds);
			if (!AniActive(hero, name))
				return false;
			float scale = want / turn->deg[turn->frames - 1];
			turnPlaying = turn;
			turnAniID = id;
			turnFrom = Flat(hero->GetAtVectorWorld());
			turnScale = scale < 0.6f ? 0.6f : (scale > 1.3f ? 1.3f : scale);
			Log("turn %s for %.0f degrees", name, want);
			return true;
		}

		static void UpdateTurn(oCNpc* hero)
		{
			if (!turnPlaying)
				return;
			zCModel* model = hero->GetModel();
			zCModelAniActive* active = model && turnAniID >= 0 ? model->GetActiveAni(turnAniID) : nullptr;
			if (!active)
			{
				turnPlaying = nullptr; // cut off (a hit, a dodge): he keeps facing where it left him
				turnEnded = Now();
				return;
			}
			int last = turnPlaying->frames - 1;
			float frame = active->isFadingOut ? (float)last : active->actFrame;
			frame = frame < 0.0f ? 0.0f : (frame > (float)last ? (float)last : frame);
			int i = (int)frame;
			float deg = turnPlaying->deg[i] + (i < last ? (turnPlaying->deg[i + 1] - turnPlaying->deg[i]) * (frame - i) : 0.0f);
			float a = deg * turnScale * PI / 180.0f;
			hero->SetHeadingAtWorld(Flat(turnFrom * cosf(a) - RightOf(turnFrom, hero) * sinf(a)));
			if (active->isFadingOut || i >= last)
			{
				turnPlaying = nullptr;
				turnEnded = Now();
			}
		}

		// Horizontal distance (cm) from Geralt to the nearest point of an NPC (its bounding box
		// taken as a circle), as the game measures to the target's capsule.
		static float DistanceTo(oCNpc* hero, oCNpc* npc)
		{
			const zTBBox3D& box = npc->bbox3D;
			float radius = 0.25f * ((box.maxs[0] - box.mins[0]) + (box.maxs[2] - box.mins[2]));
			radius = radius < 25.0f ? 25.0f : (radius > 120.0f ? 120.0f : radius);
			zVEC3 d = npc->GetPositionWorld() - hero->GetPositionWorld();
			float flat = sqrtf(d[0] * d[0] + d[2] * d[2]) - radius;
			return flat > 0.0f ? flat : 0.0f;
		}

		// One sword swing, chosen as The Witcher 3's combat.ws (OnComboAttackCallback) chooses it:
		// - the target: the hard-locked one, else the best hostile within 12 m in the way he is
		//   pushed (or faces), 80 degrees either side, re-picked at every press (soft lock);
		// - the family by the distance to it: sprinting: the run-in swings (fast/strong far);
		//   > 4 m: the far ones with the longer reach (fast far extra; strong far); > 3.25 m: far;
		//   > 2.5 m: the extra-distance swings (fast; strong has only its own); else the normal
		//   string; > 5 m or no target: the normal string in place;
		// - the stance: every swing ends in the other one (its CombatStanceLeft/Right event), the
		//   normal swings run through their string (fast 1-4, strong 1-3) and start over after a
		//   pause.
		// A swing's root motion is the game's slide template (2, 3, 4 or 5 m straight ahead): the
		// game scales it to bring Geralt onto the target, and so does Swing, through the model's
		// aniTransScale (zCModel::AdvanceAnis multiplies the root translation by it, so Gothic's
		// own collision still moves him). He turns onto the target during the wind-up (the game's
		// magnetism); the directional swings (to the side, behind) turn him inside the animation,
		// which Gothic would not carry over to his heading, so he turns and takes a forward one.
		static void Swing(oCNpc* hero, bool strong, const zVEC3& pushed, bool sprinting)
		{
			float now = Now();
			if (now - actionEnd > 0.5f)
				stringIndex = 0; // a new string
			oCNpc* target = lockTarget ? lockTarget : NearestFoe(hero, pushed, 1200.0f, 80.0f);
			float dist = target ? DistanceTo(hero, target) : 1e9f;
			static char name[32]; // actionName points at it while the swing plays
			const char* side = stanceLeft ? "L" : "R";
			float nominal, hit, next, end; // slide template (cm) and the swing's events (s)
			if (target && (sprinting || (dist > 325.0f && dist <= 500.0f)))
			{
				bool extra = !strong && dist > 400.0f && !sprinting;
				sprintf(name, "t_W3_%s%s", strong ? "SF" : (extra ? "FFX" : "FF"), side);
				nominal = strong ? 400.0f : (extra ? 500.0f : 400.0f);
				hit = strong ? 0.79f : 0.69f;
				next = strong ? 1.20f : 0.90f;
				end = strong ? 1.50f : 1.17f;
			}
			else if (!strong && target && dist > 250.0f && dist <= 500.0f)
			{
				sprintf(name, "t_W3_FX%d%s", 1 + stringIndex % 2, side);
				nominal = 300.0f, hit = 0.39f, next = 0.60f, end = 0.87f;
			}
			else
			{
				if (strong)
					sprintf(name, "t_W3_S%d%s", 1 + stringIndex % 3, side);
				else
					sprintf(name, "t_W3_F%d%s", 1 + stringIndex % 4, side);
				nominal = 200.0f;
				hit = strong ? 0.69f : 0.39f;
				next = strong ? 1.10f : 0.60f;
				end = strong ? 1.40f : 0.87f;
			}
			// the slide: onto the target, stopping swingStop short of it (beyond 5 m the game no
			// longer closes the gap: he swings where he stands, turning to it); without one a step
			float slide = dist <= 500.0f ? dist - settings.swingStop : (target ? 0.0f : settings.swingStep);
			swingScale = slide <= 0.0f ? 0.0f : (slide >= nominal ? 1.0f : slide / nominal);
			Begin(hero, name, next, end);
			if (actionName != name)
				return; // the animation is missing (logged)
			swinging = true;
			swingStrong = strong;
			swingTarget = target;
			swingStart = now;
			swingCutLogged = false;
			swingHitAt = now + hit;
			swingHitDone = false;
			stanceLeft = !stanceLeft;
			stringIndex++;
			if (strong)
				Say(hero, "attack", 0.35f);
			Log("swing %s: target %s at %.0f cm, slide x%.2f", name,
				swingTarget ? swingTarget->GetInstanceName().ToChar() : "none", target ? dist : -1.0f, swingScale);
		}

		// The hit, at the end of the swing's PreAttack event: everyone within swingReach in the
		// game's slash_short cone (110 degrees) in front of him.
		static void SwingHit(oCNpc* hero)
		{
			swingHitDone = true;
			zVEC3 at = hero->GetPositionWorld(), dir = hero->GetAtVectorWorld();
			int hits = 0;
			oCNpc* struck = nullptr;
			ForNpcsNear(at, settings.swingReach + 120.0f, [&](oCNpc* npc)
			{
				if (npc == hero || npc->IsDead() || DistanceTo(hero, npc) > settings.swingReach
					|| AngleTo(dir, at, npc->GetPositionWorld()) > 55.0f)
					return;
				if (!Hostile(npc, hero) && npc->GetAttitude(hero) == NPC_ATT_FRIENDLY)
					return;
				Hurt(hero, npc, SwordShare(hero, npc, swingStrong ? settings.strongDamage : settings.fastDamage), oEDamageType_Edge);
				oCNpc::AssessFightSound_S(hero, hero->GetPositionWorld(), npc);
				hits++;
				struck = npc;
			});
			Log("%s swing: %d hit", swingStrong ? "strong" : "fast", hits);
			if (struck && !struck->IsDead() && !struck->IsUnconscious())
				Say(hero, "hit", 0.15f, IsMonster(struck) ? 1 : 0);
		}

		// No swing (the controls went off): Gothic's own root motion again.
		static void EndSwing(oCNpc* hero)
		{
			swinging = false;
			swingHitDone = true;
			if (zCModel* model = hero->GetModel())
				model->aniTransScale = zVEC3(1.0f, 1.0f, 1.0f);
		}

		// Each frame of a swing: the slide's scale, and the turn onto the target until the hit.
		static void UpdateSwing(oCNpc* hero)
		{
			zCModel* model = hero->GetModel();
			float now = Now();
			if (swinging && now > actionEnd)
				swinging = false;
			if (swinging && !swingCutLogged && !AniActive(hero, actionName) && model)
			{
				// something of Gothic's took the body over (logged to find what); the swing still
				// lands: only an action of his own (Begin, Dodge) or a hit on him ends it
				swingCutLogged = true;
				zSTRING others;
				for (int i = 0; i < model->numActiveAnis; i++)
					if (zCModelAniActive* a = model->aniChannels[i])
						if (a->protoAni)
							others += a->protoAni->aniName + " ";
				Log("swing %s cut after %.2f s by: %s", actionName, now - swingStart, others.ToChar());
			}
			if (!swinging)
				swingHitDone = true;
			if (model)
			{
				float k = swinging ? swingScale : 1.0f;
				model->aniTransScale = zVEC3(k, 1.0f, k);
			}
			if (!swinging)
				return;
			if (swingTarget && (swingTarget->IsDead() || !swingTarget->GetHomeWorld()))
				swingTarget = nullptr;
			if (swingTarget && now < swingHitAt)
			{
				zVEC3 heroAt = Flat(hero->GetAtVectorWorld());
				zVEC3 want = Flat(swingTarget->GetPositionWorld() - hero->GetPositionWorld());
				float dot = heroAt.Dot(want);
				dot = dot > 1.0f ? 1.0f : (dot < -1.0f ? -1.0f : dot);
				float angle = acosf(dot), step = settings.swingTurn * PI / 180.0f * FrameSeconds();
				if (angle <= step || angle < 0.001f)
					hero->SetHeadingAtWorld(want);
				else
				{
					bool behind = angle > PI - 0.01f;
					zVEC3 to = behind ? RightOf(heroAt, hero) : want;
					float a = behind ? PI / 2.0f : angle, t = step / a;
					hero->SetHeadingAtWorld(Flat(heroAt * (sinf((1.0f - t) * a) / sinf(a)) + to * (sinf(t * a) / sinf(a))));
				}
			}
			if (!swingHitDone && now >= swingHitAt)
				SwingHit(hero);
		}

		static void Counter(oCNpc* hero, oCNpc* attacker)
		{
			hero->SetHeadingAtWorld(Flat(attacker->GetPositionWorld() - hero->GetPositionWorld()));
			Begin(hero, "t_W3_Counter", 0.7f, 1.23f);
			zVEC3 fly = Flat(attacker->GetPositionWorld() - hero->GetPositionWorld()) + zVEC3(0.0f, 0.5f, 0.0f);
			fly.Normalize();
			Hurt(hero, attacker, 0.05f, oEDamageType_Blunt | oEDamageType_Fly, &fly);
			Log("counter: %s kicked down", attacker->GetInstanceName().ToChar());
		}

		// --- signs ---------------------------------------------------------------------------------
		static void CastSign(oCNpc* hero)
		{
			if (stamina < 99.9f)
			{
				Log("sign %s: not enough stamina (%.0f)", SIGN_NAMES[sign], stamina);
				return;
			}
			static const char* const ANIS[SIGNS][2] = {
				{ "t_W3_AardFree", "t_W3_Aard" }, { "t_W3_YrdenFree", "t_W3_Yrden" }, { "t_W3_IgniFree", "t_W3_Igni" },
				{ "t_W3_QuenFree", "t_W3_Quen" }, { "t_W3_Axii", "t_W3_Axii" } };
			static const float THROW[SIGNS] = { 0.50f, 0.55f, 0.50f, 0.50f, 0.26f };
			static const float NEXT[SIGNS] = { 0.80f, 0.80f, 0.80f, 0.60f, 0.68f };
			if (sign != SIGN_QUEN && sign != SIGN_YRDEN)
				FaceFoe(hero, 1500.0f, 60.0f);
			Begin(hero, ANIS[sign][SwordDrawn(hero) ? 1 : 0], NEXT[sign], 1.5f);
			stamina = 0.0f;
			signCasting = sign;
			signThrowAt = Now() + THROW[sign];
		}

		static void SignEffect(oCNpc* hero, int which)
		{
			zVEC3 at = hero->GetPositionWorld(), dir = Flat(hero->GetAtVectorWorld()), up(0.0f, 1.0f, 0.0f);
			zVEC3 hand = at + dir * 60.0f + up * 30.0f;
			auto cone = [&](float range, float half, auto f)
			{
				ForNpcsNear(at, range, [&](oCNpc* npc)
				{
					if (npc != hero && !npc->IsDead() && AngleTo(dir, at, npc->GetPositionWorld()) <= half)
						f(npc);
				});
			};
			int hits = 0;
			switch (which)
			{
			case SIGN_AARD:
				Emit("MFX_WINDFIST_CAST", hand, 1.0f);
				cone(700.0f, 30.0f, [&](oCNpc* npc)
				{
					zVEC3 fly = Flat(npc->GetPositionWorld() - at) + up * 0.6f;
					fly.Normalize();
					bool down = Random() < settings.signKnockdown;
					Hurt(hero, npc, 0.01f, down ? (oEDamageType_Blunt | oEDamageType_Fly) : oEDamageType_Blunt, down ? &fly : nullptr);
					Emit("MFX_WINDFIST_TARGET", npc->GetPositionWorld(), 1.0f);
					hits++;
				});
				break;
			case SIGN_IGNI:
				Emit("MFX_FIRESTORM_CAST", hand, 1.0f);
				cone(500.0f, 30.0f, [&](oCNpc* npc)
				{
					Hurt(hero, npc, settings.igniDamage, oEDamageType_Fire);
					if (Random() < settings.igniBurn)
						for (Burning& b : burning)
							if (!b.npc || b.npc == npc)
							{
								b.npc = npc;
								b.until = Now() + 3.0f;
								b.next = Now() + 1.0f;
								break;
							}
					Emit("MFX_FIRESPELL_HUMANBURN", npc->GetPositionWorld(), 3.0f);
					hits++;
				});
				break;
			case SIGN_QUEN:
				quenUntil = Now() + 30.0f;
				Emit("MFX_HEAL_START", at, 1.5f);
				break;
			case SIGN_YRDEN:
				yrdenUntil = Now() + 10.0f;
				yrdenAt = at;
				Emit("MFX_TELEPORT_RING", at - up * 80.0f, 10.0f);
				break;
			case SIGN_AXII:
				if (oCNpc* npc = NearestFoe(hero, dir, 1500.0f, 30.0f))
				{
					Emit("MFX_CHARM_TARGET", npc->GetPositionWorld(), 5.0f);
					zCPar_Symbol* self = parser ? parser->GetSymbol(zSTRING("SELF")) : nullptr;
					zCPar_Symbol* other = parser ? parser->GetSymbol(zSTRING("OTHER")) : nullptr;
					void* oldSelf = self ? self->GetInstanceAdr() : nullptr;
					void* oldOther = other ? other->GetInstanceAdr() : nullptr;
					parser->SetInstance(zSTRING("SELF"), npc);
					parser->SetInstance(zSTRING("OTHER"), hero);
					npc->ClearEM();
					npc->state.StartAIState(zSTRING("ZS_MAGICSLEEP"), FALSE, 0, 0.0f, FALSE);
					parser->SetInstance(zSTRING("SELF"), oldSelf);
					parser->SetInstance(zSTRING("OTHER"), oldOther);
					hits = 1;
				}
				break;
			}
			Log("sign %s cast: %d affected", SIGN_NAMES[which], hits);
		}

		static void UpdateSigns(oCNpc* hero)
		{
			float now = Now(), dt = FrameSeconds();
			if (signCasting >= 0 && now >= signThrowAt)
			{
				SignEffect(hero, signCasting);
				signCasting = -1;
			}
			bool fight = HostilesNear(hero, 1800.0f) > 0;
			stamina += (fight ? 10.0f : 100.0f) * dt;
			stamina = stamina > 100.0f ? 100.0f : stamina;
			for (Burning& b : burning)
				if (b.npc)
				{
					if (now > b.until || b.npc->IsDead())
						b.npc = nullptr;
					else if (now >= b.next)
					{
						b.next += 1.0f;
						Hurt(hero, b.npc, 0.027f, oEDamageType_Fire);
					}
				}
			// Yrden: enemies in its circle move at 0.74 of their speed; only those it slowed get
			// their speed back
			for (Slowed& s : slowedNpcs)
				if (s.npc && (now >= yrdenUntil || (s.npc->GetPositionWorld() - yrdenAt).Length() > 260.0f || s.npc->IsDead()))
				{
					if (zCModel* model = LoadedModel(s.npc))
						model->timeScale = 1.0f;
					s.npc = nullptr;
				}
			if (now < yrdenUntil)
				ForNpcsNear(yrdenAt, 260.0f, [&](oCNpc* npc)
				{
					zCModel* model = npc != hero && Hostile(npc, hero) ? LoadedModel(npc) : nullptr;
					if (!model || model->timeScale < 0.75f)
						return;
					for (Slowed& s : slowedNpcs)
						if (!s.npc)
						{
							s.npc = npc;
							model->timeScale = 0.74f;
							break;
						}
				});
		}

		// --- the hero, each frame --------------------------------------------------------------------
		static bool CameraModeAllowed()
		{
			zCAICamera* ai = zCAICamera::current;
			if (!ai || !ai->camVob)
				return false;
			zSTRING mode = ai->curcammode;
			mode.Upper();
			return mode == "CAMMODNORMAL" || mode == "CAMMODRUN" || mode == "CAMMODJUMP" || mode == "CAMMODJUMPUP"
				|| mode == "CAMMODFALL" || mode == "CAMMODSWIM" || mode == "CAMMODDIVE" || mode == "CAMMODLOOKBACK"
				|| mode == "CAMMODMELEE";
		}

		static bool ControlsActive(oCNpc* hero)
		{
			if (!settings.controls || !hero || hero->IsDead() || hero->IsUnconscious() || hero->GetInteractMob() || !CameraModeAllowed())
				return false;
			int mode = hero->GetWeaponMode();
			if (mode != NPC_WEAPON_NONE && mode != NPC_WEAPON_FIST && mode != NPC_WEAPON_1HS)
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

		static void ClearGothicKeys()
		{
			gForward = gForwardEdge = gAction = gActionEdge = gBack = gBackEdge = false;
			gStrafeL = gStrafeLEdge = gStrafeR = gStrafeREdge = gJump = gJumpEdge = false;
		}

		static void UpdateHero(oCNpc* hero)
		{
			float now = Now(), dt = FrameSeconds();
			ClearGothicKeys();
			GiveStartSword(hero);
			UpdateVoice(hero, HostilesNear(hero, 1500.0f)); // only with his controls on: never in a dialogue
			float up = (keyW.held ? 1.0f : 0.0f) - (keyS.held ? 1.0f : 0.0f);
			float right = (keyD.held ? 1.0f : 0.0f) - (keyA.held ? 1.0f : 0.0f);
			moving = up != 0.0f || right != 0.0f;
			bool startedMoving = moving && !wasMoving;
			wasMoving = moving;
			zVEC3 camFwd = cameraActive ? cameraForward : Flat(zCAICamera::current->camVob->GetAtVectorWorld());
			zVEC3 want = Flat(camFwd * up + RightOf(camFwd, hero) * right);
			int state = BodyState(hero);
			bool grounded = state != BS_JUMP && state != BS_FALL;
			bool swimming = state == BS_SWIM || state == BS_DIVE;
			bool sword = SwordDrawn(hero);

			// double taps of a direction key (within 0.2 s): a dodge that way
			int tapDodge = -1;
			Key* dirKeys[4] = { &keyW, &keyA, &keyS, &keyD };
			for (int i = 0; i < 4; i++)
				if (dirKeys[i]->pressed)
				{
					if (now - lastTap[i] < 0.2f)
						tapDodge = i;
					lastTap[i] = now;
				}

			if (keyCtrl.pressed)
			{
				walking = !walking;
				Log(walking ? "walk" : "run");
			}
			// signs: 3-7 choose, Q casts
			Key* signKeys[SIGNS] = { &key3, &key4, &key5, &key6, &key7 };
			for (int i = 0; i < SIGNS; i++)
				if (signKeys[i]->pressed)
				{
					sign = i;
					Log("sign: %s", SIGN_NAMES[i]);
				}
			// the sword: 1/2 draw it, C puts it away (a weapon message, as Gothic's own key does)
			bool drawKey = key1.pressed || key2.pressed || (mouseLeft.pressed && !sword && !keyShift.held);
			if (drawKey && !sword && hero->GetEquippedMeleeWeapon() && grounded && !swimming)
			{
				hero->GetEM()->OnMessage(new oCMsgWeapon(oCMsgWeapon::EV_DRAWWEAPON, NPC_WEAPON_1HS, 0), hero);
				Log("draw the sword");
			}
			if (keyC.pressed && sword)
			{
				hero->GetEM()->OnMessage(new oCMsgWeapon(oCMsgWeapon::EV_REMOVEWEAPON, NPC_WEAPON_NONE, 0), hero);
				Log("sheathe the sword");
			}
			// Z: lock on the nearest foe in front, again to let go
			if (keyZ.pressed)
			{
				lockTarget = lockTarget ? nullptr : NearestFoe(hero, camFwd, 2000.0f, 60.0f);
				Log(lockTarget ? "lock on %s" : "lock off", lockTarget ? lockTarget->GetInstanceName().ToChar() : "");
			}
			if (lockTarget && (!Hostile(lockTarget, hero) || (lockTarget->GetPositionWorld() - hero->GetPositionWorld()).Length() > 5000.0f))
				lockTarget = nullptr;

			// E: Gothic interacts on its action key plus a forward press while the hero stands
			if (keyE.pressed)
				useUntil = now + 0.5f;
			bool interacting = now < useUntil;

			// actions in progress: Gothic gets no movement until they may be followed
			UpdateSwing(hero);
			UpdateTurn(hero);
			bool busy = Busy();
			if (now > actionEnd)
				actionName = "";

			// combat input
			if (sword && grounded && !swimming && !interacting)
			{
				if (mouseRight.pressed)
					guardPressed = now;
				// a press during an action waits for its ActionBlend (one slot, the latest press);
				// while sprinting Shift is the sprint: a click is a run-in fast swing
				if (mouseLeft.pressed)
				{
					queuedAttack = keyShift.held && sprintBlend < 0.5f ? ATTACK_STRONG : ATTACK_FAST;
					queuedAt = now;
				}
				if (now - queuedAt > 1.5f)
					queuedAttack = ATTACK_NONE;
				bool wantGuard = mouseRight.held && !busy;
				if (wantGuard && !guarding)
				{
					guarding = true;
					StartAni(hero, "t_W3_GuardStart");
					Log("guard");
				}
				else if (!mouseRight.held && guarding)
				{
					guarding = false;
					StartAni(hero, "t_W3_GuardStop");
				}
				if (!busy && !guarding)
				{
					int dodgeDir = -1;
					if (keyAlt.pressed || tapDodge >= 0)
					{
						// the way to go relative to Geralt: forward, back, left, right
						zVEC3 at = Flat(hero->GetAtVectorWorld());
						zVEC3 side = RightOf(at, hero);
						if (tapDodge >= 0 || moving)
						{
							zVEC3 d = tapDodge >= 0 ? (tapDodge == 0 ? camFwd : (tapDodge == 2 ? camFwd * -1.0f
								: (tapDodge == 1 ? RightOf(camFwd, hero) * -1.0f : RightOf(camFwd, hero)))) : want;
							float f = at.Dot(d), s = side.Dot(d);
							dodgeDir = fabsf(f) >= fabsf(s) ? (f >= 0.0f ? 0 : 1) : (s >= 0.0f ? 3 : 2);
						}
						Dodge(hero, dodgeDir);
					}
					else if (keySpace.pressed)
						Roll(hero, want, moving);
					else if (queuedAttack != ATTACK_NONE)
					{
						bool sprinting = sprintBlend > 0.5f && moving;
						Swing(hero, queuedAttack == ATTACK_STRONG, moving ? want : camFwd, sprinting);
						queuedAttack = ATTACK_NONE;
						StopSprint(hero);
					}
					else if (keyQ.pressed)
						CastSign(hero);
				}
				else if (guarding)
					queuedAttack = ATTACK_NONE;
			}
			else
			{
				if (guarding)
				{
					guarding = false;
					StartAni(hero, "t_W3_GuardStop");
				}
				queuedAttack = ATTACK_NONE;
				if (!busy && keyQ.pressed && grounded && !swimming)
					CastSign(hero);
			}
			if (guarding && hero->GetModel() && !AniActive(hero, "t_W3_GuardStart") && !AniActive(hero, "s_W3_Guard"))
				StartAni(hero, "s_W3_Guard");

			// the game's turns (no sword out: his sword locomotion has none of its own): moving off
			// from standing towards the side or back pivots him into it; reversing while running,
			// walking or sprinting is the game's 180-degree turn. Moving again cuts a stop short.
			busy = Busy();
			if (settings.gameTurns && moving && !sword && grounded && !swimming && !interacting && !busy && !guarding)
			{
				LoadTurns();
				zVEC3 at = Flat(hero->GetAtVectorWorld());
				float c = at.Dot(want);
				c = c > 1.0f ? 1.0f : (c < -1.0f ? -1.0f : c);
				float deg = acosf(c) * 180.0f / PI * (RightOf(at, hero).Dot(want) > 0.0f ? -1.0f : 1.0f);
				float size = fabsf(deg);
				bool left = deg > 0.0f;
				bool stopping = Playing(hero, "t_RunL_2_Run") || Playing(hero, "t_RunR_2_Run")
					|| Playing(hero, "t_WalkL_2_Walk") || Playing(hero, "t_WalkR_2_Walk");
				if (startedMoving && stopping)
					StartAni(hero, walking ? "s_Walk" : "s_Run");
				bool standing = stopping || (state == BS_STAND && (Playing(hero, "s_Run") || Playing(hero, "s_Walk")));
				const char* turn = nullptr;
				if (standing && startedMoving && size > 50.0f)
				{
					bool back = size > 135.0f;
					if (walking)
						turn = back ? (left ? "t_W3_WalkStartBL" : "t_W3_WalkStartBR") : (left ? "t_W3_WalkStartL" : "t_W3_WalkStartR");
					else
						turn = back ? (left ? "t_W3_RunStartBL" : "t_W3_RunStartBR") : (left ? "t_W3_RunStartL" : "t_W3_RunStartR");
				}
				else if (size > 150.0f && now - turnEnded > 0.3f && (Playing(hero, "s_RunL") || Playing(hero, "s_WalkL")))
				{
					if (Playing(hero, "s_WalkL"))
						turn = left ? "t_W3_WalkTurnL" : "t_W3_WalkTurnR";
					else if (sprintBlend > 0.5f)
						turn = left ? "t_W3_SprintTurnL" : "t_W3_SprintTurnR";
					else
						turn = left ? "t_W3_RunTurnL" : "t_W3_RunTurnR";
				}
				if (turn)
					StartTurn(hero, turn, deg);
			}
			busy = Busy(); // an action begun this frame keeps Gothic's movement and the sprint off

			// what Gothic gets for its own keys
			bool still = busy || guarding || interacting;
			gAction = interacting;
			gActionEdge = keyE.pressed;
			gForward = interacting ? state == BS_STAND : (moving && !still);
			gForwardEdge = interacting && state == BS_STAND;
			if (!busy && keySpace.pressed && !sword && grounded && !swimming)
				gJump = gJumpEdge = true;

			// sprint: Left Shift held while he runs
			bool onFoot = moving && !still && !swimming;
			UpdateSprint(hero, onFoot && keyShift.held && !walking && !mouseLeft.held, !grounded);

			// turning: towards the way to go (the game's camera-relative movement), at turnRate
			// (turnRateRun running, turnRateSprint sprinting: wider curves the faster he goes)
			if (!moving || !grounded || still)
				return;
			zVEC3 at = Flat(hero->GetAtVectorWorld());
			float dot = at.Dot(want);
			dot = dot > 1.0f ? 1.0f : (dot < -1.0f ? -1.0f : dot);
			float angle = acosf(dot);
			float rate = sprintBlend > 0.5f ? settings.turnRateSprint : (state == BS_RUN ? settings.turnRateRun : settings.turnRate);
			float step = rate * PI / 180.0f * dt;
			hero->SetHeadingAtWorld(angle > step ? TurnToward(at, want, step / angle, hero) : want);
		}

		// The Witcher 3's camera: the mouse orbits it around a point above Geralt's feet; it sits
		// over his right shoulder (the modern exploration camera: 1.85 m, offset 0.55 m right and
		// 0.6 m back), closer in a fight (1.5 m, offset 0.74 / 0.38 / 0.15, further out with more
		// enemies), turned towards a locked target, pulled in front of walls.
		static void UpdateCamera(zCAICamera* ai)
		{
			oCNpc* hero = oCNpc::player;
			zCVob* camVob = ai ? ai->camVob : nullptr;
			if (!hero || !camVob || ai->target != hero || !w3Controls)
			{
				cameraActive = false;
				return;
			}
			float dt = FrameSeconds();
			int foes = SwordDrawn(hero) ? HostilesNear(hero, 1000.0f) : 0;
			combatBlend += (foes > 0 ? dt : -dt) / 1.0f;
			combatBlend = combatBlend < 0.0f ? 0.0f : (combatBlend > 1.0f ? 1.0f : combatBlend);
			float c = combatBlend * combatBlend * (3.0f - 2.0f * combatBlend);
			float d = foes < 1 ? 1.0f : (foes > 4 ? 4.0f : (float)foes);
			d /= 1.3f;
			float h = d / 1.5f > 1.0f ? d / 1.5f : 1.0f;
			float distance = settings.camDistance + (settings.combatDistance - settings.camDistance) * c;
			float side = settings.camSide + (settings.combatSide - settings.camSide) * c;
			float back = settings.camBack + (settings.combatBack + 20.0f * d - settings.camBack) * c;
			float lift = (settings.combatUp + 10.0f * h) * c;
			float height = settings.camHeight + (settings.combatHeight - settings.camHeight) * c;
			zVEC3 feet = hero->GetPositionWorld();
			feet[1] = hero->bbox3D.mins[1];
			zVEC3 target = feet + zVEC3(0.0f, height, 0.0f);
			if (!cameraActive)
			{
				cameraForward = Flat(camVob->GetAtVectorWorld());
				cameraPitch = 10.0f;
				cameraFocus = target;
				cameraActive = true;
			}
			if ((target - cameraFocus).Length() > 400.0f)
				cameraFocus = target;
			else
				cameraFocus = cameraFocus + (target - cameraFocus) * (1.0f - expf(-dt / 0.08f));

			float dx = 0.0f, dy = 0.0f, dz = 0.0f;
			zinput->GetMousePos(dx, dy, dz);
			float now = Now();
			if (fabsf(dx) > 0.5f)
				mouseMovedX = now;
			if (fabsf(dy) > 0.5f)
				mouseMovedY = now;
			if (lockTarget) // the lock turns the camera towards the target (the game's rate 1.5)
			{
				zVEC3 toTarget = Flat(lockTarget->GetPositionWorld() - hero->GetPositionWorld());
				float k = 1.0f - expf(-dt * 1.5f * 3.0f);
				cameraForward = Flat(cameraForward + (toTarget - cameraForward) * k);
				cameraPitch += (15.0f - cameraPitch) * k;
			}
			else
			{
				float yaw = dx * settings.mouseSpeed * PI / 180.0f;
				zVEC3 r = RightOf(cameraForward, hero);
				cameraForward = Flat(cameraForward * cosf(yaw) + r * sinf(yaw));
				cameraPitch += (settings.invertMouseY ? -dy : dy) * settings.mouseSpeed;
				cameraPitch = cameraPitch < -38.0f ? -38.0f : (cameraPitch > 60.0f ? 60.0f : cameraPitch);
				// the game's AutoCameraCenter (exploration.ws: with the mouse, 5 s horizontally and 3 s
				// vertically after it last turned the camera): while he moves, the camera swings in
				// behind him and back to level
				if (settings.camAutoCenter && moving && combatBlend < 0.5f)
				{
					float k = 1.0f - expf(-dt / (settings.camAutoTime > 0.05f ? settings.camAutoTime : 0.05f));
					if (now - mouseMovedX > settings.camAutoDelay)
						cameraForward = TurnToward(cameraForward, Flat(hero->GetAtVectorWorld()), k, hero);
					if (now - mouseMovedY > settings.camAutoDelayPitch)
						cameraPitch += (10.0f - cameraPitch) * k;
				}
			}
			float pitch = cameraPitch * PI / 180.0f;
			zVEC3 look = cameraForward * cosf(pitch) + zVEC3(0.0f, -sinf(pitch), 0.0f);
			zVEC3 r = RightOf(cameraForward, hero);
			zVEC3 offset = r * side + look * -back + zVEC3(0.0f, lift, 0.0f);
			zVEC3 ray = look * -distance + offset;
			zVEC3 position = cameraFocus + ray;
			zCWorld* world = hero->GetHomeWorld();
			int flags = zTRACERAY_STAT_POLY | zTRACERAY_POLY_IGNORE_TRANSP | zTRACERAY_VOB_IGNORE_NO_CD_DYN | zTRACERAY_VOB_IGNORE_CHARACTER;
			if (world && world->TraceRayNearestHit(cameraFocus, ray, hero, flags) && world->traceRayReport.foundHit)
			{
				zVEC3 hit = world->traceRayReport.foundIntersection;
				float len = ray.Length();
				float dist = (hit - cameraFocus).Length() - 15.0f;
				dist = dist > 30.0f ? dist : 30.0f;
				position = cameraFocus + ray * (dist / (len > 1.0f ? len : 1.0f));
			}
			camVob->SetPositionWorld(position);
			camVob->SetHeadingAtWorld(look);
		}

		// --- his face (make_geralt_model.FACE_ANIS: the face rig's bones in layered animations) -----
		// Gothic moves a face only through a morph-mesh head (EV_Output starts its VISEME, the AI
		// its R_EYESBLINK; Geralt's head is too big for one, see make_geralt_model), so the plugin
		// plays his: talking while his active message is one of his lines, a blink every 2-6 s,
		// the grimace when he is hit, and the S_ expressions scripts ask for (oCNpc::StartFaceAni).
		static float nextBlink = 0.0f;

		static bool HasFace(oCNpc* hero)
		{
			return settings.face && hero && AniID(hero->GetModel(), "T_W3_FACETALK") >= 0;
		}

		static void UpdateFace()
		{
			oCNpc* hero = oCNpc::player;
			if (!HasFace(hero))
				return;
			zCModel* model = hero->GetModel();
			bool talking = false;
			if (zCEventManager* em = hero->GetEM())
				if (zCEventMessage* active = em->GetActiveMessage())
					if (oCMsgConversation* line = zDYNAMIC_CAST<oCMsgConversation>(active))
						talking = line->subType == oCMsgConversation::EV_OUTPUT || line->subType == oCMsgConversation::EV_OUTPUTSVM;
			int talk = AniID(model, "T_W3_FACETALK");
			bool playing = Playing(hero, "T_W3_FACETALK");
			if (talking && !playing)
				model->StartAni(talk, 0);
			else if (!talking && playing)
				model->FadeOutAni(talk);
			float now = Now();
			if (now >= nextBlink)
			{
				if (nextBlink > 0.0f && !Playing(hero, "S_W3_FACEEYESCLOSED"))
					StartAni(hero, "T_W3_FACEBLINK");
				nextBlink = now + 2.0f + 4.0f * Random();
			}
		}

		static void FaceHurt(oCNpc* hero)
		{
			if (HasFace(hero))
				StartAni(hero, "T_W3_FACEHURT");
		}

		// Gothic's face animation names -> his; S_NEUTRAL (or none of these) ends the expression
		static const char* const FACE_EXPRESSIONS[][2] = {
			{ "S_FRIENDLY", "S_W3_FACEFRIENDLY" }, { "S_ANGRY", "S_W3_FACEANGRY" }, { "S_HOSTILE", "S_W3_FACEHOSTILE" },
			{ "S_FRIGHTENED", "S_W3_FACEFRIGHTENED" }, { "S_EYESCLOSED", "S_W3_FACEEYESCLOSED" } };

		static void FaceExpression(oCNpc* hero, const zSTRING& gothicName, bool start)
		{
			if (!HasFace(hero))
				return;
			zSTRING name = gothicName;
			name.Upper();
			const char* mine = nullptr;
			for (auto& e : FACE_EXPRESSIONS)
				if (name == e[0])
					mine = e[1];
			if (!mine && !(name == "S_NEUTRAL"))
				return;
			zCModel* model = hero->GetModel();
			for (auto& e : FACE_EXPRESSIONS)
				if (e[1] != mine || !start)
					if (Playing(hero, e[1]))
						model->FadeOutAni(AniID(model, e[1]));
			if (mine && start && !Playing(hero, mine))
				StartAni(hero, mine);
			Log("face: %s %s", name.ToChar(), start ? "on" : "off");
		}

		// Lines whose subtitle make_geralt_dialogue.py changed with their voice (Witcher_subtitles.txt
		// in the Gothic folder: block name, tab, text), set in Gothic's output-unit library once
		// it is loaded.
		static void UpdateSubtitles()
		{
			static bool done = false;
			if (done || !ogame)
				return;
			zCCSManager* cs = ogame->GetCutsceneManager();
			if (!cs || !cs->library || !cs->library->IsLoaded())
				return;
			done = true;
			FILE* file = nullptr;
			if (fopen_s(&file, "Witcher_subtitles.txt", "r") || !file)
				return;
			static char line[1024];
			zSTRING names[16], texts[16];
			int wanted = 0;
			while (fgets(line, sizeof(line), file) && wanted < 16)
			{
				line[strcspn(line, "\r\n")] = 0;
				char* tab = strchr(line, '\t');
				if (line[0] == '#' || !tab)
					continue;
				*tab = 0;
				names[wanted] = zSTRING(line);
				names[wanted].Upper();
				texts[wanted++] = zSTRING(tab + 1);
			}
			fclose(file);
			// the library keeps the blocks' names as the scripts spell them (Info_Diego_Gamestart_15_01):
			// a look through it, case aside (LibValidateOU found none of the upper-case names)
			int changed = 0;
			zCArraySort<zCCSBlock*>& blocks = cs->library->ouList;
			for (int i = 0; i < blocks.GetNum(); i++)
			{
				zCCSBlock* block = blocks[i];
				if (!block)
					continue;
				zSTRING name = block->roleName;
				name.Upper();
				for (int k = 0; k < wanted; k++)
				{
					if (!(name == names[k]))
						continue;
					zCCSAtomicBlock* atom = static_cast<zCCSAtomicBlock*>(block->GetChild(0));
					oCMsgConversation* msg = atom && atom->command ? zDYNAMIC_CAST<oCMsgConversation>(atom->command) : nullptr;
					if (msg)
					{
						msg->text = texts[k];
						changed++;
					}
				}
			}
			Log("subtitles: %d of %d of the hero's lines changed (%d output units, the first named %s)", changed, wanted,
				blocks.GetNum(), blocks.GetNum() > 0 && blocks[0] ? blocks[0]->roleName.ToChar() : "-");
		}

		static void Update()
		{
			LoadSettings();
			if (!started)
			{
				started = true;
				Log("Witcher plugin running");
			}
			UpdateBursts();
			UpdateFace();
			UpdateSubtitles();
		}
	}

	// Gothic keeps the player on the ground it moves over at its own pace; with Geralt's Witcher
	// speeds (run 5.4, sprint 6.7 m/s) its smoothed height correction left him behind the floor on
	// slopes, and its slide angle threw him off crests (the GTA mashup's findings, docs/09 in the
	// gothic2beyond knowledge base): height correction at once and steeper walkable ground.
	void __fastcall zCAIPlayer_PropagateCollObjectStates(zCAIPlayer* self, void* vtable, int state);
	auto Hook_zCAIPlayer_PropagateCollObjectStates = Union::CreateHook(reinterpret_cast<void*>(0x004FE610),
		&zCAIPlayer_PropagateCollObjectStates, Union::HookType::Hook_Detours);
	void __fastcall zCAIPlayer_PropagateCollObjectStates(zCAIPlayer* self, void* vtable, int state)
	{
		bool hero = Witcher::w3Controls && oCNpc::player && self->vob == oCNpc::player;
		if (hero)
		{
			zCAIPlayer::zTConfig& config = self->config;
			float walk = Witcher::settings.slopeWalk * Witcher::PI / 180.0f, slide = Witcher::settings.slopeSlide * Witcher::PI / 180.0f;
			config.zMV_MAX_GROUND_ANGLE_WALK = config.zMV_MAX_GROUND_ANGLE_WALK > walk ? config.zMV_MAX_GROUND_ANGLE_WALK : walk;
			config.zMV_MAX_GROUND_ANGLE_SLIDE = config.zMV_MAX_GROUND_ANGLE_SLIDE > slide ? config.zMV_MAX_GROUND_ANGLE_SLIDE : slide;
			config.zMV_MAX_GROUND_ANGLE_SLIDE2 = config.zMV_MAX_GROUND_ANGLE_SLIDE2 > slide ? config.zMV_MAX_GROUND_ANGLE_SLIDE2 : slide;
		}
		Hook_zCAIPlayer_PropagateCollObjectStates(self, vtable, state);
		if (hero)
			if (zCCollisionObject* collision = self->vob->GetCollisionObject())
				static_cast<zCCollObjectCharacter*>(collision)->m_oConfig.m_eDoHeightCorrectionSmooth = 0;
	}

	// A frame counts as at most maxFrameTime ms while the controls are on: on a stutter the game
	// slows down instead of Geralt leaping off a slope (the GTA mashup).
	void __fastcall zCTimer_ResetTimer(zCTimer* self, void* vtable);
	auto Hook_zCTimer_ResetTimer = Union::CreateHook(reinterpret_cast<void*>(0x005CE550), &zCTimer_ResetTimer, Union::HookType::Hook_Detours);
	void __fastcall zCTimer_ResetTimer(zCTimer* self, void* vtable)
	{
		Hook_zCTimer_ResetTimer(self, vtable);
		float cap = Witcher::settings.maxFrameTime, raw = self->frameTimeFloat;
		if (self != ztimer || !Witcher::w3Controls || cap < 1.0f || raw <= cap)
			return;
		unsigned long capped = (unsigned long)cap;
		self->totalTime -= self->frameTime - capped;
		self->totalTimeFloat -= raw - (float)capped;
		self->frameTime = capped;
		self->frameTimeFloat = (float)capped;
	}

	// A new game, or a savegame loaded: see Witcher::NewGame.
	void __fastcall oCGame_LoadGame(oCGame* self, void* vtable, int slot, zSTRING const& level);
	auto Hook_oCGame_LoadGame = Union::CreateHook(reinterpret_cast<void*>(0x0063C070), &oCGame_LoadGame, Union::HookType::Hook_Detours);
	void __fastcall oCGame_LoadGame(oCGame* self, void* vtable, int slot, zSTRING const& level)
	{
		Witcher::NewGame();
		Hook_oCGame_LoadGame(self, vtable, slot, level);
	}

	void __fastcall oCGame_LoadSavegame(oCGame* self, void* vtable, int slot, int loadGlobals);
	auto Hook_oCGame_LoadSavegame = Union::CreateHook(reinterpret_cast<void*>(0x0063C2A0), &oCGame_LoadSavegame, Union::HookType::Hook_Detours);
	void __fastcall oCGame_LoadSavegame(oCGame* self, void* vtable, int slot, int loadGlobals)
	{
		Witcher::NewGame();
		Hook_oCGame_LoadSavegame(self, vtable, slot, loadGlobals);
	}

	// The hero's AI reads its controls inside DoAI; the overrides apply only during that call.
	void __fastcall oCAIHuman_DoAI(oCAIHuman* self, void* vtable, zCVob* vob, int& vobDeleted);
	auto Hook_oCAIHuman_DoAI = Union::CreateHook(reinterpret_cast<void*>(0x00615A50), &oCAIHuman_DoAI, Union::HookType::Hook_Detours);
	void __fastcall oCAIHuman_DoAI(oCAIHuman* self, void* vtable, zCVob* vob, int& vobDeleted)
	{
		oCNpc* hero = oCNpc::player;
		if (!hero || self != hero->human_ai)
			return Hook_oCAIHuman_DoAI(self, vtable, vob, vobDeleted);
		Witcher::LoadSettings();
		Witcher::FullDetail(hero);
		if (zCModel* model = hero->GetModel()) // loaded (and with -zautoconvertdata compiled) at once,
			Witcher::LoadSprint(model);           // not first on a sprint in a later session without it
		Witcher::ReadKeys();
		Witcher::UpdateSigns(hero);
		int mode = hero->GetWeaponMode();
		if (mode != NPC_WEAPON_NONE && mode != NPC_WEAPON_FIST && mode != NPC_WEAPON_1HS && Witcher::settings.controls)
		{
			Witcher::Log("weapon mode %d put away (Geralt fights with his sword and signs)", mode);
			hero->SetWeaponMode(NPC_WEAPON_NONE);
		}
		static int lastActive = -1, lastMode = -1;
		bool active = Witcher::ControlsActive(hero);
		Witcher::w3Controls = active;
		if ((active ? 1 : 0) != lastActive || mode != lastMode)
		{
			Witcher::Log("controls %s, weapon mode %d, body state %d", active ? "on" : "off", mode, Witcher::BodyState(hero));
			lastActive = active ? 1 : 0;
			lastMode = mode;
		}
		if (!active)
		{
			Witcher::moving = Witcher::guarding = false;
			Witcher::queuedAttack = Witcher::ATTACK_NONE;
			Witcher::EndSwing(hero);
			Witcher::ClearGothicKeys();
			Witcher::StopSprint(hero);
			return Hook_oCAIHuman_DoAI(self, vtable, vob, vobDeleted);
		}
		Witcher::UpdateHero(hero);
		Witcher::overrideInput = true;
		Hook_oCAIHuman_DoAI(self, vtable, vob, vobDeleted);
		Witcher::overrideInput = false;
	}

	// With the controls on only The Witcher's keys reach the hero: every movement and combat key
	// Gothic asks for is answered from them (UpdateHero), Gothic's own bindings do nothing.
	static bool OverrideKey(unsigned short key, float& state, int& toggled)
	{
		using namespace Witcher;
		auto set = [&](bool on, bool pressed) { state = on ? 1.0f : 0.0f; toggled = pressed ? 1 : 0; return true; };
		if (w3Controls)
		{
			switch (key)
			{
			case GAME_WEAPON: case GAME_LOOK: case GAME_LOOK_FP: case GAME_LOCK_TARGET: case GAME_PARADE:
			case GAME_ACTIONLEFT: case GAME_ACTIONRIGHT: case GAME_ACTION2: case GAME_SMOVE2: case GAME_SHIFT:
			case GAME_LAME_POTION: case GAME_LAME_HEAL: case GAME_SNEAK:
				return set(false, false);
			}
		}
		if (!overrideInput)
			return false;
		switch (key)
		{
		case GAME_UP:
			state = gForward ? 1.0f : 0.0f;
			toggled = gForwardEdge ? 1 : (gForward ? -1 : 0); // -1: Gothic's own toggle while held
			return true;
		case GAME_DOWN: return set(gBack, gBackEdge);
		case GAME_STRAFELEFT: return set(gStrafeL, gStrafeLEdge);
		case GAME_STRAFERIGHT: return set(gStrafeR, gStrafeREdge);
		case GAME_LEFT: case GAME_RIGHT: return set(false, false);
		case GAME_ACTION: return set(gAction, gActionEdge);
		case GAME_SLOW: return set(walking, false);
		case GAME_SMOVE: return set(gJump, gJumpEdge);
		default: return false;
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

	// Gothic turns the hero with the mouse and its turn keys here; Geralt turns towards the
	// camera-relative direction instead.
	void __fastcall oCAIHuman_PC_Turnings(oCAIHuman* self, void* vtable, int force);
	auto Hook_oCAIHuman_PC_Turnings = Union::CreateHook(reinterpret_cast<void*>(0x00614800), &oCAIHuman_PC_Turnings, Union::HookType::Hook_Detours);
	void __fastcall oCAIHuman_PC_Turnings(oCAIHuman* self, void* vtable, int force)
	{
		if (!Witcher::overrideInput)
			Hook_oCAIHuman_PC_Turnings(self, vtable, force);
	}

	void __fastcall zCAICamera_DoAI(zCAICamera* self, void* vtable, zCVob* vob, int& vobDeleted);
	auto Hook_zCAICamera_DoAI = Union::CreateHook(reinterpret_cast<void*>(0x0049D6A0), &zCAICamera_DoAI, Union::HookType::Hook_Detours);
	void __fastcall zCAICamera_DoAI(zCAICamera* self, void* vtable, zCVob* vob, int& vobDeleted)
	{
		Hook_zCAICamera_DoAI(self, vtable, vob, vobDeleted);
		Witcher::UpdateCamera(self);
	}

	// Gothic's keys that would act on their own while The Witcher's controls are on: the number
	// keys (Gothic's weapons and spells), and the keys Geralt uses (Q, E, C, Z).
	int __fastcall oCGame_HandleEvent(oCGame* self, void* vtable, int key);
	auto Hook_oCGame_HandleEvent = Union::CreateHook(reinterpret_cast<void*>(0x0065EEE0), &oCGame_HandleEvent, Union::HookType::Hook_Detours);
	int __fastcall oCGame_HandleEvent(oCGame* self, void* vtable, int key)
	{
		if (Witcher::w3Controls)
			switch (key)
			{
			case KEY_1: case KEY_2: case KEY_3: case KEY_4: case KEY_5: case KEY_6: case KEY_7: case KEY_8:
			case KEY_9: case KEY_0: case KEY_Q: case KEY_E: case KEY_C: case KEY_Z: case KEY_SPACE:
			case KEY_LCONTROL: case KEY_LMENU: case KEY_LSHIFT:
				return TRUE;
			}
		return Hook_oCGame_HandleEvent(self, vtable, key);
	}

	// Gothic armour would replace Geralt's body with a Gothic armour mesh: he keeps his own.
	void __fastcall oCNpc_EquipArmor(oCNpc* self, void* vtable, oCItem* item);
	auto Hook_oCNpc_EquipArmor = Union::CreateHook(reinterpret_cast<void*>(0x00697080), &oCNpc_EquipArmor, Union::HookType::Hook_Detours);
	void __fastcall oCNpc_EquipArmor(oCNpc* self, void* vtable, oCItem* item)
	{
		if (Witcher::settings.controls && self && self == oCNpc::player)
		{
			Witcher::Log("armour %s not put on: Geralt keeps his own", item ? item->GetInstanceName().ToChar() : "?");
			return;
		}
		Hook_oCNpc_EquipArmor(self, vtable, item);
	}

	// Damage: Geralt's hits (what the plugin deals itself, and any Gothic's own fight code lands)
	// as shares of the victim's health, and hits on him: Quen takes the whole of one, a dodge or
	// roll makes melee miss, his guard parries or counters (see the header).
	void __fastcall oCNpc_OnDamage_Hit(oCNpc* self, void* vtable, oCNpc::oSDamageDescriptor& desc);
	auto Hook_oCNpc_OnDamage_Hit = Union::CreateHook(reinterpret_cast<void*>(0x00731410), &oCNpc_OnDamage_Hit, Union::HookType::Hook_Detours);
	void __fastcall oCNpc_OnDamage_Hit(oCNpc* self, void* vtable, oCNpc::oSDamageDescriptor& desc)
	{
		using namespace Witcher;
		oCNpc* hero = oCNpc::player;
		auto setDamage = [&](float amount, int type, int index)
		{
			int damage = (int)(amount + 0.5f);
			damage = damage < 1 ? 1 : damage;
			int protection = self->protection[index];
			for (unsigned long& d : desc.aryDamage)
				d = 0;
			desc.aryDamage[index] = damage + protection;
			desc.fDamageTotal = (float)(damage + protection);
			desc.enuModeDamage = type;
			desc.dwFieldsValid |= oCNpc::oEDamageDescFlag_Damage | oCNpc::oEDamageDescFlag_DamageType;
		};
		if (self == plugTarget)
		{
			int index = (plugType & oEDamageType_Fire) ? oEDamageIndex_Fire : ((plugType & oEDamageType_Edge) ? oEDamageIndex_Edge : oEDamageIndex_Blunt);
			setDamage(plugDamage, plugType, index);
			if (plugFlyOn)
			{
				desc.aryDamage[oEDamageIndex_Fly] = self->protection[oEDamageIndex_Fly] + 1;
				desc.fDamageTotal += (float)desc.aryDamage[oEDamageIndex_Fly];
				desc.vecDirectionFly = plugFly;
				desc.dwFieldsValid |= oCNpc::oEDamageDescFlag_FlyDirection;
			}
			return Hook_oCNpc_OnDamage_Hit(self, vtable, desc);
		}
		if (hero && self == hero)
			FaceHurt(hero);
		if (hero && self == hero && w3Controls)
		{
			oCNpc* attacker = desc.pNpcAttacker;
			bool melee = attacker && attacker->GetWeaponMode() >= NPC_WEAPON_FIST && attacker->GetWeaponMode() <= NPC_WEAPON_2HS;
			float now = Now();
			if (now < quenUntil)
			{
				quenUntil = -1.0f;
				Emit("MFX_HEAL_START", hero->GetPositionWorld(), 1.0f);
				Log("Quen took the hit");
				return;
			}
			if (melee && now < evadeUntil && AngleTo(evadeDir, hero->GetPositionWorld(), attacker->GetPositionWorld()) > 30.0f)
			{
				Log("dodged %s", attacker->GetInstanceName().ToChar());
				return;
			}
			if (melee && guarding && AngleTo(hero->GetAtVectorWorld(), hero->GetPositionWorld(), attacker->GetPositionWorld()) < 90.0f)
			{
				if (now - guardPressed < settings.counterWindow)
					Counter(hero, attacker);
				else
				{
					StartAni(hero, "t_1hParade_O");
					Log("parried %s", attacker->GetInstanceName().ToChar());
				}
				return;
			}
			if (swinging)
			{
				Log("swing %s interrupted by a hit", actionName);
				EndSwing(hero); // the game skips the hit of an interrupted swing
			}
		}
		else if (hero && desc.pNpcAttacker == hero && self != hero && w3Controls && SwordDrawn(hero))
		{
			// a sword hit of Gothic's own: the share of a fast swing
			setDamage(SwordShare(hero, self, settings.fastDamage) * self->attribute[NPC_ATR_HITPOINTSMAX], oEDamageType_Edge, oEDamageIndex_Edge);
			Log("fast swing hit %s (%d/%d)", self->GetInstanceName().ToChar(), self->attribute[NPC_ATR_HITPOINTS], self->attribute[NPC_ATR_HITPOINTSMAX]);
		}
		Hook_oCNpc_OnDamage_Hit(self, vtable, desc);
	}

	// The face animations scripts ask for (Mdl_StartFaceAni and the like): the hero's go to his
	// face bones (Witcher::FaceExpression), Gothic's own call still runs.
	int __fastcall oCNpc_StartFaceAni(oCNpc* self, void* vtable, zSTRING const& name, float intensity, float holdTime);
	auto Hook_oCNpc_StartFaceAni = Union::CreateHook(reinterpret_cast<void*>(0x00695440), &oCNpc_StartFaceAni, Union::HookType::Hook_Detours);
	int __fastcall oCNpc_StartFaceAni(oCNpc* self, void* vtable, zSTRING const& name, float intensity, float holdTime)
	{
		if (self && self == oCNpc::player)
			Witcher::FaceExpression(self, name, true);
		return Hook_oCNpc_StartFaceAni(self, vtable, name, intensity, holdTime);
	}

	int __fastcall oCNpc_StopFaceAni(oCNpc* self, void* vtable, zSTRING const& name);
	auto Hook_oCNpc_StopFaceAni = Union::CreateHook(reinterpret_cast<void*>(0x00695730), &oCNpc_StopFaceAni, Union::HookType::Hook_Detours);
	int __fastcall oCNpc_StopFaceAni(oCNpc* self, void* vtable, zSTRING const& name)
	{
		if (self && self == oCNpc::player)
			Witcher::FaceExpression(self, name, false);
		return Hook_oCNpc_StopFaceAni(self, vtable, name);
	}

	void __fastcall oCGame_Render(oCGame* self, void* vtable);
	auto Hook_oCGame_Render = Union::CreateHook(reinterpret_cast<void*>(0x0063DBE0), &oCGame_Render, Union::HookType::Hook_Detours);
	void __fastcall oCGame_Render(oCGame* self, void* vtable)
	{
		Witcher::Update();
		Hook_oCGame_Render(self, vtable);
	}
}
