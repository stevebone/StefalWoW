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
#include "AuctionHouse.h"
#include "Player.h"
#include <unordered_map>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>
#include "GameTime.h"

namespace Playerbot
{

enum class MarketTrend : uint8
{
    STABLE      = 0,
    RISING      = 1,
    FALLING     = 2,
    VOLATILE    = 3,
    BULLISH     = 4,  // Strong upward trend
    BEARISH     = 5   // Strong downward trend
};

/**
 * @brief Market segment categories for analysis
 */
enum class MarketSegment : uint8
{
    EQUIPMENT       = 0,    // Weapons, armor, accessories
    CONSUMABLES     = 1,    // Potions, food, elixirs
    TRADE_GOODS     = 2,    // Crafting materials
    GEMS            = 3,    // Gems and enchanting materials
    RECIPES         = 4,    // Profession recipes
    COLLECTIBLES    = 5,    // Pets, mounts, toys
    TRANSMOG        = 6,    // Transmog items
    MISC            = 7,    // Everything else
    CRAFTING        = 8,    // Crafting materials and reagents
    QUEST_ITEMS     = 9     // Quest-related items
};

struct MarketSnapshot
{
    uint32 timestamp;
    uint32 itemId;
    uint32 activeListings;
    uint32 totalVolume;
    float averagePrice;
    float medianPrice;
    float minPrice;
    float maxPrice;
    float standardDeviation;
    uint32 salesVelocity; // Items sold per hour

    MarketSnapshot() : timestamp(GameTime::GetGameTimeMS()), itemId(0), activeListings(0)
        , totalVolume(0), averagePrice(0.0f), medianPrice(0.0f)
        , minPrice(0.0f), maxPrice(0.0f), standardDeviation(0.0f)
        , salesVelocity(0) {}
};

// Market metrics for liquidity and efficiency tracking
struct MarketMetrics
{
    float liquidity; // How easily items can be bought/sold
    float efficiency; // Price stability and fair value alignment
    float competitiveness; // Number of active sellers
    float seasonality; // Cyclical price patterns
    float momentum; // Rate of price change
    MarketTrend currentTrend;
    uint32 lastAnalysisTime;

    MarketMetrics() : liquidity(0.5f), efficiency(0.8f), competitiveness(0.6f)
        , seasonality(0.0f), momentum(0.0f), currentTrend(MarketTrend::STABLE)
        , lastAnalysisTime(GameTime::GetGameTimeMS()) {}
};

// Price analysis and forecasting data
struct PriceAnalysis
{
    float fairValue; // Calculated "true" market value
    float supportLevel; // Price floor based on historical data
    float resistanceLevel; // Price ceiling based on historical data
    float volatility; // Price variability measure
    float momentum; // Price change acceleration
    float confidence; // Reliability of the analysis (0.0-1.0)
    ::std::vector<float> movingAverages; // 7-day, 14-day, 30-day averages

    PriceAnalysis() : fairValue(0.0f), supportLevel(0.0f), resistanceLevel(0.0f)
        , volatility(0.0f), momentum(0.0f), confidence(0.5f) {}
};

// Market opportunity identification
struct MarketOpportunity
{
    uint32 itemId;
    ::std::string itemName;
    MarketSegment segment;
    float currentPrice;
    float targetPrice;
    float potentialProfit;
    float riskLevel;
    uint32 timeToTarget; // Estimated hours to reach target
    ::std::string reason;
    float confidence;

    MarketOpportunity() : itemId(0), segment(MarketSegment::EQUIPMENT)
        , currentPrice(0.0f), targetPrice(0.0f), potentialProfit(0.0f)
        , riskLevel(0.5f), timeToTarget(0), confidence(0.5f) {}
};

// Competitive analysis data
struct CompetitorAnalysis
{
    ::std::vector<uint32> majorSellers;
    ::std::unordered_map<uint32, float> sellerMarketShare; // sellerGuid -> market share
    ::std::unordered_map<uint32, float> sellerPricingStyle; // sellerGuid -> aggressiveness
    float marketConcentration; // How dominated by few sellers
    uint32 averageListingDuration;
    float averageUndercutAmount;

    CompetitorAnalysis() : marketConcentration(0.5f), averageListingDuration(86400)
        , averageUndercutAmount(0.05f) {}
};

// Analysis performance metrics
struct AnalysisMetrics
{
    ::std::atomic<uint32> predictionsGenerated{0};
    ::std::atomic<uint32> accuratePredictions{0};
    ::std::atomic<uint32> opportunitiesIdentified{0};
    ::std::atomic<uint32> profitableOpportunities{0};
    ::std::atomic<float> averageAccuracy{0.7f};
    ::std::atomic<float> averageProfitability{0.15f};
    ::std::atomic<uint32> marketUpdates{0};
    ::std::chrono::steady_clock::time_point lastUpdate;

