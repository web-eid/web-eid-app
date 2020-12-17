param(
  [string]$cmake = "C:\Program Files\CMake\bin\cmake.exe",
  [string]$vcpkgroot = "C:\vcpkg",
  [string]$qtdir = "C:\Qt\5.12.7\msvc2017_64"
)

$PROJECT_ROOT = split-path -parent $MyInvocation.MyCommand.Definition

Push-Location -Path "$PROJECT_ROOT\build"
& $cmake -A x64 "-DCMAKE_TOOLCHAIN_FILE=$vcpkgroot\scripts\buildsystems\vcpkg.cmake" "-DQt5_DIR=$qtdir/lib/cmake/Qt5" ..
& $cmake --build .
Pop-Location
