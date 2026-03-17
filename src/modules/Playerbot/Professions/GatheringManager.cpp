/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "GatheringManager.h"
#include "GameTime.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "GameObject.h"
#include "Creature.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Log.h"
#include "World.h"
#include "Map.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "GameObjectAI.h"
#include "MotionMaster.h"
#include "MovementDefines.h"
#include "UpdateFields.h"
#include <algorithm>
#include "../Spatial/SpatialGridManager.h"  // Lock-free spatial grid for deadlock fix

namespace Playerbot
{

GatheringManager::GatheringManager(Player* bot, BotAI* ai)
    : BehaviorManager(bot, ai, 1000, "GatheringManager")  // 1 second update interval
{
    // Configuration will be loaded in OnInitialize
}

bool GatheringManager::OnInitialize()
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return false;

    // CRITICAL: Do NOT call GetProfessionSkill() here!
    // The bot's skill data (mSkillStatus map) may not be loaded yet during login,
    // causing ACCESS_VIOLATION in Player::GetSkillValue(). Defer to first OnUpdate().
    _professionsInitialized = false;

    return true;
}

void GatheringManager::OnShutdown()
{
    // Clean up any ongoing gathering
    if (_isGathering.load())
    {
        StopGathering();
    }

    // Clear detected nodes (no lock needed - per-bot instance data)
    _detectedNodes.clear();

    _detectedNodeCount.store(0, ::std::memory_order_release);
    _hasNearbyResources.store(false, ::std::memory_order_release);
}

void GatheringManager::OnUpdate(uint32 elapsed)
{
    if (!GetBot() || !GetBot()->IsInWorld() || !_gatheringEnabled)
        return;

    // CRITICAL: Deferred profession initialization - bot's skill data must be ready
    if (!_professionsInitialized)
    {
        _gatherMining = GetProfessionSkill(GatheringNodeType::MINING_VEIN) > 0;
        _gatherHerbalism = GetProfessionSkill(GatheringNodeType::HERB_NODE) > 0;
        _gatherSkinning = GetProfessionSkill(GatheringNodeType::CREATURE_CORPSE) > 0;
        _gatherFishing = GetProfessionSkill(GatheringNodeType::FISHING_POOL) > 0;
        _professionsInitialized = true;

        TC_LOG_DEBUG("bot.playerbot", "GatheringManager professions initialized for bot {} - Mining: {}, Herbalism: {}, Skinning: {}, Fishing: {}",
            GetBot()->GetName(),
            _gatherMining ? "Yes" : "No",
            _gatherHerbalism ? "Yes" : "No",
            _gatherSkinning ? "Yes" : "No",
            _gatherFishing ? "Yes" : "No");
    }

    // Update node detection every few seconds
    auto now = ::std::chrono::steady_clock::now();
    if (::std::chrono::duration_cast<::std::chrono::milliseconds>(now - _lastScanTime).count() >= NODE_SCAN_INTERVAL)
    {
        UpdateDetectedNodes();
        _lastScanTime = now;
    }

    // Process humanization session state
    ProcessSession(elapsed);

    // Skip gathering if on mini-break
    if (_sessionState == GatheringSessionState::MINI_BREAK)
        return;

    // Process current gathering action
    if (_isGathering.load())
    {
        ProcessCurrentGathering();
    }
    // If not gathering and nodes are available, select best node
    else if (_hasNearbyResources.load() && !GetBot()->IsInCombat())
    {
        GatheringNode const* bestNode = SelectBestNode();
        if (bestNode && CanGatherFromNode(*bestNode))
        {
            if (IsInGatheringRange(*bestNode))
            {
                GatherFromNode(*bestNode);
            }
            else if (_gatherWhileMoving)
            {
                PathToNode(*bestNode);
            }
        }
    }

    // Clean up expired nodes
    CleanupExpiredNodes();

    // Update state flags
    _detectedNodeCount.store(static_cast<uint32>(_detectedNodes.size()), ::std::memory_order_release);
    _hasNearbyResources.store(!_detectedNodes.empty(), ::std::memory_order_release);
}

::std::vector<GatheringNode> GatheringManager::ScanForNodes(float range)
{
    ::std::vector<GatheringNode> nodes;

    if (!GetBot() || !GetBot()->IsInWorld())
        return nodes;

    // Scan for different node types based on professions
    if (_gatherMining)
    {
        auto miningNodes = DetectMiningNodes(range);
        nodes.insert(nodes.end(), miningNodes.begin(), miningNodes.end());
    }

    if (_gatherHerbalism)
    {
        auto herbNodes = DetectHerbNodes(range);
        nodes.insert(nodes.end(), herbNodes.begin(), herbNodes.end());
    }

    if (_gatherFishing)
    {
        auto fishingPools = DetectFishingPools(range);
        nodes.insert(nodes.end(), fishingPools.begin(), fishingPools.end());
    }

    if (_gatherSkinning)
    {
        auto skinnableCreatures = DetectSkinnableCreatures(range);
        nodes.insert(nodes.end(), skinnableCreatures.begin(), skinnableCreatures.end());
    }

    // Sort by distance
    ::std::sort(nodes.begin(), nodes.end(),
        [](GatheringNode const& a, GatheringNode const& b) { return a.distance < b.distance; });

    return nodes;
}

