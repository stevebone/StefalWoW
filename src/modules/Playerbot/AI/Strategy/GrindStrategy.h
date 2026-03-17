/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Strategy.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <atomic>
#include <chrono>

class Creature;
class GameObject;

namespace Playerbot
{

class GatheringManager;
class ProfessionManager;

/**
 * @class GrindStrategy
 * @brief Fallback strategy for grinding mobs when quests are unavailable
 *
 * This strategy activates as a fallback when:
 * - Bot has no active quests
 * - No quest givers found within 300 yards
 * - No suitable quest hubs available for level range
 * - Quest search failures exceed threshold (3+ failures)
 *
 * **Grinding Behavior:**
 * - Hunts level-appropriate mobs (±3 levels of bot)
 * - Prefers solo mobs over packs
 * - Avoids elite/rare mobs unless significantly lower level
 * - Stays within current zone boundaries
 * - Loots all kills for cloth/leather/gold
 *
 * **Profession Integration:**
 * - Mining: Detects and gathers ore nodes while grinding
 * - Herbalism: Detects and gathers herb nodes while grinding
 * - Skinning: Skins killed beasts for leather
 * - Cloth: Prioritizes humanoid kills for cloth drops
 *
 * **Level-Up Re-evaluation:**
 * - On level-up, immediately re-checks quest availability
 * - If new quests available, deactivates and returns to QuestStrategy
 *
 * **Priority**: 40 (below Quest=50, above Solo=10)
 * - Activates only when QuestStrategy cannot find objectives
 * - Deactivates immediately when quests become available
 *
 * @see QuestStrategy - Primary quest-based progression
 * @see SoloCombatStrategy - Handles actual combat execution
 * @see GatheringManager - Profession gathering integration
 */
class TC_GAME_API GrindStrategy : public Strategy
{
public:
    GrindStrategy();
    ~GrindStrategy() override = default;

    // ========================================================================
    // STRATEGY INTERFACE
    // ========================================================================

    void InitializeActions();
    void InitializeTriggers();
    void InitializeValues();

    void OnActivate(BotAI* ai);
    void OnDeactivate(BotAI* ai);

    bool IsActive(BotAI* ai) const;
    float GetRelevance(BotAI* ai) const;

    void UpdateBehavior(BotAI* ai, uint32 diff);

    // ========================================================================
    // GRINDING STATE
    // ========================================================================

    /**
     * @brief Check if grinding is currently active
     */
    bool IsGrinding() const { return _isGrinding.load(std::memory_order_acquire); }

    /**
     * @brief Check if bot should be grinding (fallback conditions met)
     */
    bool ShouldGrind(BotAI* ai) const;

    /**
     * @brief Get current grind target
     */
    Creature* GetGrindTarget() const { return _currentTarget; }

    /**
     * @brief Get grinding statistics
     */
    uint32 GetMobsKilled() const { return _mobsKilled.load(); }
    uint32 GetGatheringNodesCollected() const { return _gatheringNodesCollected.load(); }
    uint32 GetXPGained() const { return _xpGained.load(); }

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Set level range for target selection
     * @param range Levels above/below bot level to consider (default: 3)
     */
    void SetLevelRange(uint8 range) { _levelRange = range; }

    /**
     * @brief Set maximum aggro range for pulling
     * @param range Range in yards (default: 30)
     */
    void SetPullRange(float range) { _pullRange = range; }

    /**
     * @brief Set scan range for detecting mobs and gathering nodes
     * @param range Range in yards (default: 60)
     */
    void SetScanRange(float range) { _scanRange = range; }

    /**
     * @brief Enable/disable profession integration
     */
    void SetProfessionIntegration(bool enable) { _professionIntegrationEnabled = enable; }

    /**
     * @brief Set priority for humanoid targets (cloth farming)
     */
    void SetHumanoidPriority(float priority) { _humanoidPriority = priority; }

    /**
     * @brief Set priority for beast targets (skinning)
     */
    void SetBeastPriority(float priority) { _beastPriority = priority; }

private:
    // ========================================================================
    // TARGET SELECTION
    // ========================================================================

    /**
     * @brief Find best grinding target within scan range
     * @param ai Bot AI context
     * @return Best target or nullptr if none found
     */
    Creature* FindGrindTarget(BotAI* ai);

    /**
     * @brief Calculate target score for grinding priority
     * @param bot Bot player
     * @param creature Target creature
     * @return Score (higher = better target)
     */
    float CalculateTargetScore(Player* bot, Creature* creature) const;

    /**
     * @brief Check if creature is valid grinding target
     * @param bot Bot player
     * @param creature Target creature
     * @return true if valid target
     */
    bool IsValidGrindTarget(Player* bot, Creature* creature) const;

    /**
     * @brief Check if creature is in appropriate level range
     */
    bool IsLevelAppropriate(Player* bot, Creature* creature) const;

