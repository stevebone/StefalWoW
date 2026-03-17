# Comprehensive Movement Call Audit for PlayerBot Module

$pattern = 'GetMotionMaster.*Move(Point|Chase|Follow|Path|Idle|Random|Confuse|Flee|Jump|Home|Charge|Land|TakeOff)'
$path = 'c:\TrinityBots\TrinityCore\src\modules\Playerbot'

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "  PlayerBot Movement Call Audit" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

$results = Get-ChildItem -Path $path -Include *.cpp,*.h -Recurse |
    Select-String -Pattern $pattern

$grouped = $results | Group-Object Path |
    Select-Object @{Name='File';Expression={Split-Path $_.Name -Leaf}}, Count |
    Sort-Object Count -Descending

$grouped | Format-Table -AutoSize

Write-Host "`nTotal files with calls: $($grouped.Count)" -ForegroundColor Yellow
Write-Host "Total calls found: $(($grouped | Measure-Object -Property Count -Sum).Sum)`n" -ForegroundColor Yellow

# Show details for top 10 files
Write-Host "Top 10 files by call count:" -ForegroundColor Cyan
$grouped | Select-Object -First 10 | ForEach-Object {
    $file = $_.File
    $filePath = Get-ChildItem -Path $path -Include $file -Recurse | Select-Object -First 1 -ExpandProperty FullName
    Write-Host "`n$file ($($_.Count) calls):" -ForegroundColor Green
    Select-String -Path $filePath -Pattern $pattern |
        Select-Object -First 5 LineNumber, Line |
        Format-Table -AutoSize
}
