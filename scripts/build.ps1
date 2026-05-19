$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$cmake = "C:\Qt\Tools\CMake_64\bin\cmake.exe"

Set-Location $projectRoot

& $cmake -S . -B build -G Ninja `
    -DCMAKE_PREFIX_PATH=C:/Qt/6.11.1/mingw_64 `
    -DCMAKE_C_COMPILER=C:/Qt/Tools/mingw1310_64/bin/gcc.exe `
    -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe `
    -DCMAKE_MAKE_PROGRAM=C:/Qt/Tools/Ninja/ninja.exe `
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

& $cmake --build build