    /**
     * @brief Check if creature is safe to pull (not in pack, not elite)
     */
    bool IsSafeToPull(Player* bot, Creature* creature) const;

    // ========================================================================
    // PROFESSION INTEGRATION
    // ========================================================================

    /**
     * @brief Check for nearby gathering nodes
     * @param ai Bot AI context
     * @return true if gathering node found and being gathered
     */
    bool CheckForGatheringNodes(BotAI* ai);

    /**
     * @brief Check if killed creature is skinnable
     * @param ai Bot AI context
     * @param creature Killed creature
     * @return true if skinning initiated
     */
    bool TrySkinCreature(BotAI* ai, Creature* creature);

    /**
     * @brief Get priority modifier based on creature type and professions
     * @param bot Bot player
     * @param creature Target creature
     * @return Priority modifier (1.0 = normal, >1.0 = higher priority)
     */
    float GetProfessionPriorityModifier(Player* bot, Creature* creature) const;

    // ========================================================================
    // LEVEL-UP HANDLING
    // ========================================================================

    /**
     * @brief Handle level-up event - re-evaluate quest availability
     * @param ai Bot AI context
     */
    void OnLevelUp(BotAI* ai);

    /**
     * @brief Check if quests are now available after leveling
     * @param ai Bot AI context
     * @return true if quests became available
     */
    bool CheckQuestAvailability(BotAI* ai);

    // ========================================================================
    // MOVEMENT & PATHING
    // ========================================================================

    /**
     * @brief Move to grinding target
     * @param ai Bot AI context
     * @param target Target creature
     * @return true if movement initiated
     */
    bool MoveToTarget(BotAI* ai, Creature* target);

    /**
     * @brief Wander to new grinding area when current area is exhausted
     * @param ai Bot AI context
     * @return true if movement initiated
     */
    bool WanderToNewArea(BotAI* ai);

    /**
     * @brief Check if current area has targets
     */
    bool HasTargetsInArea(BotAI* ai) const;

    // ========================================================================
    // STATE MANAGEMENT
    // ========================================================================

    enum class GrindState : uint8
    {
        IDLE,           // Not grinding
        SCANNING,       // Looking for targets
        MOVING,         // Moving to target
        COMBAT,         // In combat with target
        LOOTING,        // Looting killed mob
        SKINNING,       // Skinning killed beast
        GATHERING,      // Gathering nearby node
        WANDERING,      // Moving to new area
        RESTING         // Recovering health/mana
    };

    GrindState _state = GrindState::IDLE;
    void SetState(GrindState state);
    const char* GetStateName() const;

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    // State flags
    std::atomic<bool> _isGrinding{false};
    std::atomic<bool> _questCheckPending{false};

    // Current target
    Creature* _currentTarget = nullptr;
    ObjectGuid _currentTargetGuid;
    Position _lastTargetPosition;

    // Configuration
    uint8 _levelRange = 3;           // Target mobs within ±3 levels
    float _pullRange = 30.0f;        // Max pull range
    float _scanRange = 60.0f;        // Scan range for targets
    bool _professionIntegrationEnabled = true;
    float _humanoidPriority = 1.5f;  // Boost for cloth farming
    float _beastPriority = 1.5f;     // Boost for skinning

    // Statistics
    std::atomic<uint32> _mobsKilled{0};
    std::atomic<uint32> _gatheringNodesCollected{0};
    std::atomic<uint32> _xpGained{0};
    uint32 _lastLevelXP = 0;

    // Timing
    uint32 _lastScanTime = 0;
    uint32 _lastWanderTime = 0;
    uint32 _combatStartTime = 0;
    uint32 _lastLevelUpCheck = 0;
    uint32 _lastQuestCheckTime = 0;  // Periodic quest giver check during grinding

    // Level tracking for re-evaluation
    uint8 _lastKnownLevel = 0;

    // Quest search failure tracking (from QuestStrategy)
    uint32 _questSearchFailures = 0;

    // Constants
    static constexpr uint32 SCAN_INTERVAL = 2000;           // 2 seconds
    static constexpr uint32 WANDER_INTERVAL = 30000;        // 30 seconds
    static constexpr uint32 LEVEL_CHECK_INTERVAL = 1000;    // 1 second
    static constexpr uint32 QUEST_FAILURE_THRESHOLD = 3;    // Activate after 3 failed quest searches
    static constexpr float WANDER_DISTANCE = 50.0f;         // Yards to wander
    static constexpr float MIN_TARGET_DISTANCE = 5.0f;      // Min distance to engage
    static constexpr float REST_HEALTH_THRESHOLD = 0.5f;    // Rest below 50% health
    static constexpr float REST_MANA_THRESHOLD = 0.3f;      // Rest below 30% mana
};

} // namespace Playerbot
