/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "AuctionMaterialsBridge.h"
#include "GameTime.h"
#include "GatheringMaterialsBridge.h"
#include "ProfessionAuctionBridge.h"
#include "ProfessionManager.h"
#include "Core/Events/GenericEventBus.h"
#include "ProfessionEvents.h"
#include "ProfessionEvents.h"
#include "../Professions/GatheringManager.h"
#include "../AI/BotAI.h"
#include "../Session/BotSession.h"
#include "../Core/Managers/GameSystemsManager.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "ItemTemplate.h"
#include "Log.h"
#include <algorithm>
#include <sstream>

namespace Playerbot
{

// ============================================================================
// STATIC MEMBER INITIALIZATION
// ============================================================================

std::unordered_map<uint32, uint32> AuctionMaterialsBridge::_vendorMaterials;
MaterialSourcingStatistics AuctionMaterialsBridge::_globalStatistics;
bool AuctionMaterialsBridge::_sharedDataInitialized = false;

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

AuctionMaterialsBridge::AuctionMaterialsBridge(Player* bot)
    : _bot(bot)
{
    // CRITICAL: Do NOT call _bot->GetName() in constructor!
    // Bot may not be fully in world yet during GameSystemsManager::Initialize(),
    // and Player::m_name is not initialized, causing ACCESS_VIOLATION.
    // Logging with bot identity deferred to first Update() call.
}

AuctionMaterialsBridge::~AuctionMaterialsBridge()
{
    // CRITICAL: No logging in destructors - can throw std::bad_alloc during memory pressure
    // Event bus unsubscription handled automatically by ProfessionEventBus
}

// ============================================================================
// CORE BRIDGE MANAGEMENT
// ============================================================================

void AuctionMaterialsBridge::Initialize()
{
    if (!_bot)
        return;

    // Load shared data once (thread-safe via static initialization)
    if (!_sharedDataInitialized)
    {
        LoadVendorMaterials();
        InitializeDefaultEconomicParameters();
        _sharedDataInitialized = true;
        TC_LOG_INFO("playerbot", "AuctionMaterialsBridge::Initialize - Loaded shared data (vendor materials)");
    }

    // Subscribe to ProfessionEventBus for event-driven reactivity (Phase 2)
    EventBus<ProfessionEvent>::instance()->SubscribeCallback(
        [this](ProfessionEvent const& event) { HandleProfessionEvent(event); },
        {
            ProfessionEventType::MATERIALS_NEEDED,
            ProfessionEventType::MATERIAL_PURCHASED
        }
    );

    // CRITICAL: Do NOT call _bot->GetName() in Initialize()!
    // Bot may not be fully in world yet during GameSystemsManager::Initialize(),
    // and Player::m_name is not initialized, causing ACCESS_VIOLATION.
    // Logging with bot identity deferred to first Update() call.
}

void AuctionMaterialsBridge::Update(uint32 /*diff*/)
{
    if (!_bot)
        return;

    // Check if enabled
    if (_profile.strategy == MaterialSourcingStrategy::NONE)
        return;

    // Throttle updates
    uint32 now = GameTime::GetGameTimeMS();
    if (now - _lastUpdateTime < DECISION_UPDATE_INTERVAL)
        return;
    _lastUpdateTime = now;

    // Check if we have an active plan to execute
    if (_activePlan.recipeId != 0 && _profile.autoExecutePlans)
    {
        ExecuteAcquisitionPlan(_activePlan);
    }
}

void AuctionMaterialsBridge::SetEnabled(bool enabled)
{
    if (!_bot)
        return;

    if (enabled)
    {
        // Initialize with default profile if needed
        if (_profile.strategy == MaterialSourcingStrategy::NONE)
        {
            _profile = BotEconomicProfile();
        }
    }
    else
    {
        _profile.strategy = MaterialSourcingStrategy::NONE;
        _activePlan = MaterialAcquisitionPlan();
    }
}

bool AuctionMaterialsBridge::IsEnabled() const
{
    return _bot && _profile.strategy != MaterialSourcingStrategy::NONE;
}

void AuctionMaterialsBridge::SetEconomicProfile(BotEconomicProfile const& profile)
{
    _profile = profile;
}

BotEconomicProfile AuctionMaterialsBridge::GetEconomicProfile() const
{
    return _profile;
}

// ============================================================================
// MATERIAL SOURCING DECISIONS
// ============================================================================

MaterialSourcingDecision AuctionMaterialsBridge::GetBestMaterialSource(
    uint32 itemId,
    uint32 quantity)
{
    MaterialSourcingDecision decision;
    decision.itemId = itemId;
    decision.quantityNeeded = quantity;

    if (!_bot)
    {
        decision.rationale = "Invalid bot";
        return decision;
    }

    // Get economic profile
    EconomicParameters params = _profile.parameters;
    MaterialSourcingStrategy strategy = _profile.strategy;

    // Analyze all acquisition methods
    decision.canGather = CanGatherMaterial(itemId);
    decision.canBuyAuction = IsMaterialAvailableOnAH(itemId, quantity);
    decision.canCraft = CanCraftMaterial(itemId);
    decision.canBuyVendor = IsAvailableFromVendor(itemId);

    // Calculate costs for each method
    if (decision.canGather)
    {
        decision.gatheringTimeEstimate = EstimateGatheringTime(itemId, quantity);
        decision.gatheringTimeCost = CalculateGatheringTimeCost(itemId, quantity);
    }

    if (decision.canBuyAuction)
    {
        decision.auctionCost = GetAuctionPrice(itemId, quantity);
        decision.auctionTimeEstimate = EstimateAuctionPurchaseTime();
    }

    if (decision.canCraft)
    {
        decision.craftingCost = CalculateCraftingCost(itemId, quantity);
        decision.craftingTimeEstimate = EstimateCraftingTime(itemId, quantity);
    }

    if (decision.canBuyVendor)
    {
        decision.vendorCost = GetVendorPrice(itemId) * quantity;
    }

    // Apply strategy-based decision logic
    switch (strategy)
    {
        case MaterialSourcingStrategy::ALWAYS_GATHER:
            if (decision.canGather)
            {
                decision.recommendedMethod = MaterialAcquisitionMethod::GATHER;
                decision.alternativeMethod = decision.canBuyAuction ? MaterialAcquisitionMethod::BUY_AUCTION : MaterialAcquisitionMethod::NONE;
            }
            break;

        case MaterialSourcingStrategy::ALWAYS_BUY:
            if (decision.canBuyAuction)
                decision.recommendedMethod = MaterialAcquisitionMethod::BUY_AUCTION;
            else if (decision.canBuyVendor)
                decision.recommendedMethod = MaterialAcquisitionMethod::VENDOR;
            else if (decision.canGather)
                decision.recommendedMethod = MaterialAcquisitionMethod::GATHER;
            break;

        case MaterialSourcingStrategy::COST_OPTIMIZED:
        {
            // Choose cheapest method considering time value
            float bestScore = -1.0f;
            MaterialAcquisitionMethod bestMethod = MaterialAcquisitionMethod::NONE;

            if (decision.canGather)
            {
                float score = ScoreAcquisitionMethod(MaterialAcquisitionMethod::GATHER, itemId, quantity, params);
                if (score > bestScore)
                {
                    bestScore = score;
                    bestMethod = MaterialAcquisitionMethod::GATHER;
                }
            }

            if (decision.canBuyAuction)
            {
                float score = ScoreAcquisitionMethod(MaterialAcquisitionMethod::BUY_AUCTION, itemId, quantity, params);
                if (score > bestScore)
                {
                    bestScore = score;
                    bestMethod = MaterialAcquisitionMethod::BUY_AUCTION;
                }
            }

            if (decision.canCraft)
            {
                float score = ScoreAcquisitionMethod(MaterialAcquisitionMethod::CRAFT, itemId, quantity, params);
                if (score > bestScore)
                {
                    bestScore = score;
                    bestMethod = MaterialAcquisitionMethod::CRAFT;
                }
            }

            if (decision.canBuyVendor)
            {
                float score = ScoreAcquisitionMethod(MaterialAcquisitionMethod::VENDOR, itemId, quantity, params);
                if (score > bestScore)
                {
                    bestScore = score;
                    bestMethod = MaterialAcquisitionMethod::VENDOR;
                }
            }

            decision.recommendedMethod = bestMethod;
            break;
        }

        case MaterialSourcingStrategy::TIME_OPTIMIZED:
        {
            // Choose fastest method
            uint32 fastestTime = UINT32_MAX;
            MaterialAcquisitionMethod fastestMethod = MaterialAcquisitionMethod::NONE;

            if (decision.canBuyVendor)
            {
                fastestTime = 10; // Instant
                fastestMethod = MaterialAcquisitionMethod::VENDOR;
            }

            if (decision.canBuyAuction && decision.auctionTimeEstimate < fastestTime)
            {
                fastestTime = decision.auctionTimeEstimate;
                fastestMethod = MaterialAcquisitionMethod::BUY_AUCTION;
            }

            if (decision.canCraft && decision.craftingTimeEstimate < fastestTime)
            {
                fastestTime = decision.craftingTimeEstimate;
                fastestMethod = MaterialAcquisitionMethod::CRAFT;
            }

            if (decision.canGather && decision.gatheringTimeEstimate < fastestTime)
            {
                fastestTime = decision.gatheringTimeEstimate;
                fastestMethod = MaterialAcquisitionMethod::GATHER;
            }

            decision.recommendedMethod = fastestMethod;
            break;
        }

        case MaterialSourcingStrategy::BALANCED:
        default:
        {
            // Balance between cost and time
            // Vendor is always preferred if available (instant + cheap)
            if (decision.canBuyVendor)
            {
                decision.recommendedMethod = MaterialAcquisitionMethod::VENDOR;
            }
            // Buy from AH if significantly cheaper than gathering time value
            else if (decision.canBuyAuction && decision.canGather)
            {
                uint32 totalAuctionCost = decision.auctionCost + (decision.auctionTimeEstimate * params.goldPerHour / 3600);
                uint32 totalGatheringCost = decision.gatheringTimeCost;

                if (totalAuctionCost < totalGatheringCost * 0.8f) // 20% savings threshold
                {
                    decision.recommendedMethod = MaterialAcquisitionMethod::BUY_AUCTION;
                    decision.alternativeMethod = MaterialAcquisitionMethod::GATHER;
                }
                else
                {
                    decision.recommendedMethod = MaterialAcquisitionMethod::GATHER;
                    decision.alternativeMethod = MaterialAcquisitionMethod::BUY_AUCTION;
                }
            }
            else if (decision.canBuyAuction)
            {
                decision.recommendedMethod = MaterialAcquisitionMethod::BUY_AUCTION;
            }
            else if (decision.canGather)
            {
                decision.recommendedMethod = MaterialAcquisitionMethod::GATHER;
            }
            else if (decision.canCraft)
            {
                decision.recommendedMethod = MaterialAcquisitionMethod::CRAFT;
            }
            break;
        }
    }

    // Calculate opportunity cost and net benefit
    decision.opportunityCost = CalculateOpportunityCost(decision.recommendedMethod, itemId, quantity);

    // =========================================================================
    // Full net benefit calculation with economic analysis
    // Factors in market value, skill gains, guild benefits, and time value
    // =========================================================================
    float netBenefit = 0.0f;

    // 1. Base cost of acquisition (negative)
    float acquisitionCost = 0.0f;
    switch (decision.recommendedMethod)
    {
        case MaterialAcquisitionMethod::GATHER:
            acquisitionCost = float(decision.gatheringTimeCost);
            break;
        case MaterialAcquisitionMethod::BUY_AUCTION:
            acquisitionCost = float(decision.auctionCost);
            break;
        case MaterialAcquisitionMethod::CRAFT:
            acquisitionCost = float(decision.craftingCost);
            break;
        case MaterialAcquisitionMethod::VENDOR:
            acquisitionCost = float(decision.vendorCost);
            break;
        default:
            break;
    }
    netBenefit -= acquisitionCost;

    // 2. Market value of the material (positive if we could sell it)
    // Get current AH price for reference
    uint32 marketPrice = GetAuctionPrice(itemId, quantity);
    if (marketPrice > 0)
    {
        // Material has resale value (factor in AH cut of 5%)
        float resaleValue = float(marketPrice) * 0.95f;
        // Only count resale value if we're buying below market
        if (acquisitionCost < resaleValue)
            netBenefit += (resaleValue - acquisitionCost) * 0.3f;  // 30% weight for liquidity option
    }

    // 3. Profession skill gain value
    // Materials used in crafting contribute to skill progression
    ProfessionManager* profMgr = GetProfessionManager();
    if (profMgr)
    {
        // Check if this material is used in any learnable recipe
        static const std::vector<ProfessionType> allProfessions = {
            ProfessionType::ALCHEMY, ProfessionType::BLACKSMITHING,
            ProfessionType::ENCHANTING, ProfessionType::ENGINEERING,
            ProfessionType::INSCRIPTION, ProfessionType::JEWELCRAFTING,
            ProfessionType::LEATHERWORKING, ProfessionType::TAILORING
        };

        for (ProfessionType profession : allProfessions)
        {
            auto recipes = profMgr->GetRecipesForProfession(profession);
            for (const RecipeInfo& recipe : recipes)
            {
                for (const RecipeInfo::Reagent& reagent : recipe.reagents)
                {
                    if (reagent.itemId == itemId)
                    {
                        // Material contributes to profession leveling
                        // Value each potential skill point at goldPerHour / 10
                        float skillPointValue = _profile.parameters.goldPerHour / 10.0f;

                        // Calculate probability of skill gain
                        float skillGainChance = 0.5f;  // Base 50% for appropriate level recipes

                        // Add skill gain value to net benefit
                        netBenefit += skillPointValue * skillGainChance * 0.5f;  // 50% weight
                        break;
                    }
                }
            }
        }
    }

    // 4. Guild bank contribution value
    // If bot is in a guild, materials have additional social value
    if (_bot && _bot->GetGuildId() != 0)
    {
        // Guild contribution bonus: 10% of material value
        float guildBonus = acquisitionCost * 0.10f;
        netBenefit += guildBonus;
    }

    // 5. Reputation gains from faction-specific materials
    // Some materials grant reputation when turned in or used
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
    if (itemTemplate)
    {
        // Check for reputation-granting items (e.g., cloth turn-ins, faction materials)
        // Simplified: Add bonus for quality materials that might have faction uses
        if (itemTemplate->GetQuality() >= ITEM_QUALITY_UNCOMMON)
        {
            float repBonus = acquisitionCost * 0.05f;  // 5% reputation value estimate
            netBenefit += repBonus;
        }
    }

    // 6. Time-adjusted present value calculation
    // Future profits are worth less than immediate profits
    // Apply discount rate based on estimated time to realize value
    uint32 timeToValue = 0;  // Seconds until value is realized
    switch (decision.recommendedMethod)
    {
        case MaterialAcquisitionMethod::GATHER:
            timeToValue = decision.gatheringTimeEstimate;
            break;
        case MaterialAcquisitionMethod::BUY_AUCTION:
            timeToValue = decision.auctionTimeEstimate;
            break;
        case MaterialAcquisitionMethod::CRAFT:
            timeToValue = decision.craftingTimeEstimate;
            break;
        case MaterialAcquisitionMethod::VENDOR:
            timeToValue = 10;  // Instant
            break;
        default:
            break;
    }

    // Apply time discount (1% per hour of delay)
    float hoursDelay = float(timeToValue) / 3600.0f;
    float discountFactor = 1.0f / (1.0f + 0.01f * hoursDelay);
    netBenefit *= discountFactor;

    // 7. Risk adjustment
    // Different acquisition methods have different risk profiles
    float riskFactor = 1.0f;
    switch (decision.recommendedMethod)
    {
        case MaterialAcquisitionMethod::GATHER:
            // Gathering has medium risk (competition, node availability)
            riskFactor = 0.85f;
            break;
        case MaterialAcquisitionMethod::BUY_AUCTION:
            // AH has low risk if item is available
            riskFactor = decision.canBuyAuction ? 0.95f : 0.0f;
            break;
        case MaterialAcquisitionMethod::CRAFT:
            // Crafting has medium-high risk (material availability)
            riskFactor = 0.80f;
            break;
        case MaterialAcquisitionMethod::VENDOR:
            // Vendor has no risk (always available at fixed price)
            riskFactor = 1.0f;
            break;
        default:
            riskFactor = 0.5f;
            break;
    }
    netBenefit *= riskFactor;

    // 8. Final opportunity cost deduction
    netBenefit -= decision.opportunityCost * 0.5f;  // 50% weight for opportunity cost

    decision.netBenefit = netBenefit;

    // Generate rationale and confidence
    decision.rationale = GenerateDecisionRationale(decision);
    decision.decisionConfidence = CalculateDecisionConfidence(decision);

    // Update statistics
    switch (decision.recommendedMethod)
    {
        case MaterialAcquisitionMethod::GATHER:
            _globalStatistics.decisionsGather++;
            _statistics.decisionsGather++;
            break;
        case MaterialAcquisitionMethod::BUY_AUCTION:
            _globalStatistics.decisionsBuy++;
            _statistics.decisionsBuy++;
            break;
        case MaterialAcquisitionMethod::CRAFT:
            _globalStatistics.decisionsCraft++;
            _statistics.decisionsCraft++;
            break;
        case MaterialAcquisitionMethod::VENDOR:
            _globalStatistics.decisionsVendor++;
            _statistics.decisionsVendor++;
            break;
        default:
            break;
    }

    return decision;
}

MaterialAcquisitionPlan AuctionMaterialsBridge::GetMaterialAcquisitionPlan(
    uint32 recipeId)
{
    MaterialAcquisitionPlan plan;
    plan.recipeId = recipeId;

    if (!_bot)
        return plan;

    // Get recipe info from ProfessionManager via GameSystemsManager
    ProfessionManager* profMgr = GetProfessionManager();
    if (!profMgr)
        return plan;

    // Find recipe across all professions
    static const std::vector<ProfessionType> allProfessions = {
        ProfessionType::ALCHEMY, ProfessionType::BLACKSMITHING,
        ProfessionType::ENCHANTING, ProfessionType::ENGINEERING,
        ProfessionType::INSCRIPTION, ProfessionType::JEWELCRAFTING,
        ProfessionType::LEATHERWORKING, ProfessionType::TAILORING,
        ProfessionType::COOKING
    };

    RecipeInfo const* recipe = nullptr;
    for (ProfessionType profession : allProfessions)
    {
        auto recipes = profMgr->GetRecipesForProfession(profession);
        for (const RecipeInfo& r : recipes)
        {
            if (r.recipeId == recipeId)
            {
                recipe = &r;
                plan.profession = profession;
                break;
            }
        }
        if (recipe)
            break;
    }

    if (!recipe)
        return plan;

    // Get material decisions for each reagent
    for (const RecipeInfo::Reagent& reagent : recipe->reagents)
    {
        MaterialSourcingDecision decision = GetBestMaterialSource(reagent.itemId, reagent.quantity);
        plan.materialDecisions.push_back(decision);

        // Accumulate costs and time
        switch (decision.recommendedMethod)
        {
            case MaterialAcquisitionMethod::GATHER:
                plan.totalCost += decision.gatheringTimeCost;
                plan.totalTime += decision.gatheringTimeEstimate;
                break;
            case MaterialAcquisitionMethod::BUY_AUCTION:
                plan.totalCost += decision.auctionCost;
                plan.totalTime += decision.auctionTimeEstimate;
                break;
            case MaterialAcquisitionMethod::CRAFT:
                plan.totalCost += decision.craftingCost;
                plan.totalTime += decision.craftingTimeEstimate;
                break;
            case MaterialAcquisitionMethod::VENDOR:
                plan.totalCost += decision.vendorCost;
                plan.totalTime += 10; // Instant
                break;
            default:
                break;
        }
    }

    // Calculate efficiency scores
    if (plan.totalTime > 0)
    {
        plan.timeScore = (float)plan.totalTime / 600.0f; // Normalize to 10 minutes
        plan.costScore = (float)plan.totalCost / 100000.0f; // Normalize to 10 gold

        // Overall efficiency (lower is better)
        plan.efficiencyScore = 1.0f / (1.0f + plan.timeScore + plan.costScore);
    }

    _globalStatistics.plansGenerated++;

    return plan;
}

MaterialAcquisitionPlan AuctionMaterialsBridge::GetLevelingMaterialPlan(
    ProfessionType profession,
    uint32 targetSkill)
{
    MaterialAcquisitionPlan plan;
    plan.profession = profession;

    if (!_bot)
        return plan;

    ProfessionManager* profMgr = GetProfessionManager();
    if (!profMgr)
        return plan;

    // Get optimal leveling recipe
    RecipeInfo const* recipe = profMgr->GetOptimalLevelingRecipe(profession);
    if (!recipe)
        return plan;

    return GetMaterialAcquisitionPlan(recipe->recipeId);
}

// ============================================================================
// ECONOMIC ANALYSIS
// ============================================================================

bool AuctionMaterialsBridge::IsBuyingCheaperThanGathering(
    uint32 itemId,
    uint32 quantity)
{
    if (!_bot)
        return false;

    // Get auction price
    uint32 auctionCost = GetAuctionPrice(itemId, quantity);
    if (auctionCost == 0)
        return false; // Not available

    // Get gathering time cost
    uint32 gatheringCost = CalculateGatheringTimeCost(itemId, quantity);
    if (gatheringCost == 0)
        return true; // Can't gather, so buying is the only option

    // Add auction travel time
    uint32 auctionTime = EstimateAuctionPurchaseTime();
    EconomicParameters params = _profile.parameters;

    uint32 totalAuctionCost = auctionCost + (auctionTime * params.goldPerHour / 3600);

    return totalAuctionCost < gatheringCost;
}

uint32 AuctionMaterialsBridge::CalculateGatheringTimeCost(
    uint32 itemId,
    uint32 quantity)
{
    if (!_bot)
        return 0;

    // Estimate gathering time
    uint32 gatheringTime = EstimateGatheringTime(itemId, quantity);
    if (gatheringTime == 0)
        return 0;

    // Get bot's gold per hour rate
    float goldPerHour = GetBotGoldPerHour();

    // Time value cost = (time in seconds) * (gold per hour / 3600)
    uint32 timeCost = static_cast<uint32>(gatheringTime * (goldPerHour / 3600.0f));

    return timeCost;
}

float AuctionMaterialsBridge::CalculateOpportunityCost(
    MaterialAcquisitionMethod method,
    uint32 itemId,
    uint32 quantity)
{
    if (!_bot)
        return 0.0f;

    float goldPerHour = GetBotGoldPerHour();
    uint32 timeSpent = 0;

    switch (method)
    {
        case MaterialAcquisitionMethod::GATHER:
            timeSpent = EstimateGatheringTime(itemId, quantity);
            break;
        case MaterialAcquisitionMethod::BUY_AUCTION:
            timeSpent = EstimateAuctionPurchaseTime();
            break;
        case MaterialAcquisitionMethod::CRAFT:
            timeSpent = EstimateCraftingTime(itemId, quantity);
            break;
        case MaterialAcquisitionMethod::VENDOR:
            timeSpent = 10; // Minimal time
            break;
        default:
            break;
    }

    // Opportunity cost = what else could be done with that time
    float opportunityCost = timeSpent * (goldPerHour / 3600.0f);

    return opportunityCost;
}

float AuctionMaterialsBridge::GetBotGoldPerHour()
{
    if (!_bot)
        return DEFAULT_GOLD_PER_HOUR;

    // Use profile if configured
    if (_profile.parameters.goldPerHour > 0)
        return _profile.parameters.goldPerHour;

    // Estimate based on level
    uint8 level = _bot->GetLevel();
    if (level >= 80)
        return 150.0f * 10000.0f; // 150 gold/hour
    else if (level >= 70)
        return 100.0f * 10000.0f; // 100 gold/hour
    else if (level >= 60)
        return 75.0f * 10000.0f;  // 75 gold/hour
    else if (level >= 50)
        return 50.0f * 10000.0f;  // 50 gold/hour
    else
        return 25.0f * 10000.0f;  // 25 gold/hour
}

// ============================================================================
// GATHERING FEASIBILITY ANALYSIS
// ============================================================================

bool AuctionMaterialsBridge::CanGatherMaterial(uint32 itemId)
{
    if (!_bot)
        return false;

    GatheringMaterialsBridge* gatherBridge = GetGatheringBridge();
    if (!gatherBridge)
        return false;

    return gatherBridge->IsItemNeededForCrafting(itemId);
}

uint32 AuctionMaterialsBridge::EstimateGatheringTime(
    uint32 itemId,
    uint32 quantity)
{
    if (!_bot)
        return 0;

    // Base time per node (average)
    uint32 timePerNode = 60; // 1 minute per node

    // Estimate yield per node
    uint32 yieldPerNode = 2; // Conservative estimate

    // Calculate number of nodes needed
    uint32 nodesNeeded = (quantity + yieldPerNode - 1) / yieldPerNode;

    // Total time = nodes * (travel time + gather time)
    uint32 totalTime = nodesNeeded * timePerNode;

    // Apply gathering efficiency
    float efficiency = _profile.parameters.gatheringEfficiency;

    // Adjust for efficiency
    totalTime = static_cast<uint32>(totalTime / efficiency);

    return totalTime;
}

float AuctionMaterialsBridge::GetGatheringSuccessProbability(uint32 itemId)
{
    if (!_bot)
        return 0.0f;

    return _profile.parameters.gatheringEfficiency;
}

// ============================================================================
// AUCTION HOUSE ANALYSIS
// ============================================================================

bool AuctionMaterialsBridge::IsMaterialAvailableOnAH(
    uint32 itemId,
    uint32 quantity)
{
    if (!_bot)
        return false;

    ProfessionAuctionBridge* auctionBridge = GetAuctionBridge();
    if (!auctionBridge)
        return false;

    uint32 maxPricePerUnit = UINT32_MAX;

    return auctionBridge->IsMaterialAvailableForPurchase(_bot, itemId, quantity, maxPricePerUnit);
}

uint32 AuctionMaterialsBridge::GetAuctionPrice(
    uint32 itemId,
    uint32 quantity)
{
    if (!_bot)
        return 0;

    ProfessionAuctionBridge* auctionBridge = GetAuctionBridge();
    if (!auctionBridge)
        return 0;

    return auctionBridge->GetOptimalMaterialPrice(_bot, itemId, quantity);
}

uint32 AuctionMaterialsBridge::EstimateAuctionPurchaseTime()
{
    if (!_bot)
        return 0;

    // Travel time to AH + transaction time
    return AUCTION_HOUSE_TRAVEL_TIME + 30; // 2.5 minutes total
}

// ============================================================================
// CRAFTING ANALYSIS
// ============================================================================

bool AuctionMaterialsBridge::CanCraftMaterial(uint32 itemId)
{
    if (!_bot)
        return false;

    // Some materials can be crafted (e.g., enchanting reagents)
    // Check if bot knows any recipe that produces this item
    ProfessionManager* profMgr = GetProfessionManager();
    if (!profMgr)
        return false;

    static const std::vector<ProfessionType> allProfessions = {
        ProfessionType::ALCHEMY, ProfessionType::BLACKSMITHING,
        ProfessionType::ENCHANTING, ProfessionType::ENGINEERING,
        ProfessionType::INSCRIPTION, ProfessionType::JEWELCRAFTING,
        ProfessionType::LEATHERWORKING, ProfessionType::TAILORING
    };

    for (ProfessionType profession : allProfessions)
    {
        auto recipes = profMgr->GetRecipesForProfession(profession);
        for (const RecipeInfo& recipe : recipes)
        {
            if (recipe.productItemId == itemId)
                return true;
        }
    }

    return false;
}

uint32 AuctionMaterialsBridge::CalculateCraftingCost(
    uint32 itemId,
    uint32 quantity)
{
    if (!_bot)
        return 0;

    // Find recipe and calculate reagent costs
    ProfessionManager* profMgr = GetProfessionManager();
    if (!profMgr)
        return 0;

    static const std::vector<ProfessionType> allProfessions = {
        ProfessionType::ALCHEMY, ProfessionType::BLACKSMITHING,
        ProfessionType::ENCHANTING, ProfessionType::ENGINEERING,
        ProfessionType::INSCRIPTION, ProfessionType::JEWELCRAFTING,
        ProfessionType::LEATHERWORKING, ProfessionType::TAILORING
    };

    for (ProfessionType profession : allProfessions)
    {
        auto recipes = profMgr->GetRecipesForProfession(profession);
        for (const RecipeInfo& recipe : recipes)
        {
            if (recipe.productItemId == itemId)
            {
                uint32 totalCost = 0;
                for (const RecipeInfo::Reagent& reagent : recipe.reagents)
                {
                    uint32 reagentCost = GetAuctionPrice(reagent.itemId, reagent.quantity * quantity);
                    totalCost += reagentCost;
                }

                // Add time value of crafting
                uint32 craftingTime = EstimateCraftingTime(itemId, quantity);
                float goldPerHour = GetBotGoldPerHour();
                totalCost += static_cast<uint32>(craftingTime * (goldPerHour / 3600.0f));

                return totalCost;
            }
        }
    }

    return 0;
}

uint32 AuctionMaterialsBridge::EstimateCraftingTime(
    uint32 itemId,
    uint32 quantity)
{
    if (!_bot)
        return 0;

    // Crafting takes ~3 seconds per item
    return quantity * 3;
}

// ============================================================================
// VENDOR ANALYSIS
// ============================================================================

bool AuctionMaterialsBridge::IsAvailableFromVendor(uint32 itemId)
{
    return _vendorMaterials.find(itemId) != _vendorMaterials.end();
}

uint32 AuctionMaterialsBridge::GetVendorPrice(uint32 itemId)
{
    auto itr = _vendorMaterials.find(itemId);
    if (itr != _vendorMaterials.end())
        return itr->second;

    return 0;
}

// ============================================================================
// PLAN EXECUTION
// ============================================================================

bool AuctionMaterialsBridge::ExecuteAcquisitionPlan(
    MaterialAcquisitionPlan const& plan)
{
    if (!_bot)
        return false;

    bool allSuccessful = true;

    for (const MaterialSourcingDecision& decision : plan.materialDecisions)
    {
        if (!AcquireMaterial(decision))
            allSuccessful = false;
    }

    if (allSuccessful)
    {
        _globalStatistics.plansExecuted++;
    }

    return allSuccessful;
}

bool AuctionMaterialsBridge::AcquireMaterial(
    MaterialSourcingDecision const& decision)
{
    if (!_bot)
        return false;

    switch (decision.recommendedMethod)
    {
        case MaterialAcquisitionMethod::GATHER:
        {
            GatheringMaterialsBridge* gatherBridge = GetGatheringBridge();
            if (gatherBridge)
                return gatherBridge->StartGatheringForMaterial(decision.itemId, decision.quantityNeeded);
            break;
        }

        case MaterialAcquisitionMethod::BUY_AUCTION:
        {
            ProfessionAuctionBridge* auctionBridge = GetAuctionBridge();
            if (auctionBridge)
                return auctionBridge->PurchaseMaterial(_bot, decision.itemId, decision.quantityNeeded, UINT32_MAX);
            break;
        }

        case MaterialAcquisitionMethod::CRAFT:
        {
            // Delegate to ProfessionManager
            ProfessionManager* profMgr = GetProfessionManager();
            if (profMgr)
            {
                // This would trigger crafting automation
                // Implementation depends on ProfessionManager API
            }
            break;
        }

        case MaterialAcquisitionMethod::VENDOR:
        {
            // Bots can travel to vendors and purchase
            // Implementation would be in bot movement/vendor interaction
            break;
        }

        default:
            return false;
    }

    return false;
}

// ============================================================================
// STATISTICS
// ============================================================================

MaterialSourcingStatistics const& AuctionMaterialsBridge::GetStatistics() const
{
    return _statistics;
}

MaterialSourcingStatistics const& AuctionMaterialsBridge::GetGlobalStatistics()
{
    return _globalStatistics;
}

void AuctionMaterialsBridge::ResetStatistics()
{
    _statistics.Reset();
}

// ============================================================================
// INITIALIZATION HELPERS
// ============================================================================

void AuctionMaterialsBridge::LoadVendorMaterials()
{
    // Common vendor materials with prices (in copper)
    // This is a subset - full implementation would load from database

    // Crafting reagents
    _vendorMaterials[2880] = 10;    // Weak Flux (10 copper)
    _vendorMaterials[2901] = 100;   // Mining Pick (1 silver)
    _vendorMaterials[3371] = 400;   // Crystal Vial (4 silver)
    _vendorMaterials[4289] = 50;    // Salt (50 copper)
    _vendorMaterials[4340] = 350;   // Gray Dye (3.5 silver)
    _vendorMaterials[4341] = 500;   // Yellow Dye (5 silver)
    _vendorMaterials[4342] = 350;   // Purple Dye (3.5 silver)
    _vendorMaterials[5956] = 18;    // Blacksmith Hammer (18 copper)
    _vendorMaterials[6217] = 124;   // Copper Rod (1.24 silver)
    _vendorMaterials[6256] = 50;    // Fishing Pole (50 copper)
    _vendorMaterials[30817] = 2500; // Simple Flour (25 silver)

    // Thread
    _vendorMaterials[2320] = 10;    // Coarse Thread (10 copper)
    _vendorMaterials[2321] = 100;   // Fine Thread (1 silver)
    _vendorMaterials[4291] = 500;   // Silken Thread (5 silver)
    _vendorMaterials[8343] = 2000;  // Heavy Silken Thread (20 silver)
    _vendorMaterials[14341] = 10000; // Rune Thread (1 gold)

    TC_LOG_INFO("playerbot", "AuctionMaterialsBridge::LoadVendorMaterials - Loaded {} vendor materials",
        _vendorMaterials.size());
}

void AuctionMaterialsBridge::InitializeDefaultEconomicParameters()
{
    // Default economic parameters are set in EconomicParameters constructor
    TC_LOG_INFO("playerbot", "AuctionMaterialsBridge::InitializeDefaultEconomicParameters - Initialized default parameters");
}

// ============================================================================
// DECISION ALGORITHM HELPERS
// ============================================================================

float AuctionMaterialsBridge::ScoreAcquisitionMethod(
    MaterialAcquisitionMethod method,
    uint32 itemId,
    uint32 quantity,
    EconomicParameters const& params)
{
    if (!_bot)
        return 0.0f;

    float score = 100.0f;

    switch (method)
    {
        case MaterialAcquisitionMethod::GATHER:
        {
            uint32 timeCost = CalculateGatheringTimeCost(itemId, quantity);
            // Lower time cost = higher score
            score = 100.0f / (1.0f + timeCost / 10000.0f);

            if (params.preferGathering)
                score *= 1.5f;
            break;
        }

        case MaterialAcquisitionMethod::BUY_AUCTION:
        {
            uint32 goldCost = GetAuctionPrice(itemId, quantity);
            // Lower gold cost = higher score
            score = 100.0f / (1.0f + goldCost / 10000.0f);

            if (params.preferSpeed)
                score *= 1.3f;
            break;
        }

        case MaterialAcquisitionMethod::CRAFT:
        {
            uint32 totalCost = CalculateCraftingCost(itemId, quantity);
            score = 100.0f / (1.0f + totalCost / 10000.0f);
            break;
        }

        case MaterialAcquisitionMethod::VENDOR:
        {
            uint32 vendorCost = GetVendorPrice(itemId) * quantity;
            // Vendor is always reliable and instant - bonus score
            score = 150.0f / (1.0f + vendorCost / 10000.0f);
            break;
        }

        default:
            score = 0.0f;
            break;
    }

    return score;
}

std::string AuctionMaterialsBridge::GenerateDecisionRationale(
    MaterialSourcingDecision const& decision)
{
    std::ostringstream oss;

    oss << "Recommended: ";

    switch (decision.recommendedMethod)
    {
        case MaterialAcquisitionMethod::GATHER:
            oss << "GATHER (time: " << decision.gatheringTimeEstimate
                << "s, cost: " << decision.gatheringTimeCost / 10000.0f << "g)";
            break;
        case MaterialAcquisitionMethod::BUY_AUCTION:
            oss << "BUY_AUCTION (time: " << decision.auctionTimeEstimate
                << "s, cost: " << decision.auctionCost / 10000.0f << "g)";
            break;
        case MaterialAcquisitionMethod::CRAFT:
            oss << "CRAFT (time: " << decision.craftingTimeEstimate
                << "s, cost: " << decision.craftingCost / 10000.0f << "g)";
            break;
        case MaterialAcquisitionMethod::VENDOR:
            oss << "VENDOR (instant, cost: " << decision.vendorCost / 10000.0f << "g)";
            break;
        default:
            oss << "NONE (no feasible method)";
            break;
    }

    if (decision.alternativeMethod != MaterialAcquisitionMethod::NONE)
    {
        oss << " | Alternative: ";
        switch (decision.alternativeMethod)
        {
            case MaterialAcquisitionMethod::GATHER:
                oss << "GATHER";
                break;
            case MaterialAcquisitionMethod::BUY_AUCTION:
                oss << "BUY_AUCTION";
                break;
            case MaterialAcquisitionMethod::CRAFT:
                oss << "CRAFT";
                break;
            default:
                break;
        }
    }

    return oss.str();
}

float AuctionMaterialsBridge::CalculateDecisionConfidence(
    MaterialSourcingDecision const& decision)
{
    if (!_bot)
        return 0.0f;

    float confidence = 1.0f;

    // Reduce confidence if no feasible method
    if (decision.recommendedMethod == MaterialAcquisitionMethod::NONE)
        return 0.0f;

    // Reduce confidence if costs are estimated (not real data)
    if (decision.recommendedMethod == MaterialAcquisitionMethod::GATHER)
    {
        if (decision.gatheringTimeEstimate == 0)
            confidence *= 0.5f;
    }

    // High confidence for vendor purchases (always reliable)
    if (decision.recommendedMethod == MaterialAcquisitionMethod::VENDOR)
        confidence = 1.0f;

    return confidence;
}

// ============================================================================
// INTEGRATION HELPERS
// ============================================================================

GatheringMaterialsBridge* AuctionMaterialsBridge::GetGatheringBridge()
{
    if (!_bot)
        return nullptr;

    BotSession* session = static_cast<BotSession*>(_bot->GetSession());
    if (!session || !session->GetAI())
        return nullptr;

    GameSystemsManager* systems = static_cast<GameSystemsManager*>(session->GetAI()->GetGameSystems());
    return systems ? systems->GetGatheringMaterialsBridge() : nullptr;
}

ProfessionAuctionBridge* AuctionMaterialsBridge::GetAuctionBridge()
{
    if (!_bot)
        return nullptr;

    BotSession* session = static_cast<BotSession*>(_bot->GetSession());
    if (!session || !session->GetAI())
        return nullptr;

    GameSystemsManager* systems = static_cast<GameSystemsManager*>(session->GetAI()->GetGameSystems());
    return systems ? systems->GetProfessionAuctionBridge() : nullptr;
}

ProfessionManager* AuctionMaterialsBridge::GetProfessionManager()
{
    if (!_bot)
        return nullptr;

    BotSession* session = static_cast<BotSession*>(_bot->GetSession());
    if (!session || !session->GetAI())
        return nullptr;

    return session->GetAI()->GetGameSystems()->GetProfessionManager();
}

// ============================================================================
// EVENT HANDLING (Phase 2)
// ============================================================================

void AuctionMaterialsBridge::HandleProfessionEvent(ProfessionEvent const& event)
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
            // When materials are needed, analyze AH prices and recommend buy vs gather
            TC_LOG_DEBUG("playerbot.events.profession",
                "AuctionMaterialsBridge: MATERIALS_NEEDED event for bot '{}' - Item {} x{} needed for profession {}",
                _bot->GetName(), event.itemId, event.quantity, static_cast<uint32>(event.profession));