GatheringNode const* GatheringManager::FindNearestNode(GatheringNodeType nodeType) const
{
    // No lock needed - _detectedNodes is per-bot instance data
    GatheringNode const* nearest = nullptr;
    float minDistance = ::std::numeric_limits<float>::max();

    for (auto const& node : _detectedNodes)
    {
        if (nodeType != GatheringNodeType::NONE && node.nodeType != nodeType)
            continue;

        if (node.distance < minDistance && node.isActive)
        {
            minDistance = node.distance;
            nearest = &node;
        }
    }

    return nearest;
}

bool GatheringManager::CanGatherFromNode(GatheringNode const& node) const
{
    if (!GetBot())
        return false;

    // Check if we have the required skill level
    uint16 currentSkill = GetProfessionSkill(node.nodeType);
    if (currentSkill < node.requiredSkill)
        return false;

    // Check if node type is enabled
    if (!IsProfessionEnabled(node.nodeType))
        return false;

    return true;
}

uint16 GatheringManager::GetRequiredSkillForNode(GatheringNode const& node) const
{
    return node.requiredSkill;
}

bool GatheringManager::GatherFromNode(GatheringNode const& node)
{
    if (!GetBot() || !CanGatherFromNode(node))
        return false;

    // Set gathering state
    _isGathering.store(true, ::std::memory_order_release);
    _currentNodeGuid = node.guid;
    _currentTarget = &node;
    _gatheringStartTime = ::std::chrono::steady_clock::now();
    _gatheringAttempts++;

    // Cast appropriate gathering spell
    return CastGatheringSpell(node);
}

bool GatheringManager::CastGatheringSpell(GatheringNode const& node)
{
    if (!GetBot())
        return false;

    uint32 spellId = GetGatheringSpellId(node.nodeType, GetProfessionSkill(node.nodeType));
    if (!spellId)
    {
        TC_LOG_DEBUG("bot.playerbot", "GatheringManager: No gathering spell for node type %u", static_cast<uint32>(node.nodeType));
        return false;
    }

    // Stop any current casting
    if (GetBot()->IsNonMeleeSpellCast(false))
        GetBot()->InterruptNonMeleeSpells(false);

    // DEADLOCK FIX: This method runs on main thread, so direct GUID resolution is safe
    // The node.guid was obtained from spatial grid snapshots (thread-safe)
    if (node.nodeType == GatheringNodeType::CREATURE_CORPSE)
    {
        // Skinning - cast spell on creature corpse
        Creature* creature = ObjectAccessor::GetCreature(*GetBot(), node.guid);
        if (!creature)
            return false;

        GetBot()->CastSpell(creature, spellId);
    }
    else
    {
        // Mining/Herbalism - use game object
        GameObject* gameObject = ObjectAccessor::GetGameObject(*GetBot(), node.guid);
        if (!gameObject)
            return false;

        gameObject->Use(GetBot());
    }

    _currentSpellId = spellId;
    return true;
}

bool GatheringManager::LootNode(GameObject* gameObject)
{
    if (!GetBot() || !gameObject)
        return false;

    // The game object's loot is handled by the core
    // We just need to mark that we're looting
    _isLooting = true;

    // Record statistics
    _statistics.nodesGathered++;

    return true;
}

bool GatheringManager::SkinCreature(Creature* creature)
{
    if (!GetBot() || !creature || !creature->HasUnitFlag(UNIT_FLAG_SKINNABLE))
        return false;

    // Cast skinning spell on the creature
    GetBot()->CastSpell(creature, SPELL_SKINNING);

    // Record statistics
    _statistics.nodesGathered++;

    return true;
}

uint32 GatheringManager::GetGatheringSpellId(GatheringNodeType nodeType, uint16 skillLevel) const
{
    switch (nodeType)
    {
        case GatheringNodeType::MINING_VEIN:
            return SPELL_MINING;
        case GatheringNodeType::HERB_NODE:
            return SPELL_HERB_GATHERING;
        case GatheringNodeType::CREATURE_CORPSE:
            return SPELL_SKINNING;
        case GatheringNodeType::FISHING_POOL:
            return SPELL_FISHING;
        default:
            return 0;
    }
}

