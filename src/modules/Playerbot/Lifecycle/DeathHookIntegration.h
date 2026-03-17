/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * DeathHookIntegration: Minimal core hooks for Playerbot death management
 * Provides safe integration points without modifying core death logic
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"

class Player;
class Corpse;

namespace Playerbot
{

// Static hook class for minimal core integration
class TC_GAME_API DeathHookIntegration
{
public:
    // Called from Player::setDeathState() - BEFORE corpse creation
    static void OnPlayerPreDeath(Player* player);

    // Called from Player::BuildPlayerRepop() - AFTER corpse creation
    static void OnPlayerCorpseCreated(Player* player, Corpse* corpse);

    // Called from Map::RemoveCorpse() - BEFORE corpse deletion
    static bool OnCorpsePreRemove(Corpse* corpse);

    // Called from Player::ResurrectPlayer() - BEFORE resurrection
    static void OnPlayerPreResurrection(Player* player);

    // Called from Player::ResurrectPlayer() - AFTER resurrection
    static void OnPlayerPostResurrection(Player* player);

    // Configuration
    static void SetEnabled(bool enabled) { s_enabled = enabled; }
    static bool IsEnabled() { return s_enabled; }

private:
    static bool s_enabled;
};

// Macro for easy core integration (add to Player.cpp)
#define PLAYERBOT_DEATH_HOOK_PRE(player) \
    if (Playerbot::DeathHookIntegration::IsEnabled()) \
        Playerbot::DeathHookIntegration::OnPlayerPreDeath(player);

#define PLAYERBOT_CORPSE_HOOK_CREATED(player, corpse) \
    if (Playerbot::DeathHookIntegration::IsEnabled()) \
        Playerbot::DeathHookIntegration::OnPlayerCorpseCreated(player, corpse);

#define PLAYERBOT_CORPSE_HOOK_REMOVE(corpse) \
    if (Playerbot::DeathHookIntegration::IsEnabled()) \
        if (!Playerbot::DeathHookIntegration::OnCorpsePreRemove(corpse)) \
            return; // Prevent removal

#define PLAYERBOT_RESURRECTION_HOOK_PRE(player) \
    if (Playerbot::DeathHookIntegration::IsEnabled()) \
        Playerbot::DeathHookIntegration::OnPlayerPreResurrection(player);

#define PLAYERBOT_RESURRECTION_HOOK_POST(player) \
    if (Playerbot::DeathHookIntegration::IsEnabled()) \
        Playerbot::DeathHookIntegration::OnPlayerPostResurrection(player);

} // namespace Playerbot