/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * BANKING MANAGER FOR PLAYERBOT
 *
 * This system provides personal banking automation for bots:
 * - Automatically deposits excess gold to prevent loss on death
 * - Automatically deposits materials based on priority (keep essentials, bank excess)
 * - Automatically withdraws materials needed for crafting
 * - Manages bank space efficiently with smart item prioritization
 * - Tracks deposit/withdrawal history for optimization
 * - Coordinates with profession systems for material management
 *
 * Integration Points:
 * - Uses ProfessionManager to determine material priorities
 * - Uses GatheringMaterialsBridge to identify needed materials
 * - Uses ProfessionAuctionBridge for stockpile coordination
 * - Works with TrinityCore bank system (PlayerBankItems)
 *
 * Design Pattern: BehaviorManager Pattern
 * - Inherits from BehaviorManager for standardized lifecycle
 * - Throttled updates for performance
 * - Thread-safe operations
 * - Event-driven deposit/withdrawal triggers
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Player.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"
#include "../AI/BehaviorManager.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <set>

namespace Playerbot
{

/**
 * @brief Banking strategy for bot
 */
enum class BankingStrategy : uint8
{
    NONE = 0,
    HOARDER,                // Keep everything, deposit all
    MINIMALIST,             // Keep only essentials, deposit rest
    PROFESSION_FOCUSED,     // Keep profession materials, bank crafted items
    GOLD_FOCUSED,           // Prioritize gold deposits, keep valuable items
    BALANCED,               // Balance between keeping and banking
    MANUAL                  // Manual control, no automation
};

/**
 * @brief Item banking priority
 */
enum class BankingPriority : uint8
{
    NEVER_BANK = 0,         // Never deposit (equipped gear, quest items)
    LOW = 1,                // Bank if space allows
    MEDIUM = 2,             // Bank when inventory is full
    HIGH = 3,               // Bank regularly
    CRITICAL = 4            // Bank immediately (excess gold, rare items)
};

/**
 * @brief Banking rule for item types
 */
struct BankingRule
{
    uint32 itemId;              // 0 for all items of category
    uint32 itemClass;           // ITEM_CLASS_* (0 for specific itemId)
    uint32 itemSubClass;        // ITEM_SUBCLASS_* (0 for any)
    uint32 itemQuality;         // ITEM_QUALITY_* (0 for any)

    BankingPriority priority;
    uint32 keepInInventory;     // Minimum stack to keep in inventory
    uint32 maxInInventory;      // Maximum stack in inventory before banking

    bool enabled;

    BankingRule()
        : itemId(0), itemClass(0), itemSubClass(0), itemQuality(0)
        , priority(BankingPriority::MEDIUM), keepInInventory(0), maxInInventory(100)
        , enabled(true) {}
};

/**
 * @brief Banking profile per bot
 */
struct BotBankingProfile
{
    BankingStrategy strategy = BankingStrategy::BALANCED;

    // Gold management
    uint32 minGoldInInventory = 100000;         // 10 gold minimum
    uint32 maxGoldInInventory = 1000000;        // 100 gold maximum
    bool autoDepositGold = true;

    // Item management
    bool autoDepositMaterials = true;
    bool autoWithdrawForCrafting = true;
    bool autoDepositCraftedItems = true;

    // Bank access
    uint32 bankCheckInterval = 300000;          // 5 minutes
    uint32 maxDistanceToBanker = 10;            // Yards
    bool travelToBankerWhenNeeded = true;

    // Banking rules (itemId/class/subclass -> rule)
    std::vector<BankingRule> customRules;

    BotBankingProfile() = default;
};

/**
 * @brief Banking transaction record
 */
struct BankingTransaction
{
    enum class Type : uint8
    {
        DEPOSIT_GOLD,
        DEPOSIT_ITEM,
        WITHDRAW_GOLD,
        WITHDRAW_ITEM
    };

    Type type;
    uint32 timestamp;
    uint32 itemId;              // 0 for gold transactions
    uint32 quantity;
    uint32 goldAmount;          // For gold transactions
    std::string reason;         // Why this transaction occurred

    BankingTransaction()
        : type(Type::DEPOSIT_GOLD), timestamp(0), itemId(0)
        , quantity(0), goldAmount(0) {}
};

/**
 * @brief Banking statistics
 */
struct BankingStatistics
{
    std::atomic<uint32> totalDeposits{0};
    std::atomic<uint32> totalWithdrawals{0};
    std::atomic<uint32> goldDeposited{0};
    std::atomic<uint32> goldWithdrawn{0};
    std::atomic<uint32> itemsDeposited{0};
    std::atomic<uint32> itemsWithdrawn{0};
    std::atomic<uint32> bankTrips{0};
    std::atomic<uint32> timeSpentBanking{0};        // Milliseconds

