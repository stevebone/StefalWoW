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
 *
 * REAGENT MANAGER - Auto-purchase consumables, reagents, and class materials
 *
 * Manages vendor restocking for bots:
 * - Food/Water for out-of-combat regeneration (level-appropriate)
 * - Class reagents (poisons, powders, etc.)
 * - Bandages for self-healing
 * - Repair awareness (plan visits to repair NPCs)
 * - Vendor NPC detection using TrinityCore grid search APIs
 *
 * Architecture:
 * - Global singleton accessed via sReagentManager macro
 * - All methods take Player* bot parameter (stateless per-bot)
 * - Thread-safe: read-only static data + per-call computation
 * - Periodic update checks throttled to reduce CPU overhead
 * - Integrates with VendorPurchaseManager for actual purchase execution
 * - Integrates with VendorInteractionManager for vendor item evaluation
 *
 * Performance Targets:
 * - NeedsRestock check: <0.1ms per bot
 * - GetRestockList: <0.5ms per bot
 * - AttemptPurchase: <1ms per vendor interaction
 * - Memory: <1KB static data (no per-bot allocation)
 */

#ifndef _PLAYERBOT_REAGENT_MANAGER_H
#define _PLAYERBOT_REAGENT_MANAGER_H

#include "Define.h"
#include "ObjectGuid.h"
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Player;
class Creature;
struct ItemTemplate;

namespace Playerbot
{

// ============================================================================
// RESTOCK CATEGORIES
// ============================================================================

enum class RestockCategory : uint8
{
    FOOD            = 0,    // Out-of-combat health regeneration food
    WATER           = 1,    // Out-of-combat mana regeneration drink
    CLASS_REAGENT   = 2,    // Class-specific reagents (poisons, powders, etc.)
    BANDAGE         = 3,    // First aid bandages for self-healing
    REPAIR          = 4,    // Equipment repair (not an item purchase, but tracked)
    MAX_CATEGORY
};

// ============================================================================
// RESTOCK DATA STRUCTURES
// ============================================================================

/**
 * @struct RestockItem
 * @brief Describes a single item that needs restocking
 */
struct RestockItem
{
    uint32 itemId = 0;                          ///< Item template entry
    uint32 currentCount = 0;                    ///< How many the bot currently has
    uint32 desiredCount = 0;                    ///< How many the bot should have
    uint32 purchaseQuantity = 0;                ///< How many to buy (desiredCount - currentCount)
    RestockCategory category = RestockCategory::FOOD;
    std::string name;                           ///< Item name for logging
};

/**
 * @struct RestockPlan
 * @brief Complete restocking plan for a bot at a vendor
 */
struct RestockPlan
{
    std::vector<RestockItem> items;             ///< Items to purchase
    uint64 estimatedCost = 0;                   ///< Total estimated gold cost (copper)
    bool needsRepair = false;                   ///< Whether equipment needs repair
    float lowestDurabilityPct = 100.0f;         ///< Lowest durability percentage across gear
    uint64 estimatedRepairCost = 0;             ///< Estimated repair cost (copper)

