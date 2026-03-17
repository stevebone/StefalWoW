/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Player.h"
#include "ObjectGuid.h"
#include "Position.h"
#include "Battleground.h"
#include "../AI/Coordination/Battleground/BGState.h"
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>

namespace Playerbot
{

// Forward declarations
class BattlegroundCoordinator;

/**
 * @brief BG strategy profile
 */
struct BGStrategyProfile
{
    bool autoAssignRoles = true;
    bool autoDefendBases = true;
    bool autoCaptureBases = true;
    bool prioritizeHealers = true;
    bool groupUpForObjectives = true;
    uint32 minPlayersForAttack = 3;
    float defenseRadius = 30.0f;
    bool allowRoleSwitch = true;

    BGStrategyProfile() = default;
};

/**
 * @brief BG performance metrics
 */
struct BGMetrics
{
    ::std::atomic<uint32> objectivesCaptured{0};
    ::std::atomic<uint32> objectivesDefended{0};
    ::std::atomic<uint32> flagCaptures{0};
    ::std::atomic<uint32> flagReturns{0};
    ::std::atomic<uint32> basesAssaulted{0};
    ::std::atomic<uint32> basesDefended{0};
    ::std::atomic<uint32> matchesWon{0};
    ::std::atomic<uint32> matchesLost{0};

    void Reset()
    {
        objectivesCaptured = 0;
        objectivesDefended = 0;
        flagCaptures = 0;
        flagReturns = 0;
        basesAssaulted = 0;
        basesDefended = 0;
        matchesWon = 0;
        matchesLost = 0;
    }

    float GetWinRate() const
    {
        uint32 total = matchesWon.load() + matchesLost.load();
        return total > 0 ? static_cast<float>(matchesWon.load()) / total : 0.0f;
    }
};

/**
 * @brief Battleground AI - Thin dispatch layer
 *
 * Routes bot BG updates to the appropriate BG script's ExecuteStrategy()
 * via the BattlegroundCoordinator system. All runtime behavior logic lives
 * in the individual BG scripts (WSG, AB, AV, EOTS, ToK, SSM, etc.).
 *
 * Responsibilities:
 * - Throttle bot updates (500ms)
 * - Ensure bot is registered with coordinator
 * - Dispatch to correct BG script based on map ID
 * - Maintain strategy profiles and metrics
 */
class TC_GAME_API BattlegroundAI final
{
public:
    static BattlegroundAI* instance();

    // ============================================================================
    // CORE
    // ============================================================================

    void Initialize();
    void Update(::Player* player, uint32 diff);

    // ============================================================================
    // PROFILES
    // ============================================================================

    void SetStrategyProfile(uint32 playerGuid, BGStrategyProfile const& profile);
    BGStrategyProfile GetStrategyProfile(uint32 playerGuid) const;

    // ============================================================================
    // METRICS
    // ============================================================================

    BGMetrics const& GetPlayerMetrics(uint32 playerGuid) const;
    BGMetrics const& GetGlobalMetrics() const;

private:
    BattlegroundAI();
    ~BattlegroundAI() = default;

    // ============================================================================
    // THIN DELEGATION METHODS
    // ============================================================================
    // Each delegates to the BG script's ExecuteStrategy() via coordinator.

    void ExecuteWSGStrategy(::Player* player);
    void ExecuteABStrategy(::Player* player);
    void ExecuteAVStrategy(::Player* player);
    void ExecuteEOTSStrategy(::Player* player);
    void ExecuteSiegeStrategy(::Player* player);
    void ExecuteKotmoguStrategy(::Player* player);
    void ExecuteSilvershardStrategy(::Player* player);
    void ExecuteDeepwindStrategy(::Player* player);
    void ExecuteSeethingShoreStrategy(::Player* player);
    void ExecuteAshranStrategy(::Player* player);

    // ============================================================================
    // HELPERS
    // ============================================================================

    BGType GetBattlegroundType(::Player* player) const;
    ::Battleground* GetPlayerBattleground(::Player* player) const;

    // ============================================================================
    // DATA
    // ============================================================================

    // Strategy profiles
    ::std::unordered_map<uint32, BGStrategyProfile> _playerProfiles;

    // Metrics
    ::std::unordered_map<uint32, BGMetrics> _playerMetrics;
    BGMetrics _globalMetrics;

    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _mutex;

    // Update intervals
    static constexpr uint32 BG_UPDATE_INTERVAL = 500;  // 500ms
    ::std::unordered_map<uint32, uint32> _lastUpdateTimes;
};

} // namespace Playerbot
