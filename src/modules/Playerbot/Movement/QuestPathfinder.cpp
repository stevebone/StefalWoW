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

#include "QuestPathfinder.h"
#include "Creature.h"
#include "Log.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "PathGenerator.h"
#include "Player.h"
#include "Quest/QuestHubDatabase.h"
#include "World.h"
#include <algorithm>
#include <cmath>
#include "GameTime.h"
#include "Core/PlayerBotHelpers.h"
#include "AI/BotAI.h"

namespace Playerbot
{
    // ============================================================================
    // QuestPathfinder Implementation
    // ============================================================================

    QuestPathfindingResult QuestPathfinder::FindAndNavigateToQuestHub(
        Player* player,
        QuestPathfindingOptions const& options,
        QuestPathfindingState& state)
    {
        // Reset state
        state.Reset();
        state.lastUpdateTime = GameTime::GetGameTimeMS();

        // Validate player
    if (!player)
        {
            state.result = QuestPathfindingResult::PLAYER_INVALID;
            return state.result;
        }

        // Get quest hub database
        auto& hubDb = QuestHubDatabase::Instance();
        if (!hubDb.IsInitialized())
        {
            TC_LOG_ERROR("playerbot.pathfinding",
                "QuestPathfinder: QuestHubDatabase not initialized!");
            state.result = QuestPathfindingResult::NO_QUEST_HUBS_AVAILABLE;
            return state.result;
        }

        // Query appropriate quest hubs for player
        ::std::vector<QuestHub const*> hubs = hubDb.GetQuestHubsForPlayer(
            player,
            options.maxQuestHubCandidates);

        if (hubs.empty())
        {
            TC_LOG_DEBUG("playerbot.pathfinding",
                "QuestPathfinder: No quest hubs available for player {} (level {}, team {})",
                player->GetName(), player->GetLevel(), player->GetTeamId());
            state.result = QuestPathfindingResult::NO_QUEST_HUBS_AVAILABLE;
            return state.result;
        }

        // Select best quest hub based on strategy
        QuestHub const* selectedHub = SelectBestQuestHub(player, hubs, options.selectionStrategy);
        if (!selectedHub)
        {
            TC_LOG_ERROR("playerbot.pathfinding",
                "QuestPathfinder: SelectBestQuestHub returned nullptr (should never happen)");
            state.result = QuestPathfindingResult::NO_QUEST_HUBS_AVAILABLE;
            return state.result;
        }

        state.targetHubId = selectedHub->hubId;

        TC_LOG_DEBUG("playerbot.pathfinding",
            "QuestPathfinder: Selected quest hub {} ({}) for player {} - {} quests available",
            selectedHub->hubId, selectedHub->name, player->GetName(), selectedHub->questIds.size());

        // Find nearest quest giver in the selected hub
        Creature* questGiver = FindNearestQuestGiverInHub(player, selectedHub);
        if (!questGiver)
        {
            TC_LOG_WARN("playerbot.pathfinding",
                "QuestPathfinder: No quest givers found in world for hub {} ({})",
                selectedHub->hubId, selectedHub->name);

            // Fallback: Use hub center position as destination
            state.destination = selectedHub->location;
            state.targetCreatureEntry = 0;
            TC_LOG_DEBUG("playerbot.pathfinding",
                "QuestPathfinder: Using hub center as fallback destination");
        }
        else
        {
            state.targetCreatureGuid = questGiver->GetGUID();
            state.targetCreatureEntry = questGiver->GetEntry();
            state.destination = *questGiver;
            TC_LOG_DEBUG("playerbot.pathfinding",
                "QuestPathfinder: Found quest giver {} (entry {}) at distance {:.1f} yards",
                questGiver->GetName(), questGiver->GetEntry(),
                player->GetDistance2d(questGiver));
        }

        // Check if already at destination
    if (HasArrivedAtDestination(player, state, 5.0f))
        {
            TC_LOG_DEBUG("playerbot.pathfinding",
                "QuestPathfinder: Player {} already at destination (within 5 yards)",
                player->GetName());
            state.result = QuestPathfindingResult::ALREADY_AT_DESTINATION;
            return state.result;
        }

        // Validate destination
        QuestPathfindingResult validationResult = ValidateInput(player, state.destination);
        if (validationResult != QuestPathfindingResult::SUCCESS)
        {
            state.result = validationResult;
            return state.result;
        }

        // Generate path using PathGenerator
        QuestPathfindingResult pathResult = GeneratePath(
            player,
            state.destination,
            options,
            state.path,
            state.pathLength);

        if (pathResult != QuestPathfindingResult::SUCCESS)
        {
            state.result = pathResult;
            return state.result;
        }

        // Estimate travel time
        state.estimatedTravelTime = EstimateTravelTime(state.pathLength, player);
        TC_LOG_DEBUG("playerbot.pathfinding",
            "QuestPathfinder: Generated path for player {} - {:.1f} yards, {:.1f}s estimated travel time",
            player->GetName(), state.pathLength, state.estimatedTravelTime);

        // Initiate movement using MotionMaster
        QuestPathfindingResult movementResult = NavigateAlongPath(player, state);
        if (movementResult != QuestPathfindingResult::SUCCESS)
        {
            state.result = movementResult;
            return state.result;
        }

        state.result = QuestPathfindingResult::SUCCESS;
        return state.result;
    }

