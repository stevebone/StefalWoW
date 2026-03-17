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

#include "RaidState.h"
#include "Core/Events/CombatEvent.h"
#include "Core/Events/ICombatEventSubscriber.h"
#include <memory>
#include <vector>
#include <map>

class Player;
class Unit;
class Map;

namespace Playerbot {

// Forward declarations for sub-managers
class RaidTankCoordinator;
class RaidHealCoordinator;
class RaidCooldownRotation;
class RaidGroupManager;
class KitingManager;
class AddManagementSystem;
class RaidPositioningManager;
class RaidEncounterManager;

/**
 * @class RaidCoordinator
 * @brief Main orchestrator for raid coordination
 *
 * Manages all aspects of raid coordination including:
 * - Tank assignments and swap automation
 * - Healer assignments to tanks/groups
 * - Raid cooldown rotation (Bloodlust, defensive CDs)
 * - 8 sub-group management with split mechanics
 * - Kiting coordination with waypoint paths
 * - Add management with priority system
 * - Position assignments for mechanics
 * - Boss encounter phase tracking
 *
 * Implements ICombatEventSubscriber to receive combat events.
 */
class RaidCoordinator 
{
public:
    RaidCoordinator(Map* raidInstance, const ::std::vector<Player*>& raidMembers);
    ~RaidCoordinator();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    void Initialize();
    void Shutdown();
    void Update(uint32 diff);

    // ========================================================================
    // COMBAT EVENT INTERFACE
    // ========================================================================

    void OnCombatEvent(const CombatEvent& event);
    CombatEventType GetSubscribedEvents() const;
    int32 GetEventPriority() const { return 50; }  // High priority for raid

    // ========================================================================
    // STATE MANAGEMENT
    // ========================================================================

    RaidState GetState() const { return _state; }
    bool IsInCombat() const { return _state == RaidState::COMBAT; }
    bool IsRecovering() const { return _state == RaidState::RECOVERING; }
    bool IsReady() const { return _state == RaidState::BUFFING; }

    void TransitionToState(RaidState newState);
    void OnRaidWipe();
    void OnEncounterStart(uint32 encounterId);
    void OnEncounterEnd(bool success);

    // ========================================================================
    // RAID ROSTER
    // ========================================================================

    void AddMember(Player* player);
    void RemoveMember(ObjectGuid playerGuid);
    void UpdateMember(ObjectGuid playerGuid);
    bool IsMember(ObjectGuid playerGuid) const;

    const ::std::vector<ObjectGuid>& GetAllMembers() const { return _raidMembers; }
    const ::std::vector<ObjectGuid>& GetTanks() const { return _tanks; }
    const ::std::vector<ObjectGuid>& GetHealers() const { return _healers; }
    const ::std::vector<ObjectGuid>& GetDPS() const { return _dps; }

    uint32 GetMemberCount() const { return static_cast<uint32>(_raidMembers.size()); }
    uint32 GetAliveMemberCount() const;

    Player* GetPlayer(ObjectGuid guid) const;

    // ========================================================================
    // SUB-MANAGER ACCESS
    // ========================================================================

    RaidTankCoordinator* GetTankCoordinator() { return _tankCoordinator.get(); }
    RaidHealCoordinator* GetHealCoordinator() { return _healCoordinator.get(); }
    RaidCooldownRotation* GetCooldownRotation() { return _cooldownRotation.get(); }
    RaidGroupManager* GetGroupManager() { return _groupManager.get(); }
    KitingManager* GetKitingManager() { return _kitingManager.get(); }
    AddManagementSystem* GetAddManager() { return _addManager.get(); }
    RaidPositioningManager* GetPositioningManager() { return _positioningManager.get(); }
    RaidEncounterManager* GetEncounterManager() { return _encounterManager.get(); }

    const RaidTankCoordinator* GetTankCoordinator() const { return _tankCoordinator.get(); }
    const RaidHealCoordinator* GetHealCoordinator() const { return _healCoordinator.get(); }
    const RaidCooldownRotation* GetCooldownRotation() const { return _cooldownRotation.get(); }
    const RaidGroupManager* GetGroupManager() const { return _groupManager.get(); }
    const KitingManager* GetKitingManager() const { return _kitingManager.get(); }
    const AddManagementSystem* GetAddManager() const { return _addManager.get(); }
    const RaidPositioningManager* GetPositioningManager() const { return _positioningManager.get(); }
    const RaidEncounterManager* GetEncounterManager() const { return _encounterManager.get(); }

    // ========================================================================
    // ENCOUNTER INFORMATION
    // ========================================================================

