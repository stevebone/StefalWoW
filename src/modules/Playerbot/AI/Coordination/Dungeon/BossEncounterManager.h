/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "DungeonState.h"
#include "Define.h"
#include "ObjectGuid.h"
#include <map>
#include <vector>
#include <functional>
#include <string>

namespace Playerbot {

class DungeonCoordinator;

/**
 * @enum BossMechanic
 * @brief Types of boss mechanics bots need to handle
 */
enum class BossMechanic : uint8
{
    NONE = 0,
    TANK_SWAP = 1,          // Requires tank swap at X stacks
    SPREAD = 2,             // Spread out (avoid splash damage)
    STACK = 3,              // Stack together (share damage)
    MOVE_OUT = 4,           // Move away from boss
    MOVE_IN = 5,            // Move to boss
    INTERRUPT = 6,          // Must interrupt cast
    DISPEL = 7,             // Requires dispel
    DODGE_AOE = 8,          // Dodge ground effect
    SOAK = 9,               // Soak mechanic (stand in something)
    KITE = 10,              // Kite add/boss
    SWITCH_TARGET = 11,     // Switch to priority add
    USE_EXTRA_BUTTON = 12,  // Use extra action button
    BLOODLUST = 13,         // Use bloodlust/heroism
    DEFENSIVE_CD = 14       // Use defensive cooldowns
};

/**
 * @struct MechanicTrigger
 * @brief Defines when a mechanic activates
 */
struct MechanicTrigger
{
    uint32 spellId = 0;          // Spell that triggers this
    BossMechanic mechanic = BossMechanic::NONE;
    uint8 phase = 0;             // 0 = all phases
    float healthThreshold = 0;   // 0 = no threshold
    ::std::string description;

    [[nodiscard]] bool MatchesPhase(uint8 currentPhase) const
    {
        return phase == 0 || phase == currentPhase;
    }
};

/**
 * @struct BossStrategy
 * @brief Strategy for handling a specific boss
 */
struct BossStrategy
{
    uint32 bossId = 0;
    ::std::string name;

    // Phase info
    uint8 totalPhases = 1;
    ::std::vector<float> phaseTransitionHealth;  // Health % for phase transitions

    // Mechanics
    ::std::vector<MechanicTrigger> mechanics;

    // Positioning
    bool tankFaceAway = false;
    ::std::array<bool, 5> spreadInPhase = {false, false, false, false, false};
    float spreadDistance = 5.0f;
    float stackDistance = 2.0f;

    // Timers
    bool hasEnrage = false;
    uint32 enrageTimeMs = 0;
    bool useBloodlustOnPull = false;
    float bloodlustHealthPercent = 30.0f;

    // Tank swap
    bool requiresTankSwap = false;
    uint32 tankSwapSpellId = 0;
    uint8 tankSwapStacks = 3;

    // Interrupts
    ::std::vector<uint32> mustInterruptSpells;
    ::std::vector<uint32> shouldInterruptSpells;

    // Priority targets
    ::std::vector<uint32> priorityAddIds;

    [[nodiscard]] bool HasTankSwap() const { return requiresTankSwap && tankSwapSpellId > 0; }
    [[nodiscard]] bool HasEnrage() const { return hasEnrage && enrageTimeMs > 0; }
};

/**
 * @class BossEncounterManager
 * @brief Manages boss encounter phases, mechanics, and strategy
 *
 * Responsibilities:
 * - Track boss health, phase, and timers
 * - Detect phase transitions
 * - Handle specific mechanics (spread, stack, tank swap)
 * - Coordinate interrupts and dispels
 * - Manage bloodlust timing
 *
 * Usage:
 * @code
 * BossEncounterManager manager(&coordinator);
 * manager.Initialize(dungeonId);
 *
 * // On boss engage
 * manager.OnBossEngaged(bossId);
 *
 * // During combat
 * if (manager.ShouldSpread())
 *     // Command bots to spread
 *
 * if (manager.NeedsTankSwap())
 *     // Execute tank swap
 *
 * if (manager.ShouldUseBloodlust())
 *     // Use bloodlust
 * @endcode
 */
class TC_GAME_API BossEncounterManager
{
public:
    explicit BossEncounterManager(DungeonCoordinator* coordinator);
    ~BossEncounterManager() = default;

    // ====================================================================
    // LIFECYCLE
    // ====================================================================

