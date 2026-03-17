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
#include "Core/Events/ICombatEventSubscriber.h"
#include "Core/Events/CombatEventType.h"
#include "Define.h"
#include "ObjectGuid.h"
#include <memory>
#include <vector>
#include <map>
#include <atomic>

class Group;
class Player;
class Map;
class Creature;

namespace Playerbot {

// Forward declarations
class TrashPullManager;
class BossEncounterManager;
class WipeRecoveryManager;
class MythicPlusManager;

/**
 * @class DungeonCoordinator
 * @brief Coordinates bot behavior in 5-man dungeons
 *
 * Phase 4 Implementation - Task 4.1
 *
 * Responsibilities:
 * - Track dungeon state (trash, boss, wipe, etc.)
 * - Coordinate trash pulls with CC and markers
 * - Manage boss encounters (phases, mechanics)
 * - Handle wipe recovery (rez order, rebuff)
 * - Support Mythic+ timer, affixes, enemy forces
 *
 * Architecture:
 * - Implements ICombatEventSubscriber for event-driven updates
 * - Delegates to specialized sub-managers:
 *   - TrashPullManager: CC, markers, pull planning
 *   - BossEncounterManager: Boss mechanics, phases
 *   - WipeRecoveryManager: Rez order, rebuff
 *   - MythicPlusManager: Timer, affixes, route
 *
 * Usage:
 * @code
 * DungeonCoordinator coord(group);
 * coord.Initialize();
 *
 * // In update loop
 * coord.Update(diff);
 *
 * // Query state
 * if (coord.GetState() == DungeonState::CLEARING_TRASH)
 * {
 *     TrashPack* pack = coord.GetCurrentPullTarget();
 *     if (pack && coord.IsSafeToPull())
 *     {
 *         coord.AssignCCTargets(*pack);
 *         // Execute pull
 *     }
 * }
 * @endcode
 */
class TC_GAME_API DungeonCoordinator : public ICombatEventSubscriber
{
public:
    explicit DungeonCoordinator(Group* group);
    ~DungeonCoordinator();

    // Prevent copying
    DungeonCoordinator(const DungeonCoordinator&) = delete;
    DungeonCoordinator& operator=(const DungeonCoordinator&) = delete;

    // ====================================================================
    // LIFECYCLE
    // ====================================================================

    /**
     * @brief Initialize the coordinator
     * Loads dungeon data, detects roles, subscribes to events
     */
    void Initialize();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Update coordination logic
     * @param diff Time since last update in milliseconds
     */
    void Update(uint32 diff);

    // ====================================================================
    // STATE MANAGEMENT
    // ====================================================================

    /**
     * @brief Get current dungeon state
     */
    [[nodiscard]] DungeonState GetState() const { return _state; }

    /**
     * @brief Set dungeon state (triggers transitions)
     */
    void SetState(DungeonState newState);

    /**
     * @brief Check if in a dungeon
     */
    [[nodiscard]] bool IsInDungeon() const { return _state != DungeonState::IDLE; }

    /**
     * @brief Get the group this coordinator manages
     */
    [[nodiscard]] Group* GetGroup() const { return _group; }

    /**
     * @brief Check if in combat (trash or boss)
     */
    [[nodiscard]] bool IsInCombat() const;

    // ====================================================================
    // PROGRESS TRACKING
    // ====================================================================

    /**
     * @brief Get dungeon progress
     */
    [[nodiscard]] const DungeonProgress& GetProgress() const { return _progress; }

    /**
     * @brief Get completion percentage
     */
    [[nodiscard]] float GetCompletionPercent() const;

    /**
     * @brief Get dungeon ID
     */
    [[nodiscard]] uint32 GetDungeonId() const { return _progress.dungeonId; }

    /**
     * @brief Get difficulty
     */
    [[nodiscard]] DungeonDifficulty GetDifficulty() const { return _progress.difficulty; }

    // ====================================================================
    // TRASH MANAGEMENT
    // ====================================================================

    /**
     * @brief Get current pull target
     * @return Pointer to next trash pack or nullptr
     */
    [[nodiscard]] TrashPack* GetCurrentPullTarget() const;

    /**
     * @brief Mark a pack for pulling
     * @param packId Pack to pull
     */
    void MarkPackForPull(uint32 packId);

