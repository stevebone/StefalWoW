/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "UnifiedInterruptSystem.h"
#include "GameTime.h"
#include "BotAI.h"
#include "Player.h"
#include "Group.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "ObjectAccessor.h"
#include "GridNotifiers.h"
#include "SpellPacketBuilder.h"
#include "InterruptDatabase.h"
#include "Log.h"
#include <algorithm>
#include <cmath>

namespace Playerbot
{

// =====================================================================
// SINGLETON IMPLEMENTATION (Thread-Safe C++11 Static Local)
// =====================================================================

UnifiedInterruptSystem* UnifiedInterruptSystem::instance()
{
    static UnifiedInterruptSystem instance;
    return &instance;
}

UnifiedInterruptSystem::UnifiedInterruptSystem()
    : _initialized(false)
    , _initTime(::std::chrono::system_clock::now())
{
    // Initialize atomic metrics
    _metrics.spellsDetected.store(0, ::std::memory_order_relaxed);
    _metrics.interruptAttempts.store(0, ::std::memory_order_relaxed);
    _metrics.interruptSuccesses.store(0, ::std::memory_order_relaxed);
    _metrics.interruptFailures.store(0, ::std::memory_order_relaxed);
    _metrics.fallbacksUsed.store(0, ::std::memory_order_relaxed);
    _metrics.movementRequired.store(0, ::std::memory_order_relaxed);
    _metrics.groupCoordinations.store(0, ::std::memory_order_relaxed);
    _metrics.rotationViolations.store(0, ::std::memory_order_relaxed);
}

UnifiedInterruptSystem::~UnifiedInterruptSystem()
{
    Shutdown();
}

bool UnifiedInterruptSystem::Initialize()
{
    ::std::lock_guard lock(_mutex);

    if (_initialized)
        return true;

    // Initialize InterruptDatabase (WoW 12.0 spell data)
    InterruptDatabase::Initialize();

    _initTime = ::std::chrono::system_clock::now();
    _initialized = true;

    TC_LOG_INFO("playerbot.interrupt", "UnifiedInterruptSystem initialized with WoW 12.0 spell database");
    return true;
}

void UnifiedInterruptSystem::Shutdown()
{
    ::std::lock_guard lock(_mutex);

    if (!_initialized)
        return;

    // Clear all data structures
    _registeredBots.clear();
    _botAI.clear();
    _activeCasts.clear();
    _interruptHistory.clear();
    _rotationOrder.clear();
    _groupAssignments.clear();

    _initialized = false;

    TC_LOG_INFO("playerbot.interrupt", "UnifiedInterruptSystem shutdown - Total interrupts: {} (Success: {}, Failed: {})",
        _metrics.interruptAttempts.load(),
        _metrics.interruptSuccesses.load(),
        _metrics.interruptFailures.load());
}

void UnifiedInterruptSystem::Update(Player* bot, uint32 diff)
{
    if (!bot || !_initialized)
        return;

    ::std::lock_guard lock(_mutex);

    ObjectGuid botGuid = bot->GetGUID();
    // Update cooldowns
    auto it = _registeredBots.find(botGuid);
    if (it != _registeredBots.end())
    {
        if (it->second.cooldownRemaining > 0)
        {
            if (it->second.cooldownRemaining <= diff)
                it->second.cooldownRemaining = 0;
            else
                it->second.cooldownRemaining -= diff;
        }

        // Update availability
        it->second.available = (it->second.cooldownRemaining == 0);
    }

    // Clean up old cast entries (older than 30 seconds)
    uint32 currentTime = GameTime::GetGameTimeMS();
    for (auto castIt = _activeCasts.begin(); castIt != _activeCasts.end();)
    {
        if (currentTime - castIt->second.castStartTime > 30000)
            castIt = _activeCasts.erase(castIt);
        else
            ++castIt;
    }
}

// =====================================================================
// BOT REGISTRATION
// =====================================================================

void UnifiedInterruptSystem::RegisterBot(Player* bot, BotAI* ai)
{
    if (!bot || !ai)
        return;

    ::std::lock_guard lock(_mutex);

    ObjectGuid botGuid = bot->GetGUID();
    BotInterruptInfo info;
    info.botGuid = botGuid;
    info.available = true;
    info.cooldownRemaining = 0;

    // Find interrupt spells from bot's spell book
    auto const& spells = bot->GetSpellMap();
    for (auto const& [spellId, spellData] : spells)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
            continue;

        // Check if spell has interrupt effect
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spellInfo->GetEffect(SpellEffIndex(i)).Effect == SPELL_EFFECT_INTERRUPT_CAST)
            {
                if (info.spellId == 0)
                {
                    info.spellId = spellId;
                    info.interruptRange = spellInfo->GetMaxRange(false);
                }
                else
                {
                    info.backupSpellId = spellId;
                }
            }
        }

        // Check for stun/silence effects (alternative interrupts)
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spellInfo->GetEffect(SpellEffIndex(i)).Effect == SPELL_EFFECT_APPLY_AURA)
            {
                if (spellInfo->GetEffect(SpellEffIndex(i)).ApplyAuraName == SPELL_AURA_MOD_STUN ||
                    spellInfo->GetEffect(SpellEffIndex(i)).ApplyAuraName == SPELL_AURA_MOD_SILENCE)
                {
                    info.alternativeInterrupts.push_back(spellId);
                }
            }
        }
    }

    _registeredBots[botGuid] = info;
    _botAI[botGuid] = ai;
    TC_LOG_DEBUG("playerbot.interrupt", "Registered bot {} with interrupt spell {} (range: {:.1f})",
        bot->GetName(), info.spellId, info.interruptRange);
}

