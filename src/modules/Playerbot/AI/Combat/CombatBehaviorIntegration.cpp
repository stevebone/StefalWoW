/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CombatBehaviorIntegration.h"
#include "GameTime.h"
#include "BotThreatManager.h"      // Core threat management infrastructure
#include "PositionManager.h"       // Enterprise-grade positioning algorithms
#include "CombatStateAnalyzer.h"
#include "AdaptiveBehaviorManager.h"
#include "TargetManager.h"
#include "InterruptManager.h"
#include "CrowdControlManager.h"
#include "DefensiveManager.h"
#include "MovementIntegration.h"
#include "TTKEstimator.h"
#include "Player.h"
#include "Group.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellHistory.h"
#include "Spell.h"
#include "Item.h"
#include "Timer.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot
{

CombatBehaviorIntegration::CombatBehaviorIntegration(Player* bot) :
    _bot(bot),
    _lastActionTime(0),
    _inCombat(false),
    _emergencyMode(false),
    _survivalMode(false),
    _combatStartTime(0),
    _updateTimer(0),
    _lastUpdateTime(0),
    _totalUpdateTime(0),
    _updateCount(0),
    _detailedLogging(false),
    _successfulActions(0),
    _failedActions(0)
{
    // Initialize core infrastructure first (dependencies for other managers)
    _threatManager = std::make_unique<BotThreatManager>(bot);
    _positionManager = std::make_unique<PositionManager>(bot, _threatManager.get());

    // Initialize all manager components
    _stateAnalyzer = std::make_unique<CombatStateAnalyzer>(bot);
    _behaviorManager = std::make_unique<AdaptiveBehaviorManager>(bot);
    _targetManager = std::make_unique<TargetManager>(bot);
    _interruptManager = std::make_unique<InterruptManager>(bot);
    _crowdControlManager = std::make_unique<CrowdControlManager>(bot);
    _defensiveManager = std::make_unique<DefensiveManager>(bot);
    _movementIntegration = std::make_unique<MovementIntegration>(bot, _positionManager.get());
    _ttkEstimator = std::make_unique<TTKEstimator>(bot);
    // CRITICAL: Do NOT access bot->GetName() in constructor!
    // Bot's internal data (m_name) is not initialized during constructor chain.
    // Accessing it causes ACCESS_VIOLATION crash in string construction.
    // Logging deferred to first Update() when bot IsInWorld()
}

CombatBehaviorIntegration::~CombatBehaviorIntegration() = default;

void CombatBehaviorIntegration::Update(uint32 diff)
{
    uint32 startTime = GameTime::GetGameTimeMS();

    _updateTimer += diff;

    // Check combat state
    bool wasInCombat = _inCombat;
    _inCombat = _bot->IsInCombat();
    if (_inCombat && !wasInCombat)
    {
        OnCombatStart();
    }
    else if (!_inCombat && wasInCombat)
    {
        OnCombatEnd();
    }

    if (!_inCombat)
    {
        _lastUpdateTime = 0;
        return;
    }

    // Update every 50ms for quick response
    if (_updateTimer >= 50)
    {
        // Update all managers
        UpdateManagers(_updateTimer);

        // Update priorities based on current state
        UpdatePriorities();

        // Generate action recommendations
        GenerateRecommendations();

        // Prioritize actions in queue
        PrioritizeActions();

        _updateTimer = 0;
    }
    // Track performance
    _lastUpdateTime = GameTime::GetGameTimeMS() - startTime;
    _totalUpdateTime += _lastUpdateTime;
    _updateCount++;

    // Log performance warning if update takes too long
    if (_lastUpdateTime > 5 && _detailedLogging)
    {
        TC_LOG_WARN("bot.playerbot", "CombatBehaviorIntegration update took {}ms for bot {}",
            _lastUpdateTime, _bot->GetName());
    }
}

void CombatBehaviorIntegration::UpdateManagers(uint32 diff)
{
    // Update state analyzer first (provides metrics for others)
    _stateAnalyzer->Update(diff);

    const CombatMetrics& metrics = _stateAnalyzer->GetCurrentMetrics();
    CombatSituation situation = _stateAnalyzer->AnalyzeSituation();

    // Update adaptive behavior based on state
    _behaviorManager->Update(diff, metrics, situation);

    // Update target management
    _targetManager->Update(diff, metrics);

    // Update interrupt management
    _interruptManager->Update(diff);

    // Update crowd control
    _crowdControlManager->Update(diff, metrics);

    // Update defensive management
    _defensiveManager->Update(diff, metrics);

    // Update movement
    _movementIntegration->Update(diff, situation);

    // Update TTK estimator
    _ttkEstimator->Update(diff);

    // Update emergency flags
    _emergencyMode = _stateAnalyzer->IsWipeImminent() ||
                     metrics.personalHealthPercent < 20.0f ||
                     (!metrics.healerAlive && metrics.averageGroupHealth < 40.0f);

    // CRITICAL FIX: The 50% health threshold was WAY too aggressive, causing
    // HandleEmergencies() to return true and skip the combat rotation entirely!
    // Changed to 25% to only trigger in actual emergency situations.
    _survivalMode = situation == CombatSituation::DEFENSIVE ||
                    metrics.personalHealthPercent < 25.0f ||
                    _defensiveManager->NeedsEmergencyDefensive();
}

void CombatBehaviorIntegration::UpdatePriorities()
{
    std::lock_guard<OrderedMutex<LockOrder::BOT_AI_STATE>> lock(_actionQueueMutex);
        const CombatMetrics& metrics = _stateAnalyzer->GetCurrentMetrics();

    // Clear old actions
    if (GameTime::GetGameTimeMS() - _lastActionTime > 1000)
    {
        _actionQueue.clear();
    }

    // Emergency takes priority
    if (_emergencyMode)
    {
        RecommendedAction emergency;
        emergency.type = CombatActionType::EMERGENCY;
        emergency.urgency = ActionUrgency::EMERGENCY;
        emergency.reason = "Emergency mode active";
        emergency.timestamp = GameTime::GetGameTimeMS();
        _actionQueue.push_back(emergency);
    }

    // Check for interrupts
    if (_interruptManager->HasUrgentInterrupt())
    {
        Unit* target = _interruptManager->GetInterruptTarget();
        if (target)
        {
            RecommendedAction interrupt;
            interrupt.type = CombatActionType::INTERRUPT;
            interrupt.urgency = EvaluateInterruptPriority(target);
            interrupt.target = target;
            interrupt.reason = "Urgent interrupt needed";
            interrupt.timestamp = GameTime::GetGameTimeMS();
            _actionQueue.push_back(interrupt);
        }
    }

    // Check defensive needs
    if (_defensiveManager->NeedsDefensive())
    {
        RecommendedAction defensive;
        defensive.type = CombatActionType::DEFENSIVE;
        defensive.urgency = EvaluateDefensivePriority();
        defensive.spellId = _defensiveManager->GetRecommendedDefensive();
        defensive.reason = "Defensive ability needed";
        defensive.timestamp = GameTime::GetGameTimeMS();
        _actionQueue.push_back(defensive);
    }

    // Check movement needs
    if (_movementIntegration->NeedsUrgentMovement())
    {
        RecommendedAction movement;
        movement.type = CombatActionType::MOVEMENT;
        movement.urgency = EvaluateMovementPriority();
        movement.position = _movementIntegration->GetTargetPosition();
        movement.reason = "Movement required";
        movement.timestamp = GameTime::GetGameTimeMS();
        _actionQueue.push_back(movement);
    }

    // Check target switch
    if (_targetManager->ShouldSwitchTarget())
    {
        Unit* newTarget = _targetManager->GetPriorityTarget();
        ObjectGuid currentTarget = _bot->GetTarget();
        if (newTarget && newTarget->GetGUID() != currentTarget)
        {
            RecommendedAction targetSwitch;
            targetSwitch.type = CombatActionType::TARGET_SWITCH;
            targetSwitch.urgency = EvaluateTargetSwitchPriority();
            targetSwitch.target = newTarget;
            targetSwitch.reason = "Priority target available";
            targetSwitch.timestamp = GameTime::GetGameTimeMS();
            _actionQueue.push_back(targetSwitch);
        }
    }
}

void CombatBehaviorIntegration::GenerateRecommendations()
{
    // Generate additional recommendations based on current state
    const CombatMetrics& metrics = _stateAnalyzer->GetCurrentMetrics();

    // Recommend consumables
    if (_behaviorManager->ShouldUseConsumables())
    {
        RecommendedAction consumable;
        consumable.type = CombatActionType::CONSUMABLE;
        consumable.urgency = metrics.personalHealthPercent < 40.0f ?
                              ActionUrgency::HIGH : ActionUrgency::NORMAL;
        consumable.reason = "Consumable usage recommended";
        consumable.timestamp = GameTime::GetGameTimeMS();
        _actionQueue.push_back(consumable);
    }

    // Recommend cooldowns
    if (_behaviorManager->ShouldUseOffensiveCooldowns())
    {
        RecommendedAction cooldown;
        cooldown.type = CombatActionType::COOLDOWN;
        cooldown.urgency = _stateAnalyzer->NeedsBurst() ?
                            ActionUrgency::HIGH : ActionUrgency::NORMAL;
        cooldown.reason = "Offensive cooldowns recommended";
        cooldown.timestamp = GameTime::GetGameTimeMS();
        _actionQueue.push_back(cooldown);
    }

    // Recommend crowd control
    if (_crowdControlManager->ShouldUseCrowdControl())
    {
        Unit* ccTarget = _crowdControlManager->GetPriorityTarget();
        if (ccTarget)
        {
            RecommendedAction cc;
            cc.type = CombatActionType::CROWD_CONTROL;
            cc.urgency = ActionUrgency::HIGH;
            cc.target = ccTarget;
            cc.spellId = _crowdControlManager->GetRecommendedSpell(ccTarget);
            cc.reason = "Crowd control opportunity";
            cc.timestamp = GameTime::GetGameTimeMS();
            _actionQueue.push_back(cc);
        }
    }
}

void CombatBehaviorIntegration::PrioritizeActions()
{
    std::lock_guard<OrderedMutex<LockOrder::BOT_AI_STATE>> lock(_actionQueueMutex);
        // Sort actions by priority and score
    std::sort(_actionQueue.begin(), _actionQueue.end(),
        [this](const RecommendedAction& a, const RecommendedAction& b)
        {
            if (a.urgency != b.urgency)
                return a.urgency > b.urgency;
            return CalculateActionScore(a) > CalculateActionScore(b);
        });

    // Keep only top actions
    if (_actionQueue.size() > 5)
        _actionQueue.resize(5);
}

bool CombatBehaviorIntegration::HandleEmergencies()
{
    // Check for emergency conditions
    if (!_emergencyMode && !_survivalMode)
        return false;
    const CombatMetrics& metrics = _stateAnalyzer->GetCurrentMetrics();

    // Use defensive cooldowns
    if (_defensiveManager->NeedsEmergencyDefensive())
    {
        uint32 spellId = _defensiveManager->UseEmergencyDefensive();
        if (spellId > 0)
        {
            if (_detailedLogging)
                TC_LOG_DEBUG("bot.playerbot", "Bot {} used emergency defensive: {}",
                    _bot->GetName(), spellId);
            return true;
        }
    }

    // Use health consumables when HP is critically low
    // Only return true if we ACTUALLY use a consumable to avoid blocking rotation
    if (metrics.personalHealthPercent < 30.0f)
    {
        // Healthstone item IDs by expansion (Warlocks create these)
        static const uint32 HEALTHSTONE_IDS[] = {
            224464,  // Healthstone (TWW)
            207030,  // Healthstone (Dragonflight)
            177278,  // Healthstone (Shadowlands)
            156438,  // Healthstone (BfA)
            152303,  // Healthstone (Legion)
            5512,    // Healthstone (generic modern)
            36889,   // Fel Healthstone
            36892,   // Demonic Healthstone
        };

        // TWW 12.0 healing potion item IDs
        static const uint32 HEALING_POTION_IDS[] = {
            191380,  // Potion of Withering Dreams
            191381,  // Dreamwalker's Healing Potion
            191383,  // Potion of Withering Vitality
            171267,  // Spiritual Healing Potion
            171270,  // Potion of Spectral Healing
            127834,  // Ancient Healing Potion
            33447,   // Runic Healing Potion
            22829,   // Super Healing Potion
            13446,   // Major Healing Potion
            3928,    // Superior Healing Potion
            1710,    // Greater Healing Potion
            929,     // Healing Potion
            118,     // Minor Healing Potion
        };

        // Try healthstone first (higher priority)
        // Note: Don't check cooldown via HasCooldown() as hardcoded spell IDs may not exist in 12.0
        // Item use will fail gracefully if the item is on cooldown
        // WoW 12.0: CastItemUseSpell signature changed to std::array<int32, 3>
        std::array<int32, 3> misc = { 0, 0, 0 };

        for (uint32 healthstoneId : HEALTHSTONE_IDS)
        {
            if (Item* item = _bot->GetItemByEntry(healthstoneId))
            {
                _bot->CastItemUseSpell(item, SpellCastTargets(), ObjectGuid::Empty, misc);
                if (_detailedLogging)
                    TC_LOG_DEBUG("bot.playerbot", "Bot {} used Healthstone (item {})",
                        _bot->GetName(), healthstoneId);
                return true;
            }
        }

        // Try healing potion
        for (uint32 potionId : HEALING_POTION_IDS)
        {
            if (Item* item = _bot->GetItemByEntry(potionId))
            {
                _bot->CastItemUseSpell(item, SpellCastTargets(), ObjectGuid::Empty, misc);
                if (_detailedLogging)
                    TC_LOG_DEBUG("bot.playerbot", "Bot {} used Health Potion (item {})",
                        _bot->GetName(), potionId);
                return true;
            }
        }

        // No consumables available, continue to rotation
        if (_detailedLogging)
            TC_LOG_DEBUG("bot.playerbot", "Bot {} needs emergency healing but no consumables available",
                _bot->GetName());
    }

    // Emergency movement
    if (_movementIntegration->NeedsEmergencyMovement())
    {
        Position safePos = _stateAnalyzer->GetSafePosition();
        _movementIntegration->MoveToPosition(safePos, true);
        return true;
    }

    return false;
}

bool CombatBehaviorIntegration::ShouldInterrupt(Unit* target)
{
    if (!target)
    {
        ObjectGuid targetGuid = _bot->GetTarget();
        target = ObjectAccessor::GetUnit(*_bot, targetGuid);
    }

    if (!target)
        return false;

    return _interruptManager->ShouldInterrupt(target);
}

bool CombatBehaviorIntegration::ShouldInterruptCurrentCast()
{
    return _interruptManager->ShouldInterruptOwnCast();
}

bool CombatBehaviorIntegration::NeedsDefensive()
{
    return _defensiveManager->NeedsDefensive() ||
           _stateAnalyzer->NeedsDefensive();
}

bool CombatBehaviorIntegration::NeedsMovement()
{
    return _movementIntegration->NeedsMovement() ||
           _stateAnalyzer->NeedsToMoveOut();
}

bool CombatBehaviorIntegration::ShouldSwitchTarget()
{
    return _targetManager->ShouldSwitchTarget();
}

bool CombatBehaviorIntegration::ShouldUseCrowdControl()
{
    return _crowdControlManager->ShouldUseCrowdControl() ||
           _behaviorManager->ShouldUseCrowdControl();
}

bool CombatBehaviorIntegration::ShouldUseConsumables()
{
    return _behaviorManager->ShouldUseConsumables() ||
           _stateAnalyzer->ShouldUseConsumables();
}

bool CombatBehaviorIntegration::ShouldUseCooldowns()
{
    return _behaviorManager->ShouldUseOffensiveCooldowns();
}

bool CombatBehaviorIntegration::ShouldSaveCooldowns()
{
    return _behaviorManager->ShouldSaveCooldowns();
}

Unit* CombatBehaviorIntegration::GetPriorityTarget()
{
    return _targetManager->GetPriorityTarget();
}

Unit* CombatBehaviorIntegration::GetInterruptTarget()
{
    return _interruptManager->GetInterruptTarget();
}

Unit* CombatBehaviorIntegration::GetCrowdControlTarget()
{
    return _crowdControlManager->GetPriorityTarget();
}

bool CombatBehaviorIntegration::ShouldFocusAdd()
{
    return _stateAnalyzer->ShouldFocusAdd();
}

bool CombatBehaviorIntegration::ShouldAOE()
{
    return _behaviorManager->PreferAOE() ||
           _stateAnalyzer->HasCleaveTargets();
}

Position CombatBehaviorIntegration::GetOptimalPosition()
{
    return _movementIntegration->GetOptimalPosition();
}

float CombatBehaviorIntegration::GetOptimalRange(Unit* target)
{
    if (!target)
    {
        ObjectGuid targetGuid = _bot->GetTarget();
        target = ObjectAccessor::GetUnit(*_bot, targetGuid);
    }

    return _movementIntegration->GetOptimalRange(target);
}

bool CombatBehaviorIntegration::ShouldMoveToPosition(const Position& pos)
{
    return _movementIntegration->ShouldMoveToPosition(pos);
}

bool CombatBehaviorIntegration::IsPositionSafe(const Position& pos)
{
    return _movementIntegration->IsPositionSafe(pos);
}

bool CombatBehaviorIntegration::NeedsRepositioning()
{
    return _movementIntegration->NeedsRepositioning();
}

bool CombatBehaviorIntegration::CanAffordSpell(uint32 spellId)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Check mana cost (simplified check)
    if (_bot->GetPowerType() == POWER_MANA)
    {
        auto costs = spellInfo->CalcPowerCost(_bot, spellInfo->GetSchoolMask());
        int32 manaCost = 0;
        for (auto const& cost : costs)
        {
            if (cost.Power == POWER_MANA)
            {
                manaCost = cost.Amount;
                break;
            }
        }

        if (manaCost > 0 && _bot->GetPower(POWER_MANA) < static_cast<uint32>(manaCost))
            return false;

        // Check if we should conserve
        if (ShouldConserveMana() && manaCost > static_cast<int32>(_bot->GetMaxPower(POWER_MANA) / 10))
            return false;
    }

    return true;
}

