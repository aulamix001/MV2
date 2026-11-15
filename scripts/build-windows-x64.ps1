$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
# Run on Windows with Visual Studio 2022 C++ Build Tools and CMake installed.
cmake -S . -B build-windows-msvc-x64 -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
cmake --build build-windows-msvc-x64 --config Release --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'Windows x64 compilation failed.' }
ctest --test-dir build-windows-msvc-x64 -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Windows tests failed; package was not generated.' }
$target = 'build-windows-msvc-x64/MV2Recreation_artefacts/Release'
$plugin = "$target/VST3/MV2 Recreation.vst3/Contents/x86_64-win/MV2 Recreation.vst3"
foreach ($file in @($plugin, "$target/Standalone/MV2 Recreation.exe")) {
    $bytes = [IO.File]::ReadAllBytes((Resolve-Path $file).Path)
    if ($bytes.Length -lt 64 -or $bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a) { throw "Not a Windows PE binary: $file" }
    $header = [BitConverter]::ToInt32($bytes, 0x3c)
    if ([BitConverter]::ToUInt32($bytes, $header) -ne 0x4550 -or [BitConverter]::ToUInt16($bytes, $header + 4) -ne 0x8664) { throw "Not a Windows x64 binary: $file" }
}
New-Item -ItemType Directory -Force dist | Out-Null
Compress-Archive -Path "$target/VST3/MV2 Recreation.vst3", "$target/Standalone/MV2 Recreation.exe" -DestinationPath 'dist/MV2-Recreation-Windows-x64.zip' -Force
Write-Output 'Created: dist/MV2-Recreation-Windows-x64.zip'
