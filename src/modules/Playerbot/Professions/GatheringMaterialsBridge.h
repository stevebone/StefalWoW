/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * GATHERING MATERIALS BRIDGE FOR PLAYERBOT
 *
 * This system bridges gathering automation with profession crafting needs:
 * - Analyzes crafting queue to determine material requirements
 * - Prioritizes gathering nodes based on what materials are needed
 * - Triggers gathering sessions for specific materials
 * - Tracks gathered materials vs crafting needs
 * - Coordinates with GatheringManager for node selection
 *
 * Integration Points:
 * - Uses ProfessionManager to determine crafting needs
 * - Uses GatheringManager for resource harvesting
 * - Coordinates with ProfessionAuctionBridge for material sourcing decisions
 * - Subscribes to ProfessionEventBus for event-driven reactivity (Phase 2)
 *
 * Design Pattern: Bridge Pattern
 * - Decouples gathering logic from crafting logic
 * - All gathering operations delegated to GatheringManager
 * - This class only manages gathering-crafting coordination
 *
 * Event Integration (Phase 2 - 2025-11-18):
 * - MATERIALS_NEEDED → Trigger gathering session for needed materials
 * - MATERIAL_GATHERED → Update fulfillment tracking and check completion
 * - CRAFTING_COMPLETED → Recalculate material needs after crafting
 *
 * Phase 4.1 Refactoring (2025-11-18):
 * - Converted from global singleton to per-bot instance
 * - Integrated into GameSystemsManager facade (18→19 managers)
 * - All methods now operate on _bot member (no Player* parameters)
 * - Per-bot data stored as direct members (not maps)
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Player.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"
#include "ProfessionManager.h"
#include "../Professions/GatheringManager.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace Playerbot
{

/**
 * @brief Material requirement priority
 */
enum class MaterialPriority : uint8
{
    NONE = 0,
    LOW = 1,           // Nice to have
    MEDIUM = 2,        // Needed for leveling
    HIGH = 3,          // Blocking crafting queue
    CRITICAL = 4       // Needed immediately for active craft
};

/**
 * @brief Material requirement entry
 */
struct MaterialRequirement
{
    uint32 itemId;
    uint32 quantityNeeded;
    uint32 quantityHave;
    uint32 quantityGathering;      // Currently being gathered
    MaterialPriority priority;
    ProfessionType forProfession;
    uint32 forRecipeId;
    uint32 addedTime;

    MaterialRequirement()
        : itemId(0), quantityNeeded(0), quantityHave(0), quantityGathering(0)
        , priority(MaterialPriority::NONE), forProfession(ProfessionType::NONE)
        , forRecipeId(0), addedTime(GameTime::GetGameTimeMS()) {}

    uint32 GetRemainingNeeded() const { return quantityNeeded > quantityHave ? quantityNeeded - quantityHave : 0; }
    bool IsFulfilled() const { return quantityHave >= quantityNeeded; }
    float GetFulfillmentPercent() const { return quantityNeeded > 0 ? (float)quantityHave / quantityNeeded : 1.0f; }
};

/**
 * @brief Gathering session for specific material
 */
struct GatheringMaterialSession
{
    uint32 targetItemId;
    uint32 targetQuantity;
    uint32 gatheredSoFar;
    GatheringNodeType nodeType;
    uint32 startTime;
    uint32 endTime;              // 0 if still active
    bool isActive;
    Position startPosition;      // Where gathering started
    std::vector<Position> nodesVisited;

    GatheringMaterialSession()
        : targetItemId(0), targetQuantity(0)
        , gatheredSoFar(0), nodeType(GatheringNodeType::NONE)
        , startTime(GameTime::GetGameTimeMS()), endTime(0), isActive(false) {}

    float GetProgress() const { return targetQuantity > 0 ? (float)gatheredSoFar / targetQuantity : 0.0f; }
    uint32 GetDuration() const { return isActive ? (GameTime::GetGameTimeMS() - startTime) : (endTime - startTime); }
};

/**
 * @brief Statistics for gathering-crafting coordination
 */
struct GatheringMaterialsStatistics
{
    std::atomic<uint32> materialsGatheredForCrafting{0};
    std::atomic<uint32> gatheringSessionsStarted{0};
    std::atomic<uint32> gatheringSessionsCompleted{0};
    std::atomic<uint32> recipesUnblockedByGathering{0};
    std::atomic<uint32> timeSpentGathering{0};  // Milliseconds
    std::atomic<float> averageGatheringEfficiency{1.0f};

    void Reset()
    {
        materialsGatheredForCrafting = 0;
        gatheringSessionsStarted = 0;
        gatheringSessionsCompleted = 0;
        recipesUnblockedByGathering = 0;
        timeSpentGathering = 0;
        averageGatheringEfficiency = 1.0f;
    }

    float GetCompletionRate() const
    {
        uint32 started = gatheringSessionsStarted.load();
        uint32 completed = gatheringSessionsCompleted.load();
        return started > 0 ? (float)completed / started : 0.0f;
    }
};

/**
 * @brief Bridge between gathering and profession crafting (per-bot instance)
 *
 * DESIGN PRINCIPLE: This class does NOT implement gathering operations.
 * All gathering operations are delegated to GatheringManager.
 * This class only coordinates gathering-crafting logic.
 *
 * PHASE 4.1: Converted to per-bot instance pattern (owned by GameSystemsManager)
 */
class TC_GAME_API GatheringMaterialsBridge final
{
public:
    /**
     * @brief Construct bridge for specific bot
     * @param bot The bot this bridge serves (non-owning)
     */
    explicit GatheringMaterialsBridge(Player* bot);

    /**
     * @brief Destructor - unsubscribes from event bus
     */
    ~GatheringMaterialsBridge();

    // ========================================================================
    // CORE BRIDGE MANAGEMENT
    // ========================================================================

    /**
     * Initialize gathering-crafting bridge (loads shared data)
     * NOTE: Called per-bot, but loads shared world data only once
     */
    void Initialize();

    /**
     * Update gathering-crafting coordination (called periodically)
     */
    void Update(uint32 diff);

    /**
     * Enable/disable gathering-crafting automation
     */
    void SetEnabled(bool enabled);
    bool IsEnabled() const;

    // ========================================================================
    // MATERIAL REQUIREMENT ANALYSIS
    // ========================================================================

    /**
     * Get materials needed for bot's current crafting queue
     * Queries ProfessionManager for queued recipes and missing materials
     */
    std::vector<MaterialRequirement> GetNeededMaterials();

    /**
     * Check if gathered item is useful for bot's professions
     * Returns true if item is needed for any known recipe
     */
    bool IsItemNeededForCrafting(uint32 itemId);

    /**
     * Get priority level for specific material
     * Based on crafting queue urgency and stockpile levels
     */
    MaterialPriority GetMaterialPriority(uint32 itemId);

    /**
     * Update material requirements based on crafting queue changes
     */
    void UpdateMaterialRequirements();

    // ========================================================================
    // GATHERING AUTOMATION
    // ========================================================================

    /**
     * Prioritize gathering nodes based on material needs
     * Sorts nodes by: priority, distance, skill-up potential
     */
    std::vector<GatheringNode> PrioritizeNodesByNeeds(
        std::vector<GatheringNode> const& nodes);

    /**
     * Trigger gathering session for specific material
     * Returns true if gathering session started successfully
     */
    bool StartGatheringForMaterial(uint32 itemId, uint32 quantity);

    /**
     * Stop current gathering session
     */
    void StopGatheringSession();

    /**
     * Get active gathering session
     */
    GatheringMaterialSession const* GetActiveSession() const;

    /**
     * Handle material gathered event
     * Called when GatheringManager completes a gathering action
     */
    void OnMaterialGathered(uint32 itemId, uint32 quantity);

    // ========================================================================
    // GATHERING NODE SCORING
    // ========================================================================

    /**
     * Calculate score for gathering node based on material needs
     * Higher score = higher priority
     */
    float CalculateNodeScore(GatheringNode const& node);

    /**
     * Check if node provides materials we need
     */
    bool DoesNodeProvideNeededMaterial(GatheringNode const& node);

    /**
     * Get estimated material yield from node
     */
    uint32 GetEstimatedYield(GatheringNode const& node);

    // ========================================================================
    // INTEGRATION WITH GATHERING MANAGER
    // ========================================================================

    /**
     * Configure GatheringManager to prioritize materials over skill-ups
     */
    void ConfigureGatheringForMaterials(bool prioritizeMaterials);

    /**
     * Get GatheringManager instance (via GameSystemsManager facade)
     */
    GatheringManager* GetGatheringManager();

    /**
     * Synchronize with gathering manager state
     */
    void SynchronizeWithGatheringManager();

    // ========================================================================
    // STATISTICS
    // ========================================================================

    GatheringMaterialsStatistics const& GetStatistics() const;
    static GatheringMaterialsStatistics const& GetGlobalStatistics();
    void ResetStatistics();

private:
    // ========================================================================
    // EVENT HANDLING (Phase 2)
    // ========================================================================

    /**
     * @brief Handle profession events from ProfessionEventBus
     * Reacts to MATERIALS_NEEDED, MATERIAL_GATHERED, CRAFTING_COMPLETED
     * Filters events by checking playerGuid == _bot->GetGUID()
     */
    void HandleProfessionEvent(struct ProfessionEvent const& event);

    // ========================================================================
    // INITIALIZATION HELPERS
    // ========================================================================

    static void LoadNodeMaterialMappings();
    static void InitializeGatheringProfessionMaterials();

    // ========================================================================
    // MATERIAL ANALYSIS HELPERS
    // ========================================================================

    /**
     * Map item ID to node type that provides it
     */
    GatheringNodeType GetNodeTypeForMaterial(uint32 itemId) const;

    /**
     * Get all recipes that use this material
     */
    std::vector<RecipeInfo> GetRecipesThatUseMaterial(uint32 itemId);

    /**
     * Check if player knows any recipes that use this material
     */
    bool PlayerKnowsRecipesUsingMaterial(uint32 itemId);

    /**
     * Calculate opportunity cost of gathering vs buying
     */
    float CalculateGatheringOpportunityCost(uint32 itemId, uint32 quantity);

    // ========================================================================
    // GATHERING SESSION MANAGEMENT
    // ========================================================================

    void StartSession(uint32 itemId, uint32 quantity);
    void UpdateGatheringSession();
    void CompleteGatheringSession(bool success);

    // ========================================================================
    // PER-BOT DATA MEMBERS (Phase 4.1: Converted from maps)
    // ========================================================================

    Player* _bot;                                       // Bot player reference (non-owning)
    bool _enabled{false};                               // Automation enabled state
    std::vector<MaterialRequirement> _materialRequirements;  // Current material needs
    GatheringMaterialSession _activeSession;            // Active gathering session (if any)
    GatheringMaterialsStatistics _statistics;           // Per-bot statistics
    uint32 _lastRequirementUpdate{0};                   // Throttle requirement updates

    // ========================================================================
    // SHARED DATA MEMBERS (Static - shared across all bots)
    // ========================================================================

    // Node type to material mapping (itemId -> node type) - world data
    static std::unordered_map<uint32, GatheringNodeType> _materialToNodeType;
    static GatheringMaterialsStatistics _globalStatistics;
    static bool _sharedDataInitialized;

    // Update intervals
    static constexpr uint32 REQUIREMENT_UPDATE_INTERVAL = 30000;   // 30 seconds
    static constexpr uint32 SESSION_CHECK_INTERVAL = 5000;         // 5 seconds
    static constexpr uint32 MAX_GATHERING_SESSION_DURATION = 1800000; // 30 minutes

    // Non-copyable
    GatheringMaterialsBridge(GatheringMaterialsBridge const&) = delete;
    GatheringMaterialsBridge& operator=(GatheringMaterialsBridge const&) = delete;
};

} // namespace Playerbot
