/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ThreatCoordinator.h"
#include "BotThreatManager.h"
#include "ThreatAbilities.h"
#include "InterruptCoordinator.h"
#include "PositionManager.h"
#include "BotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Group.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "SpellHistory.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include "Core/Events/CombatEventRouter.h"
#include "Core/Events/CombatEvent.h"
#include "AI/Coordination/Messaging/BotMessageBus.h"
#include "AI/Coordination/Messaging/BotMessage.h"
#include "../../Packets/SpellPacketBuilder.h"  // PHASE 0 WEEK 3: Packet-based spell casting
#include <algorithm>
#include <sstream>

namespace Playerbot
{

ThreatCoordinator::ThreatCoordinator(Group* group)
    : _group(group)
{
    _lastUpdate = ::std::chrono::steady_clock::now();
    _lastEmergencyCheck = ::std::chrono::steady_clock::now();

    // Phase 3: Subscribe to combat events for real-time threat tracking
    if (CombatEventRouter::Instance().IsInitialized())
    {
        CombatEventRouter::Instance().Subscribe(this);
        _subscribed = true;
        TC_LOG_DEBUG("playerbots", "ThreatCoordinator: Subscribed to CombatEventRouter (event-driven mode)");
    }
    else
    {
        TC_LOG_DEBUG("playerbots", "ThreatCoordinator: Initialized in polling mode (CombatEventRouter not ready)");
    }

    TC_LOG_DEBUG("playerbots", "ThreatCoordinator: Initialized for group");
}

ThreatCoordinator::~ThreatCoordinator()
{
    // Phase 3: Unsubscribe from combat events
    if (_subscribed && CombatEventRouter::Instance().IsInitialized())
    {
        CombatEventRouter::Instance().Unsubscribe(this);
        _subscribed = false;
        TC_LOG_DEBUG("playerbots", "ThreatCoordinator: Unsubscribed from CombatEventRouter");
    }
}

void ThreatCoordinator::Update(uint32 diff)
{
    auto startTime = ::std::chrono::high_resolution_clock::now();
    auto now = ::std::chrono::steady_clock::now();

    // ========================================================================
    // Phase 3 Event-Driven Architecture:
    // - Threat updates moved to event handlers (HandleDamageDealt, etc.)
    // - Standard updates only run when threat data is dirty OR at maintenance interval
    // - Emergency checks remain at 50ms for safety situations
    // ========================================================================

    // Emergency checks run frequently (50ms) - safety critical, always runs
    auto emergencyElapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - _lastEmergencyCheck).count();
    if (_emergencyResponseEnabled && emergencyElapsed >= _emergencyCheckInterval)
    {
        _lastEmergencyCheck = now;
        CheckEmergencySituations();
    }

    // Execute queued responses immediately (time-critical)
    ExecuteQueuedResponses();

    // Maintenance cycle - runs when dirty OR at maintenance interval
    _maintenanceTimer.fetch_add(diff, ::std::memory_order_relaxed);
    bool shouldUpdate = _threatDataDirty.load(::std::memory_order_relaxed) ||
                        _maintenanceTimer.load(::std::memory_order_relaxed) >= MAINTENANCE_INTERVAL_MS;

    if (!shouldUpdate)
        return;

    // Reset timers and dirty flag
    _maintenanceTimer.store(0, ::std::memory_order_relaxed);
    _threatDataDirty.store(false, ::std::memory_order_relaxed);
    _lastUpdate = now;

    // Core maintenance cycle (only when needed)
    UpdateGroupThreatStatus();
    UpdateBotAssignments();
    ProcessThreatResponses();
    UpdateStabilityMetrics();

    // Cleanup
    CleanupExpiredResponses();
    CleanupInactiveBots();

    _metrics.threatUpdates++;

    auto endTime = ::std::chrono::high_resolution_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);
    TrackPerformance(duration, "Update");
}

void ThreatCoordinator::RegisterBot(Player* bot, BotAI* ai)
{
    if (!bot || !ai)
        return;

    ::std::lock_guard lock(_coordinatorMutex);

    ObjectGuid botGuid = bot->GetGUID();
    // Create threat manager for the bot
    _botThreatManagers[botGuid] = ::std::make_unique<BotThreatManager>(bot);

    // Store AI reference
    _botAIs[botGuid] = ai;

    // Initialize assignment
    BotThreatAssignment assignment;
    assignment.botGuid = botGuid;
    assignment.assignedRole = DetermineRole(bot);
    assignment.targetThreatPercent = CalculateOptimalThreatPercent(botGuid, assignment.assignedRole);
    // Load available threat abilities for this bot
    auto& db = ThreatAbilitiesDB::Instance();
    auto abilities = db.GetClassAbilities(static_cast<Classes>(bot->GetClass()));
    for (const auto& ability : abilities)
    {
        if (bot->HasSpell(ability.spellId))
        {
            assignment.availableAbilities.push_back(ability.spellId);
        }
    }

    _botAssignments[botGuid] = assignment;
    // Auto-assign tanks
    if (assignment.assignedRole == ThreatRole::TANK)
    {
        if (_primaryTank.IsEmpty())
        {
            AssignPrimaryTank(botGuid);
        }
        else if (_offTank.IsEmpty())
        {
            AssignOffTank(botGuid);
        }
        else
        {
            _backupTanks.push_back(botGuid);
        }
    }

    TC_LOG_DEBUG("playerbots", "ThreatCoordinator: Registered bot {} with role {}",
                bot->GetName(), static_cast<uint32>(assignment.assignedRole));
}

