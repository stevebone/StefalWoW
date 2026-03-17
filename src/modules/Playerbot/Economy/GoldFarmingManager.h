/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * GOLD FARMING MANAGER
 *
 * Phase 3: Humanization Core (Task 13)
 *
 * Manages gold farming activities for bots:
 * - Tracks gold income and expenses
 * - Suggests profitable activities
 * - Coordinates gathering, AH, and mob farming
 * - Analyzes gold per hour efficiency
 */

#pragma once

#include "Define.h"
#include "AI/BehaviorManager.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <atomic>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>

class Player;

namespace Playerbot
{

class BotAI;

/**
 * @enum GoldFarmingMethod
 * @brief Methods of farming gold
 */
enum class GoldFarmingMethod : uint8
{
    NONE = 0,
    QUESTING,           // Quest rewards
    MOB_FARMING,        // Killing mobs for loot
    GATHERING,          // Mining, herbalism, skinning
    CRAFTING,           // Crafting and selling
    AUCTION_FLIPPING,   // Buy low, sell high
    DUNGEON_RUNS,       // Dungeon farming
    OLD_RAIDS,          // Solo old raids for transmog/gold
    FISHING,            // Fishing valuable items
    TREASURE_HUNTING,   // Finding chests/treasures

    MAX_METHOD
};

/**
 * @struct GoldTransaction
 * @brief A single gold transaction
 */
struct GoldTransaction
{
    int64 amount{0};            // Positive = income, negative = expense
    GoldFarmingMethod source{GoldFarmingMethod::NONE};
    uint32 itemId{0};           // Related item ID (if applicable)
    uint32 timestamp{0};        // GameTime
    std::string description;

    bool IsIncome() const { return amount > 0; }
    bool IsExpense() const { return amount < 0; }
};

/**
 * @struct FarmingSpot
 * @brief A location for farming gold
 */
struct FarmingSpot
{
    uint32 mapId{0};
    uint32 zoneId{0};
    Position position;
    GoldFarmingMethod method{GoldFarmingMethod::NONE};
    uint32 estimatedGoldPerHour{0};
    uint32 requiredLevel{0};
    std::string name;
    std::string description;

    bool IsValid() const { return mapId > 0 || zoneId > 0; }
};

/**
 * @struct GoldFarmingGoal
 * @brief A goal for gold farming
 */
struct GoldFarmingGoal
{
    uint64 targetGold{0};           // Target gold amount
    uint64 currentGold{0};          // Current gold
    uint64 startGold{0};            // Gold when goal started
    GoldFarmingMethod preferredMethod{GoldFarmingMethod::NONE};
    std::chrono::steady_clock::time_point deadline;
    bool hasDeadline{false};

    uint64 GetGoldNeeded() const
    {
        if (currentGold >= targetGold)
            return 0;
        return targetGold - currentGold;
    }

    float GetProgress() const
    {
        if (targetGold <= startGold)
            return 1.0f;
        uint64 needed = targetGold - startGold;
        uint64 gained = currentGold > startGold ? currentGold - startGold : 0;
        return std::min(1.0f, static_cast<float>(gained) / static_cast<float>(needed));
    }

    bool IsComplete() const
    {
        return currentGold >= targetGold;
    }
};

/**
 * @struct GoldFarmingSession
 * @brief Tracks a gold farming session
 */
struct GoldFarmingSession
{
    GoldFarmingGoal goal;
    GoldFarmingMethod activeMethod{GoldFarmingMethod::NONE};
    FarmingSpot activeSpot;
    std::chrono::steady_clock::time_point startTime;
    uint64 startGold{0};
    int64 goldGained{0};
    int64 goldSpent{0};
    uint32 itemsLooted{0};
    uint32 itemsSold{0};
    bool isActive{false};

    void Reset()
    {
        goal = GoldFarmingGoal();
        activeMethod = GoldFarmingMethod::NONE;
        activeSpot = FarmingSpot();
        goldGained = 0;
        goldSpent = 0;
        itemsLooted = 0;
        itemsSold = 0;
        isActive = false;
    }

    uint32 GetElapsedMs() const
    {
        if (!isActive)
            return 0;
        auto now = std::chrono::steady_clock::now();
        return static_cast<uint32>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count());
    }

    int64 GetNetGold() const
    {
        return goldGained - goldSpent;
    }