    /**
     * @brief Assign CC targets for a pack
     * @param pack Pack to assign CC for
     */
    void AssignCCTargets(const TrashPack& pack);

    /**
     * @brief Check if safe to pull
     * @return true if group is ready and no combat
     */
    [[nodiscard]] bool IsSafeToPull() const;

    /**
     * @brief Called when trash pack is cleared
     * @param packId Cleared pack ID
     */
    void OnTrashPackCleared(uint32 packId);

    /**
     * @brief Get all registered trash packs
     */
    [[nodiscard]] ::std::vector<TrashPack> GetAllTrashPacks() const;

    // ====================================================================
    // BOSS MANAGEMENT
    // ====================================================================

    /**
     * @brief Get current boss info
     * @return Pointer to boss info or nullptr
     */
    [[nodiscard]] BossInfo* GetCurrentBoss() const;

    /**
     * @brief Check if in boss encounter
     */
    [[nodiscard]] bool IsInBossEncounter() const { return _state == DungeonState::BOSS_COMBAT; }

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
     * @brief Called when boss wipes the group
     * @param bossId Boss creature entry
     */
    void OnBossWipe(uint32 bossId);

    /**
     * @brief Get current boss phase
     */
    [[nodiscard]] uint8 GetBossPhase() const;

    /**
     * @brief Get all bosses in this dungeon
     */
    [[nodiscard]] ::std::vector<BossInfo> GetAllBosses() const;

    // ====================================================================
    // WIPE RECOVERY
    // ====================================================================

    /**
     * @brief Called when group wipes
     */
    void OnGroupWipe();

    /**
     * @brief Check if recovering from wipe
     */
    [[nodiscard]] bool IsRecovering() const { return _state == DungeonState::WIPED; }

    /**
     * @brief Get next player to resurrect
     * @return GUID of next rez target
     */
    [[nodiscard]] ObjectGuid GetNextRezTarget() const;

    /**
     * @brief Called when player is resurrected
     * @param playerGuid Rezzed player
     */
    void OnPlayerRezzed(ObjectGuid playerGuid);

    /**
     * @brief Check if all players are alive
     */
    [[nodiscard]] bool IsGroupAlive() const;

    // ====================================================================
    // ROLE MANAGEMENT
    // ====================================================================

    /**
     * @brief Get main tank GUID
     */
    [[nodiscard]] ObjectGuid GetMainTank() const { return _mainTank; }

    /**
     * @brief Get off tank GUID
     */
    [[nodiscard]] ObjectGuid GetOffTank() const { return _offTank; }

    /**
     * @brief Get healer GUIDs
     */
    [[nodiscard]] const ::std::vector<ObjectGuid>& GetHealers() const { return _healers; }

    /**
     * @brief Get DPS GUIDs
     */
    [[nodiscard]] const ::std::vector<ObjectGuid>& GetDPS() const { return _dps; }

    /**
     * @brief Detect and assign roles
     */
    void DetectRoles();

    // ====================================================================
    // MYTHIC+ SPECIFIC
    // ====================================================================

    /**
     * @brief Check if Mythic+ dungeon
     */
    [[nodiscard]] bool IsMythicPlus() const { return _progress.isMythicPlus; }

    /**
     * @brief Get keystone level
     */
    [[nodiscard]] uint8 GetKeystoneLevel() const { return _progress.keystoneLevel; }

    /**
     * @brief Get remaining time
     */
    [[nodiscard]] uint32 GetRemainingTime() const;

    /**
     * @brief Check if should skip a pack
     * @param pack Pack to evaluate
     * @return true if pack can be skipped
     */
    [[nodiscard]] bool ShouldSkipPack(const TrashPack& pack) const;

    /**
     * @brief Get enemy forces percentage
     */
    [[nodiscard]] float GetEnemyForcesPercent() const { return _progress.enemyForcesPercent; }

    // ====================================================================
    // GROUP STATUS
    // ====================================================================

    /**
     * @brief Calculate average group health percentage
     */
    [[nodiscard]] float CalculateGroupHealth() const;

    /**
     * @brief Calculate average group mana percentage
     */
    [[nodiscard]] float CalculateGroupMana() const;

    /**
     * @brief Check if group is ready for combat
     */
    [[nodiscard]] bool IsGroupReady() const;

    // ====================================================================
    // SUB-MANAGER ACCESS
    // ====================================================================