void UnifiedInterruptSystem::UnregisterBot(ObjectGuid botGuid)
{
    ::std::lock_guard lock(_mutex);

    _registeredBots.erase(botGuid);
    _botAI.erase(botGuid);

    // Remove from rotation orders
    _rotationOrder.erase(::std::remove(_rotationOrder.begin(), _rotationOrder.end(), botGuid), _rotationOrder.end());
    // Remove group assignments
    _groupAssignments.erase(botGuid);
}

// =====================================================================
// CAST DETECTION AND TRACKING
// =====================================================================

void UnifiedInterruptSystem::OnEnemyCastStart(Unit* caster, uint32 spellId, uint32 castTime)
{
    if (!caster || !_initialized)
        return;

    ::std::lock_guard lock(_mutex);

    ObjectGuid casterGuid = caster->GetGUID();
    CastingSpellInfo castInfo;
    castInfo.casterGuid = casterGuid;
    castInfo.spellId = spellId;
    castInfo.castStartTime = GameTime::GetGameTimeMS();
    castInfo.castEndTime = castInfo.castStartTime + castTime;
    castInfo.interrupted = false;

    // Get priority from InterruptDatabase
    castInfo.priority = GetSpellPriority(spellId);

    _activeCasts[casterGuid] = castInfo;
    _metrics.spellsDetected.fetch_add(1, ::std::memory_order_relaxed);

    TC_LOG_DEBUG("playerbot.interrupt", "Enemy cast started - Caster: {}, Spell: {}, CastTime: {}ms, Priority: {}",
        casterGuid.ToString(), spellId, castTime, static_cast<uint8>(castInfo.priority));
}

void UnifiedInterruptSystem::OnEnemyCastInterrupted(ObjectGuid casterGuid, uint32 spellId)
{
    ::std::lock_guard lock(_mutex);

    auto it = _activeCasts.find(casterGuid);
    if (it != _activeCasts.end())
    {
        it->second.interrupted = true;
        _metrics.interruptSuccesses.fetch_add(1, ::std::memory_order_relaxed);

        TC_LOG_DEBUG("playerbot.interrupt", "Cast interrupted - Caster: {}, Spell: {}",
            casterGuid.ToString(), spellId);
    }
}

void UnifiedInterruptSystem::OnEnemyCastComplete(ObjectGuid casterGuid, uint32 spellId)
{
    ::std::lock_guard lock(_mutex);

    auto it = _activeCasts.find(casterGuid);
    if (it != _activeCasts.end() && !it->second.interrupted)
    {
        // Cast completed without interrupt
        _activeCasts.erase(it);
    }
}

// =====================================================================
// ACTIVE CASTS TRACKING
// =====================================================================

::std::vector<CastingSpellInfo> UnifiedInterruptSystem::GetActiveCasts() const
{
    ::std::lock_guard lock(_mutex);

    ::std::vector<CastingSpellInfo> casts;
    casts.reserve(_activeCasts.size());

    for (auto const& [guid, castInfo] : _activeCasts)
    {
        if (!castInfo.interrupted)
            casts.push_back(castInfo);
    }

    return casts;
}

// =====================================================================
// SPELL DATABASE ACCESS
// =====================================================================

InterruptPriority UnifiedInterruptSystem::GetSpellPriority(uint32 spellId, uint8 mythicLevel)
{
    // Query InterruptDatabase for WoW 12.0 spell priority with M+ scaling
    return InterruptDatabase::GetSpellPriority(spellId, mythicLevel);
}

bool UnifiedInterruptSystem::ShouldAlwaysInterrupt(uint32 spellId)
{
    return InterruptDatabase::ShouldAlwaysInterrupt(spellId);
}

SpellInterruptConfig const* UnifiedInterruptSystem::GetSpellConfig(uint32 spellId)
{
    return InterruptDatabase::GetSpellConfig(spellId);
}

// =====================================================================
// DECISION MAKING AND PLANNING
// =====================================================================

