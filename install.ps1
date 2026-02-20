# install.ps1 - Install x64dbg MCP plugin DLLs to your x64dbg installation
# This script is local-only (.gitignore'd) - edit X64DBG_PATH to match your setup.
#
# Usage:
#   .\install.ps1            - Install both x64 and x32 plugins
#   .\install.ps1 -Arch x64  - Install x64 only
#   .\install.ps1 -Arch x32  - Install x32 only

param(
    [ValidateSet('x64', 'x32', 'both')]
    [string]$Arch = 'both'
)

# ── CONFIGURE THIS ────────────────────────────────────────────────────────────
$X64DBG_PATH = "E:\reverse_tools\x64dbg_2025-08-19_19-40\release"
# ─────────────────────────────────────────────────────────────────────────────

$ScriptDir   = Split-Path -Parent $MyInvocation.MyCommand.Path
$Dp64        = Join-Path $ScriptDir "plugin\build\x64-release\bin\x64dbg_mcp.dp64"
$Dp32        = Join-Path $ScriptDir "plugin\build\x32-release\bin\x64dbg_mcp.dp32"
$PluginsX64  = Join-Path $X64DBG_PATH "x64\plugins"
$PluginsX32  = Join-Path $X64DBG_PATH "x32\plugins"

function Install-Plugin {
    param([string]$Src, [string]$DestDir, [string]$Label)

    if (-not (Test-Path $Src)) {
        Write-Host "[$Label] SKIP - not built yet: $Src" -ForegroundColor Yellow
        Write-Host "         Run: cmake --build plugin/build/$($Label.ToLower())-release" -ForegroundColor Yellow
        return
    }
    if (-not (Test-Path $DestDir)) {
        Write-Host "[$Label] ERROR - plugins directory not found: $DestDir" -ForegroundColor Red
        Write-Host "         Edit X64DBG_PATH in install.ps1" -ForegroundColor Red
        return
    }

    $Dest = Join-Path $DestDir (Split-Path -Leaf $Src)
    try {
        Copy-Item -Path $Src -Destination $Dest -Force
        $size = [math]::Round((Get-Item $Dest).Length / 1KB)
        Write-Host "[$Label] Installed ${size}KB -> $Dest" -ForegroundColor Green
    } catch {
        Write-Host "[$Label] FAILED: $_" -ForegroundColor Red
        Write-Host "         Is x64dbg currently running with the DLL locked?" -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "x64dbg MCP Plugin Installer" -ForegroundColor Cyan
Write-Host "x64dbg path: $X64DBG_PATH" -ForegroundColor DarkGray
Write-Host ""

if ($Arch -eq 'x64' -or $Arch -eq 'both') {
    Install-Plugin -Src $Dp64 -DestDir $PluginsX64 -Label "x64"
}
if ($Arch -eq 'x32' -or $Arch -eq 'both') {
    Install-Plugin -Src $Dp32 -DestDir $PluginsX32 -Label "x32"
}

Write-Host ""
Write-Host "Done. Reload x64dbg plugins or restart x64dbg." -ForegroundColor Cyan
