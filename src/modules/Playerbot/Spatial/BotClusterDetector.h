/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * BotClusterDetector - Anti-cluster live dispersal for idle bots
 *
 * R1: Detects clusters of 8+ bots within 15 yards and nudges excess
 * MINIMAL/REDUCED tier bots to nearby positions using BotMovementUtil.
 *
 * Uses SpatialGridManager snapshots — thread-safe, zero core modifications.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <unordered_set>

class Player;

namespace Playerbot
{

class TC_GAME_API BotClusterDetector
{
    BotClusterDetector() = default;
    ~BotClusterDetector() = default;
    BotClusterDetector(BotClusterDetector const&) = delete;
    BotClusterDetector& operator=(BotClusterDetector const&) = delete;

public:
    static BotClusterDetector* instance();

    /**
     * Initialize with default or config-based settings.
     */
    void Initialize();

    /**
     * Periodic update — call from module's Update(diff) loop.
     * Checks for clusters every _checkIntervalMs (default 30s).
     */
    void Update(uint32 diff);

    /**
     * Statistics for monitoring.
     */
    struct Stats
    {
        uint64 checksPerformed = 0;
        uint64 clustersDetected = 0;
        uint64 botsDispersed = 0;
    };

    Stats const& GetStats() const { return _stats; }

private:
    /**
     * Scan a single map for bot clusters and disperse excess bots.
     */
    void ScanMapForClusters(uint32 mapId);

    /**
     * Check if a bot is eligible for dispersal.
     * Ineligible: FULL tier, in BG/dungeon/instance, in combat, human player.
     */
    bool IsEligibleForDispersal(Player* bot) const;

    // Configuration
    bool   _enabled = true;
    uint32 _checkIntervalMs = 30 * 1000;  // 30 seconds
    uint32 _clusterThreshold = 8;          // Min bots to consider a cluster
    float  _clusterRadius = 15.0f;         // Yards
    float  _dispersalDistance = 25.0f;      // How far to nudge bots

    // Timer
    uint32 _timer = 0;

    // Cooldown: track recently dispersed bots to avoid re-nudging
    std::unordered_set<ObjectGuid> _recentlyDispersed;
    uint32 _cooldownClearTimer = 0;
    static constexpr uint32 COOLDOWN_CLEAR_INTERVAL = 60 * 1000;  // 60s

    Stats _stats;
};

#define sBotClusterDetector BotClusterDetector::instance()

} // namespace Playerbot