bool CombatBehaviorIntegration::ShouldConserveMana()
{
    return _behaviorManager->ShouldConserveMana();
}

bool CombatBehaviorIntegration::IsResourceLow()
{
    Powers powerType = _bot->GetPowerType();
    float powerPct = _bot->GetPowerPct(powerType);
    return powerPct < 30.0f;
}

CombatSituation CombatBehaviorIntegration::GetCurrentSituation() const
{
    return _stateAnalyzer->AnalyzeSituation();
}

const CombatMetrics& CombatBehaviorIntegration::GetCombatMetrics() const
{
    return _stateAnalyzer->GetCurrentMetrics();
}

BotRole CombatBehaviorIntegration::GetCurrentRole() const
{
    return _behaviorManager->GetPrimaryRole();
}

bool CombatBehaviorIntegration::IsEmergencyMode() const
{
    return _emergencyMode;
}

bool CombatBehaviorIntegration::IsSurvivalMode() const
{
    return _survivalMode;
}

bool CombatBehaviorIntegration::IsStrategyActive(uint32 flag) const
{
    return _behaviorManager->IsStrategyActive(flag);
}

uint32 CombatBehaviorIntegration::GetActiveStrategies() const
{
    return _behaviorManager->GetActiveStrategies();
}

void CombatBehaviorIntegration::ActivateStrategy(uint32 flags)
{
    _behaviorManager->ActivateStrategy(flags);
}

