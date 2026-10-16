# The mashup's Union plugin

`plugin/` is the shared build (union-plugin-template, made per plugin in
`plugin/cmake/add_union_plugin.cmake`; the Union libraries are submodules in
`plugin/dependencies/union`). A mashup brings only two files.

## `mashups/<game>/plugin/CMakeLists.txt`

```cmake
# The <Game> mashup's Union plugin, <Plugin>.vdf. plugin\build.bat builds it with the other
# mashups' plugins (plugin/CMakeLists.txt); all of its code is in src/Plugin.ipp.
project(<Plugin>
	DESCRIPTION "<Game>'s ... for <Hero> in Gothic 1 and Gothic 2"
	VERSION ${PROJECT_VERSION}
)

add_union_plugin()
```

Then `add_subdirectory(../mashups/<game>/plugin <game>)` in `plugin/CMakeLists.txt` (the build
folder must be `NAME`). `plugin\build.bat <Plugin>` builds
`plugin\out\build\G1-G2A-Release\<game>\vdf\<Plugin>.vdf`: the DLL and `UnionAPI.dll` packed for
Union to load from `Data`.

## `mashups/<game>/plugin/src/Plugin.ipp`

Every `.ipp` in `src/` is compiled **once per engine**, Gothic 1 and Gothic 2 Night of the Raven,
inside `namespace GOTHIC_NAMESPACE` (`plugin/src/IppFileTemplate.in`), into one DLL; Union runs
the hooks of the engine the game is. So:

- Engine classes and functions come from the gothic-api headers (`<ZenGin/zGothicAPI.h>`,
  precompiled), with each engine's own addresses.
- Hook through `SIGNATURE_OF(&Class::Method)`, never a raw address, so the same line hooks both
  games. Overloaded methods need a `static_cast` to the exact member function type.
- Content that differs goes through `GOTHIC2 ? ... : ...`.
- Put the mashup's own code in `namespace <Plugin>` inside `GOTHIC_NAMESPACE`. Each plugin is
  its own DLL, so hook names only need to be unique within it (GTA and The Witcher 3 use
  `oCAIHuman_DoAI`, Minecraft prefixes them: `Mc_oCAIHuman_DoAI`).

Start the file with a comment that says what the plugin does and lists its settings: the other
mashups' plugins do, and their READMEs' Settings sections come from it.

```cpp
// <Hero> from <Game> as the hero of Gothic 1 (GothicMod.exe 1.08k_mod) or Gothic 2 Night of the
// Raven (Gothic2.exe 2.6). make_<prefix>_model.py gives the hero <Hero>'s body (<HERO>.MDS);
// this plugin gives him <Game>'s ...
//
// Settings, read from Gothic.ini section [<PLUGIN>] at start (defaults in brackets):
//   controls=1        <Game>'s controls and camera (0: Gothic's own)
//   debugLog=0        1: write <Plugin>.log in the Gothic folder

namespace GOTHIC_NAMESPACE
{
	namespace <Plugin>
	{
		// This file is compiled once per engine: Gothic 1 and Gothic 2 Night of the Raven.
		static constexpr bool GOTHIC2 = ENGINE == Engine_G2A;

		struct Settings
		{
			bool controls = true;
			bool debugLog = false;
		};

		static Settings settings;
		static bool settingsLoaded = false;

		static void LoadSettings()
		{
			if (settingsLoaded || !zoptions)
				return;
			settingsLoaded = true;
			zSTRING s = "<PLUGIN>";
			settings.controls = zoptions->ReadBool(s, "controls", 1) != 0;
			settings.debugLog = zoptions->ReadBool(s, "debugLog", 0) != 0;
		}

		static void Log(const char* format, ...)
		{
			if (!settings.debugLog)
				return;
			FILE* file = nullptr;
			if (fopen_s(&file, "<Plugin>.log", "a") || !file)
				return;
			va_list args;
			va_start(args, format);
			fprintf(file, "%8.2f  ", ztimer ? ztimer->totalTimeFloat / 1000.0f : 0.0f);
			vfprintf(file, format, args);
			fputc('\n', file);
			va_end(args);
			fclose(file);
		}

		// The hero is ours when his model is <HERO>.MDS (make_<prefix>_model.py).
		static bool IsHero(oCNpc* npc)
		{
			zCModel* model = npc && npc->GetVisual() ? npc->GetModel() : nullptr;
			if (!model || model->modelProtoList.GetNum() == 0 || !model->modelProtoList[0])
				return false;
			zSTRING name = model->modelProtoList[0]->modelProtoName;
			name.Upper();
			return name == "<HERO>";
		}
	}

	// The hero's AI, every frame: where the plugin takes over from Gothic.
	void __fastcall X_oCAIHuman_DoAI(oCAIHuman* self, void* vtable, zCVob* vob, int& vobDeleted);
	auto Hook_X_oCAIHuman_DoAI = Union::CreateHook(SIGNATURE_OF(&oCAIHuman::DoAI), &X_oCAIHuman_DoAI, Union::HookType::Hook_Detours);
	void __fastcall X_oCAIHuman_DoAI(oCAIHuman* self, void* vtable, zCVob* vob, int& vobDeleted)
	{
		oCNpc* hero = oCNpc::player;
		if (hero && self == hero->human_ai)
		{
			<Plugin>::LoadSettings();
			static bool started = false;
			if (!started)
			{
				started = true;
				<Plugin>::Log("<Plugin> plugin (%s), hero %s", <Plugin>::GOTHIC2 ? "Gothic 2" : "Gothic 1",
					<Plugin>::IsHero(hero) ? "<Hero>" : "not <Hero>");
			}
			// ... the mashup's movement, camera, combat for the hero
		}
		Hook_X_oCAIHuman_DoAI(self, vtable, vob, vobDeleted); // Gothic's own
	}
}
```

