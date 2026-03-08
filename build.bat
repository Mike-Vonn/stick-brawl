@echo off
set CONFIG=Release
if /i "%1"=="debug" set CONFIG=Debug

if not exist build (
    cmake -B build
)
cmake --build build --config %CONFIG%
