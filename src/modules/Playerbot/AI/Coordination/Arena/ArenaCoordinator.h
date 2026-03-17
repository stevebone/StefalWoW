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

#include "ArenaState.h"
#include "Core/Events/CombatEvent.h"
#include "Core/Events/ICombatEventSubscriber.h"
#include <memory>
#include <vector>

class Battleground;
class Player;

namespace Playerbot {

class KillTargetManager;
class BurstCoordinator;
class CCChainManager;
class DefensiveCoordinator;
class ArenaPositioning;
class CrowdControlManager;  // From Phase 2 (DR tracking)

/**
 * @class ArenaCoordinator
 * @brief Coordinates AI bot behavior in arena PvP matches
 *
 * The ArenaCoordinator manages all aspects of arena combat including:
 * - Kill target selection and switching
 * - Burst window coordination
 * - CC chain management with DR tracking
 * - Defensive coordination and peeling
 * - Arena positioning (pillars, LOS)
 *
 * Subscribes to combat events via ICombatEventSubscriber for reactive decision-making.
 */
class ArenaCoordinator : public ICombatEventSubscriber
{
public:
    ArenaCoordinator(Battleground* arena, ::std::vector<Player*> team);
    ~ArenaCoordinator() override;

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    void Initialize();
    void Shutdown();
    void Update(uint32 diff);
    void Reset();

    // ========================================================================
    // STATE
    // ========================================================================

    ArenaState GetState() const { return _state; }
    ArenaType GetType() const { return _type; }
    ArenaBracket GetBracket() const { return _bracket; }
    bool IsInCombat() const { return _state == ArenaState::COMBAT; }
    bool IsActive() const { return _state >= ArenaState::PREPARATION && _state <= ArenaState::COMBAT; }
    uint32 GetMatchDuration() const;

    // ========================================================================
    // KILL TARGET
    // ========================================================================

    ObjectGuid GetKillTarget() const;
    void SetKillTarget(ObjectGuid target);
    void ClearKillTarget();
    void CallSwitch(ObjectGuid newTarget);
    TargetPriority GetTargetPriority(ObjectGuid target) const;
    bool ShouldSwitch() const;
    ObjectGuid GetRecommendedKillTarget() const;

    // ========================================================================
    // BURST COORDINATION
    // ========================================================================

    bool IsBurstWindowActive() const;
    void CallBurst(ObjectGuid target);
    void CallOffBurst();
    bool ShouldUseCooldowns() const;
    const BurstWindow* GetCurrentBurstWindow() const;
    BurstPhase GetBurstPhase() const;
    bool CanInitiateBurst() const;
    uint32 GetBurstReadyCount() const;

    // ========================================================================
    // CC MANAGEMENT
    // ========================================================================

    void RequestCC(ObjectGuid target, uint32 durationMs, uint8 priority = 1);
    void CallCCChain(ObjectGuid target);
    void EndCCChain();
    bool CanCCTarget(ObjectGuid target) const;
    float GetExpectedCCDuration(ObjectGuid target, uint32 spellId) const;
    uint8 GetDRStacks(ObjectGuid target, uint32 spellId) const;
    bool IsTargetCCImmune(ObjectGuid target) const;
    ObjectGuid GetCCChainTarget() const;
    bool IsCCChainActive() const;

    // ========================================================================
    // DEFENSIVE COORDINATION
    // ========================================================================

    void RequestPeel(ObjectGuid teammate, uint8 urgency = 1);
    void CallDefensives(ObjectGuid target);
    ObjectGuid GetPeelTarget() const;
    bool ShouldUseDefensive(ObjectGuid player) const;
    DefensiveState GetTeammateDefensiveState(ObjectGuid teammate) const;
    bool IsTeammateInTrouble(ObjectGuid teammate) const;
    ObjectGuid GetMostEndangeredTeammate() const;

    // ========================================================================
    // POSITIONING
    // ========================================================================

    void RequestReposition(ObjectGuid player, float x, float y, float z);
    bool ShouldLOS() const;
    float GetPillarDistance(ObjectGuid player) const;
    bool IsInLOSOfHealer(ObjectGuid player) const;
    bool IsInLOSOfKillTarget(ObjectGuid player) const;
    void GetRecommendedPosition(ObjectGuid player, float& x, float& y, float& z) const;

    // ========================================================================
    // ENEMY TRACKING
    // ========================================================================

    const ArenaEnemy* GetEnemy(ObjectGuid guid) const;
    ArenaEnemy* GetEnemyMutable(ObjectGuid guid);
    ::std::vector<ArenaEnemy> GetEnemies() const;
    ::std::vector<ArenaEnemy> GetAliveEnemies() const;
    bool IsEnemyTrinketDown(ObjectGuid enemy) const;
    bool IsEnemyDefensiveDown(ObjectGuid enemy) const;
    bool IsEnemyInCC(ObjectGuid enemy) const;
    const ArenaEnemy* GetEnemyHealer() const;
    uint32 GetAliveEnemyCount() const;

    // ========================================================================
    // TEAMMATE TRACKING
    // ========================================================================

