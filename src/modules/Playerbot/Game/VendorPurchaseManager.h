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

#ifndef _PLAYERBOT_VENDOR_PURCHASE_MANAGER_H
#define _PLAYERBOT_VENDOR_PURCHASE_MANAGER_H

#include "Common.h"
#include "ObjectGuid.h"
#include <memory>
#include <optional>
#include <vector>

class Player;
class Creature;
class ItemTemplate;
struct VendorItem;
struct VendorItemData;

namespace Playerbot
{
    /**
     * @enum VendorPurchaseResult
     * @brief Result codes for vendor purchase operations
     */
    enum class VendorPurchaseResult
    {
        SUCCESS = 0,                    ///< Purchase completed successfully
        VENDOR_NOT_FOUND,               ///< Vendor NPC not found in world
        NOT_A_VENDOR,                   ///< Target NPC is not a vendor
        OUT_OF_RANGE,                   ///< Player too far from vendor
        ITEM_NOT_FOUND,                 ///< Item not in vendor inventory
        INSUFFICIENT_GOLD,              ///< Not enough gold/currency
        INSUFFICIENT_CURRENCY,          ///< Missing required currency
        INVENTORY_FULL,                 ///< No inventory space
        ITEM_SOLD_OUT,                  ///< Limited stock exhausted
        REPUTATION_TOO_LOW,             ///< Reputation requirement not met
        LEVEL_TOO_LOW,                  ///< Level requirement not met
        CLASS_RESTRICTION,              ///< Wrong class for item
        FACTION_RESTRICTION,            ///< Wrong faction for item
        ACHIEVEMENT_REQUIRED,           ///< Missing required achievement
        CONDITION_NOT_MET,              ///< Player condition not satisfied
        PURCHASE_FAILED                 ///< Generic purchase failure
    };

    /**
     * @enum ItemPurchasePriority
     * @brief Priority levels for item purchases
     */
    enum class ItemPurchasePriority
    {
        CRITICAL = 0,                   ///< Essential items (food, water, reagents)
        HIGH,                           ///< Important items (gear upgrades, consumables)
        MEDIUM,                         ///< Useful items (trade goods, misc)
        LOW,                            ///< Optional items (vanity, pets)
        NONE                            ///< No priority (skip)
    };

    /**
     * @struct VendorPurchaseRequest
     * @brief Request to purchase item from vendor
     */
    struct VendorPurchaseRequest
    {
        ObjectGuid vendorGuid;          ///< Vendor NPC GUID
        uint32 itemId;                  ///< Item ID to purchase
        uint32 quantity;                ///< Quantity to buy
        ItemPurchasePriority priority;  ///< Purchase priority
        uint32 maxGoldCost;             ///< Maximum gold willing to spend (copper)
        bool allowExtendedCost;         ///< Allow currency/token purchases
        bool autoEquip;                 ///< Auto-equip if better than current gear

        VendorPurchaseRequest()
            : itemId(0), quantity(1), priority(ItemPurchasePriority::MEDIUM),
              maxGoldCost(0), allowExtendedCost(false), autoEquip(false) {}
    };

    /**
     * @struct VendorPurchaseRecommendation
     * @brief Recommended item to purchase from vendor
     */
    struct VendorPurchaseRecommendation
    {
        uint32 itemId;                  ///< Item ID
        uint32 vendorSlot;              ///< Slot in vendor inventory
        uint32 suggestedQuantity;       ///< Recommended purchase quantity
        ItemPurchasePriority priority;  ///< Purchase priority
        uint64 goldCost;                ///< Total gold cost (after discounts)
        bool isUpgrade;                 ///< True if item is gear upgrade
        float upgradeScore;             ///< Upgrade score (0-100, higher = better)
        ::std::string reason;             ///< Purchase reason (debugging)

        VendorPurchaseRecommendation()
            : itemId(0), vendorSlot(0), suggestedQuantity(1),
              priority(ItemPurchasePriority::NONE), goldCost(0),
              isUpgrade(false), upgradeScore(0.0f) {}
    };

