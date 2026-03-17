/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * CorpsePreventionManager: Prevents TrinityCore from creating Corpse objects for bots
 * This eliminates Map::SendObjectUpdates crashes by avoiding the race condition entirely
 */

#pragma once

#include "Define.h"
#include <memory>
#include <atomic>
#include <chrono>

class Player;

namespace Playerbot
{

class TC_GAME_API CorpsePreventionManager
{
public:
    CorpsePreventionManager();
    ~CorpsePreventionManager();

    // Core hook integration
    static void OnBotBeforeDeath(Player* bot);
    static void OnBotAfterDeath(Player* bot);

    // Instant resurrection without corpse creation
    static bool PreventCorpseAndResurrect(Player* bot);

    // Cache death location for "fake" corpse run
    static void CacheDeathLocation(Player* bot);

    // Check if bot should skip corpse creation
    static bool ShouldPreventCorpse(Player* bot);

    // Configuration
    static void SetEnabled(bool enabled) { s_enabled = enabled; }
    static bool IsEnabled() { return s_enabled; }

private:
    static ::std::atomic<bool> s_enabled;
    static ::std::atomic<uint32> s_preventedCorpses;
    static ::std::atomic<uint32> s_activePrevention;
};

} // namespace Playerbot