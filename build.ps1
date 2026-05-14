#powershell -ExecutionPolicy ByPass -File build.ps1
param(
  [string]$webeid = $PSScriptRoot,
  [string]$cmake = "C:\Program Files\CMake\bin\cmake.exe",
  [string]$vcpkgroot = $env:VCPKG_ROOT,
  [string]$build_number = $(if ($null -eq $env:BUILD_NUMBER) {"0"} else {$env:BUILD_NUMBER}),
  [string]$version = (Select-String -Path "$webeid/CMakeLists.txt" -Pattern 'project\(web-eid VERSION (\d+\.\d+\.\d+)').Matches[0].Groups[1].Value + ".$build_number",
  [bool]$crosscompile = ($env:PROCESSOR_ARCHITECTURE -eq "AMD64"),
  [string]$qt_dir = "C:\Qt\6.11.1",
  [string]$qt_x64 = "$qt_dir\msvc2022_64",
  [string]$qt_arm64 = $(if ($crosscompile -and (Test-Path "$qt_dir\msvc2022_arm64_cross_compiled")) { "$qt_dir\msvc2022_arm64_cross_compiled" } else { "$qt_dir\msvc2022_arm64" }),
  [string]$buildtype = "RelWithDebInfo",
  [string]$sign = $null
)

$ErrorActionPreference = "Stop"

Try { & wix > $null } Catch {
  & dotnet tool install --global --version 6.0.2 wix
  & wix extension add -g WixToolset.UI.wixext/6.0.2
  & wix extension add -g WixToolset.Util.wixext/6.0.2
  & wix extension add -g WixToolset.BootstrapperApplications.wixext/6.0.2
}

if(!(Test-Path -Path $vcpkgroot)) {
  $vcpkgroot = "$webeid\vcpkg"
  & git clone https://github.com/microsoft/vcpkg $vcpkgroot
  & $vcpkgroot\bootstrap-vcpkg.bat
}

$vcvars = "${env:VSINSTALLDIR}VC\Auxiliary\Build\vcvarsall.bat"
if (-not (Test-Path $vcvars)) {
  $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
  $vcvars = "$(& $vswhere -latest -property installationPath)\VC\Auxiliary\Build\vcvarsall.bat"
}

$msi = @{}

foreach ($arch in @("x64", "arm64")) {
  $env:PLATFORM = $arch
  $buildpath = "$webeid\build\$arch"
  $qt_path = if ($arch -eq "arm64") { $qt_arm64 } else { $qt_x64 }

  $vsarch = if ($arch -eq "arm64") { if ($crosscompile) { "amd64_arm64" } else { "arm64" } } else { "amd64" }
  cmd /c "`"$vcvars`" $vsarch && set" | Where-Object { $_ -match '=' } | ForEach-Object {
    $name, $value = $_ -split '=', 2
    [System.Environment]::SetEnvironmentVariable($name, $value)
  }

  $cmakeargs = @(
    "-A", $(if ($arch -eq "arm64") { "ARM64" } else { "x64" }),
    "-S", $webeid,
    "-B", $buildpath,
    "-DCMAKE_BUILD_TYPE=$buildtype",
    "-DCMAKE_PREFIX_PATH=$qt_path",
    "-DCMAKE_TOOLCHAIN_FILE=$vcpkgroot\scripts\buildsystems\vcpkg.cmake",
    "-DVCPKG_MANIFEST_DIR=$webeid\lib\libelectronic-id"
  )
  if ($arch -eq "arm64" -and $crosscompile) {
    $cmakeargs += "-DQT_HOST_PATH=$($qt_x64 -replace '\\','/')"
  }
  if ($sign) { $cmakeargs += "-DSIGNCERT=$sign" }

  & $cmake @cmakeargs
  & $cmake --build $buildpath --config $buildtype
  & $cmake --build $buildpath --config $buildtype --target installer

  $msi[$arch] = "$buildpath\src\app\$buildtype\web-eid_$version.$arch"
}

# Bundle
$bundle = "$webeid\build\web-eid_$version.exe"
& wix build -nologo `
  -ext WixToolset.BootstrapperApplications.wixext `
  -ext WixToolset.Util.wixext `
  -d webeid_x64="$($msi['x64'])" `
  -d webeid_arm64="$($msi['arm64'])" `
  -d MSI_VERSION=$version `
  -d path="$webeid\install" `
  "$webeid\install\plugins.wxs" `
  -o $bundle

if ($sign) {
  $signargs = @("sign", "/a", "/v", "/s", "MY", "/n", $sign,
    "/fd", "SHA256", "/du", "http://installer.id.ee",
    "/tr", "http://timestamp.digicert.com", "/td", "SHA256")
  $engine = "$webeid\build\web-eid_$version.engine.exe"
  & wix burn detach -nologo $bundle -engine $engine
  & signtool.exe @signargs $engine
  & wix burn reattach -nologo $bundle -engine $engine -o $bundle
  & signtool.exe @signargs $bundle
  Remove-Item $engine
}