    QuestPathfindingResult QuestPathfinder::CalculatePathToQuestHub(
        Player* player,
        uint32 hubId,
        QuestPathfindingOptions const& options,
        QuestPathfindingState& state)
    {
        // Reset state
        state.Reset();
        state.lastUpdateTime = GameTime::GetGameTimeMS();
        state.targetHubId = hubId;

        // Validate player
    if (!player)
        {
            state.result = QuestPathfindingResult::PLAYER_INVALID;
            return state.result;
        }

        // Get quest hub from database
        auto& hubDb = QuestHubDatabase::Instance();
        if (!hubDb.IsInitialized())
        {
            TC_LOG_ERROR("playerbot.pathfinding",
                "QuestPathfinder: QuestHubDatabase not initialized!");
            state.result = QuestPathfindingResult::NO_QUEST_HUBS_AVAILABLE;
            return state.result;
        }

        QuestHub const* hub = hubDb.GetQuestHubById(hubId);
        if (!hub)
        {
            TC_LOG_ERROR("playerbot.pathfinding",
                "QuestPathfinder: Quest hub {} not found in database", hubId);
            state.result = QuestPathfindingResult::NO_QUEST_HUBS_AVAILABLE;
            return state.result;
        }

        // Check if hub is appropriate for player
    if (!hub->IsAppropriateFor(player))
        {
            TC_LOG_WARN("playerbot.pathfinding",
                "QuestPathfinder: Quest hub {} is not appropriate for player {} (level {}, team {})",
                hubId, player->GetName(), player->GetLevel(), player->GetTeamId());
            state.result = QuestPathfindingResult::NO_QUEST_HUBS_AVAILABLE;
            return state.result;
        }

        // Find nearest quest giver in the hub
        Creature* questGiver = FindNearestQuestGiverInHub(player, hub);
        if (!questGiver)
        {
            // Fallback: Use hub center
            state.destination = hub->location;
            state.targetCreatureEntry = 0;
        }
        else
        {
            state.targetCreatureGuid = questGiver->GetGUID();
            state.targetCreatureEntry = questGiver->GetEntry();
            state.destination = *questGiver;
        }

        // Validate destination
        QuestPathfindingResult validationResult = ValidateInput(player, state.destination);
        if (validationResult != QuestPathfindingResult::SUCCESS)
        {
            state.result = validationResult;
            return state.result;
        }

        // Generate path
        QuestPathfindingResult pathResult = GeneratePath(
            player,
            state.destination,
            options,
            state.path,
            state.pathLength);

        if (pathResult != QuestPathfindingResult::SUCCESS)
        {
            state.result = pathResult;
            return state.result;
        }

        // Estimate travel time
        state.estimatedTravelTime = EstimateTravelTime(state.pathLength, player);

        state.result = QuestPathfindingResult::SUCCESS;
        return state.result;
    }

