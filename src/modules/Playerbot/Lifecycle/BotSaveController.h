/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * BotSaveController - Database save frequency tiering and coarse differential saves
 *
 * P6: Adjusts bot character save intervals based on AIBudgetTier:
 *   FULL    = 5 min  (active bots change state frequently)
 *   REDUCED = 15 min (traveling/city life, same as default)
 *   MINIMAL = 30 min (idle bots barely change state)
 *
 * P3: Skips saves entirely when bot persistent state hasn't changed
 *   (level, XP, gold, zone, equipped items, quest log).
 *
 * Uses Player::SetSaveTimer() (public API) - zero core modifications.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "AIBudgetTier.h"
#include <unordered_map>
#include <mutex>

class Player;

namespace Playerbot
{

/**
 * BotSaveController - Singleton managing bot save frequency and differential tracking
 *
 * Integration:
 * - Called from BotAI::OnBudgetTierTransition() when budget tier changes
 * - Called from BotAI::UpdateAI() on save timer tick to check state changes
 */
class TC_GAME_API BotSaveController
{
    BotSaveController() = default;
    ~BotSaveController() = default;
    BotSaveController(BotSaveController const&) = delete;
    BotSaveController& operator=(BotSaveController const&) = delete;

public:
    static BotSaveController* instance();

    /**
     * Called when a bot's AIBudgetTier changes.
     * Adjusts the save timer via Player::SetSaveTimer().
     */
    void OnBudgetTierChange(Player* bot, AIBudgetTier newTier);

    /**
     * Called before a bot save to check if the save can be skipped.
     * Compares current persistent state against stored checksums.
     * If state is unchanged, defers the save by resetting SetSaveTimer().
     *
     * @return true if save should proceed, false if skipped
     */
    bool ShouldSave(Player* bot);

    /**
     * Called after a bot save completes to update stored state checksums.
     */
    void OnSaveCompleted(Player* bot);

    /**
     * Remove tracking for a bot (logout, deletion).
     */
    void RemoveBot(ObjectGuid guid);

    /**
     * Get the save interval for a given budget tier (in milliseconds).
     */
    uint32 GetSaveIntervalForTier(AIBudgetTier tier) const;

    /**
     * Statistics for monitoring.
     */
    struct Stats
    {
        uint64 totalSaveChecks = 0;
        uint64 savesSkipped = 0;
        uint64 savesAllowed = 0;
        uint64 tierChanges = 0;

        float GetSkipRate() const
        {
            uint64 total = savesSkipped + savesAllowed;
            return total > 0 ? static_cast<float>(savesSkipped) / total : 0.0f;
        }
    };

    Stats const& GetStats() const { return _stats; }

private:
    /**
     * Persistent state snapshot for coarse differential saves.
     * Lightweight checksums to detect meaningful changes.
     */
    struct BotSaveState
    {
        uint8  level = 0;
        uint32 xp = 0;
        uint64 money = 0;
        uint32 zoneId = 0;
        uint32 inventoryChecksum = 0;   // Hash of equipped item entry IDs
        uint32 questLogChecksum = 0;    // Hash of active quest IDs + status
        bool   initialized = false;
    };

    /**
     * Compute a simple checksum of equipped item entries.
     */
    static uint32 ComputeInventoryChecksum(Player* bot);

    /**
     * Compute a simple checksum of active quest IDs and completion status.
     */
    static uint32 ComputeQuestLogChecksum(Player* bot);

    /**
     * Capture current persistent state snapshot from a Player.
     */
    static BotSaveState CaptureState(Player* bot);

    /**
     * Compare two state snapshots for meaningful differences.
     */
    static bool HasStateChanged(BotSaveState const& stored, BotSaveState const& current);

    // Per-bot state tracking (accessed from main thread only via BotAI::UpdateAI)
    std::unordered_map<ObjectGuid, BotSaveState> _botStates;
    std::mutex _mutex;  // Protects _botStates for rare cross-thread access

    // Configurable intervals (milliseconds)
    uint32 _fullInterval    = 5  * 60 * 1000;  // 5 minutes
    uint32 _reducedInterval = 15 * 60 * 1000;  // 15 minutes
    uint32 _minimalInterval = 30 * 60 * 1000;  // 30 minutes

    Stats _stats;
};

#define sBotSaveController BotSaveController::instance()

} // namespace Playerbot
