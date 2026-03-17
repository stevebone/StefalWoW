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

struct CooldownPlan
{
    uint32 phaseOrTime;
    CooldownType type;
    ObjectGuid assignedPlayer;
    uint32 spellId;
    bool used;
};

class RaidCooldownRotation
{
public:
    RaidCooldownRotation(RaidCoordinator* coordinator);

    void Initialize();
    void Update(uint32 diff);
    void Reset();

    // Bloodlust
    void UseBloodlust();
    bool ShouldUseBloodlust() const;
    bool IsBloodlustAvailable() const;
    ObjectGuid GetBloodlustProvider() const;
    bool HasBloodlustBeenUsed() const { return _bloodlustUsed; }

    // Raid Defensives
    void UseRaidDefensive();
    bool ShouldUseRaidDefensive() const;
    ObjectGuid GetNextRaidDefensiveProvider() const;
    ::std::vector<ObjectGuid> GetAvailableRaidDefensiveProviders() const;

    // Battle Rez
    void UseBattleRez(ObjectGuid target);
    bool HasBattleRezAvailable() const;
    uint32 GetBattleRezCharges() const;
    ObjectGuid GetBattleRezProvider() const;

    // Planned Rotation
    void LoadCooldownPlan(const ::std::vector<CooldownPlan>& plan);
    void ClearCooldownPlan();
    void OnPhaseChange(uint8 phase);

    // Tracking
    void OnSpellEvent(const CombatEvent& event);
    const ::std::vector<RaidCooldownEntry>& GetAllCooldowns() const { return _cooldowns; }

private:
    RaidCoordinator* _coordinator;

    ::std::vector<RaidCooldownEntry> _cooldowns;
    ::std::vector<CooldownPlan> _cooldownPlan;
    bool _bloodlustUsed = false;
    uint32 _battleRezCharges = 1;
    uint32 _maxBattleRezCharges = 1;
    float _bloodlustThreshold = 30.0f;
    float _defensiveThreshold = 50.0f;

    void BuildCooldownList();
    void UpdateCooldowns(uint32 diff);
    RaidCooldownEntry* FindCooldown(ObjectGuid player, CooldownType type);
};

} // namespace Playerbot
