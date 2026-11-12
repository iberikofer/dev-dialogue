<#
.SYNOPSIS
    Builds a self contained Windows bundle of the Dev Dialogue desktop client.

.DESCRIPTION
    Configures a release build, deploys the Qt runtime with windeployqt and
    copies everything into a staging directory that the Inno Setup script
    (installer/installer.iss) turns into the distributable installer.

.EXAMPLE
    pwsh apps/desktop/scripts/package-windows.ps1 -Installer
#>
[CmdletBinding()]
param(
    [string]$Configuration = "Release",
    [string]$BuildDir = "build",
    [string]$StageDir = "dist\package",
    [string]$QtPrefix = "",
    [switch]$Installer
)

$ErrorActionPreference = "Stop"

$desktopRoot = Split-Path -Parent $PSScriptRoot
$repoRoot = Split-Path -Parent (Split-Path -Parent $desktopRoot)

Push-Location $desktopRoot

try {
    Write-Host "==> Configuring ($Configuration)" -ForegroundColor Cyan
    cmake -S . -B $BuildDir "-DCMAKE_BUILD_TYPE=$Configuration" `
        "-DDEVDIALOGUE_BUILD_TESTS=OFF"
    if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed." }

    Write-Host "==> Building" -ForegroundColor Cyan
    cmake --build $BuildDir --config $Configuration --parallel
    if ($LASTEXITCODE -ne 0) { throw "Build failed." }

    $executable = Join-Path $BuildDir "$Configuration\DevDialogue.exe"
    if (-not (Test-Path $executable)) {
        $executable = Join-Path $BuildDir "DevDialogue.exe"
    }
    if (-not (Test-Path $executable)) {
        throw "DevDialogue.exe was not produced."
    }
    $executable = (Resolve-Path $executable).Path

    $windeployqt = ""
    if ($QtPrefix -ne "") {
        $candidate = Join-Path $QtPrefix "bin\windeployqt.exe"
        if (Test-Path $candidate) { $windeployqt = $candidate }
    }
    if ($windeployqt -eq "") {
        $windeployqt = (Get-Command windeployqt.exe -ErrorAction SilentlyContinue).Source
    }
    if (-not $windeployqt) {
        $windeployqt = Get-ChildItem "C:\msys64" -Recurse -Filter "windeployqt.exe" `
            -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
    }
    if (-not $windeployqt) { throw "windeployqt.exe could not be located." }

    Write-Host "==> Deploying the Qt runtime" -ForegroundColor Cyan
    & $windeployqt --qmldir (Join-Path $desktopRoot "qml") `
        --compiler-runtime --no-translations --no-system-d3d-compiler `
        --no-opengl-sw $executable
    if ($LASTEXITCODE -ne 0) { throw "windeployqt failed." }

    # The MinGW runtime is not part of the Qt deployment.
    foreach ($runtime in @("libstdc++-6.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll")) {
        $candidate = Get-ChildItem "C:\msys64" -Recurse -Filter $runtime `
            -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($candidate) {
            Copy-Item $candidate.FullName (Split-Path -Parent $executable) -Force
        }
    }

    Write-Host "==> Staging the bundle" -ForegroundColor Cyan
    $stagePath = Join-Path $repoRoot $StageDir
    if (Test-Path $stagePath) { Remove-Item $stagePath -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $stagePath | Out-Null

    $bundleDir = Split-Path -Parent $executable
    Copy-Item $executable $stagePath -Force

    # The Qt runtime and the QML modules deployed next to the executable.
    Get-ChildItem $bundleDir -Filter "*.dll" | ForEach-Object {
        Copy-Item $_.FullName $stagePath -Force
    }

    foreach ($folder in @("generic", "iconengines", "imageformats", "meta_types",
                          "networkinformation", "platforms", "qml", "qmltooling",
                          "qmltypes", "tls", "translations")) {
        $source = Join-Path $bundleDir $folder
        if (Test-Path $source) {
            Copy-Item $source $stagePath -Recurse -Force
        }
    }

    # The descriptor of the application's own QML module. The QML sources
    # themselves live inside the executable, only the `qmldir` has to be
    # reachable next to it.
    $moduleDescriptor = Join-Path $bundleDir "DevDialogue\App\qmldir"
    if (Test-Path $moduleDescriptor) {
        $moduleTarget = Join-Path $stagePath "DevDialogue\App"
        New-Item -ItemType Directory -Force -Path $moduleTarget | Out-Null
        Copy-Item $moduleDescriptor $moduleTarget -Force
    }

    # The deployed QML modules live next to the executable in `qml`.
    Set-Content -Path (Join-Path $stagePath "qt.conf") `
        -Value "[Paths]`nQml2Imports=qml`nQmlImports=qml`n" -NoNewline

    Write-Host "Bundle staged at $stagePath" -ForegroundColor Green

    if ($Installer) {
        $iscc = @(
            "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
            "$env:ProgramFiles\Inno Setup 6\ISCC.exe"
        ) | Where-Object { Test-Path $_ } | Select-Object -First 1

        if (-not $iscc) {
            Write-Warning "Inno Setup (ISCC.exe) was not found, skipping the installer."
        } else {
            Write-Host "==> Compiling the installer" -ForegroundColor Cyan
            & $iscc (Join-Path $desktopRoot "installer\installer.iss")
            if ($LASTEXITCODE -ne 0) { throw "Inno Setup failed." }
            Write-Host "Installer written to $(Join-Path $repoRoot 'dist')" -ForegroundColor Green
        }
    }
}
finally {
    Pop-Location
}