void ThreatCoordinator::UnregisterBot(ObjectGuid botGuid)
{
    ::std::lock_guard lock(_coordinatorMutex);

    _botThreatManagers.erase(botGuid);
    _botAssignments.erase(botGuid);
    _botAIs.erase(botGuid);

    // Update tank assignments if needed
    if (botGuid == _primaryTank)
    {
        _primaryTank.Clear();
        if (!_offTank.IsEmpty())
        {
            _primaryTank = _offTank;
            _offTank.Clear();
        }
        else if (!_backupTanks.empty())
        {
            _primaryTank = _backupTanks.front();
            _backupTanks.erase(_backupTanks.begin());
        }
    }
    else if (botGuid == _offTank)
    {
        _offTank.Clear();
        if (!_backupTanks.empty())
        {
            _offTank = _backupTanks.front();
            _backupTanks.erase(_backupTanks.begin());
        }
    }
    else
    {
        auto it = ::std::find(_backupTanks.begin(), _backupTanks.end(), botGuid);
        if (it != _backupTanks.end())
            _backupTanks.erase(it);
    }

    TC_LOG_DEBUG("playerbots", "ThreatCoordinator: Unregistered bot {}", botGuid.ToString());
}

void ThreatCoordinator::UpdateBotRole(ObjectGuid botGuid, ThreatRole role)
{
    ::std::lock_guard lock(_coordinatorMutex);

    auto it = _botAssignments.find(botGuid);
    if (it != _botAssignments.end())
    {
        it->second.assignedRole = role;
        it->second.targetThreatPercent = CalculateOptimalThreatPercent(botGuid, role);

        TC_LOG_DEBUG("playerbots", "ThreatCoordinator: Updated bot {} role to {}",
                    botGuid.ToString(), static_cast<uint32>(role));
    }
}

bool ThreatCoordinator::AssignPrimaryTank(ObjectGuid botGuid)
{
    ::std::lock_guard lock(_coordinatorMutex);

    auto it = _botAssignments.find(botGuid);
    if (it == _botAssignments.end())
        return false;

    _primaryTank = botGuid;
    it->second.assignedRole = ThreatRole::TANK;
    it->second.targetThreatPercent = _tankThreatThreshold;
    it->second.useAbilities = true;

    TC_LOG_INFO("playerbots", "ThreatCoordinator: Assigned {} as primary tank", botGuid.ToString());
    return true;
}

bool ThreatCoordinator::AssignOffTank(ObjectGuid botGuid)
{
    ::std::lock_guard lock(_coordinatorMutex);

    auto it = _botAssignments.find(botGuid);
    if (it == _botAssignments.end())
        return false;

    _offTank = botGuid;
    it->second.assignedRole = ThreatRole::TANK;
    it->second.targetThreatPercent = _tankThreatThreshold * 0.8f; // Slightly lower than main tank
    it->second.useAbilities = true;

    TC_LOG_INFO("playerbots", "ThreatCoordinator: Assigned {} as off-tank", botGuid.ToString());
    return true;
}

void ThreatCoordinator::InitiateTankSwap(ObjectGuid fromTank, ObjectGuid toTank)
{
    ::std::lock_guard lock(_coordinatorMutex);

    // Queue taunt from new tank
    for (const auto& targetGuid : _groupStatus.activeTargets)
    {
        // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
        // ObjectAccessor is intentionally retained - taunt coordination requires:
        // 1. Live Unit* for taunt spell target validation
        // 2. Real-time threat table access
        // The spatial grid cannot provide threat table data.
        Unit* target = ObjectAccessor::GetUnit(*ObjectAccessor::FindPlayer(toTank), targetGuid);
        if (!target)
            continue;

        ThreatResponseAction action;
        action.executorBot = toTank;
        action.targetUnit = targetGuid;
        action.abilitySpellId = GetTauntSpellForBot(toTank);
        action.abilityType = ThreatAbilityType::TAUNT;
        action.executeTime = ::std::chrono::steady_clock::now();
        action.priority = 1; // Highest priority

        _queuedResponses.push_back(action);
    }

    // Queue threat reduction from old tank
    ThreatResponseAction reduction;
    reduction.executorBot = fromTank;
    reduction.abilityType = ThreatAbilityType::THREAT_REDUCTION;
    reduction.executeTime = ::std::chrono::steady_clock::now() + ::std::chrono::milliseconds(500);
    reduction.priority = 2;

    _queuedResponses.push_back(reduction);

    _metrics.tankSwaps++;

    // Announce tank swap via BotMessageBus so other bots can react
    if (_group)
    {
        ObjectGuid groupGuid = _group->GetGUID();
        if (!groupGuid.IsEmpty() && !_groupStatus.activeTargets.empty())
        {
            BotMessage msg = BotMessage::RequestTankSwap(
                fromTank, groupGuid, _groupStatus.activeTargets[0], 0);
            sBotMessageBus->Publish(msg);
        }
    }

    TC_LOG_INFO("playerbots", "ThreatCoordinator: Initiated tank swap from {} to {}",
                fromTank.ToString(), toTank.ToString());
}

bool ThreatCoordinator::ExecuteTaunt(ObjectGuid tankGuid, Unit* target)
{
    if (!target)
        return false;

    auto it = _botAssignments.find(tankGuid);
    if (it == _botAssignments.end())
        return false;

    Player* tank = ObjectAccessor::FindPlayer(tankGuid);
    if (!tank)
        return false;

    // Find appropriate taunt ability
    uint32 tauntSpell = GetTauntSpellForBot(tankGuid);
    if (tauntSpell == 0)
        return false;

    // Check if taunt is ready
    if (tank->GetSpellHistory()->HasCooldown(tauntSpell))
        return false;

    // Execute taunt through AI
    auto aiIt = _botAIs.find(tankGuid);
    if (aiIt != _botAIs.end() && aiIt->second)
    {
        // Cast the taunt
        // Packet-based threat management via SpellPacketBuilder (thread-safe)

        SpellPacketBuilder::BuildOptions options;

        options.skipGcdCheck = false;

        options.skipResourceCheck = false;

        options.skipTargetCheck = false;

        options.skipStateCheck = false;

        options.skipRangeCheck = false;

        options.logFailures = true;
        auto result = SpellPacketBuilder::BuildCastSpellPacket(

            dynamic_cast<Player*>(tank), tauntSpell, target, options);
        if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)

        {
            TC_LOG_DEBUG("playerbot.threat.taunt",

                         "Bot {} queued taunt spell {} (target: {})",

                         tank->GetName(), tauntSpell, target->GetName());

        }

        _metrics.tauntExecutions++;

        TC_LOG_DEBUG("playerbots", "ThreatCoordinator: {} executed taunt on {}",
                    tank->GetName(), target->GetName());
        return true;
    }

    return false;
}