::std::vector<UnifiedInterruptTarget> UnifiedInterruptSystem::ScanForInterruptTargets(Player* bot)
{
    if (!bot)
        return {};

    ::std::vector<UnifiedInterruptTarget> targets;

    ::std::lock_guard lock(_mutex);

    uint32 currentTime = GameTime::GetGameTimeMS();

    for (auto const& [casterGuid, castInfo] : _activeCasts)
    {
        if (castInfo.interrupted)
            continue;

        // Get caster unit
        Unit* caster = ObjectAccessor::GetUnit(*bot, casterGuid);
        if (!caster || !caster->IsAlive())
            continue;

        // Check if cast is still active
    if (currentTime >= castInfo.castEndTime)
            continue;

        // Build interrupt target
        UnifiedInterruptTarget target;
        target.casterGuid = casterGuid;
        target.spellId = castInfo.spellId;
        target.priority = castInfo.priority;
        target.castStartTime = castInfo.castStartTime;
        target.castEndTime = castInfo.castEndTime;
        target.remainingCastTime = castInfo.castEndTime - currentTime;
        target.distance = ::std::sqrt(bot->GetExactDistSq(caster)); // Calculate once from squared distance
        target.inLineOfSight = bot->IsWithinLOSInMap(caster);
        target.threatLevel = CalculateThreatLevel(castInfo);

        targets.push_back(target);
    }

    // Sort by priority and remaining time
    ::std::sort(targets.begin(), targets.end(), [](UnifiedInterruptTarget const& a, UnifiedInterruptTarget const& b)
    {
        if (a.priority != b.priority)
            return a.priority > b.priority;
        return a.remainingCastTime < b.remainingCastTime;
    });

    return targets;
}

UnifiedInterruptPlan UnifiedInterruptSystem::CreateInterruptPlan(Player* bot, UnifiedInterruptTarget const& target)
{
    if (!bot)
        return {};

    ::std::lock_guard lock(_mutex);

    ObjectGuid botGuid = bot->GetGUID();
    UnifiedInterruptPlan plan;
    plan.target = const_cast<UnifiedInterruptTarget*>(&target);

    // Find bot's interrupt capability
    auto botIt = _registeredBots.find(botGuid);
    if (botIt == _registeredBots.end())
        return plan;

    BotInterruptInfo const& botInfo = botIt->second;

    // Create capability structure
    UnifiedInterruptCapability capability;
    capability.botGuid = botGuid;
    capability.spellId = botInfo.spellId;
    capability.range = botInfo.interruptRange;
    capability.cooldownRemaining = botInfo.cooldownRemaining;
    capability.available = botInfo.available;
    capability.alternativeSpells = botInfo.alternativeInterrupts;
    plan.capability = &capability;

    // Determine interrupt method
    if (botInfo.spellId > 0 && botInfo.available)
    {
        plan.method = InterruptMethod::SPELL_INTERRUPT;
        plan.successProbability = 0.95f; // Base 95% success rate
    }
    else if (!botInfo.alternativeInterrupts.empty())
    {
        plan.method = InterruptMethod::STUN;
        plan.successProbability = 0.80f; // Stuns have lower success rate
    }
    else
    {
        plan.method = InterruptMethod::LINE_OF_SIGHT;
        plan.successProbability = 0.60f; // LOS breaking is unreliable
    }

    // Calculate execution timing
    Unit* caster = ObjectAccessor::GetUnit(*bot, target.casterGuid);
    if (caster)
    {
        float distance = ::std::sqrt(bot->GetExactDistSq(caster)); // Calculate once from squared distance

        // Check if movement required
    if (distance > botInfo.interruptRange)
        {
            plan.requiresMovement = true;

            // Calculate movement time
            float moveSpeed = bot->GetSpeed(MOVE_RUN);
            float moveDistance = distance - botInfo.interruptRange + 2.0f; // Move to 2y inside range
            float moveTime = moveDistance / moveSpeed;

            plan.executionTime = moveTime + 0.3f; // Add cast time

            // Calculate ideal position
            Position botPos = bot->GetPosition();
            Position casterPos = caster->GetPosition();
            float angle = botPos.GetAngle(&casterPos);

            plan.executionPosition.Relocate(
                casterPos.GetPositionX() + ::std::cos(angle) * (botInfo.interruptRange - 2.0f),
                casterPos.GetPositionY() + ::std::sin(angle) * (botInfo.interruptRange - 2.0f),
                casterPos.GetPositionZ()
            );
        }
        else
        {
            plan.requiresMovement = false;
            plan.executionTime = 0.3f; // Just cast time
            plan.executionPosition = bot->GetPosition();
        }

        // Add reaction time (250ms)
        plan.reactionTime = 0.25f;

        // Calculate priority
        plan.priority = CalculateInterruptPriority(target, capability);

        // Generate reasoning
        plan.reasoning = GeneratePlanReasoning(target, capability, plan);
    }

    return plan;
}

