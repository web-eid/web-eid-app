param(
  [string]$cmake = "C:\Program Files\CMake\bin\cmake.exe",
  [string]$vcpkgroot = "C:\vcpkg",
  [string]$qtdir = "C:\Qt\6.5.2\msvc2019_64"
)

& $cmake -A x64 "-DCMAKE_TOOLCHAIN_FILE=$vcpkgroot\scripts\buildsystems\vcpkg.cmake" "-DQt6_DIR=$qtdir" -S . -B build
& $cmake --build build
