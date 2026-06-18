$dirs = @(
    "$env:LOCALAPPDATA\Programs\Python\Python38",
    "$env:LOCALAPPDATA\Programs\Python\Python311"
)
foreach ($d in $dirs) {
    if (Test-Path $d) {
        Write-Host "Removing $d..."
        try {
            Remove-Item -Recurse -Force $d -ErrorAction Stop
            Write-Host "  OK"
        } catch {
            Write-Host "  Normal removal failed: $($_.Exception.Message)"
            Write-Host "  Trying with cmd rmdir..."
            $rc = cmd /c "rmdir /S /Q `"$d`"" 2>&1
            Write-Host "  cmd output: $rc"
            if (Test-Path $d) {
                Write-Host "  Still there. Trying takeown + rmdir..."
                $ownerRc = cmd /c "takeown /F `"$d`" /R /D Y" 2>&1
                Write-Host "  takeown: $ownerRc"
                $rc = cmd /c "rmdir /S /Q `"$d`"" 2>&1
                Write-Host "  rmdir: $rc"
            }
            if (Test-Path $d) {
                Write-Host "  STILL there. Manual deletion needed."
            } else {
                Write-Host "  Removed."
            }
        }
    } else {
        Write-Host "$d not present."
    }
}
