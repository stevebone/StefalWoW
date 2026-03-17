/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CombatAIIntegrator.h"
// Note: Component headers commented out due to conflicts - components not currently initialized
// #include "RoleBasedCombatPositioning.h"
// #include "InterruptCoordinator.h"
// #include "ThreatCoordinator.h"
// #include "FormationManager.h"
// #include "TargetSelector.h"
// #include "PathfindingManager.h"
// #include "LineOfSightManager.h"
// #include "ObstacleAvoidanceManager.h"
// #include "KitingManager.h"
// #include "InterruptDatabase.h"
// #include "InterruptAwareness.h"
// #include "MechanicAwareness.h"
// #include "ThreatAbilities.h"
#include "ClassAI/ClassAI.h"
// #include "Movement/UnifiedMovementCoordinator.h"
// #include "Movement/Arbiter/MovementPriorityMapper.h"
#include "AI/BotAI.h"
#include "Group.h"
#include "Log.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "GameTime.h"
#include <algorithm>

namespace Playerbot
{

CombatAIIntegrator::CombatAIIntegrator(Player* bot) :
    _bot(bot),
    _classAI(nullptr),
    _currentPhase(CombatPhase::NONE),
    _currentTarget(nullptr),
    _group(nullptr),
    _lastUpdate(0),
    _combatStartTime(0),
    _phaseStartTime(0),
    _lastPositionUpdate(0),
    _lastInterruptCheck(0),
    _lastThreatUpdate(0)
{
    // Note: Component initialization disabled due to constructor signature mismatches
    // Components would need to be updated to support the expected constructor signatures
    // _positioning = ::std::make_unique<RoleBasedCombatPositioning>(bot);
    // _interruptCoordinator = ::std::make_unique<InterruptCoordinator>(bot);
    // _threatCoordinator = ::std::make_unique<ThreatCoordinator>(bot);
    // _formationManager = ::std::make_unique<FormationManager>();
    // _targetSelector = ::std::make_unique<TargetSelector>(bot);
    // _pathfinding = ::std::make_unique<PathfindingManager>(bot);
    // _losManager = ::std::make_unique<LineOfSightManager>(bot);
    // _obstacleAvoidance = ::std::make_unique<ObstacleAvoidanceManager>(bot);
    // _kitingManager = ::std::make_unique<KitingManager>(bot);
    // _interruptDB = ::std::make_unique<InterruptDatabase>();
    // _interruptAwareness = ::std::make_unique<InterruptAwareness>(bot);
    // _mechanicAwareness = ::std::make_unique<MechanicAwareness>(bot);
    // _threatAbilities = ::std::make_unique<ThreatAbilities>(bot);

    // Set default configuration
    _config = CombatAIConfig();

    // CRITICAL: Do NOT access bot->GetName() in constructor!
    // Bot's internal data (m_name) is not initialized during constructor chain.
    // Accessing it causes ACCESS_VIOLATION crash in string construction.
    // Logging deferred to first Update() when bot IsInWorld()
}

CombatAIIntegrator::~CombatAIIntegrator() = default;

IntegrationResult CombatAIIntegrator::Update(uint32 diff)
{
    IntegrationResult result;
    result.success = true;
    result.phase = _currentPhase;
    result.actionsExecuted = 0;
    result.executionTime = ::std::chrono::microseconds{0};

    // Minimal update - just track timing
    _lastUpdate += diff;
    if (_lastUpdate < _config.updateIntervalMs)
    {
        return result;
    }

    _lastUpdate = 0;
    _metrics.updateCount++;

    return result;
}

void CombatAIIntegrator::Reset()
{
    ::std::lock_guard lock(_mutex);

    _inCombat = false;
    _currentPhase = CombatPhase::NONE;
    _currentTarget = nullptr;
    _lastUpdate = 0;
    _combatStartTime = 0;
    _phaseStartTime = 0;

    _metrics.Reset();
    TC_LOG_DEBUG("bot.ai.combat", "CombatAIIntegrator reset for bot {}", _bot->GetName());
}

void CombatAIIntegrator::OnCombatStart(Unit* target)
{
    ::std::lock_guard lock(_mutex);

    _inCombat = true;
    _currentTarget = target;
    _currentPhase = CombatPhase::ENGAGING;
    _combatStartTime = GameTime::GetGameTimeMS();
    _phaseStartTime = _combatStartTime;

    // Notify class AI if registered
    if (_classAI)
        _classAI->OnCombatStart(target);

    TC_LOG_DEBUG("bot.ai.combat", "Combat started for bot {} against {}",
        _bot->GetName(), target->GetName());
}

void CombatAIIntegrator::OnCombatEnd()
{
    ::std::lock_guard lock(_mutex);

    _inCombat = false;
    _currentPhase = CombatPhase::RECOVERING;
    _currentTarget = nullptr;

    // Notify class AI if registered
    if (_classAI)
        _classAI->OnCombatEnd();
    TC_LOG_DEBUG("bot.ai.combat", "Combat ended for bot {}", _bot->GetName());
}

void CombatAIIntegrator::OnTargetChanged(Unit* newTarget)
{
    ::std::lock_guard lock(_mutex);

    Unit* oldTarget = _currentTarget;
    _currentTarget = newTarget;

    // Notify class AI if registered
    if (_classAI)
        _classAI->OnTargetChanged(newTarget);

    TC_LOG_DEBUG("bot.ai.combat", "Target changed for bot {} from {} to {}",
        _bot->GetName(),
        oldTarget ? oldTarget->GetName() : "none",
        newTarget ? newTarget->GetName() : "none");
}

void CombatAIIntegrator::SetGroup(Group* group)
{
    ::std::lock_guard lock(_mutex);
    _group = group;
}

void CombatAIIntegrator::UpdateGroupCoordination()
{
    // Minimal implementation
}

void CombatAIIntegrator::StartMetricCapture()
{
    _metrics.updateCount++;
}

void CombatAIIntegrator::EndMetricCapture(::std::chrono::microseconds elapsed)
{
    // Update CPU metrics
    float cpuPercent = (elapsed.count() / 1000.0f) / _config.updateIntervalMs * 100.0f;
    _metrics.avgCpuPercent = (_metrics.avgCpuPercent * (_metrics.updateCount - 1) + cpuPercent) / _metrics.updateCount;
}

bool CombatAIIntegrator::IsWithinPerformanceLimits() const
{
    return _metrics.avgCpuPercent <= 0.1f && _metrics.memoryUsed <= _config.maxMemoryBytes;
}

void CombatAIIntegrator::ValidateMemoryUsage()
{
    // Minimal implementation
}

void CombatAIIntegrator::CompactMemory()
{
    // Private method for internal use only
}

// Factory implementations
::std::unique_ptr<CombatAIIntegrator> CombatAIFactory::CreateCombatAI(Player* bot)
{
    return ::std::make_unique<CombatAIIntegrator>(bot);
}

::std::unique_ptr<CombatAIIntegrator> CombatAIFactory::CreateCombatAI(Player* bot, CombatAIConfig const& config)
{
    auto integrator = ::std::make_unique<CombatAIIntegrator>(bot);
    integrator->SetConfig(config);
    return integrator;
}

::std::unique_ptr<CombatAIIntegrator> CombatAIFactory::CreateTankCombatAI(Player* bot)
{
    CombatAIConfig config;
    config.enableThreatManagement = true;
    config.threatUpdateThreshold = 5.0f;
    config.positionUpdateThreshold = 3.0f;

    auto integrator = ::std::make_unique<CombatAIIntegrator>(bot);
    integrator->SetConfig(config);
    return integrator;
}

::std::unique_ptr<CombatAIIntegrator> CombatAIFactory::CreateHealerCombatAI(Player* bot)
{
    CombatAIConfig config;
    config.enablePositioning = true;
    config.enableKiting = true;
    config.positionUpdateThreshold = 10.0f;
    config.interruptReactionTimeMs = 150;

    auto integrator = ::std::make_unique<CombatAIIntegrator>(bot);
    integrator->SetConfig(config);
    return integrator;
}

::std::unique_ptr<CombatAIIntegrator> CombatAIFactory::CreateMeleeDPSCombatAI(Player* bot)
{
    CombatAIConfig config;
    config.enablePositioning = true;
    config.enableInterrupts = true;
    config.positionUpdateThreshold = 5.0f;
    config.targetSwitchCooldownMs = 500;

    auto integrator = ::std::make_unique<CombatAIIntegrator>(bot);
    integrator->SetConfig(config);
    return integrator;
}

::std::unique_ptr<CombatAIIntegrator> CombatAIFactory::CreateRangedDPSCombatAI(Player* bot)
{
    CombatAIConfig config;
    config.enablePositioning = true;
    config.enableKiting = true;
    config.enableInterrupts = true;
    config.positionUpdateThreshold = 7.0f;

    auto integrator = ::std::make_unique<CombatAIIntegrator>(bot);
    integrator->SetConfig(config);
    return integrator;
}

} // namespace Playerbot
