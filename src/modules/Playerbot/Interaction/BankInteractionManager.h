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
class GameObject;
struct ItemTemplate;

namespace Playerbot
{

/**
 * @class BankInteractionManager
 * @brief Manages all bank interactions for player bots
 *
 * This class provides complete personal bank functionality using TrinityCore's
 * bank system APIs. It handles:
 * - Item deposits and withdrawals
 * - Bank slot management and organization
 * - Smart item prioritization (what to keep in inventory vs bank)
 * - Bag slot purchases
 * - Bank space optimization
 *
 * Note: For guild bank operations, use GuildBankManager instead.
 *
 * Performance Target: <1ms per bank operation
 * Memory Target: <15KB overhead
 *
 * TrinityCore API Integration:
 * - Player::GetItemByPos() - Access bank slot items
 * - Player::SwapItem() - Move items between inventory and bank
 * - Player::BankItem() - Direct bank item placement
 * - Player::CanBankItem() - Validate bank operations
 * - Bank slot constants: BANK_SLOT_ITEM_START to BANK_SLOT_ITEM_END
 */
class BankInteractionManager
{
public:
    /**
     * @brief Item storage priority
     *
     * Determines where items should be stored:
     * - INVENTORY_ONLY: Keep in inventory, never bank
     * - PREFER_INVENTORY: Keep in inventory unless full
     * - PREFER_BANK: Bank unless needed soon
     * - BANK_ONLY: Always bank, withdraw when needed
     */
    enum class StoragePriority : uint8
    {
        INVENTORY_ONLY  = 0,  // Quest items, equipped gear
        PREFER_INVENTORY = 1, // Consumables, reagents
        PREFER_BANK      = 2, // Profession materials, extras
        BANK_ONLY        = 3  // Rare items, long-term storage
    };

    /**
     * @brief Bank item evaluation
     */
    struct ItemEvaluation
    {
        uint32 itemId;
        uint32 itemGuid;
        uint32 stackCount;
        StoragePriority priority;
        bool shouldBank;           // True if item should be in bank
        bool shouldWithdraw;       // True if item should be in inventory
        ::std::string reason;        // Human-readable reason

        ItemEvaluation()
            : itemId(0), itemGuid(0), stackCount(0)
            , priority(StoragePriority::PREFER_BANK)
            , shouldBank(false), shouldWithdraw(false)
        { }
    };

    /**
     * @brief Bank space information
     */
    struct BankSpaceInfo
    {
        uint32 totalSlots;         // Total bank slots
        uint32 usedSlots;          // Currently used slots
        uint32 freeSlots;          // Available slots
        uint32 bagSlotsPurchased;  // Number of bag slots unlocked
        uint32 maxBagSlots;        // Maximum bag slots available
        uint64 estimatedValue;     // Total value of items in bank (copper)

        BankSpaceInfo()
            : totalSlots(0), usedSlots(0), freeSlots(0)
            , bagSlotsPurchased(0), maxBagSlots(7)
            , estimatedValue(0)
        { }

        float GetUsagePercent() const
        {
            return totalSlots > 0 ? (float)usedSlots / totalSlots * 100.0f : 0.0f;
        }

        bool IsFull() const { return freeSlots == 0; }
        bool HasSpace(uint32 needed = 1) const { return freeSlots >= needed; }
    };

    BankInteractionManager(Player* bot);
    ~BankInteractionManager() = default;

    // Core Bank Methods

    /**
     * @brief Deposit an item to bank
     * @param banker Banker NPC or bank chest
     * @param item Item to deposit
     * @param count Stack count to deposit (0 = all)
     * @return True if successfully deposited
     *
     * Moves item from inventory to bank using Player::BankItem().
     */
    bool DepositItem(WorldObject* banker, Item* item, uint32 count = 0);