    uint32 GetGoldPerHour() const
    {
        uint32 elapsedMs = GetElapsedMs();
        if (elapsedMs < 60000)  // Less than 1 minute
            return 0;
        int64 net = GetNetGold();
        if (net <= 0)
            return 0;
        return static_cast<uint32>(net * 3600000.0f / elapsedMs);
    }
};

/**
 * @brief Callback for gold events
 */
using GoldCallback = std::function<void(int64 amount, GoldFarmingMethod source)>;

/**
 * @class GoldFarmingManager
 * @brief Manages gold farming for bots
 *
 * This manager:
 * - Tracks all gold income and expenses
 * - Suggests profitable farming methods
 * - Coordinates with gathering, AH, and combat systems
 * - Analyzes efficiency (gold per hour)
 *
 * Update interval: 5000ms (5 seconds)
 */
class TC_GAME_API GoldFarmingManager : public BehaviorManager
{
public:
    explicit GoldFarmingManager(Player* bot, BotAI* ai);
    ~GoldFarmingManager() override = default;

    // ========================================================================
    // FAST STATE QUERIES
    // ========================================================================

    /**
     * @brief Check if bot is actively farming gold
     */
    bool IsFarming() const { return _currentSession.isActive; }

    /**
     * @brief Get current gold amount
     */
    uint64 GetCurrentGold() const;

    /**
     * @brief Get net gold change in current session
     */
    int64 GetSessionNetGold() const { return _currentSession.GetNetGold(); }

    /**
     * @brief Get gold per hour rate
     */
    uint32 GetGoldPerHour() const { return _currentSession.GetGoldPerHour(); }

    /**
     * @brief Get active farming method
     */
    GoldFarmingMethod GetActiveMethod() const { return _currentSession.activeMethod; }

    // ========================================================================
    // GOLD TRACKING
    // ========================================================================

    /**
     * @brief Record a gold transaction
     * @param amount Amount (positive = income, negative = expense)
     * @param source Source of the transaction
     * @param itemId Related item ID (optional)
     * @param description Description (optional)
     */
    void RecordTransaction(int64 amount, GoldFarmingMethod source,
        uint32 itemId = 0, std::string const& description = "");

    /**
     * @brief Record gold gained from loot
     * @param amount Amount gained
     */
    void RecordLootGold(uint64 amount);

    /**
     * @brief Record gold from quest reward
     * @param amount Amount gained
     * @param questId Quest ID
     */
    void RecordQuestGold(uint64 amount, uint32 questId);

    /**
     * @brief Record gold from selling item
     * @param amount Amount gained
     * @param itemId Item ID
     * @param count Item count
     */
    void RecordVendorSale(uint64 amount, uint32 itemId, uint32 count);

    /**
     * @brief Record gold from auction sale
     * @param amount Amount gained (after fees)
     * @param itemId Item ID
     */
    void RecordAuctionSale(uint64 amount, uint32 itemId);

    /**
     * @brief Record gold spent
     * @param amount Amount spent
     * @param source What it was spent on
     * @param description Description
     */
    void RecordExpense(uint64 amount, GoldFarmingMethod source,
        std::string const& description = "");

    /**
     * @brief Get recent transactions
     * @param maxCount Maximum transactions to return
     * @return Vector of recent transactions
     */
    std::vector<GoldTransaction> GetRecentTransactions(uint32 maxCount = 20) const;

    // ========================================================================
    // FARMING ANALYSIS
    // ========================================================================

    /**
     * @brief Get suggested farming methods
     * @param maxCount Maximum suggestions
     * @return Vector of methods sorted by profitability
     */
    std::vector<GoldFarmingMethod> GetSuggestedMethods(uint32 maxCount = 5) const;

    /**
     * @brief Get suggested farming spots
     * @param method Specific method (NONE = any)
     * @param maxCount Maximum suggestions
     * @return Vector of farming spots
     */
    std::vector<FarmingSpot> GetSuggestedSpots(
        GoldFarmingMethod method = GoldFarmingMethod::NONE,
        uint32 maxCount = 5) const;

    /**
     * @brief Get estimated gold per hour for method
     * @param method Farming method
     * @return Estimated gold per hour
     */
    uint32 GetEstimatedGph(GoldFarmingMethod method) const;

    /**
     * @brief Get income breakdown by method
     * @return Map of method -> total gold
     */
    std::unordered_map<GoldFarmingMethod, int64> GetIncomeBreakdown() const;

