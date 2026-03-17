# PowerShell script to fix CastSpell parameter order in all *Refactored.h files

$files = Get-ChildItem -Path "C:\TrinityBots\TrinityCore\src\modules\Playerbot\AI\ClassAI" -Filter "*Refactored.h" -Recurse

foreach ($file in $files) {
    Write-Host "Processing $($file.FullName)..."
    
    $content = Get-Content $file.FullName -Raw
    
    # Fix CastSpell(SPELL_ID, target) -> CastSpell(target, SPELL_ID)
    $content = $content -replace 'this->CastSpell\(([A-Z_]+),\s*target\)', 'this->CastSpell(target, $1)'
    
    # Fix CastSpell(SPELL_ID, bot) -> CastSpell(bot, SPELL_ID)
    $content = $content -replace 'this->CastSpell\(([A-Z_]+),\s*bot\)', 'this->CastSpell(bot, $1)'
    
    Set-Content $file.FullName -Value $content -NoNewline
}

Write-Host "Done!"