bool ThreatCoordinator::ExecuteThreatReduction(ObjectGuid botGuid, float /*reductionPercent*/)
{
    auto it = _botAssignments.find(botGuid);
    if (it == _botAssignments.end())
        return false;

    Player* bot = ObjectAccessor::FindPlayer(botGuid);
    if (!bot)
        return false;

    // Find appropriate threat reduction ability
    auto& db = ThreatAbilitiesDB::Instance();
    auto abilities = db.GetClassAbilities(static_cast<Classes>(bot->GetClass()));
    for (const auto& ability : abilities)
    {
        if (ability.type == ThreatAbilityType::THREAT_REDUCTION ||
            ability.type == ThreatAbilityType::THREAT_DROP)
        {
            if (bot->HasSpell(ability.spellId) && !bot->GetSpellHistory()->HasCooldown(ability.spellId))
            {
                // Packet-based threat management via SpellPacketBuilder (thread-safe)

                SpellPacketBuilder::BuildOptions options;

                options.skipGcdCheck = false;

                options.skipResourceCheck = false;

                options.skipTargetCheck = false;

                options.skipStateCheck = false;

                options.skipRangeCheck = false;

                options.logFailures = true;


                auto result = SpellPacketBuilder::BuildCastSpellPacket(

                    dynamic_cast<Player*>(bot), ability.spellId, bot, options);
                if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)

                {

                    TC_LOG_DEBUG("playerbot.threat.threat_reduce",

                                 "Bot {} queued threat_reduce spell {} (target: {})",

                                 bot->GetName(), ability.spellId, bot->GetName());

                }

                _metrics.threatReductions++;

                TC_LOG_DEBUG("playerbots", "ThreatCoordinator: {} executed threat reduction ({})",
                            bot->GetName(), ability.name);
                return true;
            }
        }
    }

    return false;
}

bool ThreatCoordinator::ExecuteThreatTransfer(ObjectGuid fromBot, ObjectGuid toBot, Unit* target)
{
    if (!target)
        return false;

    Player* from = ObjectAccessor::FindPlayer(fromBot);
    Player* to = ObjectAccessor::FindPlayer(toBot);
    if (!from || !to)
        return false;

    // Check for threat transfer abilities (Misdirection, Tricks of the Trade)
    auto& db = ThreatAbilitiesDB::Instance();
    auto abilities = db.GetClassAbilities(static_cast<Classes>(from->GetClass()));
    for (const auto& ability : abilities)
    {
        if (ability.type == ThreatAbilityType::THREAT_TRANSFER)
        {
            if (from->HasSpell(ability.spellId) && !from->GetSpellHistory()->HasCooldown(ability.spellId))
            {
                // Packet-based threat management via SpellPacketBuilder (thread-safe)

                SpellPacketBuilder::BuildOptions options;

                options.skipGcdCheck = false;

                options.skipResourceCheck = false;

                options.skipTargetCheck = false;

                options.skipStateCheck = false;

                options.skipRangeCheck = false;

                options.logFailures = true;


                auto result = SpellPacketBuilder::BuildCastSpellPacket(

                    dynamic_cast<Player*>(from), ability.spellId, to, options);
                if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)

                {

                    TC_LOG_DEBUG("playerbot.threat.threat_transfer",

                                 "Bot {} queued threat_transfer spell {} (target: {})",

                                 from->GetName(), ability.spellId, to->GetName());

                }

                TC_LOG_DEBUG("playerbots", "ThreatCoordinator: {} transferred threat to {} using {}",
                            from->GetName(), to->GetName(), ability.name);
                return true;
            }
        }
    }

    return false;
}

void ThreatCoordinator::HandleEmergencyThreat(Unit* looseTarget)
{
    if (!looseTarget)
        return;

    ::std::lock_guard lock(_coordinatorMutex);

    // Find available tank for emergency taunt
    ObjectGuid emergencyTank;

    if (!_primaryTank.IsEmpty() && CanBotTaunt(_primaryTank))
    {
        emergencyTank = _primaryTank;
    }
    else if (!_offTank.IsEmpty() && CanBotTaunt(_offTank))
    {
        emergencyTank = _offTank;
    }
    else
    {
        for (const auto& backup : _backupTanks)
        {
            if (CanBotTaunt(backup))
            {
                emergencyTank = backup;
                break;
            }
        }
    }

    if (!emergencyTank.IsEmpty())
    {
        ThreatResponseAction action;
        action.executorBot = emergencyTank;
        action.targetUnit = looseTarget->GetGUID();
        action.abilitySpellId = GetTauntSpellForBot(emergencyTank);
        action.abilityType = ThreatAbilityType::TAUNT;
        action.executeTime = ::std::chrono::steady_clock::now();
        action.priority = 0; // Maximum priority

        _queuedResponses.push_back(action);

        _metrics.emergencyResponses++;

        TC_LOG_WARN("playerbots", "ThreatCoordinator: Emergency taunt queued for {} by {}",
                   looseTarget->GetName(), emergencyTank.ToString());
    }
}

