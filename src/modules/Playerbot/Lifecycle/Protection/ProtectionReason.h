/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file ProtectionReason.h
 * @brief Bot protection reason flags and utilities
 *
 * Protection reasons determine WHY a bot is protected from retirement.
 * Each reason corresponds to a social connection or game state that makes
 * deleting the bot unacceptable (would negatively impact real players).
 *
 * Protection is ADDITIVE - a bot with multiple reasons is MORE protected.
 * Protection score calculation uses these flags plus additional factors.
 */

#pragma once

#include "Define.h"
#include <string>
#include <string_view>
#include <vector>

namespace Playerbot
{

/**
 * @brief Protection reason flags (bitmask)
 *
 * These flags indicate WHY a bot is protected from retirement.
 * Multiple flags can be combined (bitwise OR).
 *
 * Flag values are powers of 2 for bitmask operations.
 */
enum class ProtectionReason : uint8
{
    // No protection - bot can be retired freely
    None            = 0,

    // ========================================================================
    // SOCIAL CONNECTIONS (Permanent protection while active)
    // ========================================================================

    /**
     * @brief Bot is member of ANY guild
     *
     * Guild membership is the strongest protection because:
     * - Players actively invited this bot to their guild
     * - Bot may hold guild bank items or gold
     * - Bot removal would be noticed by all guild members
     * - Guilds track member history, so deletion is visible
     *
     * Cleared when: Bot leaves guild or is kicked
     */
    InGuild         = 1 << 0,  // 0x01

    /**
     * @brief Bot is on ANY player's friend list
     *
     * Friend list protection because:
     * - Player specifically chose to track this bot
     * - Player may send mail/whispers to this bot
     * - Sudden disappearance would confuse/frustrate player
     *
     * Cleared when: All players remove bot from friend list
     */
    OnFriendList    = 1 << 1,  // 0x02

    /**
     * @brief Bot is currently grouped with a real player
     *
     * Active group protection because:
     * - Bot is actively helping player with content
     * - Mid-dungeon deletion would strand player
     * - Player may rely on bot for tanking/healing
     *
     * Cleared when: Group disbands or player leaves
     */
    InPlayerGroup   = 1 << 2,  // 0x04

    // ========================================================================
    // INTERACTION HISTORY (Time-limited protection)
    // ========================================================================

    /**
     * @brief Player interacted with bot within interaction window (default 24h)
     *
     * Interactions include: trade, whisper, invite, assist, duel request
     *
     * Temporary protection because:
     * - Recent interaction suggests ongoing relationship
     * - Player might return to interact again soon
     *
     * Cleared when: No interaction for configurable duration
     */
    RecentInteract  = 1 << 3,  // 0x08

    // ========================================================================
    // GAME STATE LOCKS (Must resolve before retirement)
    // ========================================================================

    /**
     * @brief Bot has pending mail items
     *
     * Mail protection because:
     * - Mail may contain items sent by real players
     * - COD mail may be awaiting payment
     * - Deletion would destroy player's items
     *
     * Cleared when: All mail is retrieved or expires
     */
    HasActiveMail   = 1 << 4,  // 0x10

    /**
     * @brief Bot has active auction house listings
     *
     * Auction protection because:
     * - Items are locked in auction system
     * - Gold may be pending from sold items
     * - Player economy would be affected
     *
     * Cleared when: All auctions complete or are cancelled
     */
    HasActiveAuction= 1 << 5,  // 0x20

    // ========================================================================
    // ADMINISTRATIVE (Override all other logic)
    // ========================================================================

    /**
     * @brief Admin-protected bot (GM command or config)
     *
     * Manual protection for:
     * - Test bots that should never be deleted
     * - Showcase/demo bots
     * - Bots with special roles
     *
     * Cleared only by: Admin command
     */
    ManualProtect   = 1 << 6,  // 0x40

