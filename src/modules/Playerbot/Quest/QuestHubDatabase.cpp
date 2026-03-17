/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "QuestHubDatabase.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QueryResult.h"
#include "World.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <set>

namespace Playerbot
{
    // ============================================================================
    // QuestHub Implementation
    // ============================================================================

    bool QuestHub::IsAppropriateFor(Player const* player) const
    {
        if (!player)
            return false;

        // CRITICAL: Check if player is on the same map as this quest hub
        // Without this check, hubs on different continents would be considered!
        if (player->GetMapId() != mapId)
            return false;

        // Check level range
        uint32 playerLevel = player->GetLevel();
        if (playerLevel < minLevel)
            return false;

        if (maxLevel > 0 && playerLevel > maxLevel)
            return false;

        // Check faction
        TeamId team = player->GetTeamId();
        uint32 playerFactionBit = 0;

        switch (team)
        {
            case TEAM_ALLIANCE:
                playerFactionBit = 0x01; // Bit 0
                break;
            case TEAM_HORDE:
                playerFactionBit = 0x02; // Bit 1
                break;
            default:
                playerFactionBit = 0x04; // Bit 2 (Neutral)
                break;
        }

        // Check if faction is allowed
        if (!(factionMask & playerFactionBit))
            return false;

        return true;
    }

    float QuestHub::GetDistanceFrom(Player const* player) const
    {
        if (!player)
            return ::std::numeric_limits<float>::max();

        // CRITICAL: If player is on a different map, return infinite distance
        // This prevents nonsensical distance calculations across continents
        if (player->GetMapId() != mapId)
            return ::std::numeric_limits<float>::max();

        // Use 2D distance for performance (Z-axis less relevant for quest selection)
        float dx = player->GetPositionX() - location.GetPositionX();
        float dy = player->GetPositionY() - location.GetPositionY();
        return ::std::sqrt(dx * dx + dy * dy);
    }

    bool QuestHub::ContainsPosition(Position const& pos) const
    {
        float dx = pos.GetPositionX() - location.GetPositionX();
        float dy = pos.GetPositionY() - location.GetPositionY();
        float distance = ::std::sqrt(dx * dx + dy * dy);
        return distance <= radius;
    }

    float QuestHub::CalculateSuitabilityScore(Player const* player) const
    {
        if (!IsAppropriateFor(player))
            return 0.0f;

        float score = 100.0f; // Base score

        // Level appropriateness scoring
        uint32 playerLevel = player->GetLevel();
        uint32 hubMidLevel = (minLevel + maxLevel) / 2;
        int32 levelDiff = ::std::abs(static_cast<int32>(playerLevel) - static_cast<int32>(hubMidLevel));

        // Ideal: ±2 levels = full score
        // ±5 levels = 50% score
        // ±10 levels = 10% score
        float levelScore = ::std::max(0.0f, 1.0f - (levelDiff / 10.0f));
        score *= levelScore;

        // Distance penalty (closer is better)
        float distance = GetDistanceFrom(player);
        float distanceScore = 1.0f / (1.0f + distance / 1000.0f); // Normalize to 0-1 range
        score *= distanceScore;

        // Quest availability bonus
        float questBonus = 1.0f + (questIds.size() * 0.1f); // 10% bonus per quest
        score *= ::std::min(2.0f, questBonus); // Cap at 2x multiplier

        return score;
    }

    // ============================================================================
    // QuestHubDatabase Implementation
    // ============================================================================

    QuestHubDatabase& QuestHubDatabase::Instance()
    {
        static QuestHubDatabase instance;
        return instance;
    }

