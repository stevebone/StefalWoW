/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Bot Readiness Checker - Comprehensive validation that a bot is fully loaded,
 * processed, and ready for game actions (LFG, BG, combat, etc.)
 *
 * This prevents race conditions where systems try to interact with bots
 * that haven't completed their initialization sequence.
 */

#ifndef BOT_READINESS_CHECKER_H
#define BOT_READINESS_CHECKER_H

#include "ObjectGuid.h"
#include <string>
#include <vector>

class Player;

namespace Playerbot
{

/**
 * @brief Flags indicating which readiness checks passed
 */
enum class BotReadinessFlag : uint32
{
    NONE                    = 0x00000000,

    // ObjectAccessor checks
    FOUND_CONNECTED         = 0x00000001,  // FindConnectedPlayer() succeeded
    FOUND_IN_WORLD          = 0x00000002,  // FindPlayer() succeeded (IsInWorld)

    // CharacterCache checks
    IN_CHARACTER_CACHE      = 0x00000004,  // Character registered in cache

    // Player state checks
    IS_IN_WORLD             = 0x00000008,  // player->IsInWorld()
    HAS_MAP                 = 0x00000010,  // player->GetMap() != null
    HAS_SESSION             = 0x00000020,  // player->GetSession() != null
    NOT_TELEPORTING_FAR     = 0x00000040,  // !IsBeingTeleportedFar()
    NOT_TELEPORTING_NEAR    = 0x00000080,  // !IsBeingTeleportedNear()
    NOT_LOGGING_OUT         = 0x00000100,  // Not in logout process
    IS_ALIVE_OR_GHOST       = 0x00000200,  // Alive or in ghost form (can act)

    // Bot-specific checks
    IS_BOT                  = 0x00000400,  // Confirmed as bot via PlayerBotHooks
    BOT_SESSION_ACTIVE      = 0x00000800,  // BotSession is active and not destroyed
    BOT_AI_INITIALIZED      = 0x00001000,  // BotAI is attached and initialized

    // Group/Queue checks (optional, for LFG/BG)
    NOT_IN_QUEUE            = 0x00002000,  // Not already in LFG/BG queue
    NOT_IN_GROUP            = 0x00004000,  // Not already in a group
    NOT_IN_COMBAT           = 0x00008000,  // Not currently in combat

    // Composite flags for common use cases
    BASIC_READY             = FOUND_CONNECTED | IS_IN_WORLD | HAS_MAP | HAS_SESSION,
    FULL_READY              = BASIC_READY | IN_CHARACTER_CACHE | NOT_TELEPORTING_FAR |
                              NOT_TELEPORTING_NEAR | IS_BOT | BOT_SESSION_ACTIVE,
    LFG_READY               = FULL_READY | NOT_IN_QUEUE | BOT_AI_INITIALIZED,
    COMBAT_READY            = FULL_READY | IS_ALIVE_OR_GHOST | NOT_LOGGING_OUT
};

// Enable bitwise operations on BotReadinessFlag
inline BotReadinessFlag operator|(BotReadinessFlag a, BotReadinessFlag b)
{
    return static_cast<BotReadinessFlag>(static_cast<uint32>(a) | static_cast<uint32>(b));
}

inline BotReadinessFlag operator&(BotReadinessFlag a, BotReadinessFlag b)
{
    return static_cast<BotReadinessFlag>(static_cast<uint32>(a) & static_cast<uint32>(b));
}

inline BotReadinessFlag& operator|=(BotReadinessFlag& a, BotReadinessFlag b)
{
    return a = a | b;
}

inline bool HasFlag(BotReadinessFlag flags, BotReadinessFlag flag)
{
    return (static_cast<uint32>(flags) & static_cast<uint32>(flag)) == static_cast<uint32>(flag);
}

/**
 * @brief Result of a bot readiness check
 */
struct BotReadinessResult
{
    ObjectGuid botGuid;
    Player* player = nullptr;           // Non-null if bot was found
    BotReadinessFlag passedChecks = BotReadinessFlag::NONE;
    BotReadinessFlag failedChecks = BotReadinessFlag::NONE;
    std::vector<std::string> failureReasons;

