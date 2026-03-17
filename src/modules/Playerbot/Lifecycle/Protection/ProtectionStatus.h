/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file ProtectionStatus.h
 * @brief Complete protection status for a bot
 *
 * This structure holds all protection-related information for a single bot:
 * - Protection reason flags
 * - Social connection details (guild, friends)
 * - Interaction history
 * - Calculated protection score
 *
 * The status is cached in memory and periodically synchronized with the database.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "ProtectionReason.h"
#include <chrono>
#include <set>
#include <string>

namespace Playerbot
{

/**
 * @brief Complete protection status for a single bot
 *
 * This structure contains all information needed to determine:
 * 1. Whether a bot is protected from retirement
 * 2. Why it is protected (which reasons)
 * 3. How protected it is (protection score)
 * 4. Details for debugging/reporting
 */
struct ProtectionStatus
{
    // ========================================================================
    // IDENTIFICATION
    // ========================================================================

    /**
     * @brief Bot character GUID
     */
    ObjectGuid botGuid;

    // ========================================================================
    // PROTECTION FLAGS
    // ========================================================================

    /**
     * @brief Active protection reason flags
     *
     * Bitmask of ProtectionReason values indicating all active protections.
     * Use HasProtectionReason() to check specific flags.
     */
    ProtectionReason protectionFlags = ProtectionReason::None;

    // ========================================================================
    // SOCIAL CONNECTIONS
    // ========================================================================

    /**
     * @brief Guild GUID if bot is in a guild
     *
     * Empty if bot is not in any guild.
     * When set, InGuild flag should also be set.
     */
    ObjectGuid guildGuid;

    /**
     * @brief Number of players who have this bot on their friend list
     *
     * Count of unique player->bot friend references.
     * When > 0, OnFriendList flag should be set.
     */
    uint32 friendCount = 0;

    /**
     * @brief Set of player GUIDs who have this bot as friend
     *
     * Used for efficient lookup when players modify their friend list.
     * This is populated from playerbot_friend_references table.
     */
    std::set<ObjectGuid> friendingPlayers;

    /**
     * @brief Current group GUID if bot is in a group with a player
     *
     * Only set when group contains at least one real player.
     * Empty if bot is solo or in all-bot group.
     */
    ObjectGuid playerGroupGuid;

    // ========================================================================
    // INTERACTION HISTORY
    // ========================================================================

    /**
     * @brief Total number of player interactions
     *
     * Cumulative count of all player interactions (trade, whisper, invite, etc.)
     * Higher values indicate more player engagement.
     */
    uint32 interactionCount = 0;

    /**
     * @brief Timestamp of last player interaction
     *
     * Used to determine if RecentInteract flag should be set.
     * Cleared after configurable window (default 24h).
     */
    std::chrono::system_clock::time_point lastInteraction;

    /**
     * @brief Timestamp of last time bot was grouped with a player
     *
     * Separate from lastInteraction because grouping is a stronger signal.
     */
    std::chrono::system_clock::time_point lastGroupTime;

    // ========================================================================
    // GAME STATE
    // ========================================================================

    /**
     * @brief Whether bot has pending mail
     *
     * When true, HasActiveMail flag should be set.
     */
    bool hasPendingMail = false;

    /**
     * @brief Whether bot has active auction listings
     *
     * When true, HasActiveAuction flag should be set.
     */
    bool hasActiveAuctions = false;

    // ========================================================================
    // CALCULATED VALUES
    // ========================================================================

    /**
     * @brief Calculated protection score
     *
     * Aggregate score based on all protection factors.
     * Higher score = more protected = less likely to be retired.
     *
     * Score calculation:
     * - Base: Sum of weights for each active protection reason
     * - Bonus: +10 per friend, +1 per interaction, +50 per hour grouped
     */
    float protectionScore = 0.0f;

    // ========================================================================
    // METADATA
    // ========================================================================

    /**
     * @brief When protection tracking started for this bot
     */
    std::chrono::system_clock::time_point createdAt;

    /**
     * @brief When this status was last updated
     */
    std::chrono::system_clock::time_point updatedAt;

    /**
     * @brief Whether this status needs to be saved to database
     */
    bool isDirty = false;

    // ========================================================================
    // CONSTRUCTORS
    // ========================================================================

    ProtectionStatus() = default;

    explicit ProtectionStatus(ObjectGuid guid)
        : botGuid(guid)
        , createdAt(std::chrono::system_clock::now())
        , updatedAt(std::chrono::system_clock::now())
    {
    }

