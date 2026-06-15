$exe = $args[0]
$secs = [int]$args[1]
$proc = Start-Process -FilePath $exe -PassThru -NoNewWindow
$timeout = (Get-Date).AddSeconds($secs)
while (-not $proc.HasExited -and (Get-Date) -lt $timeout) {
    Start-Sleep -Milliseconds 100
}
if (-not $proc.HasExited) {
    Write-Host "TIMEOUT after $secs sec, killing PID $($proc.Id)"
    Stop-Process -Id $proc.Id -Force
    exit 124
} else {
    exit $proc.ExitCode
}
