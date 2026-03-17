/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_TARGET_ASSIST_ACTION_H
#define PLAYERBOT_TARGET_ASSIST_ACTION_H

#include "Define.h"
#include "Action.h"
#include "ObjectGuid.h"
#include <chrono>
#include <memory>
#include <atomic>
#include <unordered_map>

// Forward declarations
class Player;
class Unit;
class Group;
class Spell;

namespace Playerbot
{

// Forward declarations
class BotAI;

/**
 * @class TargetAssistAction
 * @brief Critical action for coordinated group targeting
 *
 * This action enables bots to assist group members by attacking the same target,
 * with priority given to the group leader's target. It provides fast target
 * switching and smart target selection for optimal group coordination.
 *
 * Performance Requirements:
 * - Target switching: <1 second response time
 * - Target validation: <100ms per check
 * - Memory usage: <100KB per bot
 * - CPU usage: <0.01% per bot during combat
 */
class TC_GAME_API TargetAssistAction : public CombatAction
{
public:
    /**
     * Constructor
     * @param name Action name for identification
     */
    explicit TargetAssistAction(::std::string const& name = "target_assist");

    /**
     * Destructor
     */
    virtual ~TargetAssistAction() = default;

    // Core action interface
    /**
     * Check if action is possible to execute
     * @param ai The bot AI instance
     * @return true if bot can assist a target
     */
    bool IsPossible(BotAI* ai) const;

    /**
     * Check if action is useful in current context
     * @param ai The bot AI instance
     * @return true if assistance would be beneficial
     */
    bool IsUseful(BotAI* ai) const;

    /**
     * Execute the target assist action
     * @param ai The bot AI instance
     * @param context Action context with target information
     * @return ActionResult indicating success or failure
     */
    ActionResult Execute(BotAI* ai, ActionContext const& context);

    // Target selection methods
    /**
     * Get the group leader's current target
     * @param leader The group leader
     * @return The unit being attacked by leader
     */
    Unit* GetLeaderTarget(Player* leader) const;

    /**
     * Get the best assist target from group
     * @param bot The bot player
     * @param group The bot's group
     * @return Optimal target for assistance
     */
    Unit* GetBestAssistTarget(Player* bot, Group* group) const;

    /**
     * Check if target is valid for assistance
     * @param target The potential target
     * @param bot The bot player
     * @return true if target can be assisted
     */
    bool IsValidAssistTarget(Unit* target, Player* bot) const;

    /**
     * Check if bot should switch to new target
     * @param bot The bot player
     * @param newTarget The potential new target
     * @return true if switching is beneficial
     */
    bool ShouldSwitchTarget(Player* bot, Unit* newTarget) const;

    // Combat coordination
    /**
     * Initiate attack on assist target
     * @param bot The bot player
     * @param target The target to attack
     * @return true if attack was initiated
     */
    bool EngageTarget(Player* bot, Unit* target);

    /**
     * Switch from current to new target
     * @param bot The bot player
     * @param newTarget The new target to attack
     * @return true if switch was successful
     */
    bool SwitchTarget(Player* bot, Unit* newTarget);

    /**
     * Calculate priority of assisting specific target
     * @param bot The bot player
     * @param target The target to evaluate
     * @param group The bot's group
     * @return Priority score (higher = more important)
     */
    float CalculateAssistPriority(Player* bot, Unit* target, Group* group) const;

    // Range and positioning
    /**
     * Check if bot is in range to assist
     * @param bot The bot player
     * @param target The target to check
     * @return true if bot can reach target
     */
    bool IsInAssistRange(Player* bot, Unit* target) const;

    /**
     * Get optimal position for assisting
     * @param bot The bot player
     * @param target The target being attacked
     * @param x Output X coordinate
     * @param y Output Y coordinate
     * @param z Output Z coordinate
     * @return true if position was calculated
     */
    bool GetAssistPosition(Player* bot, Unit* target, float& x, float& y, float& z) const;

    /**
     * Move bot to assist position
     * @param bot The bot player
     * @param target The target to assist against
     * @return true if movement was initiated
     */
    bool MoveToAssistPosition(Player* bot, Unit* target);

    // Target validation
    /**
     * Check if target is being attacked by group
     * @param target The target to check
     * @param group The group to check
     * @return true if group members are attacking target
     */
    bool IsTargetUnderAttack(Unit* target, Group* group) const;

    /**
     * Count group members attacking target
     * @param target The target to check
     * @param group The group to count
     * @return Number of members attacking target
     */
    uint32 CountAssistingMembers(Unit* target, Group* group) const;

    /**
     * Check if target has crowd control
     * @param target The target to check
     * @return true if target is crowd controlled
     */
    bool IsTargetCrowdControlled(Unit* target) const;

    // Configuration
    /**
     * Set maximum assist range
     * @param range Range in yards (default: 40)
     */
    void SetMaxAssistRange(float range) { _maxAssistRange = range; }

    /**
     * Set target switch delay
     * @param delayMs Delay in milliseconds (default: 1000)
     */
    void SetSwitchDelay(uint32 delayMs) { _switchDelayMs = delayMs; }

