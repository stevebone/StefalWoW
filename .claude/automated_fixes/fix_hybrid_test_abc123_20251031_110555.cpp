// FILE: src/modules/Playerbot/Lifecycle/DeathRecoveryManager.cpp
// ENHANCEMENT: Add 100ms delay before HandleMoveTeleportAck to prevent Spell.cpp:603 crash

void DeathRecoveryManager::ExecuteReleaseSpirit(Player* bot)
{
    if (!bot || !bot->IsBot())
        return;

    // Existing code: BuildPlayerRepop creates corpse and applies Ghost aura
    bot->BuildPlayerRepop();

    // FIX: Defer HandleMoveTeleportAck by 100ms to prevent race condition
    // This prevents Spell.cpp:603 crash when bot dies
    bot->m_Events.AddEventAtOffset([bot]()
    {
        // Validate bot is still valid and in world
        if (!bot || !bot->IsInWorld())
            return;

        // Safe to handle teleport ack now
        if (bot->GetSession())
            bot->GetSession()->HandleMoveTeleportAck();

    }, 100ms);

    LOG_INFO("playerbot", "DeathRecoveryManager: Deferred teleport ack for bot {} by 100ms", bot->GetName());
}

// RATIONALE:
// - Uses TrinityCore's Player::m_Events system (existing feature)
// - Adds 100ms safety delay to prevent race condition
// - Validates bot state before teleport ack (prevents null pointer crash)
// - Module-only fix (hierarchy level 1 - PREFERRED)
// - No core modifications required
// - Leverages existing DeathRecoveryManager infrastructure