void ThreatCoordinator::ProtectHealer(ObjectGuid healerGuid, Unit* attacker)
{
    if (!attacker)
        return;

    ::std::lock_guard lock(_coordinatorMutex);

    // Priority 1: Tank taunt
    HandleEmergencyThreat(attacker);

    // Priority 2: Healer threat reduction
    Player* healer = ObjectAccessor::FindPlayer(healerGuid);
    if (healer)
    {
        ThreatResponseAction action;
        action.executorBot = healerGuid;
        action.abilityType = ThreatAbilityType::THREAT_REDUCTION;
        action.executeTime = ::std::chrono::steady_clock::now();
        action.priority = 1;

        _queuedResponses.push_back(action);
    }

    TC_LOG_WARN("playerbots", "ThreatCoordinator: Protecting healer {} from {}",
               healerGuid.ToString(), attacker->GetName());
}

GroupThreatStatus ThreatCoordinator::GetGroupThreatStatus() const
{
    ::std::lock_guard lock(_coordinatorMutex);
    return _groupStatus;
}
bool ThreatCoordinator::IsGroupThreatStable() const
{
    ::std::lock_guard lock(_coordinatorMutex);
    return _groupStatus.state == ThreatState::STABLE;
}

float ThreatCoordinator::GetGroupThreatStability() const
{
    ::std::lock_guard lock(_coordinatorMutex);

    if (_groupStatus.activeTargets.empty())
        return 1.0f;

    uint32 controlledTargets = 0;
    for (const auto& [target, tank] : _groupStatus.targetTanks)
    {
        if (tank == _primaryTank || tank == _offTank)
            controlledTargets++;
    }

    return static_cast<float>(controlledTargets) / _groupStatus.activeTargets.size();
}

// Private implementation methods

void ThreatCoordinator::UpdateGroupThreatStatus()
{
    _groupStatus = GroupThreatStatus();
    _groupStatus.primaryTank = _primaryTank;
    _groupStatus.offTank = _offTank;
    _groupStatus.lastUpdate = ::std::chrono::steady_clock::now();

    // Collect all active combat targets
    ::std::unordered_set<ObjectGuid> allTargets;

    for (const auto& [botGuid, threatMgr] : _botThreatManagers)
    {
        if (!threatMgr)
            continue;

        auto targets = threatMgr->GetAllThreatTargets();
        for (Unit* target : targets)
        {
            if (target && target->IsAlive() && target->IsInCombat())
            {
                ObjectGuid targetGuid = target->GetGUID();
                allTargets.insert(targetGuid);

                // Track highest threat holder for this target
                float threat = threatMgr->GetThreatPercent(target);
                auto it = _groupStatus.targetThreatLevels.find(targetGuid);
                if (it == _groupStatus.targetThreatLevels.end() || threat > it->second)
                {
                    _groupStatus.targetThreatLevels[targetGuid] = threat;
                    _groupStatus.targetTanks[targetGuid] = botGuid;
                }
            }
        }
    }

    _groupStatus.activeTargets.assign(allTargets.begin(), allTargets.end());

    // Analyze threat distribution
    for (const auto& targetGuid : _groupStatus.activeTargets)
    {
        // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
        // ObjectAccessor is intentionally retained - threat distribution analysis requires:
        // 1. Live Unit* for target->GetVictim() call
        // 2. ToPlayer() conversion for victim identification
        // 3. Real-time victim state (may change between snapshots)
        // The spatial grid cannot provide real-time victim relationship data.
        Unit* target = ObjectAccessor::GetUnit(*ObjectAccessor::FindPlayer(_primaryTank), targetGuid);
        if (!target)
            continue;

        Unit* victim = target->GetVictim();
        if (!victim || !victim->IsPlayer())
            continue;

        Player* victimPlayer = victim->ToPlayer();
        ObjectGuid victimGuid = victimPlayer->GetGUID();
        // Check if target is on appropriate tank
        bool onTank = (victimGuid == _primaryTank || victimGuid == _offTank);
        if (!onTank)
        {
            _groupStatus.looseTargets++;

            // Check if attacking healer/DPS
            auto assignmentIt = _botAssignments.find(victimGuid);
            if (assignmentIt != _botAssignments.end())
            {
                if (assignmentIt->second.assignedRole == ThreatRole::HEALER ||
                    assignmentIt->second.assignedRole == ThreatRole::DPS)
                {
                    _groupStatus.criticalTargets++;
                }
            }
        }
    }

    // Determine overall state
    if (_groupStatus.criticalTargets > 0)
    {
        _groupStatus.state = ThreatState::CRITICAL;
        _groupStatus.requiresEmergencyResponse = true;
    }
    else if (_groupStatus.looseTargets > 1)
    {
        _groupStatus.state = ThreatState::UNSTABLE;
        _groupStatus.requiresTaunt = true;
    }
    else if (_groupStatus.looseTargets == 1)
    {
        _groupStatus.state = ThreatState::RECOVERING;
    }
    else
    {
        _groupStatus.state = ThreatState::STABLE;
    }
}

void ThreatCoordinator::UpdateBotAssignments()
{
    for (auto& [botGuid, assignment] : _botAssignments)
    {
        auto threatMgrIt = _botThreatManagers.find(botGuid);
        if (threatMgrIt == _botThreatManagers.end() || !threatMgrIt->second)
            continue;

        // Update current threat levels
    for (const auto& targetGuid : _groupStatus.activeTargets)
        {
            // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
            // ObjectAccessor is intentionally retained - threat percentage lookup requires:
            // 1. Live Unit* for GetThreatPercent(target) call
            // 2. Real-time threat table data (changes constantly during combat)
            // The spatial grid snapshot cannot provide live threat table values.
            Unit* target = ObjectAccessor::GetUnit(*ObjectAccessor::FindPlayer(botGuid), targetGuid);
            if (!target)
                continue;

            assignment.currentThreatPercent = threatMgrIt->second->GetThreatPercent(target);
            assignment.targetGuid = targetGuid;

            // Determine if bot needs to adjust threat
            assignment.useAbilities = ShouldUseThreatAbility(assignment);
        }
    }
}

