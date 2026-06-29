param(
    [string]$BuildDir = "build",
    [string]$BuildType = "Release",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

# AUTOMATSKA DETEKCIJA: Proveravamo da li je CMakeLists.txt u istom folderu gde i skripta
if (Test-Path (Join-Path $PSScriptRoot "CMakeLists.txt")) {
    $ProjectRoot = $PSScriptRoot
} else {
    # Ako nije, pretpostavljamo da je skripta unutar nekog podfoldera (npr. /scripts)
    $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

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

# 1. Generisanje CMake fajlova
Write-Host "Konfigurisanje CMake-a..."
cmake -S $ProjectRoot -B $FullBuildDir -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=$BuildType
if ($LASTEXITCODE -ne 0) {
    throw "CMake konfiguracija nije uspela. Prekidam izvršavanje."
}

# 2. Kompajliranje (Build)
Write-Host "Kompajliranje projekta..."
cmake --build $FullBuildDir --parallel
if ($LASTEXITCODE -ne 0) {
    throw "Kompajliranje (Build) nije uspelo. Prekidam izvršavanje."
}

# 3. Pokretanje jediničnih testova
Write-Host "Pokretanje testova..."
Push-Location $FullBuildDir
try {
    ctest --output-on-failure -C $BuildType
    $testExit = $LASTEXITCODE
} finally {
    Pop-Location
}

if ($testExit -ne 0) {
    throw "Neki testovi nisu prošli. Privremeni fajlovi su sačuvani radi debagovanja. Exit code: $testExit"
}

# 4. Čišćenje "junk" fajlova - ostavljamo samo .exe fajlove u /build folderu
Write-Host "Čišćenje CMake ostataka... Čuvam samo .exe fajlove."

$BinDir = Join-Path $FullBuildDir "bin"
$TempExeDir = Join-Path $ProjectRoot "temp_exe_hold"

if (Test-Path $BinDir) {
    # Napravi privremeni folder van build-a i skloni .exe fajlove tamo da prežive brisanje
    New-Item -ItemType Directory -Force -Path $TempExeDir | Out-Null
    Move-Item -Path (Join-Path $BinDir "*.exe") -Destination $TempExeDir -Force

    # Kompletno obriši ceo build folder sa svim CMake generisanim smećem
    Remove-Item -Recurse -Force $FullBuildDir

    # Ponovo napravi prazan build folder i vrati samo .exe fajlove unutra
    New-Item -ItemType Directory -Force -Path $FullBuildDir | Out-Null
    Move-Item -Path (Join-Path $TempExeDir "*.exe") -Destination $FullBuildDir -Force

    # Obriši privremeni holding folder
    Remove-Item -Recurse -Force $TempExeDir

    Write-Host "Uspešno očišćeno! Izvršni fajlovi se nalaze u: $FullBuildDir"
} else {
    Write-Warning "Direktorijum sa binarnim fajlovima (bin) nije pronađen u build folderu."
}