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

#include "Define.h"
#include "ObjectGuid.h"
#include <vector>
#include <queue>
#include <map>
#include <functional>

namespace Playerbot {

class DungeonCoordinator;

/**
 * @enum RecoveryPhase
 * @brief Phases of wipe recovery
 */
enum class RecoveryPhase : uint8
{
    WAITING = 0,        // Waiting for combat to end
    RELEASING = 1,      // Releasing spirit
    RUNNING_BACK = 2,   // Running back to dungeon
    REZZING = 3,        // Rezzing dead members
    REBUFFING = 4,      // Rebuffing group
    MANA_REGEN = 5,     // Waiting for mana
    READY = 6           // Ready to continue
};

/**
 * @struct RezPriority
 * @brief Priority information for resurrection
 */
struct RezPriority
{
    ObjectGuid playerGuid;
    uint8 priority = 0;         // Lower = higher priority
    bool hasRezSickness = false;
    uint32 distanceToCorpse = 0;
    bool isHealer = false;
    bool isTank = false;
    bool hasRezSpell = false;   // Can rez others

    bool operator<(const RezPriority& other) const
    {
        return priority > other.priority;  // Min-heap (lower = higher priority)
    }
};

/**
 * @class WipeRecoveryManager
 * @brief Manages recovery after a group wipe
 *
 * Responsibilities:
 * - Coordinate spirit release and run back
 * - Determine optimal rez order (healers > tanks > DPS)
 * - Track rebuffing progress
 * - Monitor mana regeneration
 *
 * Rez Priority:
 * 1. Healer with battle rez (can chain rez)
 * 2. Other healers
 * 3. Tank
 * 4. DPS with battle rez
 * 5. Other DPS
 *
 * Usage:
 * @code
 * WipeRecoveryManager manager(&coordinator);
 *
 * // On wipe
 * manager.OnGroupWipe();
 *
 * // Get next rez target
 * ObjectGuid target = manager.GetNextRezTarget();
 * if (!target.IsEmpty())
 *     // Rez target
 *
 * // Check if ready
 * if (manager.IsGroupReady())
 *     // Resume dungeon
 * @endcode
 */
class TC_GAME_API WipeRecoveryManager
{
public:
    explicit WipeRecoveryManager(DungeonCoordinator* coordinator);
    ~WipeRecoveryManager() = default;

    // ====================================================================
    // LIFECYCLE
    // ====================================================================

    /**
     * @brief Initialize manager
     */
    void Initialize();

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
    // WIPE HANDLING
    // ====================================================================

    /**
     * @brief Called when group wipes
     */
    void OnGroupWipe();

    /**
     * @brief Called when combat ends
     */
    void OnCombatEnded();

    /**
     * @brief Check if recovering
     */
    [[nodiscard]] bool IsRecovering() const { return _phase != RecoveryPhase::READY; }

    /**
     * @brief Get current recovery phase
     */
    [[nodiscard]] RecoveryPhase GetPhase() const { return _phase; }

    /**
     * @brief Get recovery progress (0.0 - 1.0)
     */
    [[nodiscard]] float GetRecoveryProgress() const;

    // ====================================================================
    // REZ MANAGEMENT
    // ====================================================================

    /**
     * @brief Build rez queue based on priorities
     */
    void BuildRezQueue();

    /**
     * @brief Get next player to rez
     * @return GUID of next rez target or empty
     */
    [[nodiscard]] ObjectGuid GetNextRezTarget() const;

    /**
     * @brief Called when player is rezzed
     * @param playerGuid Rezzed player
     */
    void OnPlayerRezzed(ObjectGuid playerGuid);

    /**
     * @brief Check if all players are alive
     */
    [[nodiscard]] bool AllPlayersAlive() const;

    /**
     * @brief Get dead players
     */
    [[nodiscard]] const ::std::vector<ObjectGuid>& GetDeadPlayers() const { return _deadPlayers; }

    /**
     * @brief Get number of dead players
     */
    [[nodiscard]] uint32 GetDeadCount() const { return static_cast<uint32>(_deadPlayers.size()); }

    // ====================================================================
    // REZ PRIORITY
    // ====================================================================

    /**
     * @brief Set rez priority for player
     * @param player Player GUID
     * @param priority Priority (lower = higher)
     */
    void SetRezPriority(ObjectGuid player, uint8 priority);

    /**
     * @brief Get rez priority for player
     * @param player Player GUID
     * @return Priority value
     */
    [[nodiscard]] uint8 GetRezPriority(ObjectGuid player) const;

    /**
     * @brief Get best rezzer (alive player with rez spell)
     * @return GUID of best rezzer or empty
     */
    [[nodiscard]] ObjectGuid GetBestRezzer() const;

    // ====================================================================
    // RUN BACK
    // ====================================================================

    /**
     * @brief Check if should run back (vs wait for rez)
     */
    [[nodiscard]] bool ShouldRunBack() const;

