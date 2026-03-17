/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 */

#pragma once

#include "RaidState.h"
#include "Core/Events/CombatEvent.h"
#include <vector>
#include <map>

namespace Playerbot {

class RaidCoordinator;

class RaidEncounterManager
{
public:
    RaidEncounterManager(RaidCoordinator* coordinator);

    void Initialize();
    void Update(uint32 diff);
    void Reset();

    // Encounter State
    void OnEncounterStart(uint32 encounterId);
    void OnEncounterEnd(bool success);
    const RaidEncounterInfo* GetCurrentEncounter() const;
    ObjectGuid GetCurrentBossGuid() const { return _currentBoss; }
    bool IsInEncounter() const { return _inEncounter; }

    // Phase Management
    void OnPhaseChange(uint8 newPhase);
    EncounterPhase GetCurrentPhase() const { return _currentPhase; }
    uint8 GetPhaseNumber() const { return static_cast<uint8>(_currentPhase); }
    float GetPhaseHealthThreshold(uint8 phase) const;

    // Mechanics
    void OnMechanicTriggered(uint32 spellId);
    void RegisterMechanic(const EncounterMechanic& mechanic);
    const EncounterMechanic* GetMechanic(uint32 spellId) const;
    ::std::vector<EncounterMechanic> GetMechanicsForPhase(EncounterPhase phase) const;

    // Event Handlers
    void OnSpellEvent(const CombatEvent& event);
    void OnAuraEvent(const CombatEvent& event);

    // Timers
    uint32 GetEnrageTimer() const;
    uint32 GetTimeToEnrage() const;
    bool IsEnrageImminent() const;
    float GetBossHealthPercent() const;

    // Strategy Recommendations
    bool ShouldUseBloodlustNow() const;
    bool ShouldSaveDefensives() const;
    bool IsPhaseTransitionImminent() const;

    // Encounter Database
    void LoadEncounter(uint32 encounterId);
    void RegisterSwapTrigger(const TankSwapTrigger& trigger);

private:
    RaidCoordinator* _coordinator;

    RaidEncounterInfo _currentEncounterInfo;
    ObjectGuid _currentBoss;
    bool _inEncounter = false;
    EncounterPhase _currentPhase = EncounterPhase::PHASE_1;
    uint32 _encounterStartTime = 0;
    uint32 _phaseStartTime = 0;

    ::std::map<uint32, RaidEncounterInfo> _encounterDatabase;
    ::std::vector<EncounterMechanic> _activeMechanics;

    void CheckPhaseTransition();
    void ProcessMechanicTrigger(const EncounterMechanic& mechanic);
};

} // namespace Playerbot