    QuestPathfindingResult QuestPathfinder::CalculatePathToQuestGiver(
        Player* player,
        ObjectGuid creatureGuid,
        QuestPathfindingOptions const& options,
        QuestPathfindingState& state)
    {
        // Reset state
        state.Reset();
        state.lastUpdateTime = GameTime::GetGameTimeMS();
        state.targetCreatureGuid = creatureGuid;

        // Validate player
    if (!player)
        {
            state.result = QuestPathfindingResult::PLAYER_INVALID;
            return state.result;
        }

        // Find creature in world
        Creature* questGiver = ObjectAccessor::GetCreature(*player, creatureGuid);
        if (!questGiver)
        {
            TC_LOG_ERROR("playerbot.pathfinding",
                "QuestPathfinder: Quest giver creature {} not found in world",
                creatureGuid.ToString());
            state.result = QuestPathfindingResult::QUEST_GIVER_NOT_FOUND;
            return state.result;
        }

        // Check if creature is a quest giver
    if (!questGiver->IsQuestGiver())
        {
            TC_LOG_WARN("playerbot.pathfinding",
                "QuestPathfinder: Creature {} ({}) is not a quest giver",
                questGiver->GetName(), creatureGuid.ToString());
            state.result = QuestPathfindingResult::QUEST_GIVER_NOT_FOUND;
            return state.result;
        }

        state.targetCreatureEntry = questGiver->GetEntry();
        state.destination = *questGiver;

        // Validate destination
        QuestPathfindingResult validationResult = ValidateInput(player, state.destination);
        if (validationResult != QuestPathfindingResult::SUCCESS)
        {
            state.result = validationResult;
            return state.result;
        }

        // Generate path
        QuestPathfindingResult pathResult = GeneratePath(
            player,
            state.destination,
            options,
            state.path,
            state.pathLength);

        if (pathResult != QuestPathfindingResult::SUCCESS)
        {
            state.result = pathResult;
            return state.result;
        }

        // Estimate travel time
        state.estimatedTravelTime = EstimateTravelTime(state.pathLength, player);

        state.result = QuestPathfindingResult::SUCCESS;
        return state.result;
    }

    QuestPathfindingResult QuestPathfinder::NavigateAlongPath(
        Player* player,
        QuestPathfindingState& state)
    {
        // Validate player
    if (!player)
        {
            state.result = QuestPathfindingResult::PLAYER_INVALID;
            return state.result;
        }

        // Validate path
    if (state.path.empty())
        {
            TC_LOG_ERROR("playerbot.pathfinding",
                "QuestPathfinder: Cannot navigate - path is empty for player {}",
                player->GetName());
            state.result = QuestPathfindingResult::NO_PATH_FOUND;
            return state.result;
        }

        // Get destination (last position in path)
        Position const& destination = state.path.back();

        // Use validated pathfinding for bot players
        bool movementInitiated = false;
        if (BotAI* ai = GetBotAI(player))
        {
            if (ai->MoveTo(destination, true))
            {
                movementInitiated = true;
            }
            else
            {
                // Fallback to legacy if validation fails
                player->GetMotionMaster()->MovePoint(
                    0,                          // Movement ID
                    destination,                // Destination position
                    true,                       // Generate path using navmesh
                    ::std::nullopt,              // No specific final orientation
                    ::std::nullopt               // Use default movement speed
                );
                movementInitiated = true;
            }
        }
        else
        {
            // Non-bot player - use standard movement
            player->GetMotionMaster()->MovePoint(
                0,                          // Movement ID
                destination,                // Destination position
                true,                       // Generate path using navmesh
                ::std::nullopt,              // No specific final orientation
                ::std::nullopt               // Use default movement speed
            );
            movementInitiated = true;
        }

        state.movementInitiated = movementInitiated;
        state.lastUpdateTime = GameTime::GetGameTimeMS();

        TC_LOG_DEBUG("playerbot.pathfinding",
            "QuestPathfinder: Initiated movement for player {} to ({:.1f}, {:.1f}, {:.1f}) - {:.1f} yard path",
            player->GetName(),
            destination.GetPositionX(),
            destination.GetPositionY(),
            destination.GetPositionZ(),
            state.pathLength);

        state.result = QuestPathfindingResult::SUCCESS;
        return state.result;
    }

