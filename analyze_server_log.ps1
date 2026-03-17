# PowerShell script to analyze Server.log for bot performance issues
# Usage: .\analyze_server_log.ps1 Server.log

param(
    [string]$LogFile = "Server.log"
)

Write-Host "=== BOT PERFORMANCE LOG ANALYZER ===" -ForegroundColor Cyan
Write-Host ""

# Extract TASK START/END pairs
Write-Host "Analyzing task execution times..." -ForegroundColor Yellow
$taskStarts = @{}
$hangingTasks = @()

Get-Content $LogFile -Tail 10000 | ForEach-Object {
    if ($_ -match "TASK START for bot (.+)") {
        $botGuid = $Matches[1]
        $taskStarts[$botGuid] = $_
    }
    elseif ($_ -match "TASK END for bot (.+)") {
        $botGuid = $Matches[1]
        $taskStarts.Remove($botGuid)
    }
}

# Report hanging tasks (started but never ended)
if ($taskStarts.Count -gt 0) {
    Write-Host "CRITICAL: $($taskStarts.Count) bots have hanging tasks!" -ForegroundColor Red
    $taskStarts.GetEnumerator() | ForEach-Object {
        Write-Host "  Bot: $($_.Key)" -ForegroundColor Red
        $hangingTasks += $_.Key
    }
    Write-Host ""
}

# Extract performance reports
Write-Host "Recent performance metrics:" -ForegroundColor Yellow
Get-Content $LogFile -Tail 10000 | Select-String "PERFORMANCE REPORT" -Context 0,5 | Select-Object -Last 3

# Extract ThreadPool saturation warnings
Write-Host "`nThreadPool saturation events:" -ForegroundColor Yellow
Get-Content $LogFile -Tail 10000 | Select-String "ThreadPool saturated" -Context 1,1 | Select-Object -Last 5

# Extract Future timeout warnings
Write-Host "`nFuture timeout events:" -ForegroundColor Yellow
Get-Content $LogFile -Tail 10000 | Select-String "Future.*not ready after" -Context 1,1 | Select-Object -Last 5

# Extract database query timing
Write-Host "`nDatabase query performance:" -ForegroundColor Yellow
Get-Content $LogFile -Tail 10000 | Select-String "slow query" -Context 0,2 | Select-Object -Last 5

# Summary
Write-Host "`n=== SUMMARY ===" -ForegroundColor Cyan
Write-Host "Hanging tasks: $($hangingTasks.Count)" -ForegroundColor $(if ($hangingTasks.Count -gt 0) { "Red" } else { "Green" })
Write-Host "`nRecommendations:" -ForegroundColor Yellow

if ($hangingTasks.Count -gt 0) {
    Write-Host "  1. Tasks are hanging in Update() - likely database or mutex deadlock" -ForegroundColor Red
    Write-Host "  2. Enable SQL logging: Logger.sql.sql=3,Console Server" -ForegroundColor Yellow
    Write-Host "  3. Run debug_bot_performance.sql to check for blocking queries" -ForegroundColor Yellow
}
