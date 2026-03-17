/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * SUMMON RESPONSE MANAGER
 *
 * Handles bot responses to warlock summons, meeting stone summons, and
 * other SPELL_EFFECT_SUMMON_PLAYER requests. Bots should behave like
 * real players when receiving summon requests:
 *
 *   - Auto-accept summons from group/raid members after a brief delay
 *   - Decline summons during combat or when crowd-controlled
 *   - Cancel current activity (fishing, crafting) before accepting
 *   - Respect instance/zone restrictions
 *   - Track summon history for anti-abuse and statistics
 *
 * The manager hooks into the SMSG_SUMMON_REQUEST packet flow. When
 * Player::SendSummonRequestFrom() is called on a bot, the manager
 * evaluates whether to accept and dispatches the SummonIfPossible()
 * call after a realistic human-like delay.
 *
 * Architecture:
 *   - Per-bot instance attached to bot's AI context
 *   - Checks Player::HasSummonPending() for active requests
 *   - Calls Player::SummonIfPossible(true/false) to accept/decline
 *   - Configurable acceptance delay (1-5 seconds, randomized)
 *   - Integrates with combat state, movement, and activity systems
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <string>
#include <vector>
#include <chrono>

class Player;
class Unit;

namespace Playerbot
{

// ============================================================================
// SUMMON DECLINE REASON
// ============================================================================

enum class SummonDeclineReason : uint8
{
    NONE                = 0,    // No reason to decline (will accept)
    IN_COMBAT           = 1,    // Currently in combat
    CROWD_CONTROLLED    = 2,    // Stunned/feared/charmed
    DEAD                = 3,    // Bot is dead
    NOT_IN_GROUP        = 4,    // Summoner not in same group/raid
    SUMMONER_HOSTILE    = 5,    // Summoner is hostile
    IN_BATTLEGROUND     = 6,    // Currently in a battleground
    IN_ARENA            = 7,    // Currently in arena
    ON_TAXI             = 8,    // Currently on flight path (auto-handled by core)
    INSTANCE_LOCKED     = 9,    // Instance lock conflict
    COOLDOWN            = 10,   // Accepted too recently (anti-spam)
    EXPIRED             = 11,   // Summon request expired before we could respond
    INVALID_SUMMONER    = 12    // Summoner no longer valid/online
};

// ============================================================================
// SUMMON HISTORY ENTRY
// ============================================================================

struct SummonHistoryEntry
{
    ObjectGuid summonerGuid;
    uint32 timestamp{0};          // GameTime when summon was received
    uint32 zoneId{0};             // Zone the summon would go to
    bool accepted{false};
    SummonDeclineReason declineReason{SummonDeclineReason::NONE};
};

// ============================================================================
// SUMMON STATISTICS
// ============================================================================

struct SummonStats
{
    uint32 summonsReceived{0};
    uint32 summonsAccepted{0};
    uint32 summonsDeclined{0};
    uint32 summonsExpired{0};
    uint32 lastSummonTime{0};     // GameTime of last summon response
};

// ============================================================================
// SUMMON RESPONSE MANAGER
// ============================================================================

class SummonResponseManager
{
public:
    explicit SummonResponseManager(Player* bot);
    ~SummonResponseManager() = default;

    // ========================================================================
    // MAIN UPDATE LOOP
    // ========================================================================

    /// Called every bot update tick. Checks for pending summons and responds
    /// after the configured delay. Returns true if a summon was accepted
    /// (caller may want to cancel current activities).
    bool Update(uint32 diff);

    // ========================================================================
    // SUMMON EVALUATION
    // ========================================================================

    /// Check if there is a pending summon that we should respond to
    bool HasPendingSummon() const;

    /// Evaluate whether the bot should accept the current summon
    /// Returns NONE if should accept, otherwise returns the decline reason
    SummonDeclineReason EvaluateSummon() const;

    /// Force accept the current summon (for GM commands)
    void ForceAcceptSummon();

    /// Force decline the current summon
    void ForceDeclineSummon();

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /// Set minimum response delay in milliseconds (default: 1500)
    void SetMinResponseDelay(uint32 delayMs) { _minResponseDelayMs = delayMs; }

    /// Set maximum response delay in milliseconds (default: 4000)
    void SetMaxResponseDelay(uint32 delayMs) { _maxResponseDelayMs = delayMs; }

    /// Set whether to auto-accept summons from group members (default: true)
    void SetAutoAcceptGroupSummons(bool accept) { _autoAcceptGroupSummons = accept; }

    /// Set whether to accept summons during combat if safe to do so (default: false)
    void SetAcceptDuringCombat(bool accept) { _acceptDuringCombat = accept; }

    /// Set minimum time between accepted summons in seconds (default: 30)
    void SetSummonCooldown(uint32 cooldownSec) { _summonCooldownSec = cooldownSec; }

    // ========================================================================
    // QUERIES
    // ========================================================================

    /// Get the current decline reason (NONE if no reason to decline)
    SummonDeclineReason GetCurrentDeclineReason() const { return _currentDeclineReason; }

    /// Get decline reason as a human-readable string
    static ::std::string GetDeclineReasonString(SummonDeclineReason reason);

    /// Is the bot waiting to respond to a summon?
    bool IsWaitingToRespond() const { return _waitingToRespond; }

    /// Get time remaining before response (ms)
    uint32 GetResponseTimeRemaining() const { return _responseTimer; }

    // ========================================================================
    // STATISTICS
    // ========================================================================

    /// Get summon statistics
    SummonStats const& GetStats() const { return _stats; }

    /// Get summon history (last N entries)
    ::std::vector<SummonHistoryEntry> const& GetHistory() const { return _history; }

    /// Get formatted summary string
    ::std::string FormatSummary() const;

    /// Reset statistics
    void ResetStats();

private:
    // ========================================================================
    // INTERNAL
    // ========================================================================

    /// Calculate a random response delay between min and max
    uint32 CalculateResponseDelay() const;

    /// Record a summon event in history
    void RecordSummonEvent(ObjectGuid summoner, uint32 zoneId, bool accepted,
                           SummonDeclineReason reason);

    /// Check if the summoner is in the same group/raid
    bool IsSummonerInGroup() const;

    /// Check if the bot is crowd-controlled
    bool IsCrowdControlled() const;

    /// Check if we are on summon cooldown (anti-spam)
    bool IsOnSummonCooldown() const;

    /// Respond to the summon (accept or decline)
    void RespondToSummon(bool accept, SummonDeclineReason declineReason);

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    Player* _bot;

    // Response state
    bool _waitingToRespond{false};
    uint32 _responseTimer{0};
    SummonDeclineReason _currentDeclineReason{SummonDeclineReason::NONE};
    uint32 _summonDetectedTime{0};

    // Configuration
    uint32 _minResponseDelayMs{1500};
    uint32 _maxResponseDelayMs{4000};
    bool _autoAcceptGroupSummons{true};
    bool _acceptDuringCombat{false};
    uint32 _summonCooldownSec{30};

    // Statistics
    SummonStats _stats;

    // History (circular, last N entries)
    ::std::vector<SummonHistoryEntry> _history;
    static constexpr uint32 MAX_HISTORY_ENTRIES = 20;

    // Update interval
    static constexpr uint32 CHECK_INTERVAL_MS = 500;
    uint32 _checkTimer{0};
};

} // namespace Playerbot
