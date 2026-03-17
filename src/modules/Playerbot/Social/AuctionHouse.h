/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Player.h"
#include "AuctionHouseMgr.h"
#include "Item.h"
#include "Economy/AuctionManager.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <atomic>
#include <mutex>
#include <chrono>
#include "GameTime.h"

namespace Playerbot
{

/**
 * @brief Auction action types for session management
 */
enum class AuctionActionType : uint8
{
    SEARCH = 0,
    BID = 1,
    BUYOUT = 2,
    CREATE = 3,
    CANCEL = 4,
    SCAN_BARGAINS = 5,
    AUTO_SELL = 6,
    AUTO_BUY = 7,
    // Additional action types used by metrics and tracking
    UPDATE_BID = 8,
    BUY_ITEM = 9,
    SELL_ITEM = 10,
    CANCEL_AUCTION = 11,
    SEARCH_MARKET = 12
};

/**
 * @brief Strategy types for auction house session operations
 */
enum class AuctionHouseStrategy : uint8
{
    CONSERVATIVE = 0,    // Safe, slow trading
    AGGRESSIVE = 1,      // Fast, competitive trading
    OPPORTUNISTIC = 2,   // Bargain hunting
    MARKET_MAKER = 3,    // Supply/demand trading
    COLLECTOR = 4,       // Collecting rare items
    PROFIT_FOCUSED = 5,  // Maximum profit
    QUICK_SALE = 6,      // Fast selling at lower prices
    PREMIUM = 7          // Premium pricing for valuable items
};

/**
 * @brief Auction item representation
 */
struct AuctionItem
{
    uint32 auctionId{0};
    uint32 itemId{0};
    uint32 itemGuid{0};
    uint32 stackCount{1};
    uint32 ownerGuid{0};
    uint32 sellerGuid{0};          // Seller GUID (alias for ownerGuid)
    ::std::string ownerName;
    uint32 startBid{0};
    uint32 buyout{0};
    uint32 buyoutPrice{0};         // Buyout price (alias for buyout)
    uint32 currentBid{0};
    uint32 bidderGuid{0};
    uint32 expireTime{0};
    uint32 lastSeen{0};            // Last time this auction was seen
    uint8 quality{0};              // Item quality
    uint32 itemLevel{0};           // Item level
    uint32 pricePerItem{0};        // Price per item in stack
    bool isBargain{false};         // Whether this is a bargain item
    float marketValue{0.0f};       // Estimated market value

    AuctionItem() = default;
};

/**
 * @brief Auction profile configuration
 */
struct AuctionProfile
{
    AuctionHouseStrategy strategy{AuctionHouseStrategy::CONSERVATIVE};
    AuctionHouseStrategy primaryStrategy{AuctionHouseStrategy::CONSERVATIVE};
    uint32 maxConcurrentAuctions{10};
    uint32 maxAuctionsActive{10};
    uint32 auctionBudget{0};
    uint32 maxBiddingBudget{0};
    float undercutRate{0.05f};
    float minProfitMargin{0.1f};
    float profitMargin{0.1f};
    float bargainThreshold{0.8f};
    bool autoRelist{true};
    bool autoBuyBargains{false};
    bool autoBuyConsumables{false};
    bool autoSellJunk{true};
    ::std::vector<uint32> watchList;
    ::std::unordered_set<uint32> blackList;

    AuctionProfile() = default;
};

/**
 * @brief Auction action entry for session queue
 */
struct AuctionAction
{
    AuctionActionType type{AuctionActionType::SEARCH};
    uint32 targetId{0};
    uint32 amount{0};

    AuctionAction() = default;
    AuctionAction(AuctionActionType t, uint32 id = 0, uint32 amt = 0)
        : type(t), targetId(id), amount(amt) {}
};

/**
 * @brief Auction session tracking
 */
struct AuctionSession
{
    uint32 sessionId{0};
    AuctionActionType primaryAction{AuctionActionType::SEARCH};
    uint32 startTime{0};
    uint32 sessionStartTime{0};
    uint32 lastUpdateTime{0};
    bool isActive{false};
    bool isComplete{false};
    ::std::string lastError;
    ::std::vector<AuctionItem> results;
    ::std::queue<AuctionAction> actionQueue;
    uint32 itemsBought{0};
    uint32 itemsSold{0};
    uint64 budgetUsed{0};

