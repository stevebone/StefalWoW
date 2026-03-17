# Enterprise-Grade ClassAI Header Cleanup Script
# Removes all old dual-support specialization system members and methods from *AI.h headers

$ErrorActionPreference = "Stop"

$classaiPath = "c:\TrinityBots\TrinityCore\src\modules\Playerbot\AI\ClassAI"

# Find all *AI.h headers
$headers = Get-ChildItem -Path $classaiPath -Recurse -Filter "*AI.h" | Where-Object {
    $_.Name -match "^(DeathKnight|DemonHunter|Druid|Evoker|Hunter|Mage|Monk|Paladin|Priest|Rogue|Shaman|Warlock|Warrior)AI\.h$"
}

$totalFixed = 0
$fixedFiles = @()

foreach ($file in $headers) {
    Write-Host "Processing $($file.Name)..." -ForegroundColor Cyan

    $content = Get-Content $file.FullName -Raw
    $originalContent = $content
    $changes = 0

    # Pattern 1: Remove old specialization unique_ptr member
    # Example: std::unique_ptr<WarlockSpecialization> _specialization;
    $pattern1 = "(?m)^\s+std::unique_ptr<\w+Specialization>\s+_specialization;\s*[\r\n]+"
    if ($content -match $pattern1) {
        $content = $content -replace $pattern1, ""
        $changes++
        Write-Host "  - Removed _specialization unique_ptr" -ForegroundColor Green
    }

    # Pattern 2: Remove spec tracking variable (_currentSpec or _detectedSpec)
    # Example: WarlockSpec _currentSpec;
    # Example: DeathKnightSpec _detectedSpec;
    $pattern2 = "(?m)^\s+\w+Spec\s+_(currentSpec|detectedSpec);\s*[\r\n]+"
    if ($content -match $pattern2) {
        $content = $content -replace $pattern2, ""
        $changes++
        Write-Host "  - Removed spec tracking variable" -ForegroundColor Green
    }

    # Pattern 3: Remove InitializeSpecialization method declaration
    $pattern3 = "(?m)^\s+(void|WarlockSpec|DeathKnightSpec|DruidSpec|MageSpec|MonkSpec|PaladinSpec|PriestSpec|ShamanSpec|WarriorSpec|HunterSpec|DemonHunterSpec|RogueSpec|EvokerSpec)\s+InitializeSpecialization\(\);\s*[\r\n]+"
    if ($content -match $pattern3) {
        $content = $content -replace $pattern3, ""
        $changes++
        Write-Host "  - Removed InitializeSpecialization()" -ForegroundColor Green
    }

    # Pattern 4: Remove DetectSpecialization/DetectCurrentSpecialization method declaration
    $pattern4 = "(?m)^\s+\w+Spec\s+Detect(Current)?Specialization\(\)( const)?;\s*[\r\n]+"
    if ($content -match $pattern4) {
        $content = $content -replace $pattern4, ""
        $changes++
        Write-Host "  - Removed Detect*Specialization()" -ForegroundColor Green
    }

    # Pattern 5: Remove SwitchSpecialization method declaration
    $pattern5 = "(?m)^\s+void\s+SwitchSpecialization\(\w+Spec\s+\w+\);\s*[\r\n]+"
    if ($content -match $pattern5) {
        $content = $content -replace $pattern5, ""
        $changes++
        Write-Host "  - Removed SwitchSpecialization()" -ForegroundColor Green
    }

    # Pattern 6: Remove entire "Specialization System" comment blocks
    $pattern6 = "(?ms)^\s+// ={20,}[\r\n]+\s+// Specialization System[\r\n]+\s+// ={20,}[\r\n]+(.*?)(?=\s+// ={20,}|$)"
    if ($content -match $pattern6) {
        # Only remove if it contains old system markers
        $blockContent = $matches[1]
        if ($blockContent -match "(_specialization|_currentSpec|_detectedSpec|InitializeSpecialization|DetectSpecialization|SwitchSpecialization)") {
            $content = $content -replace $pattern6, ""
            $changes++
            Write-Host "  - Removed Specialization System comment block" -ForegroundColor Green
        }
    }

    if ($changes -gt 0) {
        Set-Content -Path $file.FullName -Value $content -NoNewline
        $totalFixed++
        $fixedFiles += $file.Name
        Write-Host "  [OK] Fixed $($file.Name) - $changes changes" -ForegroundColor Yellow
    } else {
        Write-Host "  [OK] No old system code found in $($file.Name)" -ForegroundColor Gray
    }
}

Write-Host "`n========================================" -ForegroundColor Magenta
Write-Host "CLEANUP SUMMARY" -ForegroundColor Magenta
Write-Host "========================================" -ForegroundColor Magenta
Write-Host "Files Processed: $($headers.Count)" -ForegroundColor White
Write-Host "Files Modified: $totalFixed" -ForegroundColor Yellow
Write-Host "`nModified Files:" -ForegroundColor Cyan
$fixedFiles | ForEach-Object { Write-Host "  - $_" -ForegroundColor Green }
Write-Host "`n[SUCCESS] Enterprise-grade header cleanup complete!" -ForegroundColor Green
