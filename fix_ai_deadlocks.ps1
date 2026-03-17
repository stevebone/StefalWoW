# Fix "resource deadlock would occur" by converting all AI mutexes to recursive_mutex
# This allows the same thread to lock a mutex multiple times without deadlocking

$files = @(
    "src\modules\Playerbot\AI\ClassAI\Warlocks\WarlockAI.h",
    "src\modules\Playerbot\AI\ClassAI\Warlocks\AfflictionSpecialization.h",
    "src\modules\Playerbot\AI\ClassAI\Warlocks\DestructionSpecialization.h",
    "src\modules\Playerbot\AI\ClassAI\Warlocks\DemonologySpecialization.h",
    "src\modules\Playerbot\AI\ClassAI\Druids\BalanceSpecialization.h",
    "src\modules\Playerbot\AI\ClassAI\Druids\FeralDpsSpecialization.h",
    "src\modules\Playerbot\AI\ClassAI\Druids\FeralSpecialization.h",
    "src\modules\Playerbot\AI\ClassAI\Druids\RestorationSpecialization.h",
    "src\modules\Playerbot\AI\ClassAI\Druids\GuardianSpecialization.h",
    "src\modules\Playerbot\AI\ClassAI\Evokers\EvokerAI.h",
    "src\modules\Playerbot\AI\Actions\SpellInterruptAction.h",
    "src\modules\Playerbot\AI\ClassAI\ActionPriority.h",
    "src\modules\Playerbot\AI\Combat\InterruptAwareness.h",
    "src\modules\Playerbot\AI\Combat\ThreatCoordinator.h",
    "src\modules\Playerbot\AI\ClassAI\ResourceManager.h"
)

foreach ($file in $files) {
    $path = Join-Path $PSScriptRoot $file
    if (Test-Path $path) {
        Write-Host "Processing: $file"
        $content = Get-Content $path -Raw

        # Replace non-recursive mutexes with recursive ones
        $content = $content -replace '(\s+)mutable std::mutex\s+', '$1mutable std::recursive_mutex '
        $content = $content -replace '(\s+)std::mutex\s+_', '$1std::recursive_mutex _'
        $content = $content -replace 'static inline std::mutex\s+', 'static inline std::recursive_mutex '

        Set-Content $path -Value $content -NoNewline
        Write-Host "  ✓ Fixed mutexes in $file"
    } else {
        Write-Host "  ⚠ File not found: $file"
    }
}

Write-Host ""
Write-Host "✅ Completed! All AI mutexes converted to recursive_mutex"
Write-Host "   This fixes resource deadlock would occur errors"
