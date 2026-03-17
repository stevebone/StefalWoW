/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 */

#include "RaidEncounterManager.h"
#include "RaidCoordinator.h"
#include "RaidTankCoordinator.h"
#include "RaidCooldownRotation.h"
#include "RaidPositioningManager.h"
#include "Player.h"
#include "Log.h"

namespace Playerbot {

RaidEncounterManager::RaidEncounterManager(RaidCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

void RaidEncounterManager::Initialize()
{
    Reset();
    TC_LOG_DEBUG("playerbots.raid", "RaidEncounterManager::Initialize - Initialized");
}

void RaidEncounterManager::Update(uint32 diff)
{
    if (!_inEncounter)
        return;

    _encounterStartTime += diff;

    // Check for phase transition based on boss health
    CheckPhaseTransition();
}

void RaidEncounterManager::Reset()
{
    _currentEncounterInfo = RaidEncounterInfo();
    _currentBoss.Clear();
    _inEncounter = false;
    _currentPhase = EncounterPhase::PHASE_1;
    _encounterStartTime = 0;
    _phaseStartTime = 0;
    _activeMechanics.clear();
}

void RaidEncounterManager::OnEncounterStart(uint32 encounterId)
{
    LoadEncounter(encounterId);

    _inEncounter = true;
    _currentPhase = EncounterPhase::PHASE_1;
    _encounterStartTime = 0;
    _phaseStartTime = 0;

    TC_LOG_DEBUG("playerbots.raid", "RaidEncounterManager::OnEncounterStart - Encounter %u started", encounterId);
}

void RaidEncounterManager::OnEncounterEnd(bool success)
{
    TC_LOG_DEBUG("playerbots.raid", "RaidEncounterManager::OnEncounterEnd - Encounter ended, success: %s",
        success ? "true" : "false");

    _inEncounter = false;
}

const RaidEncounterInfo* RaidEncounterManager::GetCurrentEncounter() const
{
    return _inEncounter ? &_currentEncounterInfo : nullptr;
}

void RaidEncounterManager::OnPhaseChange(uint8 newPhase)
{
    EncounterPhase phase = static_cast<EncounterPhase>(newPhase);

    TC_LOG_DEBUG("playerbots.raid", "RaidEncounterManager::OnPhaseChange - Phase %u -> %u",
        static_cast<uint8>(_currentPhase), newPhase);

    _currentPhase = phase;
    _phaseStartTime = 0;

    // Notify coordinator
    if (_coordinator->GetCooldownRotation())
    {
        _coordinator->GetCooldownRotation()->OnPhaseChange(newPhase);
    }
}

float RaidEncounterManager::GetPhaseHealthThreshold(uint8 phase) const
{
    auto it = _currentEncounterInfo.phaseHealthThresholds.find(static_cast<EncounterPhase>(phase));
    if (it != _currentEncounterInfo.phaseHealthThresholds.end())
        return it->second;
    return 0.0f;
}

void RaidEncounterManager::OnMechanicTriggered(uint32 spellId)
{
    const EncounterMechanic* mechanic = GetMechanic(spellId);
    if (mechanic)
    {
        ProcessMechanicTrigger(*mechanic);
    }
}

void RaidEncounterManager::RegisterMechanic(const EncounterMechanic& mechanic)
{
    _activeMechanics.push_back(mechanic);
}

const EncounterMechanic* RaidEncounterManager::GetMechanic(uint32 spellId) const
{
    for (const auto& mechanic : _activeMechanics)
    {
        if (mechanic.spellId == spellId)
            return &mechanic;
    }

    for (const auto& mechanic : _currentEncounterInfo.mechanics)
    {
        if (mechanic.spellId == spellId)
            return &mechanic;
    }

    return nullptr;
}

std::vector<EncounterMechanic> RaidEncounterManager::GetMechanicsForPhase(EncounterPhase phase) const
{
    std::vector<EncounterMechanic> mechanics;

    for (const auto& mechanic : _currentEncounterInfo.mechanics)
    {
        if (mechanic.phase == phase)
            mechanics.push_back(mechanic);
    }

    return mechanics;
}

void RaidEncounterManager::OnSpellEvent(const CombatEvent& event)
{
    if (event.type != CombatEventType::SPELL_CAST_SUCCESS)
        return;

    // Check for registered mechanics
    OnMechanicTriggered(event.spellId);
}

void RaidEncounterManager::OnAuraEvent(const CombatEvent& event)
{
    // Check for tank swap triggers
    for (const auto& trigger : _currentEncounterInfo.swapTriggers)
    {
        if (event.spellId == trigger.debuffSpellId)
        {
            RaidTankCoordinator* tankCoord = _coordinator->GetTankCoordinator();
            if (tankCoord)
            {
                if (event.type == CombatEventType::AURA_APPLIED)
                {
                    tankCoord->OnSwapDebuffApplied(event.target, event.spellId, 1);
                }
            }
        }
    }
}

uint32 RaidEncounterManager::GetEnrageTimer() const
{
    return _currentEncounterInfo.enrageTimer;
}

uint32 RaidEncounterManager::GetTimeToEnrage() const
{
    if (_currentEncounterInfo.enrageTimer == 0)
        return 0;

    if (_encounterStartTime >= _currentEncounterInfo.enrageTimer)
        return 0;

    return _currentEncounterInfo.enrageTimer - _encounterStartTime;
}

bool RaidEncounterManager::IsEnrageImminent() const
{
    uint32 timeToEnrage = GetTimeToEnrage();
    return timeToEnrage > 0 && timeToEnrage <= 30000; // 30 seconds
}

float RaidEncounterManager::GetBossHealthPercent() const
{
    // Would get actual boss health from world state
    return 100.0f; // Placeholder
}

bool RaidEncounterManager::ShouldUseBloodlustNow() const
{
    // Use bloodlust based on strategy
    float bossHealth = GetBossHealthPercent();

    // Final phase bloodlust
    if (_currentPhase == EncounterPhase::HARD_ENRAGE)
        return true;

    // Low health bloodlust
    if (bossHealth <= 30.0f)
        return true;

    // Enrage timer bloodlust
    if (IsEnrageImminent())
        return true;

    return false;
}

bool RaidEncounterManager::ShouldSaveDefensives() const
{
    // Save defensives for known damage phases
    auto phaseMechanics = GetMechanicsForPhase(_currentPhase);
    for (const auto& mechanic : phaseMechanics)
    {
        if (mechanic.type == MechanicType::TANK_SWAP || mechanic.type == MechanicType::SOAK)
            return true;
    }
    return false;
}

bool RaidEncounterManager::IsPhaseTransitionImminent() const
{
    float bossHealth = GetBossHealthPercent();
    uint8 nextPhase = static_cast<uint8>(_currentPhase) + 1;
    float threshold = GetPhaseHealthThreshold(nextPhase);

    return threshold > 0 && bossHealth <= threshold + 5.0f;
}

void RaidEncounterManager::LoadEncounter(uint32 encounterId)
{
    // Check encounter database
    auto it = _encounterDatabase.find(encounterId);
    if (it != _encounterDatabase.end())
    {
        _currentEncounterInfo = it->second;
        return;
    }

    // Create default encounter info
    _currentEncounterInfo = RaidEncounterInfo();
    _currentEncounterInfo.encounterId = encounterId;
    _currentEncounterInfo.totalPhases = 3;
    _currentEncounterInfo.enrageTimer = 480000; // 8 minutes default
}

void RaidEncounterManager::RegisterSwapTrigger(const TankSwapTrigger& trigger)
{
    _currentEncounterInfo.swapTriggers.push_back(trigger);

    // Also register with tank coordinator
    RaidTankCoordinator* tankCoord = _coordinator->GetTankCoordinator();
    if (tankCoord)
    {
        tankCoord->ConfigureSwapTrigger(trigger);
    }
}

void RaidEncounterManager::CheckPhaseTransition()
{
    float bossHealth = GetBossHealthPercent();

    // Check health thresholds for phase transitions
    for (const auto& pair : _currentEncounterInfo.phaseHealthThresholds)
    {
        if (pair.first > _currentPhase && bossHealth <= pair.second)
        {
            OnPhaseChange(static_cast<uint8>(pair.first));
            break;
        }
    }
}

void RaidEncounterManager::ProcessMechanicTrigger(const EncounterMechanic& mechanic)
{
    TC_LOG_DEBUG("playerbots.raid", "RaidEncounterManager::ProcessMechanicTrigger - Mechanic: %s (spell %u)",
        MechanicTypeToString(mechanic.type), mechanic.spellId);

    switch (mechanic.type)
    {
        case MechanicType::TANK_SWAP:
            if (_coordinator->GetTankCoordinator())
                _coordinator->GetTankCoordinator()->CallTankSwap();
            break;

        case MechanicType::SPREAD:
            if (_coordinator->GetPositioningManager())
                _coordinator->GetPositioningManager()->CallSpread(mechanic.radius);
            break;

        case MechanicType::STACK:
            if (_coordinator->GetPositioningManager())
                _coordinator->GetPositioningManager()->CallStack(mechanic.x, mechanic.y, mechanic.z);
            break;

        default:
            break;
    }
}

} // namespace Playerbot