    [[nodiscard]] bool IsEmpty() const { return items.empty() && !needsRepair; }
    [[nodiscard]] uint32 TotalItemsToBuy() const
    {
        uint32 total = 0;
        for (auto const& item : items)
            total += item.purchaseQuantity;
        return total;
    }
};

/**
 * @struct VendorSearchResult
 * @brief Result of searching for the nearest vendor NPC
 */
struct VendorSearchResult
{
    Creature* vendor = nullptr;                 ///< Found vendor creature (nullptr if none)
    float distance = 0.0f;                      ///< Distance to vendor in yards
    bool canRepair = false;                     ///< Whether vendor can repair equipment
    bool canSellFood = false;                   ///< Whether vendor sells food/drink
    bool canSellReagents = false;               ///< Whether vendor sells reagents
};

/**
 * @struct ReagentManagerConfig
 * @brief Configurable thresholds for restocking behavior
 */
struct ReagentManagerConfig
{
    uint32 minFoodCount = 20;                   ///< Restock food when below this count
    uint32 minWaterCount = 20;                  ///< Restock water when below this count
    uint32 minReagentCount = 20;                ///< Restock reagents when below this count
    uint32 minBandageCount = 10;                ///< Restock bandages when below this count
    uint32 targetFoodCount = 40;                ///< Target food count after restocking
    uint32 targetWaterCount = 40;               ///< Target water count after restocking
    uint32 targetReagentCount = 40;             ///< Target reagent count after restocking
    uint32 targetBandageCount = 20;             ///< Target bandage count after restocking
    float repairThresholdPct = 25.0f;           ///< Repair when durability below this %
    float vendorSearchRange = 100.0f;           ///< Range to search for vendor NPCs (yards)
    uint32 updateIntervalMs = 10000;            ///< How often to check restock status (ms)
    uint64 goldReserve = 50000;                 ///< Reserve this much gold for emergencies (copper, 5g)
};

// ============================================================================
// LEVEL-BASED ITEM DATA
// ============================================================================

/**
 * @struct LevelBracketItem
 * @brief Maps a level range to an appropriate item ID
 */
struct LevelBracketItem
{
    uint32 minLevel;    ///< Minimum player level (inclusive)
    uint32 maxLevel;    ///< Maximum player level (inclusive)
    uint32 itemId;      ///< Item template entry
    const char* name;   ///< Item name for logging
};

// ============================================================================
// REAGENT MANAGER - SINGLETON
// ============================================================================

/**
 * @class ReagentManager
 * @brief Manages auto-purchase of consumables, reagents, and class materials for bots
 *
 * This is a global singleton that provides stateless per-bot restocking services.
 * All state is computed on-demand from the Player object; no per-bot data is stored.
 *
 * Usage:
 * @code
 * // Check if a bot needs restocking
 * if (sReagentManager->NeedsRestock(bot))
 * {
 *     // Find nearest vendor
 *     VendorSearchResult vendor = sReagentManager->GetNearestVendor(bot);
 *     if (vendor.vendor)
 *     {
 *         // Purchase needed items
 *         sReagentManager->AttemptPurchase(bot, vendor.vendor);
 *     }
 * }
 *
 * // Or in periodic update
 * sReagentManager->Update(bot, diff);
 * @endcode
 *
 * Thread Safety:
 * - Static data tables are initialized once and never modified (inherently thread-safe)
 * - Config is protected by shared_mutex (read-heavy, rare writes)
 * - All per-bot methods only read from the Player object (thread-safe if called
 *   from the bot's own update context, which is the expected usage pattern)
 *
 * Performance:
 * - NeedsRestock: O(k) where k = number of tracked items for class (~5-10 items)
 * - GetRestockList: O(k) same complexity
 * - GetNearestVendor: O(n) grid search, bounded by vendorSearchRange
 * - No per-bot memory allocation
 */
class TC_GAME_API ReagentManager
{
public:
    ReagentManager();
    ~ReagentManager() = default;

    // Non-copyable, non-movable (singleton)
    ReagentManager(const ReagentManager&) = delete;
    ReagentManager& operator=(const ReagentManager&) = delete;
    ReagentManager(ReagentManager&&) = delete;
    ReagentManager& operator=(ReagentManager&&) = delete;

    // ========================================================================
    // SINGLETON ACCESS
    // ========================================================================

    /**
     * @brief Get the singleton instance
     * @return Pointer to the global ReagentManager
     */
    static ReagentManager* instance();

    // ========================================================================
    // CORE RESTOCK QUERIES
    // ========================================================================

    /**
     * @brief Check if the bot needs any restocking
     * @param bot The bot player to check
     * @return true if any tracked consumable is below threshold or gear needs repair
     *
     * This is a lightweight check intended for frequent polling.
     * It checks food, water, reagents, bandages, and durability.
     */
    [[nodiscard]] bool NeedsRestock(Player* bot) const;

    /**
     * @brief Get the complete list of items to restock
     * @param bot The bot player to evaluate
     * @return RestockPlan containing all items to buy and repair status
     *
     * Builds a full restocking plan based on the bot's class, level, and
     * current inventory. Does NOT include items the bot cannot use.
     */
    [[nodiscard]] RestockPlan GetRestockList(Player* bot) const;

