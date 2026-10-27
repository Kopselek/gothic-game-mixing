@echo off
rem Builds the mashups' Union plugins with MSVC x86, each one DLL for Gothic 1 (GothicMod.exe
rem 1.08k_mod) and Gothic 2 Night of the Raven (Gothic2.exe 2.6):
rem out\build\G1-G2A-Release\<game>\vdf\<plugin>.vdf: gta\vdf\CJMovement.vdf, witcher\vdf\Witcher.vdf,
rem minecraft\vdf\Minecraft.vdf and sims\vdf\Sims.vdf.
rem "build.bat CJMovement" (or Witcher, Minecraft, Sims) builds that one only.
rem Needs Visual Studio (or its Build Tools) with the C++ workload and CMake.
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (echo Visual Studio not found & exit /b 1)
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (echo Visual Studio with the C++ workload not found & exit /b 1)
call "%VSDIR%\VC\Auxiliary\Build\vcvars32.bat" >nul || exit /b 1
cd /d "%~dp0"
if not exist dependencies\union\union-api\CMakeLists.txt git submodule update --init --recursive || exit /b 1
cmake --preset G1-G2A-Release || exit /b 1
if "%~1"=="" (
	cmake --build --preset G1-G2A-Release || exit /b 1
) else (
	cmake --build --preset G1-G2A-Release --target build_vdf_%~1 || exit /b 1
)