bool GatheringManager::PathToNode(GatheringNode const& node)
{
    if (!GetBot() || !GetAI())
        return false;

    // Store return position if needed
    if (_returnToPathAfterGather)
    {
        _returnPosition = GetBot()->GetPosition();
    }

    // Move to the node
    GetAI()->MoveTo(node.position.GetPositionX(), node.position.GetPositionY(), node.position.GetPositionZ());

    _isMovingToNode.store(true, ::std::memory_order_release);
    _currentTarget = &node;

    TC_LOG_DEBUG("bot.playerbot", "GatheringManager: Moving to node at %.2f, %.2f, %.2f",
        node.position.GetPositionX(), node.position.GetPositionY(), node.position.GetPositionZ());

    return true;
}

bool GatheringManager::IsInGatheringRange(GatheringNode const& node) const
{
    if (!GetBot())
        return false;

    float range = (node.nodeType == GatheringNodeType::CREATURE_CORPSE) ? SKINNING_RANGE : GATHERING_RANGE;
    return GetBot()->GetDistance(node.position) <= range;
}

float GatheringManager::GetDistanceToNode(GatheringNode const& node) const
{
    if (!GetBot())
        return ::std::numeric_limits<float>::max();

    return GetBot()->GetDistance(node.position);
}

void GatheringManager::StopGathering()
{
    _isGathering.store(false, ::std::memory_order_release);
    _isMovingToNode.store(false, ::std::memory_order_release);
    _currentNodeGuid.Clear();
    _currentTarget = nullptr;
    _currentSpellId = 0;
    _isLooting = false;
    _gatheringAttempts = 0;

    // Stop any current casting
    if (GetBot() && GetBot()->IsNonMeleeSpellCast(false))
        GetBot()->InterruptNonMeleeSpells(false);

    // Return to original position if configured
    if (_returnToPathAfterGather && _returnPosition.IsPositionValid() && GetAI())
    {
        GetAI()->MoveTo(_returnPosition.GetPositionX(), _returnPosition.GetPositionY(), _returnPosition.GetPositionZ());
    }
}

void GatheringManager::SetProfessionEnabled(GatheringNodeType nodeType, bool enable)
{
    switch (nodeType)
    {
        case GatheringNodeType::MINING_VEIN:
            _gatherMining = enable;
            break;
        case GatheringNodeType::HERB_NODE:
            _gatherHerbalism = enable;
            break;
        case GatheringNodeType::CREATURE_CORPSE:
            _gatherSkinning = enable;
            break;
        case GatheringNodeType::FISHING_POOL:
            _gatherFishing = enable;
            break;
        default:
            break;
    }
}

bool GatheringManager::IsProfessionEnabled(GatheringNodeType nodeType) const
{
    switch (nodeType)
    {
        case GatheringNodeType::MINING_VEIN:
            return _gatherMining;
        case GatheringNodeType::HERB_NODE:
            return _gatherHerbalism;
        case GatheringNodeType::CREATURE_CORPSE:
            return _gatherSkinning;
        case GatheringNodeType::FISHING_POOL:
            return _gatherFishing;
        default:
            return false;
    }
}

::std::vector<GatheringNode> GatheringManager::DetectMiningNodes(float range)
{
    ::std::vector<GatheringNode> nodes;

    if (!GetBot() || !GetBot()->GetMap())
        return nodes;

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = GetBot()->GetMap();
    if (!map)
        return nodes;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return nodes;
    }

    // Query nearby GameObjects (lock-free!)
    ::std::vector<DoubleBufferedSpatialGrid::GameObjectSnapshot> nearbyObjects =
        spatialGrid->QueryNearbyGameObjects(GetBot()->GetPosition(), range);

    // Filter for mining veins using snapshot data
    for (auto const& snapshot : nearbyObjects)
    {
        // GAMEOBJECT_TYPE_CHEST (3) is used for mining/herb nodes
    if (snapshot.goType != 3 || !snapshot.isSpawned)
            continue;

        GatheringNode node;
        node.guid = snapshot.guid;
        node.nodeType = GatheringNodeType::MINING_VEIN;
        node.position = snapshot.position;
        node.isActive = true;
        node.distance = GetBot()->GetExactDist(snapshot.position);

        nodes.push_back(node);
    }

    return nodes;
}

