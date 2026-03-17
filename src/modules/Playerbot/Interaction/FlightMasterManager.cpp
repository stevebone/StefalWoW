/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "FlightMasterManager.h"
#include "Player.h"
#include "Creature.h"
#include "ObjectMgr.h"
#include "DB2Stores.h"
#include "DB2Structure.h"
#include "TaxiPathGraph.h"
#include "Log.h"
#include "World.h"
#include "WorldSession.h"
#include "GameTime.h"
#include "Map.h"
#include <cmath>
#include <algorithm>

namespace Playerbot
{
    // Flight speed constants (yards per second)
    static constexpr float FLIGHT_SPEED_NORMAL = 32.0f;       // Normal flight speed
    static constexpr float FLIGHT_SPEED_FAST = 50.0f;         // Fast flight speed (epic flying)

    // Cost calculation constants
    static constexpr uint32 FLIGHT_COST_BASE = 100;           // Base cost in copper
    static constexpr float FLIGHT_COST_PER_YARD = 0.1f;       // Cost per yard traveled

    // Major city taxi node IDs (examples - actual values from DB)
    static constexpr uint32 STORMWIND_NODE = 2;
    static constexpr uint32 IRONFORGE_NODE = 6;
    static constexpr uint32 ORGRIMMAR_NODE = 23;
    static constexpr uint32 THUNDERBLUFF_NODE = 22;
    static constexpr uint32 UNDERCITY_NODE = 11;

    FlightMasterManager::FlightMasterManager(Player* bot)
        : m_bot(bot)
        , m_cpuUsage(0.0f)
        , m_totalFlightTime(0)
        , m_flightDecisionCount(0)
        , m_lastCacheUpdate(0)
    {
        if (m_bot)
        {
            // Initialize known paths cache
            UpdateKnownPathsCache();
        }
    }

    // Core Flight Methods

    bool FlightMasterManager::LearnFlightPath(Creature* flightMaster)
    {
        if (!flightMaster || !m_bot)
            return false;

        m_stats.flightAttempts++;

        // Get taxi node at flight master's location
        uint32 nodeId = GetCurrentTaxiNode(flightMaster);
        if (nodeId == 0)
        {
            TC_LOG_DEBUG("bot.playerbot", "FlightMasterManager: No taxi node found at flight master %u location",
                flightMaster->GetEntry());
            m_stats.flightFailures++;
            return false;
        }
        // Check if already known
    if (m_bot->m_taxi.IsTaximaskNodeKnown(nodeId))
        {
            TC_LOG_DEBUG("bot.playerbot", "FlightMasterManager: Bot %s already knows taxi node %u",
                m_bot->GetName().c_str(), nodeId);
            return false;
        }

        // Learn the taxi node
    if (m_bot->m_taxi.SetTaximaskNode(nodeId))
        {
            RecordPathLearned(nodeId);

            TC_LOG_DEBUG("bot.playerbot", "FlightMasterManager: Bot %s learned new taxi node %u",
                m_bot->GetName().c_str(), nodeId);

            // Update cache
            m_knownPathsCache.insert(nodeId);

            return true;
        }

        return false;
    }