bool UnifiedInterruptSystem::ExecuteInterruptPlan(Player* bot, UnifiedInterruptPlan const& plan)
{
    if (!bot || !plan.target || !plan.capability)
        return false;

    ::std::lock_guard lock(_mutex);

    _metrics.interruptAttempts.fetch_add(1, ::std::memory_order_relaxed);

    // Get caster
    Unit* caster = ObjectAccessor::GetUnit(*bot, plan.target->casterGuid);
    if (!caster || !caster->IsAlive())
    {
        _metrics.interruptFailures.fetch_add(1, ::std::memory_order_relaxed);
        return false;
    }

    // Handle movement if required
    if (plan.requiresMovement)
    {
        if (!RequestInterruptPositioning(bot, caster))
        {
            _metrics.interruptFailures.fetch_add(1, ::std::memory_order_relaxed);
            return false;
        }

        _metrics.movementRequired.fetch_add(1, ::std::memory_order_relaxed);
    }

    // Execute based on method
    bool success = false;
    switch (plan.method)
    {
        case InterruptMethod::SPELL_INTERRUPT:
            success = ExecuteSpellInterrupt(bot, caster, plan.capability->spellId);
            break;

        case InterruptMethod::STUN:
            if (!plan.capability->alternativeSpells.empty())
                success = ExecuteSpellInterrupt(bot, caster, plan.capability->alternativeSpells[0]);
            break;

        case InterruptMethod::SILENCE:
            success = ExecuteSilence(bot, caster);
            break;

        case InterruptMethod::LINE_OF_SIGHT:
            success = RequestInterruptPositioning(bot, caster);
            break;

        case InterruptMethod::KNOCKBACK:
            success = ExecuteKnockback(bot, caster);
            break;

        case InterruptMethod::DISPEL:
            success = ExecuteDispel(bot, caster);
            break;

        default:
            break;
    }

    if (success)
    {
        // Record interrupt
        MarkInterruptUsed(bot->GetGUID(), plan.capability->spellId);

        // Add to history
        InterruptHistoryEntry entry;
        entry.timestamp = GameTime::GetGameTimeMS();
        entry.botGuid = bot->GetGUID();
        entry.targetGuid = plan.target->casterGuid;
        entry.spellId = plan.target->spellId;
        entry.interruptSpellId = plan.capability->spellId;
        entry.method = plan.method;
        entry.success = true;

        _interruptHistory.push_back(entry);

        TC_LOG_DEBUG("playerbot.interrupt", "Interrupt executed successfully - Bot: {}, Target: {}, Method: {}",
            bot->GetName(), plan.target->casterGuid.ToString(), static_cast<uint8>(plan.method));
    }
    else
    {
        _metrics.interruptFailures.fetch_add(1, ::std::memory_order_relaxed);
    }

    return success;
}

// =====================================================================
// GROUP COORDINATION
// =====================================================================

void UnifiedInterruptSystem::CoordinateGroupInterrupts(Group* group)
{
    if (!group)
        return;

    ::std::lock_guard lock(_mutex);

    _metrics.groupCoordinations.fetch_add(1, ::std::memory_order_relaxed);

    // Get all active casts
    ::std::vector<CastingSpellInfo> activeCasts;
    for (auto const& [guid, castInfo] : _activeCasts)
    {
        if (!castInfo.interrupted)
            activeCasts.push_back(castInfo);
    }

    // Sort by priority
    ::std::sort(activeCasts.begin(), activeCasts.end(), [](CastingSpellInfo const& a, CastingSpellInfo const& b)
    {
        if (a.priority != b.priority)
            return a.priority > b.priority;
        return a.castStartTime < b.castStartTime;
    });

    // Get available bots in group
    ::std::vector<ObjectGuid> availableBots;
    for (auto const& [botGuid, botInfo] : _registeredBots)
    {
        if (botInfo.available)
        {
            Player* bot = ObjectAccessor::GetPlayer(*group->GetLeader(), botGuid);
            if (bot && bot->GetGroup() == group)
                availableBots.push_back(botGuid);
        }
    }

    // Assign interrupts
    size_t botIndex = 0;
    for (auto const& castInfo : activeCasts)
    {
        if (botIndex >= availableBots.size())
            break;

        ObjectGuid assignedBot = availableBots[botIndex];

        BotInterruptAssignment assignment;
        assignment.targetGuid = castInfo.casterGuid;
        assignment.spellId = castInfo.spellId;
        assignment.assignedBotGuid = assignedBot;
        assignment.backupBotGuid = (botIndex + 1 < availableBots.size()) ? availableBots[botIndex + 1] : ObjectGuid::Empty;
        assignment.assignmentTime = GameTime::GetGameTimeMS();
        assignment.priority = castInfo.priority;
        assignment.executed = false;

        _groupAssignments[assignedBot] = assignment;

        botIndex++;
    }
}

bool UnifiedInterruptSystem::ShouldBotInterrupt(ObjectGuid botGuid, ObjectGuid& targetGuid, uint32& spellId)
{
    ::std::lock_guard lock(_mutex);

    auto it = _groupAssignments.find(botGuid);
    if (it == _groupAssignments.end())
        return false;

    BotInterruptAssignment& assignment = it->second;

    if (assignment.executed)
        return false;

    targetGuid = assignment.targetGuid;
    spellId = assignment.spellId;
    assignment.executed = true;

    return true;
}