    /**
     * @brief Get trash pull manager
     */
    [[nodiscard]] TrashPullManager* GetTrashManager() const { return _trashManager.get(); }

    /**
     * @brief Get boss encounter manager
     */
    [[nodiscard]] BossEncounterManager* GetBossManager() const { return _bossManager.get(); }

    /**
     * @brief Get wipe recovery manager
     */
    [[nodiscard]] WipeRecoveryManager* GetWipeManager() const { return _wipeManager.get(); }

    /**
     * @brief Get Mythic+ manager
     */
    [[nodiscard]] MythicPlusManager* GetMythicPlusManager() const { return _mythicPlusManager.get(); }

    // ====================================================================
    // ICombatEventSubscriber Interface
    // ====================================================================

    /**
     * @brief Handle combat events
     */
    void OnCombatEvent(const CombatEvent& event) override;

    /**
     * @brief Get subscribed event types
     * Subscribes to: UNIT_DIED, COMBAT_STARTED, COMBAT_ENDED,
     *                ENCOUNTER_START, ENCOUNTER_END, BOSS_PHASE_CHANGED
     */
    [[nodiscard]] CombatEventType GetSubscribedEventTypes() const override;

    /**
     * @brief Event priority (45 - below threat managers)
     */
    [[nodiscard]] int32 GetEventPriority() const override { return 45; }

    /**
     * @brief Filter events to dungeon context
     */
    [[nodiscard]] bool ShouldReceiveEvent(const CombatEvent& event) const;

    /**
     * @brief Subscriber name
     */
    [[nodiscard]] const char* GetSubscriberName() const override { return "DungeonCoordinator"; }

private:
    // ====================================================================
    // STATE
    // ====================================================================

    DungeonState _state = DungeonState::IDLE;
    DungeonProgress _progress;

    // ====================================================================
    // REFERENCES
    // ====================================================================

    Group* _group;
    Map* _dungeonMap = nullptr;

    // ====================================================================
    // ROLES
    // ====================================================================

    ObjectGuid _mainTank;
    ObjectGuid _offTank;
    ::std::vector<ObjectGuid> _healers;
    ::std::vector<ObjectGuid> _dps;

    // ====================================================================
    // SUB-MANAGERS
    // ====================================================================

    ::std::unique_ptr<TrashPullManager> _trashManager;
    ::std::unique_ptr<BossEncounterManager> _bossManager;
    ::std::unique_ptr<WipeRecoveryManager> _wipeManager;
    ::std::unique_ptr<MythicPlusManager> _mythicPlusManager;

    // ====================================================================
    // STATE MACHINE
    // ====================================================================

    void UpdateState(uint32 diff);
    void TransitionTo(DungeonState newState);
    void OnStateEnter(DungeonState state);
    void OnStateExit(DungeonState state);

    // ====================================================================
    // EVENT HANDLERS
    // ====================================================================

    void HandleUnitDied(const CombatEvent& event);
    void HandleCombatStarted(const CombatEvent& event);
    void HandleCombatEnded(const CombatEvent& event);
    void HandleEncounterStart(const CombatEvent& event);
    void HandleEncounterEnd(const CombatEvent& event);
    void HandleBossPhaseChanged(const CombatEvent& event);

    // ====================================================================
    // HELPERS
    // ====================================================================

    void LoadDungeonData(uint32 dungeonId);
    bool IsGroupInCombat() const;
    Player* GetGroupLeader() const;
    ::std::vector<Player*> GetGroupMembers() const;
    bool IsPlayerInGroup(ObjectGuid guid) const;

    // ====================================================================
    // EVENT SUBSCRIPTION
    // ====================================================================

    ::std::atomic<bool> _subscribed{false};

    // ====================================================================
    // TIMERS
    // ====================================================================

    uint32 _updateTimer = 0;
    uint32 _stateTimer = 0;
    static constexpr uint32 UPDATE_INTERVAL_MS = 500;   // Update every 500ms
    static constexpr uint32 READY_CHECK_TIMEOUT_MS = 30000;  // 30 second ready check
    static constexpr float MIN_HEALTH_FOR_PULL = 70.0f;  // 70% health minimum
    static constexpr float MIN_MANA_FOR_PULL = 50.0f;    // 50% mana minimum
};

} // namespace Playerbot