    bool QuestHubDatabase::Initialize()
    {
        TC_LOG_INFO("playerbot", "QuestHubDatabase: Initializing quest hub database...");

        auto startTime = ::std::chrono::steady_clock::now();

        try
        {
            // Step 1: Load quest givers from database
            uint32 questGiverCount = LoadQuestGiversFromDB();
            if (questGiverCount == 0)
            {
                TC_LOG_ERROR("playerbot", "QuestHubDatabase: No quest givers found in database!");
                return false;
            }

            TC_LOG_DEBUG("playerbot", "QuestHubDatabase: Loaded {} quest givers", questGiverCount);

            // Step 2: Cluster quest givers into hubs
            uint32 hubCount = ClusterQuestGiversIntoHubs();
            if (hubCount == 0)
            {
                TC_LOG_ERROR("playerbot", "QuestHubDatabase: Failed to create quest hubs!");
                return false;
            }

            TC_LOG_DEBUG("playerbot", "QuestHubDatabase: Created {} quest hubs", hubCount);

            // Step 3: Load quest data for hubs
            LoadQuestDataForHubs();

            // Step 4: Build spatial index
            BuildSpatialIndex();

            // Step 5: Validate data
            ValidateHubData();

            // Calculate memory usage
            _memoryUsage = _questHubs.size() * sizeof(QuestHub);
            _memoryUsage += _hubIdToIndex.size() * (sizeof(uint32) + sizeof(size_t));
            for (auto const& [zoneId, indices] : _zoneIndex)
                _memoryUsage += indices.size() * sizeof(size_t);

            _initialized = true;

            auto endTime = ::std::chrono::steady_clock::now();
            auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(endTime - startTime);

            TC_LOG_INFO("playerbot", "QuestHubDatabase: Initialization complete. "
                                     "{} hubs loaded in {} ms. Memory usage: {} KB",
                        hubCount, duration.count(), _memoryUsage / 1024);

            return true;
        }
        catch (::std::exception const& ex)
        {
            TC_LOG_ERROR("playerbot", "QuestHubDatabase: Exception during initialization: {}", ex.what());
            return false;
        }
    }

    bool QuestHubDatabase::Reload()
    {
        TC_LOG_INFO("playerbot", "QuestHubDatabase: Reloading quest hub database...");

        // Exclusive lock for writing
        ::std::unique_lock lock(_mutex);

        // Clear existing data
        _questHubs.clear();
        _hubIdToIndex.clear();
        _zoneIndex.clear();
        _initialized = false;
        _memoryUsage = 0;

        // Release lock before calling Initialize (which will acquire its own locks as needed)
        lock.unlock();

        return Initialize();
    }

    ::std::vector<QuestHub const*> QuestHubDatabase::GetQuestHubsForPlayer(
        Player const* player,
        uint32 maxCount) const
    {
        if (!player || !_initialized)
            return {};

        // Shared lock for reading
        ::std::shared_lock lock(_mutex);

        // Collect all appropriate hubs with their scores
        ::std::vector<::std::pair<QuestHub const*, float>> scoredHubs;
        scoredHubs.reserve(_questHubs.size());

        for (auto const& hub : _questHubs)
        {
            float score = hub.CalculateSuitabilityScore(player);
            if (score > 0.0f)
                scoredHubs.emplace_back(&hub, score);
        }

        // Sort by score (highest first)
        ::std::sort(scoredHubs.begin(), scoredHubs.end(),
            [](auto const& a, auto const& b) { return a.second > b.second; });

        // Extract top N hubs
        ::std::vector<QuestHub const*> result;
        result.reserve(::std::min(maxCount, static_cast<uint32>(scoredHubs.size())));

        for (size_t i = 0; i < scoredHubs.size() && i < maxCount; ++i)
            result.push_back(scoredHubs[i].first);

        return result;
    }

    QuestHub const* QuestHubDatabase::GetNearestQuestHub(Player const* player) const
    {
        if (!player || !_initialized)
            return nullptr;

        // Shared lock for reading
        ::std::shared_lock lock(_mutex);

        QuestHub const* nearest = nullptr;
        float minDistance = ::std::numeric_limits<float>::max();

        for (auto const& hub : _questHubs)
        {
            if (!hub.IsAppropriateFor(player))
                continue;

            float distance = hub.GetDistanceFrom(player);
            if (distance < minDistance)
            {
                minDistance = distance;
                nearest = &hub;
            }
        }

        return nearest;
    }

    QuestHub const* QuestHubDatabase::GetQuestHubById(uint32 hubId) const
    {
        if (!_initialized)
            return nullptr;

        // Shared lock for reading
        ::std::shared_lock lock(_mutex);

        auto it = _hubIdToIndex.find(hubId);
        if (it == _hubIdToIndex.end())
            return nullptr;

        return &_questHubs[it->second];
    }

    ::std::vector<QuestHub const*> QuestHubDatabase::GetQuestHubsInZone(uint32 zoneId) const
    {
        if (!_initialized)
            return {};

        // Shared lock for reading
        ::std::shared_lock lock(_mutex);

        auto it = _zoneIndex.find(zoneId);
        if (it == _zoneIndex.end())
            return {};

        ::std::vector<QuestHub const*> result;
        result.reserve(it->second.size());

        for (size_t index : it->second)
            result.push_back(&_questHubs[index]);

        return result;
    }