            // Analyze best sourcing method for this material
            MaterialSourcingDecision decision = GetBestMaterialSource(event.itemId, event.quantity);

            // Log recommendation
            TC_LOG_DEBUG("playerbot",
                "AuctionMaterialsBridge: Bot '{}' material sourcing for {} x{}: {}",
                _bot->GetName(), event.itemId, event.quantity, decision.rationale);

            // Statistics already updated in GetBestMaterialSource()
            break;
        }

        case ProfessionEventType::MATERIAL_PURCHASED:
        {
            // When materials are purchased, track spending and update economic metrics
            TC_LOG_DEBUG("playerbot.events.profession",
                "AuctionMaterialsBridge: Bot '{}' MATERIAL_PURCHASED event - Item {} x{} for {} gold",
                _bot->GetName(), event.itemId, event.quantity, event.goldAmount);

            // Update bot's economic profile
            _profile.totalGoldSpentOnMaterials += event.goldAmount;
            _profile.totalMaterialsBought += event.quantity;

            TC_LOG_DEBUG("playerbot",
                "AuctionMaterialsBridge: Bot '{}' tracked {} gold spent on {} x{}",
                _bot->GetName(), event.goldAmount, event.itemId, event.quantity);
            break;
        }

        default:
            // Ignore other event types
            break;
    }
}

} // namespace Playerbot
