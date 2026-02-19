@echo off
setlocal

:: StickBrawl Windows Build Script
:: Requires: cmake, Visual Studio (or MinGW), vcpkg

set SCRIPT_DIR=%~dp0
set BUILD_DIR=%SCRIPT_DIR%build
set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

:: --- Locate vcpkg ---
if defined VCPKG_ROOT (
    set VCPKG_TOOLCHAIN=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
) else if exist "%USERPROFILE%\vcpkg\scripts\buildsystems\vcpkg.cmake" (
    set VCPKG_TOOLCHAIN=%USERPROFILE%\vcpkg\scripts\buildsystems\vcpkg.cmake
) else (
    echo ERROR: Cannot find vcpkg.
    echo   Set VCPKG_ROOT or install vcpkg to %%USERPROFILE%%\vcpkg:
    echo     git clone https://github.com/microsoft/vcpkg.git %%USERPROFILE%%\vcpkg
    echo     %%USERPROFILE%%\vcpkg\bootstrap-vcpkg.bat
    exit /b 1
)

echo === StickBrawl Build ===
echo   Build type : %BUILD_TYPE%
echo   Build dir  : %BUILD_DIR%
echo   vcpkg      : %VCPKG_TOOLCHAIN%
echo.

:: --- Install dependencies via vcpkg ---
echo --- Installing dependencies via vcpkg ---
for %%I in ("%VCPKG_TOOLCHAIN%") do set VCPKG_BIN=%%~dpI..\vcpkg.exe
if exist "%VCPKG_BIN%" (
    "%VCPKG_BIN%" install sfml box2d nlohmann-json
) else (
    echo Warning: vcpkg.exe not found, assuming deps are already installed.
)
echo.

:: --- Configure ---
echo --- Configuring (CMake) ---
cmake -B "%BUILD_DIR%" -S "%SCRIPT_DIR%" -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%"
if errorlevel 1 (
    echo CMake configure failed.
    exit /b 1
)
echo.

:: --- Build ---
echo --- Building ---
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE%
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)
echo.

echo === Build complete ===
echo   Run: %BUILD_DIR%\%BUILD_TYPE%\StickBrawl.exe

endlocal