void CombatBehaviorIntegration::DeactivateStrategy(uint32 flags)
{
    _behaviorManager->DeactivateStrategy(flags);
}

RecommendedAction CombatBehaviorIntegration::GetNextAction()
{
    std::lock_guard<OrderedMutex<LockOrder::BOT_AI_STATE>> lock(_actionQueueMutex);
        if (_actionQueue.empty())
        return RecommendedAction();

    RecommendedAction action = _actionQueue.front();
    _actionQueue.erase(_actionQueue.begin());
    _currentAction = action;
    _lastActionTime = GameTime::GetGameTimeMS();

    return action;
}

bool CombatBehaviorIntegration::HasPendingAction() const
{
    std::lock_guard<OrderedMutex<LockOrder::BOT_AI_STATE>> lock(_actionQueueMutex);
        return !_actionQueue.empty();
}

void CombatBehaviorIntegration::ClearPendingActions()
{
    std::lock_guard<OrderedMutex<LockOrder::BOT_AI_STATE>> lock(_actionQueueMutex);
        _actionQueue.clear();
}

void CombatBehaviorIntegration::RecordActionResult(const RecommendedAction& action, bool success)
{
    if (success)
    {
        _successfulActions++;
        _actionSuccesses[action.type]++;
    }
    else
    {
        _failedActions++;
    }

    _actionCounts[action.type]++;

    // Record in behavior manager for learning
    std::string decisionName = std::string(GetActionName(action.type)) + "_" + action.reason;
    _behaviorManager->RecordDecisionOutcome(decisionName, success);

    if (_detailedLogging)
    {
        LogAction(action, success);
    }
}

