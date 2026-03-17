/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * BotClusterDetector Implementation
 *
 * R1: Periodic cluster detection and dispersal of idle bots.
 * Uses BotWorldSessionMgr::GetAllBotPlayers() + position grouping.
 */

#include "BotClusterDetector.h"
#include "BotWorldSessionMgr.h"
#include "BotAI.h"
#include "PlayerBotHelpers.h"
#include "Player.h"
#include "MotionMaster.h"
#include "Map.h"
#include "Log.h"
#include "WorldSession.h"
#include <unordered_map>
#include <vector>

namespace Playerbot
{

BotClusterDetector* BotClusterDetector::instance()
{
    static BotClusterDetector inst;
    return &inst;
}

void BotClusterDetector::Initialize()
{
    _enabled = true;
    _timer = 0;
    _cooldownClearTimer = 0;
    _recentlyDispersed.clear();
    _stats = {};

    TC_LOG_INFO("module.playerbot",
        "BotClusterDetector: Initialized (threshold={}, radius={:.0f}yd, interval={}s)",
        _clusterThreshold, _clusterRadius, _checkIntervalMs / 1000);
}

void BotClusterDetector::Update(uint32 diff)
{
    if (!_enabled)
        return;

    // Cooldown clear timer
    _cooldownClearTimer += diff;
    if (_cooldownClearTimer >= COOLDOWN_CLEAR_INTERVAL)
    {
        _cooldownClearTimer = 0;
        _recentlyDispersed.clear();
    }

    // Main check timer
    _timer += diff;
    if (_timer < _checkIntervalMs)
        return;
    _timer = 0;

    _stats.checksPerformed++;

    // Get all active bot players
    auto allBots = Playerbot::BotWorldSessionMgr::instance()->GetAllBotPlayers();
    if (allBots.size() < _clusterThreshold)
        return;  // Not enough bots to form any cluster

    // Group bots by mapId
    std::unordered_map<uint32, std::vector<Player*>> botsByMap;
    for (Player* bot : allBots)
    {
        if (!bot || !bot->IsInWorld() || !bot->IsAlive())
            continue;
        botsByMap[bot->GetMapId()].push_back(bot);
    }

    // Check each map for clusters
    for (auto& [mapId, bots] : botsByMap)
    {
        if (bots.size() < _clusterThreshold)
            continue;

        // Simple O(n^2) cluster detection with visited tracking
        // For each unvisited bot, flood-fill neighbors within radius
        std::vector<bool> visited(bots.size(), false);

        for (size_t i = 0; i < bots.size(); ++i)
        {
            if (visited[i])
                continue;

            // Build cluster starting from bot[i]
            std::vector<size_t> cluster;
            std::vector<size_t> queue;
            queue.push_back(i);
            visited[i] = true;

            while (!queue.empty())
            {
                size_t current = queue.back();
                queue.pop_back();
                cluster.push_back(current);

                Position const& pos = bots[current]->GetPosition();
                for (size_t j = 0; j < bots.size(); ++j)
                {
                    if (visited[j])
                        continue;

                    float dist = pos.GetExactDist2d(bots[j]->GetPosition());
                    if (dist <= _clusterRadius)
                    {
                        visited[j] = true;
                        queue.push_back(j);
                    }
                }
            }

            // Check if cluster exceeds threshold
            if (cluster.size() < _clusterThreshold)
                continue;

            _stats.clustersDetected++;

            TC_LOG_DEBUG("module.playerbot",
                "BotClusterDetector: Cluster of {} bots on map {} near ({:.0f}, {:.0f})",
                cluster.size(), mapId,
                bots[cluster[0]]->GetPositionX(),
                bots[cluster[0]]->GetPositionY());

            // Keep up to 5 bots in place, disperse the rest
            std::vector<size_t> eligible;
            uint32 kept = 0;
            static constexpr uint32 MAX_KEEP = 5;

            for (size_t idx : cluster)
            {
                if (kept < MAX_KEEP || !IsEligibleForDispersal(bots[idx]))
                {
                    ++kept;
                    continue;
                }
                eligible.push_back(idx);
            }

            // Disperse eligible excess bots using MotionMaster::MoveRandom
            for (size_t idx : eligible)
            {
                Player* bot = bots[idx];
                ObjectGuid guid = bot->GetGUID();

                // Skip if recently dispersed
                if (_recentlyDispersed.count(guid))
                    continue;

                MotionMaster* mm = bot->GetMotionMaster();
                if (!mm)
                    continue;

                // Use MoveRandom with walk speed for natural-looking dispersal
                mm->MoveRandom(_dispersalDistance, Milliseconds(60000), {},
                    MovementWalkRunSpeedSelectionMode::ForceWalk);

                _recentlyDispersed.insert(guid);
                _stats.botsDispersed++;

                TC_LOG_DEBUG("module.playerbot",
                    "BotClusterDetector: Dispersed bot {} from cluster on map {}",
                    bot->GetName(), mapId);
            }
        }
    }
}

bool BotClusterDetector::IsEligibleForDispersal(Player* bot) const
{
    if (!bot || !bot->IsInWorld())
        return false;

    // Never disperse human players
    WorldSession* session = bot->GetSession();
    if (!session || !session->IsBot())
        return false;

    // Never disperse bots in combat
    if (bot->IsInCombat())
        return false;

    // Never disperse bots in BG or dungeon/raid
    if (bot->InBattleground())
        return false;
    if (Map* map = bot->GetMap())
    {
        if (map->IsDungeon() || map->IsRaid())
            return false;
    }

    // Only disperse MINIMAL or REDUCED tier bots
    BotAI* ai = Playerbot::GetBotAI(bot);
    if (!ai)
        return false;

    AIBudgetTier tier = ai->GetCurrentBudgetTier();
    return tier == AIBudgetTier::MINIMAL || tier == AIBudgetTier::REDUCED;
}

} // namespace Playerbot