    /**
     * @brief Initialize for dungeon
     * @param dungeonId Dungeon map ID
     */
    void Initialize(uint32 dungeonId);

    /**
     * @brief Update logic
     * @param diff Time since last update
     */
    void Update(uint32 diff);

    /**
     * @brief Reset state
     */
    void Reset();

    // ====================================================================
    // ENCOUNTER LIFECYCLE
    // ====================================================================

    /**
     * @brief Called when boss is engaged
     * @param bossId Boss creature entry
     */
    void OnBossEngaged(uint32 bossId);

    /**
     * @brief Called when boss is defeated
     * @param bossId Boss creature entry
     */
    void OnBossDefeated(uint32 bossId);

    /**
     * @brief Called on wipe
     * @param bossId Boss creature entry
     */
    void OnBossWipe(uint32 bossId);

    /**
     * @brief Called on phase change
     * @param newPhase New phase number
     */
    void OnPhaseChanged(uint8 newPhase);

    /**
     * @brief Check if encounter is active
     */
    [[nodiscard]] bool IsInEncounter() const { return _currentBossId > 0; }

    // ====================================================================
    // STRATEGY ACCESS
    // ====================================================================

    /**
     * @brief Get current boss strategy
     * @return Pointer to strategy or nullptr
     */
    [[nodiscard]] const BossStrategy* GetCurrentStrategy() const;

    /**
     * @brief Get strategy for specific boss
     * @param bossId Boss creature entry
     * @return Pointer to strategy or nullptr
     */
    [[nodiscard]] const BossStrategy* GetStrategy(uint32 bossId) const;

    /**
     * @brief Load boss strategies for dungeon
     * @param dungeonId Dungeon map ID
     */
    void LoadBossStrategies(uint32 dungeonId);

    /**
     * @brief Register a boss strategy
     * @param strategy Strategy to register
     */
    void RegisterStrategy(const BossStrategy& strategy);

    /**
     * @brief Get all bosses
     */
    [[nodiscard]] ::std::vector<BossInfo> GetAllBosses() const;

    // ====================================================================
    // PHASE TRACKING
    // ====================================================================

    /**
     * @brief Get current phase
     */
    [[nodiscard]] uint8 GetCurrentPhase() const { return _currentPhase; }

    /**
     * @brief Get phase progress (0.0-1.0 within current phase)
     */
    [[nodiscard]] float GetPhaseProgress() const;

    /**
     * @brief Check if in phase transition
     */
    [[nodiscard]] bool IsPhaseTransition() const { return _inPhaseTransition; }

    /**
     * @brief Detect phase based on boss health
     * @param healthPercent Current boss health %
     */
    void DetectPhaseTransition(float healthPercent);

    // ====================================================================
    // MECHANIC HANDLING
    // ====================================================================

    /**
     * @brief Called when mechanic spell is cast
     * @param spellId Mechanic spell ID
     */
    void OnMechanicTriggered(uint32 spellId);

    /**
     * @brief Get current active mechanic
     */
    [[nodiscard]] BossMechanic GetActiveMechanic() const { return _activeMechanic; }

    /**
     * @brief Clear active mechanic
     */
    void ClearActiveMechanic() { _activeMechanic = BossMechanic::NONE; }

    /**
     * @brief Check if should spread
     */
    [[nodiscard]] bool ShouldSpread() const;

    /**
     * @brief Check if should stack
     */
    [[nodiscard]] bool ShouldStack() const;

    /**
     * @brief Get stack target
     * @return GUID to stack on (usually tank or boss)
     */
    [[nodiscard]] ObjectGuid GetStackTarget() const;

    /**
     * @brief Get spread distance
     */
    [[nodiscard]] float GetSpreadDistance() const;

    // ====================================================================
    // TANK SWAP
    // ====================================================================

    /**
     * @brief Check if tank swap needed
     */
    [[nodiscard]] bool NeedsTankSwap() const;

    /**
     * @brief Called when tank swap completed
     */
    void OnTankSwapCompleted();

    /**
     * @brief Get tank debuff stacks
     * @param tank Tank GUID
     * @return Number of stacks
     */
    [[nodiscard]] uint8 GetTankSwapStacks(ObjectGuid tank) const;

    /**
     * @brief Update tank debuff stacks
     * @param tank Tank GUID
     * @param stacks New stack count
     */
    void UpdateTankStacks(ObjectGuid tank, uint8 stacks);

    // ====================================================================
    // INTERRUPTS
    // ====================================================================

