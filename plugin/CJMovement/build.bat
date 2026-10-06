@echo off
rem Builds CJMovement.dll / CJMovement.vdf for Gothic 1 (GothicMod.exe 1.08k_mod) with MSVC x86.
rem Needs Visual Studio (or its Build Tools) with the C++ workload and CMake; the result is
rem out\build\G1-Release\vdf\CJMovement.vdf.
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (echo Visual Studio not found & exit /b 1)
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (echo Visual Studio with the C++ workload not found & exit /b 1)
call "%VSDIR%\VC\Auxiliary\Build\vcvars32.bat" >nul || exit /b 1
cd /d "%~dp0"
if not exist dependencies\union\union-api\CMakeLists.txt git submodule update --init --recursive || exit /b 1
cmake --preset G1-Release || exit /b 1
cmake --build --preset G1-Release || exit /b 1