void ThreatCoordinator::ProcessThreatResponses()
{
    GenerateThreatResponses();
}

void ThreatCoordinator::CheckEmergencySituations()
{
    if (_groupStatus.state == ThreatState::CRITICAL ||
        _groupStatus.requiresEmergencyResponse)
    {
        InitiateEmergencyProtocol();
    }
}

void ThreatCoordinator::GenerateThreatResponses()
{
    for (const auto& [botGuid, assignment] : _botAssignments)
    {
        if (!assignment.useAbilities)
            continue;

        // Tank: Generate taunt if needed
    if (assignment.assignedRole == ThreatRole::TANK)
        {
            if (assignment.currentThreatPercent < 100.0f && _groupStatus.requiresTaunt)
            {
                // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
                // ObjectAccessor is intentionally retained - taunt decision requires:
                // 1. Live Unit* for target->GetVictim() comparison
                // 2. Real-time victim verification (who is target attacking now?)
                // 3. Spell cast target validation
                // The spatial grid cannot provide live victim relationship data.
                Unit* target = ObjectAccessor::GetUnit(*ObjectAccessor::FindPlayer(botGuid), assignment.targetGuid);
                if (target && target->GetVictim() != ObjectAccessor::FindPlayer(botGuid))
                {
                    ThreatResponseAction action;
                    action.executorBot = botGuid;
                    action.targetUnit = assignment.targetGuid;
                    action.abilitySpellId = GetTauntSpellForBot(botGuid);
                    action.abilityType = ThreatAbilityType::TAUNT;
                    action.executeTime = ::std::chrono::steady_clock::now() + ::std::chrono::milliseconds(100);
                    action.priority = 2;

                    QueueThreatResponse(action);
                }
            }
        }
        // DPS/Healer: Generate threat reduction if needed
        else if (assignment.currentThreatPercent > assignment.targetThreatPercent)
        {
            ThreatResponseAction action;
            action.executorBot = botGuid;
            action.abilityType = ThreatAbilityType::THREAT_REDUCTION;
            action.executeTime = ::std::chrono::steady_clock::now() + ::std::chrono::milliseconds(200);
            action.priority = 3;

            QueueThreatResponse(action);
        }
    }
}

void ThreatCoordinator::QueueThreatResponse(ThreatResponseAction const& action)
{
    if (_queuedResponses.size() >= MAX_RESPONSE_QUEUE_SIZE)
    {
        // Remove lowest priority action
        auto it = ::std::max_element(_queuedResponses.begin(), _queuedResponses.end(),
            [](const ThreatResponseAction& a, const ThreatResponseAction& b) {
                return a.priority < b.priority;
            });
        if (it != _queuedResponses.end())
            _queuedResponses.erase(it);
    }

    _queuedResponses.push_back(action);

    // Sort by priority and execution time
    ::std::sort(_queuedResponses.begin(), _queuedResponses.end(),
        [](const ThreatResponseAction& a, const ThreatResponseAction& b) {
            if (a.priority != b.priority)
                return a.priority < b.priority;
            return a.executeTime < b.executeTime;
        });
}

void ThreatCoordinator::ExecuteQueuedResponses()
{
    auto now = ::std::chrono::steady_clock::now();

    for (auto& action : _queuedResponses)
    {
        if (action.executed || !action.IsReady())
            continue;

        Player* executor = ObjectAccessor::FindPlayer(action.executorBot);
        if (!executor)
        {
            action.executed = true;
            continue;
        }

        bool success = false;

        switch (action.abilityType)
        {
            case ThreatAbilityType::TAUNT:
            {
                // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
                // ObjectAccessor is intentionally retained - taunt execution requires:
                // 1. Live Unit* for ExecuteTaunt() spell target parameter
                // 2. Real-time IsAlive() verification before spell cast
                // The spatial grid snapshot cannot be used as a spell target.
                Unit* target = ObjectAccessor::GetUnit(*executor, action.targetUnit);
                success = ExecuteTaunt(action.executorBot, target);
                if (success)
                    _metrics.tauntSuccesses++;
                break;
            }
            case ThreatAbilityType::THREAT_REDUCTION:
            case ThreatAbilityType::THREAT_DROP:
            {
                success = ExecuteThreatReduction(action.executorBot, 0.5f);
                break;
            }
            case ThreatAbilityType::THREAT_TRANSFER:
            {
                // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
                // ObjectAccessor is intentionally retained - threat transfer requires:
                // 1. Live Unit* for ExecuteThreatTransfer() spell target
                // 2. Real-time threat table modification
                // The spatial grid snapshot cannot receive transferred threat.
                Unit* target = ObjectAccessor::GetUnit(*executor, action.targetUnit);
                success = ExecuteThreatTransfer(action.executorBot, _primaryTank, target);
                break;
            }
            default:
                break;
        }

        action.executed = true;
        action.succeeded = success;
    }
}