    QuestHub const* QuestHubDatabase::GetQuestHubAtPosition(
        Position const& pos,
        ::std::optional<uint32> zoneId) const
    {
        if (!_initialized)
            return nullptr;

        // Shared lock for reading
        ::std::shared_lock lock(_mutex);

        // If zone is specified, only check hubs in that zone
    if (zoneId.has_value())
        {
            auto it = _zoneIndex.find(*zoneId);
            if (it == _zoneIndex.end())
                return nullptr;

            for (size_t index : it->second)
            {
                if (_questHubs[index].ContainsPosition(pos))
                    return &_questHubs[index];
            }
        }
        else
        {
            // Check all hubs
    for (auto const& hub : _questHubs)
            {
                if (hub.ContainsPosition(pos))
                    return &hub;
            }
        }

        return nullptr;
    }

    size_t QuestHubDatabase::GetQuestHubCount() const
    {
        ::std::shared_lock lock(_mutex);
        return _questHubs.size();
    }

    size_t QuestHubDatabase::GetMemoryUsage() const
    {
        ::std::shared_lock lock(_mutex);
        return _memoryUsage;
    }

    // ============================================================================
    // Private Helper Methods
    // ============================================================================

    // QuestGiverData struct is now defined in the header (required for std::vector)

    uint32 QuestHubDatabase::LoadQuestGiversFromDB()
    {
        _tempQuestGivers.clear();

        // Query creature spawns that are quest givers (ALL MAPS - including expansions) using prepared statement
        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_QUEST_GIVER_SPAWNS);
        PreparedQueryResult result = WorldDatabase.Query(stmt);

        uint32 count = 0;
        ::std::unordered_map<uint32, uint32> zoneDistribution;
        ::std::unordered_map<uint32, uint32> mapDistribution;

        do
        {
            Field* fields = result->Fetch();

            QuestGiverData data;
            data.creatureEntry = fields[1].GetUInt32();
            data.position.Relocate(
                fields[2].GetFloat(),  // x
                fields[3].GetFloat(),  // y
                fields[4].GetFloat()   // z
            );
            data.mapId = fields[5].GetUInt32();
            data.zoneId = fields[7].GetUInt32();
            data.factionTemplate = fields[6].GetUInt32();

            _tempQuestGivers.push_back(data);
            ++count;

            // Track distribution
            zoneDistribution[data.zoneId]++;
            mapDistribution[data.mapId]++;

        } while (result->NextRow());

        TC_LOG_INFO("playerbot", "QuestHubDatabase: Loaded {} quest givers from database", count);

        // Log map distribution with expansion names
        ::std::vector<::std::pair<uint32, uint32>> mapCounts(mapDistribution.begin(), mapDistribution.end());
        ::std::sort(mapCounts.begin(), mapCounts.end(),
            [](auto const& a, auto const& b) { return a.second > b.second; });

        TC_LOG_INFO("playerbot", "QuestHubDatabase: Quest giver distribution across {} maps:", mapCounts.size());
        for (size_t i = 0; i < ::std::min(size_t(10), mapCounts.size()); ++i)
        {
            ::std::string mapName;
            switch (mapCounts[i].first)
            {
                case 0: mapName = "Eastern Kingdoms"; break;
                case 1: mapName = "Kalimdor"; break;
                case 530: mapName = "Outland"; break;
                case 571: mapName = "Northrend"; break;
                case 654: mapName = "Pandaria"; break;
                case 648: mapName = "Broken Isles"; break;
                case 646: mapName = "Broken Isles (Dalaran)"; break;
                case 1643: mapName = "Ardenweald"; break;
                case 1220: mapName = "Broken Shore"; break;
                default:
                    mapName = "Map ";
                    mapName += ::std::to_string(mapCounts[i].first);
                    break;
            }
            TC_LOG_INFO("playerbot", "  {}: {} quest givers", mapName, mapCounts[i].second);
        }

        // Log top 5 zones by quest giver count
        ::std::vector<::std::pair<uint32, uint32>> zoneCounts(zoneDistribution.begin(), zoneDistribution.end());
        ::std::sort(zoneCounts.begin(), zoneCounts.end(),
            [](auto const& a, auto const& b) { return a.second > b.second; });