BotInterruptAssignment UnifiedInterruptSystem::GetBotAssignment(ObjectGuid botGuid) const
{
    ::std::lock_guard lock(_mutex);

    auto it = _groupAssignments.find(botGuid);
    if (it != _groupAssignments.end())
        return it->second;

    return BotInterruptAssignment();
}

void UnifiedInterruptSystem::ClearAssignments(ObjectGuid groupGuid)
{
    ::std::lock_guard lock(_mutex);

    // Clear all assignments for bots in this group
    for (auto it = _groupAssignments.begin(); it != _groupAssignments.end();)
    {
        // Note: Would need group lookup to verify group membership
        // For now, just clear all assignments
        it = _groupAssignments.erase(it);
    }
}

// =====================================================================
// ROTATION SYSTEM
// =====================================================================

ObjectGuid UnifiedInterruptSystem::GetNextInRotation(Group* group)
{
    if (!group)
        return ObjectGuid::Empty;

    ::std::lock_guard lock(_mutex);

    ObjectGuid groupGuid = group->GetGUID();

    // Initialize rotation if needed
    if (_rotationOrder.find(groupGuid) == _rotationOrder.end())
    {
        ::std::vector<ObjectGuid> rotation;

        for (auto const& [botGuid, botInfo] : _registeredBots)
        {
            Player* bot = ObjectAccessor::GetPlayer(*group->GetLeader(), botGuid);
            if (bot && bot->GetGroup() == group && botInfo.available)
                rotation.push_back(botGuid);
        }

        _rotationOrder[groupGuid] = rotation;
        _rotationIndex[groupGuid] = 0;
    }

    auto& rotation = _rotationOrder[groupGuid];
    if (rotation.empty())
        return ObjectGuid::Empty;

    uint32& index = _rotationIndex[groupGuid];

    // Find next available bot
    uint32 attempts = 0;
    while (attempts < rotation.size())
    {
        ObjectGuid candidate = rotation[index];
        index = (index + 1) % rotation.size();

        auto it = _registeredBots.find(candidate);
        if (it != _registeredBots.end() && it->second.available)
            return candidate;

        attempts++;
    }

    return ObjectGuid::Empty;
}

void UnifiedInterruptSystem::MarkInterruptUsed(ObjectGuid botGuid, uint32 spellId)
{
    ::std::lock_guard lock(_mutex);

    auto it = _registeredBots.find(botGuid);
    if (it == _registeredBots.end())
        return;

    // Get spell cooldown
    auto aiIt = _botAI.find(botGuid);
    if (aiIt == _botAI.end())
        return;

    Player* bot = aiIt->second->GetBot();
    if (!bot)
        return;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return;

    uint32 cooldown = spellInfo->RecoveryTime;

    it->second.cooldownRemaining = cooldown;
    it->second.available = false;

    TC_LOG_DEBUG("playerbot.interrupt", "Bot {} used interrupt spell {} - Cooldown: {}ms",
        bot->GetName(), spellId, cooldown);
}

void UnifiedInterruptSystem::ResetRotation(ObjectGuid groupGuid)
{
    ::std::lock_guard lock(_mutex);

    _rotationIndex[groupGuid] = 0;
}

// =====================================================================
// FALLBACK LOGIC
// =====================================================================

bool UnifiedInterruptSystem::HandleFailedInterrupt(Player* bot, Unit* target, uint32 failedSpellId)
{
    if (!bot || !target)
        return false;

    ::std::lock_guard lock(_mutex);

    FallbackMethod method = SelectFallbackMethod(bot, target, failedSpellId);

    if (method == FallbackMethod::NONE)
        return false;

    bool success = ExecuteFallback(bot, target, method);

    if (success)
        _metrics.fallbacksUsed.fetch_add(1, ::std::memory_order_relaxed);

    return success;
}

FallbackMethod UnifiedInterruptSystem::SelectFallbackMethod(Player* bot, Unit* target, uint32 spellId)
{
    if (!bot || !target)
        return FallbackMethod::NONE;

    ::std::lock_guard lock(_mutex);

    ObjectGuid botGuid = bot->GetGUID();
    auto it = _registeredBots.find(botGuid);
    if (it == _registeredBots.end())
        return FallbackMethod::NONE;

    BotInterruptInfo const& botInfo = it->second;

    // Check available fallback options

    // 1. Try backup interrupt spell
    if (botInfo.backupSpellId > 0)
        return FallbackMethod::NONE; // Will be handled as normal interrupt

    // 2. Try stun
    for (uint32 altSpellId : botInfo.alternativeInterrupts)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(altSpellId, DIFFICULTY_NONE);
        if (!spellInfo)
            continue;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spellInfo->GetEffect(SpellEffIndex(i)).Effect == SPELL_EFFECT_APPLY_AURA &&
                spellInfo->GetEffect(SpellEffIndex(i)).ApplyAuraName == SPELL_AURA_MOD_STUN)
            {
                return FallbackMethod::STUN;
            }
        }
    }

    // 3. Try silence
    for (uint32 altSpellId : botInfo.alternativeInterrupts)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(altSpellId, DIFFICULTY_NONE);
        if (!spellInfo)
            continue;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spellInfo->GetEffect(SpellEffIndex(i)).Effect == SPELL_EFFECT_APPLY_AURA &&
                spellInfo->GetEffect(SpellEffIndex(i)).ApplyAuraName == SPELL_AURA_MOD_SILENCE)
            {
                return FallbackMethod::SILENCE;
            }
        }
    }

    // 4. Line of sight
    if (bot->IsWithinLOSInMap(target))
        return FallbackMethod::LINE_OF_SIGHT;

    // 5. Range (move out of spell range)
    return FallbackMethod::RANGE;
}