    bool FlightMasterManager::FlyToDestination(Creature* flightMaster, uint32 destinationNodeId)
    {
        if (!flightMaster || !m_bot || destinationNodeId == 0)
            return false;

        m_stats.flightAttempts++;

        // Get current taxi node
        uint32 currentNodeId = GetCurrentTaxiNode(flightMaster);
        if (currentNodeId == 0)
        {
            TC_LOG_DEBUG("bot.playerbot", "FlightMasterManager: No taxi node at current location");
            m_stats.flightFailures++;
            return false;
        }

        // Can't fly to same node
    if (currentNodeId == destinationNodeId)
        {
            TC_LOG_DEBUG("bot.playerbot", "FlightMasterManager: Already at destination node %u", destinationNodeId);
            m_stats.flightFailures++;
            return false;
        }

        // Get taxi node entries
        TaxiNodesEntry const* fromNode = GetTaxiNode(currentNodeId);
        TaxiNodesEntry const* toNode = GetTaxiNode(destinationNodeId);

        if (!fromNode || !toNode)
        {
            TC_LOG_DEBUG("bot.playerbot", "FlightMasterManager: Invalid taxi nodes (from: %u, to: %u)",
                currentNodeId, destinationNodeId);
            m_stats.flightFailures++;
            return false;
        }

        // Check if both nodes are known
    if (!m_bot->m_taxi.IsTaximaskNodeKnown(currentNodeId))
        {
            TC_LOG_DEBUG("bot.playerbot", "FlightMasterManager: Current node %u not known", currentNodeId);
            m_stats.pathNotKnown++;
            m_stats.flightFailures++;
            return false;
        }

        if (!m_bot->m_taxi.IsTaximaskNodeKnown(destinationNodeId))
        {
            TC_LOG_DEBUG("bot.playerbot", "FlightMasterManager: Destination node %u not known", destinationNodeId);
            m_stats.pathNotKnown++;
            m_stats.flightFailures++;
            return false;
        }

        // Calculate route
        ::std::vector<uint32> route;
        if (!CalculateRoute(fromNode, toNode, route))
        {
            TC_LOG_DEBUG("bot.playerbot", "FlightMasterManager: No route found from %u to %u",
                currentNodeId, destinationNodeId);
            m_stats.flightFailures++;
            return false;
        }

        // Calculate cost
        uint32 cost = CalculateFlightCost(fromNode, toNode);

        // Check if bot can afford it
    if (!CanAffordFlight(cost))
        {
            TC_LOG_DEBUG("bot.playerbot", "FlightMasterManager: Bot %s cannot afford flight cost %u copper",
                m_bot->GetName().c_str(), cost);
            m_stats.insufficientGold++;
            m_stats.flightFailures++;
            return false;
        }

        // Execute flight
        bool success = ExecuteFlight(route, flightMaster);
        RecordFlight(cost, success);

        return success;
    }

    bool FlightMasterManager::SmartFlight(Creature* flightMaster)
    {
        if (!flightMaster || !m_bot)
            return false;

        // Get current taxi node
        uint32 currentNodeId = GetCurrentTaxiNode(flightMaster);
        if (currentNodeId == 0)
            return false;

        TaxiNodesEntry const* fromNode = GetTaxiNode(currentNodeId);
        if (!fromNode)
            return false;

        // Get all reachable destinations
        ::std::vector<FlightDestination> destinations = GetReachableDestinations(flightMaster);
        if (destinations.empty())
        {
            TC_LOG_DEBUG("bot.playerbot", "FlightMasterManager: No reachable destinations from node %u", currentNodeId);
            return false;
        }

        // Evaluate each destination
        ::std::vector<FlightPathEvaluation> evaluations;
        for (auto const& dest : destinations)
        {
            if (!dest.isKnown || dest.nodeId == currentNodeId)
                continue;

            TaxiNodesEntry const* toNode = GetTaxiNode(dest.nodeId);
            if (!toNode)
                continue;

            FlightPathEvaluation eval = EvaluateDestination(fromNode, toNode);
            evaluations.push_back(eval);
        }
        if (evaluations.empty())
        {
            TC_LOG_DEBUG("bot.playerbot", "FlightMasterManager: No valid destinations to evaluate");
            return false;
        }

        // Sort by priority (lowest value = highest priority)
        ::std::sort(evaluations.begin(), evaluations.end(),
            [](FlightPathEvaluation const& a, FlightPathEvaluation const& b)
            {
                if (a.priority != b.priority)
                    return a.priority < b.priority;
                // Same priority - prefer closer destinations
                return a.distance < b.distance;
            });

        // Try to fly to best destination
        FlightPathEvaluation const& best = evaluations.front();

        TC_LOG_DEBUG("bot.playerbot", "FlightMasterManager: Bot %s selecting flight to node %u (priority: %u, reason: %s)",
            m_bot->GetName().c_str(), best.nodeId, static_cast<uint32>(best.priority), best.reason.c_str());

        return FlyToDestination(flightMaster, best.nodeId);
    }