void ThreatCoordinator::InitiateEmergencyProtocol()
{
    TC_LOG_WARN("playerbots", "ThreatCoordinator: Initiating emergency threat protocol");

    // Execute immediate taunts on all loose targets
    for (const auto& targetGuid : _groupStatus.activeTargets)
    {
        // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
        // ObjectAccessor is intentionally retained - emergency protocol requires:
        // 1. Live Unit* for immediate taunt spell execution
        // 2. Real-time IsAlive() and victim state verification
        // 3. Spell target validation
        // The spatial grid snapshot cannot be used for emergency spell casts.
        Unit* target = ObjectAccessor::GetUnit(*ObjectAccessor::FindPlayer(_primaryTank), targetGuid);
        if (!target)
            continue;

        Unit* victim = target->GetVictim();
        if (!victim || !victim->IsPlayer())
            continue;

        ObjectGuid victimGuid = victim->ToPlayer()->GetGUID();
        // If not on tank, execute emergency taunt
    if (victimGuid != _primaryTank && victimGuid != _offTank)
        {
            ExecuteEmergencyTaunt(target);
        }
    }

    // Mass threat reduction for non-tanks
    ExecuteMassTheatReduction();

    _metrics.emergencyResponses++;
}

void ThreatCoordinator::ExecuteEmergencyTaunt(Unit* target)
{
    if (!target)
        return;

    // Try primary tank first
    if (!_primaryTank.IsEmpty())
    {
        Player* tank = ObjectAccessor::FindPlayer(_primaryTank);
        if (tank && tank->IsAlive() && tank->GetDistance2d(target) <= 30.0f)
        {
            ExecuteTaunt(_primaryTank, target);
            return;
        }
    }

    // Try off-tank
    if (!_offTank.IsEmpty())
    {
        Player* tank = ObjectAccessor::FindPlayer(_offTank);
        if (tank && tank->IsAlive() && tank->GetDistance2d(target) <= 30.0f)
        {
            ExecuteTaunt(_offTank, target);
            return;
        }
    }

    // Try backup tanks
    for (const auto& backupGuid : _backupTanks)
    {
        Player* tank = ObjectAccessor::FindPlayer(backupGuid);
        if (tank && tank->IsAlive() && tank->GetDistance2d(target) <= 30.0f)
        {
            ExecuteTaunt(backupGuid, target);
            return;
        }
    }
}

void ThreatCoordinator::ExecuteMassTheatReduction()
{
    for (const auto& [botGuid, assignment] : _botAssignments)
    {
        if (assignment.assignedRole == ThreatRole::DPS ||
            assignment.assignedRole == ThreatRole::HEALER)
        {
            ExecuteThreatReduction(botGuid, 0.5f);
        }
    }
}

void ThreatCoordinator::CleanupExpiredResponses()
{
    _queuedResponses.erase(
        ::std::remove_if(_queuedResponses.begin(), _queuedResponses.end(),
            [](const ThreatResponseAction& action) {
                return action.executed;
            }),
        _queuedResponses.end()
    );
}

float ThreatCoordinator::CalculateOptimalThreatPercent(ObjectGuid /*botGuid*/, ThreatRole role) const
{
    switch (role)
    {
        case ThreatRole::TANK:
            return _tankThreatThreshold;
        case ThreatRole::DPS:
            return _dpsThreatThreshold;
        case ThreatRole::HEALER:
            return _healerThreatThreshold;
        case ThreatRole::SUPPORT:
            return _healerThreatThreshold * 0.9f;
        default:
            return 50.0f;
    }
}

bool ThreatCoordinator::ShouldUseThreatAbility(BotThreatAssignment const& assignment) const
{
    // Tanks should always be ready to use threat abilities
    if (assignment.assignedRole == ThreatRole::TANK)
    {
        return assignment.currentThreatPercent < assignment.targetThreatPercent ||
               _groupStatus.requiresTaunt;
    }

    // DPS/Healers use threat reduction when approaching cap
    if (assignment.assignedRole == ThreatRole::DPS ||
        assignment.assignedRole == ThreatRole::HEALER)
    {
        return assignment.currentThreatPercent > assignment.targetThreatPercent;
    }

    return false;
}

void ThreatCoordinator::TrackPerformance(::std::chrono::microseconds duration, ::std::string const& /*operation*/)
{
    _metrics.maxUpdateTime = ::std::max(_metrics.maxUpdateTime, duration);

    // Update moving average
    static ::std::chrono::microseconds totalTime{0};
    static uint32 samples = 0;

    totalTime += duration;
    samples++;

    if (samples >= 100) // Reset every 100 samples
    {
        _metrics.averageUpdateTime = totalTime / samples;
        totalTime = ::std::chrono::microseconds{0};
        samples = 0;
    }
}

void ThreatCoordinator::UpdateStabilityMetrics()
{
    float stability = GetGroupThreatStability();

    // Update moving average
    static float totalStability = 0.0f;
    static uint32 samples = 0;

    totalStability += stability;
    samples++;

    if (samples >= 100)
    {
        _metrics.averageThreatStability = totalStability / samples;
        totalStability = 0.0f;
        samples = 0;
    }

    // Update tank control rate
    if (!_groupStatus.activeTargets.empty())
    {
        uint32 tankedTargets = 0;
        for (const auto& [target, tank] : _groupStatus.targetTanks)
        {
            if (tank == _primaryTank || tank == _offTank)
                tankedTargets++;
        }

        _metrics.tankControlRate = static_cast<float>(tankedTargets) / _groupStatus.activeTargets.size();
    }
}

void ThreatCoordinator::ResetMetrics()
{
    _metrics.Reset();
}

// Helper methods

ThreatRole ThreatCoordinator::DetermineRole(Player* bot) const
{
    if (!bot)
        return ThreatRole::UNDEFINED;

    uint8 classId = bot->GetClass();
    // Check specialization for hybrid classes
    // This is simplified - in production, check actual spec
    switch (classId)
    {
        case CLASS_WARRIOR:
            return ThreatRole::TANK; // Protection assumed
        case CLASS_PALADIN:
            return ThreatRole::TANK; // Protection assumed
        case CLASS_DEATH_KNIGHT:
            return ThreatRole::TANK; // Blood assumed
        case CLASS_DEMON_HUNTER:
            return ThreatRole::TANK; // Vengeance assumed
        case CLASS_MONK:
            return ThreatRole::TANK; // Brewmaster assumed
        case CLASS_DRUID:
            return ThreatRole::TANK; // Guardian assumed
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
        case CLASS_EVOKER:
            return ThreatRole::HEALER;
        case CLASS_ROGUE:
        case CLASS_HUNTER:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            return ThreatRole::DPS;
        default:
            return ThreatRole::UNDEFINED;
    }
}