    // ========================================================================
    // QUERY METHODS
    // ========================================================================

    /**
     * @brief Check if bot is protected from retirement
     * @return true if any protection reason is active
     */
    bool IsProtected() const noexcept
    {
        return HasAnyProtection(protectionFlags);
    }

    /**
     * @brief Check if bot has social connections (strongest protection)
     * @return true if bot is in guild, on friend list, or in player group
     */
    bool HasSocialConnections() const noexcept
    {
        return HasSocialProtection(protectionFlags);
    }

    /**
     * @brief Check if bot has game state locks
     * @return true if bot has pending mail or active auctions
     */
    bool HasGameLocks() const noexcept
    {
        return HasGameStateLock(protectionFlags);
    }

    /**
     * @brief Check if bot is admin-protected
     * @return true if manually protected by admin
     */
    bool IsManuallyProtected() const noexcept
    {
        return IsAdminProtected(protectionFlags);
    }

    /**
     * @brief Check if specific protection reason is active
     * @param reason The reason to check
     * @return true if the reason is active
     */
    bool HasReason(ProtectionReason reason) const noexcept
    {
        return HasProtectionReason(protectionFlags, reason);
    }

    /**
     * @brief Get number of protection reasons active
     * @return Count of individual protection reasons
     */
    uint32 GetReasonCount() const noexcept
    {
        return CountProtectionReasons(protectionFlags);
    }

    /**
     * @brief Check if recent interaction window is still active
     * @param interactionWindowHours Window duration in hours (default 24)
     * @return true if bot was interacted with within window
     */
    bool HasRecentInteraction(uint32 interactionWindowHours = 24) const
    {
        if (lastInteraction == std::chrono::system_clock::time_point{})
            return false;

        auto windowDuration = std::chrono::hours(interactionWindowHours);
        auto cutoff = std::chrono::system_clock::now() - windowDuration;
        return lastInteraction > cutoff;
    }

    // ========================================================================
    // MODIFICATION METHODS
    // ========================================================================

    /**
     * @brief Add a protection reason flag
     * @param reason The reason to add
     */
    void AddReason(ProtectionReason reason) noexcept
    {
        protectionFlags |= reason;
        isDirty = true;
        updatedAt = std::chrono::system_clock::now();
    }

    /**
     * @brief Remove a protection reason flag
     * @param reason The reason to remove
     */
    void RemoveReason(ProtectionReason reason) noexcept
    {
        protectionFlags &= ~reason;
        isDirty = true;
        updatedAt = std::chrono::system_clock::now();
    }

    /**
     * @brief Set or clear a protection reason flag
     * @param reason The reason to modify
     * @param active Whether to set (true) or clear (false)
     */
    void SetReason(ProtectionReason reason, bool active) noexcept
    {
        if (active)
            AddReason(reason);
        else
            RemoveReason(reason);
    }

    /**
     * @brief Record a player interaction
     */
    void RecordInteraction() noexcept
    {
        ++interactionCount;
        lastInteraction = std::chrono::system_clock::now();
        AddReason(ProtectionReason::RecentInteract);
        isDirty = true;
    }

    /**
     * @brief Record grouping with a player
     * @param groupGuid The group GUID
     */
    void RecordGroupJoin(ObjectGuid groupGuid) noexcept
    {
        playerGroupGuid = groupGuid;
        lastGroupTime = std::chrono::system_clock::now();
        AddReason(ProtectionReason::InPlayerGroup);
        RecordInteraction(); // Grouping counts as interaction
    }

    /**
     * @brief Record leaving a player group
     */
    void RecordGroupLeave() noexcept
    {
        playerGroupGuid = ObjectGuid::Empty;
        RemoveReason(ProtectionReason::InPlayerGroup);
    }

    /**
     * @brief Add a friend reference
     * @param playerGuid The player who added bot as friend
     */
    void AddFriendReference(ObjectGuid playerGuid)
    {
        if (friendingPlayers.insert(playerGuid).second)
        {
            ++friendCount;
            AddReason(ProtectionReason::OnFriendList);
        }
    }

    /**
     * @brief Remove a friend reference
     * @param playerGuid The player who removed bot from friends
     */
    void RemoveFriendReference(ObjectGuid playerGuid)
    {
        if (friendingPlayers.erase(playerGuid) > 0)
        {
            if (friendCount > 0)
                --friendCount;
            if (friendCount == 0)
                RemoveReason(ProtectionReason::OnFriendList);
        }
    }

