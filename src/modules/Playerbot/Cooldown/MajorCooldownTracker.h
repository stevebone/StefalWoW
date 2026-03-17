/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_MAJOR_COOLDOWN_TRACKER_H
#define PLAYERBOT_MAJOR_COOLDOWN_TRACKER_H

#include "CooldownEvents.h"
#include "MajorCooldownDatabase.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <mutex>
#include <chrono>

namespace Playerbot
{

/**
 * @brief Tracked major cooldown state
 */
struct TrackedMajorCooldown
{
    uint32 spellId;
    MajorCooldownTier tier;
    std::chrono::steady_clock::time_point availableAt;
    bool isOnCooldown;
};

/**
 * @brief Major Cooldown Tracker
 *
 * This singleton component subscribes to CooldownEvent (SPELL_COOLDOWN_START,
 * SPELL_COOLDOWN_CLEAR) and identifies major cooldowns using MajorCooldownDatabase.
 * When a major cooldown is detected, it publishes MAJOR_CD_USED / MAJOR_CD_AVAILABLE
 * events for raid/group coordination.
 *
 * Thread Safety: Uses mutex for thread-safe access to tracked cooldowns.
 *
 * Usage:
 *   // Initialize during playerbot module startup
 *   MajorCooldownTracker::instance()->Initialize();
 *
 *   // Check if a bot has a major CD available
 *   auto info = MajorCooldownTracker::instance()->GetCooldownState(botGuid, spellId);
 *   if (info && !info->isOnCooldown) { ... }
 *
 *   // Get all available external CDs for coordination
 *   auto available = MajorCooldownTracker::instance()->GetAvailableExternalCDs();
 */
class MajorCooldownTracker
{
public:
    static MajorCooldownTracker* instance()
    {
        static MajorCooldownTracker instance;
        return &instance;
    }

    /**
     * @brief Initialize the tracker and subscribe to cooldown events
     *
     * Must be called during playerbot module startup.
     */
    void Initialize();

    /**
     * @brief Shutdown the tracker and unsubscribe from events
     */
    void Shutdown();

    /**
     * @brief Handle a cooldown event
     *
     * Called by the event bus callback. Checks if the spell is a major CD
     * and publishes MAJOR_CD_USED / MAJOR_CD_AVAILABLE as appropriate.
     *
     * @param event The cooldown event to process
     */
    void HandleCooldownEvent(CooldownEvent const& event);

    /**
     * @brief Get the cooldown state for a specific bot and spell
     *
     * @param botGuid The bot's GUID
     * @param spellId The spell ID
     * @return Pointer to tracked state, or nullptr if not tracked
     */
    TrackedMajorCooldown const* GetCooldownState(ObjectGuid botGuid, uint32 spellId) const;

    /**
     * @brief Check if a bot has a specific major CD available
     *
     * @param botGuid The bot's GUID
     * @param spellId The spell ID
     * @return true if available (not on cooldown)
     */
    bool IsCooldownAvailable(ObjectGuid botGuid, uint32 spellId) const;

    /**
     * @brief Get all bots with a specific major CD available
     *
     * Useful for finding who can use Bloodlust, Battle Res, etc.
     *
     * @param spellId The spell ID
     * @return Vector of bot GUIDs that have this CD available
     */
    std::vector<ObjectGuid> GetBotsWithCDAvailable(uint32 spellId) const;

    /**
     * @brief Get all available external CDs (for defensive coordination)
     *
     * Returns a list of {botGuid, spellId, tier} for all external CDs
     * that are currently available.
     *
     * @return Vector of available external CDs
     */
    std::vector<std::tuple<ObjectGuid, uint32, MajorCooldownTier>> GetAvailableExternalCDs() const;

    /**
     * @brief Get all available raid-wide CDs
     *
     * @return Vector of available raid CDs
     */
    std::vector<std::tuple<ObjectGuid, uint32, MajorCooldownTier>> GetAvailableRaidCDs() const;

    /**
     * @brief Remove all tracked cooldowns for a bot (on bot despawn)
     *
     * @param botGuid The bot's GUID
     */
    void ClearBotCooldowns(ObjectGuid botGuid);

    /**
     * @brief Get statistics for monitoring
     */
    struct Statistics
    {
        uint32 trackedBots;
        uint32 trackedCooldowns;
        uint32 majorCDUsedEventsPublished;
        uint32 majorCDAvailableEventsPublished;
    };

    Statistics GetStatistics() const;

private:
    MajorCooldownTracker() = default;
    ~MajorCooldownTracker() = default;

    // Disable copy/move
    MajorCooldownTracker(MajorCooldownTracker const&) = delete;
    MajorCooldownTracker& operator=(MajorCooldownTracker const&) = delete;

    void HandleCooldownStart(CooldownEvent const& event);
    void HandleCooldownClear(CooldownEvent const& event);

    void PublishMajorCDUsed(ObjectGuid caster, uint32 spellId, MajorCooldownTier tier, uint32 cooldownMs);
    void PublishMajorCDAvailable(ObjectGuid caster, uint32 spellId, MajorCooldownTier tier);

    // Bot GUID -> Spell ID -> Tracked state
    using BotCooldownMap = std::unordered_map<uint32, TrackedMajorCooldown>;
    std::unordered_map<ObjectGuid, BotCooldownMap> _trackedCooldowns;
    mutable std::mutex _mutex;

    uint32 _callbackId{0};
    bool _initialized{false};

    // Statistics
    mutable std::atomic<uint32> _majorCDUsedCount{0};
    mutable std::atomic<uint32> _majorCDAvailableCount{0};
};

} // namespace Playerbot

#endif // PLAYERBOT_MAJOR_COOLDOWN_TRACKER_H