    /**
     * @brief Called when player reaches corpse
     * @param playerGuid Player GUID
     */
    void OnPlayerReachedCorpse(ObjectGuid playerGuid);

    /**
     * @brief Get players still running back
     */
    [[nodiscard]] ::std::vector<ObjectGuid> GetPlayersRunningBack() const;

    // ====================================================================
    // READY CHECK
    // ====================================================================

    /**
     * @brief Check if group is ready to continue
     */
    [[nodiscard]] bool IsGroupReady() const;

    /**
     * @brief Get group mana percentage
     */
    [[nodiscard]] float GetGroupManaPercent() const;

    /**
     * @brief Get group health percentage
     */
    [[nodiscard]] float GetGroupHealthPercent() const;

    /**
     * @brief Check if buffs are complete
     */
    [[nodiscard]] bool AreBuffsComplete() const;

    /**
     * @brief Get minimum mana threshold
     */
    [[nodiscard]] float GetMinManaThreshold() const { return _minManaThreshold; }

    /**
     * @brief Set minimum mana threshold
     * @param percent Mana percentage (0-100)
     */
    void SetMinManaThreshold(float percent) { _minManaThreshold = percent; }

    // ====================================================================
    // REBUFF TRACKING
    // ====================================================================

    /**
     * @brief Check if player needs buffs
     * @param player Player GUID
     * @return true if missing important buffs
     */
    [[nodiscard]] bool NeedsBuffs(ObjectGuid player) const;

    /**
     * @brief Get players needing buffs
     */
    [[nodiscard]] ::std::vector<ObjectGuid> GetPlayersNeedingBuffs() const;

    /**
     * @brief Mark player as buffed
     * @param player Player GUID
     */
    void OnPlayerBuffed(ObjectGuid player);

private:
    DungeonCoordinator* _coordinator;

    // Recovery state
    RecoveryPhase _phase = RecoveryPhase::READY;
    uint32 _recoveryStartTime = 0;
    uint32 _phaseTimer = 0;

    // Dead player tracking
    ::std::vector<ObjectGuid> _deadPlayers;
    ::std::vector<ObjectGuid> _rezzedPlayers;
    ::std::vector<ObjectGuid> _playersRunningBack;

    // Rez queue
    ::std::priority_queue<RezPriority> _rezQueue;
    ::std::map<ObjectGuid, uint8> _customPriorities;

    // Rebuff tracking
    ::std::vector<ObjectGuid> _playersNeedingBuffs;

    // Configuration
    float _minManaThreshold = 80.0f;
    float _minHealthThreshold = 90.0f;

    // Phase timeouts
    static constexpr uint32 WAITING_TIMEOUT_MS = 10000;     // 10 seconds
    static constexpr uint32 RELEASING_TIMEOUT_MS = 15000;   // 15 seconds
    static constexpr uint32 RUNNING_BACK_TIMEOUT_MS = 120000; // 2 minutes
    static constexpr uint32 REBUFFING_TIMEOUT_MS = 60000;   // 1 minute
    static constexpr uint32 MANA_REGEN_TIMEOUT_MS = 60000;  // 1 minute

    // ====================================================================
    // HELPERS
    // ====================================================================

    /**
     * @brief Calculate rez priority for player
     * @param playerGuid Player GUID
     * @return Priority (lower = higher)
     */
    [[nodiscard]] uint8 CalculateRezPriority(ObjectGuid playerGuid) const;

    /**
     * @brief Transition to next phase
     * @param newPhase Phase to transition to
     */
    void TransitionToPhase(RecoveryPhase newPhase);

    /**
     * @brief Check if player has battle rez
     * @param player Player GUID
     * @return true if has combat rez ability
     */
    [[nodiscard]] bool HasBattleRez(ObjectGuid player) const;

    /**
     * @brief Check if player has rez spell
     * @param player Player GUID
     * @return true if can rez
     */
    [[nodiscard]] bool HasRezSpell(ObjectGuid player) const;

    /**
     * @brief Get alive healers
     */
    [[nodiscard]] ::std::vector<ObjectGuid> GetAliveHealers() const;

    /**
     * @brief Get alive players with rez
     */
    [[nodiscard]] ::std::vector<ObjectGuid> GetAliveRezzers() const;
};

/**
 * @brief Convert RecoveryPhase to string
 */
inline const char* RecoveryPhaseToString(RecoveryPhase phase)
{
    switch (phase)
    {
        case RecoveryPhase::WAITING:      return "Waiting";
        case RecoveryPhase::RELEASING:    return "Releasing";
        case RecoveryPhase::RUNNING_BACK: return "Running Back";
        case RecoveryPhase::REZZING:      return "Rezzing";
        case RecoveryPhase::REBUFFING:    return "Rebuffing";
        case RecoveryPhase::MANA_REGEN:   return "Mana Regen";
        case RecoveryPhase::READY:        return "Ready";
        default:                          return "Unknown";
    }
}

} // namespace Playerbot