    void Reset()
    {
        predictionsGenerated = 0; accuratePredictions = 0;
        opportunitiesIdentified = 0; profitableOpportunities = 0;
        averageAccuracy = 0.7f; averageProfitability = 0.15f;
        marketUpdates = 0; lastUpdate = ::std::chrono::steady_clock::now();
    }

    float GetPredictionAccuracy() const
    {
        uint32 total = predictionsGenerated.load();
        uint32 accurate = accuratePredictions.load();
        return total > 0 ? (float)accurate / total : 0.0f;
    }
};

/**
 * @brief Advanced market analysis system for auction house intelligence
 *
 * This system provides deep market insights, trend analysis, and predictive
 * pricing models to help playerbots make informed auction house decisions.
 */
class TC_GAME_API MarketAnalysis final 
{
public:
    static MarketAnalysis* instance();

    // Core market analysis
    MarketSnapshot GetMarketSnapshot(uint32 itemId);
    MarketTrend GetMarketTrend(uint32 itemId, uint32 daysBack = 7);
    float GetPricePrediction(uint32 itemId, uint32 hoursAhead = 24);
    ::std::vector<uint32> GetTrendingItems(MarketSegment segment = MarketSegment::EQUIPMENT);

    // Market intelligence
    void AnalyzeMarketConditions();
    void UpdateMarketData(uint32 itemId, uint32 price, uint32 quantity, uint32 timestamp = 0);
    void RecordSale(uint32 itemId, uint32 price, uint32 quantity, uint32 sellTime);
    void TrackMarketMovement();

    // Advanced market metrics
    MarketMetrics GetMarketMetrics(uint32 itemId);
    MarketMetrics GetSegmentMetrics(MarketSegment segment);

    // Price analysis and forecasting
    PriceAnalysis AnalyzePrice(uint32 itemId);
    float CalculateFairValue(uint32 itemId);
    ::std::pair<float, float> GetPriceRange(uint32 itemId, float confidence = 0.95f);
    bool IsPriceAnomaly(uint32 itemId, uint32 price);

    // Market opportunity identification
    ::std::vector<MarketOpportunity> IdentifyOpportunities(Player* player, uint32 budgetLimit = 0);
    ::std::vector<MarketOpportunity> FindArbitrageOpportunities();
    ::std::vector<MarketOpportunity> FindFlipOpportunities(uint32 maxInvestment);
    bool IsGoodBuyingOpportunity(uint32 itemId, uint32 price);
    bool IsGoodSellingOpportunity(uint32 itemId, uint32 price);

    // Competitive analysis
    CompetitorAnalysis AnalyzeCompetition(uint32 itemId);
    ::std::vector<uint32> GetTopSellers(uint32 itemId, uint32 count = 5);
    float GetSellerReputationScore(uint32 sellerGuid);
    bool IsMarketDominated(uint32 itemId, float threshold = 0.6f);

    // Seasonal and cyclical analysis
    void DetectSeasonalPatterns(uint32 itemId);
    ::std::vector<::std::pair<uint32, float>> GetHistoricalPricePattern(uint32 itemId, uint32 daysBack = 90);
    bool HasWeeklyPattern(uint32 itemId);
    bool HasDailyPattern(uint32 itemId);
    float GetSeasonalityFactor(uint32 itemId, uint32 timestamp = 0);

    // Market segment analysis
    void AnalyzeMarketSegment(MarketSegment segment);
    ::std::vector<uint32> GetTopItemsInSegment(MarketSegment segment, uint32 count = 10);
    float GetSegmentGrowthRate(MarketSegment segment);
    MarketTrend GetSegmentTrend(MarketSegment segment);

    // Performance and accuracy tracking
    AnalysisMetrics const& GetAnalysisMetrics() const { return _metrics; }

    // Configuration and learning
    void SetAnalysisDepth(float depth); // 0.0 = fast, 1.0 = thorough
    void EnableLearning(bool enable) { _learningEnabled = enable; }
    void UpdatePredictionAccuracy(uint32 itemId, float predictedPrice, float actualPrice);
    void LearnFromMarketEvents();

    // Update and maintenance
    void Update(uint32 diff);
    void UpdateTrendAnalysis();
    void CleanupOldData();
    void RecalibrateModels();

private:
    MarketAnalysis();
    ~MarketAnalysis() = default;

    // Market data storage
    ::std::unordered_map<uint32, ::std::vector<MarketSnapshot>> _priceHistory; // itemId -> snapshots
    ::std::unordered_map<uint32, MarketMetrics> _itemMetrics; // itemId -> metrics
    ::std::unordered_map<MarketSegment, ::std::vector<uint32>> _segmentItems; // segment -> itemIds
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _marketMutex;

    // Analysis models and algorithms

