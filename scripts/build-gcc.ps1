param(
    [string]$BuildDir = "build-gcc",
    [string]$BuildType = "Release",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$FullBuildDir = Join-Path $ProjectRoot $BuildDir

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "CMake nije pronađen u PATH okruženju."
}

if (-not (Get-Command g++ -ErrorAction SilentlyContinue)) {
    throw "g++ nije pronađen u PATH okruženju. Pokreni skriptu iz MinGW/MSYS2 okruženja ili dodaj g++ u PATH."
}

if ($Clean -and (Test-Path $FullBuildDir)) {
    Remove-Item -Recurse -Force $FullBuildDir
}

cmake -S $ProjectRoot -B $FullBuildDir -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=$BuildType
cmake --build $FullBuildDir --parallel
Write-Host "Running tests..."
Push-Location $FullBuildDir
try {
    ctest --output-on-failure -C $BuildType
    $testExit = $LASTEXITCODE
} finally {
    Pop-Location
}
if ($testExit -ne 0) {
    throw "Neki testovi nisu prošli. Exit code: $testExit"
}
