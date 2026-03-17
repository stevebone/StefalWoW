/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * PROFESSION AUCTION BRIDGE FOR PLAYERBOT
 *
 * This system bridges profession automation with the existing auction house system:
 * - Automatically sells excess gathered materials
 * - Automatically sells crafted items for profit
 * - Buys materials when needed for crafting leveling
 * - Manages material stockpiles for optimal auction listings
 * - Coordinates with FarmingCoordinator for material targets
 *
 * Integration Points:
 * - Uses existing Playerbot::AuctionHouse for all auction operations
 * - Coordinates with ProfessionManager for crafting
 * - Works with GatheringAutomation for material collection
 * - Uses FarmingCoordinator for stockpile management
 * - Subscribes to ProfessionEventBus for event-driven reactivity (Phase 2)
 *
 * Design Pattern: Bridge Pattern
 * - Decouples profession logic from auction logic
 * - All auction operations delegated to existing AuctionHouse class
 * - This class only manages profession-auction coordination
 *
 * Event Integration (Phase 2 - 2025-11-18):
 * - CRAFTING_COMPLETED → List crafted items for sale on AH
 * - ITEM_BANKED → Update inventory tracking and stockpile management
 *
 * Phase 4.3 Refactoring (2025-11-18):
 * - Converted from global singleton to per-bot instance
 * - Integrated into GameSystemsManager facade (20→21 managers)
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

// Forward declaration - existing auction house system
class AuctionHouse;

/**
 * @brief Auction listing strategy for profession materials/crafts
 */
enum class ProfessionAuctionStrategy : uint8
{
    NONE = 0,
    SELL_EXCESS,            // Sell materials above stockpile threshold
    SELL_ALL_GATHERED,      // Sell all gathered materials immediately
    SELL_CRAFTED_ONLY,      // Only sell crafted items, keep materials
    BALANCED,               // Balance between selling and stockpiling
    PROFIT_MAXIMIZATION     // Sell for maximum profit (wait for good prices)
};

/**
 * @brief Material stockpile target configuration
 */
struct MaterialStockpileConfig
{
    uint32 itemId;
    uint32 minStackSize;        // Keep at least this much in inventory
    uint32 maxStackSize;        // Start selling when inventory exceeds this
    uint32 auctionStackSize;    // Size of stacks to list on AH
    bool sellOnlyFull;          // Only sell full stacks
    bool preferBuyout;          // List with buyout price

    MaterialStockpileConfig()
        : itemId(0), minStackSize(20), maxStackSize(100),
          auctionStackSize(20), sellOnlyFull(false), preferBuyout(true) {}
};

/**
 * @brief Crafted item auction configuration
 */
struct CraftedItemAuctionConfig
{
    uint32 itemId;
    ProfessionType profession;
    uint32 minProfitMargin;     // Minimum profit in copper (materials + 20% etc.)
    uint32 maxListingDuration;  // 12h, 24h, 48h
    bool undercutCompetition;   // Undercut existing listings
    float undercutRate;         // Undercut by this percentage (0.05 = 5%)
    bool relistUnsold;          // Relist if not sold

    CraftedItemAuctionConfig()
        : itemId(0), profession(ProfessionType::NONE), minProfitMargin(5000),
          maxListingDuration(24 * 3600), undercutCompetition(true),
          undercutRate(0.05f), relistUnsold(true) {}
};

/**
 * @brief Profession auction bridge profile per bot
 */
struct ProfessionAuctionProfile
{
    bool autoSellEnabled = true;
    ProfessionAuctionStrategy strategy = ProfessionAuctionStrategy::BALANCED;
    uint32 auctionCheckInterval = 600000;   // Check every 10 minutes
    uint32 maxActiveAuctions = 20;          // Max profession-related auctions
    uint32 auctionBudget = 100000;          // Gold for buying materials (copper)
    bool buyMaterialsForLeveling = true;    // Auto-buy materials to level professions

    // Material management
    std::unordered_map<uint32, MaterialStockpileConfig> materialConfigs;

    // Crafted item management
    std::unordered_map<uint32, CraftedItemAuctionConfig> craftedItemConfigs;

    ProfessionAuctionProfile() = default;
};

/**
 * @brief Statistics for profession auction activity
 */
struct ProfessionAuctionStatistics
{
    std::atomic<uint32> materialsListedCount{0};
    std::atomic<uint32> materialsSoldCount{0};
    std::atomic<uint32> craftedsListedCount{0};
    std::atomic<uint32> craftedsSoldCount{0};
    std::atomic<uint32> goldEarnedFromMaterials{0};
    std::atomic<uint32> goldEarnedFromCrafts{0};
    std::atomic<uint32> goldSpentOnMaterials{0};
    std::atomic<uint32> materialsBought{0};

