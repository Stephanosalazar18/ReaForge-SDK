#requires -RunAsAdministrator
# Cleanup duplicate Python installs + reinstall clean 3.13 with meson + ninja.

$ErrorActionPreference = "Stop"

Write-Host "=== 1. Discovering Python installs ===" -ForegroundColor Cyan
$pythonHKLM = Get-ItemProperty HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* `
  -ErrorAction SilentlyContinue | Where-Object { $_.DisplayName -match "^Python 3\.(8|11|12)\." }
$pythonHKCU = Get-ItemProperty HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* `
  -ErrorAction SilentlyContinue | Where-Object { $_.DisplayName -match "^Python 3\.(8|11|12)\." }

# Get the top-level uninstaller (not the subcomponents)
$topLevelHKLM = $pythonHKLM | Where-Object { $_.DisplayName -match " \(64-bit\)$" }
$topLevelHKCU = $pythonHKCU | Where-Object { $_.DisplayName -match " \(64-bit\)$" }

Write-Host "Found these old Pythons to remove:"
$topLevelHKLM | ForEach-Object { Write-Host "  HKLM: $($_.DisplayName) @ $($_.InstallLocation)" }
$topLevelHKCU | ForEach-Object { Write-Host "  HKCU: $($_.DisplayName) @ $($_.InstallLocation)" }

Write-Host ""
Write-Host "=== 2. Uninstalling old Pythons (quiet) ===" -ForegroundColor Cyan
foreach ($p in @($topLevelHKLM + $topLevelHKCU)) {
    if ($p.UninstallString) {
        Write-Host "  Uninstalling $($p.DisplayName)..."
        $uninst = $p.UninstallString -replace '"', ''
        $proc = Start-Process -FilePath $uninst -ArgumentList "/quiet" -PassThru -Wait -WindowStyle Hidden
        Write-Host "    exit code: $($proc.ExitCode)"
    }
}

Write-Host ""
Write-Host "=== 3. Cleaning PATH of Python leftovers ===" -ForegroundColor Cyan
$currentPath = [Environment]::GetEnvironmentVariable("Path", "User")
$cleaned = ($currentPath -split ';' | Where-Object { $_ -notmatch 'Python|Python313|Python312|Python311|Python38' }) -join ';'
[Environment]::SetEnvironmentVariable("Path", $cleaned, "User")
$env:Path = $cleaned
Write-Host "  PATH cleaned."

Write-Host ""
Write-Host "=== 4. Removing residual folders ===" -ForegroundColor Cyan
$folders = @(
    "C:\Python38",
    "C:\Python311",
    "C:\Python312",
    "C:\Python313",
    "$env:APPDATA\Python",
    "$env:LOCALAPPDATA\Programs\Python"
)
foreach ($f in $folders) {
    if (Test-Path $f) {
        Write-Host "  Removing $f..."
        Remove-Item -Recurse -Force $f -ErrorAction SilentlyContinue
    }
}

Write-Host ""
Write-Host "=== 5. Downloading fresh Python 3.13 ===" -ForegroundColor Cyan
$installer = "$env:TEMP\python-3.13.5-installer.exe"
if (-not (Test-Path $installer)) {
    Invoke-WebRequest -Uri "https://www.python.org/ftp/python/3.13.5/python-3.13.5-amd64.exe" -OutFile $installer
    Write-Host "  Downloaded."
} else {
    Write-Host "  Already cached at $installer"
}

Write-Host ""
Write-Host "=== 6. Installing Python 3.13 (all users, with PATH) ===" -ForegroundColor Cyan
$proc = Start-Process -FilePath $installer -ArgumentList `
    "/quiet", "InstallAllUsers=1", "PrependPath=1", "Include_test=0", "Include_doc=0", "Include_launcher=1" `
    -Wait -PassThru
Write-Host "  exit code: $($proc.ExitCode)"

Write-Host ""
Write-Host "=== 7. Verifying clean Python ===" -ForegroundColor Cyan
$env:Path = [Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [Environment]::GetEnvironmentVariable("Path", "User")
$py = (Get-Command py.exe -ErrorAction SilentlyContinue).Source
$python = (Get-Command python.exe -ErrorAction SilentlyContinue).Source
Write-Host "  py.exe:   $py"
Write-Host "  python.exe: $python"
& python --version
& py --version

Write-Host ""
Write-Host "=== 8. Installing meson + ninja ===" -ForegroundColor Cyan
& python -m pip install --upgrade meson ninja
& python -m meson --version
& python -m ninja --version

Write-Host ""
Write-Host "=== DONE ===" -ForegroundColor Green
Write-Host "Now close PowerShell, open 'x64 Native Tools Command Prompt for VS 2022', and run:"
Write-Host "  cd C:\Users\stephano\Dev\audio\ReaForge-SDK"
Write-Host "  meson setup build"
Write-Host "  ninja -C build"
