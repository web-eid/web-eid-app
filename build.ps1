param(
  [string]$cmake = "C:\Program Files\CMake\bin\cmake.exe",
  [string]$vcpkgroot = "C:\vcpkg",
  [string]$qtdir = "C:\Qt\6.10.0\msvc2022_64",
  [string]$buildtype = "RelWithDebInfo",
  [string]$arch = "x64"
)

& $cmake -S . -B build\windows -A $arch -DCMAKE_BUILD_TYPE=$buildtype "-DCMAKE_PREFIX_PATH=$qtdir" "-DCMAKE_TOOLCHAIN_FILE=$vcpkgroot\scripts\buildsystems\vcpkg.cmake" "-DVCPKG_MANIFEST_DIR=lib/libelectronic-id"
& $cmake --build build\windows --config $buildtype