::std::vector<GatheringNode> GatheringManager::DetectHerbNodes(float range)
{
    ::std::vector<GatheringNode> nodes;

    if (!GetBot() || !GetBot()->GetMap())
        return nodes;

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = GetBot()->GetMap();
    if (!map)
        return nodes;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return nodes;
    }

    // Query nearby GameObjects (lock-free!)
    ::std::vector<DoubleBufferedSpatialGrid::GameObjectSnapshot> nearbyObjects =
        spatialGrid->QueryNearbyGameObjects(GetBot()->GetPosition(), range);

    // Filter for herb nodes using snapshot data
    for (auto const& snapshot : nearbyObjects)
    {
        // GAMEOBJECT_TYPE_CHEST (3) is used for mining/herb nodes
    if (snapshot.goType != 3 || !snapshot.isSpawned)
            continue;

        GatheringNode node;
        node.guid = snapshot.guid;
        node.nodeType = GatheringNodeType::HERB_NODE;
        node.position = snapshot.position;
        node.isActive = true;
        node.distance = GetBot()->GetExactDist(snapshot.position);

        nodes.push_back(node);
    }

    return nodes;
}

::std::vector<GatheringNode> GatheringManager::DetectFishingPools(float range)
{
    ::std::vector<GatheringNode> nodes;

    if (!GetBot() || !GetBot()->GetMap())
        return nodes;

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = GetBot()->GetMap();
    if (!map)
        return nodes;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return nodes;
    }

    // Query nearby GameObjects (lock-free!)
    ::std::vector<DoubleBufferedSpatialGrid::GameObjectSnapshot> nearbyObjects =
        spatialGrid->QueryNearbyGameObjects(GetBot()->GetPosition(), range);

    // Filter for fishing pools using snapshot data
    for (auto const& snapshot : nearbyObjects)
    {
        // GAMEOBJECT_TYPE_FISHINGHOLE (25) or GAMEOBJECT_TYPE_FISHINGNODE (17)
    if ((snapshot.goType != 25 && snapshot.goType != 17) || !snapshot.isSpawned)
            continue;

        GatheringNode node;
        node.guid = snapshot.guid;
        node.nodeType = GatheringNodeType::FISHING_POOL;
        node.position = snapshot.position;
        node.isActive = true;
        node.distance = GetBot()->GetExactDist(snapshot.position);

        nodes.push_back(node);
    }

    return nodes;
}

::std::vector<GatheringNode> GatheringManager::DetectSkinnableCreatures(float range)
{
    ::std::vector<GatheringNode> nodes;

    if (!GetBot() || !GetBot()->GetMap())
        return nodes;

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = GetBot()->GetMap();
    if (!map)
        return nodes;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return nodes;
    }

    // DEADLOCK FIX: Use snapshot-based query (thread-safe, lock-free)
    ::std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> nearbyCreatures =
        spatialGrid->QueryNearbyCreatures(GetBot()->GetPosition(), range);

    for (auto const& snapshot : nearbyCreatures)
    {
        // Use snapshot fields instead of pointer method calls
    if (!snapshot.isDead || !snapshot.isSkinnable)
            continue;

        // Create node info from snapshot data
        GatheringNode node;
        node.guid = snapshot.guid;
        node.nodeType = GatheringNodeType::CREATURE_CORPSE;
        node.position = snapshot.position;
        node.isActive = true;
        node.distance = GetBot()->GetExactDist(snapshot.position);
        nodes.push_back(node);
    }

    return nodes;
}

GatheringNode GatheringManager::CreateNodeInfo(GameObject* gameObject, GatheringNodeType nodeType)
{
    GatheringNode node;

    if (!gameObject)
        return node;

    node.guid = gameObject->GetGUID();
    node.nodeType = nodeType;
    node.entry = gameObject->GetEntry();
    node.position = gameObject->GetPosition();
    node.requiredSkill = 1;  // Would need proper skill requirement lookup
    node.detectionTime = GameTime::GetGameTimeMS();
    node.isActive = gameObject->isSpawned() && gameObject->GetGoState() == GO_STATE_READY;
    node.distance = GetBot() ? GetBot()->GetDistance(gameObject) : 0.0f;

    return node;
}

GatheringNode GatheringManager::CreateNodeInfo(Creature* creature)
{
    GatheringNode node;

    if (!creature)
        return node;

    node.guid = creature->GetGUID();
    node.nodeType = GatheringNodeType::CREATURE_CORPSE;
    node.entry = creature->GetEntry();
    node.position = creature->GetPosition();
    node.requiredSkill = 1;  // Would need proper skill requirement lookup
    node.detectionTime = GameTime::GetGameTimeMS();
    node.isActive = creature->isDead() && creature->HasUnitFlag(UNIT_FLAG_SKINNABLE);
    node.distance = GetBot() ? GetBot()->GetDistance(creature) : 0.0f;

    return node;
}