    uint32 FlightMasterManager::GetCurrentTaxiNode(Creature* flightMaster) const
    {
        if (!flightMaster || !m_bot)
            return 0;

        // Use TrinityCore's ObjectMgr to find nearest taxi node
        uint32 nodeId = sObjectMgr->GetNearestTaxiNode(
            flightMaster->GetPositionX(),
            flightMaster->GetPositionY(),
            flightMaster->GetPositionZ(),
            flightMaster->GetMapId(),
            m_bot->GetTeam()
        );

        return nodeId;
    }

    bool FlightMasterManager::IsFlightPathKnown(uint32 nodeId) const
    {
        if (!m_bot)
            return false;

        return m_bot->m_taxi.IsTaximaskNodeKnown(nodeId);
    }

    ::std::vector<uint32> FlightMasterManager::GetKnownFlightPaths() const
    {
        ::std::vector<uint32> knownPaths;

        if (!m_bot)
            return knownPaths;

        // Iterate through all taxi nodes
    for (TaxiNodesEntry const* node : sTaxiNodesStore)
        {
            if (node && m_bot->m_taxi.IsTaximaskNodeKnown(node->ID))
                knownPaths.push_back(node->ID);
        }

        return knownPaths;
    }

    ::std::vector<FlightMasterManager::FlightDestination> FlightMasterManager::GetReachableDestinations(Creature* flightMaster) const
    {
        ::std::vector<FlightDestination> destinations;

        if (!flightMaster || !m_bot)
            return destinations;

        uint32 currentNodeId = GetCurrentTaxiNode(flightMaster);
        if (currentNodeId == 0)
            return destinations;

        TaxiNodesEntry const* currentNode = GetTaxiNode(currentNodeId);
        if (!currentNode)
            return destinations;

        // Get reachable nodes mask
        TaxiMask reachableNodes;
        TaxiPathGraph::GetReachableNodesMask(currentNode, &reachableNodes);

        // Build destination list
        for (TaxiNodesEntry const* node : sTaxiNodesStore)
        {
            if (!node || node->ID == currentNodeId)
                continue;

            // FACTION CHECK: Only include taxi nodes visible to bot's faction
            // This prevents Horde bots from seeing Alliance-only flight points and vice versa
            bool isVisibleForFaction = false;
            switch (m_bot->GetTeam())
            {
                case HORDE:
                    isVisibleForFaction = node->GetFlags().HasFlag(TaxiNodeFlags::ShowOnHordeMap);
                    break;
                case ALLIANCE:
                    isVisibleForFaction = node->GetFlags().HasFlag(TaxiNodeFlags::ShowOnAllianceMap);
                    break;
                default:
                    break;
            }
            if (!isVisibleForFaction)
                continue;

            // Check if reachable
            uint32 field = uint32((node->ID - 1) / (sizeof(TaxiMask::value_type) * 8));
            TaxiMask::value_type submask = TaxiMask::value_type(1 << ((node->ID - 1) % (sizeof(TaxiMask::value_type) * 8)));
            bool isReachable = (reachableNodes[field] & submask) != 0;

            if (!isReachable)
                continue;

            FlightDestination dest;
            dest.nodeId = node->ID;
            dest.name = node->Name[static_cast<LocaleConstant>(0)];  // English name (LOCALE_enUS)
            dest.x = node->Pos.X;
            dest.y = node->Pos.Y;
            dest.z = node->Pos.Z;
            dest.mapId = node->ContinentID;
            dest.continentId = node->ContinentID;
            dest.isKnown = m_bot->m_taxi.IsTaximaskNodeKnown(node->ID);
            dest.isReachable = true;

            destinations.push_back(dest);
        }

        return destinations;
    }

    // Flight Evaluation Methods

