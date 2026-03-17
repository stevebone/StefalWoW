/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "SummonResponseManager.h"
#include "Player.h"
#include "Group.h"
#include "Log.h"
#include "GameTime.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"

#include <random>
#include <sstream>
#include <iomanip>

namespace Playerbot
{

// ============================================================================
// CONSTRUCTOR
// ============================================================================

SummonResponseManager::SummonResponseManager(Player* bot)
    : _bot(bot)
{
}

// ============================================================================
// MAIN UPDATE LOOP
// ============================================================================

bool SummonResponseManager::Update(uint32 diff)
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    // Throttle checks
    _checkTimer += diff;
    if (_checkTimer < CHECK_INTERVAL_MS)
        return false;
    _checkTimer = 0;

    // If we are already waiting to respond, count down the delay
    if (_waitingToRespond)
    {
        if (_responseTimer <= diff)
        {
            // Time to respond
            _responseTimer = 0;

            // Re-evaluate in case conditions changed during the delay
            SummonDeclineReason reason = EvaluateSummon();
            bool accept = (reason == SummonDeclineReason::NONE);

            RespondToSummon(accept, reason);
            _waitingToRespond = false;

            return accept;
        }
        else
        {
            _responseTimer -= diff;
            return false;
        }
    }

    // Check for new pending summon
    if (_bot->HasSummonPending() && !_waitingToRespond)
    {
        // New summon detected - start the response delay
        _waitingToRespond = true;
        _responseTimer = CalculateResponseDelay();
        _summonDetectedTime = GameTime::GetGameTimeMS();

        SummonDeclineReason reason = EvaluateSummon();
        _currentDeclineReason = reason;

        if (reason == SummonDeclineReason::NONE)
        {
            TC_LOG_DEBUG("module.playerbot", "SummonResponseManager [{}]: Summon pending, "
                "will accept in {} ms",
                _bot->GetName(), _responseTimer);
        }
        else
        {
            // If we know we'll decline, respond faster (shorter delay)
            _responseTimer = std::min(_responseTimer, _minResponseDelayMs);

            TC_LOG_DEBUG("module.playerbot", "SummonResponseManager [{}]: Summon pending, "
                "will decline in {} ms (reason: {})",
                _bot->GetName(), _responseTimer, GetDeclineReasonString(reason));
        }
    }

    return false;
}

// ============================================================================
// SUMMON EVALUATION
// ============================================================================

bool SummonResponseManager::HasPendingSummon() const
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    return _bot->HasSummonPending();
}

SummonDeclineReason SummonResponseManager::EvaluateSummon() const
{
    if (!_bot || !_bot->IsInWorld())
        return SummonDeclineReason::INVALID_SUMMONER;

    // Check if bot is dead
    if (!_bot->IsAlive())
        return SummonDeclineReason::DEAD;

    // Check if in combat (and combat acceptance is disabled)
    if (_bot->IsInCombat() && !_acceptDuringCombat)
        return SummonDeclineReason::IN_COMBAT;

    // Check if crowd-controlled
    if (IsCrowdControlled())
        return SummonDeclineReason::CROWD_CONTROLLED;

    // Check if in a battleground
    if (_bot->InBattleground())
        return SummonDeclineReason::IN_BATTLEGROUND;

    // Check if in arena
    if (_bot->InArena())
        return SummonDeclineReason::IN_ARENA;

    // Check if in flight
    if (_bot->IsInFlight())
    {
        // The core will handle taxi cancellation in SummonIfPossible (FinishTaxiFlight)
        // but we should not decline for this reason - let the core handle it
    }

    // Check anti-spam cooldown
    if (IsOnSummonCooldown())
        return SummonDeclineReason::COOLDOWN;

    // Check if summon is from a group member (if auto-accept is enabled)
    if (_autoAcceptGroupSummons)
    {
        if (!IsSummonerInGroup())
        {
            // Even non-group summons can be valid (warlock summon via /who, etc.)
            // but for bots we only accept group summons by default
            return SummonDeclineReason::NOT_IN_GROUP;
        }
    }

    // Check if summon has expired
    if (!_bot->HasSummonPending())
        return SummonDeclineReason::EXPIRED;

    return SummonDeclineReason::NONE;
}