bool GatheringManager::ValidateNode(GatheringNode const& node)
{
    if (!GetBot() || !GetBot()->GetMap())
        return false;

    // DEADLOCK FIX: Use spatial grid to validate node instead of ObjectAccessor
    Map* map = GetBot()->GetMap();
    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
        return false;

    if (node.nodeType == GatheringNodeType::CREATURE_CORPSE)
    {
        // Query nearby creatures and find matching GUID
        ::std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> nearbyCreatures =
            spatialGrid->QueryNearbyCreatures(GetBot()->GetPosition(), 100.0f);

        for (auto const& snapshot : nearbyCreatures)
        {
            if (snapshot.guid == node.guid)
                return snapshot.isDead && snapshot.isSkinnable;
        }
        return false;
    }
    else
    {
        // Query nearby game objects and find matching GUID
        ::std::vector<DoubleBufferedSpatialGrid::GameObjectSnapshot> nearbyObjects =
            spatialGrid->QueryNearbyGameObjects(GetBot()->GetPosition(), 100.0f);

        for (auto const& snapshot : nearbyObjects)
        {
            if (snapshot.guid == node.guid)
                return snapshot.isSpawned && snapshot.goState == GO_STATE_READY;
        }
        return false;
    }
}

float GatheringManager::GetSkillUpChance(GatheringNode const& node) const
{
    uint16 currentSkill = GetProfessionSkill(node.nodeType);
    if (currentSkill >= node.requiredSkill + 100)
        return 0.0f;  // Grey - no skill-up
    if (currentSkill >= node.requiredSkill + 50)
        return 0.25f;  // Green - low chance
    if (currentSkill >= node.requiredSkill + 25)
        return 0.5f;  // Yellow - medium chance

    return 1.0f;  // Orange/Red - guaranteed or high chance
}

void GatheringManager::HandleGatheringResult(GatheringNode const& node, bool success)
{
    if (success)
    {
        _statistics.nodesGathered++;

        // NOTE: Skill point gains are handled by TrinityCore's spell system, not here.
        // The _statistics.skillPointsGained is updated via OnSpellCastComplete callback
        // when TrinityCore processes the skill gain event.

        // Update humanization goal progress
        // Item count is estimated as 1-3 per node (TrinityCore handles actual loot)
        // Skill point tracking is done separately via spell callbacks
        UpdateGoalProgress(2, 0, 0);
    }
    else
    {
        _statistics.failedAttempts++;
    }

    // Mark node as inactive if successfully gathered
    if (success)
    {
        // No lock needed - _detectedNodes is per-bot instance data
        auto it = ::std::find_if(_detectedNodes.begin(), _detectedNodes.end(),
            [&node](GatheringNode const& n) { return n.guid == node.guid; });

        if (it != _detectedNodes.end())
        {
            it->isActive = false;
        }
    }

    StopGathering();
}

void GatheringManager::UpdateDetectedNodes()
{
    // No lock needed - _detectedNodes is per-bot instance data
    // Clear old nodes
    _detectedNodes.clear();

    // Scan for new nodes
    auto newNodes = ScanForNodes(_detectionRange);
    _detectedNodes = ::std::move(newNodes);

    TC_LOG_DEBUG("bot.playerbot", "GatheringManager: Detected %zu gathering nodes", _detectedNodes.size());
}

GatheringNode const* GatheringManager::SelectBestNode() const
{
    // No lock needed - _detectedNodes is per-bot instance data
    GatheringNode const* bestNode = nullptr;
    float bestScore = 0.0f;

    for (auto const& node : _detectedNodes)
    {
        if (!node.isActive || !CanGatherFromNode(node))
            continue;

        // Calculate score based on distance and skill-up chance
        float score = 100.0f / (1.0f + node.distance);  // Distance factor
    if (_prioritizeSkillUps)
        {
            score *= (1.0f + GetSkillUpChance(node));  // Skill-up bonus
        }

        if (score > bestScore)
        {
            bestScore = score;
            bestNode = &node;
        }
    }

    return bestNode;
}

void GatheringManager::ProcessCurrentGathering()
{
    if (!GetBot() || !_currentTarget)
    {
        StopGathering();
        return;
    }

    // Check if gathering timed out
    auto elapsed = ::std::chrono::steady_clock::now() - _gatheringStartTime;
    if (::std::chrono::duration_cast<::std::chrono::milliseconds>(elapsed).count() > GATHERING_CAST_TIME * 3)
    {
        TC_LOG_DEBUG("bot.playerbot", "GatheringManager: Gathering timed out");
        HandleGatheringResult(*_currentTarget, false);
        return;
    }

    // Check if node is still valid
    if (!ValidateNode(*_currentTarget))
    {
        TC_LOG_DEBUG("bot.playerbot", "GatheringManager: Node no longer valid");
        HandleGatheringResult(*_currentTarget, false);
        return;
    }

    // Check if we're still in range
    if (!IsInGatheringRange(*_currentTarget))
    {
        TC_LOG_DEBUG("bot.playerbot", "GatheringManager: Moved out of gathering range");
        StopGathering();
        return;
    }

    // Check if gathering is complete (simplified - real implementation would check spell cast result)
    if (!GetBot()->IsNonMeleeSpellCast(false) && !_isLooting)
    {
        HandleGatheringResult(*_currentTarget, true);
    }
}