    AuctionSession() = default;
    AuctionSession(uint32 id, AuctionActionType action, uint32 start)
        : sessionId(id), primaryAction(action), startTime(start), sessionStartTime(start)
        , lastUpdateTime(start), isActive(true), isComplete(false)
        , itemsBought(0), itemsSold(0), budgetUsed(0) {}
};

/**
 * @brief Auction metrics tracking
 */
struct AuctionMetrics
{
    ::std::atomic<uint32> auctionsCreated{0};
    ::std::atomic<uint32> auctionsSold{0};
    ::std::atomic<uint32> auctionsBought{0};
    ::std::atomic<uint32> auctionsCancelled{0};
    ::std::atomic<uint32> auctionsExpired{0};
    ::std::atomic<uint64> goldSpent{0};
    ::std::atomic<uint64> goldEarned{0};
    ::std::atomic<uint64> totalGoldEarned{0};
    ::std::atomic<uint64> profit{0};
    ::std::atomic<uint32> marketScans{0};          // Number of market scans performed
    ::std::atomic<uint32> itemsPurchased{0};       // Total items purchased
    ::std::atomic<uint64> totalGoldSpent{0};       // Total gold spent (alias for goldSpent)
    ::std::atomic<uint32> bargainsFound{0};        // Bargain opportunities found
    ::std::atomic<float> averageProfitMargin{0.1f};

    AuctionMetrics() = default;

    // Copy constructor - loads values from atomics
    AuctionMetrics(const AuctionMetrics& other)
        : auctionsCreated(other.auctionsCreated.load())
        , auctionsSold(other.auctionsSold.load())
        , auctionsBought(other.auctionsBought.load())
        , auctionsCancelled(other.auctionsCancelled.load())
        , auctionsExpired(other.auctionsExpired.load())
        , goldSpent(other.goldSpent.load())
        , goldEarned(other.goldEarned.load())
        , totalGoldEarned(other.totalGoldEarned.load())
        , profit(other.profit.load())
        , marketScans(other.marketScans.load())
        , itemsPurchased(other.itemsPurchased.load())
        , totalGoldSpent(other.totalGoldSpent.load())
        , bargainsFound(other.bargainsFound.load())
        , averageProfitMargin(other.averageProfitMargin.load())
    {}

    // Copy assignment - loads values from atomics
    AuctionMetrics& operator=(const AuctionMetrics& other)
    {
        if (this != &other)
        {
            auctionsCreated = other.auctionsCreated.load();
            auctionsSold = other.auctionsSold.load();
            auctionsBought = other.auctionsBought.load();
            auctionsCancelled = other.auctionsCancelled.load();
            auctionsExpired = other.auctionsExpired.load();
            goldSpent = other.goldSpent.load();
            goldEarned = other.goldEarned.load();
            totalGoldEarned = other.totalGoldEarned.load();
            profit = other.profit.load();
            marketScans = other.marketScans.load();
            itemsPurchased = other.itemsPurchased.load();
            totalGoldSpent = other.totalGoldSpent.load();
            bargainsFound = other.bargainsFound.load();
            averageProfitMargin = other.averageProfitMargin.load();
        }
        return *this;
    }

    float GetROI() const
    {
        uint64 spent = goldSpent.load();
        uint64 earned = goldEarned.load();
        return spent > 0 ? static_cast<float>(earned - spent) / spent : 0.0f;
    }
};

struct AuctionSearchQuery
{
    std::string itemName;
    uint32 itemId;
    uint32 minLevel;
    uint32 maxLevel;
    uint32 minQuality;
    uint32 maxQuality;
    uint32 maxPrice;
    uint32 minItemLevel;
    std::vector<uint32> itemClasses;
    std::vector<uint32> itemSubClasses;
    bool exactMatch;
    bool usableOnly;

    AuctionSearchQuery() : itemId(0), minLevel(0), maxLevel(0), minQuality(0)
        , maxQuality(6), maxPrice(0), minItemLevel(0), exactMatch(false)
        , usableOnly(false) {}
};

/**
 * @brief Advanced auction house system for automated buying and selling
 *
 * This system provides intelligent auction house interactions, market analysis,
 * and automated trading strategies for playerbots using TrinityCore's auction system.
 */
class TC_GAME_API AuctionHouse final
{
public:
    explicit AuctionHouse(Player* bot);
    ~AuctionHouse();
    AuctionHouse(AuctionHouse const&) = delete;
    AuctionHouse& operator=(AuctionHouse const&) = delete;

    // Core auction house operations using TrinityCore's AuctionHouseMgr
    void SearchAuctionHouse(const AuctionSearchQuery& query);
    bool PlaceAuctionBid(uint32 auctionId, uint32 bidAmount);
    bool BuyoutAuction(uint32 auctionId);
    bool CreateAuction(uint32 itemGuid, uint32 stackCount, uint32 bid, uint32 buyout, uint32 duration);
    bool CancelAuction(uint32 auctionId);

    // Intelligent auction strategies
    void ExecuteAuctionStrategy(AuctionHouseStrategy strategy);
    void ScanForBargains();
    void AutoSellItems(const std::vector<uint32>& itemGuids);
    void AutoBuyNeededItems();
    void ManageActiveAuctions();