void SummonResponseManager::ForceAcceptSummon()
{
    if (!_bot || !_bot->IsInWorld())
        return;

    if (_bot->HasSummonPending())
    {
        _waitingToRespond = false;
        RespondToSummon(true, SummonDeclineReason::NONE);
        TC_LOG_INFO("module.playerbot", "SummonResponseManager [{}]: Force-accepted summon",
            _bot->GetName());
    }
}

void SummonResponseManager::ForceDeclineSummon()
{
    if (!_bot || !_bot->IsInWorld())
        return;

    if (_bot->HasSummonPending())
    {
        _waitingToRespond = false;
        RespondToSummon(false, SummonDeclineReason::NONE);
        TC_LOG_INFO("module.playerbot", "SummonResponseManager [{}]: Force-declined summon",
            _bot->GetName());
    }
}

// ============================================================================
// QUERIES
// ============================================================================

::std::string SummonResponseManager::GetDeclineReasonString(SummonDeclineReason reason)
{
    switch (reason)
    {
        case SummonDeclineReason::NONE:              return "None";
        case SummonDeclineReason::IN_COMBAT:         return "In Combat";
        case SummonDeclineReason::CROWD_CONTROLLED:  return "Crowd Controlled";
        case SummonDeclineReason::DEAD:              return "Dead";
        case SummonDeclineReason::NOT_IN_GROUP:      return "Summoner Not in Group";
        case SummonDeclineReason::SUMMONER_HOSTILE:  return "Summoner Hostile";
        case SummonDeclineReason::IN_BATTLEGROUND:   return "In Battleground";
        case SummonDeclineReason::IN_ARENA:          return "In Arena";
        case SummonDeclineReason::ON_TAXI:           return "On Taxi";
        case SummonDeclineReason::INSTANCE_LOCKED:   return "Instance Lock Conflict";
        case SummonDeclineReason::COOLDOWN:          return "Summon Cooldown";
        case SummonDeclineReason::EXPIRED:           return "Summon Expired";
        case SummonDeclineReason::INVALID_SUMMONER:  return "Invalid Summoner";
        default: return "Unknown";
    }
}

// ============================================================================
// STATISTICS
// ============================================================================

::std::string SummonResponseManager::FormatSummary() const
{
    ::std::ostringstream oss;
    oss << "=== Summon Response Summary [" << _bot->GetName() << "] ===\n";
    oss << "  Received: " << _stats.summonsReceived << "\n";
    oss << "  Accepted: " << _stats.summonsAccepted << "\n";
    oss << "  Declined: " << _stats.summonsDeclined << "\n";
    oss << "  Expired:  " << _stats.summonsExpired << "\n";

    float acceptRate = (_stats.summonsReceived > 0)
        ? (static_cast<float>(_stats.summonsAccepted) / _stats.summonsReceived * 100.0f)
        : 0.0f;
    oss << "  Accept Rate: " << ::std::fixed << ::std::setprecision(1) << acceptRate << "%\n";

    if (!_history.empty())
    {
        oss << "  Recent History:\n";
        // Show last 5 entries
        size_t start = (_history.size() > 5) ? (_history.size() - 5) : 0;
        for (size_t i = start; i < _history.size(); ++i)
        {
            auto const& entry = _history[i];
            oss << "    [" << entry.timestamp << "] "
                << (entry.accepted ? "ACCEPTED" : "DECLINED")
                << " from " << entry.summonerGuid.ToString();
            if (!entry.accepted)
                oss << " (" << GetDeclineReasonString(entry.declineReason) << ")";
            oss << "\n";
        }
    }

    return oss.str();
}