    /**
     * @brief Attempt to purchase all needed items from a vendor
     * @param bot The bot player doing the purchasing
     * @param vendor The vendor creature to buy from
     * @return Number of items successfully purchased (0 if vendor has nothing needed)
     *
     * Executes the full purchase flow:
     * 1. Generate restock plan
     * 2. Check what the vendor actually sells
     * 3. Verify gold and bag space
     * 4. Purchase items via Player::BuyItemFromVendorSlot
     * 5. Repair equipment if vendor is an armorer
     */
    uint32 AttemptPurchase(Player* bot, Creature* vendor);

    // ========================================================================
    // VENDOR PLANNING
    // ========================================================================

    /**
     * @brief Determine if the bot should actively seek a vendor
     * @param bot The bot player to evaluate
     * @return true if restocking or repair is urgent enough to justify travel
     *
     * Returns true when:
     * - Food/water is critically low (below half of threshold)
     * - Equipment durability is below repair threshold
     * - Class reagents are depleted
     */
    [[nodiscard]] bool ShouldVisitVendor(Player* bot) const;

    /**
     * @brief Find the nearest vendor NPC to the bot
     * @param bot The bot player to search around
     * @return VendorSearchResult with the nearest vendor and capabilities
     *
     * Searches for nearby NPCs with vendor flags using TrinityCore's
     * grid-based creature search. Prefers vendors that can both sell
     * items and repair equipment.
     */
    [[nodiscard]] VendorSearchResult GetNearestVendor(Player* bot) const;

    // ========================================================================
    // PERIODIC UPDATE
    // ========================================================================

    /**
     * @brief Periodic restock check for integration into bot update loops
     * @param bot The bot player to update
     * @param diff Time since last call in milliseconds
     *
     * Throttled update that:
     * 1. Checks if bot needs restocking (throttled by updateIntervalMs)
     * 2. If near a vendor, auto-purchases needed items
     * 3. Logs restock activity
     *
     * This method maintains a per-bot timer using an internal map
     * protected by a mutex for thread safety.
     */
    void Update(Player* bot, uint32 diff);

    // ========================================================================
    // ITEM KNOWLEDGE
    // ========================================================================

    /**
     * @brief Get the appropriate food item ID for a bot's level
     * @param level The bot's current level
     * @return Item template entry for level-appropriate food, or 0 if none
     */
    [[nodiscard]] uint32 GetFoodForLevel(uint32 level) const;

    /**
     * @brief Get the appropriate water item ID for a bot's level
     * @param level The bot's current level
     * @return Item template entry for level-appropriate water, or 0 if none
     */
    [[nodiscard]] uint32 GetWaterForLevel(uint32 level) const;

    /**
     * @brief Get the appropriate bandage item ID for a bot's level
     * @param level The bot's current level
     * @return Item template entry for level-appropriate bandage, or 0 if none
     */
    [[nodiscard]] uint32 GetBandageForLevel(uint32 level) const;

    /**
     * @brief Get the list of class-specific reagent item IDs
     * @param classId The bot's class (CLASS_WARRIOR, CLASS_MAGE, etc.)
     * @return Vector of item IDs for required reagents (empty if class has none)
     */
    [[nodiscard]] std::vector<uint32> GetClassReagents(uint8 classId) const;

    // ========================================================================
    // DURABILITY QUERIES
    // ========================================================================

    /**
     * @brief Calculate the lowest durability percentage across all equipped items
     * @param bot The bot player to check
     * @return Lowest durability percentage (0-100), or 100.0 if no items have durability
     */
    [[nodiscard]] float GetLowestDurabilityPct(Player* bot) const;

    /**
     * @brief Check if the bot's equipment needs repair
     * @param bot The bot player to check
     * @return true if any equipped item is below the repair threshold
     */
    [[nodiscard]] bool NeedsRepair(Player* bot) const;

    /**
     * @brief Estimate the total repair cost for all equipped items
     * @param bot The bot player to check
     * @return Estimated repair cost in copper
     */
    [[nodiscard]] uint64 EstimateRepairCost(Player* bot) const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Get the current configuration (thread-safe read)
     * @return Copy of the current configuration
     */
    [[nodiscard]] ReagentManagerConfig GetConfig() const;

