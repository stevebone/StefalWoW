/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <vector>
#include <unordered_map>

class Player;
class Creature;
class Item;
struct ItemTemplate;
struct VendorItem;

namespace Playerbot
{

/**
 * @class VendorInteractionManager
 * @brief Manages all vendor purchase interactions for player bots
 *
 * This class provides complete vendor purchase functionality using TrinityCore's
 * vendor system APIs. It handles:
 * - Vendor item lookup and validation
 * - Purchase execution with proper gold/currency handling
 * - Priority-based smart purchasing (reagents > consumables > equipment > luxury)
 * - Budget management with repair cost reservation
 * - Bag space validation
 * - Extended cost support (badges, honor, etc.)
 *
 * Performance Target: <1ms per purchase decision
 * Memory Target: <50KB overhead
 *
 * TrinityCore API Integration:
 * - Creature::GetVendorItems() - Vendor inventory access
 * - Player::BuyItemFromVendorSlot() - Purchase execution
 * - VendorItem structure - Item data with pricing
 * - Player::GetMoney() - Gold validation
 * - Player::IsSpaceForItem() - Inventory validation
 */
class VendorInteractionManager
{
public:
    /**
     * @brief Purchase priority levels
     *
     * Controls the order in which items are purchased when budget is limited:
     * - CRITICAL: Essential class reagents (rogue poison, warlock soul shards)
     * - HIGH: Combat consumables (food, water, ammo for hunters)
     * - MEDIUM: Equipment upgrades (better gear for current level)
     * - LOW: Luxury items (cosmetic, convenience items)
     */
    enum class PurchasePriority : uint8
    {
        CRITICAL = 0,  // Must-have reagents for class abilities
        HIGH     = 1,  // Food, water, ammo
        MEDIUM   = 2,  // Equipment upgrades
        LOW      = 3   // Luxury/convenience items
    };

    /**
     * @brief Vendor purchase request structure
     *
     * Encapsulates all data needed to execute a vendor purchase transaction
     */
    struct VendorPurchaseRequest
    {
        uint32 vendorEntry;       // Vendor NPC entry ID
        uint32 itemId;            // Item template ID to purchase
        uint32 quantity;          // Number of items to buy (for stackable items)
        uint32 vendorSlot;        // Slot index in vendor's inventory
        uint64 goldCost;          // Gold cost in copper
        uint32 extendedCostId;    // Extended cost ID (0 if gold only)
        PurchasePriority priority; // Purchase priority level

        VendorPurchaseRequest()
            : vendorEntry(0), itemId(0), quantity(1), vendorSlot(0)
            , goldCost(0), extendedCostId(0), priority(PurchasePriority::LOW)
        { }
    };

    /**
     * @brief Vendor item evaluation result
     *
     * Contains analysis of whether and why an item should be purchased
     */
    struct VendorItemEvaluation
    {
        uint32 itemId;
        uint32 vendorSlot;
        PurchasePriority priority;
        uint64 goldCost;
        uint32 extendedCostId;
        uint32 recommendedQuantity;
        bool shouldPurchase;
        ::std::string reason;  // Human-readable reason for decision

        VendorItemEvaluation()
            : itemId(0), vendorSlot(0), priority(PurchasePriority::LOW)
            , goldCost(0), extendedCostId(0), recommendedQuantity(0)
            , shouldPurchase(false)
        { }
    };

    /**
     * @brief Budget allocation for different purchase categories
     *
     * Manages how available gold is distributed across purchase priorities
     */
    struct BudgetAllocation
    {
        uint64 totalAvailable;     // Total gold available
        uint64 reservedForRepairs; // Gold reserved for equipment repairs
        uint64 criticalBudget;     // Budget for CRITICAL items
        uint64 highBudget;         // Budget for HIGH priority items
        uint64 mediumBudget;       // Budget for MEDIUM priority items
        uint64 lowBudget;          // Budget for LOW priority items

        BudgetAllocation()
            : totalAvailable(0), reservedForRepairs(0)
            , criticalBudget(0), highBudget(0), mediumBudget(0), lowBudget(0)
        { }
    };

    VendorInteractionManager(Player* bot);
    ~VendorInteractionManager() = default;

