/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * DeathHookIntegration: Minimal core hooks for Playerbot death management
 *
 * P1 FIX: Task 7/7 from SESSION_LIFECYCLE_FIXES
 * Updated to use unified CorpseCrashMitigation instead of separate managers
 */

#include "DeathHookIntegration.h"
#include "Player.h"
#include "Corpse.h"
#include "WorldSession.h"
#include "Log.h"
#include "CorpseCrashMitigation.h"  // P1 FIX: Unified component replaces old managers
#include "DeathRecoveryManager.h"
#include "../AI/BotAI.h"

namespace Playerbot
{

bool DeathHookIntegration::s_enabled = true;

void DeathHookIntegration::OnPlayerPreDeath(Player* player)
{
    if (!player || !player->GetSession()->IsBot())
        return;

    TC_LOG_DEBUG("playerbot.death.hook", "OnPlayerPreDeath: Bot {} about to die",
        player->GetName());

    // P1 FIX: Use unified CorpseCrashMitigation (dual-strategy pattern)
    // This handles both prevention (strategy 1) and safe tracking (strategy 2)
    sCorpseCrashMitigation.OnBotDeath(player);

    // Also notify DeathRecoveryManager (death location caching)
    if (BotAI* ai = dynamic_cast<BotAI*>(player->GetAI()))
    {
        if (DeathRecoveryManager* drm = ai->GetDeathRecoveryManager())
        {
            // This ensures corpse location is cached BEFORE the race condition
            drm->OnDeath();
        }
    }
}

void DeathHookIntegration::OnPlayerCorpseCreated(Player* player, Corpse* corpse)
{
    if (!player || !player->GetSession()->IsBot())
        return;

    TC_LOG_DEBUG("playerbot.death.hook", "OnPlayerCorpseCreated: Bot {} corpse {}",
        player->GetName(), corpse ? corpse->GetGUID().ToString() : "none (prevented)");

    // P1 FIX: Use unified CorpseCrashMitigation
    // If corpse is null, prevention succeeded (strategy 1)
    // If corpse exists, fallback to safe tracking (strategy 2)
    sCorpseCrashMitigation.OnCorpseCreated(player, corpse);
}

bool DeathHookIntegration::OnCorpsePreRemove(Corpse* corpse)
{
    if (!corpse)
        return true; // Allow removal

    ObjectGuid corpseGuid = corpse->GetGUID();

    // P1 FIX: Use unified CorpseCrashMitigation
    // Check if this corpse is safe to remove (strategy 2: safe tracking)
    if (!sCorpseCrashMitigation.IsCorpseSafeToDelete(corpseGuid))
    {
        TC_LOG_DEBUG("playerbot.death.hook",
            "OnCorpsePreRemove: Delaying corpse {} removal - Map update in progress",
            corpseGuid.ToString());
        return false; // Prevent removal
    }

    TC_LOG_TRACE("playerbot.death.hook", "OnCorpsePreRemove: Corpse {} safe to remove",
        corpseGuid.ToString());
    return true; // Allow removal
}

void DeathHookIntegration::OnPlayerPreResurrection(Player* player)
{
    if (!player || !player->GetSession()->IsBot())
        return;

    TC_LOG_DEBUG("playerbot.death.hook", "OnPlayerPreResurrection: Bot {} about to resurrect",
        player->GetName());

    // P1 FIX: Use unified CorpseCrashMitigation
    // Check if bot has a tracked corpse
    ObjectGuid playerGuid = player->GetGUID();
    float x, y, z;
    uint32 mapId;

    if (sCorpseCrashMitigation.GetCorpseLocation(playerGuid, x, y, z, mapId))
    {
        // We know the player is resurrecting, so corpse can be safely removed
        // after current Map update cycle completes
        TC_LOG_DEBUG("playerbot.death.hook",
            "Bot {} resurrecting - corpse tracked at ({:.2f}, {:.2f}, {:.2f})",
            player->GetName(), x, y, z);
    }
}

void DeathHookIntegration::OnPlayerPostResurrection(Player* player)
{
    if (!player || !player->GetSession()->IsBot())
        return;

    TC_LOG_DEBUG("playerbot.death.hook", "OnPlayerPostResurrection: Bot {} resurrected",
        player->GetName());

    // P1 FIX: Use unified CorpseCrashMitigation
    // Clean up death locations and corpse tracking
    sCorpseCrashMitigation.OnBotResurrection(player);

    // Notify DeathRecoveryManager
    if (BotAI* ai = dynamic_cast<BotAI*>(player->GetAI()))
    {
        if (DeathRecoveryManager* drm = ai->GetDeathRecoveryManager())
        {
            drm->OnResurrection();
        }
    }
}

} // namespace Playerbot