    // Seller reputation tracking data for market trust analysis
    struct SellerReputation
    {
        uint32 sellerGuid = 0;
        uint32 firstSeenTime = 0;
        uint32 lastActivityTime = 0;
        uint32 lastCalculationTime = 0;

        // Auction history
        uint32 completedAuctions = 0;
        uint32 expiredAuctions = 0;
        uint32 cancelledAuctions = 0;

        // Pricing analysis
        float pricingDeviationSum = 0.0f;   // Sum of deviations from fair value
        uint32 totalPricingChecks = 0;

        // Sale velocity
        uint32 totalSaleTimeHours = 0;      // Total time items took to sell

        // Market manipulation indicators
        float largestPriceSwing = 0.0f;     // Largest single price change
        uint32 suspiciousTimingPatterns = 0; // Count of suspicious listing patterns
        uint32 monopolyAttempts = 0;        // Times seller cornered a market

        // Cached score
        float reputationScore = 0.5f;
    };

    // Seller reputation cache
    ::std::unordered_map<uint32, SellerReputation> _sellerReputations;

    struct PredictionModel
    {
        ::std::vector<float> weights;
        float bias;
        float accuracy;
        uint32 trainingSamples;
        uint32 lastTraining;

        PredictionModel() : bias(0.0f), accuracy(0.5f), trainingSamples(0)
            , lastTraining(GameTime::GetGameTimeMS()) {}
    };

    ::std::unordered_map<uint32, PredictionModel> _predictionModels; // itemId -> model
    ::std::unordered_map<uint32, CompetitorAnalysis> _competitorCache; // itemId -> analysis

    // Performance tracking
    AnalysisMetrics _metrics;

    // Configuration
    ::std::atomic<float> _analysisDepth{0.8f};
    ::std::atomic<bool> _learningEnabled{true};
    ::std::atomic<uint32> _maxHistoryDays{90};

    // Helper functions
    void InitializeSegmentMappings();
    MarketSegment DetermineItemSegment(uint32 itemId);
    void UpdateMovingAverages(uint32 itemId);

    // Statistical analysis
    float CalculateStandardDeviation(const ::std::vector<float>& prices);
    float CalculateCorrelation(const ::std::vector<float>& series1, const ::std::vector<float>& series2);
    ::std::vector<float> CalculateMovingAverage(const ::std::vector<float>& prices, uint32 window);
    float CalculateVolatility(const ::std::vector<float>& prices);

    // Trend analysis algorithms
    MarketTrend AnalyzeTrendDirection(const ::std::vector<float>& prices);
    float CalculateTrendStrength(const ::std::vector<float>& prices);
    bool DetectTrendReversal(const ::std::vector<float>& prices);
    float CalculateMomentum(const ::std::vector<float>& prices);

    // Price prediction algorithms
    float PredictLinearRegression(uint32 itemId, uint32 hoursAhead);
    float PredictMovingAverage(uint32 itemId, uint32 hoursAhead);
    float PredictSeasonalAdjusted(uint32 itemId, uint32 hoursAhead);
    void TrainPredictionModel(uint32 itemId);

    // Opportunity detection algorithms
    ::std::vector<MarketOpportunity> ScanForPriceDiscrepancies();
    ::std::vector<MarketOpportunity> ScanForTrendBreakouts();
    ::std::vector<MarketOpportunity> ScanForMeanReversion();
    bool ValidateOpportunity(const MarketOpportunity& opportunity);

    // Market efficiency analysis
    float CalculateMarketEfficiency(uint32 itemId);
    bool IsMarketManipulated(uint32 itemId);
    float CalculateLiquidityScore(uint32 itemId);
    void DetectAnomalies(uint32 itemId);

    // Learning and adaptation
    void UpdateModelWeights(uint32 itemId, float error);
    void AdaptToMarketConditions();
    void RecalibrateModel(uint32 itemId);
    void ValidatePredictions();

    // Constants
    static constexpr uint32 ANALYSIS_UPDATE_INTERVAL = 60000; // 1 minute
    static constexpr uint32 TREND_ANALYSIS_INTERVAL = 300000; // 5 minutes
    static constexpr uint32 MODEL_TRAINING_INTERVAL = 3600000; // 1 hour
    static constexpr uint32 MIN_SAMPLES_FOR_PREDICTION = 10;
    static constexpr float TREND_THRESHOLD = 0.05f; // 5% change for trend detection
    static constexpr float VOLATILITY_THRESHOLD = 0.2f; // 20% for high volatility
    static constexpr uint32 MOVING_AVERAGE_WINDOWS[3] = {7, 14, 30}; // Days
    static constexpr float PREDICTION_CONFIDENCE_THRESHOLD = 0.6f;
    static constexpr uint32 MAX_OPPORTUNITIES = 20;
    static constexpr float ANOMALY_THRESHOLD = 2.0f; // Standard deviations
};

} // namespace Playerbot