bool UnifiedInterruptSystem::ExecuteFallback(Player* bot, Unit* target, FallbackMethod method)
{
    if (!bot || !target)
        return false;

    switch (method)
    {
        case FallbackMethod::STUN:
            return ExecuteStun(bot, target);

        case FallbackMethod::SILENCE:
            return ExecuteSilence(bot, target);

        case FallbackMethod::LINE_OF_SIGHT:
            return ExecuteLOSBreak(bot, target);

        case FallbackMethod::RANGE:
            return ExecuteRangeEscape(bot, target);

        case FallbackMethod::DEFENSIVE:
            return ExecuteDefensiveCooldown(bot);

        case FallbackMethod::KNOCKBACK:
            return ExecuteKnockback(bot, target);

        default:
            return false;
    }
}

// =====================================================================
// MOVEMENT INTEGRATION
// =====================================================================

bool UnifiedInterruptSystem::RequestInterruptPositioning(Player* bot, Unit* target)
{
    if (!bot || !target)
        return false;

    ::std::lock_guard lock(_mutex);

    ObjectGuid botGuid = bot->GetGUID();
    auto aiIt = _botAI.find(botGuid);
    if (aiIt == _botAI.end())
        return false;

    BotAI* botAI = aiIt->second;
    if (!botAI || !botAI->GetUnifiedMovementCoordinator())
        return false;

    // Get interrupt range
    auto botIt = _registeredBots.find(botGuid);
    if (botIt == _registeredBots.end())
        return false;

    float interruptRange = botIt->second.interruptRange;

    // Calculate ideal position (inside interrupt range)
    Position botPos = bot->GetPosition();
    Position targetPos = target->GetPosition();
    float angle = botPos.GetAngle(&targetPos);

    Position idealPos;
    idealPos.Relocate(
        targetPos.GetPositionX() + ::std::cos(angle) * (interruptRange - 2.0f),
        targetPos.GetPositionY() + ::std::sin(angle) * (interruptRange - 2.0f),
        targetPos.GetPositionZ()
    );

    // Request movement with INTERRUPT_POSITIONING priority (220)
    return botAI->RequestPointMovement(
        PlayerBotMovementPriority::INTERRUPT_POSITIONING,
        idealPos,
        "Interrupt positioning",
        "UnifiedInterruptSystem"
    );
}

// =====================================================================
// METRICS AND STATISTICS
// =====================================================================

UnifiedInterruptMetrics UnifiedInterruptSystem::GetMetrics() const
{
    // Atomic loads are thread-safe without lock
    UnifiedInterruptMetrics metrics;
    metrics.spellsDetected.store(_metrics.spellsDetected.load(::std::memory_order_relaxed));
    metrics.interruptAttempts.store(_metrics.interruptAttempts.load(::std::memory_order_relaxed));
    metrics.interruptSuccesses.store(_metrics.interruptSuccesses.load(::std::memory_order_relaxed));
    metrics.interruptFailures.store(_metrics.interruptFailures.load(::std::memory_order_relaxed));
    metrics.fallbacksUsed.store(_metrics.fallbacksUsed.load(::std::memory_order_relaxed));
    metrics.movementRequired.store(_metrics.movementRequired.load(::std::memory_order_relaxed));
    metrics.groupCoordinations.store(_metrics.groupCoordinations.load(::std::memory_order_relaxed));
    metrics.rotationViolations.store(_metrics.rotationViolations.load(::std::memory_order_relaxed));
    return metrics;
}

BotInterruptStats UnifiedInterruptSystem::GetBotStats(ObjectGuid botGuid) const
{
    ::std::lock_guard lock(_mutex);

    BotInterruptStats stats;
    stats.botGuid = botGuid;

    // Count interrupts from history
    for (auto const& entry : _interruptHistory)
    {
        if (entry.botGuid == botGuid)
        {
            stats.totalInterrupts++;
            if (entry.success)
                stats.successfulInterrupts++;
            else
                stats.failedInterrupts++;
        }
    }

    // Calculate success rate
    if (stats.totalInterrupts > 0)
        stats.successRate = static_cast<float>(stats.successfulInterrupts) / static_cast<float>(stats.totalInterrupts);

    return stats;
}