uint32 ThreatCoordinator::GetTauntSpellForBot(ObjectGuid botGuid) const
{
    auto it = _botAssignments.find(botGuid);
    if (it == _botAssignments.end())
        return 0;

    Player* bot = ObjectAccessor::FindPlayer(botGuid);
    if (!bot)
        return 0;

    // Get class-specific taunt
    switch (bot->GetClass())
    {
        case CLASS_WARRIOR:
            return ThreatSpells::TAUNT;
        case CLASS_PALADIN:
            return ThreatSpells::HAND_OF_RECKONING;
        case CLASS_DEATH_KNIGHT:
            return ThreatSpells::DARK_COMMAND;
        case CLASS_DEMON_HUNTER:
            return ThreatSpells::TORMENT;
        case CLASS_MONK:
            return ThreatSpells::PROVOKE;
        case CLASS_DRUID:
            return ThreatSpells::GROWL;
        default:
            return 0;
    }
}

bool ThreatCoordinator::CanBotTaunt(ObjectGuid botGuid) const
{
    Player* bot = ObjectAccessor::FindPlayer(botGuid);
    if (!bot)
        return false;

    uint32 tauntSpell = GetTauntSpellForBot(botGuid);
    if (tauntSpell == 0)
        return false;

    return !bot->GetSpellHistory()->HasCooldown(tauntSpell);
}

void ThreatCoordinator::LogThreatStatus() const
{
    ::std::stringstream ss;
    ss << "=== Threat Status Report ===\n";
    ss << "State: " << static_cast<uint32>(_groupStatus.state) << "\n";
    ss << "Active Targets: " << _groupStatus.activeTargets.size() << "\n";
    ss << "Loose Targets: " << _groupStatus.looseTargets << "\n";
    ss << "Critical Targets: " << _groupStatus.criticalTargets << "\n";
    ss << "Group Stability: " << GetGroupThreatStability() * 100 << "%\n";
    ss << "Tank Control Rate: " << _metrics.tankControlRate * 100 << "%\n";

    TC_LOG_INFO("playerbots", "{}", ss.str());
}

::std::string ThreatCoordinator::GetThreatReport() const
{
    ::std::stringstream report;

    report << "Threat Coordination Report:\n";
    report << "- Updates: " << _metrics.threatUpdates << "\n";
    report << "- Taunts: " << _metrics.tauntExecutions << " (Success: " << _metrics.tauntSuccesses << ")\n";
    report << "- Reductions: " << _metrics.threatReductions << "\n";
    report << "- Emergencies: " << _metrics.emergencyResponses << "\n";
    report << "- Tank Swaps: " << _metrics.tankSwaps << "\n";
    report << "- Avg Update Time: " << _metrics.averageUpdateTime.count() << " μs\n";
    report << "- Max Update Time: " << _metrics.maxUpdateTime.count() << " μs\n";
    report << "- Avg Stability: " << _metrics.averageThreatStability * 100 << "%\n";

    return report.str();
}

// ============================================================================
// ICombatEventSubscriber Implementation (Phase 3 Event-Driven Architecture)
// ============================================================================

CombatEventType ThreatCoordinator::GetSubscribedEventTypes() const
{
    // Subscribe to all events that affect threat tracking:
    // - DAMAGE_TAKEN: Updates threat from damage received
    // - DAMAGE_DEALT: Updates threat from damage dealt
    // - HEALING_DONE: Updates threat from healing generated
    // - THREAT_CHANGED: Direct threat table updates
    // - UNIT_DIED: Remove dead units from tracking
    return CombatEventType::DAMAGE_TAKEN |
           CombatEventType::DAMAGE_DEALT |
           CombatEventType::HEALING_DONE |
           CombatEventType::THREAT_CHANGED |
           CombatEventType::UNIT_DIED;
}

bool ThreatCoordinator::ShouldReceiveEvent(const CombatEvent& event) const
{
    // Only receive events involving our group members
    // This filters out events from unrelated players/creatures

    // For threat events, check if either participant is a group member
    if (!event.source.IsEmpty() && IsGroupMember(event.source))
        return true;

    if (!event.target.IsEmpty() && IsGroupMember(event.target))
        return true;

    return false;
}

void ThreatCoordinator::OnCombatEvent(const CombatEvent& event)
{
    // Route event to appropriate handler based on type
    switch (event.type)
    {
        case CombatEventType::DAMAGE_TAKEN:
            HandleDamageTaken(event);
            break;

        case CombatEventType::DAMAGE_DEALT:
            HandleDamageDealt(event);
            break;

        case CombatEventType::HEALING_DONE:
            HandleHealingDone(event);
            break;

        case CombatEventType::THREAT_CHANGED:
            HandleThreatChanged(event);
            break;

        case CombatEventType::UNIT_DIED:
            HandleUnitDied(event);
            break;

        default:
            // Ignore unhandled event types
            break;
    }
}