    uint32 GetCurrentEncounterId() const { return _currentEncounterId; }
    const RaidEncounterInfo* GetCurrentEncounter() const;
    RaidDifficulty GetDifficulty() const { return _difficulty; }
    const RaidMatchStats& GetMatchStats() const { return _matchStats; }

    // ========================================================================
    // RAID-WIDE CALLS
    // ========================================================================

    void CallBloodlust();
    void CallRaidDefensive();
    void CallBattleRez(ObjectGuid target);
    void CallReadyCheck();
    void CallPull(uint32 countdown);
    void CallWipe();

    // ========================================================================
    // QUICK ACCESS QUERIES
    // ========================================================================

    ObjectGuid GetMainTank() const;
    ObjectGuid GetOffTank() const;
    ObjectGuid GetKillTarget() const;
    ObjectGuid GetCurrentBossTarget() const;

    bool ShouldUseBloodlustNow() const;
    bool ShouldUseRaidDefensiveNow() const;
    bool HasBattleRezAvailable() const;

    float GetRaidHealthPercent() const;
    float GetRaidManaPercent() const;
    float GetBossHealthPercent() const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    void SetDifficulty(RaidDifficulty difficulty) { _difficulty = difficulty; }
    void SetAutoTankSwap(bool enabled) { _autoTankSwap = enabled; }
    void SetAutoAssignHealers(bool enabled) { _autoAssignHealers = enabled; }

    bool IsAutoTankSwapEnabled() const { return _autoTankSwap; }
    bool IsAutoAssignHealersEnabled() const { return _autoAssignHealers; }

private:
    Map* _raidInstance;

    // ========================================================================
    // STATE
    // ========================================================================

    RaidState _state = RaidState::IDLE;
    RaidDifficulty _difficulty = RaidDifficulty::NORMAL;
    uint32 _currentEncounterId = 0;
    RaidMatchStats _matchStats;

    // ========================================================================
    // ROSTER
    // ========================================================================

    ::std::vector<ObjectGuid> _raidMembers;
    ::std::vector<ObjectGuid> _tanks;
    ::std::vector<ObjectGuid> _healers;
    ::std::vector<ObjectGuid> _dps;
    ::std::map<ObjectGuid, Player*> _playerCache;

    // ========================================================================
    // SUB-MANAGERS
    // ========================================================================

    ::std::unique_ptr<RaidTankCoordinator> _tankCoordinator;
    ::std::unique_ptr<RaidHealCoordinator> _healCoordinator;
    ::std::unique_ptr<RaidCooldownRotation> _cooldownRotation;
    ::std::unique_ptr<RaidGroupManager> _groupManager;
    ::std::unique_ptr<KitingManager> _kitingManager;
    ::std::unique_ptr<AddManagementSystem> _addManager;
    ::std::unique_ptr<RaidPositioningManager> _positioningManager;
    ::std::unique_ptr<RaidEncounterManager> _encounterManager;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    bool _autoTankSwap = true;
    bool _autoAssignHealers = true;
    uint32 _pullCountdown = 5000;       // 5 second pull timer

    // ========================================================================
    // TIMERS
    // ========================================================================

    uint32 _updateInterval = 100;       // 100ms update tick
    uint32 _lastUpdateTime = 0;
    uint32 _combatStartTime = 0;

    // ========================================================================
    // INITIALIZATION
    // ========================================================================

    void CreateSubManagers();
    void DestroySubManagers();
    void CategorizeRoster();
    void RegisterCombatEvents();
    void UnregisterCombatEvents();

    // ========================================================================
    // EVENT HANDLERS
    // ========================================================================

    void HandleDamageEvent(const CombatEvent& event);
    void HandleHealingEvent(const CombatEvent& event);
    void HandleSpellEvent(const CombatEvent& event);
    void HandleAuraEvent(const CombatEvent& event);
    void HandleDeathEvent(const CombatEvent& event);

    // ========================================================================
    // STATE UPDATES
    // ========================================================================

    void UpdateIdle(uint32 diff);
    void UpdateForming(uint32 diff);
    void UpdateBuffing(uint32 diff);
    void UpdatePulling(uint32 diff);
    void UpdateCombat(uint32 diff);
    void UpdatePhaseTransition(uint32 diff);
    void UpdateWiped(uint32 diff);
    void UpdateRecovering(uint32 diff);

    // ========================================================================
    // UTILITY
    // ========================================================================

    bool AreAllMembersReady() const;
    bool AreAllMembersBuffed() const;
    bool AreAllMembersAlive() const;
    void RefreshPlayerCache();
};

} // namespace Playerbot