    // Core Purchase Methods

    /**
     * @brief Purchase a single item from a vendor
     * @param vendor Vendor creature to purchase from
     * @param itemId Item template ID to purchase
     * @param quantity Number of items to purchase (default 1)
     * @return True if purchase successful
     *
     * This method performs the complete purchase transaction:
     * 1. Validates vendor has the item
     * 2. Checks gold/currency costs
     * 3. Validates bag space
     * 4. Executes purchase via Player::BuyItemFromVendorSlot()
     * 5. Updates statistics
     */
    bool PurchaseItem(Creature* vendor, uint32 itemId, uint32 quantity = 1);

    /**
     * @brief Purchase multiple items from a vendor based on priority
     * @param vendor Vendor creature
     * @param itemIds List of item IDs to consider purchasing
     * @return Number of items successfully purchased
     *
     * Evaluates all requested items, prioritizes based on bot needs,
     * respects budget constraints, and purchases in priority order.
     */
    uint32 PurchaseItems(Creature* vendor, ::std::vector<uint32> const& itemIds);

    /**
     * @brief Smart purchase - automatically determines what to buy
     * @param vendor Vendor creature
     * @return Number of items successfully purchased
     *
     * Analyzes vendor inventory, determines bot needs (reagents, food, etc.),
     * creates optimal purchase plan within budget, and executes purchases.
     */
    uint32 SmartPurchase(Creature* vendor);

    // Vendor Analysis Methods

    /**
     * @brief Get vendor's available items with pricing
     * @param vendor Vendor creature
     * @return Vector of vendor items (uses Creature::GetVendorItems())
     */
    ::std::vector<VendorItem const*> GetVendorItems(Creature* vendor) const;

    /**
     * @brief Evaluate whether an item should be purchased
     * @param vendor Vendor selling the item
     * @param item Item to evaluate
     * @param vendorSlot Slot index in vendor inventory
     * @return Evaluation result with priority and reasoning
     */
    VendorItemEvaluation EvaluateVendorItem(Creature* vendor, ItemTemplate const* item, uint32 vendorSlot) const;

    /**
     * @brief Calculate item priority based on bot needs
     * @param item Item template to evaluate
     * @return Priority level (CRITICAL/HIGH/MEDIUM/LOW)
     */
    PurchasePriority CalculateItemPriority(ItemTemplate const* item) const;

    /**
     * @brief Check if bot can afford an item
     * @param goldCost Gold cost in copper
     * @param extendedCostId Extended cost ID (0 for gold only)
     * @return True if bot has sufficient resources
     */
    bool CanAfford(uint64 goldCost, uint32 extendedCostId = 0) const;

    /**
     * @brief Get item's vendor price
     * @param vendor Vendor creature
     * @param itemId Item template ID
     * @param vendorSlot Vendor slot index
     * @param quantity Number of items (for stackable items)
     * @return Total gold cost in copper
     */
    uint64 GetVendorPrice(Creature* vendor, uint32 itemId, uint32 vendorSlot, uint32 quantity = 1) const;

    // Budget Management Methods

    /**
     * @brief Calculate available budget with repair reservation
     * @return Budget allocation structure
     *
     * Allocates available gold across priority categories:
     * - Reserves gold for repairs (if needed)
     * - Allocates remaining gold by priority (50% critical, 30% high, 15% medium, 5% low)
     */
    BudgetAllocation CalculateBudget() const;

    /**
     * @brief Calculate estimated repair costs
     * @return Estimated gold needed for repairs
     *
     * Examines equipment durability and calculates repair costs
     * to ensure gold is reserved for essential maintenance.
     */
    uint64 CalculateRepairCostEstimate() const;

    /**
     * @brief Check if purchase fits within budget
     * @param goldCost Item cost
     * @param priority Item priority
     * @param budget Current budget allocation
     * @return True if purchase fits budget
     */
    bool FitsWithinBudget(uint64 goldCost, PurchasePriority priority, BudgetAllocation const& budget) const;

    // Reagent and Consumable Methods