    FlightMasterManager::FlightPathEvaluation FlightMasterManager::EvaluateDestination(
        TaxiNodesEntry const* from, TaxiNodesEntry const* to) const
    {
        FlightPathEvaluation eval;

        if (!from || !to || !m_bot)
            return eval;

        eval.nodeId = to->ID;
        eval.isKnown = m_bot->m_taxi.IsTaximaskNodeKnown(to->ID);

        // Calculate route
        CalculateRoute(from, to, eval.route);

        // Calculate distance
        eval.distance = CalculateDistance(from, to);

        // Calculate cost and time
        eval.estimatedCost = CalculateFlightCost(from, to);
        eval.estimatedTime = CalculateFlightTime(eval.distance);

        // Determine priority
        eval.priority = CalculateDestinationPriority(to->ID, to);

        // Generate reason
    switch (eval.priority)
        {
            case DestinationPriority::QUEST_OBJECTIVE:
                eval.reason = "Near quest objective location";
                break;
            case DestinationPriority::TRAINER_VENDOR:
                eval.reason = "Major city with trainers/vendors";
                break;
            case DestinationPriority::LEVELING_ZONE:
                eval.reason = "Appropriate leveling zone for current level";
                break;
            case DestinationPriority::EXPLORATION:
                eval.reason = "Exploration and discovery";
                break;
            default:
                eval.reason = "Unknown";
                break;
        }

        return eval;
    }

    FlightMasterManager::DestinationPriority FlightMasterManager::CalculateDestinationPriority(
        uint32 nodeId, TaxiNodesEntry const* nodeEntry) const
    {
        if (!nodeEntry || !m_bot)
            return DestinationPriority::EXPLORATION;

        // Check cache first
        auto it = m_priorityCache.find(nodeId);
        if (it != m_priorityCache.end())
            return it->second;

        DestinationPriority priority = DestinationPriority::EXPLORATION;

        // Check if near quest objectives (highest priority)
    if (IsNearQuestObjectives(nodeEntry))
        {
            priority = DestinationPriority::QUEST_OBJECTIVE;
        }
        // Check if major city (training/vendors)
        else if (IsMajorCity(nodeEntry))
        {
            priority = DestinationPriority::TRAINER_VENDOR;
        }
        // Check if appropriate leveling zone
        else if (IsAppropriateForLevel(nodeEntry))
        {
            priority = DestinationPriority::LEVELING_ZONE;
        }

        return priority;
    }

    uint32 FlightMasterManager::CalculateFlightCost(TaxiNodesEntry const* from, TaxiNodesEntry const* to) const
    {
        if (!from || !to)
            return 0;
        // Calculate distance-based cost
        float distance = CalculateDistance(from, to);
        uint32 cost = FLIGHT_COST_BASE + static_cast<uint32>(distance * FLIGHT_COST_PER_YARD);

        // Apply level-based discount (higher level = slight discount)
    if (m_bot)
        {
            uint32 level = m_bot->GetLevel();
            if (level >= 60)
                cost = static_cast<uint32>(cost * 0.8f);  // 20% discount at max level
            else if (level >= 40)
                cost = static_cast<uint32>(cost * 0.9f);  // 10% discount at mid level
        }

        return cost;
    }

    uint32 FlightMasterManager::CalculateFlightTime(float distance) const
    {
        if (distance <= 0.0f)
            return 0;

        // Use fast flight speed if bot has epic flying
        float speed = FLIGHT_SPEED_NORMAL;
        if (m_bot && m_bot->GetLevel() >= 60)
            speed = FLIGHT_SPEED_FAST;

        return static_cast<uint32>(distance / speed);
    }

    bool FlightMasterManager::CanAffordFlight(uint32 cost) const
    {
        if (!m_bot)
            return false;

        return m_bot->GetMoney() >= cost;
    }

    // Goal-Based Flight Selection

