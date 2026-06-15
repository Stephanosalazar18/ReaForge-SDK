Write-Host "=== Search for Python 3.14 anywhere ===" -ForegroundColor Cyan
$locations = @(
    "$env:LOCALAPPDATA\Programs\Python\Python314",
    "C:\Program Files\Python314",
    "C:\Python314",
    "C:\Program Files (x86)\Python314"
)
foreach ($loc in $locations) {
    if (Test-Path $loc) {
        Write-Host "FOUND: $loc"
        $ver = & "$loc\python.exe" --version 2>&1
        Write-Host "  Version: $ver"
    } else {
        Write-Host "Not found: $loc"
    }
}

Write-Host ""
Write-Host "=== Where does 'python' alias point? ===" -ForegroundColor Cyan
$wherePy = (Get-Command python.exe -ErrorAction SilentlyContinue).Source
$whereLauncher = (Get-Command py.exe -ErrorAction SilentlyContinue).Source
Write-Host "python.exe alias: $wherePy"
Write-Host "py.exe launcher:  $whereLauncher"

Write-Host ""
Write-Host "=== Where Python 3.14 is registered (registry) ===" -ForegroundColor Cyan
Get-ItemProperty HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* `
    -ErrorAction SilentlyContinue | Where-Object { $_.DisplayName -like "*Python 3.14*" } | ForEach-Object {
    Write-Host "DisplayName: $($_.DisplayName)"
    Write-Host "InstallLocation: $($_.InstallLocation)"
    Write-Host "UninstallString: $($_.UninstallString)"
}
Get-ItemProperty HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* `
    -ErrorAction SilentlyContinue | Where-Object { $_.DisplayName -like "*Python 3.14*" } | ForEach-Object {
    Write-Host "DisplayName: $($_.DisplayName)"
    Write-Host "InstallLocation: $($_.InstallLocation)"
    Write-Host "UninstallString: $($_.UninstallString)"
}
