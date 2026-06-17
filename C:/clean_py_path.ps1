foreach ($scope in @("User", "Machine")) {
    $p = [Environment]::GetEnvironmentVariable("Path", $scope)
    if ($p) {
        $cleaned = ($p.Split(';') | Where-Object {
            $_ -notlike '*Python*' -and
            $_ -notlike '*Programs\Python*' -and
            $_ -notlike '*AppData\Roaming\Python*'
        }) -join ';'
        [Environment]::SetEnvironmentVariable("Path", $cleaned, $scope)
        Write-Host "Cleaned $scope PATH: was $($p.Length) chars, now $($cleaned.Length) chars"
    }
}
[Environment]::SetEnvironmentVariable("PYTHONPATH", $null, "User")
[Environment]::SetEnvironmentVariable("PYTHONUSERBASE", $null, "User")
[Environment]::SetEnvironmentVariable("PYTHONNOUSERSITE", $null, "User")
Write-Host "Python env vars wiped."

$env:Path = [Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [Environment]::GetEnvironmentVariable("Path", "User")
Write-Host ""
Write-Host "Python entries in PATH now:"
$env:Path.Split(';') | Where-Object { $_ -like '*Python*' } | ForEach-Object { Write-Host "  $_" }
Write-Host "(if empty, no Python in PATH)"