    // Quick status checks
    bool IsFullyReady() const
    {
        return HasFlag(passedChecks, BotReadinessFlag::FULL_READY);
    }

    bool IsLFGReady() const
    {
        return HasFlag(passedChecks, BotReadinessFlag::LFG_READY);
    }

    bool IsCombatReady() const
    {
        return HasFlag(passedChecks, BotReadinessFlag::COMBAT_READY);
    }

    bool IsBasicReady() const
    {
        return HasFlag(passedChecks, BotReadinessFlag::BASIC_READY);
    }

    // Check if specific flag passed
    bool Passed(BotReadinessFlag flag) const
    {
        return HasFlag(passedChecks, flag);
    }

    // Check if specific flag failed
    bool Failed(BotReadinessFlag flag) const
    {
        return HasFlag(failedChecks, flag);
    }

    // Get a summary string for logging
    std::string GetSummary() const;

    // Get detailed failure report
    std::string GetFailureReport() const;
};

/**
 * @brief Comprehensive bot readiness validation
 *
 * Use this class to verify a bot is fully loaded and ready before
 * performing actions that require a stable bot state.
 *
 * Example usage:
 * @code
 * BotReadinessResult result = BotReadinessChecker::Check(botGuid);
 * if (result.IsLFGReady())
 * {
 *     // Safe to queue bot for LFG
 *     QueueBot(result.player, ...);
 * }
 * else
 * {
 *     TC_LOG_DEBUG("playerbot", "Bot not ready: {}", result.GetFailureReport());
 * }
 * @endcode
 */
class TC_GAME_API BotReadinessChecker
{
public:
    /**
     * @brief Perform all readiness checks for a bot by GUID
     * @param botGuid The bot's ObjectGuid
     * @param requiredFlags Optional: specific flags to check (default: all)
     * @return BotReadinessResult with detailed pass/fail information
     */
    static BotReadinessResult Check(ObjectGuid botGuid,
                                     BotReadinessFlag requiredFlags = BotReadinessFlag::FULL_READY);

    /**
     * @brief Perform all readiness checks for a bot by Player pointer
     * @param player The bot's Player object (must not be null)
     * @param requiredFlags Optional: specific flags to check (default: all)
     * @return BotReadinessResult with detailed pass/fail information
     */
    static BotReadinessResult Check(Player* player,
                                     BotReadinessFlag requiredFlags = BotReadinessFlag::FULL_READY);

    /**
     * @brief Quick check if bot is ready (no detailed result)
     * @param botGuid The bot's ObjectGuid
     * @param requiredFlags Flags that must all pass
     * @return true if all required flags pass
     */
    static bool IsReady(ObjectGuid botGuid,
                        BotReadinessFlag requiredFlags = BotReadinessFlag::FULL_READY);

    /**
     * @brief Quick check if bot is ready for LFG
     * @param botGuid The bot's ObjectGuid
     * @return true if bot can be queued for LFG
     */
    static bool IsLFGReady(ObjectGuid botGuid);

    /**
     * @brief Quick check if bot is ready for combat
     * @param botGuid The bot's ObjectGuid
     * @return true if bot can perform combat actions
     */
    static bool IsCombatReady(ObjectGuid botGuid);

    /**
     * @brief Get human-readable name for a readiness flag
     */
    static char const* GetFlagName(BotReadinessFlag flag);

private:
    // Individual check implementations
    static void CheckObjectAccessor(ObjectGuid botGuid, BotReadinessResult& result);
    static void CheckCharacterCache(ObjectGuid botGuid, BotReadinessResult& result);
    static void CheckPlayerState(Player* player, BotReadinessResult& result);
    static void CheckBotSpecific(Player* player, BotReadinessResult& result);
    static void CheckQueueState(Player* player, BotReadinessResult& result);

    // Helper to add failure
    static void AddFailure(BotReadinessResult& result, BotReadinessFlag flag, std::string reason);
    static void AddSuccess(BotReadinessResult& result, BotReadinessFlag flag);
};

} // namespace Playerbot

#endif // BOT_READINESS_CHECKER_H
