/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Action.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include <chrono>
#include <memory>
#include <mutex>
#include <vector>

class Unit;
class Player;

namespace Playerbot
{

class InterruptCoordinatorFixed;
struct InterruptAssignment;

// Type alias for backward compatibility
using InterruptCoordinator = InterruptCoordinatorFixed;

/**
 * Context data for spell interrupt actions
 */
struct InterruptContext
{
    ObjectGuid targetCaster;        // Unit casting the spell to interrupt
    uint32 targetSpell = 0;         // Spell ID being cast
    uint32 interruptSpell = 0;      // Interrupt ability to use
    float targetDistance = 0.0f;    // Distance to target
    uint32 remainingCastTime = 0;   // Time left on target cast
    bool isChanneled = false;       // Whether target spell is channeled
    uint32 priority = 0;            // Interrupt priority (1=critical, 5=ignore)

    InterruptContext() = default;

    InterruptContext(ObjectGuid caster, uint32 spell, uint32 interrupt)
        : targetCaster(caster), targetSpell(spell), interruptSpell(interrupt) {}

    bool IsValid() const
    {
        return !targetCaster.IsEmpty() && targetSpell != 0 && interruptSpell != 0;
    }
};

/**
 * Action for executing spell interrupts in coordinated group scenarios
 *
 * This action handles the execution of interrupt abilities assigned by the
 * InterruptCoordinator, including movement, targeting, and timing optimization.
 *
 * Features:
 * - Automatic movement to interrupt range
 * - Precise timing to maximize interrupt success rate
 * - Integration with InterruptCoordinator for reporting results
 * - Support for all class interrupt abilities
 * - Performance optimized for high-frequency execution
 */
class TC_GAME_API SpellInterruptAction : public Action
{
public:
    SpellInterruptAction();
    ~SpellInterruptAction() override = default;

    // === Action Interface Implementation ===

    ActionResult Execute(BotAI* ai, ActionContext const& context);
    bool IsPossible(BotAI* ai) const;
    uint32 GetPriority(BotAI* ai) const;
    float GetRelevance(BotAI* ai) const;

    // Resource requirements - customizable behavior
    float GetCooldown() const override { return 0.0f; } // Varies by interrupt spell

    // === Interrupt-Specific Interface ===

    // Execute specific interrupt with context
    ActionResult ExecuteInterrupt(BotAI* ai, InterruptContext const& interruptContext);

    // Check if bot can execute a specific interrupt
    bool CanExecuteInterrupt(BotAI* ai, InterruptContext const& interruptContext) const;

    // Get the best available interrupt for a target
    uint32 GetBestInterruptSpell(BotAI* ai, ::Unit* target) const;

    // Check interrupt spell availability and cooldown
    bool IsInterruptAvailable(BotAI* ai, uint32 interruptSpell) const;

    // Calculate optimal interrupt timing
    uint32 CalculateOptimalTiming(InterruptContext const& context) const;

    // === Integration Methods ===

    // Set the interrupt coordinator for result reporting
    void SetInterruptCoordinator(::std::shared_ptr<InterruptCoordinator> coordinator);

    // Get pending interrupt assignments for this bot
    ::std::vector<InterruptAssignment> GetPendingAssignments(BotAI* ai) const;

    // Report interrupt execution result
    void ReportInterruptResult(BotAI* ai, InterruptContext const& context, bool success, ::std::string const& reason = "");

protected:
    // === Internal Implementation ===

    // Movement and positioning
    ActionResult MoveToInterruptRange(BotAI* ai, ::Unit* target, float requiredRange);
    bool IsInInterruptRange(BotAI* ai, ::Unit* target, float requiredRange) const;

    // Target validation
    bool IsValidInterruptTarget(::Unit* target) const;
    bool IsTargetCastingInterruptible(::Unit* target) const;
    ::Unit* GetInterruptTarget(BotAI* ai, ObjectGuid targetGuid) const;

    // Interrupt execution
    ActionResult CastInterrupt(BotAI* ai, ::Unit* target, uint32 interruptSpell);
    bool ValidateInterruptCast(BotAI* ai, ::Unit* target, uint32 interruptSpell) const;