    bool QuestPathfinder::HasArrivedAtDestination(
        Player const* player,
        QuestPathfindingState const& state,
        float interactionRange)
    {
        if (!player || !state.IsValid())
            return false;

        // Calculate 2D distance to destination
        float dx = player->GetPositionX() - state.destination.GetPositionX();
        float dy = player->GetPositionY() - state.destination.GetPositionY();
        float distance = ::std::sqrt(dx * dx + dy * dy);

        return distance <= interactionRange;
    }

    Creature* QuestPathfinder::FindNearestQuestGiverInHub(
        Player const* player,
        QuestHub const* hub)
    {
        if (!player || !hub)
            return nullptr;

        Creature* nearestQuestGiver = nullptr;
        float minDistance = ::std::numeric_limits<float>::max();

        // Iterate through all creature entries in the hub
    for (uint32 creatureEntry : hub->creatureIds)
        {
            // Find all creatures with this entry in the world
            // Use Map::GetCreatureBySpawnId for efficient lookup
            Map* map = player->GetMap();
            if (!map)
                continue;

            // Alternative: Use creature spawns from map
            // Since we don't have spawn IDs, we need to search the map's creature list
            // This is O(n) but acceptable since quest hubs are spatially localized

            auto const& creatures = map->GetCreatureBySpawnIdStore();
            for (auto const& [spawnId, creature] : creatures)
            {
                if (!creature || !creature->IsInWorld())
                    continue;

                // Check if this is the creature entry we're looking for
    if (creature->GetEntry() != creatureEntry)
                    continue;

                // Check if creature is a quest giver
    if (!creature->IsQuestGiver())
                    continue;

                // Check if creature is within hub radius
    if (!hub->ContainsPosition(*creature))
                    continue;

                // Calculate distance to player
                float distance = player->GetDistance2d(creature);
                if (distance < minDistance)
                {
                    minDistance = distance;
                    nearestQuestGiver = creature;
                }
            }
        }

        return nearestQuestGiver;
    }

    char const* QuestPathfinder::GetResultString(QuestPathfindingResult result)
    {
        switch (result)
        {
            case QuestPathfindingResult::SUCCESS:
                return "SUCCESS";
            case QuestPathfindingResult::NO_QUEST_HUBS_AVAILABLE:
                return "NO_QUEST_HUBS_AVAILABLE";
            case QuestPathfindingResult::NO_PATH_FOUND:
                return "NO_PATH_FOUND";
            case QuestPathfindingResult::PLAYER_INVALID:
                return "PLAYER_INVALID";
            case QuestPathfindingResult::QUEST_GIVER_NOT_FOUND:
                return "QUEST_GIVER_NOT_FOUND";
            case QuestPathfindingResult::ALREADY_AT_DESTINATION:
                return "ALREADY_AT_DESTINATION";
            case QuestPathfindingResult::MOVEMENT_DISABLED:
                return "MOVEMENT_DISABLED";
            case QuestPathfindingResult::PATH_TOO_LONG:
                return "PATH_TOO_LONG";
            case QuestPathfindingResult::INVALID_DESTINATION:
                return "INVALID_DESTINATION";
            default:
                return "UNKNOWN";
        }
    }