        TC_LOG_INFO("playerbot", "QuestHubDatabase: Top zones by quest giver count:");
        for (size_t i = 0; i < ::std::min(size_t(5), zoneCounts.size()); ++i)
        {
            TC_LOG_INFO("playerbot", "  Zone {}: {} quest givers", zoneCounts[i].first, zoneCounts[i].second);
        }

        // Log sample positions
    if (count > 0)
        {
            TC_LOG_DEBUG("playerbot", "QuestHubDatabase: Sample quest giver positions:");
            for (size_t i = 0; i < ::std::min(size_t(5), _tempQuestGivers.size()); ++i)
            {
                TC_LOG_DEBUG("playerbot", "  Entry {} at ({:.2f}, {:.2f}, {:.2f}) in zone {}",
                    _tempQuestGivers[i].creatureEntry,
                    _tempQuestGivers[i].position.GetPositionX(),
                    _tempQuestGivers[i].position.GetPositionY(),
                    _tempQuestGivers[i].position.GetPositionZ(),
                    _tempQuestGivers[i].zoneId);
            }
        }

        return count;
    }

    uint32 QuestHubDatabase::ClusterQuestGiversIntoHubs()
    {
        TC_LOG_ERROR("playerbot", "QuestHubDatabase: !!!!! CLUSTER FUNCTION ENTERED - _tempQuestGivers.size() = {} !!!!!", _tempQuestGivers.size());

        if (_tempQuestGivers.empty())
        {
            TC_LOG_ERROR("playerbot", "QuestHubDatabase: No quest giver data to cluster!");
            return 0;
        }

        TC_LOG_INFO("playerbot", "QuestHubDatabase: Starting DBSCAN clustering on {} quest givers",
            _tempQuestGivers.size());

        // DBSCAN clustering parameters
        constexpr float EPSILON = 75.0f;  // 75 yards search radius
        constexpr uint32 MIN_POINTS = 2;  // Minimum 2 quest givers per hub

        TC_LOG_INFO("playerbot", "QuestHubDatabase: Clustering parameters - EPSILON={} yards, MIN_POINTS={}",
            EPSILON, MIN_POINTS);

        ::std::vector<bool> visited(_tempQuestGivers.size(), false);
        ::std::vector<int32> clusterIds(_tempQuestGivers.size(), -1); // -1 = noise
        int32 currentClusterId = 0;

        // Lambda: Find neighbors within EPSILON (MUST be on same map!)
        auto findNeighbors = [&](size_t index) -> ::std::vector<size_t>
        {
            ::std::vector<size_t> neighbors;
            Position const& pos = _tempQuestGivers[index].position;
            uint32 mapId = _tempQuestGivers[index].mapId;

            for (size_t i = 0; i < _tempQuestGivers.size(); ++i)
            {
                if (i == index)
                    continue;

                // CRITICAL: Only cluster quest givers on the SAME MAP
                // Without this check, quest givers on different continents could be
                // incorrectly clustered together based on similar X/Y coordinates
                if (_tempQuestGivers[i].mapId != mapId)
                    continue;

                Position const& otherPos = _tempQuestGivers[i].position;
                float dx = pos.GetPositionX() - otherPos.GetPositionX();
                float dy = pos.GetPositionY() - otherPos.GetPositionY();
                float distance = ::std::sqrt(dx * dx + dy * dy);

                if (distance <= EPSILON)
                    neighbors.push_back(i);
            }

            return neighbors;
        };

        // Sample neighbor counts for diagnostic
        ::std::vector<size_t> sampleNeighborCounts;
        for (size_t i = 0; i < ::std::min(size_t(10), _tempQuestGivers.size()); ++i)
        {
            sampleNeighborCounts.push_back(findNeighbors(i).size());
        }

        TC_LOG_INFO("playerbot", "QuestHubDatabase: Sample neighbor counts for first 10 quest givers:");
        for (size_t i = 0; i < sampleNeighborCounts.size(); ++i)
        {
            TC_LOG_INFO("playerbot", "  QuestGiver {}: {} neighbors within {} yards",
                i, sampleNeighborCounts[i], EPSILON);
        }

        // DBSCAN algorithm
        uint32 noiseCount = 0;
        for (size_t i = 0; i < _tempQuestGivers.size(); ++i)
        {
            if (visited[i])
                continue;

            visited[i] = true;
            ::std::vector<size_t> neighbors = findNeighbors(i);

            if (neighbors.size() < MIN_POINTS)
            {
                // Mark as noise (singleton quest giver)
                clusterIds[i] = -1;
                ++noiseCount;
                continue;
            }

            // Start new cluster
            clusterIds[i] = currentClusterId;
            TC_LOG_DEBUG("playerbot", "QuestHubDatabase: Started cluster {} with {} initial neighbors",
                currentClusterId, neighbors.size());

            // Expand cluster
    for (size_t j = 0; j < neighbors.size(); ++j)
            {
                size_t neighborIdx = neighbors[j];

                if (!visited[neighborIdx])
                {
                    visited[neighborIdx] = true;
                    ::std::vector<size_t> neighborNeighbors = findNeighbors(neighborIdx);

                    if (neighborNeighbors.size() >= MIN_POINTS)
                    {
                        // Add new neighbors to expansion list
                        neighbors.insert(neighbors.end(),
                                       neighborNeighbors.begin(),
                                       neighborNeighbors.end());
                    }
                }

                if (clusterIds[neighborIdx] == -1)
                    clusterIds[neighborIdx] = currentClusterId;
            }

            ++currentClusterId;
        }

        TC_LOG_INFO("playerbot", "QuestHubDatabase: DBSCAN clustering complete - {} clusters formed, {} noise points",
            currentClusterId, noiseCount);

        // Create QuestHub objects from clusters
        ::std::unordered_map<int32, ::std::vector<size_t>> clusterMap;
        uint32 noisePoints = 0;

        for (size_t i = 0; i < clusterIds.size(); ++i)
        {
            if (clusterIds[i] >= 0)
            {
                clusterMap[clusterIds[i]].push_back(i);
            }
            else
            {
                // This is a singleton/noise point - excluded from hubs as intended
                ++noisePoints;
            }
        }

        TC_LOG_INFO("playerbot", "QuestHubDatabase: Cluster analysis - {} real clusters formed, {} singleton quest givers excluded",
            currentClusterId, noisePoints);

        if (currentClusterId == 0)
        {
            TC_LOG_ERROR("playerbot", "QuestHubDatabase: No quest hubs were formed by clustering!");
            TC_LOG_ERROR("playerbot", "QuestHubDatabase: This indicates EPSILON ({} yards) may be too small, or quest givers are too far apart", 75.0f);
            TC_LOG_ERROR("playerbot", "QuestHubDatabase: Consider increasing EPSILON or investigating quest giver spatial distribution");
            return 0;
        }

        uint32 hubId = 1;
        for (auto const& [clusterId, indices] : clusterMap)
        {
            // PROPER IMPLEMENTATION: Only create hubs from actual clusters (not singletons)
            // This maintains the purpose of QuestHubDatabase: directing bots to EFFICIENT LEVELING HUBS
    if (indices.size() < MIN_POINTS)
            {
                TC_LOG_WARN("playerbot", "QuestHubDatabase: Skipping cluster {} with only {} quest givers (minimum {})",
                    clusterId, indices.size(), MIN_POINTS);
                continue;
            }

            QuestHub hub;
            hub.hubId = hubId++;

            TC_LOG_DEBUG("playerbot", "QuestHubDatabase: Creating hub {} from {} quest givers (cluster ID: {})",
                hub.hubId, indices.size(), clusterId);

            // Calculate center position (average of all quest giver positions)
            float sumX = 0.0f, sumY = 0.0f, sumZ = 0.0f;
            ::std::set<uint32> uniqueCreatures;
            uint32 mapId = 0;
            uint32 zoneId = 0;

            for (size_t idx : indices)
            {
                QuestGiverData const& qg = _tempQuestGivers[idx];
                sumX += qg.position.GetPositionX();
                sumY += qg.position.GetPositionY();
                sumZ += qg.position.GetPositionZ();
                uniqueCreatures.insert(qg.creatureEntry);

                // All quest givers in a cluster are on the same map (enforced by findNeighbors)
                if (mapId == 0)
                    mapId = qg.mapId;
                if (zoneId == 0)
                    zoneId = qg.zoneId;
            }

            hub.location.Relocate(
                sumX / indices.size(),
                sumY / indices.size(),
                sumZ / indices.size()
            );

            hub.mapId = mapId;
            hub.zoneId = zoneId;
            hub.creatureIds.assign(uniqueCreatures.begin(), uniqueCreatures.end());

            // Calculate radius (max distance from center)
            float maxDist = 0.0f;
            for (size_t idx : indices)
            {
                Position const& pos = _tempQuestGivers[idx].position;
                float dx = pos.GetPositionX() - hub.location.GetPositionX();
                float dy = pos.GetPositionY() - hub.location.GetPositionY();
                float dist = ::std::sqrt(dx * dx + dy * dy);
                maxDist = ::std::max(maxDist, dist);
            }
            hub.radius = maxDist + 10.0f; // Add 10 yard buffer

            // Default faction mask (will be refined with quest data)
            hub.factionMask = 0x07; // All factions initially

            // Generate name (will be refined with zone data)
            hub.name = "Quest Hub ";
            hub.name += ::std::to_string(hub.hubId);

            _questHubs.push_back(::std::move(hub));
        }

        return static_cast<uint32>(_questHubs.size());
    }

    void QuestHubDatabase::LoadQuestDataForHubs()
    {
        TC_LOG_INFO("playerbot", "QuestHubDatabase: Loading quest data for {} hubs...", _questHubs.size());

        // OPTIMIZATION: Collect ALL unique creature IDs from ALL hubs in a single pass
        ::std::set<uint32> allCreatureIds;
        for (auto const& hub : _questHubs)
        {
            allCreatureIds.insert(hub.creatureIds.begin(), hub.creatureIds.end());
        }

        TC_LOG_INFO("playerbot", "QuestHubDatabase: Querying quest data for {} unique creatures across all hubs...",
            allCreatureIds.size());

        // Build map of creature ID -> quest data
        // Tuple format: (questId, contentTuningId, unused, allowableRaces)
        ::std::unordered_map<uint32, ::std::vector<::std::tuple<uint32, uint32, uint32, uint64>>> creatureQuests;

        // BATCH QUERIES: Split large IN clause into chunks of 100 creatures to avoid crashes
        constexpr size_t BATCH_SIZE = 100;
        ::std::vector<uint32> creatureIdVec(allCreatureIds.begin(), allCreatureIds.end());

        for (size_t batchStart = 0; batchStart < creatureIdVec.size(); batchStart += BATCH_SIZE)
        {
            size_t batchEnd = ::std::min(batchStart + BATCH_SIZE, creatureIdVec.size());
            size_t batchCount = batchEnd - batchStart;

            // Build IN clause for this batch
            ::std::string creatureList;
            size_t batchSize = batchEnd - batchStart;
            creatureList.reserve(batchSize * 8); // Pre-allocate approximate size
    for (size_t i = batchStart; i < batchEnd; ++i)
            {
                if (i != batchStart)
                    creatureList += ",";
                creatureList += ::std::to_string(creatureIdVec[i]);
            }

            // Execute query for this batch
            // WoW 12.0: Level info is in DB2 via ContentTuningID, not in quest_template columns
            ::std::string query = "SELECT DISTINCT qr.id, qr.quest, qt.ContentTuningID, qt.AllowableRaces "
                               "FROM creature_queststarter qr "
                               "INNER JOIN quest_template qt ON qr.quest = qt.ID "
                               "WHERE qr.id IN (" + creatureList + ")";

            QueryResult result = WorldDatabase.Query(query.c_str());

            if (!result)
            {
                TC_LOG_DEBUG("playerbot", "QuestHubDatabase: No quest data found for batch {} ({} creatures)",
                    (batchStart / BATCH_SIZE) + 1, batchCount);
                continue;
            }

            // Process results from this batch
            do
            {
                Field* fields = result->Fetch();

                uint32 creatureId = fields[0].GetUInt32();
                uint32 questId = fields[1].GetUInt32();
                uint32 contentTuningId = fields[2].GetUInt32();
                uint64 allowableRaces = fields[3].GetUInt64();

                // Store ContentTuningID instead of min/max level (will use TrinityCore APIs later)
                creatureQuests[creatureId].emplace_back(questId, contentTuningId, 0, allowableRaces);

            } while (result->NextRow());

            TC_LOG_DEBUG("playerbot", "QuestHubDatabase: Processed batch {} of {} ({} creatures)",
                (batchStart / BATCH_SIZE) + 1, (creatureIdVec.size() + BATCH_SIZE - 1) / BATCH_SIZE, batchCount);
        }

        TC_LOG_INFO("playerbot", "QuestHubDatabase: Retrieved quest data for {} creatures, now populating hubs...",
            creatureQuests.size());

        // Now populate each hub with its quest data (in-memory lookup, very fast!)
        uint32 hubsProcessed = 0;
        for (auto& hub : _questHubs)
        {
            if (hub.creatureIds.empty())
                continue;

            ::std::set<uint32> uniqueQuests;

            // Lookup quest data for each creature in this hub (fast in-memory operation)
    for (uint32 creatureId : hub.creatureIds)
            {
                auto it = creatureQuests.find(creatureId);
                if (it == creatureQuests.end())
                    continue;

                // Tuple format: (questId, contentTuningId, unused, allowableRaces)
    for (auto const& [questId, contentTuningId, unused, allowableRaces] : it->second)
                {
                    uniqueQuests.insert(questId);

                    // Update faction mask based on allowable races
    if (allowableRaces == 0 || allowableRaces == 0xFFFFFFFFFFFFFFFF)
                    {
                        hub.factionMask |= 0x07; // All factions
                    }
                    else
                    {
                        // Simplified faction detection
                        bool hasAlliance = (allowableRaces & 0x0000044D) != 0; // Alliance race bits
                        bool hasHorde = (allowableRaces & 0x000002B2) != 0;    // Horde race bits
    if (hasAlliance)
                            hub.factionMask |= 0x01;
                        if (hasHorde)
                            hub.factionMask |= 0x02;
                    }
                }
            }

            // Assign quest data to hub
            hub.questIds.assign(uniqueQuests.begin(), uniqueQuests.end());

            // WoW 12.0: Level ranges are determined dynamically via ContentTuningID in DB2
            // For initial hub classification, use zone-based approximations
            // This will be refined at runtime when bots query for appropriate hubs
            hub.minLevel = 1;
            hub.maxLevel = 70; // Max level in WoW 12.0

            // Refine hub name with zone info if available
    if (hub.zoneId > 0)
            {
                hub.name = "Quest Hub (Zone " + ::std::to_string(hub.zoneId) + ")";
            }

            ++hubsProcessed;
        }

        TC_LOG_INFO("playerbot", "QuestHubDatabase: Completed loading quest data for {} hubs", hubsProcessed);
    }

    void QuestHubDatabase::BuildSpatialIndex()
    {
        _hubIdToIndex.clear();
        _zoneIndex.clear();

        for (size_t i = 0; i < _questHubs.size(); ++i)
        {
            QuestHub const& hub = _questHubs[i];

            // Build hub ID index
            _hubIdToIndex[hub.hubId] = i;

            // Build zone index
            _zoneIndex[hub.zoneId].push_back(i);
        }
    }

    void QuestHubDatabase::ValidateHubData()
    {
        uint32 warnings = 0;

        for (auto const& hub : _questHubs)
        {
            // Check for empty hubs
    if (hub.questIds.empty())
            {
                TC_LOG_WARN("playerbot", "QuestHubDatabase: Hub {} has no quests", hub.hubId);
                ++warnings;
            }

            // Check for invalid positions
    if (hub.location.GetPositionX() == 0.0f &&
                hub.location.GetPositionY() == 0.0f &&
                hub.location.GetPositionZ() == 0.0f)
            {
                TC_LOG_WARN("playerbot", "QuestHubDatabase: Hub {} has invalid position (0,0,0)", hub.hubId);
                ++warnings;
            }

            // Check for invalid level ranges
    if (hub.minLevel > hub.maxLevel && hub.maxLevel > 0)
            {
                TC_LOG_WARN("playerbot", "QuestHubDatabase: Hub {} has invalid level range ({}-{})",
                           hub.hubId, hub.minLevel, hub.maxLevel);
                ++warnings;
            }

            // Check for no faction access
    if (hub.factionMask == 0)
            {
                TC_LOG_WARN("playerbot", "QuestHubDatabase: Hub {} has no faction access", hub.hubId);
                ++warnings;
            }
        }

        if (warnings > 0)
        {
            TC_LOG_WARN("playerbot", "QuestHubDatabase: Validation found {} warnings", warnings);
        }
        else
        {
            TC_LOG_DEBUG("playerbot", "QuestHubDatabase: Validation passed with no warnings");
        }
    }

} // namespace Playerbot