uint32 CombatBehaviorIntegration::GetAverageUpdateTime() const
{
    if (_updateCount == 0)
        return 0;
    return _totalUpdateTime / _updateCount;
}

void CombatBehaviorIntegration::DumpState() const
{
    TC_LOG_INFO("bot.playerbot", "=== Combat Behavior State for {} ===", _bot->GetName());
    TC_LOG_INFO("bot.playerbot", "Situation: {}", static_cast<uint32>(GetCurrentSituation()));
    TC_LOG_INFO("bot.playerbot", "Role: {}", GetRoleName(GetCurrentRole()));
    TC_LOG_INFO("bot.playerbot", "Emergency Mode: {}", _emergencyMode);
    TC_LOG_INFO("bot.playerbot", "Survival Mode: {}", _survivalMode);
    TC_LOG_INFO("bot.playerbot", "Active Strategies: 0x{:08X}", GetActiveStrategies());
{ std::lock_guard<OrderedMutex<LockOrder::BOT_AI_STATE>> lock(_actionQueueMutex);
TC_LOG_INFO("bot.playerbot", "Pending Actions: {}", _actionQueue.size()); }
    TC_LOG_INFO("bot.playerbot", "Success Rate: {}/{}",
        _successfulActions, _successfulActions + _failedActions);

    const CombatMetrics& metrics = GetCombatMetrics();
    TC_LOG_INFO("bot.playerbot", "Health: {:.1f}%, Mana: {:.1f}%",
        metrics.personalHealthPercent, metrics.manaPercent);
    TC_LOG_INFO("bot.playerbot", "Enemies: {}, Elites: {}, Bosses: {}",
        metrics.enemyCount, metrics.eliteCount, metrics.bossCount);
}