    // ============================================================================
    // Private Helper Methods
    // ============================================================================

    QuestPathfindingResult QuestPathfinder::GeneratePath(
        Player const* player,
        Position const& destination,
        QuestPathfindingOptions const& options,
        ::std::vector<Position>& path,
        float& pathLength)
    {
        path.clear();
        pathLength = 0.0f;

        // Validate input
        QuestPathfindingResult validationResult = ValidateInput(player, destination);
        if (validationResult != QuestPathfindingResult::SUCCESS)
            return validationResult;

        // Create PathGenerator for player
        PathGenerator pathGen(player);

        // Configure PathGenerator options
        pathGen.SetUseStraightPath(options.allowStraightPath);
        pathGen.SetPathLengthLimit(options.maxPathDistance);

        // Calculate path from player's current position to destination
        bool pathCalculated = pathGen.CalculatePath(
            destination.GetPositionX(),
            destination.GetPositionY(),
            destination.GetPositionZ(),
            options.forceDestination
        );

        if (!pathCalculated)
        {
            TC_LOG_WARN("playerbot.pathfinding",
                "QuestPathfinder: PathGenerator::CalculatePath failed for player {} to ({:.1f}, {:.1f}, {:.1f})",
                player->GetName(),
                destination.GetPositionX(),
                destination.GetPositionY(),
                destination.GetPositionZ());
            return QuestPathfindingResult::NO_PATH_FOUND;
        }

        // Get path type to check for errors
        PathType pathType = pathGen.GetPathType();

        // Check for no path
    if (pathType & PATHFIND_NOPATH)
        {
            TC_LOG_WARN("playerbot.pathfinding",
                "QuestPathfinder: No valid path found (PATHFIND_NOPATH) for player {} to ({:.1f}, {:.1f}, {:.1f})",
                player->GetName(),
                destination.GetPositionX(),
                destination.GetPositionY(),
                destination.GetPositionZ());
            return QuestPathfindingResult::NO_PATH_FOUND;
        }

        // Get calculated path
        Movement::PointsArray const& pathPoints = pathGen.GetPath();
        if (pathPoints.empty())
        {
            TC_LOG_WARN("playerbot.pathfinding",
                "QuestPathfinder: PathGenerator returned empty path for player {}",
                player->GetName());
            return QuestPathfindingResult::NO_PATH_FOUND;
        }

        // Convert PointsArray to std::vector<Position>
        path.reserve(pathPoints.size());
        for (auto const& point : pathPoints)
        {
            Position pos;
            pos.Relocate(point.x, point.y, point.z);
            path.push_back(pos);
        }

        // Get path length from PathGenerator
        pathLength = pathGen.GetPathLength();

        // Check if path exceeds maximum distance
    if (pathLength > options.maxPathDistance)
        {
            TC_LOG_WARN("playerbot.pathfinding",
                "QuestPathfinder: Path length ({:.1f} yards) exceeds maximum ({:.1f} yards) for player {}",
                pathLength, options.maxPathDistance, player->GetName());
            return QuestPathfindingResult::PATH_TOO_LONG;
        }

        // Log path type diagnostics
    if (pathType & PATHFIND_INCOMPLETE)
        {
            TC_LOG_DEBUG("playerbot.pathfinding",
                "QuestPathfinder: Generated incomplete path (PATHFIND_INCOMPLETE) for player {} - {:.1f} yards",
                player->GetName(), pathLength);
        }
        else if (pathType & PATHFIND_SHORTCUT)
        {
            TC_LOG_DEBUG("playerbot.pathfinding",
                "QuestPathfinder: Generated shortcut path (PATHFIND_SHORTCUT) for player {} - {:.1f} yards",
                player->GetName(), pathLength);
        }
        else if (pathType & PATHFIND_NORMAL)
        {
            TC_LOG_DEBUG("playerbot.pathfinding",
                "QuestPathfinder: Generated normal path (PATHFIND_NORMAL) for player {} - {:.1f} yards",
                player->GetName(), pathLength);
        }

        return QuestPathfindingResult::SUCCESS;
    }

