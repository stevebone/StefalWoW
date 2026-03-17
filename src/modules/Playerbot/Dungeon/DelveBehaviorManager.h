/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * DELVE BEHAVIOR MANAGER
 *
 * Manages bot behavior in Delve content - small instanced areas for 1-4
 * players. Handles delve-specific mechanics including:
 *   - Delve detection and tier awareness
 *   - Objective tracking and completion
 *   - NPC companion (Brann) interaction awareness
 *   - Adaptive difficulty response based on tier
 *   - Loot chest discovery and interaction
 *   - Boss encounter handling within delves
 *
 * Architecture:
 *   - Per-bot instance managed during delve content
 *   - State machine: IDLE -> ENTERING -> EXPLORING -> OBJECTIVE -> BOSS -> LOOTING -> COMPLETED
 *   - Integrates with existing DungeonBehavior for combat coordination
 *   - Leverages ConsumableManager content type awareness
 *
 * Usage:
 *   DelveBehaviorManager delveMgr(bot);
 *   delveMgr.OnDelveEntered(mapId, tier);
 *   delveMgr.Update(diff);
 *   if (delveMgr.ShouldInteractWithCompanion())
 *       delveMgr.HandleCompanionInteraction();
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <string>
#include <vector>
#include <unordered_map>

class Player;
class Unit;
class Creature;
class GameObject;

namespace Playerbot
{

// ============================================================================
// DELVE STATE MACHINE
// ============================================================================

enum class DelveState : uint8
{
    IDLE            = 0,    // Not in a delve
    ENTERING        = 1,    // Just entered, orienting
    EXPLORING       = 2,    // Moving through the delve
    OBJECTIVE       = 3,    // Working on an objective
    COMBAT          = 4,    // In delve combat
    BOSS            = 5,    // Fighting delve boss
    LOOTING         = 6,    // Looting chests/rewards
    COMPLETED       = 7,    // Delve finished, awaiting exit
    FAILED          = 8     // Delve failed (wipe/timeout)
};

// ============================================================================
// DELVE TIER CONFIGURATION
// ============================================================================

struct DelveTierConfig
{
    uint8 tier{1};                          // 1-11
    float difficultyMultiplier{1.0f};       // Scales with tier
    bool requiresConsumables{false};        // Should use flasks/food at this tier
    bool requiresFullGroup{false};          // Needs 4 players at this tier
    uint32 expectedDurationMs{300000};      // Expected completion time
    float defensiveCooldownThreshold{0.4f}; // HP% to use defensives
    bool useBurstOnBoss{true};              // Should burst during boss
};

// ============================================================================
// DELVE OBJECTIVE
// ============================================================================

struct DelveObjective
{
    uint32 objectiveId{0};
    ::std::string description;
    Position location;
    bool completed{false};
    bool isBoss{false};
    uint32 creatureEntry{0};        // For kill objectives
    uint32 gameObjectEntry{0};      // For interaction objectives
    float discoveryRadius{30.0f};   // How close before we "discover" it
};

// ============================================================================
// DELVE COMPANION INFO
// ============================================================================

struct DelveCompanionInfo
{
    ObjectGuid companionGuid;       // Brann Bronzebeard or equivalent
    bool isActive{false};
    bool needsInteraction{false};   // Companion wants player to interact
    Position lastKnownPosition;
    uint32 companionEntry{0};       // NPC entry
    float healthPercent{100.0f};
};

// ============================================================================
// DELVE STATISTICS
// ============================================================================

struct DelveStats
{
    uint32 delvesEntered{0};
    uint32 delvesCompleted{0};
    uint32 delvesFailed{0};
    uint32 objectivesCompleted{0};
    uint32 bossesKilled{0};
    uint32 chestsLooted{0};
    uint64 totalDelveDurationMs{0};
    uint32 highestTierCompleted{0};
    uint32 deathsInDelves{0};
};

// ============================================================================
// DELVE BEHAVIOR MANAGER
// ============================================================================

class DelveBehaviorManager
{
public:
    explicit DelveBehaviorManager(Player* bot);
    ~DelveBehaviorManager() = default;

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /// Called when bot enters a delve instance
    void OnDelveEntered(uint32 mapId, uint8 tier);

    /// Called when bot exits a delve
    void OnDelveExited();

    /// Called when a delve objective is completed
    void OnObjectiveCompleted(uint32 objectiveId);

    /// Called when the delve boss is killed
    void OnBossKilled(uint32 creatureEntry);

    /// Called on bot death inside a delve
    void OnDeathInDelve();

    /// Main update loop
    void Update(uint32 diff);

    // ========================================================================
    // STATE QUERIES
    // ========================================================================

    /// Is the bot currently in a delve?
    bool IsInDelve() const { return _state != DelveState::IDLE; }