    uint32 FlightMasterManager::FindNearestFlightMasterToPosition(float targetX, float targetY, float targetZ, uint32 mapId) const
    {
        if (!m_bot)
            return 0;

        uint32 nearestNode = 0;
        float minDistance = ::std::numeric_limits<float>::max();

        for (TaxiNodesEntry const* node : sTaxiNodesStore)
        {
            if (!node || node->ContinentID != mapId)
                continue;

            // FACTION CHECK: Only consider taxi nodes visible to bot's faction
            // This prevents Horde bots from selecting Alliance flight points and vice versa
            bool isVisibleForFaction = false;
            switch (m_bot->GetTeam())
            {
                case HORDE:
                    isVisibleForFaction = node->GetFlags().HasFlag(TaxiNodeFlags::ShowOnHordeMap);
                    break;
                case ALLIANCE:
                    isVisibleForFaction = node->GetFlags().HasFlag(TaxiNodeFlags::ShowOnAllianceMap);
                    break;
                default:
                    break;
            }
            if (!isVisibleForFaction)
                continue;

            // Calculate distance to target
            float dx = node->Pos.X - targetX;
            float dy = node->Pos.Y - targetY;
            float dz = node->Pos.Z - targetZ;
            float dist = ::std::sqrt(dx * dx + dy * dy + dz * dz);

            if (dist < minDistance)
            {
                minDistance = dist;
                nearestNode = node->ID;
            }
        }

        return nearestNode;
    }

    uint32 FlightMasterManager::GetFlightDestinationForQuest(uint32 questId) const
    {
        if (!m_bot || questId == 0)
            return 0;

        // This would require quest objective location lookup
        // Simplified implementation - would need QuestPOI or quest template data
        // For now, return 0 (not implemented)
        return 0;
    }

    uint32 FlightMasterManager::GetFlightDestinationForTraining() const
    {
        if (!m_bot)
            return 0;

        // Return capital city based on faction
    if (m_bot->GetTeam() == ALLIANCE)
        {
            // Alliance - prefer Stormwind or Ironforge
    if (IsFlightPathKnown(STORMWIND_NODE))
                return STORMWIND_NODE;
            if (IsFlightPathKnown(IRONFORGE_NODE))
                return IRONFORGE_NODE;
        }
        else
        {
            // Horde - prefer Orgrimmar or Undercity
    if (IsFlightPathKnown(ORGRIMMAR_NODE))
                return ORGRIMMAR_NODE;
            if (IsFlightPathKnown(UNDERCITY_NODE))
                return UNDERCITY_NODE;
        }

        return 0;
    }

    uint32 FlightMasterManager::GetFlightDestinationForLeveling() const
    {
        if (!m_bot)
            return 0;

        return GetRecommendedLevelingZone();
    }
    // Internal Helper Methods

    TaxiNodesEntry const* FlightMasterManager::GetTaxiNode(uint32 nodeId) const
    {
        return sTaxiNodesStore.LookupEntry(nodeId);
    }

    bool FlightMasterManager::CalculateRoute(TaxiNodesEntry const* from, TaxiNodesEntry const* to, ::std::vector<uint32>& route) const
    {
        if (!from || !to || !m_bot)
            return false;

        route.clear();

        // Use TrinityCore's TaxiPathGraph to calculate complete route
        TaxiPathGraph::GetCompleteNodeRoute(from, to, m_bot, route);

        return !route.empty();
    }