    // Reserved for future use
    // Reserved7    = 1 << 7,  // 0x80
};

// ============================================================================
// BITWISE OPERATORS
// ============================================================================

/**
 * @brief Bitwise OR operator for combining protection reasons
 */
inline constexpr ProtectionReason operator|(ProtectionReason a, ProtectionReason b) noexcept
{
    return static_cast<ProtectionReason>(
        static_cast<uint8>(a) | static_cast<uint8>(b)
    );
}

/**
 * @brief Bitwise AND operator for checking protection reasons
 */
inline constexpr ProtectionReason operator&(ProtectionReason a, ProtectionReason b) noexcept
{
    return static_cast<ProtectionReason>(
        static_cast<uint8>(a) & static_cast<uint8>(b)
    );
}

/**
 * @brief Bitwise XOR operator for toggling protection reasons
 */
inline constexpr ProtectionReason operator^(ProtectionReason a, ProtectionReason b) noexcept
{
    return static_cast<ProtectionReason>(
        static_cast<uint8>(a) ^ static_cast<uint8>(b)
    );
}

/**
 * @brief Bitwise NOT operator for inverting protection reasons
 */
inline constexpr ProtectionReason operator~(ProtectionReason a) noexcept
{
    return static_cast<ProtectionReason>(~static_cast<uint8>(a));
}

/**
 * @brief Compound OR assignment operator
 */
inline ProtectionReason& operator|=(ProtectionReason& a, ProtectionReason b) noexcept
{
    a = a | b;
    return a;
}

/**
 * @brief Compound AND assignment operator
 */
inline ProtectionReason& operator&=(ProtectionReason& a, ProtectionReason b) noexcept
{
    a = a & b;
    return a;
}

/**
 * @brief Compound XOR assignment operator
 */
inline ProtectionReason& operator^=(ProtectionReason& a, ProtectionReason b) noexcept
{
    a = a ^ b;
    return a;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * @brief Check if a specific protection reason flag is set
 * @param flags The protection flags to check
 * @param reason The specific reason to check for
 * @return true if the reason flag is set
 */
inline constexpr bool HasProtectionReason(ProtectionReason flags, ProtectionReason reason) noexcept
{
    return (flags & reason) != ProtectionReason::None;
}

/**
 * @brief Check if any protection reason is set
 * @param flags The protection flags to check
 * @return true if any protection reason is set
 */
inline constexpr bool HasAnyProtection(ProtectionReason flags) noexcept
{
    return flags != ProtectionReason::None;
}

/**
 * @brief Check if bot has social protection (guild, friends, or group)
 * @param flags The protection flags to check
 * @return true if bot has any social connection protection
 */
inline constexpr bool HasSocialProtection(ProtectionReason flags) noexcept
{
    constexpr auto socialMask = ProtectionReason::InGuild |
                                 ProtectionReason::OnFriendList |
                                 ProtectionReason::InPlayerGroup;
    return (flags & socialMask) != ProtectionReason::None;
}

/**
 * @brief Check if bot has game state locks (mail, auctions)
 * @param flags The protection flags to check
 * @return true if bot has active game state locks
 */
inline constexpr bool HasGameStateLock(ProtectionReason flags) noexcept
{
    constexpr auto lockMask = ProtectionReason::HasActiveMail |
                               ProtectionReason::HasActiveAuction;
    return (flags & lockMask) != ProtectionReason::None;
}

/**
 * @brief Check if bot is admin-protected
 * @param flags The protection flags to check
 * @return true if bot is manually protected by admin
 */
inline constexpr bool IsAdminProtected(ProtectionReason flags) noexcept
{
    return HasProtectionReason(flags, ProtectionReason::ManualProtect);
}

/**
 * @brief Count the number of protection reasons set
 * @param flags The protection flags to count
 * @return Number of individual reasons set
 */
inline constexpr uint32 CountProtectionReasons(ProtectionReason flags) noexcept
{
    uint8 value = static_cast<uint8>(flags);
    uint32 count = 0;
    while (value)
    {
        count += value & 1;
        value >>= 1;
    }
    return count;
}

/**
 * @brief Convert protection reason to display string
 * @param reason Single protection reason flag
 * @return Human-readable string
 */
inline constexpr std::string_view ProtectionReasonToString(ProtectionReason reason) noexcept
{
    switch (reason)
    {
        case ProtectionReason::None:            return "None";
        case ProtectionReason::InGuild:         return "InGuild";
        case ProtectionReason::OnFriendList:    return "OnFriendList";
        case ProtectionReason::InPlayerGroup:   return "InPlayerGroup";
        case ProtectionReason::RecentInteract:  return "RecentInteraction";
        case ProtectionReason::HasActiveMail:   return "HasActiveMail";
        case ProtectionReason::HasActiveAuction:return "HasActiveAuction";
        case ProtectionReason::ManualProtect:   return "ManualProtection";
        default:                                return "Unknown";
    }
}

/**
 * @brief Get all active protection reasons as string list
 * @param flags The protection flags to enumerate
 * @return Vector of reason strings
 */
inline std::vector<std::string_view> GetProtectionReasonStrings(ProtectionReason flags)
{
    std::vector<std::string_view> reasons;

    if (HasProtectionReason(flags, ProtectionReason::InGuild))
        reasons.push_back(ProtectionReasonToString(ProtectionReason::InGuild));
    if (HasProtectionReason(flags, ProtectionReason::OnFriendList))
        reasons.push_back(ProtectionReasonToString(ProtectionReason::OnFriendList));
    if (HasProtectionReason(flags, ProtectionReason::InPlayerGroup))
        reasons.push_back(ProtectionReasonToString(ProtectionReason::InPlayerGroup));
    if (HasProtectionReason(flags, ProtectionReason::RecentInteract))
        reasons.push_back(ProtectionReasonToString(ProtectionReason::RecentInteract));
    if (HasProtectionReason(flags, ProtectionReason::HasActiveMail))
        reasons.push_back(ProtectionReasonToString(ProtectionReason::HasActiveMail));
    if (HasProtectionReason(flags, ProtectionReason::HasActiveAuction))
        reasons.push_back(ProtectionReasonToString(ProtectionReason::HasActiveAuction));
    if (HasProtectionReason(flags, ProtectionReason::ManualProtect))
        reasons.push_back(ProtectionReasonToString(ProtectionReason::ManualProtect));

    return reasons;
}

/**
 * @brief Convert protection flags to formatted string for logging
 * @param flags The protection flags to format
 * @return Formatted string like "InGuild|OnFriendList"
 */
inline std::string FormatProtectionReasons(ProtectionReason flags)
{
    if (flags == ProtectionReason::None)
        return "None";

    auto reasons = GetProtectionReasonStrings(flags);
    std::string result;
    for (size_t i = 0; i < reasons.size(); ++i)
    {
        if (i > 0)
            result += "|";
        result += reasons[i];
    }
    return result;
}

// ============================================================================
// PROTECTION WEIGHTS (for score calculation)
// ============================================================================

/**
 * @brief Get the protection weight for a specific reason
 *
 * Weights are used to calculate overall protection score.
 * Higher weights = stronger protection = less likely to retire.
 *
 * @param reason The protection reason
 * @return Weight value (0.0 - 100.0)
 */
inline constexpr float GetProtectionWeight(ProtectionReason reason) noexcept
{
    switch (reason)
    {
        case ProtectionReason::InGuild:         return 100.0f;  // Strongest
        case ProtectionReason::OnFriendList:    return 80.0f;
        case ProtectionReason::InPlayerGroup:   return 90.0f;
        case ProtectionReason::RecentInteract:  return 40.0f;
        case ProtectionReason::HasActiveMail:   return 50.0f;
        case ProtectionReason::HasActiveAuction:return 30.0f;
        case ProtectionReason::ManualProtect:   return 1000.0f; // Override
        default:                                return 0.0f;
    }
}

/**
 * @brief Calculate total protection score from flags
 *
 * The score is the sum of weights for all active protection reasons.
 * This provides a single value for retirement priority decisions.
 *
 * @param flags The protection flags
 * @return Total protection score
 */
inline constexpr float CalculateProtectionScore(ProtectionReason flags) noexcept
{
    float score = 0.0f;

    if (HasProtectionReason(flags, ProtectionReason::InGuild))
        score += GetProtectionWeight(ProtectionReason::InGuild);
    if (HasProtectionReason(flags, ProtectionReason::OnFriendList))
        score += GetProtectionWeight(ProtectionReason::OnFriendList);
    if (HasProtectionReason(flags, ProtectionReason::InPlayerGroup))
        score += GetProtectionWeight(ProtectionReason::InPlayerGroup);
    if (HasProtectionReason(flags, ProtectionReason::RecentInteract))
        score += GetProtectionWeight(ProtectionReason::RecentInteract);
    if (HasProtectionReason(flags, ProtectionReason::HasActiveMail))
        score += GetProtectionWeight(ProtectionReason::HasActiveMail);
    if (HasProtectionReason(flags, ProtectionReason::HasActiveAuction))
        score += GetProtectionWeight(ProtectionReason::HasActiveAuction);
    if (HasProtectionReason(flags, ProtectionReason::ManualProtect))
        score += GetProtectionWeight(ProtectionReason::ManualProtect);

    return score;
}

} // namespace Playerbot
