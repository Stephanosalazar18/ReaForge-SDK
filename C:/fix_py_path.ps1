$pythonPath = "$env:LOCALAPPDATA\Programs\Python\Python314"
$scriptsPath = "$env:LOCALAPPDATA\Programs\Python\Python314\Scripts"
$currentPath = [Environment]::GetEnvironmentVariable("Path", "User")
$newPath = "$pythonPath;$scriptsPath;$currentPath"
[Environment]::SetEnvironmentVariable("Path", $newPath, "User")
$env:Path = $newPath
Write-Host "Added Python 3.14 to User PATH"
Write-Host "Python: $pythonPath"
Write-Host "Scripts: $scriptsPath"