    // Market analysis and price discovery
    float GetMarketPrice(uint32 itemId, uint32 stackSize = 1);
    float GetPriceHistory(uint32 itemId, uint32 days = 7);
    std::vector<AuctionItem> GetSimilarAuctions(uint32 itemId, uint32 maxResults = 10);
    bool IsPriceBelowMarket(uint32 itemId, uint32 price, float threshold = 0.8f);
    void UpdateMarketData();

    // Advanced auction features (AuctionProfile defined in IAuctionHouse.h interface)
    void SetAuctionProfile(const AuctionProfile& profile);
    AuctionProfile GetAuctionProfile();

    // Auction monitoring and automation (AuctionSession defined in IAuctionHouse.h interface)
    uint32 StartAuctionSession(AuctionActionType primaryAction);
    void UpdateAuctionSession(uint32 sessionId);
    void CompleteAuctionSession(uint32 sessionId);
    AuctionSession GetAuctionSession(uint32 sessionId);

    // Price optimization and profit calculation
    uint32 CalculateOptimalListingPrice(uint32 itemId, uint32 stackSize);
    uint32 CalculateMaxBidAmount(uint32 itemId, uint32 stackSize);
    float CalculatePotentialProfit(const AuctionItem& auction, uint32 resellPrice);
    bool IsWorthBuying(const AuctionItem& auction);
    bool ShouldUndercut(uint32 itemId, uint32 currentLowest);

    // Market intelligence and learning
    void TrackPriceMovement(uint32 itemId, uint32 price, uint32 timestamp);
    void AnalyzeMarketTrends(uint32 itemId);
    void LearnFromAuctionOutcome(uint32 auctionId, bool wasSuccessful);
    void AdaptAuctionBehavior();

    // Specialized auction services
    void HandleConsumableAutomation();
    void HandleEquipmentUpgrades();
    void HandleCraftingMaterials();
    void HandleCollectibleTrading();
    void HandleBulkItemTrading();

    // Competition analysis
    void AnalyzeCompetition(uint32 itemId);
    std::vector<uint32> GetFrequentSellers(uint32 itemId);
    float GetCompetitorUndercutRate(uint32 sellerGuid);
    void TrackCompetitorBehavior(uint32 sellerGuid, const AuctionItem& auction);

    // Performance monitoring (AuctionMetrics defined in IAuctionHouse.h interface)
    AuctionMetrics GetAuctionMetrics();
    AuctionMetrics GetGlobalAuctionMetrics();

    // Auction house integration with TrinityCore
    void LoadAuctionData();
    void SynchronizeWithAuctionHouseMgr();
    AuctionHouseObject* GetAuctionHouseForPlayer();
    bool ValidateAuctionAccess(uint32 auctionHouseId);

    // Configuration and customization
    void SetAuctionHouseEnabled(bool enabled);
    void SetMaxConcurrentAuctions(uint32 maxAuctions);
    void SetAuctionBudget(uint32 budget);
    void AddToWatchList(uint32 itemId);
    void RemoveFromWatchList(uint32 itemId);

    // Error handling and recovery
    void HandleAuctionError(uint32 sessionId, const std::string& error);
    void RecoverFromAuctionFailure(uint32 sessionId);
    void HandleInsufficientFunds(uint32 requiredAmount);
    void HandleAuctionTimeout(uint32 auctionId);

    // Update and maintenance
    void Update(uint32 diff);
    void UpdateAuctionSessions();
    void UpdateMarketAnalysis();
    void CleanupExpiredData();

private:
    Player* _bot;

    // Per-bot instance data
    AuctionProfile _profile;
    std::unordered_map<uint32, AuctionSession> _activeSessions; // sessionId -> session (this bot's sessions)
    AuctionMetrics _metrics;
    bool _auctionHouseEnabled{true};

    // Shared static data (session IDs must be unique across all bots)
    static std::atomic<uint32> _nextSessionId;

    // Market data and analysis
    struct MarketData
    {
        uint32 itemId;
        std::vector<std::pair<uint32, uint32>> priceHistory; // price, timestamp
        float averagePrice;
        float medianPrice;
        uint32 totalVolume;
        uint32 activeListings;
        float volatility;
        uint32 lastAnalysisTime;

        MarketData() : itemId(0), averagePrice(0.0f), medianPrice(0.0f)
            , totalVolume(0), activeListings(0), volatility(0.0f)
            , lastAnalysisTime(0) {}

        MarketData(uint32 id) : itemId(id), averagePrice(0.0f), medianPrice(0.0f)
            , totalVolume(0), activeListings(0), volatility(0.0f)
            , lastAnalysisTime(GameTime::GetGameTimeMS()) {}
    };