    // ========================================================================
    // SESSION CONTROL
    // ========================================================================

    /**
     * @brief Start a gold farming session
     * @param targetGold Target gold to reach (0 = no target)
     * @param method Preferred method (NONE = auto-select)
     * @return true if session started
     */
    bool StartSession(uint64 targetGold = 0, GoldFarmingMethod method = GoldFarmingMethod::NONE);

    /**
     * @brief Stop the current session
     * @param reason Why stopping
     */
    void StopSession(std::string const& reason = "");

    /**
     * @brief Change farming method mid-session
     * @param method New method
     * @return true if changed
     */
    bool ChangeMethod(GoldFarmingMethod method);

    /**
     * @brief Move to a farming spot
     * @param spot Spot to farm at
     * @return true if started moving
     */
    bool GoToFarmingSpot(FarmingSpot const& spot);

    /**
     * @brief Get current session info
     */
    GoldFarmingSession const& GetCurrentSession() const { return _currentSession; }

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Set minimum gold to keep (won't spend below this)
     */
    void SetMinGoldReserve(uint64 amount) { _minGoldReserve = amount; }

    /**
     * @brief Get minimum gold reserve
     */
    uint64 GetMinGoldReserve() const { return _minGoldReserve; }

    /**
     * @brief Set preferred farming method
     */
    void SetPreferredMethod(GoldFarmingMethod method) { _preferredMethod = method; }

    /**
     * @brief Enable/disable specific farming method
     */
    void SetMethodEnabled(GoldFarmingMethod method, bool enabled);

    /**
     * @brief Check if method is enabled
     */
    bool IsMethodEnabled(GoldFarmingMethod method) const;

    /**
     * @brief Set callback for gold events
     */
    void SetCallback(GoldCallback callback) { _callback = std::move(callback); }

    // ========================================================================
    // STATISTICS
    // ========================================================================

    struct GoldStatistics
    {
        std::atomic<int64> totalIncome{0};
        std::atomic<int64> totalExpenses{0};
        std::atomic<uint32> itemsLooted{0};
        std::atomic<uint32> itemsSold{0};
        std::atomic<uint32> auctionsSold{0};
        std::atomic<uint64> totalFarmingTimeMs{0};
        std::atomic<uint32> bestGphAchieved{0};

        void Reset()
        {
            totalIncome = 0;
            totalExpenses = 0;
            itemsLooted = 0;
            itemsSold = 0;
            auctionsSold = 0;
            totalFarmingTimeMs = 0;
            bestGphAchieved = 0;
        }

        int64 GetNetProfit() const
        {
            return totalIncome.load() - totalExpenses.load();
        }
    };

    GoldStatistics const& GetStatistics() const { return _statistics; }

protected:
    // ========================================================================
    // BEHAVIOR MANAGER INTERFACE
    // ========================================================================

    void OnUpdate(uint32 elapsed);
    bool OnInitialize();
    void OnShutdown();

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Update current gold tracking
     */
    void UpdateGoldTracking();

    /**
     * @brief Update session progress
     */
    void UpdateSessionProgress();

    /**
     * @brief Auto-select best farming method
     * @return Best method for current context
     */
    GoldFarmingMethod AutoSelectMethod() const;

    /**
     * @brief Trim old transactions
     */
    void TrimTransactionHistory();

    /**
     * @brief Notify callback
     */
    void NotifyCallback(int64 amount, GoldFarmingMethod source);

    // ========================================================================
    // DATA
    // ========================================================================

    // Session state
    GoldFarmingSession _currentSession;

    // Transaction history
    std::vector<GoldTransaction> _transactions;
    std::unordered_map<GoldFarmingMethod, int64> _incomeByMethod;

    // Gold tracking
    uint64 _lastKnownGold{0};
    std::chrono::steady_clock::time_point _lastGoldCheck;

    // Configuration
    uint64 _minGoldReserve{0};
    GoldFarmingMethod _preferredMethod{GoldFarmingMethod::NONE};
    std::unordered_set<GoldFarmingMethod> _enabledMethods;

    // Callback
    GoldCallback _callback;

    // Statistics
    GoldStatistics _statistics;

    // Constants
    static constexpr uint32 MAX_TRANSACTION_HISTORY = 100;
    static constexpr uint32 GOLD_CHECK_INTERVAL_MS = 10000;  // 10 seconds
};

} // namespace Playerbot
