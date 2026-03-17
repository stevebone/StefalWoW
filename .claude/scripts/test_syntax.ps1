# Minimal test script
$ErrorActionPreference = "Stop"

function Write-Info { Write-Host $args -ForegroundColor Cyan }

Write-Info "Testing nested if-else..."

if (Test-Path "test.txt") {
    $response = "y"
    if ($response -eq "y") {
        Write-Info "  ✅ Inner if: yes"
    } else {
        Write-Info "  ℹ️ Inner if: no"
    }
} else {
    Write-Info "  ℹ️ File does not exist"
}

Write-Info "Test complete"