    /**
     * @brief Deposit item by ID
     * @param banker Banker NPC or bank chest
     * @param itemId Item template ID
     * @param count Stack count to deposit (0 = all of this item)
     * @return True if successfully deposited
     */
    bool DepositItemById(WorldObject* banker, uint32 itemId, uint32 count = 0);

    /**
     * @brief Withdraw an item from bank
     * @param banker Banker NPC or bank chest
     * @param itemId Item template ID to withdraw
     * @param count Stack count to withdraw (0 = all)
     * @return True if successfully withdrawn
     */
    bool WithdrawItem(WorldObject* banker, uint32 itemId, uint32 count = 0);

    /**
     * @brief Smart deposit - automatically deposit appropriate items
     * @param banker Banker NPC or bank chest
     * @return Number of items deposited
     *
     * Evaluates all inventory items and deposits those better suited for bank storage.
     */
    uint32 SmartDeposit(WorldObject* banker);

    /**
     * @brief Smart withdraw - get items needed for current activities
     * @param banker Banker NPC or bank chest
     * @return Number of items withdrawn
     *
     * Evaluates bank contents and withdraws items needed for:
     * - Active quests
     * - Profession crafting
     * - Combat consumables
     */
    uint32 SmartWithdraw(WorldObject* banker);

    /**
     * @brief Organize bank for optimal space usage
     * @param banker Banker NPC or bank chest
     * @return True if organization was performed
     *
     * Consolidates stacks, sorts items by type, and optimizes layout.
     */
    bool OrganizeBank(WorldObject* banker);

    // Bank Analysis Methods

    /**
     * @brief Get bank space information
     * @return BankSpaceInfo structure
     */
    BankSpaceInfo GetBankSpaceInfo() const;

    /**
     * @brief Evaluate an item for storage location
     * @param item Item to evaluate
     * @return Evaluation result
     */
    ItemEvaluation EvaluateItem(Item* item) const;

    /**
     * @brief Calculate storage priority for an item type
     * @param itemTemplate Item template
     * @return Storage priority
     */
    StoragePriority CalculateStoragePriority(ItemTemplate const* itemTemplate) const;

    /**
     * @brief Get count of item in bank
     * @param itemId Item template ID
     * @return Total count in bank
     */
    uint32 GetBankItemCount(uint32 itemId) const;

    /**
     * @brief Check if item is in bank
     * @param itemId Item template ID
     * @return True if at least one is in bank
     */
    bool IsItemInBank(uint32 itemId) const;

    /**
     * @brief Get all items in bank
     * @return Vector of bank items
     */
    ::std::vector<Item*> GetBankItems() const;

    /**
     * @brief Check if bot has bank space
     * @param slotsNeeded Number of slots needed
     * @return True if sufficient space
     */
    bool HasBankSpace(uint32 slotsNeeded = 1) const;

    // Bank Bag Slot Management

    /**
     * @brief Get number of bank bag slots purchased
     * @return Number of purchased bag slots
     */
    uint32 GetPurchasedBagSlots() const;

    /**
     * @brief Get cost to purchase next bank bag slot
     * @return Cost in copper, or 0 if all slots purchased
     */
    uint32 GetNextBagSlotCost() const;

    /**
     * @brief Purchase next bank bag slot
     * @param banker Banker NPC
     * @return True if successfully purchased
     */
    bool PurchaseBagSlot(Creature* banker);

    /**
     * @brief Check if bot can afford next bag slot
     * @return True if can afford
     */
    bool CanAffordBagSlot() const;

    // Utility Methods

    /**
     * @brief Check if at a banker
     * @param target Potential banker
     * @return True if valid banker
     */
    bool IsBanker(WorldObject* target) const;

    /**
     * @brief Find nearest banker
     * @param maxRange Maximum search range
     * @return Nearest banker or nullptr
     */
    Creature* FindNearestBanker(float maxRange = 100.0f) const;

