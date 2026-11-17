<#
.SYNOPSIS
    Installs or updates Ollama (if missing or too old), ensures the Ollama service
    is running, pulls the required local AI model (default: gemma4:e4b), and
    downloads the local Parakeet TDT 0.6B v3 speech-to-text ONNX model.
#>
[CmdletBinding()]
param(
    [string] $Model = 'gemma4:e4b',
    [string] $OllamaUrl = 'http://127.0.0.1:11434',
    [string] $SttDir = (Join-Path $env:LOCALAPPDATA 'DevDialogue\models\parakeet-tdt-0.6b')
)

$ErrorActionPreference = 'Stop'

function Find-OllamaExe {
    $cmd = Get-Command 'ollama' -ErrorAction SilentlyContinue
    if ($cmd -and (Test-Path $cmd.Source)) {
        return $cmd.Source
    }
    $candidates = @(
        (Join-Path $env:LOCALAPPDATA 'Programs\Ollama\ollama.exe'),
        (Join-Path $env:ProgramFiles 'Ollama\ollama.exe'),
        (Join-Path ${env:ProgramFiles(x86)} 'Ollama\ollama.exe')
    )
    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path $candidate)) {
            return $candidate
        }
    }
    return $null
}

function Test-OllamaApi {
    param([string] $Url)
    try {
        $null = Invoke-RestMethod -Uri "$($Url.TrimEnd('/'))/api/tags" -Method Get -TimeoutSec 2
        return $true
    }
    catch {
        return $false
    }
}

function Install-OrUpdateOllama {
    Write-Host "Stopping existing Ollama processes (if running)..."
    Stop-Process -Name 'ollama app' -Force -ErrorAction SilentlyContinue
    Stop-Process -Name 'ollama' -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 800

    Write-Host "Downloading latest official Ollama Windows installer..."
    $installerPath = Join-Path $env:TEMP 'OllamaSetup.exe'
    Invoke-WebRequest -Uri 'https://ollama.com/download/OllamaSetup.exe' -OutFile $installerPath -UseBasicParsing
    Write-Host "Installing Ollama silently..."
    $proc = Start-Process -FilePath $installerPath -ArgumentList '/VERYSILENT', '/NORESTART' -Wait -PassThru
    if ($proc.ExitCode -ne 0) {
        throw "Ollama installer exited with code $($proc.ExitCode)."
    }
    Remove-Item $installerPath -Force -ErrorAction SilentlyContinue
    $exe = Find-OllamaExe
    if (-not $exe) {
        throw "Ollama was installed, but ollama.exe could not be located."
    }
    return $exe
}

function Ensure-OllamaServing {
    param([string] $OllamaExe, [string] $Url)
    if (Test-OllamaApi -Url $Url) {
        return
    }
    Write-Host "Starting Ollama background server..."
    Start-Process -FilePath $OllamaExe -ArgumentList 'serve' -WindowStyle Hidden
    for ($i = 0; $i -lt 30; $i++) {
        Start-Sleep -Milliseconds 500
        if (Test-OllamaApi -Url $Url) {
            return
        }
    }
    throw "Ollama server did not respond on $Url."
}

Write-Host "== Dev Dialogue: Local AI Model Setup ($Model) =="

$ollama = Find-OllamaExe
if (-not $ollama) {
    $ollama = Install-OrUpdateOllama
}

Ensure-OllamaServing -OllamaExe $ollama -Url $OllamaUrl

$tags = Invoke-RestMethod -Uri "$($OllamaUrl.TrimEnd('/'))/api/tags" -Method Get -TimeoutSec 5
$alreadyPresent = $false
if ($tags.models) {
    foreach ($m in $tags.models) {
        if ($m.name -eq $Model -or $m.model -eq $Model) {
            $alreadyPresent = $true
            break
        }
    }
}

if ($alreadyPresent) {
    Write-Host "Local model '$Model' is already installed."
}
else {
    Write-Host "Pulling local model '$Model' (~5.5 GB). Please wait..."
    $pullOutput = & $ollama pull $Model 2>&1
    $pullExit = $LASTEXITCODE
    $pullOutput | ForEach-Object { Write-Host $_ }

    if ($pullExit -ne 0) {
        $combined = ($pullOutput | Out-String)
        if ($combined -match '412' -or $combined -match 'newer version of Ollama') {
            Write-Host "Current Ollama version is too old for '$Model'. Updating Ollama and retrying..."
            $ollama = Install-OrUpdateOllama
            Ensure-OllamaServing -OllamaExe $ollama -Url $OllamaUrl
            & $ollama pull $Model
            if ($LASTEXITCODE -ne 0) {
                throw "Failed to pull model '$Model' even after updating Ollama (exit code $LASTEXITCODE)."
            }
        }
        else {
            throw "Failed to pull model '$Model' (exit code $pullExit)."
        }
    }
}

Write-Host "Local AI model '$Model' is ready!"

# ------------------------------------------------------------------------------
# Install Parakeet TDT 0.6B v3 (INT8 ONNX) Speech-to-Text model
# ------------------------------------------------------------------------------
Write-Host "== Dev Dialogue: Speech-to-Text Model Setup (parakeet-tdt-0.6b-v3) =="
$typeWhisperDir = Join-Path $env:LOCALAPPDATA 'TypeWhisper\PluginData\com.typewhisper.sherpa-onnx\Models\parakeet-tdt-0.6b'
$sttFiles = @('encoder.int8.onnx', 'decoder.int8.onnx', 'joiner.int8.onnx', 'tokens.txt')

function Test-SttModelComplete {
    param([string] $Dir)
    if (-not $Dir -or -not (Test-Path $Dir)) { return $false }
    foreach ($f in $sttFiles) {
        if (-not (Test-Path (Join-Path $Dir $f))) { return $false }
    }
    return $true
}

if ((Test-SttModelComplete $SttDir) -or (Test-SttModelComplete $typeWhisperDir)) {
    Write-Host "Parakeet TDT 0.6B v3 STT model is already installed."
}
else {
    New-Item -ItemType Directory -Path $SttDir -Force | Out-Null
    $baseUrl = 'https://huggingface.co/csukuangfj/sherpa-onnx-nemo-parakeet-tdt-0.6b-v3-int8/resolve/main'
    foreach ($f in $sttFiles) {
        $dest = Join-Path $SttDir $f
        if (-not (Test-Path $dest)) {
            Write-Host "Downloading STT file: $f..."
            Invoke-WebRequest -Uri "$baseUrl/$f" -OutFile $dest -UseBasicParsing
        }
    }
    if (-not (Test-SttModelComplete $SttDir)) {
        throw "Failed to verify all Parakeet STT model files in $SttDir."
    }
    Write-Host "Parakeet TDT 0.6B v3 STT model installed to $SttDir."
}

exit 0