    void Reset()
    {
        materialsListedCount = 0;
        materialsSoldCount = 0;
        craftedsListedCount = 0;
        craftedsSoldCount = 0;
        goldEarnedFromMaterials = 0;
        goldEarnedFromCrafts = 0;
        goldSpentOnMaterials = 0;
        materialsBought = 0;
    }

    uint32 GetNetProfit() const
    {
        uint32 earned = goldEarnedFromMaterials.load() + goldEarnedFromCrafts.load();
        uint32 spent = goldSpentOnMaterials.load();
        return earned > spent ? earned - spent : 0;
    }
};

/**
 * @brief Bridge between profession system and auction house (per-bot instance)
 *
 * DESIGN PRINCIPLE: This class does NOT implement auction operations.
 * All auction operations are delegated to the existing AuctionHouse singleton.
 * This class only coordinates profession-specific auction logic.
 *
 * PHASE 4.3: Converted to per-bot instance pattern (owned by GameSystemsManager)
 */
class TC_GAME_API ProfessionAuctionBridge final
{
public:
    /**
     * @brief Construct bridge for specific bot
     * @param bot The bot this bridge serves (non-owning)
     */
    explicit ProfessionAuctionBridge(Player* bot);

    /**
     * @brief Destructor - unsubscribes from event bus
     */
    ~ProfessionAuctionBridge();

    // ============================================================================
    // CORE BRIDGE MANAGEMENT
    // ============================================================================

    /**
     * Initialize profession auction bridge (loads shared data)
     * NOTE: Called per-bot, but loads shared world data only once
     */
    void Initialize();

    /**
     * Update profession auction automation (called periodically)
     */
    void Update(::Player* player, uint32 diff);

    /**
     * Enable/disable profession auction automation
     */
    void SetEnabled(::Player* player, bool enabled);
    bool IsEnabled(::Player* player) const;

    /**
     * Set/get auction profile for this bot
     */
    void SetAuctionProfile(uint32 playerGuid, ProfessionAuctionProfile const& profile);
    ProfessionAuctionProfile GetAuctionProfile(uint32 playerGuid) const;

    // ============================================================================
    // MATERIAL AUCTION AUTOMATION
    // ============================================================================

    /**
     * Scan inventory for excess materials and list on auction house
     * Uses existing AuctionHouse::CreateAuction internally
     */
    void SellExcessMaterials(::Player* player);

    /**
     * Check if material should be sold based on stockpile config
     */
    bool ShouldSellMaterial(::Player* player, uint32 itemId, uint32 currentCount) const;

    /**
     * List material on auction house
     * Delegates to AuctionHouse::CreateAuction
     */
    bool ListMaterialOnAuction(::Player* player, uint32 itemGuid, MaterialStockpileConfig const& config);

    /**
     * Get optimal price for material based on market analysis
     * Uses AuctionHouse::CalculateOptimalListingPrice
     */
    uint32 GetOptimalMaterialPrice(::Player* player, uint32 itemId, uint32 stackSize) const;

    // ============================================================================
    // CRAFTED ITEM AUCTION AUTOMATION
    // ============================================================================

    /**
     * Scan inventory for crafted items and list profitable ones
     * Uses existing AuctionHouse::CreateAuction internally
     */
    void SellCraftedItems(::Player* player);

    /**
     * Check if crafted item should be sold based on profit margin
     */
    bool ShouldSellCraftedItem(::Player* player, uint32 itemId, uint32 materialCost) const;

    /**
     * List crafted item on auction house
     * Delegates to AuctionHouse::CreateAuction
     */
    bool ListCraftedItemOnAuction(::Player* player, uint32 itemGuid, CraftedItemAuctionConfig const& config);

    /**
     * Calculate profit margin for crafted item
     * = (sale_price - material_cost) / material_cost
     */
    float CalculateProfitMargin(::Player* player, uint32 itemId, uint32 marketPrice, uint32 materialCost) const;

    // ============================================================================
    // MATERIAL PURCHASING AUTOMATION
    // ============================================================================

    /**
     * Buy materials from auction house for profession leveling
     * Uses existing AuctionHouse::BuyoutAuction internally
     */
    void BuyMaterialsForLeveling(::Player* player, ProfessionType profession);

    /**
     * Get materials needed for next profession skill-up
     * Queries ProfessionManager for optimal leveling recipe
     */
    ::std::vector<::std::pair<uint32, uint32>> GetNeededMaterialsForLeveling(::Player* player, ProfessionType profession) const;

    /**
     * Check if material is available on auction house at good price
     * Uses AuctionHouse::GetMarketPrice and IsPriceBelowMarket
     */
    bool IsMaterialAvailableForPurchase(::Player* player, uint32 itemId, uint32 quantity, uint32 maxPricePerUnit) const;