void GatheringManager::CleanupExpiredNodes()
{
    // No lock needed - _detectedNodes is per-bot instance data
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Remove nodes that are too old or no longer valid
    _detectedNodes.erase(
        ::std::remove_if(_detectedNodes.begin(), _detectedNodes.end(),
            [currentTime, this](GatheringNode const& node) {
                return !node.isActive ||
                       (currentTime - node.detectionTime > NODE_CACHE_DURATION) ||
                       !ValidateNode(node);
            }),
        _detectedNodes.end()
    );
}

void GatheringManager::RecordStatistics(GatheringNode const& node, bool success)
{
    if (success)
    {
        _statistics.nodesGathered++;
        _statistics.distanceTraveled += static_cast<uint32>(node.distance);
    }
    else
    {
        _statistics.failedAttempts++;
    }

    auto elapsed = ::std::chrono::steady_clock::now() - _gatheringStartTime;
    _statistics.timeSpentGathering += static_cast<uint32>(
        ::std::chrono::duration_cast<::std::chrono::milliseconds>(elapsed).count());
}

bool GatheringManager::HasProfession(GatheringNodeType nodeType) const
{
    return GetProfessionSkill(nodeType) > 0;
}

uint16 GatheringManager::GetProfessionSkill(GatheringNodeType nodeType) const
{
    if (!GetBot())
        return 0;

    uint32 skillId = 0;
    switch (nodeType)
    {
        case GatheringNodeType::MINING_VEIN:
            skillId = SKILL_MINING;
            break;
        case GatheringNodeType::HERB_NODE:
            skillId = SKILL_HERBALISM;
            break;
        case GatheringNodeType::CREATURE_CORPSE:
            skillId = SKILL_SKINNING;
            break;
        case GatheringNodeType::FISHING_POOL:
            skillId = SKILL_FISHING;
            break;
        default:
            return 0;
    }

    return GetBot()->GetSkillValue(skillId);
}

void GatheringManager::OnSpellCastComplete(Spell const* spell)
{
    if (!spell || spell->m_spellInfo->Id != _currentSpellId)
        return;

    // Gathering spell completed
    if (_currentTarget)
    {
        // Check if the spell was executed successfully (true for successful cast)
        HandleGatheringResult(*_currentTarget, true);
    }

    // NOTE: Skill point gains are handled by TrinityCore's spell/skill system.
    // To track skill gains for statistics, we would need to hook into
    // Player::UpdateSkillPro() or similar TrinityCore callbacks.
    // The _statistics.skillPointsGained field is available for tracking
    // when such hooks are implemented.
}

// ============================================================================
// HUMANIZATION SESSION IMPLEMENTATION
// ============================================================================

bool GatheringManager::StartSession(GatheringNodeType nodeType, uint32 durationMs)
{
    if (_sessionState != GatheringSessionState::INACTIVE)
    {
        TC_LOG_DEBUG("bot.playerbot", "GatheringManager: Session already active, stopping first");
        StopSession("Starting new session");
    }

    // Set up duration goal
    GatheringSessionGoal goal;
    goal.type = GatheringGoalType::DURATION;
    goal.targetValue = durationMs > 0 ? durationMs : DEFAULT_SESSION_DURATION_MS;

    return StartSessionWithGoal(nodeType, goal);
}

bool GatheringManager::StartSessionWithGoal(GatheringNodeType nodeType, GatheringSessionGoal const& goal)
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return false;

    // Check if we have the profession
    if (!HasProfession(nodeType))
    {
        TC_LOG_DEBUG("bot.playerbot", "GatheringManager: Bot does not have profession for node type %u",
            static_cast<uint32>(nodeType));
        return false;
    }

    _sessionGoal = goal;
    _sessionNodeType = nodeType;
    _sessionStartTime = ::std::chrono::steady_clock::now();
    _lastMiniBreakTime = _sessionStartTime;
    _sessionElapsedMs = 0;
    _timeSinceLastBreakMs = 0;

    // Enable the specific gathering type
    SetProfessionEnabled(nodeType, true);

    TransitionSessionState(GatheringSessionState::ACTIVE);

    TC_LOG_DEBUG("bot.playerbot", "GatheringManager: Started session for bot %s, goal type %u, target %u",
        GetBot()->GetName().c_str(), static_cast<uint32>(goal.type), goal.targetValue);

    return true;
}

