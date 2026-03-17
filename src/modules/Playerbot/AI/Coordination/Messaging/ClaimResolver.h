/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_CLAIM_RESOLVER_H
#define PLAYERBOT_CLAIM_RESOLVER_H

#include "BotMessage.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>
#include <functional>

namespace Playerbot
{

/**
 * @brief Claim key for identifying unique claim targets
 *
 * A claim is uniquely identified by: messageType + targetGuid + (spellId OR auraId)
 * For example: CLAIM_INTERRUPT on target X for spell Y
 */
struct ClaimKey
{
    BotMessageType type;
    ObjectGuid targetGuid;
    uint32 spellOrAuraId;

    bool operator==(ClaimKey const& other) const
    {
        return type == other.type && targetGuid == other.targetGuid && spellOrAuraId == other.spellOrAuraId;
    }
};

} // namespace Playerbot

namespace std
{
    template<>
    struct hash<Playerbot::ClaimKey>
    {
        std::size_t operator()(Playerbot::ClaimKey const& key) const noexcept
        {
            std::size_t h1 = std::hash<uint8>{}(static_cast<uint8>(key.type));
            std::size_t h2 = std::hash<ObjectGuid>{}(key.targetGuid);
            std::size_t h3 = std::hash<uint32>{}(key.spellOrAuraId);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}

namespace Playerbot
{

/**
 * @brief Active claim record
 */
struct ActiveClaim
{
    ClaimKey key;
    ObjectGuid claimerGuid;
    ObjectGuid groupGuid;
    ClaimPriority priority;
    std::chrono::steady_clock::time_point claimedAt;
    std::chrono::steady_clock::time_point expiresAt;
    uint32 messageId;
};

/**
 * @brief Pending claim awaiting resolution
 */
struct PendingClaim
{
    BotMessage message;
    std::chrono::steady_clock::time_point receivedAt;
};

/**
 * @brief Claim Resolver - Handles claim conflicts between bots
 *
 * When multiple bots try to claim the same action (e.g., interrupt the same cast),
 * the ClaimResolver determines who wins. Resolution strategies:
 *
 * 1. First-Claim-Wins (default): First claim submitted wins within 200ms window
 * 2. Priority-Based: Higher priority claim wins (shorter CD, better positioned)
 * 3. Role-Based: Preferred role wins (healer for dispel, tank for taunt)
 *
 * Thread Safety: All public methods are thread-safe using mutex.
 *
 * Usage:
 *   // Submit a claim
 *   ClaimStatus status = resolver->SubmitClaim(claimMessage);
 *
 *   // Check if claim was granted
 *   if (status == ClaimStatus::GRANTED) { ... do the action ... }
 *
 *   // Release a claim (bot died, OOM, etc)
 *   resolver->ReleaseClaim(claimerGuid, claimKey);
 */
class ClaimResolver
{
public:
    using ClaimCallback = std::function<void(BotMessage const&, ClaimStatus)>;

    static ClaimResolver* instance()
    {
        static ClaimResolver instance;
        return &instance;
    }

    /**
     * @brief Submit a claim for an action
     *
     * Submits a claim for a specific action (interrupt, dispel, etc).
     * Returns immediately with PENDING status. The claim will be resolved
     * after the claim window (default 200ms) and the callback invoked.
     *
     * @param message The claim message
     * @param callback Optional callback when claim is resolved
     * @return ClaimStatus::PENDING if accepted for resolution
     */
    ClaimStatus SubmitClaim(BotMessage const& message, ClaimCallback callback = nullptr);

    /**
     * @brief Check the current status of a claim
     *
     * @param claimerGuid The bot that submitted the claim
     * @param key The claim key
     * @return Current claim status
     */
    ClaimStatus GetClaimStatus(ObjectGuid claimerGuid, ClaimKey const& key) const;

    /**
     * @brief Check if a claim is currently active for a target
     *
     * @param key The claim key
     * @return true if someone has claimed this target
     */
    bool IsTargetClaimed(ClaimKey const& key) const;

    /**
     * @brief Get the current claimer for a target
     *
     * @param key The claim key
     * @return GUID of the claimer, or empty if not claimed
     */
    ObjectGuid GetCurrentClaimer(ClaimKey const& key) const;

    /**
     * @brief Release a claim (voluntarily give up)
     *
     * Call this when a bot can no longer fulfill their claim (death, OOM, stunned).
     *
     * @param claimerGuid The bot releasing the claim
     * @param key The claim key
     */
    void ReleaseClaim(ObjectGuid claimerGuid, ClaimKey const& key);

    /**
     * @brief Release all claims by a bot (on death/disconnect)
     *
     * @param claimerGuid The bot releasing all claims
     */
    void ReleaseAllClaims(ObjectGuid claimerGuid);

    /**
     * @brief Process pending claims and resolve conflicts
     *
     * Called periodically (every tick) to resolve pending claims after
     * their claim windows expire.
     *
     * @param now Current time
     * @return Number of claims resolved
     */
    uint32 ProcessPendingClaims(std::chrono::steady_clock::time_point now);

    /**
     * @brief Clean up expired claims
     *
     * @param now Current time
     * @return Number of claims cleaned up
     */
    uint32 CleanupExpiredClaims(std::chrono::steady_clock::time_point now);

    /**
     * @brief Set the claim window duration
     *
     * @param ms Duration in milliseconds (default 200ms)
     */
    void SetClaimWindowMs(uint32 ms) { _claimWindowMs = ms; }

    /**
     * @brief Get statistics
     */
    struct Statistics
    {
        std::atomic<uint32> totalClaimsSubmitted{0};
        std::atomic<uint32> totalClaimsGranted{0};
        std::atomic<uint32> totalClaimsDenied{0};
        std::atomic<uint32> totalClaimsReleased{0};
        std::atomic<uint32> totalClaimsExpired{0};
    };

    Statistics const& GetStatistics() const { return _stats; }

private:
    ClaimResolver() = default;
    ~ClaimResolver() = default;

    // Disable copy/move
    ClaimResolver(ClaimResolver const&) = delete;
    ClaimResolver& operator=(ClaimResolver const&) = delete;

    ClaimKey MakeKey(BotMessage const& msg) const;
    void ResolveClaim(ClaimKey const& key, std::vector<PendingClaim> const& claims);
    void NotifyClaimResult(PendingClaim const& claim, ClaimStatus status);

    // Active claims: key -> ActiveClaim
    std::unordered_map<ClaimKey, ActiveClaim> _activeClaims;

    // Pending claims awaiting resolution: key -> vector of competing claims
    std::unordered_map<ClaimKey, std::vector<PendingClaim>> _pendingClaims;

    // Callbacks for claim resolution
    std::unordered_map<uint32, ClaimCallback> _callbacks;

    mutable std::mutex _mutex;
    uint32 _nextMessageId{1};
    uint32 _claimWindowMs{200};

    Statistics _stats;
};

} // namespace Playerbot

#endif // PLAYERBOT_CLAIM_RESOLVER_H