void SummonResponseManager::ResetStats()
{
    _stats = SummonStats{};
    _history.clear();
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

uint32 SummonResponseManager::CalculateResponseDelay() const
{
    // Use a random delay between min and max to simulate human response time
    static thread_local ::std::mt19937 rng(::std::random_device{}());
    ::std::uniform_int_distribution<uint32> dist(_minResponseDelayMs, _maxResponseDelayMs);
    return dist(rng);
}

void SummonResponseManager::RecordSummonEvent(ObjectGuid summoner, uint32 zoneId,
                                               bool accepted, SummonDeclineReason reason)
{
    SummonHistoryEntry entry;
    entry.summonerGuid = summoner;
    entry.timestamp = GameTime::GetGameTimeMS();
    entry.zoneId = zoneId;
    entry.accepted = accepted;
    entry.declineReason = reason;

    _history.push_back(::std::move(entry));

    // Trim history if too large
    if (_history.size() > MAX_HISTORY_ENTRIES)
    {
        _history.erase(_history.begin(),
                       _history.begin() + static_cast<ptrdiff_t>(_history.size() - MAX_HISTORY_ENTRIES));
    }
}

bool SummonResponseManager::IsSummonerInGroup() const
{
    if (!_bot)
        return false;

    Group const* group = _bot->GetGroup();
    if (!group)
        return false;

    // The summon location is stored but we don't have direct access to the
    // summoner's GUID from the Player class. We check if the bot is in a group
    // at all - if they are, the summon is almost certainly from a group member
    // since in normal gameplay only group members can summon each other via
    // warlock summoning portal or meeting stone.
    //
    // For extra safety, we check group size > 1 (bot is not alone in group)
    return group->GetMembersCount() > 1;
}

bool SummonResponseManager::IsCrowdControlled() const
{
    if (!_bot)
        return false;

    // Check hard CC states
    if (_bot->HasUnitState(UNIT_STATE_STUNNED))
        return true;
    if (_bot->HasUnitState(UNIT_STATE_CONFUSED))
        return true;
    if (_bot->HasUnitState(UNIT_STATE_FLEEING))
        return true;

    // Check aura-based CC
    if (_bot->HasAuraType(SPELL_AURA_MOD_STUN))
        return true;
    if (_bot->HasAuraType(SPELL_AURA_MOD_FEAR))
        return true;
    if (_bot->HasAuraType(SPELL_AURA_MOD_CHARM))
        return true;
    if (_bot->HasAuraType(SPELL_AURA_TRANSFORM))
        return true;

    return false;
}

bool SummonResponseManager::IsOnSummonCooldown() const
{
    if (_stats.lastSummonTime == 0)
        return false;

    uint32 now = GameTime::GetGameTimeMS();
    uint32 cooldownMs = _summonCooldownSec * 1000;

    // Handle wrap-around
    if (now < _stats.lastSummonTime)
        return false;

    return (now - _stats.lastSummonTime) < cooldownMs;
}

void SummonResponseManager::RespondToSummon(bool accept, SummonDeclineReason declineReason)
{
    if (!_bot || !_bot->IsInWorld())
        return;

    // Verify summon is still pending
    if (!_bot->HasSummonPending())
    {
        TC_LOG_DEBUG("module.playerbot", "SummonResponseManager [{}]: Summon expired before response",
            _bot->GetName());

        _stats.summonsReceived++;
        _stats.summonsExpired++;
        RecordSummonEvent(ObjectGuid::Empty, 0, false, SummonDeclineReason::EXPIRED);
        return;
    }

    _stats.summonsReceived++;

    if (accept)
    {
        // Call the core's SummonIfPossible - this handles teleportation,
        // taxi cancellation, aura removal, criteria updates, and group notification
        _bot->SummonIfPossible(true);

        _stats.summonsAccepted++;
        _stats.lastSummonTime = GameTime::GetGameTimeMS();

        TC_LOG_INFO("module.playerbot", "SummonResponseManager [{}]: Accepted summon",
            _bot->GetName());
    }
    else
    {
        // Decline the summon - this clears m_summon_expire and notifies the group
        _bot->SummonIfPossible(false);

        _stats.summonsDeclined++;

        TC_LOG_INFO("module.playerbot", "SummonResponseManager [{}]: Declined summon (reason: {})",
            _bot->GetName(), GetDeclineReasonString(declineReason));
    }

    // Record in history
    RecordSummonEvent(ObjectGuid::Empty, _bot->GetZoneId(), accept, declineReason);
}

} // namespace Playerbot