    float FlightMasterManager::CalculateDistance(TaxiNodesEntry const* from, TaxiNodesEntry const* to) const
    {
        if (!from || !to)
            return 0.0f;

        float dx = to->Pos.X - from->Pos.X;
        float dy = to->Pos.Y - from->Pos.Y;
        float dz = to->Pos.Z - from->Pos.Z;

        return ::std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    bool FlightMasterManager::IsAppropriateForLevel(TaxiNodesEntry const* nodeEntry) const
    {
        if (!nodeEntry || !m_bot)
            return false;

        // This would require zone level range data
        // Simplified - assume nodes in same continent as player are appropriate
        return nodeEntry->ContinentID == m_bot->GetMapId();
    }

    bool FlightMasterManager::IsNearQuestObjectives(TaxiNodesEntry const* nodeEntry) const
    {
        if (!nodeEntry || !m_bot)
            return false;

        // This would require active quest objective location comparison
        // Simplified implementation - return false for now
        return false;
    }

    bool FlightMasterManager::IsMajorCity(TaxiNodesEntry const* nodeEntry) const
    {
        if (!nodeEntry)
            return false;

        // Check if node is a known major city
        return nodeEntry->ID == STORMWIND_NODE ||
               nodeEntry->ID == IRONFORGE_NODE ||
               nodeEntry->ID == ORGRIMMAR_NODE ||
               nodeEntry->ID == THUNDERBLUFF_NODE ||
               nodeEntry->ID == UNDERCITY_NODE;
    }

    uint32 FlightMasterManager::GetRecommendedLevelingZone() const
    {
        if (!m_bot)
            return 0;

        uint32 level = m_bot->GetLevel();
        // This would require zone level range lookup
        // Simplified - return capital city for now
        return GetFlightDestinationForTraining();
    }

    bool FlightMasterManager::ExecuteFlight(::std::vector<uint32> const& route, Creature* flightMaster)
    {
        if (route.empty() || !flightMaster || !m_bot)
            return false;

        // Use TrinityCore's Player::ActivateTaxiPathTo
        bool success = m_bot->ActivateTaxiPathTo(route, flightMaster);
        if (success)
        {
            TC_LOG_DEBUG("bot.playerbot", "FlightMasterManager: Bot %s successfully started flight with %u nodes",
                m_bot->GetName().c_str(), static_cast<uint32>(route.size()));
        }
        else
        {
            TC_LOG_DEBUG("bot.playerbot", "FlightMasterManager: Bot %s failed to start flight",
                m_bot->GetName().c_str());
        }

        return success;
    }

    void FlightMasterManager::RecordFlight(uint32 cost, bool success)
    {
        if (success)
        {
            m_stats.flightsTaken++;
            m_stats.totalGoldSpent += cost;
        }
        else
        {
            m_stats.flightFailures++;
        }
    }

    void FlightMasterManager::RecordPathLearned(uint32 nodeId)
    {
        m_stats.flightPathsLearned++;

        TC_LOG_DEBUG("bot.playerbot", "FlightMasterManager: Recorded path learned for node %u (total: %u)",
            nodeId, m_stats.flightPathsLearned);
    }

    void FlightMasterManager::UpdateKnownPathsCache()
    {
        m_knownPathsCache.clear();

        if (!m_bot)
            return;

        for (TaxiNodesEntry const* node : sTaxiNodesStore)
        {
            if (node && m_bot->m_taxi.IsTaximaskNodeKnown(node->ID))
                m_knownPathsCache.insert(node->ID);
        }

        m_lastCacheUpdate = GameTime::GetGameTimeMS();
    }

    size_t FlightMasterManager::GetMemoryUsage() const
    {
        size_t memory = sizeof(*this);
        memory += m_priorityCache.size() * (sizeof(uint32) + sizeof(DestinationPriority));
        memory += m_knownPathsCache.size() * sizeof(uint32);
        return memory;
    }

    // ========================================================================
    // STATIC UTILITY METHODS IMPLEMENTATION
    // ========================================================================

    uint32 FlightMasterManager::FindNearestTaxiNode(Position const& pos, uint32 mapId, Player* player)
    {
        if (!player)
            return 0;

        // Use TrinityCore's sObjectMgr->GetNearestTaxiNode with faction awareness
        uint32 nodeId = sObjectMgr->GetNearestTaxiNode(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), mapId, player->GetTeam());

        return nodeId;
    }

