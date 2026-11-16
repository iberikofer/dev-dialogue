<#
.SYNOPSIS
    Builds the Dev Dialogue desktop app and packs it into a Windows installer.

.DESCRIPTION
    1. cmake --build the desktop app (Release)
    2. windeployqt the Qt runtime into dist\stage
    3. compile installer\dev-dialogue.iss -> dist\DevDialogue-Setup-<version>-x64.exe

    Requires: CMake + Ninja + Qt 6 (MSYS2 ucrt64), Inno Setup 6.
#>
[CmdletBinding()]
param(
    [string] $Configuration = 'Release',
    # Where the bootstrapper fetches the full installer from. Defaults to the
    # GitHub release of the current version.
    [string] $PayloadUrl = ''
)

$ErrorActionPreference = 'Stop'

$root     = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $root 'apps\desktop\build'
$stageDir = Join-Path $root 'dist\stage'
$issFile  = Join-Path $root 'installer\dev-dialogue.iss'
$exeName  = 'DevDialogue.exe'

function Find-Tool {
    param([string] $Name, [string[]] $Candidates)
    foreach ($candidate in $Candidates) {
        if (Test-Path $candidate) { return $candidate }
    }
    $onPath = Get-Command $Name -ErrorAction SilentlyContinue
    if ($onPath) { return $onPath.Source }
    return $null
}

# 1. Configure (only when there is no build folder yet).
if (-not (Test-Path (Join-Path $buildDir 'CMakeCache.txt'))) {
    Write-Host '== Configuring (CMake) =='
    cmake -B $buildDir -S (Join-Path $root 'apps\desktop') -DCMAKE_BUILD_TYPE=$Configuration
    if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
}

# 2. Build.
Write-Host '== Building =='
cmake --build $buildDir --config $Configuration
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }

# 3. Locate the executable (single-config generators put it straight in build/).
$exe = Join-Path $buildDir (Join-Path $Configuration $exeName)
if (-not (Test-Path $exe)) { $exe = Join-Path $buildDir $exeName }
if (-not (Test-Path $exe)) { throw "Built executable not found in $buildDir." }

# 4. Locate windeployqt and the Qt tool directories.
$qtBin = 'C:\msys64\ucrt64\bin'
$deploy = Find-Tool 'windeployqt' @(
    (Join-Path $qtBin 'windeployqt.exe'),
    (Join-Path $qtBin 'windeployqt-qt6.exe'),
    (Join-Path $qtBin 'windeployqt6.exe')
)
if (-not $deploy) { throw 'windeployqt not found. Install Qt 6 (MSYS2) or add it to PATH.' }
$qtTools = Join-Path (Split-Path -Parent $qtBin) 'share\qt6\bin'

# windeployqt starts qmlimportscanner, which MSYS2 keeps in share/qt6/bin.
$env:PATH = "$qtBin;$qtTools;$env:PATH"

# 5. Stage the runtime: the executable, the Qt runtime, the plugins and the
#    QML modules - nothing else, no build leftovers.
Write-Host '== Staging runtime =='
if (Test-Path $stageDir) { Remove-Item $stageDir -Recurse -Force }
New-Item -ItemType Directory -Path $stageDir | Out-Null
Copy-Item $exe (Join-Path $stageDir $exeName) -Force
& $deploy --qmldir (Join-Path $root 'apps\desktop\qml') --no-translations (Join-Path $stageDir $exeName)
if ($LASTEXITCODE -ne 0) { throw 'windeployqt failed.' }

# The QML module of the app itself has to sit next to the executable: the QML
# engine resolves "import DevDialogue.App" from the file system, not from the
# resource. The module list comes from the qmldir CMake generates in the build.
$moduleDir = Join-Path $stageDir 'DevDialogue\App'
Remove-Item (Join-Path $stageDir 'DevDialogue') -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $moduleDir -Force | Out-Null
Copy-Item (Join-Path $root 'apps\desktop\qml') (Join-Path $moduleDir 'qml') -Recurse -Force
Copy-Item (Join-Path $buildDir 'DevDialogue\App\qmldir') (Join-Path $moduleDir 'qmldir') -Force
Copy-Item (Join-Path $buildDir 'DevDialogue\App\DevDialogue.qmltypes') (Join-Path $moduleDir 'DevDialogue.qmltypes') -Force -ErrorAction SilentlyContinue