    /**
     * @brief Get list of required class reagents
     * @return Vector of required reagent item IDs
     *
     * Returns class-specific reagents needed for abilities:
     * - Rogue: Poisons, Blinding Powder
     * - Warlock: Soul Shards
     * - Mage: Arcane Powder, Runes
     * - Priest: Holy Candles
     * - Shaman: Ankhs
     * - Druid: Wild Thornroot
     */
    ::std::vector<uint32> GetRequiredReagents() const;

    /**
     * @brief Get required food/water for bot level
     * @return Vector of appropriate consumable item IDs
     *
     * Returns level-appropriate food and water:
     * - Level 1-5: Refreshing Spring Water, Tough Jerky
     * - Level 5-15: Still Water, Haunch of Meat
     * - etc. (scaled by level)
     */
    ::std::vector<uint32> GetRequiredConsumables() const;

    /**
     * @brief Check if bot needs ammunition (hunters only)
     * @return True if hunter needs ammo
     */
    bool NeedsAmmunition() const;

    /**
     * @brief Get appropriate ammunition for hunter
     * @return Item ID of suitable ammo, or 0 if not hunter
     */
    uint32 GetAppropriateAmmunition() const;

    // Inventory Validation

    /**
     * @brief Check if bot has bag space for item
     * @param itemId Item template ID
     * @param quantity Number of items
     * @return True if sufficient bag space
     */
    bool HasBagSpace(uint32 itemId, uint32 quantity = 1) const;

    /**
     * @brief Get number of free bag slots
     * @return Count of empty bag slots
     */
    uint32 GetFreeBagSlots() const;

    // Statistics and Performance

    struct Statistics
    {
        uint32 itemsPurchased;
        uint64 totalGoldSpent;
        uint32 purchaseAttempts;
        uint32 purchaseFailures;
        uint32 insufficientGold;
        uint32 noBagSpace;
        uint32 vendorOutOfStock;

        Statistics()
            : itemsPurchased(0), totalGoldSpent(0), purchaseAttempts(0)
            , purchaseFailures(0), insufficientGold(0), noBagSpace(0)
            , vendorOutOfStock(0)
        { }
    };

    Statistics const& GetStatistics() const { return m_stats; }
    void ResetStatistics() { m_stats = Statistics(); }

    float GetCPUUsage() const { return m_cpuUsage; }
    size_t GetMemoryUsage() const;

private:
    // Internal Helper Methods

    /**
     * @brief Find vendor item by item ID
     * @param vendor Vendor creature
     * @param itemId Item template ID
     * @return Pointer to VendorItem, or nullptr if not found
     */
    VendorItem const* FindVendorItem(Creature* vendor, uint32 itemId) const;

    /**
     * @brief Execute purchase transaction via TrinityCore API
     * @param vendor Vendor creature
     * @param vendorSlot Slot in vendor inventory
     * @param itemId Item template ID
     * @param quantity Number to purchase
     * @param bag Destination bag (255 = auto)
     * @param slot Destination slot (0 = auto)
     * @return True if purchase successful
     */
    bool ExecutePurchase(Creature* vendor, uint32 vendorSlot, uint32 itemId, uint32 quantity, uint8 bag, uint8 slot);

    /**
     * @brief Check if item is a class-specific reagent
     */
    bool IsClassReagent(ItemTemplate const* item) const;

    /**
     * @brief Check if item is food or water
     */
    bool IsConsumable(ItemTemplate const* item) const;

    /**
     * @brief Check if item is equipment upgrade
     */
    bool IsEquipmentUpgrade(ItemTemplate const* item) const;

    /**
     * @brief Get recommended purchase quantity for stackable items
     */
    uint32 GetRecommendedQuantity(ItemTemplate const* item) const;

    /**
     * @brief Record purchase in statistics
     */
    void RecordPurchase(uint32 itemId, uint64 goldCost, bool success);

    // Member Variables
    Player* m_bot;
    Statistics m_stats;

    // Performance Tracking
    float m_cpuUsage;
    uint32 m_totalPurchaseTime; // microseconds
    uint32 m_purchaseCount;

    // Cache for frequently accessed data
    ::std::unordered_map<uint32, PurchasePriority> m_priorityCache;
    ::std::unordered_map<uint32, uint64> m_priceCache;
};

} // namespace Playerbot