::std::vector<InterruptHistoryEntry> UnifiedInterruptSystem::GetInterruptHistory(uint32 count) const
{
    ::std::lock_guard lock(_mutex);

    ::std::vector<InterruptHistoryEntry> history;

    if (count == 0)
    {
        history = _interruptHistory;
    }
    else
    {
        size_t startIndex = (_interruptHistory.size() > count) ? (_interruptHistory.size() - count) : 0;
        history.assign(_interruptHistory.begin() + startIndex, _interruptHistory.end());
    }

    return history;
}

void UnifiedInterruptSystem::ResetMetrics()
{
    ::std::lock_guard lock(_mutex);

    _metrics.spellsDetected.store(0, ::std::memory_order_relaxed);
    _metrics.interruptAttempts.store(0, ::std::memory_order_relaxed);
    _metrics.interruptSuccesses.store(0, ::std::memory_order_relaxed);
    _metrics.interruptFailures.store(0, ::std::memory_order_relaxed);
    _metrics.fallbacksUsed.store(0, ::std::memory_order_relaxed);
    _metrics.movementRequired.store(0, ::std::memory_order_relaxed);
    _metrics.groupCoordinations.store(0, ::std::memory_order_relaxed);
    _metrics.rotationViolations.store(0, ::std::memory_order_relaxed);

    _interruptHistory.clear();
}

// =====================================================================
// HELPER METHODS (Private)
// =====================================================================

uint32 UnifiedInterruptSystem::CalculateThreatLevel(CastingSpellInfo const& castInfo) const
{
    // Threat level based on priority and remaining time
    uint32 baseThreat = static_cast<uint32>(castInfo.priority) * 100;

    uint32 currentTime = GameTime::GetGameTimeMS();
    uint32 remainingTime = (castInfo.castEndTime > currentTime) ? (castInfo.castEndTime - currentTime) : 0;
    // Threat increases as cast nears completion
    if (remainingTime < 500)
        baseThreat += 500;
    else if (remainingTime < 1000)
        baseThreat += 300;
    else if (remainingTime < 2000)
        baseThreat += 100;

    return baseThreat;
}

uint32 UnifiedInterruptSystem::CalculateInterruptPriority(UnifiedInterruptTarget const& target, UnifiedInterruptCapability const& capability) const
{
    uint32 priority = static_cast<uint32>(target.priority) * 1000;

    // Higher priority if interrupt is immediately available
    if (capability.available)
        priority += 500;

    // Higher priority if target is closer
    if (target.distance < capability.range)
        priority += 300;

    // Higher priority if in line of sight
    if (target.inLineOfSight)
        priority += 200;

    // Urgency based on remaining cast time
    if (target.remainingCastTime < 500)
        priority += 400;
    else if (target.remainingCastTime < 1000)
        priority += 200;

    return priority;
}

::std::string UnifiedInterruptSystem::GeneratePlanReasoning(
    UnifiedInterruptTarget const& target,
    UnifiedInterruptCapability const& capability,
    UnifiedInterruptPlan const& plan) const
{
    ::std::ostringstream reasoning;

    reasoning << "Priority: " << static_cast<uint32>(target.priority);
    reasoning << ", Method: " << static_cast<uint32>(plan.method);
    reasoning << ", Distance: " << ::std::fixed << ::std::setprecision(1) << target.distance;
    reasoning << ", RemainingCast: " << target.remainingCastTime << "ms";

    if (plan.requiresMovement)
        reasoning << ", RequiresMovement";

    reasoning << ", Success: " << ::std::fixed << ::std::setprecision(0) << (plan.successProbability * 100.0f) << "%";

    return reasoning.str();
}

bool UnifiedInterruptSystem::ExecuteSpellInterrupt(Player* bot, Unit* target, uint32 spellId)
{
    if (!bot || !target || spellId == 0)
        return false;

    // Use SpellPacketBuilder for thread-safe packet-based execution
    SpellCastOptions options;
    options.validateRange = true;
    options.validateLOS = true;
    options.validateCooldown = true;

    auto result = SpellPacketBuilder::BuildCastSpellPacket(bot, spellId, target, options);

    return result.result == ValidationResult::SUCCESS;
}

bool UnifiedInterruptSystem::ExecuteStun(Player* bot, Unit* target)
{
    if (!bot || !target)
        return false;

    ::std::lock_guard lock(_mutex);

    ObjectGuid botGuid = bot->GetGUID();
    auto it = _registeredBots.find(botGuid);
    if (it == _registeredBots.end())
        return false;

    // Find stun spell
    for (uint32 spellId : it->second.alternativeInterrupts)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
            continue;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spellInfo->GetEffect(SpellEffIndex(i)).Effect == SPELL_EFFECT_APPLY_AURA &&
                spellInfo->GetEffect(SpellEffIndex(i)).ApplyAuraName == SPELL_AURA_MOD_STUN)
            {
                return ExecuteSpellInterrupt(bot, target, spellId);
            }
        }
    }

    return false;
}

