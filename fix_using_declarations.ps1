# PowerShell script to fix using declarations in all refactored class files
# This fixes the compilation error where template parameters cannot be used in using declarations

$ErrorActionPreference = "Stop"

# Define the patterns to fix
$patterns = @{
    "RangedDpsSpecialization" = @{
        "search" = "using RangedDpsSpecialization<(.+?)>::"
        "replacement" = 'using Base = RangedDpsSpecialization<$1>;${newline}    using Base::'
        "baseTypeAdded" = $false
    }
    "MeleeDpsSpecialization" = @{
        "search" = "using MeleeDpsSpecialization<(.+?)>::"
        "replacement" = 'using Base = MeleeDpsSpecialization<$1>;${newline}    using Base::'
        "baseTypeAdded" = $false
    }
    "HealerSpecialization" = @{
        "search" = "using HealerSpecialization<(.+?)>::"
        "replacement" = 'using Base = HealerSpecialization<$1>;${newline}    using Base::'
        "baseTypeAdded" = $false
    }
    "TankSpecialization" = @{
        "search" = "using TankSpecialization<(.+?)>::"
        "replacement" = 'using Base = TankSpecialization<$1>;${newline}    using Base::'
        "baseTypeAdded" = $false
    }
}

# Get all refactored header files
$refactoredFiles = Get-ChildItem -Path "src\modules\Playerbot\AI\ClassAI" -Filter "*Refactored.h" -Recurse

Write-Host "Found $($refactoredFiles.Count) refactored files to process" -ForegroundColor Green

$filesModified = 0

foreach ($file in $refactoredFiles) {
    Write-Host "Processing: $($file.FullName)" -ForegroundColor Cyan

    $content = Get-Content $file.FullName -Raw
    $originalContent = $content
    $modified = $false

    foreach ($patternName in $patterns.Keys) {
        $pattern = $patterns[$patternName]

        # Check if file contains this pattern
        if ($content -match $pattern.search) {
            Write-Host "  Found $patternName pattern" -ForegroundColor Yellow

            # Find the first using declaration
            if ($content -match "(?s)public:\s*\n\s*(using $patternName<[^>]+>::)") {
                $firstUsing = $matches[1]

                # Extract the template parameter
                if ($firstUsing -match "using $patternName<([^>]+)>::") {
                    $templateParam = $matches[1]

                    # Replace the first using with the Base typedef
                    $newLine = "    // Use base class members with type alias for cleaner syntax`n    using Base = $patternName<$templateParam>;"
                    $content = $content -replace [regex]::Escape($firstUsing), "using Base::$firstUsing"
                    $content = $content -replace "(public:\s*\n\s*)using Base::", "`$1$newLine`n    using Base::"

                    # Replace all other using declarations with Base::
                    $content = $content -replace "using $patternName<[^>]+>::", "using Base::"

                    $modified = $true
                }
            }
        }
    }

    # Only write if content changed
    if ($modified -and ($content -ne $originalContent)) {
        Set-Content -Path $file.FullName -Value $content -NoNewline
        Write-Host "  Modified!" -ForegroundColor Green
        $filesModified++
    } else {
        Write-Host "  No changes needed" -ForegroundColor Gray
    }
}

Write-Host "`nTotal files modified: $filesModified" -ForegroundColor Green
Write-Host "Done!" -ForegroundColor Green