    // Class-specific interrupt handling
    ActionResult ExecuteWarriorInterrupt(BotAI* ai, ::Unit* target, uint32 spellId);
    ActionResult ExecuteRogueInterrupt(BotAI* ai, ::Unit* target, uint32 spellId);
    ActionResult ExecuteMageInterrupt(BotAI* ai, ::Unit* target, uint32 spellId);
    ActionResult ExecuteDeathKnightInterrupt(BotAI* ai, ::Unit* target, uint32 spellId);
    ActionResult ExecuteShamanInterrupt(BotAI* ai, ::Unit* target, uint32 spellId);
    ActionResult ExecuteHunterInterrupt(BotAI* ai, ::Unit* target, uint32 spellId);
    ActionResult ExecutePaladinInterrupt(BotAI* ai, ::Unit* target, uint32 spellId);
    ActionResult ExecutePriestInterrupt(BotAI* ai, ::Unit* target, uint32 spellId);
    ActionResult ExecuteWarlockInterrupt(BotAI* ai, ::Unit* target, uint32 spellId);
    ActionResult ExecuteMonkInterrupt(BotAI* ai, ::Unit* target, uint32 spellId);
    ActionResult ExecuteDruidInterrupt(BotAI* ai, ::Unit* target, uint32 spellId);
    ActionResult ExecuteDemonHunterInterrupt(BotAI* ai, ::Unit* target, uint32 spellId);
    ActionResult ExecuteEvokerInterrupt(BotAI* ai, ::Unit* target, uint32 spellId);

    // Timing and precision
    bool IsOptimalInterruptTime(InterruptContext const& context) const;
    void WaitForOptimalTiming(InterruptContext const& context) const;

    // Error handling and recovery
    void HandleInterruptFailure(BotAI* ai, InterruptContext const& context, ::std::string const& reason);
    bool CanRetryInterrupt(InterruptContext const& context) const;

    // Performance optimization
    void OptimizeForFrequency();
    bool ShouldSkipLowPriorityInterrupt(BotAI* ai, InterruptContext const& context) const;

private:
    // Interrupt coordinator integration
    ::std::weak_ptr<InterruptCoordinator> _coordinator;

    // Execution tracking
    ::std::chrono::steady_clock::time_point _lastExecution;
    uint32 _executionCount = 0;

    // Performance metrics
    struct ExecutionMetrics
    {
        uint32 totalAttempts = 0;
        uint32 successfulInterrupts = 0;
        uint32 movementFailures = 0;
        uint32 timingFailures = 0;
        uint32 cooldownFailures = 0;
        uint32 rangeFailures = 0;
        ::std::chrono::microseconds averageExecutionTime{0};

        float GetSuccessRate() const
        {
            return totalAttempts > 0 ? static_cast<float>(successfulInterrupts) / totalAttempts : 0.0f;
        }
    } _metrics;

    // Configuration
    static constexpr uint32 MAX_INTERRUPT_RANGE = 40;    // Maximum interrupt range in yards
    static constexpr uint32 MIN_CAST_TIME_MS = 100;     // Minimum cast time to attempt interrupt
    static constexpr uint32 TIMING_PRECISION_MS = 50;   // Timing precision for interrupt execution
    static constexpr uint32 MOVEMENT_TIMEOUT_MS = 3000; // Max time to spend moving to range
    static constexpr float RANGE_TOLERANCE = 0.5f;      // Range tolerance for positioning

    // Spell ID mappings for class interrupts
    struct ClassInterrupts
    {
        static constexpr uint32 WARRIOR_PUMMEL = 6552;
        static constexpr uint32 WARRIOR_SHIELD_BASH = 72;

        static constexpr uint32 ROGUE_KICK = 1766;
        static constexpr uint32 ROGUE_KIDNEY_SHOT = 408;

        static constexpr uint32 MAGE_COUNTERSPELL = 2139;

        static constexpr uint32 DEATH_KNIGHT_MIND_FREEZE = 47528;
        static constexpr uint32 DEATH_KNIGHT_DEATH_GRIP = 49576;

        static constexpr uint32 SHAMAN_WIND_SHEAR = 57994;

        static constexpr uint32 HUNTER_COUNTER_SHOT = 147362;
        static constexpr uint32 HUNTER_TRANQ_SHOT = 19801;

        static constexpr uint32 PALADIN_REBUKE = 96231;

        static constexpr uint32 PRIEST_SILENCE = 15487;
        static constexpr uint32 PRIEST_PSYCHIC_SCREAM = 8122;

        static constexpr uint32 WARLOCK_SPELL_LOCK = 19647;
        static constexpr uint32 WARLOCK_DEATH_COIL = 6789;

        static constexpr uint32 MONK_SPEAR_HAND = 116705;

        static constexpr uint32 DRUID_SOLAR_BEAM = 78675;
        static constexpr uint32 DRUID_SKULL_BASH = 80964;

        static constexpr uint32 DEMON_HUNTER_DISRUPT = 183752;
        static constexpr uint32 DEMON_HUNTER_CHAOS_NOVA = 179057;

        static constexpr uint32 EVOKER_QUELL = 351338;
    };

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _metricsMutex;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _executionMutex;

    // Deleted operations
    SpellInterruptAction(SpellInterruptAction const&) = delete;
    SpellInterruptAction& operator=(SpellInterruptAction const&) = delete;
    SpellInterruptAction(SpellInterruptAction&&) = delete;
    SpellInterruptAction& operator=(SpellInterruptAction&&) = delete;
};

} // namespace Playerbot