    /**
     * @brief Set guild membership
     * @param guild The guild GUID (empty to clear)
     */
    void SetGuild(ObjectGuid guild) noexcept
    {
        guildGuid = guild;
        SetReason(ProtectionReason::InGuild, !guild.IsEmpty());
    }

    /**
     * @brief Update mail status
     * @param hasMail Whether bot has pending mail
     */
    void SetMailStatus(bool hasMail) noexcept
    {
        hasPendingMail = hasMail;
        SetReason(ProtectionReason::HasActiveMail, hasMail);
    }

    /**
     * @brief Update auction status
     * @param hasAuctions Whether bot has active auctions
     */
    void SetAuctionStatus(bool hasAuctions) noexcept
    {
        hasActiveAuctions = hasAuctions;
        SetReason(ProtectionReason::HasActiveAuction, hasAuctions);
    }

    /**
     * @brief Set manual (admin) protection
     * @param protect Whether to enable manual protection
     */
    void SetManualProtection(bool protect) noexcept
    {
        SetReason(ProtectionReason::ManualProtect, protect);
    }

    /**
     * @brief Recalculate protection score based on current state
     *
     * Score calculation:
     * - Base score from protection reason weights
     * - +10 per friend on friend list
     * - +1 per recorded interaction
     * - +5 per hour since last group (capped at 50)
     */
    void RecalculateScore() noexcept
    {
        // Base score from protection flags
        protectionScore = CalculateProtectionScore(protectionFlags);

        // Bonus for friend count
        protectionScore += friendCount * 10.0f;

        // Bonus for interaction history
        protectionScore += std::min(interactionCount, 100u) * 1.0f;

        // Bonus for recent grouping
        if (lastGroupTime != std::chrono::system_clock::time_point{})
        {
            auto hoursSinceGroup = std::chrono::duration_cast<std::chrono::hours>(
                std::chrono::system_clock::now() - lastGroupTime).count();
            // Decay over 10 hours, max +50
            float groupBonus = std::max(0.0f, 50.0f - (hoursSinceGroup * 5.0f));
            protectionScore += groupBonus;
        }

        isDirty = true;
        updatedAt = std::chrono::system_clock::now();
    }

    /**
     * @brief Check and update time-based protection flags
     * @param interactionWindowHours Window for RecentInteract (default 24h)
     */
    void UpdateTimeBasedFlags(uint32 interactionWindowHours = 24) noexcept
    {
        // Check if RecentInteract should expire
        if (HasReason(ProtectionReason::RecentInteract))
        {
            if (!HasRecentInteraction(interactionWindowHours))
            {
                RemoveReason(ProtectionReason::RecentInteract);
            }
        }
    }

    // ========================================================================
    // FORMATTING
    // ========================================================================

    /**
     * @brief Get protection status as formatted string for logging
     * @return Formatted status string
     */
    std::string ToString() const
    {
        std::string result = "ProtectionStatus{";
        result += "guid=" + botGuid.ToString();
        result += ", protected=" + std::string(IsProtected() ? "true" : "false");
        result += ", score=" + std::to_string(protectionScore);
        result += ", flags=" + FormatProtectionReasons(protectionFlags);
        result += ", friends=" + std::to_string(friendCount);
        result += ", interactions=" + std::to_string(interactionCount);
        if (!guildGuid.IsEmpty())
            result += ", guild=" + guildGuid.ToString();
        result += "}";
        return result;
    }
};

/**
 * @brief Protection status comparator for priority queue (lowest score first)
 *
 * Used when selecting retirement candidates - bots with lowest protection
 * score should be considered first for retirement.
 */
struct ProtectionStatusComparator
{
    bool operator()(ProtectionStatus const& a, ProtectionStatus const& b) const noexcept
    {
        // Lower score = lower priority = more likely to retire
        return a.protectionScore < b.protectionScore;
    }
};

/**
 * @brief Protection status comparator for retirement (highest priority first)
 *
 * Used when selecting bots for retirement queue - least protected bots first.
 */
struct RetirementPriorityComparator
{
    bool operator()(ProtectionStatus const& a, ProtectionStatus const& b) const noexcept
    {
        // First: Protected bots should never be retired
        if (a.IsProtected() != b.IsProtected())
            return b.IsProtected(); // Unprotected (false) comes before protected (true)

        // Then: Lower protection score = higher retirement priority
        return a.protectionScore < b.protectionScore;
    }
};

} // namespace Playerbot
