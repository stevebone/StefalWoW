# PowerShell script to migrate sConfigMgr calls to sPlayerbotConfig
# For playerbot module configuration migration

$files = @(
    "src/modules/Playerbot/Config/PlayerbotQuestConfig.cpp",
    "src/modules/Playerbot/Config/PlayerbotTradeConfig.cpp",
    "src/modules/Playerbot/Economy/AuctionManager.cpp",
    "src/modules/Playerbot/Lifecycle/DeathRecoveryManager.cpp",
    "src/modules/Playerbot/Session/BotWorldSessionMgr.cpp"
)

foreach ($file in $files) {
    Write-Host "Processing $file..."
    
    if (Test-Path $file) {
        $content = Get-Content $file -Raw
        
        # Replace #include "Config.h" with #include "PlayerbotConfig.h" if needed
        if ($file -notlike "*PlayerbotQuestConfig.cpp" -and $file -notlike "*PlayerbotTradeConfig.cpp") {
            $content = $content -replace '#include "Config\.h"', '#include "../Config/PlayerbotConfig.h"'
        }
        
        # Replace sConfigMgr-> calls with sPlayerbotConfig->
        $content = $content -replace 'sConfigMgr->GetBoolDefault\(', 'sPlayerbotConfig->GetBool('
        $content = $content -replace 'sConfigMgr->GetIntDefault\(', 'sPlayerbotConfig->GetInt('
        $content = $content -replace 'sConfigMgr->GetFloatDefault\(', 'sPlayerbotConfig->GetFloat('
        $content = $content -replace 'sConfigMgr->GetStringDefault\(', 'sPlayerbotConfig->GetString('
        
        # Save back
        Set-Content -Path $file -Value $content -NoNewline
        Write-Host "  Completed $file"
    } else {
        Write-Host "  File not found: $file"
    }
}

Write-Host "`nMigration complete!"