    void Reset()
    {
        totalDeposits = 0;
        totalWithdrawals = 0;
        goldDeposited = 0;
        goldWithdrawn = 0;
        itemsDeposited = 0;
        itemsWithdrawn = 0;
        bankTrips = 0;
        timeSpentBanking = 0;
    }

    uint32 GetNetGoldChange() const
    {
        uint32 deposited = goldDeposited.load();
        uint32 withdrawn = goldWithdrawn.load();
        return deposited > withdrawn ? deposited - withdrawn : 0;
    }
};

/**
 * @brief Bank space analysis
 */
struct BankSpaceInfo
{
    uint32 totalSlots;
    uint32 usedSlots;
    uint32 freeSlots;
    uint32 estimatedValue;      // Total value of items in bank (copper)

    std::unordered_map<uint32, uint32> itemCounts; // itemId -> quantity

    BankSpaceInfo()
        : totalSlots(0), usedSlots(0), freeSlots(0), estimatedValue(0) {}

    float GetUsagePercent() const
    {
        return totalSlots > 0 ? (float)usedSlots / totalSlots : 0.0f;
    }

    bool IsFull() const
    {
        return freeSlots == 0;
    }

    bool HasSpace(uint32 slotsNeeded = 1) const
    {
        return freeSlots >= slotsNeeded;
    }
};

/**
 * @brief Banking Manager - Personal bank automation
 *
 * **Phase 5.1: Singleton → Per-Bot Instance Pattern**
 *
 * DESIGN PRINCIPLE: Per-bot instance owned by GameSystemsManager
 * - Each bot has its own BankingManager instance
 * - No mutex locking (per-bot isolation)
 * - Direct member access (no map lookups)
 * - Integrates with profession and gathering systems via facade
 * - Does NOT handle guild bank (use GuildBankManager)
 *
 * **Ownership:**
 * - Owned by GameSystemsManager via std::unique_ptr
 * - Constructed per-bot with Player* reference
 * - Destroyed with bot cleanup
 */
class TC_GAME_API BankingManager final : public BehaviorManager
{
public:
    /**
     * @brief Construct banking manager for bot
     * @param bot The bot player this manager serves
     */
    explicit BankingManager(Player* bot);

    /**
     * @brief Destructor - cleanup per-bot resources
     */
    ~BankingManager();

    // ========================================================================
    // LIFECYCLE (BehaviorManager override)
    // ========================================================================

    bool Initialize();
    void Update(uint32 diff);
    void Shutdown();

    // ========================================================================
    // CORE BANKING OPERATIONS
    // ========================================================================

    /**
     * Enable/disable banking automation for this bot
     */
    void SetEnabled(bool enabled);
    bool IsEnabled() const;

    /**
     * Set banking profile for this bot
     */
    void SetBankingProfile(BotBankingProfile const& profile);
    BotBankingProfile GetBankingProfile() const;

    /**
     * Add custom banking rule for this bot
     */
    void AddBankingRule(BankingRule const& rule);
    void RemoveBankingRule(uint32 itemId);

    // ========================================================================
    // GOLD MANAGEMENT
    // ========================================================================

    /**
     * Deposit gold to bank
     * Automatically called when gold exceeds maxGoldInInventory
     */
    bool DepositGold(uint32 amount);

    /**
     * Withdraw gold from bank
     * Automatically called when gold falls below minGoldInInventory
     */
    bool WithdrawGold(uint32 amount);

    /**
     * Check if bot should deposit gold
     */
    bool ShouldDepositGold();

    /**
     * Check if bot should withdraw gold
     */
    bool ShouldWithdrawGold();

    /**
     * Get recommended gold deposit amount
     */
    uint32 GetRecommendedGoldDeposit();

    // ========================================================================
    // ITEM MANAGEMENT
    // ========================================================================

    /**
     * Deposit item to bank
     * Delegates to TrinityCore bank system
     */
    bool DepositItem(uint32 itemGuid, uint32 quantity);

    /**
     * Withdraw item from bank
     * Delegates to TrinityCore bank system
     */
    bool WithdrawItem(uint32 itemId, uint32 quantity);

    /**
     * Check if item should be banked based on rules
     */
    bool ShouldDepositItem(uint32 itemId, uint32 currentCount);

    /**
     * Get banking priority for item
     */
    BankingPriority GetItemBankingPriority(uint32 itemId);

    /**
     * Scan inventory and deposit items based on rules
     */
    void DepositExcessItems();

    /**
     * Withdraw materials needed for crafting
     * Coordinates with ProfessionManager for material needs
     */
    void WithdrawMaterialsForCrafting();