bool GatheringManager::StartRouteSession(FarmingRoute const& route)
{
    if (route.IsEmpty())
    {
        TC_LOG_DEBUG("bot.playerbot", "GatheringManager: Cannot start route session - route is empty");
        return false;
    }

    // Start session with duration goal based on route
    GatheringSessionGoal goal;
    goal.type = GatheringGoalType::DURATION;
    goal.targetValue = route.estimatedLoopTimeMs * 2;  // Allow 2 loops

    if (!StartSessionWithGoal(route.primaryNodeType, goal))
        return false;

    // Set up route
    _activeRoute = route;
    _currentWaypointIndex = 0;
    _hasActiveRoute = true;

    TC_LOG_DEBUG("bot.playerbot", "GatheringManager: Started route session '%s' with %zu waypoints",
        route.routeName.c_str(), route.waypoints.size());

    return true;
}

void GatheringManager::StopSession(::std::string const& reason)
{
    if (_sessionState == GatheringSessionState::INACTIVE)
        return;

    // Stop any active gathering
    if (_isGathering.load())
    {
        StopGathering();
    }

    // Clear route
    _activeRoute = FarmingRoute();
    _currentWaypointIndex = 0;
    _hasActiveRoute = false;

    // Reset goal
    _sessionGoal.Reset();
    _sessionNodeType = GatheringNodeType::NONE;

    TransitionSessionState(GatheringSessionState::COMPLETED);

    TC_LOG_DEBUG("bot.playerbot", "GatheringManager: Session stopped for bot %s, reason: %s",
        GetBot() ? GetBot()->GetName().c_str() : "unknown",
        reason.empty() ? "none" : reason.c_str());

    // Final transition to inactive
    TransitionSessionState(GatheringSessionState::INACTIVE);
}

void GatheringManager::PauseSession()
{
    if (_sessionState != GatheringSessionState::ACTIVE)
        return;

    TransitionSessionState(GatheringSessionState::PAUSED);

    TC_LOG_DEBUG("bot.playerbot", "GatheringManager: Session paused for bot %s",
        GetBot() ? GetBot()->GetName().c_str() : "unknown");
}

void GatheringManager::ResumeSession()
{
    if (_sessionState != GatheringSessionState::PAUSED)
        return;

    TransitionSessionState(GatheringSessionState::ACTIVE);

    TC_LOG_DEBUG("bot.playerbot", "GatheringManager: Session resumed for bot %s",
        GetBot() ? GetBot()->GetName().c_str() : "unknown");
}

void GatheringManager::UpdateGoalProgress(uint32 itemCount, uint32 skillPoints, uint32 goldValue)
{
    switch (_sessionGoal.type)
    {
        case GatheringGoalType::ITEM_COUNT:
        case GatheringGoalType::SPECIFIC_ITEM:
            _sessionGoal.currentValue += itemCount;
            break;
        case GatheringGoalType::SKILL_POINTS:
            _sessionGoal.currentValue += skillPoints;
            break;
        case GatheringGoalType::GOLD_VALUE:
            _sessionGoal.currentValue += goldValue;
            break;
        case GatheringGoalType::DURATION:
            // Duration is updated in ProcessSession
            break;
        case GatheringGoalType::FILL_BAGS:
            // Check bag space
            if (GetBot())
            {
                uint32 freeSlots = GetBot()->GetFreeInventorySlotCount();
                if (freeSlots == 0)
                    _sessionGoal.currentValue = _sessionGoal.targetValue;
            }
            break;
        default:
            break;
    }

    // Check if goal is complete
    if (_sessionGoal.IsComplete())
    {
        TransitionSessionState(GatheringSessionState::COMPLETING);
    }
}

bool GatheringManager::ShouldTakeMiniBreak() const
{
    if (_sessionState != GatheringSessionState::ACTIVE)
        return false;

    // Check if enough time has passed since last break
    return _timeSinceLastBreakMs >= _miniBreakIntervalMinMs;
}

void GatheringManager::StartMiniBreak(uint32 durationMs)
{
    if (_sessionState != GatheringSessionState::ACTIVE)
        return;

    // Stop current gathering
    if (_isGathering.load())
    {
        StopGathering();
    }

    _miniBreakDurationMs = durationMs > 0 ? durationMs : CalculateMiniBreakDuration();
    _miniBreakStartTime = ::std::chrono::steady_clock::now();

    TransitionSessionState(GatheringSessionState::MINI_BREAK);

    TC_LOG_DEBUG("bot.playerbot", "GatheringManager: Bot %s taking mini-break for %u ms",
        GetBot() ? GetBot()->GetName().c_str() : "unknown", _miniBreakDurationMs);
}

