/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Lock-Free Refactored Gathering Manager
 * This implementation removes all ObjectAccessor calls from worker threads
 * and uses action queuing for main thread execution.
 */

#include "GatheringManager.h"
#include "GameTime.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "ObjectMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Log.h"
#include "Map.h"
#include "Spatial/SpatialGridManager.h"
#include "Threading/BotActionQueue.h"
#include "Threading/BotAction.h"
#include "SharedDefines.h"

namespace Playerbot
{

// Constants
static constexpr float GATHERING_RANGE = 5.0f;
static constexpr float GATHERING_SEARCH_RANGE = 100.0f;
static constexpr float SKINNING_RANGE = 5.0f;

// Gathering spell IDs
static constexpr uint32 SPELL_SKINNING = 8613;
static constexpr uint32 SPELL_MINING = 2575;
static constexpr uint32 SPELL_HERBALISM = 2366;

/**
 * Lock-Free Implementation of ScanForNodes
 * Uses spatial grid snapshots instead of ObjectAccessor
 */
::std::vector<GatheringNode> GatheringManager::ScanForNodes_LockFree(float range)
{
    ::std::vector<GatheringNode> nodes;

    Player* bot = GetBot();
    if (!bot)
        return nodes;

    Map* map = bot->GetMap();
    if (!map)
        return nodes;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
        return nodes;

    uint32 currentTime = GameTime::GetGameTimeMS();

    // Scan for herb/mining nodes (GameObjects)
    if (HasGatheringSkill(GatheringSkillType::HERBALISM) ||
        HasGatheringSkill(GatheringSkillType::MINING))
    {
        ::std::vector<DoubleBufferedSpatialGrid::GameObjectSnapshot> objects =
            spatialGrid->QueryNearbyGameObjects(bot->GetPosition(), range);

        for (auto const& snapshot : objects)
        {
            // Check if it's a gathering node
            GatheringNodeType nodeType = GetNodeTypeFromEntry(snapshot.entry);
            if (nodeType == GatheringNodeType::NONE)
                continue;

            // Check if node is available
    if (!snapshot.isSpawned || snapshot.isInUse)
                continue;

            // Check skill requirements
    if (nodeType == GatheringNodeType::HERB &&
                !HasGatheringSkill(GatheringSkillType::HERBALISM))
                continue;
                !HasGatheringSkill(GatheringSkillType::MINING))
                continue;

            // Create gathering node data
            GatheringNode node;
            node.guid = snapshot.guid;
            node.entry = snapshot.entry;
            node.nodeType = nodeType;
            node.position = snapshot.position;
            node.skillRequired = GetRequiredSkillLevel(snapshot.entry);
            node.distance = snapshot.position.GetExactDist(bot->GetPosition());
            node.lastSeen = currentTime;

            nodes.push_back(node);

            TC_LOG_DEBUG("playerbot.gathering",
                "Found %s node %u at distance %.1f",
                nodeType == GatheringNodeType::HERB ? "herb" : "mining",
                snapshot.entry, node.distance);
        }
    }

    // Scan for skinnable creatures
    if (HasGatheringSkill(GatheringSkillType::SKINNING))
    {
        ::std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> creatures =
            spatialGrid->QueryNearbyCreatures(bot->GetPosition(), range);

        for (auto const& snapshot : creatures)
        {
            // Check if creature is dead and skinnable
    if (snapshot.isAlive)
                continue;

            if (!snapshot.isSkinnable)
                continue;

            // Check if it's lootable by the bot
    if (!snapshot.lootRecipients.empty())
            {
                bool canLoot = false;
                for (auto const& recipient : snapshot.lootRecipients)
                {
                    if (recipient == bot->GetGUID())
                    {
                        canLoot = true;
                        break;
                    }
                }

                if (!canLoot)
                    continue;
            }

            // Create gathering node for skinning
            GatheringNode node;
            node.guid = snapshot.guid;
            node.entry = snapshot.entry;
            node.nodeType = GatheringNodeType::CREATURE_CORPSE;
            node.position = snapshot.position;
            node.skillRequired = GetRequiredSkinningLevel(snapshot.level);
            node.distance = snapshot.position.GetExactDist(bot->GetPosition());
            node.lastSeen = currentTime;

            nodes.push_back(node);

            TC_LOG_DEBUG("playerbot.gathering",
                "Found skinnable creature %u at distance %.1f",
                snapshot.entry, node.distance);
        }
    }