void CombatBehaviorIntegration::Reset()
{
    { std::lock_guard<OrderedMutex<LockOrder::BOT_AI_STATE>> lock(_actionQueueMutex); _actionQueue.clear(); }
    _currentAction = RecommendedAction();
    _lastActionTime = 0;
    _inCombat = false;
    _emergencyMode = false;
    _survivalMode = false;
    _combatStartTime = 0;
    _updateTimer = 0;

    // Reset all managers
    _stateAnalyzer->Reset();
    _behaviorManager->Reset();
    _targetManager->Reset();
    _interruptManager->Reset();
    _crowdControlManager->Reset();
    _defensiveManager->Reset();
    _movementIntegration->Reset();
    _ttkEstimator->Reset();

    // Clear statistics
    _successfulActions = 0;
    _failedActions = 0;
    _actionCounts.clear();
    _actionSuccesses.clear();
}

void CombatBehaviorIntegration::OnCombatStart()
{
    _inCombat = true;
    _combatStartTime = GameTime::GetGameTimeMS();
    _emergencyMode = false;
    _survivalMode = false;
    TC_LOG_DEBUG("bot.playerbot", "Bot {} entering combat", _bot->GetName());

    // Initialize managers for combat
    _behaviorManager->AssignRoles();
}

void CombatBehaviorIntegration::OnCombatEnd()
{
    _inCombat = false;

    TC_LOG_DEBUG("bot.playerbot", "Bot {} leaving combat - Duration: {}ms, Success rate: {:.1f}%",
        _bot->GetName(), GameTime::GetGameTimeMS() - _combatStartTime,
        _successfulActions > 0 ? (float)_successfulActions / (_successfulActions + _failedActions) * 100.0f : 0.0f);

    // Learn from combat
    _behaviorManager->AdjustBehaviorWeights();

    // Reset combat state
    Reset();
}