uint32 GatheringManager::GetRemainingMiniBreakMs() const
{
    if (_sessionState != GatheringSessionState::MINI_BREAK)
        return 0;

    auto elapsed = ::std::chrono::steady_clock::now() - _miniBreakStartTime;
    uint32 elapsedMs = static_cast<uint32>(
        ::std::chrono::duration_cast<::std::chrono::milliseconds>(elapsed).count());

    return elapsedMs < _miniBreakDurationMs ? _miniBreakDurationMs - elapsedMs : 0;
}

void GatheringManager::SetMiniBreakInterval(uint32 minMs, uint32 maxMs)
{
    _miniBreakIntervalMinMs = minMs;
    _miniBreakIntervalMaxMs = maxMs;
}

FarmingRoute const* GatheringManager::GetCurrentRoute() const
{
    return _hasActiveRoute ? &_activeRoute : nullptr;
}

bool GatheringManager::AdvanceWaypoint()
{
    if (!_hasActiveRoute || _activeRoute.IsEmpty())
        return false;

    _currentWaypointIndex++;
    if (_currentWaypointIndex >= _activeRoute.waypoints.size())
    {
        _currentWaypointIndex = 0;  // Loop back to start
    }

    // Move to next waypoint
    if (GetAI())
    {
        FarmingWaypoint const& wp = _activeRoute.waypoints[_currentWaypointIndex];
        GetAI()->MoveTo(wp.position.GetPositionX(), wp.position.GetPositionY(), wp.position.GetPositionZ());
    }

    return true;
}

void GatheringManager::ProcessSession(uint32 elapsed)
{
    if (_sessionState == GatheringSessionState::INACTIVE)
        return;

    _sessionElapsedMs += elapsed;
    _timeSinceLastBreakMs += elapsed;

    // Update duration goal
    if (_sessionGoal.type == GatheringGoalType::DURATION)
    {
        _sessionGoal.currentValue = _sessionElapsedMs;
    }

    // Check session end conditions
    if (CheckSessionEnd())
    {
        TransitionSessionState(GatheringSessionState::COMPLETING);
        return;
    }

    // Handle different states
    switch (_sessionState)
    {
        case GatheringSessionState::ACTIVE:
            // Check for mini-break
            if (ShouldTakeMiniBreak())
            {
                // Random chance to take break
                if (rand_norm() < 0.3f)  // 30% chance when interval reached
                {
                    StartMiniBreak();
                }
            }
            break;

        case GatheringSessionState::MINI_BREAK:
            ProcessMiniBreak(elapsed);
            break;

        case GatheringSessionState::COMPLETING:
            StopSession("Goal complete");
            break;

        default:
            break;
    }
}

void GatheringManager::ProcessMiniBreak(uint32 /*elapsed*/)
{
    if (GetRemainingMiniBreakMs() == 0)
    {
        // Break is over
        _lastMiniBreakTime = ::std::chrono::steady_clock::now();
        _timeSinceLastBreakMs = 0;

        TransitionSessionState(GatheringSessionState::ACTIVE);

        TC_LOG_DEBUG("bot.playerbot", "GatheringManager: Bot %s mini-break ended, resuming gathering",
            GetBot() ? GetBot()->GetName().c_str() : "unknown");
    }
}

void GatheringManager::TransitionSessionState(GatheringSessionState newState)
{
    if (_sessionState == newState)
        return;

    GatheringSessionState oldState = _sessionState;
    _sessionState = newState;

    TC_LOG_DEBUG("bot.playerbot", "GatheringManager: Session state transition %u -> %u for bot %s",
        static_cast<uint32>(oldState), static_cast<uint32>(newState),
        GetBot() ? GetBot()->GetName().c_str() : "unknown");

    NotifySessionStateChange();
}

uint32 GatheringManager::CalculateMiniBreakDuration() const
{
    // Random duration between min and max
    uint32 range = MAX_MINI_BREAK_DURATION_MS - MIN_MINI_BREAK_DURATION_MS;
    return MIN_MINI_BREAK_DURATION_MS + (rand() % range);
}

bool GatheringManager::CheckSessionEnd() const
{
    // Check goal completion
    if (_sessionGoal.IsComplete())
        return true;

    // Check combat interruption
    if (GetBot() && GetBot()->IsInCombat())
        return false;  // Don't end, just pause

    // Check if bags are full (for any gathering)
    if (GetBot())
    {
        uint32 freeSlots = GetBot()->GetFreeInventorySlotCount();
        if (freeSlots == 0)
            return true;
    }

    return false;
}

void GatheringManager::NotifySessionStateChange()
{
    if (_sessionCallback)
    {
        _sessionCallback(_sessionState, _sessionGoal);
    }
}

} // namespace Playerbot