    /**
     * @brief Update the configuration (thread-safe write)
     * @param config New configuration to apply
     */
    void SetConfig(ReagentManagerConfig const& config);

private:
    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================

    /**
     * @brief Check if bot needs food restocking
     * @param bot The bot player
     * @param config Current config snapshot
     * @return true if food count is below threshold
     */
    [[nodiscard]] bool NeedsFoodRestock(Player* bot, ReagentManagerConfig const& config) const;

    /**
     * @brief Check if bot needs water restocking (mana-using classes only)
     * @param bot The bot player
     * @param config Current config snapshot
     * @return true if water count is below threshold
     */
    [[nodiscard]] bool NeedsWaterRestock(Player* bot, ReagentManagerConfig const& config) const;

    /**
     * @brief Check if bot needs class reagent restocking
     * @param bot The bot player
     * @param config Current config snapshot
     * @return true if any class reagent is below threshold
     */
    [[nodiscard]] bool NeedsReagentRestock(Player* bot, ReagentManagerConfig const& config) const;

    /**
     * @brief Check if bot needs bandage restocking
     * @param bot The bot player
     * @param config Current config snapshot
     * @return true if bandage count is below threshold
     */
    [[nodiscard]] bool NeedsBandageRestock(Player* bot, ReagentManagerConfig const& config) const;

    /**
     * @brief Check if a bot class uses mana
     * @param classId The class to check
     * @return true if the class has a mana resource
     */
    [[nodiscard]] bool ClassUsesMana(uint8 classId) const;

    /**
     * @brief Try to buy a specific item from a vendor
     * @param bot The bot player
     * @param vendor The vendor creature
     * @param itemId Item to purchase
     * @param quantity Number to buy
     * @return true if purchase was successful
     */
    bool TryPurchaseItem(Player* bot, Creature* vendor, uint32 itemId, uint32 quantity) const;

    /**
     * @brief Find an item in the vendor's inventory and return its slot
     * @param vendor The vendor creature
     * @param itemId Item to find
     * @return Vendor slot index, or UINT32_MAX if not found
     */
    [[nodiscard]] uint32 FindVendorSlotForItem(Creature* vendor, uint32 itemId) const;

    /**
     * @brief Calculate the purchase price for an item from a vendor
     * @param bot The bot player (for reputation discount)
     * @param vendor The vendor creature
     * @param itemId Item to price
     * @param quantity Number of items
     * @return Total cost in copper
     */
    [[nodiscard]] uint64 CalculateItemCost(Player* bot, Creature* vendor, uint32 itemId, uint32 quantity) const;

    // ========================================================================
    // STATIC ITEM DATA
    // ========================================================================

    /**
     * @brief Get the food database (level-bracketed)
     * @return Reference to static food item table
     */
    static std::vector<LevelBracketItem> const& GetFoodTable();

    /**
     * @brief Get the water database (level-bracketed)
     * @return Reference to static water item table
     */
    static std::vector<LevelBracketItem> const& GetWaterTable();

    /**
     * @brief Get the bandage database (level-bracketed)
     * @return Reference to static bandage item table
     */
    static std::vector<LevelBracketItem> const& GetBandageTable();

    /**
     * @brief Get the class reagent database
     * @return Reference to static map of classId -> list of reagent item IDs
     */
    static std::unordered_map<uint8, std::vector<uint32>> const& GetClassReagentTable();

    // ========================================================================
    // MEMBER DATA
    // ========================================================================

    /// Configuration protected by shared mutex (read-heavy pattern)
    mutable std::shared_mutex _configMutex;
    ReagentManagerConfig _config;

    /// Per-bot update timers (botGUID -> accumulated time ms)
    /// Protected by regular mutex since it's write-on-every-update
    mutable std::mutex _timerMutex;
    mutable std::unordered_map<uint64, uint32> _updateTimers;
};

} // namespace Playerbot

#define sReagentManager Playerbot::ReagentManager::instance()

#endif // _PLAYERBOT_REAGENT_MANAGER_H