    ::std::optional<FlightMasterLocation> FlightMasterManager::FindNearestFlightMaster(Player* player)
    {
        if (!player)
            return ::std::nullopt;

        Map* map = player->GetMap();
        if (!map)
            return ::std::nullopt;

        // Search for nearest flight master NPC
        float searchRadius = 1000.0f;
        Creature* nearestFM = nullptr;
        float nearestDist = searchRadius;

        // Search for creatures with UNIT_NPC_FLAG_FLIGHTMASTER
        // Note: This is a simplified approach - in production you'd use Grid::Visitor
        auto const& creatures = map->GetCreatureBySpawnIdStore();
        for (auto const& pair : creatures)
        {
            Creature* creature = pair.second;
            if (!creature || !creature->IsAlive())
                continue;

            if (!creature->HasNpcFlag(UNIT_NPC_FLAG_FLIGHTMASTER))
                continue;

            float dist = player->GetDistance(creature);
            if (dist < nearestDist)
            {
                nearestDist = dist;
                nearestFM = creature;
            }
        }

        if (!nearestFM)
            return ::std::nullopt;

        FlightMasterLocation loc;
        loc.name = nearestFM->GetName();
        loc.guid = nearestFM->GetGUID();
        loc.distanceFromPlayer = nearestDist;
        loc.position = Position(nearestFM->GetPositionX(), nearestFM->GetPositionY(), nearestFM->GetPositionZ());

        // Get the taxi node at this flight master's location
        loc.taxiNode = sObjectMgr->GetNearestTaxiNode(
            nearestFM->GetPositionX(), nearestFM->GetPositionY(), nearestFM->GetPositionZ(),
            nearestFM->GetMapId(), player->GetTeam());

        return loc;
    }

    bool FlightMasterManager::HasTaxiNode(Player* player, uint32 nodeId)
    {
        if (!player || nodeId == 0)
            return false;

        return player->m_taxi.IsTaximaskNodeKnown(nodeId);
    }

    bool FlightMasterManager::HasValidFlightPath(uint32 startNode, uint32 endNode, Player* player)
    {
        if (!player || startNode == 0 || endNode == 0)
            return false;

        // Same node - no path needed
        if (startNode == endNode)
            return false;

        // Get node entries
        TaxiNodesEntry const* startEntry = sTaxiNodesStore.LookupEntry(startNode);
        TaxiNodesEntry const* endEntry = sTaxiNodesStore.LookupEntry(endNode);

        if (!startEntry || !endEntry)
            return false;

        // Check if nodes are on same continent (ContinentID field)
        if (startEntry->ContinentID != endEntry->ContinentID)
            return false;

        // Check if player knows both nodes
        if (!player->m_taxi.IsTaximaskNodeKnown(startNode) ||
            !player->m_taxi.IsTaximaskNodeKnown(endNode))
            return false;

        // Use TaxiPathGraph to check if a route exists
        ::std::vector<uint32> route;
        uint32 cost = 0;
        if (!TaxiPathGraph::GetCompleteNodeRoute(startEntry, endEntry, player, route))
            return false;

        return !route.empty();
    }

    ::std::optional<FlightPathInfo> FlightMasterManager::CalculateFlightPath(
        Player* player, uint32 startNode, uint32 endNode, FlightPathStrategy /*strategy*/)
    {
        if (!player || startNode == 0 || endNode == 0)
            return ::std::nullopt;

        TaxiNodesEntry const* startEntry = sTaxiNodesStore.LookupEntry(startNode);
        TaxiNodesEntry const* endEntry = sTaxiNodesStore.LookupEntry(endNode);

        if (!startEntry || !endEntry)
            return ::std::nullopt;

        // Calculate route using TaxiPathGraph
        ::std::vector<uint32> route;
        if (!TaxiPathGraph::GetCompleteNodeRoute(startEntry, endEntry, player, route))
            return ::std::nullopt;

        if (route.empty())
            return ::std::nullopt;

        FlightPathInfo info;
        info.nodes = route;
        info.stopCount = static_cast<uint32>(route.size());
        info.crossesContinent = (startEntry->ContinentID != endEntry->ContinentID);

        // Calculate cost and time
        float totalDistance = 0.0f;
        for (size_t i = 0; i < route.size() - 1; ++i)
        {
            TaxiNodesEntry const* node1 = sTaxiNodesStore.LookupEntry(route[i]);
            TaxiNodesEntry const* node2 = sTaxiNodesStore.LookupEntry(route[i + 1]);
            if (node1 && node2)
            {
                float dx = node2->Pos.X - node1->Pos.X;
                float dy = node2->Pos.Y - node1->Pos.Y;
                float dz = node2->Pos.Z - node1->Pos.Z;
                totalDistance += std::sqrt(dx * dx + dy * dy + dz * dz);
            }
        }

        info.goldCost = static_cast<uint32>(FLIGHT_COST_BASE + totalDistance * FLIGHT_COST_PER_YARD);
        info.flightTime = static_cast<uint32>(totalDistance / FLIGHT_SPEED_NORMAL);

        return info;
    }