    // Shared static market data (all bots share market intelligence)
    static std::unordered_map<uint32, MarketData> _marketData;
    static std::unordered_map<uint32, std::vector<AuctionItem>> _auctionCache;
    static Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::TRADE_MANAGER> _marketMutex;

    // Competition tracking (shared)
    struct CompetitorProfile
    {
        uint32 sellerGuid;
        std::string sellerName;
        std::vector<uint32> frequentItems;
        std::vector<std::pair<uint32, uint32>> pricingHistory; // itemId, price
        float averageUndercutRate;
        float aggressiveness;
        uint32 totalAuctions;
        uint32 successfulSales;
        uint32 lastActivity;

        CompetitorProfile() : sellerGuid(0), averageUndercutRate(0.05f)
            , aggressiveness(0.5f), totalAuctions(0), successfulSales(0)
            , lastActivity(0) {}

        CompetitorProfile(uint32 guid) : sellerGuid(guid), averageUndercutRate(0.05f)
            , aggressiveness(0.5f), totalAuctions(0), successfulSales(0)
            , lastActivity(GameTime::GetGameTimeMS()) {}
    };

    static std::unordered_map<uint32, CompetitorProfile> _competitors; // sellerGuid -> profile

    // Performance tracking
    static AuctionMetrics _globalMetrics;

    // Strategy implementations
    void ExecuteConservativeStrategy(AuctionSession& session);
    void ExecuteAggressiveStrategy(AuctionSession& session);
    void ExecuteOpportunisticStrategy(AuctionSession& session);
    void ExecuteMarketMakerStrategy(AuctionSession& session);
    void ExecuteCollectorStrategy(AuctionSession& session);
    void ExecuteProfitFocusedStrategy(AuctionSession& session);

    // Market analysis helpers
    void UpdateItemMarketData(uint32 itemId);
    float CalculateMarketVolatility(const std::vector<std::pair<uint32, uint32>>& priceHistory);
    float PredictPriceMovement(uint32 itemId);
    bool IsMarketTrendy(uint32 itemId, bool& isRising);

    // Search and filtering
    std::vector<AuctionItem> FilterAuctionResults(const std::vector<AuctionItem>& auctions,
                                                 const AuctionSearchQuery& query);
    std::vector<AuctionItem> FindBargainAuctions( float maxPriceRatio = 0.8f);
    std::vector<AuctionItem> FindFlipOpportunities();
    bool MatchesSearchCriteria(const AuctionItem& auction, const AuctionSearchQuery& query);

    // Price calculation algorithms
    uint32 CalculateReasonableBid(const AuctionItem& auction, float aggressiveness = 0.5f);
    uint32 CalculateCompetitivePrice(uint32 itemId, uint32 stackSize);
    uint32 CalculateUndercutPrice(uint32 itemId, uint32 currentLowest, float undercutRate = 0.05f);
    float CalculateExpectedReturn(const AuctionItem& auction, uint32 resellPrice);

    // Auction execution helpers
    bool ExecuteBuyAction( const AuctionItem& auction);
    bool ExecuteSellAction( uint32 itemGuid, uint32 stackCount, uint32 price);
    bool ExecuteCancelAction( uint32 auctionId);
    void ProcessActionQueue( AuctionSession& session);

    // Learning and adaptation
    void UpdateSuccessRates( bool wasSuccessful, AuctionActionType actionType);
    void AdaptPricingStrategy();
    void LearnMarketPatterns(uint32 itemId);
    void UpdatePlayerPreferences( uint32 itemId, bool wasPurchased);

    // Performance optimization
    void CacheFrequentAuctions();
    void PreloadMarketData(const std::vector<uint32>& itemIds);
    void OptimizeAuctionQueries();
    void UpdateAuctionMetrics(uint32 playerGuid, AuctionActionType actionType,
                             uint32 goldAmount, bool wasSuccessful);

    // Constants
    static constexpr uint32 AUCTION_UPDATE_INTERVAL = 30000; // 30 seconds
    static constexpr uint32 MARKET_ANALYSIS_INTERVAL = 300000; // 5 minutes
    static constexpr uint32 SESSION_TIMEOUT = 1800000; // 30 minutes
    static constexpr uint32 MAX_SEARCH_RESULTS = 100;
    static constexpr float DEFAULT_UNDERCUT_RATE = 0.05f; // 5%
    static constexpr float MIN_PROFIT_MARGIN = 0.1f; // 10%
    static constexpr uint32 PRICE_HISTORY_DAYS = 30;
    static constexpr uint32 MAX_CONCURRENT_SESSIONS = 50;
    static constexpr float BARGAIN_THRESHOLD = 0.8f; // 80% of market price
    static constexpr uint32 MARKET_DATA_CACHE_DURATION = 600000; // 10 minutes
};

} // namespace Playerbot