    // Sort by distance
    ::std::sort(nodes.begin(), nodes.end(),
        [](GatheringNode const& a, GatheringNode const& b)
        {
            return a.distance < b.distance;
        });

    return nodes;
}

/**
 * Lock-Free Implementation of GatherNode
 * Queues gathering action for main thread execution
 */
bool GatheringManager::QueueGatherNode_LockFree(GatheringNode const& node)
{
    Player* bot = GetBot();
    if (!bot || node.guid.IsEmpty())
        return false;

    // Determine gathering spell based on node type
    uint32 spellId = 0;
    switch (node.nodeType)
    {
        case GatheringNodeType::HERB:
            spellId = SPELL_HERBALISM;
            break;
        case GatheringNodeType::MINERAL:
            spellId = SPELL_MINING;
            break;
        case GatheringNodeType::CREATURE_CORPSE:
            spellId = SPELL_SKINNING;
            break;
        default:
            return false;
    }

    // Check skill level
    uint32 currentSkill = GetGatheringSkillLevel(node.nodeType);
    if (currentSkill < node.skillRequired)
    {
        TC_LOG_DEBUG("playerbot.gathering",
            "Bot %s lacks skill for node (has %u, needs %u)",
            bot->GetName().c_str(), currentSkill, node.skillRequired);
        return false;
    }

    // Verify node still exists and is in range using spatial grid
    Map* map = bot->GetMap();
    if (!map)
        return false;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
        return false;

    bool nodeValid = false;
    float nodeDistance = 0.0f;
    Position nodePosition;

    if (node.nodeType == GatheringNodeType::CREATURE_CORPSE)
    {
        // Check creature snapshots
        ::std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> creatures =
            spatialGrid->QueryNearbyCreatures(bot->GetPosition(), GATHERING_SEARCH_RANGE);

        for (auto const& snapshot : creatures)
        {
            if (snapshot.guid == node.guid)
            {
                // Verify creature is still dead and skinnable
    if (!snapshot.isAlive && snapshot.isSkinnable)
                {
                    nodeValid = true;
                    nodeDistance = snapshot.position.GetExactDist(bot->GetPosition());
                    nodePosition = snapshot.position;
                }
                break;
            }
        }
    }
    else
    {
        // Check GameObject snapshots
        ::std::vector<DoubleBufferedSpatialGrid::GameObjectSnapshot> objects =
            spatialGrid->QueryNearbyGameObjects(bot->GetPosition(), GATHERING_SEARCH_RANGE);

        for (auto const& snapshot : objects)
        {
            if (snapshot.guid == node.guid)
            {
                // Verify object is still spawned and not in use
    if (snapshot.isSpawned && !snapshot.isInUse)
                {
                    nodeValid = true;
                    nodeDistance = snapshot.position.GetExactDist(bot->GetPosition());
                    nodePosition = snapshot.position;
                }
                break;
            }
        }
    }

    if (!nodeValid)
    {
        TC_LOG_DEBUG("playerbot.gathering",
            "Node %s no longer valid", node.guid.ToString().c_str());
        return false;
    }

    // Check range and queue appropriate action
    float requiredRange = (node.nodeType == GatheringNodeType::CREATURE_CORPSE) ?
        SKINNING_RANGE : GATHERING_RANGE;

    if (nodeDistance <= requiredRange)
    {
        // Queue gathering action for main thread
        BotAction action;

        if (node.nodeType == GatheringNodeType::CREATURE_CORPSE)
        {
            action.type = BotActionType::SKIN_CREATURE;
        }
        else
        {
            action.type = BotActionType::GATHER_OBJECT;
        }

        action.botGuid = bot->GetGUID();
        action.targetGuid = node.guid;
        action.spellId = spellId;
        action.priority = 5;  // Gathering is medium priority
        action.queuedTime = GameTime::GetGameTimeMS();

        BotActionQueue::Instance()->Push(action);

        // Update internal state
        _currentNode = node;
        _isGathering = true;
        _lastGatherTime = GameTime::GetGameTimeMS();
        TC_LOG_DEBUG("playerbot.gathering",
            "Bot %s queued gathering for %s %s",
            bot->GetName().c_str(),
            GetNodeTypeName(node.nodeType),
            node.guid.ToString().c_str());

        return true;
    }
    else
    {
        // Need to move closer first
        Position targetPos = CalculateApproachPosition(
            bot->GetPosition(), nodePosition, requiredRange - 1.0f);

        BotAction moveAction = BotAction::MoveToPosition(
            bot->GetGUID(), targetPos, GameTime::GetGameTimeMS()
        );
        moveAction.priority = 4;  // Movement for gathering is lower priority

        BotActionQueue::Instance()->Push(moveAction);

        TC_LOG_DEBUG("playerbot.gathering",
            "Bot %s moving to gathering node, distance %.1f",
            bot->GetName().c_str(), nodeDistance);

        return false;  // Not gathering yet, just moving
    }
}

