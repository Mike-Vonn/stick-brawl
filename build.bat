@echo off
if /i "%1"=="clean" (
    if exist build rmdir /s /q build
    echo Build directory cleaned.
    exit /b
)

set CONFIG=Release
if /i "%1"=="debug" set CONFIG=Debug

if not exist build (
    cmake -B build -DCMAKE_TOOLCHAIN_FILE=D:/Home/vcpkg/scripts/buildsystems/vcpkg.cmake
)
cmake --build build --config %CONFIG%
