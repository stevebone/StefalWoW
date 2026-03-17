# PowerShell Script to Fix Namespace Ambiguity in Playerbot Module
# This script adds explicit :: namespace qualifiers to TrinityCore types

Write-Host "==================================================================" -ForegroundColor Cyan
Write-Host "Playerbot Namespace Ambiguity Fix Script" -ForegroundColor Cyan
Write-Host "==================================================================" -ForegroundColor Cyan
Write-Host ""

$FilesToFix = @(
    "src\modules\Playerbot\Interaction\Core\InteractionValidator.h",
    "src\modules\Playerbot\Interaction\Core\InteractionValidator.cpp"
)

$Replacements = @{
    # Match Player* but not ::Player*
    '(?<!:)(?<!::)Player\*' = '::Player*'
    '(?<!:)(?<!::)Creature\*' = '::Creature*'
    '(?<!:)(?<!::)WorldObject\*' = '::WorldObject*'
    '(?<!:)(?<!::)Item\*' = '::Item*'
    '(?<!:)(?<!::)GameObject\*' = '::GameObject*'
    '(?<!:)(?<!::)SpellInfo const\*' = '::SpellInfo const*'
    '(?<!:)(?<!::)SpellInfo\*' = '::SpellInfo*'
    '(?<!:)(?<!::)WorldPacket const&' = '::WorldPacket const&'
}

foreach ($file in $FilesToFix) {
    $fullPath = Join-Path $PSScriptRoot $file

    if (!(Test-Path $fullPath)) {
        Write-Host "WARNING: File not found: $fullPath" -ForegroundColor Yellow
        continue
    }

    Write-Host "Processing: $file" -ForegroundColor Green

    # Read file content
    $content = Get-Content $fullPath -Raw -Encoding UTF8
    $originalContent = $content

    # Apply each replacement
    foreach ($pattern in $Replacements.Keys) {
        $replacement = $Replacements[$pattern]
        $before = ($content -split "`n").Count
        $content = $content -replace $pattern, $replacement
        $after = ($content -split "`n").Count

        if ($before -ne $after) {
            Write-Host "  ERROR: Line count changed during replacement!" -ForegroundColor Red
            Write-Host "  Pattern: $pattern" -ForegroundColor Red
            Write-Host "  Before: $before lines, After: $after lines" -ForegroundColor Red
            $content = $originalContent
            break
        }
    }

    # Count changes
    $changes = 0
    if ($content -ne $originalContent) {
        # Simple change detection by comparing lengths
        $changes = ([regex]::Matches($content, '::')).Count - ([regex]::Matches($originalContent, '::')).Count

        # Create backup
        $backupPath = "$fullPath.backup"
        Copy-Item $fullPath $backupPath -Force
        Write-Host "  Created backup: $backupPath" -ForegroundColor Cyan

        # Write fixed content
        $content | Set-Content $fullPath -Encoding UTF8 -NoNewline
        Write-Host "  Applied $changes namespace qualifications" -ForegroundColor Green
    }
    else {
        Write-Host "  No changes needed" -ForegroundColor Gray
    }

    Write-Host ""
}

Write-Host "==================================================================" -ForegroundColor Cyan
Write-Host "Namespace Fix Complete!" -ForegroundColor Green
Write-Host "==================================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. Review changes in InteractionValidator.h and .cpp" -ForegroundColor White
Write-Host "2. Run: cmake --build build --config RelWithDebInfo" -ForegroundColor White
Write-Host "3. Check for any remaining compilation errors" -ForegroundColor White
Write-Host ""
Write-Host "Backup files created with .backup extension" -ForegroundColor Gray
Write-Host "Restore with: Copy-Item *.backup (original name)" -ForegroundColor Gray