/**
 * Lock-Free Implementation of main Update loop
 * Coordinates gathering without ObjectAccessor calls
 */
void GatheringManager::Update_LockFree(uint32 diff)
{
    if (!IsEnabled())
        return;

    Player* bot = GetBot();
    if (!bot)
        return;

    // Update timers
    if (_scanCooldown > diff)
        _scanCooldown -= diff;
    else
        _scanCooldown = 0;

    if (_gatherCooldown > diff)
        _gatherCooldown -= diff;
    else
        _gatherCooldown = 0;

    // Check if currently gathering
    if (_isGathering)
    {
        // Check if gathering completed (based on time)
        uint32 gatherDuration = GetGatheringDuration(_currentNode.nodeType);
        if (getMSTimeDiff(_lastGatherTime, GameTime::GetGameTimeMS()) > gatherDuration)
        {
            _isGathering = false;
            _gatherCooldown = 1000;  // 1 second cooldown between gathers

            // Update statistics
            _statistics.nodesGathered++;
            _statistics.lastGatherTime = GameTime::GetGameTimeMS();

            TC_LOG_DEBUG("playerbot.gathering",
                "Bot %s completed gathering node %s",
                bot->GetName().c_str(), _currentNode.guid.ToString().c_str());
        }

        return;  // Don't scan while gathering
    }

    // Check cooldowns
    if (_scanCooldown > 0 || _gatherCooldown > 0)
        return;

    // Scan for new nodes
    ::std::vector<GatheringNode> nearbyNodes = ScanForNodes_LockFree(GATHERING_SEARCH_RANGE);

    if (nearbyNodes.empty())
    {
        _scanCooldown = 5000;  // Scan again in 5 seconds
        return;
    }

    // Try to gather nearest valid node
    for (auto const& node : nearbyNodes)
    {
        // Check if we can gather this node
    if (GetGatheringSkillLevel(node.nodeType) < node.skillRequired)
            continue;

        // Try to queue gathering
    if (QueueGatherNode_LockFree(node))
        {
            break;  // Successfully queued gathering
        }
    }

    // Set scan cooldown
    _scanCooldown = 2000;  // Scan again in 2 seconds
}

/**
 * Helper: Get node type from GameObject entry
 */
GatheringNodeType GatheringManager::GetNodeTypeFromEntry(uint32 entry)
{
    // This would query DBC/DB2 data in real implementation
    // For now, use simplified logic based on entry ranges

    // Herb nodes typically in range 1731-2047, 142140-142145, etc.
    if ((entry >= 1731 && entry <= 2047) ||
        (entry >= 142140 && entry <= 142145) ||
        (entry >= 176583 && entry <= 176589))
    {
        return GatheringNodeType::HERB;
    }

    // Mining nodes typically in range 1731-1735, 2040-2047, etc.
    if ((entry >= 1731 && entry <= 1735) ||
        (entry >= 2040 && entry <= 2047) ||
        (entry >= 165658 && entry <= 165658))
    {
        return GatheringNodeType::MINERAL;
    }

    return GatheringNodeType::NONE;
}

