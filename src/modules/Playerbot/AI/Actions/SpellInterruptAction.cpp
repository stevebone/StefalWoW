/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#include "SpellInterruptAction.h"
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellHistory.h"
#include "MotionMaster.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "../BotAI.h"
#include "Core/PlayerBotHelpers.h"
#include "../Combat/InterruptCoordinator.h"
#include <algorithm>
#include <thread>

namespace Playerbot
{

SpellInterruptAction::SpellInterruptAction()
    : Action("spell_interrupt")
    , _lastExecution(::std::chrono::steady_clock::now())
{
}

ActionResult SpellInterruptAction::Execute(BotAI* ai, ActionContext const& context)
{
    if (!ai)
        return ActionResult::FAILED;

    auto executionStart = ::std::chrono::steady_clock::now();
    ::std::lock_guard lock(_executionMutex);

    // Check for pending interrupt assignments from coordinator
    ::std::vector<InterruptAssignment> assignments = GetPendingAssignments(ai);
    if (assignments.empty())
    {
        TC_LOG_DEBUG("playerbot", "SpellInterruptAction: No pending interrupt assignments for bot %s",
                     ai->GetBot()->GetName().c_str());
        return ActionResult::FAILED;
    }

    // Process the highest priority assignment
    const InterruptAssignment& assignment = assignments[0];

    // Build interrupt context
    InterruptContext interruptContext;
    interruptContext.targetCaster = assignment.targetCaster;
    interruptContext.targetSpell = assignment.targetSpell;
    interruptContext.interruptSpell = assignment.interruptSpell;

    // Get target unit
    ::Unit* target = GetInterruptTarget(ai, assignment.targetCaster);
    if (!target)
    {
        ReportInterruptResult(ai, interruptContext, false, "Target not found");
        return ActionResult::FAILED;
    }

    // Update context with current target information
    if (Player* bot = ai->GetBot())
    {
        interruptContext.targetDistance = ::std::sqrt(bot->GetExactDistSq(target)); // Calculate once from squared distance
    }

    // Execute the interrupt
    ActionResult result = ExecuteInterrupt(ai, interruptContext);

    // Update performance metrics
    {
        ::std::lock_guard metricsLock(_metricsMutex);
        _metrics.totalAttempts++;

        auto executionTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(
            ::std::chrono::steady_clock::now() - executionStart);

        // Rolling average
        _metrics.averageExecutionTime = ::std::chrono::microseconds(
            (_metrics.averageExecutionTime.count() * 9 + executionTime.count()) / 10);

        if (result == ActionResult::SUCCESS)
            _metrics.successfulInterrupts++;
    }

    _lastExecution = ::std::chrono::steady_clock::now();
    _executionCount++;

    return result;
}

bool SpellInterruptAction::IsPossible(BotAI* ai) const
{
    if (!ai || !ai->GetBot())
        return false;

    // Must be in combat or have interrupt assignments
    if (!ai->IsInCombat() && GetPendingAssignments(ai).empty())
        return false;

    // Must have interrupt abilities available
    uint32 bestInterrupt = GetBestInterruptSpell(ai, nullptr);
    if (bestInterrupt == 0)
        return false;

    // Check if interrupt is off cooldown
    return IsInterruptAvailable(ai, bestInterrupt);
}

uint32 SpellInterruptAction::GetPriority(BotAI* ai) const
{
    if (!IsPossible(ai))
        return 0;

    ::std::vector<InterruptAssignment> assignments = GetPendingAssignments(ai);
    if (assignments.empty())
        return 0;

    // Base priority + urgency factor
    uint32 basePriority = 800;

    // Higher priority for critical interrupts
    for (const auto& assignment : assignments)
    {
        // Calculate urgency based on time remaining
        uint32 timeRemaining = assignment.GetTimeUntilDeadline();
        if (timeRemaining < 500) // Less than 500ms remaining
            basePriority += 200;
        else if (timeRemaining < 1000) // Less than 1s remaining
            basePriority += 100;
    }

    return basePriority;
}

float SpellInterruptAction::GetRelevance(BotAI* ai) const
{
    if (!IsPossible(ai))
        return 0.0f;

    ::std::vector<InterruptAssignment> assignments = GetPendingAssignments(ai);
    if (assignments.empty())
        return 0.0f;

    // Base relevance for having assignments
    float relevance = 0.8f;

    // Increase relevance based on assignment urgency
    for (const auto& assignment : assignments)
    {
        uint32 timeRemaining = assignment.GetTimeUntilDeadline();
        if (timeRemaining < 1000) // Very urgent
            relevance = ::std::min(1.0f, relevance + 0.2f);
    }

    return relevance;
}

ActionResult SpellInterruptAction::ExecuteInterrupt(BotAI* ai, InterruptContext const& interruptContext)
{
    if (!ai || !interruptContext.IsValid())
        return ActionResult::FAILED;

    Player* bot = ai->GetBot();
    if (!bot)
        return ActionResult::FAILED;

    // Get target unit
    ::Unit* target = GetInterruptTarget(ai, interruptContext.targetCaster);
    if (!target)
    {
        ReportInterruptResult(ai, interruptContext, false, "Target not found");
        return ActionResult::FAILED;
    }

    // Validate target is still casting the spell we want to interrupt
    if (!IsTargetCastingInterruptible(target))
    {
        ReportInterruptResult(ai, interruptContext, false, "Target not casting interruptible spell");
        return ActionResult::FAILED;
    }

    // Check if we need to move to range
    const SpellInfo* interruptSpell = sSpellMgr->GetSpellInfo(interruptContext.interruptSpell, DIFFICULTY_NONE);
    if (!interruptSpell)
    {
        ReportInterruptResult(ai, interruptContext, false, "Invalid interrupt spell");
        return ActionResult::FAILED;
    }

    float requiredRange = interruptSpell->GetMaxRange();
    if (requiredRange == 0.0f)
        requiredRange = 5.0f; // Default melee range

    // Move to range if necessary
    if (!IsInInterruptRange(ai, target, requiredRange))
    {
        ActionResult moveResult = MoveToInterruptRange(ai, target, requiredRange);
        if (moveResult != ActionResult::SUCCESS)
        {
            ::std::lock_guard lock(_metricsMutex);
            _metrics.movementFailures++;
            ReportInterruptResult(ai, interruptContext, false, "Failed to move to range");
            return moveResult;
        }
    }

    // Wait for optimal timing
    if (!IsOptimalInterruptTime(interruptContext))
    {
        WaitForOptimalTiming(interruptContext);
    }

    // Execute the interrupt based on class
    ActionResult castResult = CastInterrupt(ai, target, interruptContext.interruptSpell);

    // Report result
    bool success = (castResult == ActionResult::SUCCESS);
    ::std::string resultReason = success ? "Success" : "Cast failed";
    ReportInterruptResult(ai, interruptContext, success, resultReason);

    return castResult;
}

bool SpellInterruptAction::CanExecuteInterrupt(BotAI* ai, InterruptContext const& interruptContext) const
{
    if (!ai || !interruptContext.IsValid())
        return false;

    // Check if interrupt spell is available
    if (!IsInterruptAvailable(ai, interruptContext.interruptSpell))
        return false;

    // Check if target is valid
    ::Unit* target = GetInterruptTarget(ai, interruptContext.targetCaster);
    if (!target || !IsValidInterruptTarget(target))
        return false;

    // Check if target is casting something we can interrupt
    return IsTargetCastingInterruptible(target);
}

uint32 SpellInterruptAction::GetBestInterruptSpell(BotAI* ai, ::Unit* target) const
{
    if (!ai)
        return 0;

    Player* bot = ai->GetBot();
    if (!bot)
        return 0;

    Classes playerClass = static_cast<Classes>(bot->GetClass());
    float targetDistance = target ? ::std::sqrt(bot->GetExactDistSq(static_cast<const WorldObject*>(target))) : 0.0f; // Calculate once from squared distance

    // Return best available interrupt for this class
    switch (playerClass)
    {
        case CLASS_WARRIOR:
            if (IsInterruptAvailable(ai, ClassInterrupts::WARRIOR_PUMMEL))
                return ClassInterrupts::WARRIOR_PUMMEL;
            if (IsInterruptAvailable(ai, ClassInterrupts::WARRIOR_SHIELD_BASH))
                return ClassInterrupts::WARRIOR_SHIELD_BASH;
            break;

        case CLASS_ROGUE:
            if (IsInterruptAvailable(ai, ClassInterrupts::ROGUE_KICK))
                return ClassInterrupts::ROGUE_KICK;
            break;

        case CLASS_MAGE:
            if (targetDistance <= 40.0f && IsInterruptAvailable(ai, ClassInterrupts::MAGE_COUNTERSPELL))
                return ClassInterrupts::MAGE_COUNTERSPELL;
            break;

        case CLASS_DEATH_KNIGHT:
            if (IsInterruptAvailable(ai, ClassInterrupts::DEATH_KNIGHT_MIND_FREEZE))
                return ClassInterrupts::DEATH_KNIGHT_MIND_FREEZE;
            break;

        case CLASS_SHAMAN:
            if (IsInterruptAvailable(ai, ClassInterrupts::SHAMAN_WIND_SHEAR))
                return ClassInterrupts::SHAMAN_WIND_SHEAR;
            break;

        case CLASS_HUNTER:
            if (targetDistance <= 40.0f && IsInterruptAvailable(ai, ClassInterrupts::HUNTER_COUNTER_SHOT))
                return ClassInterrupts::HUNTER_COUNTER_SHOT;
            break;

        case CLASS_PALADIN:
            if (IsInterruptAvailable(ai, ClassInterrupts::PALADIN_REBUKE))
                return ClassInterrupts::PALADIN_REBUKE;
            break;

        case CLASS_PRIEST:
            if (targetDistance <= 30.0f && IsInterruptAvailable(ai, ClassInterrupts::PRIEST_SILENCE))
                return ClassInterrupts::PRIEST_SILENCE;
            break;

        case CLASS_WARLOCK:
            if (targetDistance <= 30.0f && IsInterruptAvailable(ai, ClassInterrupts::WARLOCK_SPELL_LOCK))
                return ClassInterrupts::WARLOCK_SPELL_LOCK;
            break;

        case CLASS_MONK:
            if (IsInterruptAvailable(ai, ClassInterrupts::MONK_SPEAR_HAND))
                return ClassInterrupts::MONK_SPEAR_HAND;
            break;

        case CLASS_DRUID:
            if (targetDistance <= 30.0f && IsInterruptAvailable(ai, ClassInterrupts::DRUID_SOLAR_BEAM))
                return ClassInterrupts::DRUID_SOLAR_BEAM;
            if (IsInterruptAvailable(ai, ClassInterrupts::DRUID_SKULL_BASH))
                return ClassInterrupts::DRUID_SKULL_BASH;
            break;

        case CLASS_DEMON_HUNTER:
            if (targetDistance <= 20.0f && IsInterruptAvailable(ai, ClassInterrupts::DEMON_HUNTER_DISRUPT))
                return ClassInterrupts::DEMON_HUNTER_DISRUPT;
            break;

        case CLASS_EVOKER:
            if (targetDistance <= 25.0f && IsInterruptAvailable(ai, ClassInterrupts::EVOKER_QUELL))
                return ClassInterrupts::EVOKER_QUELL;
            break;

        default:
            break;
    }

    return 0;
}

bool SpellInterruptAction::IsInterruptAvailable(BotAI* ai, uint32 interruptSpell) const
{
    if (!ai)
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    // Check if bot knows the spell
    if (!bot->HasSpell(interruptSpell))
        return false;

    // Check cooldown
    const SpellInfo* interruptSpellInfo = sSpellMgr->GetSpellInfo(interruptSpell, DIFFICULTY_NONE);
    if (interruptSpellInfo && bot->GetSpellHistory()->GetRemainingCooldown(interruptSpellInfo) > ::std::chrono::milliseconds(0))
        return false;

    // Check resources (most interrupts are free, but some might require mana/rage/etc)
    const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(interruptSpell, DIFFICULTY_NONE);
    if (spellInfo)
    {
        ::std::vector<SpellPowerCost> costs = spellInfo->CalcPowerCost(bot, spellInfo->GetSchoolMask());
        for (const auto& cost : costs)
        {
            if (bot->GetPower(cost.Power) < cost.Amount)
                return false;
        }
    }

    return true;
}

uint32 SpellInterruptAction::CalculateOptimalTiming(InterruptContext const& context) const
{
    // For channeled spells, interrupt quickly
    if (context.isChanneled)
        return 200; // 200ms delay

    // For cast spells, interrupt near the end but with safety margin
    uint32 castTime = context.remainingCastTime;
    if (castTime <= 500)
        return 100; // Very short casts, interrupt quickly

    // Standard timing - interrupt with 200-300ms remaining
    return ::std::max(200u, castTime - 300u);
}

void SpellInterruptAction::SetInterruptCoordinator(::std::shared_ptr<InterruptCoordinator> coordinator)
{
    _coordinator = coordinator;
}

::std::vector<InterruptAssignment> SpellInterruptAction::GetPendingAssignments(BotAI* ai) const
{
    ::std::vector<InterruptAssignment> assignments;

    if (auto coordinator = _coordinator.lock())
    {
        if (ai && ai->GetBot())
        {
            ObjectGuid botGuid = ai->GetBot()->GetGUID();
            auto pendingAssignments = coordinator->GetPendingAssignments();

            for (const auto& assignment : pendingAssignments)
            {
                if (assignment.assignedBot == botGuid && !assignment.executed)
                {
                    assignments.push_back(assignment);
                }
            }

            // Sort by execution deadline (most urgent first)
            ::std::sort(assignments.begin(), assignments.end(),
                     [](const InterruptAssignment& a, const InterruptAssignment& b) {
                         return a.executionDeadline < b.executionDeadline;
                     });
        }
    }

    return assignments;
}

void SpellInterruptAction::ReportInterruptResult(BotAI* ai, InterruptContext const& context, bool success, ::std::string const& reason)
{
    if (auto coordinator = _coordinator.lock())
    {
        if (ai && ai->GetBot())
        {
            ObjectGuid botGuid = ai->GetBot()->GetGUID();
            coordinator->OnInterruptExecuted(botGuid, context.targetCaster, context.interruptSpell, success);

            if (!success && !reason.empty())
            {
                coordinator->OnInterruptFailed(botGuid, context.targetCaster, context.interruptSpell, reason);
            }
        }
    }

    TC_LOG_DEBUG("playerbot", "SpellInterruptAction: Bot %s interrupt result - Spell: %u, Target: %s, Success: %s, Reason: %s",
                 ai && ai->GetBot() ? ai->GetBot()->GetName().c_str() : "unknown",
                 context.interruptSpell,
                 context.targetCaster.ToString().c_str(),
                 success ? "true" : "false",
                 reason.c_str());
}

ActionResult SpellInterruptAction::MoveToInterruptRange(BotAI* ai, ::Unit* target, float requiredRange)
{
    if (!ai || !target)
        return ActionResult::FAILED;

    Player* bot = ai->GetBot();
    if (!bot)
        return ActionResult::FAILED;

    float currentDistance = ::std::sqrt(bot->GetExactDistSq(target)); // Calculate once from squared distance
    if (currentDistance <= requiredRange + RANGE_TOLERANCE)
        return ActionResult::SUCCESS;

    // Calculate position to move to
    float deltaX = bot->GetPositionX() - target->GetPositionX();
    float deltaY = bot->GetPositionY() - target->GetPositionY();
    float angle = atan2(deltaY, deltaX);

    float newX = target->GetPositionX() + cos(angle) * (requiredRange - 1.0f);
    float newY = target->GetPositionY() + sin(angle) * (requiredRange - 1.0f);
    float newZ = target->GetPositionZ();
    // Move to position with validated pathfinding
    Position dest(newX, newY, newZ, 0.0f);
    if (BotAI* ai = GetBotAI(bot))
    {
        if (!ai->MoveTo(dest, true))
        {
            // Fallback to legacy if validation fails
            bot->GetMotionMaster()->MovePoint(0, newX, newY, newZ);
        }
    }
    else
    {
        // Non-bot player - use standard movement
        bot->GetMotionMaster()->MovePoint(0, newX, newY, newZ);
    }

    // Wait a short time for movement to start
    auto moveStart = ::std::chrono::steady_clock::now();
    float rangeCheckSq = (requiredRange + RANGE_TOLERANCE) * (requiredRange + RANGE_TOLERANCE);
    while (::std::chrono::steady_clock::now() - moveStart < ::std::chrono::milliseconds(100))
    {
        if (bot->GetExactDistSq(target) <= rangeCheckSq)
            return ActionResult::SUCCESS;

        ::std::this_thread::sleep_for(::std::chrono::milliseconds(10));
    }

    // Check if we made progress
    float newDistance = ::std::sqrt(bot->GetExactDistSq(target)); // Calculate once from squared distance
    if (newDistance < currentDistance && newDistance <= requiredRange + RANGE_TOLERANCE)
        return ActionResult::SUCCESS;

    // If we couldn't get in range quickly, this will be a partial success
    // The bot can continue trying to move while attempting the interrupt
    return newDistance < currentDistance ? ActionResult::SUCCESS : ActionResult::FAILED;
}
bool SpellInterruptAction::IsInInterruptRange(BotAI* ai, ::Unit* target, float requiredRange) const
{
    if (!ai || !target)
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    float rangeCheckSq = (requiredRange + RANGE_TOLERANCE) * (requiredRange + RANGE_TOLERANCE);
    return bot->GetExactDistSq(target) <= rangeCheckSq;
}

bool SpellInterruptAction::IsValidInterruptTarget(::Unit* target) const
{
    if (!target)
        return false;

    // Must be alive
    if (!target->IsAlive())
        return false;

    // Must be casting
    if (!target->HasUnitState(UNIT_STATE_CASTING))
        return false;

    // Must be an enemy (or neutral that's attacking us)
    // This check would depend on the specific implementation of faction relationships

    return true;
}

bool SpellInterruptAction::IsTargetCastingInterruptible(::Unit* target) const
{
    if (!target || !target->HasUnitState(UNIT_STATE_CASTING))
        return false;

    // Get current spell being cast
    if (Spell* currentSpell = target->GetCurrentSpell(CURRENT_GENERIC_SPELL))
    {
        const SpellInfo* spellInfo = currentSpell->GetSpellInfo();
        if (spellInfo)
        {
            // Check if spell can be interrupted
            return spellInfo->CanBeInterrupted(nullptr, target);
        }
    }

    // Check channeled spell
    if (Spell* channeledSpell = target->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
    {
        const SpellInfo* spellInfo = channeledSpell->GetSpellInfo();
        if (spellInfo)
        {
            return spellInfo->CanBeInterrupted(nullptr, target);
        }
    }

    return false;
}

::Unit* SpellInterruptAction::GetInterruptTarget(BotAI* ai, ObjectGuid targetGuid) const
{
    if (!ai || targetGuid.IsEmpty())
        return nullptr;

    // Try to find the target in various ways
    if (::Unit* target = ObjectAccessor::GetUnit(*ai->GetBot(), targetGuid))
        return target;

    return nullptr;
}

ActionResult SpellInterruptAction::CastInterrupt(BotAI* ai, ::Unit* target, uint32 interruptSpell)
{
    if (!ai || !target)
        return ActionResult::FAILED;

    Player* bot = ai->GetBot();
    if (!bot)
        return ActionResult::FAILED;

    // Validate the interrupt can be cast
    if (!ValidateInterruptCast(ai, target, interruptSpell))
        return ActionResult::FAILED;

    // Execute class-specific interrupt
    Classes playerClass = static_cast<Classes>(bot->GetClass());
    switch (playerClass)
    {
        case CLASS_WARRIOR:
            return ExecuteWarriorInterrupt(ai, target, interruptSpell);
        case CLASS_ROGUE:
            return ExecuteRogueInterrupt(ai, target, interruptSpell);
        case CLASS_MAGE:
            return ExecuteMageInterrupt(ai, target, interruptSpell);
        case CLASS_DEATH_KNIGHT:
            return ExecuteDeathKnightInterrupt(ai, target, interruptSpell);
        case CLASS_SHAMAN:
            return ExecuteShamanInterrupt(ai, target, interruptSpell);
        case CLASS_HUNTER:
            return ExecuteHunterInterrupt(ai, target, interruptSpell);
        case CLASS_PALADIN:
            return ExecutePaladinInterrupt(ai, target, interruptSpell);
        case CLASS_PRIEST:
            return ExecutePriestInterrupt(ai, target, interruptSpell);
        case CLASS_WARLOCK:
            return ExecuteWarlockInterrupt(ai, target, interruptSpell);
        case CLASS_MONK:
            return ExecuteMonkInterrupt(ai, target, interruptSpell);
        case CLASS_DRUID:
            return ExecuteDruidInterrupt(ai, target, interruptSpell);
        case CLASS_DEMON_HUNTER:
            return ExecuteDemonHunterInterrupt(ai, target, interruptSpell);
        case CLASS_EVOKER:
            return ExecuteEvokerInterrupt(ai, target, interruptSpell);
        default:
            return ActionResult::FAILED;
    }
}

bool SpellInterruptAction::ValidateInterruptCast(BotAI* ai, ::Unit* target, uint32 interruptSpell) const
{
    if (!ai || !target)
        return false;

    Player* bot = ai->GetBot();
    if (!bot)
        return false;

    // Check if we can cast the interrupt
    if (!IsInterruptAvailable(ai, interruptSpell))
        return false;

    // Check if target is still valid
    if (!IsValidInterruptTarget(target))
        return false;

    // Check line of sight
    if (!bot->IsWithinLOSInMap(target))
        return false;

    return true;
}

// Class-specific interrupt implementations
ActionResult SpellInterruptAction::ExecuteWarriorInterrupt(BotAI* ai, ::Unit* target, uint32 spellId)
{
    Player* bot = ai->GetBot();
    if (!bot || !target)
        return ActionResult::FAILED;

    // Set target and cast interrupt
    bot->SetTarget(target->GetGUID());

    if (bot->CastSpell(CastSpellTargetArg(target), spellId))
        return ActionResult::SUCCESS;

    return ActionResult::FAILED;
}

ActionResult SpellInterruptAction::ExecuteRogueInterrupt(BotAI* ai, ::Unit* target, uint32 spellId)
{
    return ExecuteWarriorInterrupt(ai, target, spellId); // Same basic logic
}

ActionResult SpellInterruptAction::ExecuteMageInterrupt(BotAI* ai, ::Unit* target, uint32 spellId)
{
    Player* bot = ai->GetBot();
    if (!bot || !target)
        return ActionResult::FAILED;

    // Mage counterspell is ranged
    bot->SetTarget(target->GetGUID());

    if (bot->CastSpell(CastSpellTargetArg(target), spellId))
        return ActionResult::SUCCESS;

    return ActionResult::FAILED;
}

ActionResult SpellInterruptAction::ExecuteDeathKnightInterrupt(BotAI* ai, ::Unit* target, uint32 spellId)
{
    return ExecuteWarriorInterrupt(ai, target, spellId); // Same basic logic
}

ActionResult SpellInterruptAction::ExecuteShamanInterrupt(BotAI* ai, ::Unit* target, uint32 spellId)
{
    return ExecuteWarriorInterrupt(ai, target, spellId); // Same basic logic
}

ActionResult SpellInterruptAction::ExecuteHunterInterrupt(BotAI* ai, ::Unit* target, uint32 spellId)
{
    return ExecuteMageInterrupt(ai, target, spellId); // Ranged interrupt like mage
}

ActionResult SpellInterruptAction::ExecutePaladinInterrupt(BotAI* ai, ::Unit* target, uint32 spellId)
{
    return ExecuteWarriorInterrupt(ai, target, spellId); // Same basic logic
}

ActionResult SpellInterruptAction::ExecutePriestInterrupt(BotAI* ai, ::Unit* target, uint32 spellId)
{
    return ExecuteMageInterrupt(ai, target, spellId); // Ranged interrupt
}

ActionResult SpellInterruptAction::ExecuteWarlockInterrupt(BotAI* ai, ::Unit* target, uint32 spellId)
{
    return ExecuteMageInterrupt(ai, target, spellId); // Pet or ranged interrupt
}

ActionResult SpellInterruptAction::ExecuteMonkInterrupt(BotAI* ai, ::Unit* target, uint32 spellId)
{
    return ExecuteWarriorInterrupt(ai, target, spellId); // Same basic logic
}

ActionResult SpellInterruptAction::ExecuteDruidInterrupt(BotAI* ai, ::Unit* target, uint32 spellId)
{
    if (spellId == ClassInterrupts::DRUID_SOLAR_BEAM)
        return ExecuteMageInterrupt(ai, target, spellId); // Ranged
    else
        return ExecuteWarriorInterrupt(ai, target, spellId); // Melee
}

ActionResult SpellInterruptAction::ExecuteDemonHunterInterrupt(BotAI* ai, ::Unit* target, uint32 spellId)
{
    return ExecuteMageInterrupt(ai, target, spellId); // Ranged interrupt
}

ActionResult SpellInterruptAction::ExecuteEvokerInterrupt(BotAI* ai, ::Unit* target, uint32 spellId)
{
    return ExecuteMageInterrupt(ai, target, spellId); // Ranged interrupt
}

bool SpellInterruptAction::IsOptimalInterruptTime(InterruptContext const& context) const
{
    uint32 optimalTiming = CalculateOptimalTiming(context);
    uint32 remainingTime = context.remainingCastTime;

    // Check if we're within the optimal timing window
    return remainingTime <= optimalTiming + TIMING_PRECISION_MS &&
           remainingTime >= optimalTiming - TIMING_PRECISION_MS;
}

void SpellInterruptAction::WaitForOptimalTiming(InterruptContext const& context) const
{
    uint32 optimalTiming = CalculateOptimalTiming(context);
    uint32 remainingTime = context.remainingCastTime;

    if (remainingTime > optimalTiming)
    {
        uint32 waitTime = remainingTime - optimalTiming;
        if (waitTime <= 1000) // Don't wait more than 1 second
        {
            ::std::this_thread::sleep_for(::std::chrono::milliseconds(waitTime));
        }
    }
}

void SpellInterruptAction::HandleInterruptFailure(BotAI* ai, InterruptContext const& context, ::std::string const& reason)
{
    TC_LOG_DEBUG("playerbot", "SpellInterruptAction: Interrupt failed for bot %s - %s",
                 ai && ai->GetBot() ? ai->GetBot()->GetName().c_str() : "unknown", reason.c_str());

    // Update failure metrics based on reason
    ::std::lock_guard lock(_metricsMutex);
    if (reason.find("range") != ::std::string::npos)
        _metrics.rangeFailures++;
    else if (reason.find("cooldown") != ::std::string::npos)
        _metrics.cooldownFailures++;
    else if (reason.find("timing") != ::std::string::npos)
        _metrics.timingFailures++;
    else if (reason.find("movement") != ::std::string::npos)
        _metrics.movementFailures++;
}

bool SpellInterruptAction::CanRetryInterrupt(InterruptContext const& context) const
{
    // Can retry if there's still time and the target is still casting
    return context.remainingCastTime > MIN_CAST_TIME_MS;
}

void SpellInterruptAction::OptimizeForFrequency()
{
    // Periodically optimize for high-frequency execution
    if (_executionCount % 100 == 0)
    {
        // Reset metrics periodically to prevent overflow
        ::std::lock_guard lock(_metricsMutex);
        if (_metrics.totalAttempts > 10000)
        {
            // Scale down all metrics by half
            _metrics.totalAttempts /= 2;
            _metrics.successfulInterrupts /= 2;
            _metrics.movementFailures /= 2;
            _metrics.timingFailures /= 2;
            _metrics.cooldownFailures /= 2;
            _metrics.rangeFailures /= 2;
        }
    }
}

bool SpellInterruptAction::ShouldSkipLowPriorityInterrupt(BotAI* ai, InterruptContext const& context) const
{
    // Skip low priority interrupts if we're busy with high priority actions
    if (context.priority >= 4) // Low priority
    {
        // Check if bot is busy with more important tasks
    if (ai && ai->IsInCombat())
        {
            // Could check for other high priority actions here
            return false; // For now, don't skip any interrupts in combat
        }
    }

    return false;
}

} // namespace Playerbot