ActionUrgency CombatBehaviorIntegration::EvaluateInterruptPriority(Unit* target)
{
    if (!target || !target->HasUnitState(UNIT_STATE_CASTING))
        return ActionUrgency::LOW;
    // Check if cast is dangerous
    if (_interruptManager->IsCastDangerous(target))
        return ActionUrgency::EMERGENCY;

    // High priority for heals and crowd control
    if (_interruptManager->IsCastHighPriority(target))
        return ActionUrgency::HIGH;

    return ActionUrgency::NORMAL;
}

ActionUrgency CombatBehaviorIntegration::EvaluateDefensivePriority()
{
    const CombatMetrics& metrics = _stateAnalyzer->GetCurrentMetrics();

    if (metrics.personalHealthPercent < 20.0f)
        return ActionUrgency::EMERGENCY;

    if (metrics.personalHealthPercent < 40.0f)
        return ActionUrgency::CRITICAL;
    if (_defensiveManager->NeedsDefensive())
        return ActionUrgency::HIGH;

    return ActionUrgency::NORMAL;
}

ActionUrgency CombatBehaviorIntegration::EvaluateMovementPriority()
{
    if (_stateAnalyzer->IsInVoidZone())
        return ActionUrgency::EMERGENCY;

    if (_movementIntegration->NeedsUrgentMovement())
        return ActionUrgency::CRITICAL;

    if (_stateAnalyzer->NeedsToMoveOut())
        return ActionUrgency::HIGH;

    return ActionUrgency::NORMAL;
}

