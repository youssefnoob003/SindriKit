@echo off
setlocal

set CMAKE_FLAGS=-DSND_ENABLE_DEBUG=OFF
set BUILD_TIER=SILENT
set BUILD_OUTPUT=DEBUGGER
set HASH_ALGO=DJB2
set BUILD_TESTS=OFF
set BUILD_POCS=OFF
set BUILD_CRTLESS=OFF
set CLEAN_BUILD=OFF
set RANDOMIZE=OFF

:parse_args
if "%~1"=="" goto :done_args

if /I "%~1"=="debug" (
    set CMAKE_FLAGS=%CMAKE_FLAGS% -DSND_ENABLE_DEBUG=ON
    set BUILD_TIER=DEBUG
) else if /I "%~1"=="console" (
    set CMAKE_FLAGS=%CMAKE_FLAGS% -DSND_USE_PRINTF=ON
    set BUILD_OUTPUT=CONSOLE
) else if /I "%~1"=="fnv1a" (
    set HASH_ALGO=FNV1A
) else if /I "%~1"=="djb2" (
    set HASH_ALGO=DJB2
) else if /I "%~1"=="tests" (
    :: Tests automatically enable DEBUG, PRINTF, and POCS
    set CMAKE_FLAGS=%CMAKE_FLAGS% -DSND_ENABLE_DEBUG=ON -DSND_USE_PRINTF=ON -DSND_BUILD_TESTS=ON -DSND_BUILD_PAYLOADS=ON
    set BUILD_TIER=DEBUG
    set BUILD_OUTPUT=CONSOLE
    set BUILD_TESTS=ON
    set BUILD_POCS=ON
) else if /I "%~1"=="pocs" (
    set CMAKE_FLAGS=%CMAKE_FLAGS% -DSND_BUILD_PAYLOADS=ON
    set BUILD_POCS=ON
) else if /I "%~1"=="crtless" (
    set CMAKE_FLAGS=%CMAKE_FLAGS% -DSND_CRTLESS=ON
    set BUILD_CRTLESS=ON
) else if /I "%~1"=="clean" (
    set CLEAN_BUILD=ON
) else if /I "%~1"=="random" (
    set CMAKE_FLAGS=%CMAKE_FLAGS% -DSND_RANDOMIZE_SEED=ON
    set RANDOMIZE=ON
) else (
    echo [!] Unknown argument: %~1
    echo.
    echo Usage: build.bat [debug] [console] [djb2^|fnv1a] [tests] [pocs] [crtless] [clean] [random]
    echo.
    echo    debug      Enable debug prints  ^(default: silent^)
    echo    console    Use printf output    ^(default: OutputDebugString^)
    echo    djb2       Use DJB2 hashing     ^(default^)
    echo    fnv1a      Use FNV1A hashing
    echo    tests      Build test fixtures  ^(implies debug and console^)
    echo    pocs       Build PoC binaries
    echo    crtless    Build CRT-less
    echo    clean      Delete build dirs before compiling
    echo    random     Use randomized seed for compile-time hashes
    exit /b 1
)
shift
goto :parse_args

:done_args
set CMAKE_FLAGS=%CMAKE_FLAGS% -DSND_HASH_ALGO=%HASH_ALGO%

echo [*] SindriKit Build
echo [*]   Tier    : %BUILD_TIER%
echo [*]   Output  : %BUILD_OUTPUT%
echo [*]   Hash    : %HASH_ALGO%
echo [*]   Tests   : %BUILD_TESTS%
echo [*]   PoCs    : %BUILD_POCS%
echo [*]   CRTless : %BUILD_CRTLESS%
echo [*]   Random  : %RANDOMIZE%
echo.

if "%CLEAN_BUILD%"=="ON" (
    echo [*] Cleaning previous builds...
    if exist build32 rmdir /s /q build32
    if exist build64 rmdir /s /q build64
    echo [+] Clean.
    echo.
)

echo [*] Configuring x86...
cmake -B build32 -A Win32 %CMAKE_FLAGS%
if %errorlevel% neq 0 (
    echo [!] CMake configure failed ^(x86^).
    exit /b %errorlevel%
)

echo [*] Building x86...
cmake --build build32 --config Release
if %errorlevel% neq 0 (
    echo [!] Build failed ^(x86^).
    exit /b %errorlevel%
)
echo.

echo [*] Configuring x64...
cmake -B build64 -A x64 %CMAKE_FLAGS%
if %errorlevel% neq 0 (
    echo [!] CMake configure failed ^(x64^).
    exit /b %errorlevel%
)

echo [*] Building x64...
cmake --build build64 --config Release
if %errorlevel% neq 0 (
    echo [!] Build failed ^(x64^).
    exit /b %errorlevel%
)
echo.

echo [+] Done. %BUILD_TIER% / %BUILD_OUTPUT% / %HASH_ALGO%
endlocal
