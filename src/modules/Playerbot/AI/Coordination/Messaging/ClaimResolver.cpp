/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ClaimResolver.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot
{

ClaimKey ClaimResolver::MakeKey(BotMessage const& msg) const
{
    ClaimKey key;
    key.type = msg.type;
    key.targetGuid = msg.targetGuid;
    key.spellOrAuraId = (msg.spellId > 0) ? msg.spellId : msg.auraId;
    return key;
}

ClaimStatus ClaimResolver::SubmitClaim(BotMessage const& message, ClaimCallback callback)
{
    if (!message.IsClaim())
    {
        TC_LOG_ERROR("playerbot.messaging", "ClaimResolver: Non-claim message submitted: {}",
            GetMessageTypeName(message.type));
        return ClaimStatus::DENIED;
    }

    std::lock_guard lock(_mutex);

    ClaimKey key = MakeKey(message);

    // Check if already claimed by someone else (and not expired)
    auto activeIt = _activeClaims.find(key);
    if (activeIt != _activeClaims.end())
    {
        auto now = std::chrono::steady_clock::now();
        if (now < activeIt->second.expiresAt)
        {
            // Already claimed, check if this is a higher priority claim
            if (message.claimPriority < activeIt->second.priority)
            {
                // Higher priority - override the existing claim
                TC_LOG_DEBUG("playerbot.messaging", "ClaimResolver: Priority override - {} overrides {}",
                    message.senderGuid.ToString(), activeIt->second.claimerGuid.ToString());

                // Notify previous claimer of denial
                auto callbackIt = _callbacks.find(activeIt->second.messageId);
                if (callbackIt != _callbacks.end())
                {
                    callbackIt->second(message, ClaimStatus::DENIED);
                    _callbacks.erase(callbackIt);
                }

                // Update the active claim
                activeIt->second.claimerGuid = message.senderGuid;
                activeIt->second.groupGuid = message.groupGuid;
                activeIt->second.priority = message.claimPriority;
                activeIt->second.claimedAt = message.timestamp;
                activeIt->second.expiresAt = message.expiryTime;
                activeIt->second.messageId = _nextMessageId++;

                if (callback)
                    _callbacks[activeIt->second.messageId] = callback;

                _stats.totalClaimsGranted++;
                return ClaimStatus::GRANTED;
            }
            else
            {
                // Same or lower priority - deny
                _stats.totalClaimsDenied++;
                TC_LOG_DEBUG("playerbot.messaging", "ClaimResolver: Claim denied - {} already claimed by {}",
                    key.targetGuid.ToString(), activeIt->second.claimerGuid.ToString());
                return ClaimStatus::DENIED;
            }
        }
        else
        {
            // Previous claim expired, remove it
            _activeClaims.erase(activeIt);
        }
    }

    // No active claim - add this one to pending claims for resolution
    PendingClaim pending;
    pending.message = message;
    pending.message.messageId = _nextMessageId++;
    pending.receivedAt = std::chrono::steady_clock::now();

    _pendingClaims[key].push_back(pending);

    if (callback)
        _callbacks[pending.message.messageId] = callback;

    _stats.totalClaimsSubmitted++;

    TC_LOG_DEBUG("playerbot.messaging", "ClaimResolver: Claim submitted - {} for {}",
        message.senderGuid.ToString(), key.targetGuid.ToString());

    return ClaimStatus::PENDING;
}

ClaimStatus ClaimResolver::GetClaimStatus(ObjectGuid claimerGuid, ClaimKey const& key) const
{
    std::lock_guard lock(_mutex);

    auto activeIt = _activeClaims.find(key);
    if (activeIt != _activeClaims.end())
    {
        if (activeIt->second.claimerGuid == claimerGuid)
        {
            auto now = std::chrono::steady_clock::now();
            if (now < activeIt->second.expiresAt)
                return ClaimStatus::GRANTED;
            else
                return ClaimStatus::EXPIRED;
        }
        return ClaimStatus::DENIED;
    }

    // Check pending
    auto pendingIt = _pendingClaims.find(key);
    if (pendingIt != _pendingClaims.end())
    {
        for (auto const& pending : pendingIt->second)
        {
            if (pending.message.senderGuid == claimerGuid)
                return ClaimStatus::PENDING;
        }
    }

    return ClaimStatus::DENIED;
}

bool ClaimResolver::IsTargetClaimed(ClaimKey const& key) const
{
    std::lock_guard lock(_mutex);

    auto it = _activeClaims.find(key);
    if (it != _activeClaims.end())
    {
        return std::chrono::steady_clock::now() < it->second.expiresAt;
    }
    return false;
}

ObjectGuid ClaimResolver::GetCurrentClaimer(ClaimKey const& key) const
{
    std::lock_guard lock(_mutex);

    auto it = _activeClaims.find(key);
    if (it != _activeClaims.end())
    {
        if (std::chrono::steady_clock::now() < it->second.expiresAt)
            return it->second.claimerGuid;
    }
    return ObjectGuid::Empty;
}

void ClaimResolver::ReleaseClaim(ObjectGuid claimerGuid, ClaimKey const& key)
{
    std::lock_guard lock(_mutex);

    auto it = _activeClaims.find(key);
    if (it != _activeClaims.end() && it->second.claimerGuid == claimerGuid)
    {
        // Remove callback
        _callbacks.erase(it->second.messageId);

        _activeClaims.erase(it);
        _stats.totalClaimsReleased++;

        TC_LOG_DEBUG("playerbot.messaging", "ClaimResolver: Claim released by {}",
            claimerGuid.ToString());
    }
}

void ClaimResolver::ReleaseAllClaims(ObjectGuid claimerGuid)
{
    std::lock_guard lock(_mutex);

    // Remove from active claims
    for (auto it = _activeClaims.begin(); it != _activeClaims.end(); )
    {
        if (it->second.claimerGuid == claimerGuid)
        {
            _callbacks.erase(it->second.messageId);
            it = _activeClaims.erase(it);
            _stats.totalClaimsReleased++;
        }
        else
        {
            ++it;
        }
    }

    // Remove from pending claims
    for (auto& [key, claims] : _pendingClaims)
    {
        claims.erase(
            std::remove_if(claims.begin(), claims.end(),
                [&claimerGuid](PendingClaim const& c) {
                    return c.message.senderGuid == claimerGuid;
                }),
            claims.end()
        );
    }

    TC_LOG_DEBUG("playerbot.messaging", "ClaimResolver: All claims released for {}",
        claimerGuid.ToString());
}

uint32 ClaimResolver::ProcessPendingClaims(std::chrono::steady_clock::time_point now)
{
    std::lock_guard lock(_mutex);

    uint32 resolved = 0;

    // Process each key with pending claims
    for (auto it = _pendingClaims.begin(); it != _pendingClaims.end(); )
    {
        ClaimKey const& key = it->first;
        std::vector<PendingClaim>& claims = it->second;

        if (claims.empty())
        {
            it = _pendingClaims.erase(it);
            continue;
        }

        // Check if claim window has passed for oldest claim
        auto oldestTime = claims.front().receivedAt;
        auto windowEnd = oldestTime + std::chrono::milliseconds(_claimWindowMs);

        if (now >= windowEnd)
        {
            // Resolve this claim group
            ResolveClaim(key, claims);
            resolved += static_cast<uint32>(claims.size());
            it = _pendingClaims.erase(it);
        }
        else
        {
            ++it;
        }
    }

    return resolved;
}

void ClaimResolver::ResolveClaim(ClaimKey const& key, std::vector<PendingClaim> const& claims)
{
    if (claims.empty())
        return;

    // Find the winning claim (highest priority = lowest numeric value)
    PendingClaim const* winner = &claims[0];
    for (size_t i = 1; i < claims.size(); ++i)
    {
        if (claims[i].message.claimPriority < winner->message.claimPriority)
        {
            winner = &claims[i];
        }
        else if (claims[i].message.claimPriority == winner->message.claimPriority)
        {
            // Same priority - first come first served
            if (claims[i].receivedAt < winner->receivedAt)
            {
                winner = &claims[i];
            }
        }
    }

    // Grant the winning claim
    ActiveClaim active;
    active.key = key;
    active.claimerGuid = winner->message.senderGuid;
    active.groupGuid = winner->message.groupGuid;
    active.priority = winner->message.claimPriority;
    active.claimedAt = winner->message.timestamp;
    active.expiresAt = winner->message.expiryTime;
    active.messageId = winner->message.messageId;

    _activeClaims[key] = active;
    _stats.totalClaimsGranted++;

    TC_LOG_DEBUG("playerbot.messaging", "ClaimResolver: Claim granted to {} for {}",
        winner->message.senderGuid.ToString(), key.targetGuid.ToString());

    // Notify winner
    NotifyClaimResult(*winner, ClaimStatus::GRANTED);

    // Deny all other claims
    for (auto const& claim : claims)
    {
        if (claim.message.senderGuid != winner->message.senderGuid)
        {
            NotifyClaimResult(claim, ClaimStatus::DENIED);
            _stats.totalClaimsDenied++;
        }
    }
}

void ClaimResolver::NotifyClaimResult(PendingClaim const& claim, ClaimStatus status)
{
    auto callbackIt = _callbacks.find(claim.message.messageId);
    if (callbackIt != _callbacks.end())
    {
        try
        {
            callbackIt->second(claim.message, status);
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR("playerbot.messaging", "ClaimResolver: Callback exception: {}", e.what());
        }
        _callbacks.erase(callbackIt);
    }
}

uint32 ClaimResolver::CleanupExpiredClaims(std::chrono::steady_clock::time_point now)
{
    std::lock_guard lock(_mutex);

    uint32 cleaned = 0;

    for (auto it = _activeClaims.begin(); it != _activeClaims.end(); )
    {
        if (now >= it->second.expiresAt)
        {
            _callbacks.erase(it->second.messageId);
            it = _activeClaims.erase(it);
            _stats.totalClaimsExpired++;
            cleaned++;
        }
        else
        {
            ++it;
        }
    }

    return cleaned;
}

} // namespace Playerbot