    FlightResult FlightMasterManager::FlyToTaxiNode(Player* player, uint32 destinationNode, FlightPathStrategy /*strategy*/)
    {
        if (!player)
            return FlightResult::INTERNAL_ERROR;

        if (!m_bot)
            m_bot = player;

        // Check if already flying
        if (player->IsInFlight())
            return FlightResult::ALREADY_FLYING;

        // Check destination node validity
        TaxiNodesEntry const* destEntry = sTaxiNodesStore.LookupEntry(destinationNode);
        if (!destEntry)
            return FlightResult::INVALID_NODE;

        // Check if player knows destination
        if (!player->m_taxi.IsTaximaskNodeKnown(destinationNode))
            return FlightResult::NODE_UNKNOWN;

        // Get current taxi node (player must be at a flight master)
        uint32 currentNode = sObjectMgr->GetNearestTaxiNode(
            player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(),
            player->GetMapId(), player->GetTeam());

        if (currentNode == 0)
            return FlightResult::NOT_AT_NODE;

        TaxiNodesEntry const* currentEntry = sTaxiNodesStore.LookupEntry(currentNode);
        if (!currentEntry)
            return FlightResult::INTERNAL_ERROR;

        // Calculate route
        ::std::vector<uint32> route;
        if (!TaxiPathGraph::GetCompleteNodeRoute(currentEntry, destEntry, player, route))
            return FlightResult::PATH_NOT_FOUND;

        if (route.empty())
            return FlightResult::PATH_NOT_FOUND;

        // Calculate cost
        uint32 cost = 0;
        for (size_t i = 0; i < route.size() - 1; ++i)
        {
            uint32 path, pathCost;
            sObjectMgr->GetTaxiPath(route[i], route[i + 1], path, pathCost);
            cost += pathCost;
        }

        // Check if player can afford
        if (!player->HasEnoughMoney(static_cast<uint64>(cost)))
            return FlightResult::INSUFFICIENT_GOLD;

        // Initiate flight
        player->ModifyMoney(-static_cast<int64>(cost));
        player->ActivateTaxiPathTo(route);

        // Update statistics
        m_stats.flightsTaken++;
        m_stats.totalGoldSpent += cost;

        return FlightResult::SUCCESS;
    }

    ::std::string FlightMasterManager::GetResultString(FlightResult result)
    {
        switch (result)
        {
            case FlightResult::SUCCESS:           return "Flight initiated successfully";
            case FlightResult::ALREADY_FLYING:    return "Already in flight";
            case FlightResult::NO_FLIGHT_MASTER:  return "No flight master nearby";
            case FlightResult::NODE_UNKNOWN:      return "Destination node not discovered";
            case FlightResult::PATH_NOT_FOUND:    return "No valid path found";
            case FlightResult::INSUFFICIENT_GOLD: return "Insufficient gold";
            case FlightResult::INVALID_NODE:      return "Invalid taxi node";
            case FlightResult::CROSS_CONTINENT:   return "Cannot fly across continents";
            case FlightResult::NOT_AT_NODE:       return "Not at a taxi node";
            case FlightResult::INTERNAL_ERROR:    return "Internal error";
            default:                              return "Unknown error";
        }
    }

} // namespace Playerbot
