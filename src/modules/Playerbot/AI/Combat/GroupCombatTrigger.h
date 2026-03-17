/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_GROUP_COMBAT_TRIGGER_H
#define PLAYERBOT_GROUP_COMBAT_TRIGGER_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Triggers/Trigger.h"
#include "ObjectGuid.h"
#include <chrono>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <mutex>

// Forward declarations
class Player;
class Group;
class Unit;

namespace Playerbot
{

// Forward declarations
class BotAI;

/**
 * @class GroupCombatTrigger
 * @brief Critical trigger for group combat synchronization
 *
 * This trigger enables bots to engage combat when their group leader or members
 * enter combat. It provides fast combat detection and target sharing across
 * group members with optimized performance for bot-scale operations.
 *
 * Performance Requirements:
 * - Combat detection: <3 seconds from leader engagement
 * - Target switching: <1 second response time
 * - Memory usage: <0.5MB additional per bot
 * - CPU usage: <0.01% additional per bot
 */
class TC_GAME_API GroupCombatTrigger : public CombatTrigger
{
public:
    /**
     * Constructor
     * @param name Trigger name for identification
     */
    explicit GroupCombatTrigger(::std::string const& name = "group_combat");

    /**
     * Destructor
     */
    virtual ~GroupCombatTrigger() = default;

    // Core trigger interface
    /**
     * Check if trigger conditions are met
     * @param ai The bot AI instance
     * @return true if group is in combat and bot should engage
     */
    bool Check(BotAI* ai) const;

    /**
     * Evaluate trigger and generate action result
     * @param ai The bot AI instance
     * @return TriggerResult with suggested action and context
     */
    TriggerResult Evaluate(BotAI* ai) const;

    /**
     * Calculate urgency of combat engagement
     * @param ai The bot AI instance
     * @return Urgency value 0.0-1.0 (higher = more urgent)
     */
    float CalculateUrgency(BotAI* ai) const;

    // Group combat detection
    /**
     * Check if any group member is in combat
     * @param group The group to check
     * @return true if group has members in combat
     */
    bool IsGroupInCombat(Group* group) const;

    /**
     * Determine if bot should engage combat based on group state
     * @param bot The bot player
     * @param group The bot's group
     * @return true if bot should enter combat
     */
    bool ShouldEngageCombat(Player* bot, Group* group) const;

    /**
     * Get the primary target of the group
     * @param group The group to check
     * @return The unit being targeted by most group members
     */
    Unit* GetGroupTarget(Group* group) const;

    /**
     * Get the leader's current combat target
     * @param group The group to check
     * @return The unit being attacked by group leader
     */
    Unit* GetLeaderTarget(Group* group) const;

    /**
     * Get assist target based on group focus
     * @param bot The bot player
     * @param group The bot's group
     * @return Best target for assistance
     */
    Unit* GetAssistTarget(Player* bot, Group* group) const;

    // Combat state management
    /**
     * Track combat state changes for the group
     * @param group The group to track
     * @param inCombat New combat state
     */
    void UpdateGroupCombatState(Group* group, bool inCombat);

    /**
     * Get time since group entered combat
     * @param group The group to check
     * @return Milliseconds since combat started
     */
    uint32 GetCombatDuration(Group* group) const;

    /**
     * Check if bot is within engagement range
     * @param bot The bot player
     * @param target The potential target
     * @return true if bot can engage target
     */
    bool IsInEngagementRange(Player* bot, Unit* target) const;

    // Target validation
    /**
     * Validate if target is appropriate for group combat
     * @param bot The bot player
     * @param target The potential target
     * @return true if target is valid for engagement
     */
    bool IsValidGroupTarget(Player* bot, Unit* target) const;

    /**
     * Check if target is already engaged by group
     * @param group The group to check
     * @param target The target to validate
     * @return true if group members are attacking target
     */
    bool IsTargetEngaged(Group* group, Unit* target) const;

    // Performance optimization
    /**
     * Set update interval for combat checking
     * @param intervalMs Interval in milliseconds (min: 100ms)
     */
    void SetUpdateInterval(uint32 intervalMs);

    /**
     * Get current update interval
     * @return Update interval in milliseconds
     */
    uint32 GetUpdateInterval() const { return _updateIntervalMs; }

    /**
     * Enable or disable combat caching
     * @param enable true to enable caching
     */
    void SetCachingEnabled(bool enable) { _cachingEnabled = enable; }