    /**
     * @brief Check if bot is in bank interaction range
     * @param banker Banker NPC
     * @return True if in range
     */
    bool IsInBankRange(WorldObject* banker) const;

    // Statistics and Performance

    struct Statistics
    {
        uint32 itemsDeposited;      // Total items deposited
        uint32 itemsWithdrawn;      // Total items withdrawn
        uint32 depositOperations;   // Number of deposit operations
        uint32 withdrawOperations;  // Number of withdraw operations
        uint32 bagSlotsPurchased;   // Bank bag slots purchased
        uint32 organizationRuns;    // Times bank was organized
        uint64 totalGoldSpent;      // Gold spent on bag slots

        Statistics()
            : itemsDeposited(0), itemsWithdrawn(0)
            , depositOperations(0), withdrawOperations(0)
            , bagSlotsPurchased(0), organizationRuns(0), totalGoldSpent(0)
        { }
    };

    Statistics const& GetStatistics() const { return m_stats; }
    void ResetStatistics() { m_stats = Statistics(); }

    float GetCPUUsage() const { return m_cpuUsage; }
    size_t GetMemoryUsage() const;

private:
    // Internal Helper Methods

    /**
     * @brief Find empty bank slot
     * @return Bank slot index, or 0xFF if none available
     */
    uint8 FindEmptyBankSlot() const;

    /**
     * @brief Execute deposit via TrinityCore API
     * @param item Item to deposit
     * @param bankSlot Target bank slot
     * @return True if successful
     */
    bool ExecuteDeposit(Item* item, uint8 bankSlot);

    /**
     * @brief Execute withdrawal via TrinityCore API
     * @param bankSlot Source bank slot
     * @param count Stack count
     * @return True if successful
     */
    bool ExecuteWithdraw(uint8 bankSlot, uint32 count);

    /**
     * @brief Check if item is needed for quests
     * @param itemId Item template ID
     * @return True if quest item
     */
    bool IsQuestItem(uint32 itemId) const;

    /**
     * @brief Check if item is profession material
     * @param itemTemplate Item template
     * @return True if profession material
     */
    bool IsProfessionMaterial(ItemTemplate const* itemTemplate) const;

    /**
     * @brief Check if item is a consumable
     * @param itemTemplate Item template
     * @return True if consumable
     */
    bool IsConsumable(ItemTemplate const* itemTemplate) const;

    /**
     * @brief Get items that should be deposited
     * @return Vector of items to deposit
     */
    ::std::vector<Item*> GetItemsToDeposit() const;

    /**
     * @brief Get items that should be withdrawn
     * @return Vector of item IDs to withdraw
     */
    ::std::vector<uint32> GetItemsToWithdraw() const;

    /**
     * @brief Record deposit in statistics
     */
    void RecordDeposit(uint32 itemId, uint32 count);

    /**
     * @brief Record withdrawal in statistics
     */
    void RecordWithdraw(uint32 itemId, uint32 count);

    // Member Variables
    Player* m_bot;
    Statistics m_stats;

    // Performance Tracking
    float m_cpuUsage;
    uint32 m_totalOperationTime;   // microseconds
    uint32 m_operationCount;

    // Cache
    ::std::unordered_map<uint32, StoragePriority> m_priorityCache;
    mutable BankSpaceInfo m_cachedBankInfo;
    mutable uint32 m_lastBankInfoCheck;
    static constexpr uint32 BANK_CACHE_DURATION = 30000; // 30 seconds

    // Bank slot constants
    static constexpr uint8 BANK_SLOT_START = 39;    // BANK_SLOT_ITEM_START
    static constexpr uint8 BANK_SLOT_END = 67;      // BANK_SLOT_ITEM_END
    static constexpr uint8 BANK_BAG_SLOT_START = 67; // BANK_SLOT_BAG_START
    static constexpr uint8 BANK_BAG_SLOT_END = 74;   // BANK_SLOT_BAG_END
};

} // namespace Playerbot
