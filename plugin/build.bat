@echo off
rem Builds the mashups' Union plugins for Gothic 1 (GothicMod.exe 1.08k_mod) with MSVC x86:
rem out\build\G1-Release\gta\vdf\CJMovement.vdf and out\build\G1-Release\witcher\vdf\Witcher.vdf.
rem "build.bat CJMovement" or "build.bat Witcher" builds that one only.
rem Needs Visual Studio (or its Build Tools) with the C++ workload and CMake.
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (echo Visual Studio not found & exit /b 1)
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (echo Visual Studio with the C++ workload not found & exit /b 1)
call "%VSDIR%\VC\Auxiliary\Build\vcvars32.bat" >nul || exit /b 1
cd /d "%~dp0"
if not exist dependencies\union\union-api\CMakeLists.txt git submodule update --init --recursive || exit /b 1
cmake --preset G1-Release || exit /b 1
if "%~1"=="" (
	cmake --build --preset G1-Release || exit /b 1
) else (
	cmake --build --preset G1-Release --target build_vdf_%~1 || exit /b 1
)