    /**
     * @class VendorPurchaseManager
     * @brief High-performance vendor purchase system for bots
     *
     * Purpose:
     * - Automate smart vendor purchases for bot leveling and progression
     * - Identify gear upgrades, consumables, and essential items
     * - Manage gold budgets and purchase priorities
     * - Integrate with TrinityCore's vendor system (Player::BuyItemFromVendorSlot)
     *
     * Features:
     * - Automatic gear upgrade detection (item level, stat weights)
     * - Consumable restocking (food, water, reagents, ammo)
     * - Budget management (gold limits per purchase session)
     * - Priority-based purchasing (critical items first)
     * - Extended cost support (reputation tokens, currencies)
     * - Reputation discount calculation
     *
     * Performance Targets:
     * - Vendor inventory analysis: < 5ms for 100 items
     * - Purchase decision: < 1ms per item
     * - Memory: < 1KB per active vendor session
     *
     * Quality Standards:
     * - NO shortcuts - Full Player::BuyItemFromVendorSlot integration
     * - Complete error handling and validation
     * - Production-ready code (no TODOs)
     *
     * @code
     * // Example usage:
     * VendorPurchaseManager mgr;
     * auto recommendations = mgr.GetPurchaseRecommendations(bot, vendor, 10000); // 1 gold budget
     *
     * for (auto const& rec : recommendations)
     * {
     *     if (rec.priority == ItemPurchasePriority::CRITICAL)
     *     {
     *         VendorPurchaseRequest request;
     *         request.vendorGuid = vendor->GetGUID();
     *         request.itemId = rec.itemId;
     *         request.quantity = rec.suggestedQuantity;
     *
     *         VendorPurchaseResult result = mgr.PurchaseItem(bot, request);
     *         if (result == VendorPurchaseResult::SUCCESS)
     *             TC_LOG_DEBUG("playerbot", "Purchased {}", rec.reason);
     *     }
     * }
     * @endcode
     */
    class VendorPurchaseManager
    {
    public:
        VendorPurchaseManager() = default;
        ~VendorPurchaseManager() = default;

        /// Delete copy/move (stateless utility class)
        VendorPurchaseManager(VendorPurchaseManager const&) = delete;
        VendorPurchaseManager(VendorPurchaseManager&&) = delete;
        VendorPurchaseManager& operator=(VendorPurchaseManager const&) = delete;
        VendorPurchaseManager& operator=(VendorPurchaseManager&&) = delete;

        /**
         * @brief Purchases item from vendor using TrinityCore API
         *
         * Workflow:
         * 1. Validate vendor and player
         * 2. Find item in vendor inventory
         * 3. Validate purchase requirements (gold, level, reputation)
         * 4. Call Player::BuyItemFromVendorSlot()
         * 5. Return result code
         *
         * @param player Bot making purchase
         * @param request Purchase request details
         * @return Result code indicating success or failure reason
         *
         * Performance: < 1ms typical
         * Thread-safety: Main thread only (modifies player inventory)
         */
        [[nodiscard]] VendorPurchaseResult PurchaseItem(
            Player* player,
            VendorPurchaseRequest const& request);

        /**
         * @brief Gets recommended items to purchase from vendor
         *
         * Analyzes vendor inventory and recommends purchases based on:
         * - Gear upgrades (item level, stat weights)
         * - Consumables (food, water, potions, reagents)
         * - Class-specific items (ammo, totems, souls shards)
         * - Budget constraints
         *
         * @param player Bot analyzing vendor
         * @param vendor Vendor NPC
         * @param goldBudget Maximum gold to spend (copper)
         * @return Vector of purchase recommendations (sorted by priority)
         *
         * Performance: < 5ms for 100 vendor items
         * Thread-safety: Thread-safe (read-only)
         */
        [[nodiscard]] ::std::vector<VendorPurchaseRecommendation> GetPurchaseRecommendations(
            Player const* player,
            Creature const* vendor,
            uint64 goldBudget) const;