/**
 * Helper: Get required skill level for node
 */
uint32 GatheringManager::GetRequiredSkillLevel(uint32 entry)
{
    // This would query DBC/DB2 data in real implementation
    // For now, return simplified values

    // Copper: 1
    if (entry == 1731) return 1;
    // Tin: 65
    if (entry == 1732) return 65;
    // Iron: 125
    if (entry == 1735) return 125;
    // Mithril: 175
    if (entry == 2040) return 175;

    // Default
    return 1;
}

/**
 * Helper: Get required skinning level based on creature level
 */
uint32 GatheringManager::GetRequiredSkinningLevel(uint32 creatureLevel)
{
    // Formula: creature_level * 5
    return creatureLevel * 5;
}

/**
 * Helper: Get current gathering skill level
 */
uint32 GatheringManager::GetGatheringSkillLevel(GatheringNodeType nodeType)
{
    Player* bot = GetBot();
    if (!bot)
        return 0;

    uint32 skillId = 0;
    switch (nodeType)
    {
        case GatheringNodeType::HERB:
            skillId = SKILL_HERBALISM;
            break;
        case GatheringNodeType::MINERAL:
            skillId = SKILL_MINING;
            break;
        case GatheringNodeType::CREATURE_CORPSE:
            skillId = SKILL_SKINNING;
            break;
        default:
            return 0;
    }

    return bot->GetSkillValue(skillId);
}

/**
 * Helper: Check if bot has gathering skill
 */
bool GatheringManager::HasGatheringSkill(GatheringSkillType skillType)
{
    Player* bot = GetBot();
    if (!bot)
        return false;

    uint32 skillId = 0;
    switch (skillType)
    {
        case GatheringSkillType::HERBALISM:
            skillId = SKILL_HERBALISM;
            break;
        case GatheringSkillType::MINING:
            skillId = SKILL_MINING;
            break;
        case GatheringSkillType::SKINNING:
            skillId = SKILL_SKINNING;
            break;
        default:
            return false;
    }

    return bot->HasSkill(skillId);
}

/**
 * Helper: Get gathering duration in milliseconds
 */
uint32 GatheringManager::GetGatheringDuration(GatheringNodeType nodeType)
{
    switch (nodeType)
    {
        case GatheringNodeType::HERB:
            return 3000;  // 3 seconds for herbs
        case GatheringNodeType::MINERAL:
            return 3000;  // 3 seconds for mining
        case GatheringNodeType::CREATURE_CORPSE:
            return 2000;  // 2 seconds for skinning
        default:
            return 3000;
    }
}

/**
 * Helper: Get node type name for logging
 */
const char* GatheringManager::GetNodeTypeName(GatheringNodeType nodeType)
{
    switch (nodeType)
    {
        case GatheringNodeType::HERB:
            return "herb";
        case GatheringNodeType::MINERAL:
            return "mineral";
        case GatheringNodeType::CREATURE_CORPSE:
            return "corpse";
        default:
            return "unknown";
    }
}

/**
 * Helper: Calculate position to approach a target
 */
Position GatheringManager::CalculateApproachPosition(
    Position const& from,
    Position const& to,
    float desiredDistance)
{
    float angle = from.GetAngle(&to);
    float currentDist = from.GetExactDist(&to);

    if (currentDist <= desiredDistance)
        return from;  // Already close enough

    float moveDist = currentDist - desiredDistance;

    Position result;
    result.m_positionX = from.m_positionX + moveDist * cos(angle);
    result.m_positionY = from.m_positionY + moveDist * sin(angle);
    result.m_positionZ = from.m_positionZ;  // Will be corrected by pathfinding

    return result;
}

} // namespace Playerbot