ActionUrgency CombatBehaviorIntegration::EvaluateTargetSwitchPriority()
{
    // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
    // ObjectAccessor is intentionally retained because:
    // 1. _bot->GetTarget() returns a GUID, needs resolution to Unit*
    // 2. We need the live Unit* to check HasAura() for immunity detection
    // 3. Target switching requires real-time unit state
    Unit* currentTarget = ObjectAccessor::GetUnit(*_bot, _bot->GetTarget());
    Unit* priorityTarget = _targetManager->GetPriorityTarget();

    if (!currentTarget || !priorityTarget)
        return ActionUrgency::LOW;

    // Emergency switch if current target is immune
    if (currentTarget->HasAura(642)) // Divine Shield example
        return ActionUrgency::URGENT;

    // High priority for dangerous adds
    if (_targetManager->IsHighPriorityTarget(priorityTarget))
        return ActionUrgency::HIGH;

    return ActionUrgency::NORMAL;
}

bool CombatBehaviorIntegration::IsManagerReady() const
{
    return _stateAnalyzer && _behaviorManager && _targetManager &&
           _interruptManager && _crowdControlManager && _defensiveManager &&
           _movementIntegration;
}

void CombatBehaviorIntegration::LogAction(const RecommendedAction& action, bool executed)
{
    TC_LOG_DEBUG("bot.playerbot", "Bot {} {} action: {} (Priority: {}, Reason: {})",
        _bot->GetName(),
        executed ? "executed" : "failed",
        GetActionName(action.type),
        GetUrgencyName(action.urgency),
        action.reason);
}

float CombatBehaviorIntegration::CalculateActionScore(const RecommendedAction& action) const
{
    float score = 100.0f;

    // Priority weight
    score *= (1.0f + static_cast<float>(action.urgency) * 0.2f);

    // Success rate weight
    if (_actionCounts.find(action.type) != _actionCounts.end())
    {
        uint32 count = _actionCounts.at(action.type);
        uint32 success = _actionSuccesses.count(action.type) ? _actionSuccesses.at(action.type) : 0;
        if (count > 0)
        {
            float successRate = static_cast<float>(success) / count;
            score *= (0.5f + successRate * 0.5f);
        }
    }

    // Freshness weight (newer actions score higher)
    uint32 age = GameTime::GetGameTimeMS() - action.timestamp;
    if (age > 1000)
        score *= 0.8f;

    return score;
}

} // namespace Playerbot