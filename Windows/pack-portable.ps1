# pack-portable.ps1 - Create office激活工具 portable zip from Windows\output\
param(
    [string]$OutputDir = "output",
    [string]$Version = "1.0.0"
)

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

if (-not (Test-Path $OutputDir)) {
    Write-Host "ERROR: Build output not found at $OutputDir." -ForegroundColor Red
    exit 1
}

$exeName = "office激活工具.exe"
$required = @($exeName, "ProxyBridgeCore.dll", "WinDivert.dll", "WinDivert64.sys")
foreach ($f in $required) {
    if (-not (Test-Path (Join-Path $OutputDir $f))) {
        Write-Host "ERROR: Missing $f in $OutputDir" -ForegroundColor Red
        exit 1
    }
}

$portableName = "office激活工具-Portable-$Version"
$staging = Join-Path $root "portable-staging"
$zipPath = Join-Path $OutputDir "$portableName.zip"

if (Test-Path $staging) { Remove-Item $staging -Recurse -Force }
New-Item -ItemType Directory -Path $staging -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $staging "data") -Force | Out-Null

foreach ($f in $required) {
    Copy-Item (Join-Path $OutputDir $f) -Destination $staging -Force
}

"" | Out-File -FilePath (Join-Path $staging "portable.flag") -Encoding ascii -NoNewline

@(
    "office激活工具 便携版"
    "===================="
    ""
    "1. 解压到任意目录。"
    "2. 右键 office激活工具.exe，以管理员身份运行。"
    "3. 按提示输入软件激活码后使用。"
    ""
    "配置文件保存在程序目录下的 data 文件夹。"
) | Out-File -FilePath (Join-Path $staging "README.txt") -Encoding utf8

if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Compress-Archive -Path (Join-Path $staging "*") -DestinationPath $zipPath -Force
Remove-Item $staging -Recurse -Force

$size = [math]::Round((Get-Item $zipPath).Length / 1MB, 2)
Write-Host "Portable package ready: $zipPath ($size MB)" -ForegroundColor Green