    /**
     * @brief Check if spell should be interrupted
     * @param spellId Spell being cast
     * @return true if must/should interrupt
     */
    [[nodiscard]] bool ShouldInterrupt(uint32 spellId) const;

    /**
     * @brief Get interrupt priority
     * @param spellId Spell being cast
     * @return Priority (0 = don't interrupt, 1 = optional, 2 = must)
     */
    [[nodiscard]] uint8 GetInterruptPriority(uint32 spellId) const;

    // ====================================================================
    // BLOODLUST
    // ====================================================================

    /**
     * @brief Check if should use bloodlust
     */
    [[nodiscard]] bool ShouldUseBloodlust() const;

    /**
     * @brief Called when bloodlust is used
     */
    void OnBloodlustUsed();

    /**
     * @brief Check if bloodlust was used
     */
    [[nodiscard]] bool WasBloodlustUsed() const { return _bloodlustUsed; }

    // ====================================================================
    // COMBAT STATS
    // ====================================================================

    /**
     * @brief Get encounter duration
     * @return Duration in milliseconds
     */
    [[nodiscard]] uint32 GetEncounterDuration() const;

    /**
     * @brief Get boss health percent
     */
    [[nodiscard]] float GetBossHealthPercent() const;

    /**
     * @brief Set boss health percent (for tracking)
     * @param percent Health percentage
     */
    void SetBossHealthPercent(float percent);

    /**
     * @brief Check if boss is enraging
     */
    [[nodiscard]] bool IsEnraging() const;

    /**
     * @brief Get time until enrage
     * @return Time in milliseconds, 0 if no enrage
     */
    [[nodiscard]] uint32 GetTimeToEnrage() const;

    /**
     * @brief Get current boss info
     */
    [[nodiscard]] BossInfo* GetCurrentBoss();

private:
    DungeonCoordinator* _coordinator;

    // Current encounter state
    uint32 _currentBossId = 0;
    uint8 _currentPhase = 0;
    bool _inPhaseTransition = false;
    BossMechanic _activeMechanic = BossMechanic::NONE;
    uint32 _encounterStartTime = 0;
    bool _bloodlustUsed = false;
    float _bossHealthPercent = 100.0f;

    // Tank swap tracking
    ::std::map<ObjectGuid, uint8> _tankSwapStacks;
    ObjectGuid _currentTank;
    bool _tankSwapPending = false;

    // Strategies loaded
    ::std::map<uint32, BossStrategy> _strategies;

    // Boss info
    ::std::map<uint32, BossInfo> _bosses;

    // Mechanic timers
    uint32 _mechanicTimer = 0;
    static constexpr uint32 MECHANIC_DURATION_MS = 5000;  // 5 second mechanic window

    // ====================================================================
    // MECHANIC HANDLERS
    // ====================================================================

    void HandleTankSwapMechanic(const MechanicTrigger& trigger);
    void HandleSpreadMechanic(const MechanicTrigger& trigger);
    void HandleStackMechanic(const MechanicTrigger& trigger);
    void HandleDodgeMechanic(const MechanicTrigger& trigger);
    void HandleInterruptMechanic(const MechanicTrigger& trigger);
};

/**
 * @brief Convert BossMechanic to string
 */
inline const char* BossMechanicToString(BossMechanic mechanic)
{
    switch (mechanic)
    {
        case BossMechanic::NONE:            return "None";
        case BossMechanic::TANK_SWAP:       return "Tank Swap";
        case BossMechanic::SPREAD:          return "Spread";
        case BossMechanic::STACK:           return "Stack";
        case BossMechanic::MOVE_OUT:        return "Move Out";
        case BossMechanic::MOVE_IN:         return "Move In";
        case BossMechanic::INTERRUPT:       return "Interrupt";
        case BossMechanic::DISPEL:          return "Dispel";
        case BossMechanic::DODGE_AOE:       return "Dodge AoE";
        case BossMechanic::SOAK:            return "Soak";
        case BossMechanic::KITE:            return "Kite";
        case BossMechanic::SWITCH_TARGET:   return "Switch Target";
        case BossMechanic::USE_EXTRA_BUTTON:return "Extra Button";
        case BossMechanic::BLOODLUST:       return "Bloodlust";
        case BossMechanic::DEFENSIVE_CD:    return "Defensive CD";
        default:                            return "Unknown";
    }
}

} // namespace Playerbot