void ThreatCoordinator::HandleDamageTaken(const CombatEvent& event)
{
    // When a group member takes damage, track who is attacking them
    // This helps identify loose targets attacking non-tanks

    if (!event.target || !event.source)
        return;

    // Check if the victim is one of our group members
    if (!IsGroupMember(event.target))
        return;

    ::std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_coordinatorMutex);

    // Find the victim's assignment
    auto it = _botAssignments.find(event.target);
    if (it == _botAssignments.end())
        return;

    // If a healer or DPS is taking damage, this may indicate a threat issue
    if (it->second.assignedRole == ThreatRole::HEALER ||
        it->second.assignedRole == ThreatRole::DPS)
    {
        // Track this as a potential loose target situation
        _groupStatus.looseTargets++;

        // If significant damage, mark as critical
        if (event.amount > 0)
        {
            _groupStatus.criticalTargets++;
            _groupStatus.requiresEmergencyResponse = true;
        }
    }

    // Mark threat data as dirty for next Update() cycle
    _threatDataDirty = true;
}

void ThreatCoordinator::HandleDamageDealt(const CombatEvent& event)
{
    // When a group member deals damage, update their threat tracking
    // Helps identify when DPS is pulling aggro

    if (!event.source || !event.target)
        return;

    // Check if the attacker is one of our group members
    if (!IsGroupMember(event.source))
        return;

    ::std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_coordinatorMutex);

    // Find the attacker's assignment
    auto it = _botAssignments.find(event.source);
    if (it == _botAssignments.end())
        return;

    // Track that this bot generated threat against this target
    // The actual threat percent calculation happens in UpdateGroupThreatStatus()

    // For DPS roles, check if they're approaching threat cap
    if (it->second.assignedRole == ThreatRole::DPS)
    {
        // High damage may indicate approaching threat cap
        // Actual threat calculation done in maintenance cycle
        _threatDataDirty = true;
    }

    // Mark threat data as dirty
    _threatDataDirty = true;
}

void ThreatCoordinator::HandleHealingDone(const CombatEvent& event)
{
    // Healing generates threat spread across all mobs in combat
    // This is important for healer threat management

    if (!event.source)
        return;

    // Check if the healer is one of our group members
    if (!IsGroupMember(event.source))
        return;

    ::std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_coordinatorMutex);

    // Find the healer's assignment
    auto it = _botAssignments.find(event.source);
    if (it == _botAssignments.end())
        return;

    // Only track for healer role
    if (it->second.assignedRole != ThreatRole::HEALER)
        return;

    // Healing threat is split among all mobs in combat
    // Significant healing may approach healer threat threshold
    if (event.amount > 5000)  // Significant heal
    {
        // Mark for threat recalculation
        _threatDataDirty = true;
    }
}

void ThreatCoordinator::HandleThreatChanged(const CombatEvent& event)
{
    // Direct threat table update from TrinityCore ThreatManager
    // This is the most accurate threat information

    if (!event.source || !event.target)
        return;

    // Only care if the threat holder is a group member
    // (source is the unit on the threat table, target is the mob)
    if (!IsGroupMember(event.source))
        return;

    ::std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_coordinatorMutex);

    // Find the bot's assignment
    auto it = _botAssignments.find(event.source);
    if (it == _botAssignments.end())
        return;

    // Update the bot's threat data directly from the event
    float threatDelta = event.newThreat - event.oldThreat;

    // Large threat changes may indicate:
    // - Tank building aggro (good)
    // - DPS pulling threat (bad)
    // - Threat reduction ability used
    if (::std::abs(threatDelta) > 1000.0f)
    {
        // Significant threat change, recalculate status
        _threatDataDirty = true;

        // Check for threat spike on non-tank
        if (it->second.assignedRole != ThreatRole::TANK && threatDelta > 0)
        {
            // DPS/Healer gaining significant threat
            if (event.newThreat > _dpsThreatThreshold)
            {
                _groupStatus.requiresTaunt = true;
            }
        }
    }

    // Always mark dirty for any threat change
    _threatDataDirty = true;
}

void ThreatCoordinator::HandleUnitDied(const CombatEvent& event)
{
    // When a unit dies, remove it from our tracking

    if (!event.source)
        return;

    ::std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_coordinatorMutex);

    // Check if it's a group member that died
    if (IsGroupMember(event.source))
    {
        // Remove from bot assignments
        _botAssignments.erase(event.source);
        _botAIs.erase(event.source);
        _botThreatManagers.erase(event.source);

        // Check if it was a tank
        if (_primaryTank == event.source)
        {
            _primaryTank = ObjectGuid::Empty;
            _groupStatus.requiresEmergencyResponse = true;
        }
        if (_offTank == event.source)
        {
            _offTank = ObjectGuid::Empty;
        }

        // Remove from backup tanks
        _backupTanks.erase(
            ::std::remove(_backupTanks.begin(), _backupTanks.end(), event.source),
            _backupTanks.end());
    }

    // Check if it's a target we were tracking
    auto targetIt = _groupStatus.targetThreatLevels.find(event.source);
    if (targetIt != _groupStatus.targetThreatLevels.end())
    {
        // Remove from active targets
        _groupStatus.targetThreatLevels.erase(targetIt);
        _groupStatus.targetTanks.erase(event.source);
        _groupStatus.activeTargets.erase(
            ::std::remove(_groupStatus.activeTargets.begin(), _groupStatus.activeTargets.end(), event.source),
            _groupStatus.activeTargets.end());
    }

    // Mark for status recalculation
    _threatDataDirty = true;
}

bool ThreatCoordinator::IsGroupMember(ObjectGuid guid) const
{
    // Check if the GUID belongs to one of our registered bots
    // This is used to filter events to only those involving our group

    ::std::lock_guard<Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE>> lock(_coordinatorMutex);

    // Check bot assignments (all registered bots)
    if (_botAssignments.find(guid) != _botAssignments.end())
        return true;

    // Also check the group if available (TrinityCore 11.x API)
    if (_group)
    {
        // Check if GUID is in the group using GetMemberSlots()
        Group::MemberSlotList const& memberSlots = _group->GetMemberSlots();
        for (auto const& slot : memberSlots)
        {
            if (slot.guid == guid)
                return true;
        }
    }

    return false;
}

} // namespace Playerbot