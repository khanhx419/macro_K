@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo       Building Macro Recorder K (Standalone Static)
echo ========================================================

REM Find MinGW g++ in PATH or from WinGet Packages
where g++ >nul 2>nul
if %errorlevel% neq 0 (
    for /d %%i in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT*") do (
        if exist "%%i\mingw64\bin\g++.exe" (
            set "PATH=%%i\mingw64\bin;!PATH!"
        )
    )
)

where g++ >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] g++ compiler not found in PATH!
    pause
    exit /b 1
)

if not exist "bin" mkdir bin
if not exist "macros" mkdir macros

echo [1/4] Compiling Standalone Win32 GUI App (bin\macro_k_gui.exe)...
g++ -std=c++17 -O2 -mwindows -static -static-libgcc -static-libstdc++ ^
    src\gui\main_gui.cpp ^
    src\core\HookManager.cpp ^
    src\core\MacroPlayer.cpp ^
    src\core\MacroStorage.cpp ^
    -lcomctl32 -lcomdlg32 -lwinmm -luser32 -lgdi32 ^
    -o bin\macro_k_gui.exe

if %errorlevel% neq 0 (
    echo [ERROR] Failed to compile macro_k_gui.exe!
    pause
    exit /b %errorlevel%
)

echo [2/4] Compiling Standalone CLI Application (bin\macro_k.exe)...
g++ -std=c++17 -O2 -static -static-libgcc -static-libstdc++ ^
    src\cli\main.cpp ^
    src\core\HookManager.cpp ^
    src\core\MacroPlayer.cpp ^
    src\core\MacroStorage.cpp ^
    -lwinmm -luser32 ^
    -o bin\macro_k.exe

if %errorlevel% neq 0 (
    echo [ERROR] Failed to compile macro_k.exe!
    pause
    exit /b %errorlevel%
)

echo [3/4] Compiling Core DLL (bin\macro_k_core.dll)...
g++ -std=c++17 -O2 -shared -static-libgcc -static-libstdc++ ^
    src\core\MacroCoreApi.cpp ^
    src\core\HookManager.cpp ^
    src\core\MacroPlayer.cpp ^
    src\core\MacroStorage.cpp ^
    -lwinmm -luser32 ^
    -Wl,--out-implib,bin\libmacro_k_core.a ^
    -o bin\macro_k_core.dll

if %errorlevel% neq 0 (
    echo [ERROR] Failed to compile macro_k_core.dll!
    pause
    exit /b %errorlevel%
)

echo [4/4] Compiling Unit Tests (bin\test_core.exe)...
g++ -std=c++17 -O2 -static -static-libgcc -static-libstdc++ ^
    src\tests\test_core.cpp ^
    src\core\MacroStorage.cpp ^
    -lwinmm ^
    -o bin\test_core.exe

if %errorlevel% neq 0 (
    echo [ERROR] Failed to compile test_core.exe!
    pause
    exit /b %errorlevel%
)

echo.
echo ========================================================
echo [SUCCESS] Build completed successfully! (Zero Dependencies)
echo   - GUI App   : bin\macro_k_gui.exe  (Giao dien do hoa)
echo   - CLI App   : bin\macro_k.exe      (Giao dien console)
echo   - Core DLL  : bin\macro_k_core.dll (Thu vien dung chung)
echo   - Tests     : bin\test_core.exe    (Kiem thu don vi)
echo ========================================================
