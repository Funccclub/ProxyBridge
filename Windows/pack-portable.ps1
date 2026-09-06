# pack-portable.ps1 - Create Doggie portable zip from Windows\output\
param(
    [string]$OutputDir = "output",
    [string]$Version = "1.0.0"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

if (-not (Test-Path $OutputDir)) {
    Write-Host "ERROR: Build output not found at $OutputDir. Run compile.ps1 -NoSign first." -ForegroundColor Red
    exit 1
}

$required = @("Doggie.exe", "ProxyBridgeCore.dll", "WinDivert.dll", "WinDivert64.sys")
foreach ($f in $required) {
    if (-not (Test-Path (Join-Path $OutputDir $f))) {
        Write-Host "ERROR: Missing $f in $OutputDir" -ForegroundColor Red
        exit 1
    }
}

$portableName = "Doggie-Portable-$Version"
$staging = Join-Path $root "portable-staging"
$zipPath = Join-Path $OutputDir "$portableName.zip"

if (Test-Path $staging) { Remove-Item $staging -Recurse -Force }
New-Item -ItemType Directory -Path $staging -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $staging "data") -Force | Out-Null

foreach ($f in $required) {
    Copy-Item (Join-Path $OutputDir $f) -Destination $staging -Force
}

# Optional CLI
$cli = Join-Path $OutputDir "ProxyBridge_CLI.exe"
if (Test-Path $cli) { Copy-Item $cli -Destination $staging -Force }

"" | Out-File -FilePath (Join-Path $staging "portable.flag") -Encoding ascii -NoNewline

@(
    "Doggie Portable Edition"
    "========================"
    ""
    "1. Extract this folder anywhere."
    "2. Run Doggie.exe as Administrator."
    "3. First launch: Sign Up to set your password."
    "4. Later launches: Sign In with your password."
    ""
    "Settings and profiles are stored in the data\ folder next to the exe."
    "You can copy the whole folder to a USB drive or another PC."
    ""
    "Requires: Windows 10+ 64-bit, Administrator privileges."
) | Out-File -FilePath (Join-Path $staging "README.txt") -Encoding utf8

if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Compress-Archive -Path (Join-Path $staging "*") -DestinationPath $zipPath -Force
Remove-Item $staging -Recurse -Force

$size = [math]::Round((Get-Item $zipPath).Length / 1MB, 2)
Write-Host "Portable package ready: $zipPath ($size MB)" -ForegroundColor Green
