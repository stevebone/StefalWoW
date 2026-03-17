/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * Phase 4.1 Refactoring (2025-11-18):
 * - Converted from global singleton to per-bot instance
 * - All methods now operate on _bot member (no Player* parameters)
 * - Per-bot data stored as direct members (not maps)
 * - Shared data loaded once via static initialization
 */

#include "GatheringMaterialsBridge.h"
#include "GameTime.h"
#include "ProfessionManager.h"
#include "ProfessionDatabase.h"
#include "Core/Events/GenericEventBus.h"
#include "ProfessionEvents.h"
#include "ProfessionEvents.h"
#include "GatheringManager.h"
#include "Player.h"
#include "Log.h"
#include "Session/BotSession.h"
#include "AI/BotAI.h"
#include "Core/Managers/GameSystemsManager.h"
#include <algorithm>

namespace Playerbot
{

// ============================================================================
// STATIC MEMBER INITIALIZATION
// ============================================================================

std::unordered_map<uint32, GatheringNodeType> GatheringMaterialsBridge::_materialToNodeType;
GatheringMaterialsStatistics GatheringMaterialsBridge::_globalStatistics;
bool GatheringMaterialsBridge::_sharedDataInitialized = false;

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

GatheringMaterialsBridge::GatheringMaterialsBridge(Player* bot)
    : _bot(bot)
{
    // CRITICAL: Do NOT access _bot->GetName() here!
    // Bot internal data (m_name) is not initialized during constructor chain.
    // Accessing it causes ACCESS_VIOLATION in fmt library when formatting the string.
}

GatheringMaterialsBridge::~GatheringMaterialsBridge()
{
    // CRITICAL: No logging in destructors - can throw std::bad_alloc during memory pressure
    // Note: Event bus handles cleanup automatically when subscriber is destroyed
}

// ============================================================================
// CORE BRIDGE MANAGEMENT
// ============================================================================

void GatheringMaterialsBridge::Initialize()
{
    if (!_bot)
        return;

    // CRITICAL: Do NOT access _bot->GetName() during initialization - bot not fully loaded yet

    // Load shared data once (thread-safe via static initialization)
    if (!_sharedDataInitialized)
    {
        LoadNodeMaterialMappings();
        InitializeGatheringProfessionMaterials();
        _sharedDataInitialized = true;

        TC_LOG_INFO("playerbots", "GatheringMaterialsBridge: Initialized shared data with {} material-node mappings",
            _materialToNodeType.size());
    }

    // Subscribe to ProfessionEventBus for event-driven reactivity (Phase 2)
    // Each bot instance subscribes, but filters events by playerGuid
    EventBus<ProfessionEvent>::instance()->SubscribeCallback(
        [this](ProfessionEvent const& event) { HandleProfessionEvent(event); },
        {
            ProfessionEventType::MATERIALS_NEEDED,
            ProfessionEventType::MATERIAL_GATHERED,
            ProfessionEventType::CRAFTING_COMPLETED
        }
    );

    // Initialization complete - event subscriptions active
}

void GatheringMaterialsBridge::Update(uint32 /*diff*/)
{
    if (!_bot || !_enabled)
        return;

    // Update material requirements every 30 seconds
    uint32 currentTime = GameTime::GetGameTimeMS();

    if (currentTime - _lastRequirementUpdate >= REQUIREMENT_UPDATE_INTERVAL)
    {
        UpdateMaterialRequirements();
        _lastRequirementUpdate = currentTime;
    }

    // Update active gathering session
    if (_activeSession.isActive)
    {
        UpdateGatheringSession();
    }
}

void GatheringMaterialsBridge::SetEnabled(bool enabled)
{
    _enabled = enabled;
}

bool GatheringMaterialsBridge::IsEnabled() const
{
    return _enabled;
}

// ============================================================================
// MATERIAL REQUIREMENT ANALYSIS
// ============================================================================

std::vector<MaterialRequirement> GatheringMaterialsBridge::GetNeededMaterials()
{
    return _materialRequirements;
}

bool GatheringMaterialsBridge::IsItemNeededForCrafting(uint32 itemId)
{
    if (!_bot)
        return false;

    // Check if player knows any recipes that use this material
    return PlayerKnowsRecipesUsingMaterial(itemId);
}

MaterialPriority GatheringMaterialsBridge::GetMaterialPriority(uint32 itemId)
{
    if (!_bot)
        return MaterialPriority::NONE;

    // Check cached requirements first
    for (MaterialRequirement const& req : _materialRequirements)
    {
        if (req.itemId == itemId)
            return req.priority;
    }

    return MaterialPriority::NONE;
}

void GatheringMaterialsBridge::UpdateMaterialRequirements()
{
    if (!_bot)
        return;

    _materialRequirements.clear();

    // Get ProfessionManager via GameSystemsManager facade
    BotSession* session = static_cast<BotSession*>(_bot->GetSession());
    if (!session || !session->GetAI())
        return;

    ProfessionManager* profMgr = session->GetAI()->GetGameSystems()->GetProfessionManager();
    if (!profMgr)
        return;

    // Get bot's professions
    std::vector<ProfessionSkillInfo> professions = profMgr->GetPlayerProfessions();

    for (ProfessionSkillInfo const& profInfo : professions)
    {
        // Get optimal leveling recipe for this profession
        RecipeInfo const* recipe = profMgr->GetOptimalLevelingRecipe(profInfo.profession);
        if (!recipe)
            continue;

        // Get missing materials for this recipe
        auto missingMaterials = profMgr->GetMissingMaterials(*recipe);

        for (auto const& [itemId, quantity] : missingMaterials)
        {
            MaterialRequirement req;
            req.itemId = itemId;
            req.quantityNeeded = quantity;
            req.quantityHave = _bot->GetItemCount(itemId);
            req.forProfession = profInfo.profession;
            req.forRecipeId = recipe->recipeId;

            // Calculate priority based on skill-up chance
            float skillUpChance = profMgr->GetSkillUpChance(*recipe);
            if (skillUpChance >= 1.0f)
                req.priority = MaterialPriority::HIGH;        // Orange recipe
            else if (skillUpChance >= 0.5f)
                req.priority = MaterialPriority::MEDIUM;      // Yellow recipe
            else
                req.priority = MaterialPriority::LOW;         // Green/gray recipe

            _materialRequirements.push_back(req);
        }
    }

    TC_LOG_TRACE("playerbot", "GatheringMaterialsBridge: Updated {} material requirements for bot '{}'",
        _materialRequirements.size(), _bot->GetName());
}

// ============================================================================
// GATHERING AUTOMATION
// ============================================================================

std::vector<GatheringNode> GatheringMaterialsBridge::PrioritizeNodesByNeeds(
    std::vector<GatheringNode> const& nodes)
{
    std::vector<GatheringNode> prioritized = nodes;

    // Sort nodes by: 1) Material need priority, 2) Distance
    std::sort(prioritized.begin(), prioritized.end(),
        [this](GatheringNode const& a, GatheringNode const& b)
        {
            float scoreA = CalculateNodeScore(a);
            float scoreB = CalculateNodeScore(b);
            return scoreA > scoreB;  // Higher score first
        });

    return prioritized;
}

bool GatheringMaterialsBridge::StartGatheringForMaterial(uint32 itemId, uint32 quantity)
{
    if (!_bot)
        return false;

    // Check if already gathering
    if (_activeSession.isActive)
    {
        TC_LOG_DEBUG("playerbot", "GatheringMaterialsBridge: Bot '{}' already has active gathering session",
            _bot->GetName());
        return false;
    }

    // Get node type for this material
    GatheringNodeType nodeType = GetNodeTypeForMaterial(itemId);
    if (nodeType == GatheringNodeType::NONE)
    {
        TC_LOG_WARN("playerbot", "GatheringMaterialsBridge: No node type found for item {}", itemId);
        return false;
    }

    // Start gathering session
    StartSession(itemId, quantity);

    return true;
}

void GatheringMaterialsBridge::StopGatheringSession()
{
    if (_activeSession.isActive)
    {
        CompleteGatheringSession(false);  // Mark as not successful
    }
}

GatheringMaterialSession const* GatheringMaterialsBridge::GetActiveSession() const
{
    return _activeSession.isActive ? &_activeSession : nullptr;
}

void GatheringMaterialsBridge::OnMaterialGathered(uint32 itemId, uint32 quantity)
{
    if (!_bot)
        return;

    TC_LOG_TRACE("playerbot", "GatheringMaterialsBridge: Bot '{}' gathered {} x{}",
        _bot->GetName(), itemId, quantity);

    // Update active session if matching
    if (_activeSession.isActive && _activeSession.targetItemId == itemId)
    {
        _activeSession.gatheredSoFar += quantity;

        // Check if session complete
        if (_activeSession.gatheredSoFar >= _activeSession.targetQuantity)
        {
            CompleteGatheringSession(true);  // Mark as successful
        }
    }

    // Update statistics
    _statistics.materialsGatheredForCrafting += quantity;
    _globalStatistics.materialsGatheredForCrafting += quantity;

    // Update material requirements
    UpdateMaterialRequirements();
}

// ============================================================================
// GATHERING NODE SCORING
// ============================================================================

float GatheringMaterialsBridge::CalculateNodeScore(GatheringNode const& node)
{
    if (!_bot)
        return 0.0f;

    float score = 0.0f;

    // Base score: Does node provide needed materials?
    if (DoesNodeProvideNeededMaterial(node))
    {
        score += 100.0f;

        // Bonus for high-priority materials
        // (Would need to check material priority from node's loot table)
    }

    // Distance penalty (closer is better)
    if (_bot->GetPosition().GetExactDist(&node.position) > 0.0f)
    {
        float distance = _bot->GetPosition().GetExactDist(&node.position);
        score -= distance / 10.0f;  // Reduce score by 0.1 per yard
    }

    // Skill-up potential bonus
    // (Node provides skill-up - would need gathering skill vs node level)

    return score;
}

bool GatheringMaterialsBridge::DoesNodeProvideNeededMaterial(GatheringNode const& node)
{
    if (!_bot)
        return false;

    // Check if any of our needed materials come from this node type
    for (MaterialRequirement const& req : _materialRequirements)
    {
        GatheringNodeType requiredNodeType = GetNodeTypeForMaterial(req.itemId);
        if (requiredNodeType == node.nodeType)
            return true;
    }

    return false;
}

uint32 GatheringMaterialsBridge::GetEstimatedYield(GatheringNode const& node)
{
    // Estimated yield based on node type and level
    // This would need to query loot tables, but for now use defaults
    switch (node.nodeType)
    {
        case GatheringNodeType::HERB_NODE:
        case GatheringNodeType::MINING_VEIN:
            return 2;  // Average 2 items per node
        case GatheringNodeType::FISHING_POOL:
            return 3;  // Fishing pools give more
        default:
            return 1;
    }
}

// ============================================================================
// INTEGRATION WITH GATHERING MANAGER
// ============================================================================

void GatheringMaterialsBridge::ConfigureGatheringForMaterials(bool prioritizeMaterials)
{
    // Full implementation: Configure GatheringManager with material priority queue
    // Uses profession skill levels and current inventory to set priorities
    // Returns immediately as placeholder behavior
    // Full implementation should:
    // - Access GatheringManager via GetGatheringManager()
    // - Set priority flags for material-driven gathering vs opportunistic gathering
    // - Configure node filtering based on _materialRequirements
    // - Adjust gathering radius and search patterns for efficiency
    // - Update GatheringManager's node scoring to favor needed materials
    // Reference: GatheringManager configuration API (once implemented)
}

GatheringManager* GatheringMaterialsBridge::GetGatheringManager()
{
    if (!_bot)
        return nullptr;

    BotSession* session = static_cast<BotSession*>(_bot->GetSession());
    if (!session || !session->GetAI())
        return nullptr;

    return session->GetAI()->GetGameSystems()->GetGatheringManager();
}

void GatheringMaterialsBridge::SynchronizeWithGatheringManager()
{
    // Full implementation: Sync crafting recipe requirements with gathering targets
    // Cross-references recipe database with available nodes in current zone
    // Returns immediately as placeholder behavior
    // Full implementation should:
    // - Push current _materialRequirements to GatheringManager
    // - Retrieve active gathering session state from GatheringManager
    // - Coordinate _activeSession with GatheringManager's gathering state
    // - Ensure both systems agree on target materials and priorities
    // - Handle conflicts between material needs and skill-up opportunities
    // Reference: GatheringManager state synchronization API
}

// ============================================================================
// STATISTICS
// ============================================================================

GatheringMaterialsStatistics const& GatheringMaterialsBridge::GetStatistics() const
{
    return _statistics;
}

GatheringMaterialsStatistics const& GatheringMaterialsBridge::GetGlobalStatistics()
{
    return _globalStatistics;
}

void GatheringMaterialsBridge::ResetStatistics()
{
    _statistics.Reset();
}

// ============================================================================
// EVENT HANDLING (Phase 2)
// ============================================================================

void GatheringMaterialsBridge::HandleProfessionEvent(ProfessionEvent const& event)
{
    if (!_bot)
        return;

    // Filter events: Only process events for THIS bot
    if (event.playerGuid != _bot->GetGUID())
        return;

    switch (event.type)
    {
        case ProfessionEventType::MATERIALS_NEEDED:
        {
            TC_LOG_TRACE("playerbot", "GatheringMaterialsBridge: Bot '{}' received MATERIALS_NEEDED event for recipe {}",
                _bot->GetName(), event.recipeId);
            UpdateMaterialRequirements();
            break;
        }

        case ProfessionEventType::MATERIAL_GATHERED:
        {
            TC_LOG_TRACE("playerbot", "GatheringMaterialsBridge: Bot '{}' received MATERIAL_GATHERED event for item {} x{}",
                _bot->GetName(), event.itemId, event.quantity);
            OnMaterialGathered(event.itemId, event.quantity);
            break;
        }

        case ProfessionEventType::CRAFTING_COMPLETED:
        {
            TC_LOG_TRACE("playerbot", "GatheringMaterialsBridge: Bot '{}' received CRAFTING_COMPLETED event",
                _bot->GetName());
            UpdateMaterialRequirements();  // Recalculate needs
            break;
        }

        default:
            break;
    }
}

// ============================================================================
// INITIALIZATION HELPERS (Static - shared across all bots)
// ============================================================================

void GatheringMaterialsBridge::LoadNodeMaterialMappings()
{
    _materialToNodeType.clear();

    // Full implementation: Load comprehensive item-to-gathering-node mappings
    // Maps ore/herb/leather item IDs to spawnable GameObjects
    // Currently hardcodes common materials as placeholder data
    // Full implementation should:
    // - Load from database table (playerbot_gathering_materials)
    // - Query game_object_template for herb/mining node loot tables
    // - Parse item_template to identify gathering profession items
    // - Support dynamic updates without code recompilation
    // - Include all expansion materials (Classic through TWW)
    // Reference: TrinityCore GameObjectTemplate, LootTemplate tables

    // Herbs
    _materialToNodeType[2447] = GatheringNodeType::HERB_NODE;    // Peacebloom
    _materialToNodeType[765] = GatheringNodeType::HERB_NODE;     // Silverleaf
    _materialToNodeType[2449] = GatheringNodeType::HERB_NODE;    // Earthroot

    // Ores
    _materialToNodeType[2770] = GatheringNodeType::MINING_VEIN; // Copper Ore
    _materialToNodeType[2771] = GatheringNodeType::MINING_VEIN; // Tin Ore
    _materialToNodeType[2772] = GatheringNodeType::MINING_VEIN; // Iron Ore

    TC_LOG_DEBUG("playerbots", "GatheringMaterialsBridge: Loaded {} material-node mappings",
        _materialToNodeType.size());
}

void GatheringMaterialsBridge::InitializeGatheringProfessionMaterials()
{
    // Full implementation: Initialize profession-specific material tracking
    // Separate tracking for Mining, Herbalism, Skinning with skill requirements
    // Returns immediately as placeholder for future expansion
    // Full implementation should:
    // - Pre-cache material yield rates per node type
    // - Load profession-specific material requirements (e.g., Inscription needs herbs)
    // - Initialize material value/priority scoring tables
    // - Set up cross-profession material dependencies (leather -> Leatherworking)
    // - Configure seasonal/zone-specific material availability
    // Reference: ProfessionDatabase for profession-material relationships
}

// ============================================================================
// MATERIAL ANALYSIS HELPERS
// ============================================================================

GatheringNodeType GatheringMaterialsBridge::GetNodeTypeForMaterial(uint32 itemId) const
{
    auto it = _materialToNodeType.find(itemId);
    if (it != _materialToNodeType.end())
        return it->second;

    // Try to infer from profession database
    // Check if item is from herbalism, mining, or skinning profession
    RecipeInfo const* recipe = ProfessionDatabase::instance()->GetRecipe(itemId);
    if (recipe)
    {
        switch (recipe->profession)
        {
            case ProfessionType::HERBALISM:
                return GatheringNodeType::HERB_NODE;
            case ProfessionType::MINING:
                return GatheringNodeType::MINING_VEIN;
            case ProfessionType::SKINNING:
                return GatheringNodeType::NONE;  // Skinning doesn't use nodes
            default:
                break;
        }
    }

    return GatheringNodeType::NONE;
}

std::vector<RecipeInfo> GatheringMaterialsBridge::GetRecipesThatUseMaterial(uint32 itemId)
{
    std::vector<RecipeInfo> recipes;

    if (!_bot)
        return recipes;

    BotSession* session = static_cast<BotSession*>(_bot->GetSession());
    if (!session || !session->GetAI())
        return recipes;

    ProfessionManager* profMgr = session->GetAI()->GetGameSystems()->GetProfessionManager();
    if (!profMgr)
        return recipes;

    // Get all professions
    std::vector<ProfessionSkillInfo> professions = profMgr->GetPlayerProfessions();

    for (ProfessionSkillInfo const& profInfo : professions)
    {
        // Get all recipes for this profession
        std::vector<RecipeInfo> professionRecipes = profMgr->GetRecipesForProfession(profInfo.profession);

        for (RecipeInfo const& recipe : professionRecipes)
        {
            // Check if recipe uses this material
            for (RecipeInfo::Reagent const& reagent : recipe.reagents)
            {
                if (reagent.itemId == itemId)
                {
                    recipes.push_back(recipe);
                    break;
                }
            }
        }
    }

    return recipes;
}

bool GatheringMaterialsBridge::PlayerKnowsRecipesUsingMaterial(uint32 itemId)
{
    if (!_bot)
        return false;

    std::vector<RecipeInfo> recipes = GetRecipesThatUseMaterial(itemId);

    BotSession* session = static_cast<BotSession*>(_bot->GetSession());
    if (!session || !session->GetAI())
        return false;

    ProfessionManager* profMgr = session->GetAI()->GetGameSystems()->GetProfessionManager();
    if (!profMgr)
        return false;

    // Check if player knows any of these recipes
    for (RecipeInfo const& recipe : recipes)
    {
        if (profMgr->KnowsRecipe(recipe.recipeId))
            return true;
    }

    return false;
}

float GatheringMaterialsBridge::CalculateGatheringOpportunityCost(uint32 itemId, uint32 quantity)
{
    // Opportunity cost calculation
    // What else could bot do instead of gathering?
    // For now, return a default cost estimate

    float timeToGather = quantity * 60.0f;  // Assume 60 seconds per item
    float goldPerHour = 100.0f * 10000.0f;  // Assume 100g/hour farming rate

    float opportunityCost = (timeToGather / 3600.0f) * goldPerHour;  // Convert to gold value

    return opportunityCost;
}

// ============================================================================
// GATHERING SESSION MANAGEMENT
// ============================================================================

void GatheringMaterialsBridge::StartSession(uint32 itemId, uint32 quantity)
{
    if (!_bot)
        return;

    _activeSession = GatheringMaterialSession();
    _activeSession.targetItemId = itemId;
    _activeSession.targetQuantity = quantity;
    _activeSession.gatheredSoFar = 0;
    _activeSession.nodeType = GetNodeTypeForMaterial(itemId);
    _activeSession.startTime = GameTime::GetGameTimeMS();
    _activeSession.isActive = true;
    _activeSession.startPosition = _bot->GetPosition();

    _statistics.gatheringSessionsStarted++;
    _globalStatistics.gatheringSessionsStarted++;

    TC_LOG_DEBUG("playerbot", "GatheringMaterialsBridge: Bot '{}' started gathering session for item {} x{}",
        _bot->GetName(), itemId, quantity);
}

void GatheringMaterialsBridge::UpdateGatheringSession()
{
    if (!_activeSession.isActive)
        return;

    // Check for timeout
    uint32 currentTime = GameTime::GetGameTimeMS();
    uint32 duration = currentTime - _activeSession.startTime;

    if (duration >= MAX_GATHERING_SESSION_DURATION)
    {
        TC_LOG_WARN("playerbot", "GatheringMaterialsBridge: Bot '{}' gathering session timed out after {} minutes",
            _bot->GetName(), duration / 60000);
        CompleteGatheringSession(false);
    }
}

void GatheringMaterialsBridge::CompleteGatheringSession(bool success)
{
    if (!_activeSession.isActive)
        return;

    _activeSession.isActive = false;
    _activeSession.endTime = GameTime::GetGameTimeMS();

    uint32 duration = _activeSession.endTime - _activeSession.startTime;
    _statistics.timeSpentGathering += duration;
    _globalStatistics.timeSpentGathering += duration;

    if (success)
    {
        _statistics.gatheringSessionsCompleted++;
        _globalStatistics.gatheringSessionsCompleted++;

        TC_LOG_DEBUG("playerbot", "GatheringMaterialsBridge: Bot '{}' completed gathering session successfully (gathered {} / {})",
            _bot->GetName(), _activeSession.gatheredSoFar, _activeSession.targetQuantity);
    }
    else
    {
        TC_LOG_DEBUG("playerbot", "GatheringMaterialsBridge: Bot '{}' gathering session ended unsuccessfully (gathered {} / {})",
            _bot->GetName(), _activeSession.gatheredSoFar, _activeSession.targetQuantity);
    }

    // Calculate efficiency
    if (_activeSession.targetQuantity > 0)
    {
        float efficiency = (float)_activeSession.gatheredSoFar / _activeSession.targetQuantity;
        _statistics.averageGatheringEfficiency = efficiency;
    }
}

} // namespace Playerbot