A hook is three lines: the replacement's declaration, `Union::CreateHook(...)` giving the
original as `Hook_...`, and the replacement, which calls `Hook_...(self, vtable, ...)` for
Gothic's own behaviour. Methods are `__fastcall` with `self` and an unused `vtable` (`edx`)
before the real arguments.

## Where to look before writing a feature

The hooks the existing plugins already use, with both engines' signatures checked:

| Need | Hook (see the mashup's `Plugin.ipp`) |
|---|---|
| the hero each frame, take over movement | `oCAIHuman::DoAI` (all three) |
| the camera | `zCAICamera::DoAI` (all three) |
| keys Gothic reads | `zCInput_Win32::GetState`, `GetToggled` (all three); `KeyPressed`, `KeyToggled`, `GetMouseButtonPressedLeft` (GTA) |
| Gothic's turning off | `oCAIHuman::PC_Turnings` (all three) |
| Gothic's fall check off | `zCAIPlayer::CheckPhysics` (Minecraft) |
| pose bones after Gothic's animation | `zCModel::AdvanceAnis` (Minecraft) |
| HUD drawn over Gothic's | `zCView::DrawItems` (GTA, Minecraft), `oCGame::UpdatePlayerStatus` (GTA, Minecraft) |
| drawing every frame | `oCGame::Render` (GTA, The Witcher 3) |
| damage rules | `oCNpc::OnDamage_Hit` (GTA, The Witcher 3) |
| overlays swapped for the hero's skeleton | `zCModel::ApplyModelProtoOverlay`, `RemoveModelProtoOverlay` (Minecraft) |
| other characters' look, armour | `oCNpc::SetAdditionalVisuals` (Minecraft), `oCNpc::EquipArmor` (The Witcher 3, Minecraft) |
| face animations | `oCNpc::StartFaceAni`, `StopFaceAni` (The Witcher 3) |
| game keys and events | `oCGame::HandleEvent` (The Witcher 3, Minecraft) |
| items used | `oCNpc::UseItem` (Minecraft) |
| load, new game, level change | `oCGame::LoadGame`, `LoadSavegame` (all three), `ChangeLevel` (Minecraft) |
| frame time, pause | `zCTimer::ResetTimer` (GTA, The Witcher 3), `oCWorldTimer::Timer` (GTA) |

Search a plugin's `Plugin.ipp` for `Union::CreateHook` to see all of its hooks.

## Debugging

- `debugLog=1`, reproduce, read `<Plugin>.log`. Log state changes and every decision the plugin
  makes that Gothic would have made otherwise; log once per change, not every frame.
- A crash: log the step the plugin is in before each part of its frame (Minecraft's `step` and
  `InstallCrashLog()` write the exception and the plugin's return addresses before Gothic's
  handler ends the game).
- Union's console (its debug window) shows which plugins loaded.