    /**
     * Set whether to prioritize leader's target
     * @param prioritize true to always assist leader first
     */
    void SetPrioritizeLeader(bool prioritize) { _prioritizeLeader = prioritize; }

    /**
     * Set minimum health percentage to switch targets
     * @param healthPct Health percentage threshold
     */
    void SetSwitchHealthThreshold(float healthPct) { _switchHealthThreshold = healthPct; }

    // Statistics and monitoring
    struct AssistStats
    {
        uint32 totalAssists = 0;
        uint32 leaderAssists = 0;
        uint32 targetSwitches = 0;
        uint32 failedAssists = 0;
        ::std::chrono::milliseconds averageSwitchTime{0};
        ::std::chrono::steady_clock::time_point lastAssist;
    };

    /**
     * Get current assist statistics
     * @return Assist stats for monitoring
     */
    AssistStats const& GetStats() const { return _stats; }

    /**
     * Reset assist statistics
     */
    void ResetStats() { _stats = AssistStats{}; }

    // Combat role integration
    /**
     * Get appropriate range based on bot's class
     * @param bot The bot player
     * @return Optimal combat range for class
     */
    float GetClassAssistRange(Player* bot) const;

    /**
     * Check if bot's class should assist
     * @param bot The bot player
     * @return true if class role includes assisting
     */
    bool ShouldClassAssist(Player* bot) const;

private:
    // Internal data structures
    struct TargetInfo
    {
        ObjectGuid guid;
        float priority = 0.0f;
        uint32 assistingCount = 0;
        float distance = 0.0f;
        float healthPct = 100.0f;
        bool isLeaderTarget = false;
        ::std::chrono::steady_clock::time_point firstSeen;
    };

    // Internal methods
    /**
     * Evaluate all potential targets
     * @param bot The bot player
     * @param group The bot's group
     * @return Map of target GUIDs to their info
     */
    ::std::unordered_map<ObjectGuid, TargetInfo> EvaluateTargets(Player* bot, Group* group) const;

    /**
     * Select best target from evaluated list
     * @param targets Map of evaluated targets
     * @param bot The bot player
     * @return GUID of best target
     */
    ObjectGuid SelectBestTarget(::std::unordered_map<ObjectGuid, TargetInfo> const& targets, Player* bot) const;

    /**
     * Calculate threat adjustment for target
     * @param bot The bot player
     * @param target The target to evaluate
     * @return Threat modifier for prioritization
     */
    float CalculateThreatFactor(Player* bot, Unit* target) const;

    /**
     * Check if bot can interrupt target
     * @param bot The bot player
     * @param target The target casting
     * @return true if bot has interrupt available
     */
    bool CanInterruptTarget(Player* bot, Unit* target) const;

    /**
     * Log assist action for debugging
     * @param action Action description
     * @param bot The bot involved
     * @param target The target involved
     */
    void LogAssistAction(::std::string const& action, Player* bot, Unit* target) const;

    /**
     * Update statistics after assist
     * @param assistedLeader Whether assisting leader
     * @param switchTime Time taken to switch
     */
    void UpdateStatistics(bool assistedLeader, ::std::chrono::milliseconds switchTime);

    /**
     * Check line of sight to target
     * @param bot The bot player
     * @param target The target to check
     * @return true if bot has LOS to target
     */
    bool HasLineOfSight(Player* bot, Unit* target) const;

    /**
     * Get facing requirement for class
     * @param bot The bot player
     * @return true if class needs to face target
     */
    bool RequiresFacingForClass(Player* bot) const;

    // Member variables
    ::std::unordered_map<ObjectGuid, ::std::chrono::steady_clock::time_point> _lastTargetSwitch; // Track switch times

    // Configuration
    float _maxAssistRange = 40.0f;                     // Maximum range to assist
    uint32 _switchDelayMs = 1000;                      // Delay before switching targets
    bool _prioritizeLeader = true;                     // Always assist leader's target first
    float _switchHealthThreshold = 20.0f;              // Switch if current target below this health
    float _minThreatThreshold = 0.0f;                  // Minimum threat to consider target
    bool _avoidCrowdControlled = true;                 // Don't assist CC'd targets

    // Statistics
    mutable AssistStats _stats;                        // Performance statistics

    // Constants
    static constexpr float MELEE_ASSIST_RANGE = 8.0f;     // Melee classes assist range
    static constexpr float RANGED_ASSIST_RANGE = 35.0f;   // Ranged classes assist range
    static constexpr float HEALER_ASSIST_RANGE = 40.0f;   // Healer assist range (rarely used)
    static constexpr uint32 MIN_SWITCH_DELAY = 500;       // Minimum 500ms between switches
    static constexpr uint32 MAX_SWITCH_DELAY = 5000;      // Maximum 5s between switches
    static constexpr float LEADER_PRIORITY_BONUS = 20.0f; // Priority bonus for leader's target
    static constexpr float LOW_HEALTH_BONUS = 10.0f;      // Priority bonus for low health targets
};

} // namespace Playerbot

#endif // PLAYERBOT_TARGET_ASSIST_ACTION_H