    /**
     * Purchase specific material from auction house
     * Delegates to AuctionHouse::BuyoutAuction
     */
    bool PurchaseMaterial(::Player* player, uint32 itemId, uint32 quantity, uint32 maxPricePerUnit);

    // ============================================================================
    // STOCKPILE MANAGEMENT
    // ============================================================================

    /**
     * Configure material stockpile target
     */
    void SetMaterialStockpile(uint32 playerGuid, uint32 itemId, MaterialStockpileConfig const& config);

    /**
     * Configure crafted item auction behavior
     */
    void SetCraftedItemAuction(uint32 playerGuid, uint32 itemId, CraftedItemAuctionConfig const& config);

    /**
     * Get current stockpile status for material
     */
    uint32 GetCurrentStockpile(::Player* player, uint32 itemId) const;

    /**
     * Check if stockpile target is met
     */
    bool IsStockpileTargetMet(::Player* player, uint32 itemId) const;

    // ============================================================================
    // INTEGRATION WITH EXISTING AUCTION HOUSE
    // ============================================================================

    /**
     * Get reference to existing AuctionHouse singleton
     * All auction operations delegated here
     */
    AuctionHouse* GetAuctionHouse() const;

    /**
     * Synchronize with auction house session
     * Called when auction house operations are in progress
     */
    void SynchronizeWithAuctionHouse(::Player* player);

    // ============================================================================
    // STATISTICS
    // ============================================================================

    ProfessionAuctionStatistics const& GetPlayerStatistics(uint32 playerGuid) const;
    ProfessionAuctionStatistics const& GetGlobalStatistics() const;

    /**
     * Reset statistics for this bot
     */
    void ResetStatistics(uint32 playerGuid);

private:

    // ============================================================================
    // EVENT HANDLING (Phase 2)
    // ============================================================================

    /**
     * @brief Handle profession events from ProfessionEventBus
     * Reacts to CRAFTING_COMPLETED, ITEM_BANKED
     */
    void HandleProfessionEvent(struct ProfessionEvent const& event);

    // ============================================================================
    // INITIALIZATION HELPERS
    // ============================================================================

    void LoadDefaultStockpileConfigs();
    void InitializeMiningMaterials();
    void InitializeHerbalismMaterials();
    void InitializeSkinningMaterials();
    void InitializeCraftedItemConfigs();

    // ============================================================================
    // AUCTION HELPERS
    // ============================================================================

    /**
     * Get item info from inventory
     */
    struct ItemInfo
    {
        uint32 itemGuid;
        uint32 itemId;
        uint32 stackCount;
        uint32 quality;
    };
    std::vector<ItemInfo> GetProfessionItemsInInventory(bool materialsOnly = false) const;

    /**
     * Calculate material cost for crafted item
     */
    uint32 CalculateMaterialCost(uint32 itemId) const;

    /**
     * Check if item is profession-related material
     */
    bool IsProfessionMaterial(uint32 itemId) const;

    /**
     * Check if item is crafted by professions
     */
    bool IsCraftedItem(uint32 itemId, ProfessionType& outProfession) const;

    /**
     * Validate auction house access
     */
    bool CanAccessAuctionHouse() const;

    /**
     * Get ProfessionManager via GameSystemsManager
     */
    ProfessionManager* GetProfessionManager();

    // ============================================================================
    // PER-BOT DATA MEMBERS (Phase 4.3: Converted from maps)
    // ============================================================================

    Player* _bot;                                       // Bot player reference (non-owning)
    ProfessionAuctionProfile _profile;                  // Bot's auction profile
    uint32 _lastAuctionCheckTime{0};                    // Last auction check timestamp
    std::vector<uint32> _activeAuctionIds;              // Active auction IDs for tracking
    ProfessionAuctionStatistics _statistics;            // Per-bot statistics

    // ============================================================================
    // SHARED DATA MEMBERS (Static - shared across all bots)
    // ============================================================================

    // Reference to existing auction house (set in Initialize)
    static AuctionHouse* _auctionHouse;
    static ProfessionAuctionStatistics _globalStatistics;
    static bool _sharedDataInitialized;

    // Update intervals
    static constexpr uint32 AUCTION_CHECK_INTERVAL = 600000;        // 10 minutes
    static constexpr uint32 MATERIAL_SCAN_INTERVAL = 300000;        // 5 minutes
    static constexpr uint32 DEFAULT_AUCTION_DURATION = 24 * 3600;   // 24 hours

    // Non-copyable
    ProfessionAuctionBridge(ProfessionAuctionBridge const&) = delete;
    ProfessionAuctionBridge& operator=(ProfessionAuctionBridge const&) = delete;
};

} // namespace Playerbot
