/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file InstanceBotSlot.h
 * @brief Data structure representing a single bot slot in the Instance Bot Pool
 *
 * The InstanceBotSlot tracks all information about a bot in the pool:
 * - Identity (GUID, account, name)
 * - Classification (pool type, role, class, spec)
 * - Current state and assignment
 * - Timing information for cooldowns
 *
 * Thread Safety:
 * - Individual slots are NOT thread-safe
 * - Thread safety is provided by the owning InstanceBotPool
 * - All modifications should be done through InstanceBotPool methods
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "PoolSlotState.h"
#include "StringFormat.h"
#include "Character/ZoneLevelHelper.h"
#include <chrono>
#include <string>

namespace Playerbot
{

/**
 * @brief Data structure representing a single bot slot in the Instance Bot Pool
 *
 * This structure contains all information needed to manage a bot's lifecycle
 * within the warm pool and elastic overflow system.
 */
struct TC_GAME_API InstanceBotSlot
{
    // ========================================================================
    // IDENTITY
    // ========================================================================

    ObjectGuid botGuid;                 ///< The bot's unique identifier
    uint32 accountId = 0;               ///< Account ID the bot belongs to
    std::string botName;                ///< Character name for logging/display

    // ========================================================================
    // CLASSIFICATION
    // ========================================================================

    PoolType poolType = PoolType::PvE;          ///< Which pool this bot belongs to
    BotRole role = BotRole::DPS;                ///< Tank/Healer/DPS classification
    uint8 playerClass = 0;                      ///< WoW class ID (1=Warrior, 2=Paladin, etc.)
    uint32 specId = 0;                          ///< Specialization ID
    Faction faction = Faction::Alliance;        ///< Alliance or Horde
    ExpansionTier bracket = ExpansionTier::TheWarWithin;  ///< Level bracket

    // ========================================================================
    // STATS
    // ========================================================================

    uint32 level = 80;                  ///< Character level
    uint32 gearScore = 0;               ///< Item level / gear score
    uint32 healthMax = 0;               ///< Max health (for quick validation)
    uint32 manaMax = 0;                 ///< Max mana (for casters)

    // ========================================================================
    // STATE
    // ========================================================================

    PoolSlotState state = PoolSlotState::Empty;     ///< Current lifecycle state
    std::chrono::steady_clock::time_point stateChangeTime;    ///< When state last changed
    std::chrono::steady_clock::time_point lastAssignment;     ///< When last assigned to instance

    // ========================================================================
    // ASSIGNMENT TRACKING
    // ========================================================================

    uint32 currentInstanceId = 0;       ///< Instance ID if assigned (0 if not)
    uint32 currentContentId = 0;        ///< Dungeon/Raid/BG ID if assigned
    InstanceType currentInstanceType = InstanceType::Dungeon; ///< Type of current instance
    uint32 reservationId = 0;           ///< Reservation ID if reserved (0 if not)
    uint32 assignmentCount = 0;         ///< Total lifetime assignments
    ObjectGuid humanPlayerGuid;         ///< Human player this bot is associated with (for BG invitation tracking)

    // ========================================================================
    // PERFORMANCE METRICS
    // ========================================================================

    uint32 totalInstanceTime = 0;       ///< Total seconds spent in instances
    uint32 successfulCompletions = 0;   ///< Instances completed successfully
    uint32 earlyExits = 0;              ///< Times removed before completion

    // ========================================================================
    // METHODS - State Queries
    // ========================================================================

    /**
     * @brief Check if bot is available for new assignment
     * @return true if bot can be assigned to instance
     */
    bool IsAvailable() const
    {
        return IsAvailableState(state);
    }

    /**
     * @brief Check if bot can be assigned to specific instance
     * @param instanceId Target instance ID
     * @return true if bot can be assigned
     */
    bool CanAssignTo(uint32 instanceId) const
    {
        // Must be in Ready state
        if (!IsAvailable())
            return false;

        // Cannot assign to the instance we're on cooldown from
        // (This prevents immediate re-entry after leaving)
        if (currentInstanceId == instanceId && state == PoolSlotState::Cooldown)
            return false;

        return true;
    }

    /**
     * @brief Check if bot matches role requirement
     * @param requiredRole Required role
     * @return true if bot has the required role
     */
    bool MatchesRole(BotRole requiredRole) const
    {
        return role == requiredRole;
    }

    /**
     * @brief Check if bot is in level range
     * @param targetLevel Target level to match
     * @param range Acceptable level variance
     * @return true if bot level is within range
     */
    bool IsInLevelRange(uint32 targetLevel, uint32 range = 5) const
    {
        int32 diff = static_cast<int32>(level) - static_cast<int32>(targetLevel);
        return std::abs(diff) <= static_cast<int32>(range);
    }

