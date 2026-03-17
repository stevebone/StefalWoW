/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BossEncounterManager.h"
#include "DungeonCoordinator.h"
#include "AI/Coordination/Messaging/BotMessageBus.h"
#include "AI/Coordination/Messaging/BotMessage.h"
#include "Player.h"
#include "Creature.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "GameTime.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot {

BossEncounterManager::BossEncounterManager(DungeonCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

void BossEncounterManager::Initialize(uint32 dungeonId)
{
    Reset();
    LoadBossStrategies(dungeonId);

    TC_LOG_DEBUG("playerbot", "BossEncounterManager::Initialize - Initialized for dungeon %u, %zu strategies loaded",
                 dungeonId, _strategies.size());
}

void BossEncounterManager::Update(uint32 diff)
{
    if (!IsInEncounter())
        return;

    // Update mechanic timer
    if (_activeMechanic != BossMechanic::NONE)
    {
        _mechanicTimer += diff;
        if (_mechanicTimer >= MECHANIC_DURATION_MS)
        {
            _activeMechanic = BossMechanic::NONE;
            _mechanicTimer = 0;
        }
    }

    // Check for phase transitions
    DetectPhaseTransition(_bossHealthPercent);

    // Check for tank swap needs
    if (const BossStrategy* strategy = GetCurrentStrategy())
    {
        if (strategy->requiresTankSwap)
        {
            // Check tank debuff stacks
            ObjectGuid activeTank = _currentTank;
            if (!activeTank.IsEmpty())
            {
                uint8 stacks = GetTankSwapStacks(activeTank);
                if (stacks >= strategy->tankSwapStacks)
                {
                    _tankSwapPending = true;
                }
            }
        }
    }
}

void BossEncounterManager::Reset()
{
    _currentBossId = 0;
    _currentPhase = 0;
    _inPhaseTransition = false;
    _activeMechanic = BossMechanic::NONE;
    _encounterStartTime = 0;
    _bloodlustUsed = false;
    _bossHealthPercent = 100.0f;
    _tankSwapStacks.clear();
    _currentTank = ObjectGuid::Empty;
    _tankSwapPending = false;
    _mechanicTimer = 0;
}

// ============================================================================
// ENCOUNTER LIFECYCLE
// ============================================================================

void BossEncounterManager::OnBossEngaged(uint32 bossId)
{
    Reset();

    _currentBossId = bossId;
    _currentPhase = 1;
    _encounterStartTime = GameTime::GetGameTimeMS();

    // Set initial tank
    _currentTank = _coordinator->GetMainTank();

    // Update boss info
    if (_bosses.find(bossId) != _bosses.end())
    {
        _bosses[bossId].combatStartTime = _encounterStartTime;
        _bosses[bossId].currentPhase = _currentPhase;
    }

    TC_LOG_DEBUG("playerbot", "BossEncounterManager::OnBossEngaged - Boss %u engaged", bossId);
}

void BossEncounterManager::OnBossDefeated(uint32 bossId)
{
    TC_LOG_DEBUG("playerbot", "BossEncounterManager::OnBossDefeated - Boss %u defeated in %u ms",
                 bossId, GetEncounterDuration());

    if (_bosses.find(bossId) != _bosses.end())
    {
        _bosses[bossId].healthPercent = 0.0f;
    }

    Reset();
}

void BossEncounterManager::OnBossWipe(uint32 bossId)
{
    TC_LOG_DEBUG("playerbot", "BossEncounterManager::OnBossWipe - Boss %u wipe at %.1f%% after %u ms",
                 bossId, _bossHealthPercent, GetEncounterDuration());

    Reset();
}

void BossEncounterManager::OnPhaseChanged(uint8 newPhase)
{
    if (_currentPhase == newPhase)
        return;

    TC_LOG_DEBUG("playerbot", "BossEncounterManager::OnPhaseChanged - Phase %u -> %u", _currentPhase, newPhase);

    _currentPhase = newPhase;
    _inPhaseTransition = true;

    // Clear active mechanic on phase change
    _activeMechanic = BossMechanic::NONE;
    _mechanicTimer = 0;

    // Update boss info
    if (_bosses.find(_currentBossId) != _bosses.end())
    {
        _bosses[_currentBossId].currentPhase = newPhase;
    }

    // Phase transition is brief
    // Will be cleared on next update or mechanic
}

// ============================================================================
// STRATEGY ACCESS
// ============================================================================

const BossStrategy* BossEncounterManager::GetCurrentStrategy() const
{
    auto it = _strategies.find(_currentBossId);
    return it != _strategies.end() ? &it->second : nullptr;
}

const BossStrategy* BossEncounterManager::GetStrategy(uint32 bossId) const
{
    auto it = _strategies.find(bossId);
    return it != _strategies.end() ? &it->second : nullptr;
}

void BossEncounterManager::LoadBossStrategies(uint32 dungeonId)
{
    // Boss-specific strategies require per-encounter scripting (void zones, phase transitions, etc.).
    // Default strategy: tank-and-spank with interrupt rotation and basic positioning.
    // Custom strategies can be added via BossStrategy subclasses per dungeon/raid encounter.

    TC_LOG_DEBUG("playerbot", "BossEncounterManager::LoadBossStrategies - Loading strategies for dungeon %u", dungeonId);
}

void BossEncounterManager::RegisterStrategy(const BossStrategy& strategy)
{
    _strategies[strategy.bossId] = strategy;

    // Create boss info entry
    BossInfo info;
    info.bossId = strategy.bossId;
    info.name = strategy.name;
    info.maxPhases = strategy.totalPhases;
    info.hasEnrageTimer = strategy.hasEnrage;
    info.enrageTimeMs = strategy.enrageTimeMs;
    _bosses[strategy.bossId] = info;

    TC_LOG_DEBUG("playerbot", "BossEncounterManager::RegisterStrategy - Registered strategy for boss %u (%s)",
                 strategy.bossId, strategy.name.c_str());
}

::std::vector<BossInfo> BossEncounterManager::GetAllBosses() const
{
    ::std::vector<BossInfo> result;
    for (const auto& [id, info] : _bosses)
    {
        result.push_back(info);
    }
    return result;
}

// ============================================================================
// PHASE TRACKING
// ============================================================================

float BossEncounterManager::GetPhaseProgress() const
{
    const BossStrategy* strategy = GetCurrentStrategy();
    if (!strategy || strategy->phaseTransitionHealth.empty())
        return 0.0f;

    // Calculate progress within current phase
    float phaseStart = 100.0f;
    float phaseEnd = 0.0f;

    if (_currentPhase > 0 && _currentPhase <= strategy->phaseTransitionHealth.size())
    {
        phaseStart = _currentPhase == 1 ? 100.0f : strategy->phaseTransitionHealth[_currentPhase - 2];
        phaseEnd = strategy->phaseTransitionHealth[_currentPhase - 1];
    }

    float range = phaseStart - phaseEnd;
    if (range <= 0)
        return 1.0f;

    float progress = (phaseStart - _bossHealthPercent) / range;
    return ::std::clamp(progress, 0.0f, 1.0f);
}

void BossEncounterManager::DetectPhaseTransition(float healthPercent)
{
    const BossStrategy* strategy = GetCurrentStrategy();
    if (!strategy)
        return;

    // Check for phase transition based on health thresholds
    for (size_t i = 0; i < strategy->phaseTransitionHealth.size(); ++i)
    {
        float threshold = strategy->phaseTransitionHealth[i];
        uint8 phase = static_cast<uint8>(i + 2);  // Phase 2, 3, etc.

        if (healthPercent <= threshold && _currentPhase < phase)
        {
            OnPhaseChanged(phase);
            break;
        }
    }

    _inPhaseTransition = false;
}

// ============================================================================
// MECHANIC HANDLING
// ============================================================================

void BossEncounterManager::OnMechanicTriggered(uint32 spellId)
{
    const BossStrategy* strategy = GetCurrentStrategy();
    if (!strategy)
        return;

    // Find mechanic for this spell
    for (const MechanicTrigger& trigger : strategy->mechanics)
    {
        if (trigger.spellId == spellId && trigger.MatchesPhase(_currentPhase))
        {
            _activeMechanic = trigger.mechanic;
            _mechanicTimer = 0;

            TC_LOG_DEBUG("playerbot", "BossEncounterManager::OnMechanicTriggered - Mechanic %s triggered",
                         BossMechanicToString(trigger.mechanic));

            // Handle specific mechanics
            switch (trigger.mechanic)
            {
                case BossMechanic::TANK_SWAP:
                    HandleTankSwapMechanic(trigger);
                    break;
                case BossMechanic::SPREAD:
                    HandleSpreadMechanic(trigger);
                    break;
                case BossMechanic::STACK:
                    HandleStackMechanic(trigger);
                    break;
                case BossMechanic::DODGE_AOE:
                    HandleDodgeMechanic(trigger);
                    break;
                case BossMechanic::INTERRUPT:
                    HandleInterruptMechanic(trigger);
                    break;
                default:
                    break;
            }

            break;
        }
    }
}

bool BossEncounterManager::ShouldSpread() const
{
    if (_activeMechanic == BossMechanic::SPREAD)
        return true;

    const BossStrategy* strategy = GetCurrentStrategy();
    if (!strategy)
        return false;

    if (_currentPhase > 0 && _currentPhase <= 5)
    {
        return strategy->spreadInPhase[_currentPhase - 1];
    }

    return false;
}

bool BossEncounterManager::ShouldStack() const
{
    return _activeMechanic == BossMechanic::STACK;
}

ObjectGuid BossEncounterManager::GetStackTarget() const
{
    // Stack on active tank by default
    if (!_currentTank.IsEmpty())
        return _currentTank;

    return _coordinator->GetMainTank();
}

float BossEncounterManager::GetSpreadDistance() const
{
    const BossStrategy* strategy = GetCurrentStrategy();
    return strategy ? strategy->spreadDistance : 5.0f;
}

// ============================================================================
// TANK SWAP
// ============================================================================

bool BossEncounterManager::NeedsTankSwap() const
{
    return _tankSwapPending;
}

void BossEncounterManager::OnTankSwapCompleted()
{
    // Swap current tank to off-tank
    if (_currentTank == _coordinator->GetMainTank())
        _currentTank = _coordinator->GetOffTank();
    else
        _currentTank = _coordinator->GetMainTank();

    _tankSwapPending = false;

    TC_LOG_DEBUG("playerbot", "BossEncounterManager::OnTankSwapCompleted - Tank swap complete");
}

uint8 BossEncounterManager::GetTankSwapStacks(ObjectGuid tank) const
{
    auto it = _tankSwapStacks.find(tank);
    return it != _tankSwapStacks.end() ? it->second : 0;
}

void BossEncounterManager::UpdateTankStacks(ObjectGuid tank, uint8 stacks)
{
    _tankSwapStacks[tank] = stacks;

    // Check if swap needed
    const BossStrategy* strategy = GetCurrentStrategy();
    if (strategy && strategy->requiresTankSwap && stacks >= strategy->tankSwapStacks)
    {
        _tankSwapPending = true;
    }
}

// ============================================================================
// INTERRUPTS
// ============================================================================

bool BossEncounterManager::ShouldInterrupt(uint32 spellId) const
{
    return GetInterruptPriority(spellId) > 0;
}

uint8 BossEncounterManager::GetInterruptPriority(uint32 spellId) const
{
    const BossStrategy* strategy = GetCurrentStrategy();
    if (!strategy)
        return 0;

    // Must interrupt = priority 2
    for (uint32 id : strategy->mustInterruptSpells)
    {
        if (id == spellId)
            return 2;
    }

    // Should interrupt = priority 1
    for (uint32 id : strategy->shouldInterruptSpells)
    {
        if (id == spellId)
            return 1;
    }

    return 0;
}

// ============================================================================
// BLOODLUST
// ============================================================================

bool BossEncounterManager::ShouldUseBloodlust() const
{
    if (_bloodlustUsed)
        return false;

    const BossStrategy* strategy = GetCurrentStrategy();
    if (!strategy)
        return false;

    // Use on pull if configured
    if (strategy->useBloodlustOnPull && GetEncounterDuration() < 5000)
        return true;

    // Use at health threshold
    if (_bossHealthPercent <= strategy->bloodlustHealthPercent)
        return true;

    return false;
}

void BossEncounterManager::OnBloodlustUsed()
{
    _bloodlustUsed = true;
    TC_LOG_DEBUG("playerbot", "BossEncounterManager::OnBloodlustUsed - Bloodlust used at %.1f%% boss health",
                 _bossHealthPercent);
}

// ============================================================================
// COMBAT STATS
// ============================================================================

uint32 BossEncounterManager::GetEncounterDuration() const
{
    if (_encounterStartTime == 0)
        return 0;

    return GameTime::GetGameTimeMS() - _encounterStartTime;
}

float BossEncounterManager::GetBossHealthPercent() const
{
    return _bossHealthPercent;
}

void BossEncounterManager::SetBossHealthPercent(float percent)
{
    _bossHealthPercent = percent;

    // Update boss info
    if (_bosses.find(_currentBossId) != _bosses.end())
    {
        _bosses[_currentBossId].healthPercent = percent;
    }
}

bool BossEncounterManager::IsEnraging() const
{
    const BossStrategy* strategy = GetCurrentStrategy();
    if (!strategy || !strategy->hasEnrage)
        return false;

    uint32 duration = GetEncounterDuration();
    return duration >= strategy->enrageTimeMs;
}

uint32 BossEncounterManager::GetTimeToEnrage() const
{
    const BossStrategy* strategy = GetCurrentStrategy();
    if (!strategy || !strategy->hasEnrage)
        return 0;

    uint32 duration = GetEncounterDuration();
    if (duration >= strategy->enrageTimeMs)
        return 0;

    return strategy->enrageTimeMs - duration;
}

BossInfo* BossEncounterManager::GetCurrentBoss()
{
    auto it = _bosses.find(_currentBossId);
    return it != _bosses.end() ? &it->second : nullptr;
}

// ============================================================================
// MECHANIC HANDLERS
// ============================================================================

void BossEncounterManager::HandleTankSwapMechanic(const MechanicTrigger& trigger)
{
    _tankSwapPending = true;
}

void BossEncounterManager::HandleSpreadMechanic(const MechanicTrigger& trigger)
{
    TC_LOG_DEBUG("playerbot", "BossEncounterManager: Spread mechanic - spread to %.1f yards",
                 GetSpreadDistance());

    // Broadcast SPREAD command via BotMessageBus
    if (_coordinator && _coordinator->GetGroup())
    {
        Group* group = _coordinator->GetGroup();
        ObjectGuid groupGuid = group->GetGUID();
        ObjectGuid leaderGuid = group->GetLeaderGUID();
        BotMessage msg = BotMessage::CommandSpread(leaderGuid, groupGuid);
        sBotMessageBus->Publish(msg);
    }
}

void BossEncounterManager::HandleStackMechanic(const MechanicTrigger& trigger)
{
    TC_LOG_DEBUG("playerbot", "BossEncounterManager: Stack mechanic triggered");

    // Broadcast STACK command via BotMessageBus
    if (_coordinator && _coordinator->GetGroup())
    {
        Group* group = _coordinator->GetGroup();
        ObjectGuid groupGuid = group->GetGUID();
        ObjectGuid leaderGuid = group->GetLeaderGUID();
        ObjectGuid stackOn = GetStackTarget();

        // Get position of the stack target
        Player* stackPlayer = ObjectAccessor::FindPlayer(stackOn);
        Position stackPos = stackPlayer ? stackPlayer->GetPosition() : Position();
        BotMessage msg = BotMessage::CommandStack(leaderGuid, groupGuid, stackPos);
        sBotMessageBus->Publish(msg);
    }
}

void BossEncounterManager::HandleDodgeMechanic(const MechanicTrigger& trigger)
{
    TC_LOG_DEBUG("playerbot", "BossEncounterManager: Dodge mechanic triggered");
}

void BossEncounterManager::HandleInterruptMechanic(const MechanicTrigger& trigger)
{
    TC_LOG_DEBUG("playerbot", "BossEncounterManager: Interrupt required for spell %u", trigger.spellId);
}

} // namespace Playerbot