    /// Get current delve state
    DelveState GetState() const { return _state; }

    /// Get current delve tier
    uint8 GetCurrentTier() const { return _currentTier; }

    /// Get current delve map ID
    uint32 GetCurrentMapId() const { return _currentMapId; }

    /// Get state as string for logging
    ::std::string GetStateString() const;

    /// Get progress (0.0 - 1.0)
    float GetProgress() const;

    /// Get time spent in current delve (ms)
    uint32 GetDelveDuration() const;

    // ========================================================================
    // COMPANION MANAGEMENT
    // ========================================================================

    /// Check if companion needs attention
    bool ShouldInteractWithCompanion() const;

    /// Handle companion interaction
    void HandleCompanionInteraction();

    /// Update companion tracking
    void UpdateCompanionTracking();

    /// Get companion info
    DelveCompanionInfo const& GetCompanionInfo() const { return _companion; }

    // ========================================================================
    // OBJECTIVE MANAGEMENT
    // ========================================================================

    /// Get current objective (if any)
    DelveObjective const* GetCurrentObjective() const;

    /// Get all known objectives
    ::std::vector<DelveObjective> const& GetObjectives() const { return _objectives; }

    /// Get number of completed objectives
    uint32 GetCompletedObjectiveCount() const;

    /// Get total number of objectives
    uint32 GetTotalObjectiveCount() const { return static_cast<uint32>(_objectives.size()); }

    // ========================================================================
    // COMBAT BEHAVIOR QUERIES
    // ========================================================================

    /// Should bots use consumables for this delve tier?
    bool ShouldUseConsumables() const;

    /// Should bots use burst cooldowns on the current target?
    bool ShouldBurstCurrentTarget() const;

    /// Get defensive cooldown threshold for this tier
    float GetDefensiveCooldownThreshold() const;

    /// Should bots group up tightly?
    bool ShouldGroupUp() const;

    /// Is this a boss encounter in the delve?
    bool IsInBossEncounter() const { return _state == DelveState::BOSS; }

    // ========================================================================
    // STATISTICS
    // ========================================================================

    /// Get delve statistics
    DelveStats const& GetStats() const { return _stats; }

    /// Get a formatted summary string
    ::std::string FormatSummary() const;

    /// Reset statistics
    void ResetStats() { _stats = DelveStats{}; }

private:
    // ========================================================================
    // STATE TRANSITIONS
    // ========================================================================

    void TransitionTo(DelveState newState);
    void UpdateEnteringState(uint32 diff);
    void UpdateExploringState(uint32 diff);
    void UpdateObjectiveState(uint32 diff);
    void UpdateCombatState(uint32 diff);
    void UpdateBossState(uint32 diff);
    void UpdateLootingState(uint32 diff);
    void UpdateCompletedState(uint32 diff);

    // ========================================================================
    // DISCOVERY
    // ========================================================================

    void DiscoverObjectives();
    void DiscoverCompanion();
    void ScanForLootChests();
    DelveObjective* FindNearestIncompleteObjective();

    // ========================================================================
    // TIER CONFIGURATION
    // ========================================================================

    DelveTierConfig GetTierConfig(uint8 tier) const;

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    Player* _bot;
    DelveState _state{DelveState::IDLE};
    uint8 _currentTier{0};
    uint32 _currentMapId{0};
    uint32 _delveStartTime{0};
    DelveTierConfig _tierConfig;

    // Objectives
    ::std::vector<DelveObjective> _objectives;
    uint32 _currentObjectiveIndex{0};

    // Companion
    DelveCompanionInfo _companion;

    // Chest tracking
    ::std::vector<ObjectGuid> _discoveredChests;
    ::std::vector<ObjectGuid> _lootedChests;

    // Statistics
    DelveStats _stats;

    // Timers
    uint32 _stateTimer{0};
    uint32 _scanTimer{0};
    uint32 _companionCheckTimer{0};

    static constexpr uint32 STATE_UPDATE_INTERVAL = 1000;     // 1s
    static constexpr uint32 SCAN_INTERVAL = 5000;             // 5s
    static constexpr uint32 COMPANION_CHECK_INTERVAL = 3000;  // 3s
    static constexpr uint32 ENTERING_ORIENTATION_TIME = 3000; // 3s to orient
    static constexpr uint32 LOOTING_TIMEOUT = 10000;          // 10s max looting
    static constexpr uint32 COMPLETED_LINGER_TIME = 5000;     // 5s before exit
    static constexpr float OBJECTIVE_PROXIMITY = 10.0f;       // Yards to trigger objective
    static constexpr float COMPANION_MAX_DISTANCE = 40.0f;    // Max yards from companion
    static constexpr float CHEST_INTERACTION_RANGE = 5.0f;    // Yards to interact with chest
};

} // namespace Playerbot
