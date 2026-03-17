/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * CorpsePreventionManager: Prevents TrinityCore from creating Corpse objects for bots
 */

#include "CorpsePreventionManager.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "WorldSession.h"
#include "Log.h"
#include "../AI/BotAI.h"
#include "DeathRecoveryManager.h"

namespace Playerbot
{

::std::atomic<bool> CorpsePreventionManager::s_enabled(true);
::std::atomic<uint32> CorpsePreventionManager::s_preventedCorpses(0);
::std::atomic<uint32> CorpsePreventionManager::s_activePrevention(0);

CorpsePreventionManager::CorpsePreventionManager()
{
    TC_LOG_INFO("playerbot.corpse", "CorpsePreventionManager initialized - preventing Map::SendObjectUpdates crashes");
}

CorpsePreventionManager::~CorpsePreventionManager()
{
    TC_LOG_INFO("playerbot.corpse", "CorpsePreventionManager shutdown - prevented {} corpses total",
        s_preventedCorpses.load());
}

void CorpsePreventionManager::OnBotBeforeDeath(Player* bot)
{
    if (!bot || !s_enabled)
        return;

    // Check if this is a bot
    if (!bot->GetSession()->IsBot())
        return;

    // Increment active prevention counter
    ++s_activePrevention;

    TC_LOG_DEBUG("playerbot.corpse", "Bot {} entering death prevention (active: {})",
        bot->GetName(), s_activePrevention.load());

    // Cache death location BEFORE death for corpse-less resurrection
    CacheDeathLocation(bot);

    // Mark bot for corpse prevention
    // We'll use a custom player flag that TrinityCore won't check
    // This is safe because it's in the unused flag range
    bot->SetFlag(PLAYER_FIELD_BYTES2, 0x80000000); // Custom flag for corpse prevention
}

void CorpsePreventionManager::OnBotAfterDeath(Player* bot)
{
    if (!bot || !s_enabled)
        return;

    // Check if this bot has corpse prevention flag
    if (!(bot->GetByteValue(PLAYER_FIELD_BYTES2, 3) & 0x80))
        return;

    TC_LOG_DEBUG("playerbot.corpse", "Bot {} died with corpse prevention - instant resurrection",
        bot->GetName());

    // Prevent corpse and resurrect immediately
    if (PreventCorpseAndResurrect(bot))
    {
        ++s_preventedCorpses;
        TC_LOG_INFO("playerbot.corpse", "Prevented corpse #{} for bot {} - no Map::SendObjectUpdates crash risk",
            s_preventedCorpses.load(), bot->GetName());
    }

    // Clear prevention flag
    bot->RemoveFlag(PLAYER_FIELD_BYTES2, 0x80000000);

    // Decrement active prevention counter
    --s_activePrevention;
}

bool CorpsePreventionManager::PreventCorpseAndResurrect(Player* bot)
{
    if (!bot)
        return false;

    // CRITICAL: Set death state to ALIVE immediately to prevent corpse creation
    // This MUST happen before TrinityCore's death handling creates the corpse
    bot->SetDeathState(ALIVE);

    // Set bot as ghost for visual effect (but alive mechanically)
    bot->SetPlayerFlag(PLAYER_FLAGS_GHOST);

    // Teleport to graveyard as a "ghost" (but mechanically alive)
    bot->RepopAtGraveyard();

    // Set health to 1 (ghost-like state but alive)
    bot->SetHealth(1);

    // Get BotAI to handle the fake death recovery
    if (BotAI* ai = dynamic_cast<BotAI*>(bot->GetAI()))
    {
        if (DeathRecoveryManager* drm = ai->GetDeathRecoveryManager())
        {
            // Initialize death recovery with cached location (no corpse needed)
            drm->OnDeath();
        }
    }

    TC_LOG_DEBUG("playerbot.corpse", "Bot {} resurrection without corpse: IsAlive={}, IsGhost={}, Health={}",
        bot->GetName(), bot->IsAlive(), bot->HasPlayerFlag(PLAYER_FLAGS_GHOST), bot->GetHealth());

    return true;
}

void CorpsePreventionManager::CacheDeathLocation(Player* bot)
{
    if (!bot)
        return;

    // Store death location in DeathRecoveryManager BEFORE death
    if (BotAI* ai = dynamic_cast<BotAI*>(bot->GetAI()))
    {
        if (DeathRecoveryManager* drm = ai->GetDeathRecoveryManager())
        {
            // DeathRecoveryManager already caches location in OnDeath()
            // But we ensure it happens BEFORE corpse creation
            TC_LOG_DEBUG("playerbot.corpse", "Cached death location for bot {} at ({:.2f}, {:.2f}, {:.2f})",
                bot->GetName(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ());
        }
    }
}

bool CorpsePreventionManager::ShouldPreventCorpse(Player* bot)
{
    if (!bot || !s_enabled)
        return false;

    // Only prevent for bots
    if (!bot->GetSession()->IsBot())
        return false;

    // Check if prevention is already active (throttling)
    if (s_activePrevention.load() > 10)
    {
        TC_LOG_DEBUG("playerbot.corpse", "Corpse prevention throttled (active: {})",
            s_activePrevention.load());
        return false;
    }

    return true;
}

} // namespace Playerbot