bool UnifiedInterruptSystem::ExecuteSilence(Player* bot, Unit* target)
{
    if (!bot || !target)
        return false;

    ::std::lock_guard lock(_mutex);

    ObjectGuid botGuid = bot->GetGUID();
    auto it = _registeredBots.find(botGuid);
    if (it == _registeredBots.end())
        return false;

    // Find silence spell
    for (uint32 spellId : it->second.alternativeInterrupts)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
            continue;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spellInfo->GetEffect(SpellEffIndex(i)).Effect == SPELL_EFFECT_APPLY_AURA &&
                spellInfo->GetEffect(SpellEffIndex(i)).ApplyAuraName == SPELL_AURA_MOD_SILENCE)
            {
                return ExecuteSpellInterrupt(bot, target, spellId);
            }
        }
    }

    return false;
}

bool UnifiedInterruptSystem::ExecuteKnockback(Player* bot, Unit* target)
{
    if (!bot || !target)
        return false;

    // Find knockback spell in bot's spellbook
    auto const& spells = bot->GetSpellMap();
    for (auto const& [spellId, _] : spells)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
            continue;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spellInfo->GetEffect(SpellEffIndex(i)).Effect == SPELL_EFFECT_KNOCK_BACK)
            {
                return ExecuteSpellInterrupt(bot, target, spellId);
            }
        }
    }

    return false;
}

bool UnifiedInterruptSystem::ExecuteDispel(Player* bot, Unit* target)
{
    if (!bot || !target)
        return false;

    // Find dispel spell
    auto const& spells = bot->GetSpellMap();
    for (auto const& [spellId, _] : spells)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
            continue;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spellInfo->GetEffect(SpellEffIndex(i)).Effect == SPELL_EFFECT_DISPEL)
            {
                return ExecuteSpellInterrupt(bot, target, spellId);
            }
        }
    }

    return false;
}

bool UnifiedInterruptSystem::ExecuteLOSBreak(Player* bot, Unit* target)
{
    if (!bot || !target)
        return false;

    // Request movement to break LOS
    ::std::lock_guard lock(_mutex);

    ObjectGuid botGuid = bot->GetGUID();
    auto aiIt = _botAI.find(botGuid);
    if (aiIt == _botAI.end())
        return false;

    BotAI* botAI = aiIt->second;
    if (!botAI || !botAI->GetUnifiedMovementCoordinator())
        return false;

    // Find position behind obstacle
    Position targetPos = target->GetPosition();
    Position botPos = bot->GetPosition();
    float angle = targetPos.GetAngle(&botPos) + static_cast<float>(M_PI); // Opposite direction

    Position losPos;
    losPos.Relocate(
        botPos.GetPositionX() + ::std::cos(angle) * 10.0f,
        botPos.GetPositionY() + ::std::sin(angle) * 10.0f,
        botPos.GetPositionZ()
    );

    return botAI->RequestPointMovement(
        PlayerBotMovementPriority::INTERRUPT_POSITIONING,
        losPos,
        "Breaking LOS",
        "UnifiedInterruptSystem"
    );
}

bool UnifiedInterruptSystem::ExecuteRangeEscape(Player* bot, Unit* target)
{
    if (!bot || !target)
        return false;

    // Move away from target
    ::std::lock_guard lock(_mutex);

    ObjectGuid botGuid = bot->GetGUID();
    auto aiIt = _botAI.find(botGuid);
    if (aiIt == _botAI.end())
        return false;

    BotAI* botAI = aiIt->second;
    if (!botAI || !botAI->GetUnifiedMovementCoordinator())
        return false;

    Position targetPos = target->GetPosition();
    Position botPos = bot->GetPosition();
    float angle = targetPos.GetAngle(&botPos) + static_cast<float>(M_PI); // Away from target

    Position escapePos;
    escapePos.Relocate(
        botPos.GetPositionX() + ::std::cos(angle) * 20.0f,
        botPos.GetPositionY() + ::std::sin(angle) * 20.0f,
        botPos.GetPositionZ()
    );

    return botAI->RequestPointMovement(
        PlayerBotMovementPriority::DEFENSIVE_MOVEMENT,
        escapePos,
        "Escaping spell range",
        "UnifiedInterruptSystem"
    );
}

bool UnifiedInterruptSystem::ExecuteDefensiveCooldown(Player* bot)
{
    if (!bot)
        return false;

    // Find defensive cooldown based on class
    uint32 defensiveSpell = 0;

    switch (bot->GetClass())
    {
        case CLASS_WARRIOR:
            defensiveSpell = 871; // Shield Wall
            break;
        case CLASS_PALADIN:
            defensiveSpell = 642; // Divine Shield
            break;
        case CLASS_ROGUE:
            defensiveSpell = 5277; // Evasion
            break;
        case CLASS_MAGE:
            defensiveSpell = 45438; // Ice Block
            break;
        default:
            return false;
    }

    SpellCastOptions options;
    options.validateCooldown = true;

    auto result = SpellPacketBuilder::BuildCastSpellPacket(bot, defensiveSpell, bot, options);
    return result.result == ValidationResult::SUCCESS;
}

} // namespace Playerbot