    // ========================================================================
    // BANK SPACE ANALYSIS
    // ========================================================================

    /**
     * Get current bank space information
     */
    BankSpaceInfo GetBankSpaceInfo();

    /**
     * Check if bot has bank space
     */
    bool HasBankSpace(uint32 slotsNeeded = 1);

    /**
     * Get item count in bank
     */
    uint32 GetItemCountInBank(uint32 itemId);

    /**
     * Check if item is in bank
     */
    bool IsItemInBank(uint32 itemId);

    /**
     * Optimize bank space (consolidate stacks, remove junk)
     */
    void OptimizeBankSpace();

    // ========================================================================
    // BANKER ACCESS
    // ========================================================================

    /**
     * Check if bot is near banker
     */
    bool IsNearBanker();

    /**
     * Get distance to nearest banker
     */
    float GetDistanceToNearestBanker();

    /**
     * Travel to nearest banker (triggers bot movement)
     */
    bool TravelToNearestBanker();

    // ========================================================================
    // TRANSACTION HISTORY
    // ========================================================================

    /**
     * Get recent banking transactions for this bot
     */
    std::vector<BankingTransaction> GetRecentTransactions(uint32 count = 10);

    /**
     * Record banking transaction for this bot
     */
    void RecordTransaction(BankingTransaction const& transaction);

    // ========================================================================
    // STATISTICS
    // ========================================================================

    BankingStatistics const& GetStatistics() const;
    static BankingStatistics const& GetGlobalStatistics();
    void ResetStatistics();

private:
    // Non-copyable
    BankingManager(BankingManager const&) = delete;
    BankingManager& operator=(BankingManager const&) = delete;

    // ========================================================================
    // INITIALIZATION HELPERS
    // ========================================================================

    void InitializeDefaultRules();
    static void LoadBankingRules();  // Load shared rules once

    // ========================================================================
    // BANKING LOGIC HELPERS
    // ========================================================================

    /**
     * Find applicable banking rule for item
     */
    BankingRule const* FindBankingRule(uint32 itemId);

    /**
     * Calculate item banking priority using rules and heuristics
     */
    BankingPriority CalculateItemPriority(uint32 itemId);

    /**
     * Check if item matches banking rule
     */
    static bool ItemMatchesRule(uint32 itemId, BankingRule const& rule);

    /**
     * Get items to deposit from inventory
     */
    struct DepositCandidate
    {
        uint32 itemGuid;
        uint32 itemId;
        uint32 quantity;
        BankingPriority priority;
    };
    std::vector<DepositCandidate> GetDepositCandidates();

    /**
     * Get materials to withdraw for crafting
     */
    struct WithdrawRequest
    {
        uint32 itemId;
        uint32 quantity;
        std::string reason;
    };
    std::vector<WithdrawRequest> GetWithdrawRequests();

    // ========================================================================
    // INTEGRATION HELPERS
    // ========================================================================

    /**
     * Check if item is needed for professions
     */
    bool IsNeededForProfessions(uint32 itemId);

    /**
     * Get material priority from profession system
     */
    uint32 GetMaterialPriorityFromProfessions(uint32 itemId);

    /**
     * Get ProfessionManager via GameSystemsManager facade
     */
    class ProfessionManager* GetProfessionManager();

protected:
    // Override pure virtual from BehaviorManager
    void OnUpdate(uint32 diff) override { Update(diff); }

private:
    // ========================================================================
    // PER-BOT INSTANCE DATA
    // ========================================================================

    Player* _bot;                                   // Bot reference (non-owning)
    BotBankingProfile _profile;                     // Banking profile for this bot
    std::vector<BankingTransaction> _transactionHistory; // Transaction history
    BankingStatistics _statistics;                  // Statistics for this bot
    uint32 _lastBankAccessTime{0};                  // Last bank access timestamp
    bool _currentlyBanking{false};                  // Is bot currently banking
    bool _enabled{true};                            // Banking automation enabled
    Creature* _cachedNearestBanker{nullptr};        // Cached nearest banker for TravelToNearestBanker

    // ========================================================================
    // SHARED STATIC DATA
    // ========================================================================

    static std::vector<BankingRule> _defaultRules;  // Default banking rules
    static BankingStatistics _globalStatistics;     // Global statistics across all bots
    static bool _defaultRulesInitialized;           // Initialization flag

    // Update intervals
    static constexpr uint32 BANKING_CHECK_INTERVAL = 300000;       // 5 minutes
    static constexpr uint32 GOLD_CHECK_INTERVAL = 60000;           // 1 minute
    static constexpr uint32 MAX_TRANSACTION_HISTORY = 100;         // Keep last 100 transactions
};

} // namespace Playerbot