    /**
     * @brief Check if bot meets gear score requirement
     * @param minGearScore Minimum required gear score
     * @return true if bot gear score is sufficient
     */
    bool MeetsGearScore(uint32 minGearScore) const
    {
        return gearScore >= minGearScore;
    }

    // ========================================================================
    // METHODS - Timing
    // ========================================================================

    /**
     * @brief Get time since last state change
     * @return Duration since state change
     */
    std::chrono::seconds TimeSinceStateChange() const
    {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - stateChangeTime);
    }

    /**
     * @brief Get time since last assignment
     * @return Duration since last instance assignment
     */
    std::chrono::seconds TimeSinceLastAssignment() const
    {
        if (lastAssignment.time_since_epoch().count() == 0)
            return std::chrono::seconds::max();

        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - lastAssignment);
    }

    /**
     * @brief Get remaining cooldown time
     * @param cooldownDuration Total cooldown duration
     * @return Remaining cooldown time (0 if expired)
     */
    std::chrono::seconds CooldownRemaining(std::chrono::seconds cooldownDuration) const
    {
        if (state != PoolSlotState::Cooldown)
            return std::chrono::seconds(0);

        auto elapsed = TimeSinceStateChange();
        if (elapsed >= cooldownDuration)
            return std::chrono::seconds(0);

        return cooldownDuration - elapsed;
    }

    /**
     * @brief Check if cooldown has expired
     * @param cooldownDuration Total cooldown duration
     * @return true if cooldown is complete
     */
    bool IsCooldownExpired(std::chrono::seconds cooldownDuration) const
    {
        return CooldownRemaining(cooldownDuration) == std::chrono::seconds(0);
    }

    // ========================================================================
    // METHODS - State Transitions
    // ========================================================================

    /**
     * @brief Attempt to transition to new state
     * @param newState Target state
     * @return true if transition was valid and successful
     *
     * This method validates the transition and updates timing.
     * Invalid transitions are rejected.
     */
    bool TransitionTo(PoolSlotState newState)
    {
        if (!CanTransitionTo(state, newState))
            return false;

        state = newState;
        stateChangeTime = std::chrono::steady_clock::now();
        return true;
    }

    /**
     * @brief Force state change (bypasses validation)
     * @param newState Target state
     *
     * Use sparingly - mainly for error recovery or initialization.
     */
    void ForceState(PoolSlotState newState)
    {
        state = newState;
        stateChangeTime = std::chrono::steady_clock::now();
    }

    // ========================================================================
    // METHODS - Assignment Operations
    // ========================================================================

    /**
     * @brief Assign bot to an instance
     * @param instanceId Instance to assign to
     * @param contentId Content ID (dungeon/raid/bg ID)
     * @param type Type of instance
     * @return true if assignment was successful
     */
    bool AssignToInstance(uint32 instanceId, uint32 contentId, InstanceType type)
    {
        if (!TransitionTo(PoolSlotState::Assigned))
            return false;

        currentInstanceId = instanceId;
        currentContentId = contentId;
        currentInstanceType = type;
        lastAssignment = std::chrono::steady_clock::now();
        ++assignmentCount;

        return true;
    }

    /**
     * @brief Release bot from instance (enter cooldown)
     * @param success Whether the instance was completed successfully
     */
    void ReleaseFromInstance(bool success = true)
    {
        if (state == PoolSlotState::Assigned)
        {
            // Track metrics
            auto duration = TimeSinceStateChange();
            totalInstanceTime += static_cast<uint32>(duration.count());

            if (success)
                ++successfulCompletions;
            else
                ++earlyExits;

            // Enter cooldown
            TransitionTo(PoolSlotState::Cooldown);
        }

        // Clear assignment tracking (keep in history)
        currentInstanceId = 0;
        currentContentId = 0;
        reservationId = 0;
    }

    /**
     * @brief Reserve bot for upcoming instance
     * @param resId Reservation ID
     * @return true if reservation was successful
     */
    bool Reserve(uint32 resId)
    {
        if (!TransitionTo(PoolSlotState::Reserved))
            return false;

        reservationId = resId;
        return true;
    }

    /**
     * @brief Cancel reservation and return to ready
     */
    void CancelReservation()
    {
        if (state == PoolSlotState::Reserved)
        {
            TransitionTo(PoolSlotState::Ready);
            reservationId = 0;
        }
    }

    /**
     * @brief Fulfill reservation (transition to assigned)
     * @param instanceId Instance to assign to
     * @param contentId Content ID
     * @param type Instance type
     * @return true if fulfillment was successful
     */
    bool FulfillReservation(uint32 instanceId, uint32 contentId, InstanceType type)
    {
        if (state != PoolSlotState::Reserved)
            return false;

        // Transition directly to Assigned (from Reserved)
        if (!TransitionTo(PoolSlotState::Assigned))
            return false;

        currentInstanceId = instanceId;
        currentContentId = contentId;
        currentInstanceType = type;
        lastAssignment = std::chrono::steady_clock::now();
        ++assignmentCount;

        return true;
    }

    // ========================================================================
    // METHODS - Lifecycle
    // ========================================================================

    /**
     * @brief Initialize slot for a new bot
     * @param guid Bot's GUID
     * @param account Account ID
     * @param name Character name
     * @param type Pool type
     * @param botRole Bot's role
     */
    void Initialize(ObjectGuid guid, uint32 account, std::string const& name,
                    PoolType type, BotRole botRole)
    {
        botGuid = guid;
        accountId = account;
        botName = name;
        poolType = type;
        role = botRole;

        // Reset state
        state = PoolSlotState::Creating;
        stateChangeTime = std::chrono::steady_clock::now();
        currentInstanceId = 0;
        currentContentId = 0;
        reservationId = 0;
    }

    /**
     * @brief Clear all slot data
     */
    void Clear()
    {
        botGuid = ObjectGuid::Empty;
        accountId = 0;
        botName.clear();
        poolType = PoolType::PvE;
        role = BotRole::DPS;
        playerClass = 0;
        specId = 0;
        faction = Faction::Alliance;
        bracket = ExpansionTier::TheWarWithin;
        level = 80;
        gearScore = 0;
        healthMax = 0;
        manaMax = 0;
        state = PoolSlotState::Empty;
        stateChangeTime = std::chrono::steady_clock::now();
        lastAssignment = {};
        currentInstanceId = 0;
        currentContentId = 0;
        currentInstanceType = InstanceType::Dungeon;
        reservationId = 0;
        assignmentCount = 0;
        humanPlayerGuid = ObjectGuid::Empty;
        totalInstanceTime = 0;
        successfulCompletions = 0;
        earlyExits = 0;
    }

    /**
     * @brief Complete warming and mark as ready
     * @param characterLevel Bot's level
     * @param characterGearScore Bot's gear score
     * @param characterClass WoW class ID
     * @param characterSpec Specialization ID
     * @param characterFaction Faction
     * @return true if transition to Ready was successful
     */
    bool CompleteWarming(uint32 characterLevel, uint32 characterGearScore,
                         uint8 characterClass, uint32 characterSpec,
                         Faction characterFaction)
    {
        if (state != PoolSlotState::Warming)
            return false;

        level = characterLevel;
        gearScore = characterGearScore;
        playerClass = characterClass;
        specId = characterSpec;
        faction = characterFaction;

        // Determine bracket from level
        if (level < 10)
            bracket = ExpansionTier::Starting;
        else if (level < 60)
            bracket = ExpansionTier::ChromieTime;
        else if (level < 70)
            bracket = ExpansionTier::Dragonflight;
        else
            bracket = ExpansionTier::TheWarWithin;

        return TransitionTo(PoolSlotState::Ready);
    }

    // ========================================================================
    // METHODS - Utility
    // ========================================================================

    /**
     * @brief Calculate assignment score for selection
     * @param targetLevel Desired level
     * @param requiredGearScore Minimum gear score
     * @return Score (higher is better match)
     *
     * Used to select the best bot when multiple are available.
     */
    float CalculateAssignmentScore(uint32 targetLevel, uint32 requiredGearScore) const
    {
        float score = 100.0f;

        // Level match (±0-10 points based on difference)
        int32 levelDiff = std::abs(static_cast<int32>(level) - static_cast<int32>(targetLevel));
        score -= levelDiff * 2.0f;

        // Gear score bonus (up to +20 points)
        if (gearScore > requiredGearScore)
        {
            uint32 excess = gearScore - requiredGearScore;
            score += std::min(excess / 10.0f, 20.0f);
        }

        // Time since last assignment bonus (spread out usage)
        auto timeSinceLast = TimeSinceLastAssignment();
        if (timeSinceLast > std::chrono::minutes(30))
            score += 10.0f;
        else if (timeSinceLast > std::chrono::minutes(10))
            score += 5.0f;

        // Success rate bonus
        if (assignmentCount > 0)
        {
            float successRate = static_cast<float>(successfulCompletions) /
                               static_cast<float>(assignmentCount);
            score += successRate * 10.0f;
        }

        return score;
    }

    /**
     * @brief Get string representation for logging
     * @return Formatted string
     */
    std::string ToString() const
    {
        return Trinity::StringFormat("[Slot: {} ({}) Role={} State={} Level={} GS={}]",
            botName,
            botGuid.ToString(),
            BotRoleToString(role),
            PoolSlotStateToString(state),
            level,
            gearScore);
    }
};

} // namespace Playerbot