# windeployqt does not write this file: Qt needs it to find the plugins and
# the QML import path next to the executable.
@"
[Paths]
Prefix = .
QmlImports = qml
Plugins = .
"@ | Set-Content -Path (Join-Path $stageDir 'qt.conf') -Encoding ASCII

# windeployqt does not copy every runtime dependency of the MinGW build (ICU
# above all). Resolve the dependency closure and fill the gaps.
$objdump = Find-Tool 'objdump' @(
    (Join-Path $qtBin 'objdump.exe')
)
if ($objdump) {
    $present = @{}
    Get-ChildItem $stageDir -Recurse -File -Include *.dll, *.exe |
        ForEach-Object { $present[$_.Name] = $true }
    $scanned = @()
    do {
        $added = $false
        $scanned = Get-ChildItem $stageDir -Recurse -File -Include *.dll, *.exe
        foreach ($file in $scanned) {
            $imports = & $objdump -p $file.FullName 2>$null |
                Select-String 'DLL Name' |
                ForEach-Object { ($_.Line -replace '.*DLL Name:\s*', '').Trim() }
            foreach ($import in $imports) {
                if ($present[$import]) { continue }
                $source = Join-Path $qtBin $import
                if (-not (Test-Path $source)) { continue }
                Copy-Item $source $stageDir
                $present[$import] = $true
                $added = $true
            }
        }
    } while ($added)
}

# 6. Compile the installer.
$iscc = Find-Tool 'ISCC' @(
    (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe'),
    'C:\Program Files (x86)\Inno Setup 6\ISCC.exe',
    'C:\Program Files\Inno Setup 6\ISCC.exe'
)
if (-not $iscc) {
    throw 'Inno Setup 6 (ISCC.exe) not found. Install it with: winget install JRSoftware.InnoSetup'
}

Write-Host '== Building installer =='
& $iscc $issFile
if ($LASTEXITCODE -ne 0) { throw 'Inno Setup failed.' }

# 7. Bootstrapper: a small installer that downloads the installer above.
$bootstrapIss = Join-Path $root 'installer\dev-dialogue-bootstrap.iss'
$payload = Get-ChildItem (Join-Path $root 'dist') -Filter 'DevDialogue-Setup-*.exe' |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $payload) { throw 'Installer was not produced.' }
$payloadHash = (Get-FileHash $payload.FullName -Algorithm SHA256).Hash.ToLower()

if (-not $PayloadUrl) {
    $remote = (& git -C $root remote get-url origin) -join ''
    $slug = ($remote -replace '^.*github\.com[:/]', '') -replace '\.git$', ''
    $version = ([regex]::Match((Get-Content $issFile -Raw), '#define AppVersion "([^"]+)"')).Groups[1].Value
    $PayloadUrl = "https://github.com/$slug/releases/download/v$version/$($payload.Name)"
}

Write-Host '== Building bootstrapper =='
& $iscc "/DPayloadUrl=$PayloadUrl" "/DPayloadSha256=$payloadHash" "/DPayloadSize=$($payload.Length)" $bootstrapIss
if ($LASTEXITCODE -ne 0) { throw 'Inno Setup (bootstrapper) failed.' }

# 8. Report.
$bootstrap = Get-ChildItem (Join-Path $root 'dist') -Filter 'DevDialogue-Bootstrap-*.exe' |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
Write-Host ''
Write-Host ('Full installer:  {0}  ({1:N1} MB)' -f $payload.FullName, ($payload.Length / 1MB))
Write-Host ('Bootstrapper:   {0}  ({1:N1} MB)' -f $bootstrap.FullName, ($bootstrap.Length / 1MB))
Write-Host ('Payload URL:    {0}' -f $PayloadUrl)
Write-Host ('Staged runtime: {0:N1} MB' -f ((Get-ChildItem $stageDir -Recurse -File | Measure-Object Length -Sum).Sum / 1MB))