    QuestHub const* QuestPathfinder::SelectBestQuestHub(
        Player const* player,
        ::std::vector<QuestHub const*> const& hubs,
        QuestPathfindingOptions::SelectionStrategy strategy)
    {
        if (hubs.empty())
            return nullptr;

        switch (strategy)
        {
            case QuestPathfindingOptions::SelectionStrategy::NEAREST_FIRST:
            {
                // Select hub with minimum distance
                QuestHub const* nearest = nullptr;
                float minDistance = ::std::numeric_limits<float>::max();

                for (auto const* hub : hubs)
                {
                    float distance = hub->GetDistanceFrom(player);
                    if (distance < minDistance)
                    {
                        minDistance = distance;
                        nearest = hub;
                    }
                }

                return nearest;
            }

            case QuestPathfindingOptions::SelectionStrategy::MOST_QUESTS_FIRST:
            {
                // Select hub with maximum quest count
                QuestHub const* mostQuests = nullptr;
                size_t maxQuestCount = 0;

                for (auto const* hub : hubs)
                {
                    if (hub->questIds.size() > maxQuestCount)
                    {
                        maxQuestCount = hub->questIds.size();
                        mostQuests = hub;
                    }
                }

                return mostQuests;
            }

            case QuestPathfindingOptions::SelectionStrategy::BEST_SUITABILITY_SCORE:
            default:
            {
                // Select hub with highest suitability score (default)
                // GetQuestHubsForPlayer already returns hubs sorted by suitability score
                // So just return the first hub (highest score)
                return hubs.front();
            }
        }
    }

    QuestPathfindingResult QuestPathfinder::ValidateInput(
        Player const* player,
        Position const& destination)
    {
        // Check player validity
    if (!player)
            return QuestPathfindingResult::PLAYER_INVALID;

        // Check destination validity (not 0,0,0)
    if (destination.GetPositionX() == 0.0f &&
            destination.GetPositionY() == 0.0f &&
            destination.GetPositionZ() == 0.0f)
        {
            TC_LOG_ERROR("playerbot.pathfinding",
                "QuestPathfinder: Invalid destination (0,0,0) for player {}",
                player->GetName());
            return QuestPathfindingResult::INVALID_DESTINATION;
        }

        // Check if player and destination are on same map
        // PathGenerator will handle cross-map pathfinding errors
        // but we can pre-validate here for better error messages
        Map* map = player->GetMap();
        if (!map)
        {
            TC_LOG_ERROR("playerbot.pathfinding",
                "QuestPathfinder: Player {} has no map",
                player->GetName());
            return QuestPathfindingResult::PLAYER_INVALID;
        }

        return QuestPathfindingResult::SUCCESS;
    }

    float QuestPathfinder::EstimateTravelTime(float pathLength, Player const* player)
    {
        if (pathLength <= 0.0f)
            return 0.0f;

        // Movement speed constants (yards per second)
        constexpr float WALK_SPEED = 2.5f;      // Walking speed
        constexpr float RUN_SPEED = 7.0f;       // Running speed
        constexpr float MOUNT_SPEED = 14.0f;    // 100% mount speed

        // Determine current movement speed
        float speed = RUN_SPEED; // Default to running
    if (player)
        {
            // Use actual player run speed (includes mount speed, speed buffs, and talent modifiers)
            speed = player->GetSpeed(MOVE_RUN);
        }

        // Calculate travel time in seconds
        float travelTime = pathLength / speed;

        return travelTime;
    }

} // namespace Playerbot
