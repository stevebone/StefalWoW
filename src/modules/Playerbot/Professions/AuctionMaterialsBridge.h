/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * AUCTION MATERIALS BRIDGE FOR PLAYERBOT
 *
 * This system provides intelligent material sourcing decisions:
 * - Analyzes whether gathering or buying materials is more efficient
 * - Performs time-value economic analysis for bot activities
 * - Calculates opportunity costs (gathering vs crafting/selling)
 * - Coordinates material acquisition across gathering and auction systems
 * - Optimizes gold expenditure vs time investment
 *
 * Integration Points:
 * - Uses GatheringMaterialsBridge to estimate gathering time/difficulty
 * - Uses ProfessionAuctionBridge for auction market prices
 * - Uses ProfessionManager to determine crafting value
 * - Coordinates with GatheringManager for gathering feasibility
 * - Subscribes to ProfessionEventBus for event-driven reactivity (Phase 2)
 *
 * Design Pattern: Bridge + Strategy Pattern
 * - Bridges auction and gathering systems with intelligent decision-making
 * - Strategy pattern for different economic models (time-value, opportunity cost)
 * - All decisions based on configurable economic parameters
 *
 * Event Integration (Phase 2 - 2025-11-18):
 * - MATERIALS_NEEDED → Analyze AH prices and recommend buy vs gather
 * - MATERIAL_PURCHASED → Track spending and update economic metrics
 *
 * Phase 4.2 Refactoring (2025-11-18):
 * - Converted from global singleton to per-bot instance
 * - Integrated into GameSystemsManager facade (19→20 managers)
 * - All methods now operate on _bot member (no Player* parameters)
 * - Per-bot data stored as direct members (not maps)
 * - Event filtering by playerGuid for per-bot event handling
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Player.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"
#include "ProfessionManager.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace Playerbot
{

// Forward declarations
class GatheringMaterialsBridge;
class ProfessionAuctionBridge;

/**
 * @brief Material sourcing strategy
 */
enum class MaterialSourcingStrategy : uint8
{
    NONE = 0,
    ALWAYS_GATHER,          // Always gather materials (free-to-play approach)
    ALWAYS_BUY,             // Always buy materials (time-saver approach)
    COST_OPTIMIZED,         // Buy if cheaper than gathering time value
    TIME_OPTIMIZED,         // Always choose fastest method
    PROFIT_MAXIMIZED,       // Choose method that maximizes net profit
    BALANCED,               // Balance between time and cost
    HYBRID                  // Mix of gathering and buying based on availability
};

/**
 * @brief Material acquisition method
 */
enum class MaterialAcquisitionMethod : uint8
{
    NONE = 0,
    GATHER,                 // Gather from nodes
    BUY_AUCTION,            // Buy from auction house
    CRAFT,                  // Craft from other materials
    VENDOR,                 // Buy from vendor
    QUEST_REWARD,           // Obtain via quest
    FARM_MOBS,              // Farm from creature drops
    HYBRID_GATHER_BUY       // Gather some, buy remainder
};

/**
 * @brief Material sourcing decision
 */
struct MaterialSourcingDecision
{
    uint32 itemId;
    uint32 quantityNeeded;
    MaterialAcquisitionMethod recommendedMethod;
    MaterialAcquisitionMethod alternativeMethod;

    // Economic analysis
    uint32 gatheringTimeCost;       // Value of time spent gathering (copper)
    uint32 auctionCost;             // Cost to buy from AH (copper)
    uint32 craftingCost;            // Cost to craft (materials + time)
    uint32 vendorCost;              // Cost to buy from vendor (if available)

    // Time analysis
    uint32 gatheringTimeEstimate;   // Estimated seconds to gather
    uint32 auctionTimeEstimate;     // Estimated seconds to buy from AH
    uint32 craftingTimeEstimate;    // Estimated seconds to craft

    // Feasibility
    bool canGather;                 // Has gathering profession
    bool canBuyAuction;             // Available on AH at reasonable price
    bool canCraft;                  // Has recipe and materials
    bool canBuyVendor;              // Available from vendor

    // Opportunity cost
    float opportunityCost;          // What else could be done with the time
    float netBenefit;               // Net benefit of recommended method

    // Confidence
    float decisionConfidence;       // 0.0-1.0 confidence in recommendation
    std::string rationale;          // Human-readable explanation

    MaterialSourcingDecision()
        : itemId(0), quantityNeeded(0)
        , recommendedMethod(MaterialAcquisitionMethod::NONE)
        , alternativeMethod(MaterialAcquisitionMethod::NONE)
        , gatheringTimeCost(0), auctionCost(0), craftingCost(0), vendorCost(0)
        , gatheringTimeEstimate(0), auctionTimeEstimate(0), craftingTimeEstimate(0)
        , canGather(false), canBuyAuction(false), canCraft(false), canBuyVendor(false)
        , opportunityCost(0.0f), netBenefit(0.0f), decisionConfidence(0.0f) {}
};

/**
 * @brief Material acquisition plan for recipe
 */
struct MaterialAcquisitionPlan
{
    uint32 recipeId;
    ProfessionType profession;
    uint32 totalCost;                   // Total gold cost (copper)
    uint32 totalTime;                   // Total time required (seconds)
    std::vector<MaterialSourcingDecision> materialDecisions;

    // Plan optimization
    float efficiencyScore;              // Overall efficiency (0.0-1.0)
    float costScore;                    // Cost efficiency (lower is better)
    float timeScore;                    // Time efficiency (lower is better)

    MaterialAcquisitionPlan()
        : recipeId(0), profession(ProfessionType::NONE)
        , totalCost(0), totalTime(0)
        , efficiencyScore(0.0f), costScore(0.0f), timeScore(0.0f) {}

    float GetCostPerHour() const
    {
        return totalTime > 0 ? (float)totalCost / (totalTime / 3600.0f) : 0.0f;
    }
};

/**
 * @brief Economic parameters for decision-making
 */
struct EconomicParameters
{
    float goldPerHour;              // Bot's estimated gold/hour farming rate
    float gatheringEfficiency;      // 0.0-1.0 gathering success rate
    float auctionPriceThreshold;    // Max % above vendor price (1.5 = 150%)
    float timeValueMultiplier;      // Time value multiplier (1.0 = standard)
    bool preferGathering;           // Prefer gathering when costs are close
    bool preferSpeed;               // Prefer faster methods

    EconomicParameters()
        : goldPerHour(100.0f * 10000.0f)    // 100 gold/hour default
        , gatheringEfficiency(0.8f)
        , auctionPriceThreshold(1.5f)
        , timeValueMultiplier(1.0f)
        , preferGathering(false)
        , preferSpeed(false) {}
};

/**
 * @brief Per-bot economic profile
 */
struct BotEconomicProfile
{
    MaterialSourcingStrategy strategy = MaterialSourcingStrategy::BALANCED;
    EconomicParameters parameters;
    uint32 materialBudget = 500000;     // 50 gold default
    uint32 maxTimePerMaterial = 600;    // 10 minutes max per material
    bool autoExecutePlans = true;       // Automatically execute acquisition plans

    // Historical tracking
    uint32 totalGoldSpentOnMaterials = 0;
    uint32 totalTimeSpentGathering = 0;
    uint32 totalMaterialsGathered = 0;
    uint32 totalMaterialsBought = 0;

    BotEconomicProfile() = default;
};

/**
 * @brief Statistics for material sourcing decisions
 */
struct MaterialSourcingStatistics
{
    std::atomic<uint32> decisionsGather{0};
    std::atomic<uint32> decisionsBuy{0};
    std::atomic<uint32> decisionsCraft{0};
    std::atomic<uint32> decisionsVendor{0};
    std::atomic<uint32> decisionsHybrid{0};

    std::atomic<uint32> goldSavedByGathering{0};
    std::atomic<uint32> timeSavedByBuying{0};
    std::atomic<uint32> plansGenerated{0};
    std::atomic<uint32> plansExecuted{0};

    std::atomic<float> averageDecisionConfidence{0.0f};
    std::atomic<float> averageEfficiencyScore{0.0f};

    void Reset()
    {
        decisionsGather = 0;
        decisionsBuy = 0;
        decisionsCraft = 0;
        decisionsVendor = 0;
        decisionsHybrid = 0;
        goldSavedByGathering = 0;
        timeSavedByBuying = 0;
        plansGenerated = 0;
        plansExecuted = 0;
        averageDecisionConfidence = 0.0f;
        averageEfficiencyScore = 0.0f;
    }

    uint32 GetTotalDecisions() const
    {
        return decisionsGather.load() + decisionsBuy.load() +
               decisionsCraft.load() + decisionsVendor.load() + decisionsHybrid.load();
    }
};

/**
 * @brief Bridge for intelligent material sourcing decisions (per-bot instance)
 *
 * DESIGN PRINCIPLE: This class makes decisions but does NOT execute them.
 * Execution is delegated to GatheringMaterialsBridge and ProfessionAuctionBridge.
 * This class only provides economic analysis and recommendations.
 *
 * PHASE 4.2: Converted to per-bot instance pattern (owned by GameSystemsManager)
 */
class TC_GAME_API AuctionMaterialsBridge final
{
public:
    /**
     * @brief Construct bridge for specific bot
     * @param bot The bot this bridge serves (non-owning)
     */
    explicit AuctionMaterialsBridge(Player* bot);

    /**
     * @brief Destructor - unsubscribes from event bus
     */
    ~AuctionMaterialsBridge();

    // ========================================================================
    // CORE BRIDGE MANAGEMENT
    // ========================================================================

    /**
     * Initialize material sourcing bridge (loads shared data)
     * NOTE: Called per-bot, but loads shared world data only once
     */
    void Initialize();

    /**
     * Update material sourcing decisions (called periodically)
     */
    void Update(uint32 diff);

    /**
     * Enable/disable smart material sourcing
     */
    void SetEnabled(bool enabled);
    bool IsEnabled() const;

    /**
     * Set economic profile for bot
     */
    void SetEconomicProfile(BotEconomicProfile const& profile);
    BotEconomicProfile GetEconomicProfile() const;

    // ========================================================================
    // MATERIAL SOURCING DECISIONS
    // ========================================================================

    /**
     * Get best material source for single item
     * Analyzes all acquisition methods and returns optimal recommendation
     */
    MaterialSourcingDecision GetBestMaterialSource(
        uint32 itemId,
        uint32 quantity);

    /**
     * Get material acquisition plan for recipe
     * Multi-material optimization considering all reagents
     */
    MaterialAcquisitionPlan GetMaterialAcquisitionPlan(
        uint32 recipeId);

    /**
     * Get material acquisition plan for profession leveling
     * Optimizes material acquisition across multiple recipes
     */
    MaterialAcquisitionPlan GetLevelingMaterialPlan(
        ProfessionType profession,
        uint32 targetSkill);

    // ========================================================================
    // ECONOMIC ANALYSIS
    // ========================================================================

    /**
     * Check if buying from AH is cheaper than gathering
     * Factors in time value of gathering
     */
    bool IsBuyingCheaperThanGathering(
        uint32 itemId,
        uint32 quantity);

    /**
     * Calculate time value cost of gathering material
     * = gathering_time_seconds * (gold_per_hour / 3600)
     */
    uint32 CalculateGatheringTimeCost(
        uint32 itemId,
        uint32 quantity);

    /**
     * Calculate opportunity cost of acquisition method
     * What else could the bot do with that time/gold?
     */
    float CalculateOpportunityCost(
        MaterialAcquisitionMethod method,
        uint32 itemId,
        uint32 quantity);

    /**
     * Get bot's estimated gold per hour farming rate
     * Based on level, gear, class, and historical data
     */
    float GetBotGoldPerHour();

    // ========================================================================
    // GATHERING FEASIBILITY ANALYSIS
    // ========================================================================

    /**
     * Check if bot can gather this material
     * Requires gathering profession at appropriate skill
     */
    bool CanGatherMaterial(uint32 itemId);

    /**
     * Estimate time required to gather material
     * Factors in node density, competition, travel time
     */
    uint32 EstimateGatheringTime(
        uint32 itemId,
        uint32 quantity);

    /**
     * Get gathering success probability
     * Based on skill level vs required level
     */
    float GetGatheringSuccessProbability(uint32 itemId);

    // ========================================================================
    // AUCTION HOUSE ANALYSIS
    // ========================================================================

    /**
     * Check if material is available on AH at reasonable price
     * Uses auctionPriceThreshold from economic parameters
     */
    bool IsMaterialAvailableOnAH(
        uint32 itemId,
        uint32 quantity);

    /**
     * Get current auction house price for material
     * Delegates to ProfessionAuctionBridge
     */
    uint32 GetAuctionPrice(
        uint32 itemId,
        uint32 quantity);

    /**
     * Estimate time to buy from auction house
     * Includes travel time to AH + transaction time
     */
    uint32 EstimateAuctionPurchaseTime();

    // ========================================================================
    // CRAFTING ANALYSIS
    // ========================================================================

    /**
     * Check if bot can craft this material
     * Some "materials" are actually crafted items (e.g., enchanting reagents)
     */
    bool CanCraftMaterial(uint32 itemId);

    /**
     * Calculate cost to craft material
     * Includes reagent costs + time value
     */
    uint32 CalculateCraftingCost(
        uint32 itemId,
        uint32 quantity);

    /**
     * Estimate time required to craft material
     */
    uint32 EstimateCraftingTime(
        uint32 itemId,
        uint32 quantity);

    // ========================================================================
    // VENDOR ANALYSIS
    // ========================================================================

    /**
     * Check if material is sold by vendors
     */
    bool IsAvailableFromVendor(uint32 itemId);

    /**
     * Get vendor price for material
     */
    uint32 GetVendorPrice(uint32 itemId);

    // ========================================================================
    // PLAN EXECUTION
    // ========================================================================

    /**
     * Execute material acquisition plan
     * Coordinates with GatheringMaterialsBridge and ProfessionAuctionBridge
     */
    bool ExecuteAcquisitionPlan(MaterialAcquisitionPlan const& plan);

    /**
     * Acquire single material using recommended method
     */
    bool AcquireMaterial(MaterialSourcingDecision const& decision);

    // ========================================================================
    // STATISTICS
    // ========================================================================

    MaterialSourcingStatistics const& GetStatistics() const;
    static MaterialSourcingStatistics const& GetGlobalStatistics();
    void ResetStatistics();

private:

    // ========================================================================
    // EVENT HANDLING (Phase 2)
    // ========================================================================

    /**
     * @brief Handle profession events from ProfessionEventBus
     * Reacts to MATERIALS_NEEDED, MATERIAL_PURCHASED
     */
    void HandleProfessionEvent(struct ProfessionEvent const& event);

    // ========================================================================
    // INITIALIZATION HELPERS
    // ========================================================================

    void LoadVendorMaterials();
    void InitializeDefaultEconomicParameters();

    // ========================================================================
    // DECISION ALGORITHM HELPERS
    // ========================================================================

    /**
     * Score acquisition method for material
     * Higher score = better choice
     */
    float ScoreAcquisitionMethod(
        MaterialAcquisitionMethod method,
        uint32 itemId,
        uint32 quantity,
        EconomicParameters const& params);

    /**
     * Generate decision rationale for logging/debugging
     */
    std::string GenerateDecisionRationale(
        MaterialSourcingDecision const& decision);

    /**
     * Calculate decision confidence based on data quality
     */
    float CalculateDecisionConfidence(
        MaterialSourcingDecision const& decision);

    // ========================================================================
    // INTEGRATION HELPERS
    // ========================================================================

    GatheringMaterialsBridge* GetGatheringBridge();
    ProfessionAuctionBridge* GetAuctionBridge();
    ProfessionManager* GetProfessionManager();

    // ========================================================================
    // PER-BOT DATA MEMBERS (Phase 4.2: Converted from maps)
    // ========================================================================

    Player* _bot;                                       // Bot player reference (non-owning)
    BotEconomicProfile _profile;                        // Bot's economic profile
    MaterialAcquisitionPlan _activePlan;                // Active acquisition plan (if any)
    MaterialSourcingStatistics _statistics;             // Per-bot statistics
    uint32 _lastUpdateTime{0};                          // Last update timestamp

    // ========================================================================
    // SHARED DATA MEMBERS (Static - shared across all bots)
    // ========================================================================

    // Vendor materials (itemId -> vendor price) - world data
    static std::unordered_map<uint32, uint32> _vendorMaterials;
    static MaterialSourcingStatistics _globalStatistics;
    static bool _sharedDataInitialized;

    // Update intervals
    static constexpr uint32 DECISION_UPDATE_INTERVAL = 60000;      // 1 minute
    static constexpr uint32 PLAN_EXECUTION_CHECK = 5000;           // 5 seconds

    // Economic constants
    static constexpr float DEFAULT_GOLD_PER_HOUR = 100.0f * 10000.0f; // 100 gold
    static constexpr float DEFAULT_GATHERING_EFFICIENCY = 0.8f;    // 80% success rate
    static constexpr uint32 AUCTION_HOUSE_TRAVEL_TIME = 120;       // 2 minutes

    // Non-copyable
    AuctionMaterialsBridge(AuctionMaterialsBridge const&) = delete;
    AuctionMaterialsBridge& operator=(AuctionMaterialsBridge const&) = delete;
};

} // namespace Playerbot