    // Configuration
    /**
     * Set maximum engagement range
     * @param range Range in yards (default: 40)
     */
    void SetMaxEngagementRange(float range) { _maxEngagementRange = range; }

    /**
     * Set engagement delay after leader enters combat
     * @param delayMs Delay in milliseconds (default: 1000)
     */
    void SetEngagementDelay(uint32 delayMs) { _engagementDelayMs = delayMs; }

    /**
     * Set whether to prioritize leader's target
     * @param prioritize true to always assist leader first
     */
    void SetPrioritizeLeader(bool prioritize) { _prioritizeLeader = prioritize; }

    // Statistics and monitoring
    struct CombatStats
    {
        uint32 totalEngagements = 0;
        uint32 groupCombatTriggers = 0;
        uint32 leaderAssists = 0;
        uint32 targetSwitches = 0;
        ::std::chrono::milliseconds averageEngagementTime{0};
        ::std::chrono::steady_clock::time_point lastEngagement;
    };

    /**
     * Get current combat statistics
     * @return Combat stats for monitoring
     */
    CombatStats const& GetStats() const { return _stats; }

    /**
     * Reset combat statistics
     */
    void ResetStats() { _stats = CombatStats{}; }

private:
    // Internal data structures
    struct GroupCombatInfo
    {
        bool inCombat = false;
        ObjectGuid primaryTarget;
        ::std::chrono::steady_clock::time_point combatStartTime;
        ::std::chrono::steady_clock::time_point lastUpdateTime;
        uint32 membersInCombat = 0;
        ::std::unordered_map<ObjectGuid, ObjectGuid> memberTargets; // member -> target mapping
    };

    // Internal methods
    /**
     * Update cached combat information
     * @param group The group to update
     * @return true if cache was updated
     */
    bool UpdateCombatCache(Group* group) const;

    /**
     * Check if cache is still valid
     * @param group The group to check
     * @return true if cache is within update interval
     */
    bool IsCacheValid(Group* group) const;

    /**
     * Calculate target priority based on group focus
     * @param group The group to analyze
     * @param target The target to score
     * @return Priority score (higher = better target)
     */
    float CalculateTargetPriority(Group* group, Unit* target) const;

    /**
     * Count group members attacking specific target
     * @param group The group to check
     * @param target The target to count
     * @return Number of members attacking target
     */
    uint32 CountMembersOnTarget(Group* group, Unit* target) const;

    /**
     * Log combat event for debugging
     * @param event Event description
     * @param bot The bot involved
     * @param target Optional target involved
     */
    void LogCombatEvent(::std::string const& event, Player* bot, Unit* target = nullptr) const;

    /**
     * Update statistics after combat engagement
     * @param assistingLeader Whether bot is assisting leader
     * @param engagementTime Time taken to engage
     */
    void UpdateStatistics(bool assistingLeader, ::std::chrono::milliseconds engagementTime);

    // Member variables
    mutable ::std::unordered_map<ObjectGuid, GroupCombatInfo> _combatCache; // Group combat state cache
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _cacheMutex;                                       // Thread safety for cache

    // Configuration
    uint32 _updateIntervalMs = 500;                    // Cache update interval (500ms default)
    uint32 _engagementDelayMs = 1000;                  // Delay before engaging (1s default)
    float _maxEngagementRange = 40.0f;                 // Maximum range to engage targets
    bool _cachingEnabled = true;                       // Enable combat state caching
    bool _prioritizeLeader = true;                     // Prioritize leader's target
    float _minimumThreatThreshold = 0.0f;              // Minimum threat to consider target

    // Statistics
    mutable CombatStats _stats;                        // Performance statistics

    // Constants
    static constexpr uint32 MIN_UPDATE_INTERVAL = 100;    // Minimum 100ms update
    static constexpr uint32 MAX_UPDATE_INTERVAL = 2000;   // Maximum 2s update
    static constexpr uint32 CACHE_EXPIRY_TIME = 5000;    // Cache expires after 5s
    static constexpr float MIN_ENGAGEMENT_RANGE = 5.0f;   // Minimum 5 yard range
    static constexpr float MAX_ENGAGEMENT_RANGE = 100.0f; // Maximum 100 yard range
    static constexpr uint32 MAX_ENGAGEMENT_DELAY = 5000;  // Maximum 5s delay
};

} // namespace Playerbot

#endif // PLAYERBOT_GROUP_COMBAT_TRIGGER_H