    const ArenaTeammate* GetTeammate(ObjectGuid guid) const;
    ArenaTeammate* GetTeammateMutable(ObjectGuid guid);
    ::std::vector<ArenaTeammate> GetTeammates() const;
    ::std::vector<ArenaTeammate> GetAliveTeammates() const;
    const ArenaTeammate* GetTeamHealer() const;
    uint32 GetAliveTeammateCount() const;
    float GetTeamHealthPercent() const;
    float GetTeamManaPercent() const;

    // ========================================================================
    // MATCH STATISTICS
    // ========================================================================

    const ArenaMatchStats& GetMatchStats() const { return _matchStats; }

    // ========================================================================
    // COMBAT EVENT INTERFACE
    // ========================================================================

    void OnCombatEvent(const CombatEvent& event) override;
    CombatEventType GetSubscribedEventTypes() const override;
    int32 GetEventPriority() const override { return 40; }  // High priority for arena
    const char* GetSubscriberName() const override { return "ArenaCoordinator"; }

    // ========================================================================
    // SUB-MANAGER ACCESS
    // ========================================================================

    KillTargetManager* GetKillTargetManager() const { return _killTargetManager.get(); }
    BurstCoordinator* GetBurstCoordinator() const { return _burstCoordinator.get(); }
    CCChainManager* GetCCChainManager() const { return _ccChainManager.get(); }
    DefensiveCoordinator* GetDefensiveCoordinator() const { return _defensiveCoordinator.get(); }
    ArenaPositioning* GetPositioning() const { return _positioning.get(); }

private:
    // ========================================================================
    // STATE
    // ========================================================================

    ArenaState _state = ArenaState::IDLE;
    ArenaType _type;
    ArenaBracket _bracket;

    // ========================================================================
    // REFERENCES
    // ========================================================================

    Battleground* _arena;
    ::std::vector<Player*> _team;

    // ========================================================================
    // TRACKING
    // ========================================================================

    ::std::vector<ArenaEnemy> _enemies;
    ::std::vector<ArenaTeammate> _teammates;
    ArenaMatchStats _matchStats;

    uint32 _matchStartTime = 0;
    uint32 _gatesOpenTime = 0;

    // ========================================================================
    // CURRENT BURST
    // ========================================================================

    BurstWindow _currentBurst;

    // ========================================================================
    // SUB-MANAGERS
    // ========================================================================

    ::std::unique_ptr<KillTargetManager> _killTargetManager;
    ::std::unique_ptr<BurstCoordinator> _burstCoordinator;
    ::std::unique_ptr<CCChainManager> _ccChainManager;
    ::std::unique_ptr<DefensiveCoordinator> _defensiveCoordinator;
    ::std::unique_ptr<ArenaPositioning> _positioning;
    CrowdControlManager* _ccManager;  // Shared, for DR tracking

    // ========================================================================
    // STATE MACHINE
    // ========================================================================

    void UpdateState(uint32 diff);
    void TransitionTo(ArenaState newState);
    void OnStateEnter(ArenaState state);
    void OnStateExit(ArenaState state);

    // ========================================================================
    // EVENT HANDLERS
    // ========================================================================

    void HandleDamageTaken(const CombatEvent& event);
    void HandleDamageDealt(const CombatEvent& event);
    void HandleHealingDone(const CombatEvent& event);
    void HandleSpellCastStart(const CombatEvent& event);
    void HandleSpellCastSuccess(const CombatEvent& event);
    void HandleSpellInterrupted(const CombatEvent& event);
    void HandleAuraApplied(const CombatEvent& event);
    void HandleAuraRemoved(const CombatEvent& event);
    void HandleUnitDied(const CombatEvent& event);

    // ========================================================================
    // TRACKING UPDATES
    // ========================================================================

    void UpdateEnemyTracking(uint32 diff);
    void UpdateTeammateTracking(uint32 diff);
    void TrackCooldownUsage(ObjectGuid caster, uint32 spellId);
    void TrackTrinketUsage(ObjectGuid player);
    void TrackDefensiveUsage(ObjectGuid player, uint32 spellId);
    void UpdateCooldownTimers(uint32 diff);

    // ========================================================================
    // DECISION MAKING
    // ========================================================================

    ObjectGuid EvaluateBestKillTarget() const;
    bool ShouldCallSwitch() const;
    bool ShouldCallBurst() const;
    bool ShouldDefendTeammate() const;

    // ========================================================================
    // INITIALIZATION
    // ========================================================================

    void InitializeEnemyTracking();
    void InitializeTeammateTracking();
    void DetectArenaType();
    ArenaRole DeterminePlayerRole(Player* player) const;

    // ========================================================================
    // UTILITY
    // ========================================================================

    bool IsEnemy(ObjectGuid guid) const;
    bool IsTeammate(ObjectGuid guid) const;
    Player* GetPlayer(ObjectGuid guid) const;
    TargetPriority CalculateTargetPriority(const ArenaEnemy& enemy) const;

    // ========================================================================
    // COOLDOWN DATABASE
    // ========================================================================

    static bool IsTrinketSpell(uint32 spellId);
    static bool IsDefensiveCooldown(uint32 spellId);
    static bool IsMajorOffensiveCooldown(uint32 spellId);
    static uint32 GetCooldownDuration(uint32 spellId);
};

} // namespace Playerbot