        /**
         * @brief Finds vendor slot index for item ID
         *
         * @param vendor Vendor creature
         * @param itemId Item ID to find
         * @return Vendor slot index, or std::nullopt if not found
         *
         * Performance: O(n) where n = vendor items, ~0.1ms typical
         * Thread-safety: Thread-safe (read-only)
         */
        [[nodiscard]] static ::std::optional<uint32> FindVendorSlot(
            Creature const* vendor,
            uint32 itemId);

        /**
         * @brief Checks if item is an upgrade for player
         *
         * @param player Player to check for
         * @param itemTemplate Item to evaluate
         * @param[out] upgradeScore Score 0-100 (higher = better upgrade)
         * @return true if item is an upgrade
         *
         * Performance: < 0.5ms (stat weight calculation)
         * Thread-safety: Thread-safe (read-only)
         */
        [[nodiscard]] static bool IsItemUpgrade(
            Player const* player,
            ItemTemplate const* itemTemplate,
            float& upgradeScore);

        /**
         * @brief Calculates item purchase priority for bot
         *
         * Priority determination:
         * - CRITICAL: Food/water below 20%, reagents depleted
         * - HIGH: Gear upgrades, essential consumables
         * - MEDIUM: Quality-of-life items, trade goods
         * - LOW: Vanity items, pets
         *
         * @param player Bot evaluating item
         * @param itemTemplate Item to evaluate
         * @return Priority level
         *
         * Performance: < 0.1ms
         * Thread-safety: Thread-safe (read-only)
         */
        [[nodiscard]] static ItemPurchasePriority CalculateItemPriority(
            Player const* player,
            ItemTemplate const* itemTemplate);

        /**
         * @brief Gets human-readable error message for purchase result
         *
         * @param result Purchase result code
         * @return String description
         */
        [[nodiscard]] static char const* GetResultString(VendorPurchaseResult result);

    private:
        /**
         * @brief Validates purchase request before execution
         *
         * Checks:
         * - Vendor exists and is accessible
         * - Item exists in vendor inventory
         * - Player meets requirements (gold, level, reputation)
         * - Inventory space available
         *
         * @param player Player making purchase
         * @param vendor Vendor NPC
         * @param vendorSlot Slot index in vendor inventory
         * @param itemId Item ID
         * @param quantity Purchase quantity
         * @return Result code (SUCCESS if valid, error otherwise)
         */
        [[nodiscard]] static VendorPurchaseResult ValidatePurchase(
            Player const* player,
            Creature const* vendor,
            uint32 vendorSlot,
            uint32 itemId,
            uint32 quantity);

        /**
         * @brief Calculates total purchase cost including discounts
         *
         * @param player Player making purchase
         * @param vendor Vendor NPC
         * @param vendorItem Vendor item data
         * @param itemTemplate Item template
         * @param quantity Purchase quantity
         * @return Total gold cost in copper (after reputation discounts)
         */
        [[nodiscard]] static uint64 CalculatePurchaseCost(
            Player const* player,
            Creature const* vendor,
            VendorItem const* vendorItem,
            ItemTemplate const* itemTemplate,
            uint32 quantity);

        /**
         * @brief Checks if player has inventory space for item
         *
         * @param player Player to check
         * @param itemTemplate Item to store
         * @param quantity Quantity to store
         * @return true if sufficient space
         */
        [[nodiscard]] static bool HasInventorySpace(
            Player const* player,
            ItemTemplate const* itemTemplate,
            uint32 quantity);

        /**
         * @brief Determines recommended purchase quantity for consumable
         *
         * @param player Player evaluating
         * @param itemTemplate Consumable item
         * @return Recommended quantity (accounts for current stock)
         */
        [[nodiscard]] static uint32 GetRecommendedConsumableQuantity(
            Player const* player,
            ItemTemplate const* itemTemplate);
    };

} // namespace Playerbot

#endif // _PLAYERBOT_VENDOR_PURCHASE_MANAGER_H
