# Builds client.dll + injector.exe and packs them into installer\Output\PotatoClient-Setup.exe
#
#   powershell -ExecutionPolicy Bypass -File installer\build-installer.ps1
#   powershell -ExecutionPolicy Bypass -File installer\build-installer.ps1 -SkipBuild   # reuse build\Release
#
# Needs: Visual Studio 2019+ with "Desktop development with C++", and Inno Setup 6
#   (winget install JRSoftware.InnoSetup)
param([switch]$SkipBuild)
# Not "Stop": PowerShell 5.1 turns cmake's stderr warnings into terminating errors.
# Native failures are checked through $LASTEXITCODE instead.
$ErrorActionPreference = "Continue"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

function Find-Exe($name, [string[]]$candidates) {
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    foreach ($c in $candidates) { if ($c -and (Test-Path $c)) { return $c } }
    return $null
}

if (-not $SkipBuild) {
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        # Not inside a Developer PowerShell: enter the newest Visual Studio's dev environment
        $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        if (-not (Test-Path $vswhere)) { throw "Visual Studio が見つかりません (「C++ によるデスクトップ開発」を入れてください)" }
        $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if (-not $vs) { throw "C++ ツール付きの Visual Studio が見つかりません" }
        Import-Module "$vs\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
        Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64" | Out-Null
    }
    cmake -S . -B build -A x64
    if ($LASTEXITCODE) { throw "cmake configure に失敗しました" }
    cmake --build build --config Release
    if ($LASTEXITCODE) { throw "cmake build に失敗しました" }
}

foreach ($f in "build\Release\client.dll", "build\Release\injector.exe") {
    if (-not (Test-Path $f)) { throw "$f がありません。先にビルドしてください" }
}

$iscc = Find-Exe "iscc" @(
    "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
    "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
    "$env:ProgramFiles\Inno Setup 6\ISCC.exe")
if (-not $iscc) { throw "Inno Setup 6 が見つかりません: winget install JRSoftware.InnoSetup" }

& $iscc "installer\PotatoClient.iss"
if ($LASTEXITCODE) { throw "Inno Setup のコンパイルに失敗しました" }
Write-Host "`n完成: $root\installer\Output\PotatoClient-Setup.exe" -ForegroundColor Green
