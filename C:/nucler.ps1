#requires -RunAsAdministrator
$ErrorActionPreference = "Stop"

Write-Host "=== STEP 1: Force-uninstall every Python from the registry ===" -ForegroundColor Cyan
$allEntries = @()
$allEntries += Get-ItemProperty HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* -ErrorAction SilentlyContinue | Where-Object { $_.DisplayName -like "*Python*" -or $_.DisplayName -like "*Pip*" }
$allEntries += Get-ItemProperty HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* -ErrorAction SilentlyContinue | Where-Object { $_.DisplayName -like "*Python*" -or $_.DisplayName -like "*Pip*" }

$topLevel = $allEntries | Where-Object { $_.DisplayName -match "^Python 3\.\d+\.\d+ \(\d+-bit\)$" }
Write-Host "Top-level Pythons found:"
$topLevel | ForEach-Object { Write-Host "  $($_.DisplayName) -> $($_.UninstallString)" }

foreach ($p in $topLevel) {
    if ($p.UninstallString) {
        Write-Host "  Uninstalling $($_.DisplayName)..."
        $uninst = $p.UninstallString -replace '"',''
        try {
            $proc = Start-Process -FilePath $uninst -ArgumentList "/quiet" -PassThru -Wait -WindowStyle Hidden -ErrorAction SilentlyContinue
            Write-Host "    exit code: $($proc.ExitCode)"
        } catch {
            Write-Host "    error: $_"
        }
    }
}

Write-Host ""
Write-Host "=== STEP 2: Brutally delete every Python folder ===" -ForegroundColor Cyan
$folders = @(
    "C:\Python38",
    "C:\Python311",
    "C:\Python312",
    "C:\Python313",
    "C:\Program Files\Python313",
    "C:\Program Files (x86)\Python313",
    "$env:APPDATA\Python",
    "$env:LOCALAPPDATA\Programs\Python",
    "C:\Users\stephano\AppData\Roaming\Python",
    "C:\Users\stephano\AppData\Local\Programs\Python"
)
foreach ($f in $folders) {
    if (Test-Path $f) {
        Write-Host "  Removing $f..."
        Remove-Item -Recurse -Force $f -ErrorAction SilentlyContinue
    }
}

Write-Host ""
Write-Host "=== STEP 3: Wipe Python-related entries from PATH ===" -ForegroundColor Cyan
foreach ($scope in @("User", "Machine")) {
    $p = [Environment]::GetEnvironmentVariable("Path", $scope)
    if ($p) {
        $cleaned = ($p -split ';' | Where-Object {
            $_ -notlike '*Python*' -and `
            $_ -notlike '*Programs\Python*' -and `
            $_ -notlike '*AppData\Roaming\Python*' -and `
            $_ -notlike '*PySpark*' -and `
            $_ -notlike '*py4j*'
        }) -join ';'
        [Environment]::SetEnvironmentVariable("Path", $cleaned, $scope)
        Write-Host "  Cleaned $scope PATH"
    }
}
$env:Path = [Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [Environment]::GetEnvironmentVariable("Path", "User")

Write-Host ""
Write-Host "=== STEP 4: Wipe PYTHONPATH and PYTHONUSERBASE ===" -ForegroundColor Cyan
[Environment]::SetEnvironmentVariable("PYTHONPATH", $null, "User")
[Environment]::SetEnvironmentVariable("PYTHONUSERBASE", $null, "User")
[Environment]::SetEnvironmentVariable("PYTHONNOUSERSITE", $null, "User")
$env:PYTHONPATH = $null

Write-Host ""
Write-Host "=== STEP 5: Verify Python is GONE ===" -ForegroundColor Cyan
$pyCount = (Get-Command python.exe -ErrorAction SilentlyContinue | Measure-Object).Count
$pyLauncherCount = (Get-Command py.exe -ErrorAction SilentlyContinue | Measure-Object).Count
Write-Host "  python.exe: $pyCount locations"
Write-Host "  py.exe: $pyLauncherCount locations"
$env:Path -split ';' | Where-Object { $_ -like '*Python*' -or $_ -like '*PySpark*' } | ForEach-Object { Write-Host "  PATH residue: $_" }

Write-Host ""
Write-Host "=== STEP 6: Download and install fresh Python 3.13 ===" -ForegroundColor Cyan
$installer = "$env:TEMP\python-3.13.5-fresh.exe"
if (-not (Test-Path $installer) -or (Get-Item $installer).Length -lt 1000000) {
    Write-Host "  Downloading..."
    Invoke-WebRequest -Uri "https://www.python.org/ftp/python/3.13.5/python-3.13.5-amd64.exe" -OutFile $installer
}
Write-Host "  Installing (this takes 1-2 min)..."
$proc = Start-Process -FilePath $installer -ArgumentList "/quiet", "InstallAllUsers=1", "PrependPath=1", "Include_test=0", "Include_doc=0", "Include_launcher=1" -Wait -PassThru
Write-Host "  exit code: $($proc.ExitCode)"

# Refresh PATH
$env:Path = [Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [Environment]::GetEnvironmentVariable("Path", "User")

Write-Host ""
Write-Host "=== STEP 7: Verify fresh Python ===" -ForegroundColor Cyan
$python = (Get-Command python.exe -ErrorAction SilentlyContinue).Source
$py = (Get-Command py.exe -ErrorAction SilentlyContinue).Source
Write-Host "  python.exe: $python"
Write-Host "  py.exe:     $py"
& python --version
& py --version
& python -c "import sys; print('prefix:', sys.prefix); print('site-packages check:'); import os; sp = os.path.join(sys.prefix, 'Lib', 'site-packages'); print('  exists:', os.path.exists(sp), '->', sp)"

Write-Host ""
Write-Host "=== STEP 8: Install meson + ninja into system site-packages ===" -ForegroundColor Cyan
& python -m pip install --upgrade meson ninja
& python -m meson --version
& python -m ninja --version

Write-Host ""
Write-Host "=== DONE ===" -ForegroundColor Green
Write-Host "Close this PowerShell. Open 'x64 Native Tools Command Prompt for VS 2022'. Then:"
Write-Host "  cd C:\Users\stephano\Dev\audio\ReaForge-SDK"
Write-Host "  meson setup build"
Write-Host "  ninja -C build"
