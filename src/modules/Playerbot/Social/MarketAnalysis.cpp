/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "MarketAnalysis.h"
#include "AuctionHouse.h"
#include "AuctionHouseMgr.h"
#include "Player.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "ObjectMgr.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Util.h"
#include "World.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <random>
#include "GameTime.h"

namespace Playerbot
{

MarketAnalysis* MarketAnalysis::instance()
{
    static MarketAnalysis instance;
    return &instance;
}

MarketAnalysis::MarketAnalysis()
{
    _metrics.Reset();
    InitializeSegmentMappings();
}

MarketSnapshot MarketAnalysis::GetMarketSnapshot(uint32 itemId)
{
    ::std::lock_guard lock(_marketMutex);

    MarketSnapshot snapshot;
    snapshot.itemId = itemId;
    snapshot.timestamp = GameTime::GetGameTimeMS();

    // Get current auction house data for this item
    auto* auctionHouseMgr = sAuctionMgr;
    if (!auctionHouseMgr)
        return snapshot;

    // Collect auction data from all auction houses
    ::std::vector<float> prices;
    uint32 totalVolume = 0;

    // Iterate through all auction houses (Alliance, Horde, Neutral)
    for (uint32 auctionHouseId = 1; auctionHouseId <= 3; ++auctionHouseId)
    {
        AuctionHouseObject* auctionHouse = auctionHouseMgr->GetAuctionsById(auctionHouseId);
        if (!auctionHouse)
            continue;

        // Iterate through all auctions using proper TrinityCore API
    for (auto itr = auctionHouse->GetAuctionsBegin(); itr != auctionHouse->GetAuctionsEnd(); ++itr)
        {
            const AuctionPosting& auction = itr->second;

            // Check if this auction contains items with the target itemId
    for (Item* item : auction.Items)
            {
                if (item && item->GetTemplate()->GetId() == itemId)
                {
                    uint32 itemCount = auction.GetTotalItemCount();
                    if (itemCount > 0 && auction.BuyoutOrUnitPrice > 0)
                    {
                        float pricePerItem = float(auction.BuyoutOrUnitPrice) / float(itemCount);
                        if (pricePerItem > 0)
                        {
                            prices.push_back(pricePerItem);
                            totalVolume += itemCount;
                            snapshot.activeListings++;
                        }
                    }
                }
            }
        }
    }

    if (!prices.empty())
    {
        // Calculate statistics
        ::std::sort(prices.begin(), prices.end());

        snapshot.totalVolume = totalVolume;
        snapshot.minPrice = prices.front();
        snapshot.maxPrice = prices.back();

        // Calculate average
        snapshot.averagePrice = ::std::accumulate(prices.begin(), prices.end(), 0.0f) / prices.size();

        // Calculate median
        size_t medianIndex = prices.size() / 2;
        if (prices.size() % 2 == 0)
            snapshot.medianPrice = (prices[medianIndex - 1] + prices[medianIndex]) / 2.0f;
        else
            snapshot.medianPrice = prices[medianIndex];

        // Calculate standard deviation
        snapshot.standardDeviation = CalculateStandardDeviation(prices);

        // Estimate sales velocity (simplified)
        snapshot.salesVelocity = static_cast<uint32>(totalVolume * 0.1f); // 10% per hour estimate
    }

    return snapshot;
}

MarketTrend MarketAnalysis::GetMarketTrend(uint32 itemId, uint32 daysBack)
{
    ::std::lock_guard lock(_marketMutex);

    auto historyIt = _priceHistory.find(itemId);
    if (historyIt == _priceHistory.end() || historyIt->second.size() < 2)
        return MarketTrend::STABLE;

    const auto& history = historyIt->second;
    uint32 cutoffTime = GameTime::GetGameTimeMS() - (daysBack * 24 * 60 * 60 * 1000);

    // Filter recent history
    ::std::vector<float> recentPrices;
    for (const auto& snapshot : history)
    {
        if (snapshot.timestamp >= cutoffTime)
            recentPrices.push_back(snapshot.averagePrice);
    }

    if (recentPrices.size() < 2)
        return MarketTrend::STABLE;

    return AnalyzeTrendDirection(recentPrices);
}

float MarketAnalysis::GetPricePrediction(uint32 itemId, uint32 hoursAhead)
{
    // Use multiple prediction models and combine results
    float linearPrediction = PredictLinearRegression(itemId, hoursAhead);
    float movingAvgPrediction = PredictMovingAverage(itemId, hoursAhead);
    float seasonalPrediction = PredictSeasonalAdjusted(itemId, hoursAhead);

    // Weighted combination of predictions
    float combinedPrediction = (linearPrediction * 0.4f) +
                              (movingAvgPrediction * 0.4f) +
                              (seasonalPrediction * 0.2f);

    _metrics.predictionsGenerated++;
    return combinedPrediction;
}

::std::vector<uint32> MarketAnalysis::GetTrendingItems(MarketSegment segment)
{
    ::std::lock_guard lock(_marketMutex);

    ::std::vector<uint32> trendingItems;
    auto segmentIt = _segmentItems.find(segment);

    if (segmentIt == _segmentItems.end())
        return trendingItems;

    // Analyze items in the segment for trending patterns
    for (uint32 itemId : segmentIt->second)
    {
        MarketTrend trend = GetMarketTrend(itemId, 7);
        if (trend == MarketTrend::RISING || trend == MarketTrend::BULLISH)
        {
            trendingItems.push_back(itemId);
        }
    }

    // Sort by trend strength (simplified)
    ::std::sort(trendingItems.begin(), trendingItems.end());

    // Return top trending items
    if (trendingItems.size() > 10)
        trendingItems.resize(10);

    return trendingItems;
}

void MarketAnalysis::AnalyzeMarketConditions()
{
    // Analyze overall market health
    // Update market efficiency metrics
    // Detect market anomalies
    // Update trend analysis

    _metrics.marketUpdates++;
}

void MarketAnalysis::UpdateMarketData(uint32 itemId, uint32 price, uint32 quantity, uint32 timestamp)
{
    ::std::lock_guard lock(_marketMutex);

    if (timestamp == 0)
        timestamp = GameTime::GetGameTimeMS();

    // Update price history
    auto& history = _priceHistory[itemId];

    // Create new snapshot if enough time has passed or it's the first entry
    bool createNewSnapshot = history.empty() ||
                            (timestamp - history.back().timestamp > 300000); // 5 minutes
    if (createNewSnapshot)
    {
        MarketSnapshot snapshot;
        snapshot.itemId = itemId;
        snapshot.timestamp = timestamp;

        // Get current market data
        snapshot = GetMarketSnapshot(itemId);

        history.push_back(snapshot);

        // Limit history size
    if (history.size() > 1000)
        {
            history.erase(history.begin(), history.begin() + 100);
        }

        // Update moving averages
        UpdateMovingAverages(itemId);
    }

    // Update item metrics
    auto& metrics = _itemMetrics[itemId];
    metrics.lastAnalysisTime = timestamp;

    _metrics.marketUpdates++;
}

void MarketAnalysis::RecordSale(uint32 itemId, uint32 price, uint32 quantity, uint32 sellTime)
{
    // Record actual sale data for analysis accuracy
    // Update sales velocity calculations
    // Improve prediction models

    UpdateMarketData(itemId, price, quantity, sellTime);
}

void MarketAnalysis::TrackMarketMovement()
{
    // Track significant price movements
    // Detect market manipulation
    // Update volatility metrics
}

MarketMetrics MarketAnalysis::GetMarketMetrics(uint32 itemId)
{
    ::std::lock_guard lock(_marketMutex);

    auto it = _itemMetrics.find(itemId);
    if (it != _itemMetrics.end())
        return it->second;

    return MarketMetrics(); // Return default metrics
}

MarketMetrics MarketAnalysis::GetSegmentMetrics(MarketSegment segment)
{
    MarketMetrics segmentMetrics;

    auto segmentIt = _segmentItems.find(segment);
    if (segmentIt == _segmentItems.end())
        return segmentMetrics;

    // Aggregate metrics from all items in segment
    float totalLiquidity = 0.0f;
    float totalEfficiency = 0.0f;
    float totalCompetitiveness = 0.0f;
    uint32 itemCount = 0;

    for (uint32 itemId : segmentIt->second)
    {
        MarketMetrics itemMetrics = GetMarketMetrics(itemId);
        totalLiquidity += itemMetrics.liquidity;
        totalEfficiency += itemMetrics.efficiency;
        totalCompetitiveness += itemMetrics.competitiveness;
        itemCount++;
    }

    if (itemCount > 0)
    {
        segmentMetrics.liquidity = totalLiquidity / itemCount;
        segmentMetrics.efficiency = totalEfficiency / itemCount;
        segmentMetrics.competitiveness = totalCompetitiveness / itemCount;
        segmentMetrics.currentTrend = GetSegmentTrend(segment);
    }

    return segmentMetrics;
}

PriceAnalysis MarketAnalysis::AnalyzePrice(uint32 itemId)
{
    PriceAnalysis analysis;

    MarketSnapshot snapshot = GetMarketSnapshot(itemId);
    analysis.fairValue = CalculateFairValue(itemId);

    // Calculate support and resistance levels
    auto priceRange = GetPriceRange(itemId, 0.95f);
    analysis.supportLevel = priceRange.first;
    analysis.resistanceLevel = priceRange.second;

    // Calculate volatility
    auto historyIt = _priceHistory.find(itemId);
    if (historyIt != _priceHistory.end() && !historyIt->second.empty())
    {
        ::std::vector<float> prices;
        for (const auto& snap : historyIt->second)
            prices.push_back(snap.averagePrice);

        analysis.volatility = CalculateVolatility(prices);
        analysis.momentum = CalculateMomentum(prices);
    }

    // Calculate confidence based on data quality
    analysis.confidence = ::std::min(1.0f, float(snapshot.activeListings) / 10.0f);

    // Calculate moving averages
    if (historyIt != _priceHistory.end())
    {
        ::std::vector<float> prices;
        for (const auto& snap : historyIt->second)
            prices.push_back(snap.averagePrice);

        for (uint32 window : MOVING_AVERAGE_WINDOWS)
        {
            auto ma = CalculateMovingAverage(prices, window);
            if (!ma.empty())
                analysis.movingAverages.push_back(ma.back());
        }
    }

    return analysis;
}

float MarketAnalysis::CalculateFairValue(uint32 itemId)
{
    MarketSnapshot snapshot = GetMarketSnapshot(itemId);

    // Use median price as baseline for fair value
    float fairValue = snapshot.medianPrice;

    // Adjust based on market efficiency
    MarketMetrics metrics = GetMarketMetrics(itemId);
    fairValue *= metrics.efficiency;

    // Apply seasonality adjustments
    float seasonalFactor = GetSeasonalityFactor(itemId);
    fairValue *= seasonalFactor;

    return fairValue;
}

::std::pair<float, float> MarketAnalysis::GetPriceRange(uint32 itemId, float confidence)
{
    auto historyIt = _priceHistory.find(itemId);
    if (historyIt == _priceHistory.end() || historyIt->second.empty())
        return {0.0f, 0.0f};

    ::std::vector<float> prices;
    for (const auto& snapshot : historyIt->second)
        prices.push_back(snapshot.averagePrice);

    if (prices.empty())
        return {0.0f, 0.0f};

    ::std::sort(prices.begin(), prices.end());

    float lowerBound = confidence / 2.0f;
    float upperBound = 1.0f - (confidence / 2.0f);

    size_t lowerIndex = static_cast<size_t>(prices.size() * lowerBound);
    size_t upperIndex = static_cast<size_t>(prices.size() * upperBound);

    lowerIndex = ::std::min(lowerIndex, prices.size() - 1);
    upperIndex = ::std::min(upperIndex, prices.size() - 1);

    return {prices[lowerIndex], prices[upperIndex]};
}

bool MarketAnalysis::IsPriceAnomaly(uint32 itemId, uint32 price)
{
    PriceAnalysis analysis = AnalyzePrice(itemId);

    if (analysis.fairValue == 0.0f)
        return false;

    float deviation = ::std::abs(float(price) - analysis.fairValue) / analysis.fairValue;
    return deviation > ANOMALY_THRESHOLD * analysis.volatility;
}

::std::vector<MarketOpportunity> MarketAnalysis::IdentifyOpportunities(Player* player, uint32 budgetLimit)
{
    ::std::vector<MarketOpportunity> opportunities;

    // Scan for various types of opportunities
    auto priceDiscrepancies = ScanForPriceDiscrepancies();
    auto trendBreakouts = ScanForTrendBreakouts();
    auto meanReversions = ScanForMeanReversion();

    // Combine all opportunities
    opportunities.insert(opportunities.end(), priceDiscrepancies.begin(), priceDiscrepancies.end());
    opportunities.insert(opportunities.end(), trendBreakouts.begin(), trendBreakouts.end());
    opportunities.insert(opportunities.end(), meanReversions.begin(), meanReversions.end());

    // Filter by budget if specified
    if (budgetLimit > 0)
    {
        opportunities.erase(
            ::std::remove_if(opportunities.begin(), opportunities.end(),
                [budgetLimit](const MarketOpportunity& opp) {
                    return opp.currentPrice > budgetLimit;
                }),
            opportunities.end()
        );
    }

    // Sort by potential profit and confidence
    ::std::sort(opportunities.begin(), opportunities.end(),
        [](const MarketOpportunity& a, const MarketOpportunity& b) {
            return (a.potentialProfit * a.confidence) > (b.potentialProfit * b.confidence);
        });

    // Limit to top opportunities
    if (opportunities.size() > MAX_OPPORTUNITIES)
        opportunities.resize(MAX_OPPORTUNITIES);

    _metrics.opportunitiesIdentified += static_cast<uint32>(opportunities.size());

    return opportunities;
}

::std::vector<MarketOpportunity> MarketAnalysis::FindArbitrageOpportunities()
{
    ::std::vector<MarketOpportunity> opportunities;

    // Look for price differences between auction houses
    // This would require cross-server data in a real implementation

    return opportunities;
}

::std::vector<MarketOpportunity> MarketAnalysis::FindFlipOpportunities(uint32 maxInvestment)
{
    ::std::vector<MarketOpportunity> opportunities;

    // Find items that can be bought low and sold high quickly
    for (const auto& historyPair : _priceHistory)
    {
        uint32 itemId = historyPair.first;
        const auto& history = historyPair.second;

        if (history.size() < 10) // Need sufficient history
            continue;

        MarketSnapshot current = GetMarketSnapshot(itemId);
        if (current.activeListings == 0 || current.averagePrice > maxInvestment)
            continue;

        float fairValue = CalculateFairValue(itemId);
        if (current.averagePrice < fairValue * 0.8f) // 20% below fair value
        {
            MarketOpportunity opp;
            opp.itemId = itemId;
            opp.currentPrice = current.averagePrice;
            opp.targetPrice = fairValue;
            opp.potentialProfit = fairValue - current.averagePrice;
            opp.confidence = 0.7f;
            opp.timeToTarget = 24; // 24 hours estimate
            opp.reason = "Price below fair value - flip opportunity";

            if (ValidateOpportunity(opp))
                opportunities.push_back(opp);
        }
    }

    return opportunities;
}

bool MarketAnalysis::IsGoodBuyingOpportunity(uint32 itemId, uint32 price)
{
    PriceAnalysis analysis = AnalyzePrice(itemId);

    if (analysis.fairValue == 0.0f)
        return false;

    // Consider it a good buying opportunity if price is below fair value
    // and trend is not strongly bearish
    bool belowFairValue = (price == 0) ? true : (float(price) < analysis.fairValue * 0.95f);

    MarketTrend trend = GetMarketTrend(itemId, 7);
    bool trendOk = (trend != MarketTrend::BEARISH && trend != MarketTrend::FALLING);

    return belowFairValue && trendOk && analysis.confidence > PREDICTION_CONFIDENCE_THRESHOLD;
}

bool MarketAnalysis::IsGoodSellingOpportunity(uint32 itemId, uint32 price)
{
    PriceAnalysis analysis = AnalyzePrice(itemId);

    if (analysis.fairValue == 0.0f)
        return false;

    // Consider it a good selling opportunity if current market price is above fair value
    MarketSnapshot snapshot = GetMarketSnapshot(itemId);
    bool aboveFairValue = snapshot.averagePrice > analysis.fairValue * 1.05f;

    MarketTrend trend = GetMarketTrend(itemId, 7);
    bool trendOk = (trend != MarketTrend::BULLISH); // Don't sell during strong uptrend

    return aboveFairValue && trendOk && analysis.confidence > PREDICTION_CONFIDENCE_THRESHOLD;
}

CompetitorAnalysis MarketAnalysis::AnalyzeCompetition(uint32 itemId)
{
    CompetitorAnalysis analysis;

    // Analyze current auction listings for this item
    auto* auctionHouseMgr = sAuctionMgr;
    if (!auctionHouseMgr)
        return analysis;

    ::std::unordered_map<uint32, uint32> sellerCounts; // sellerId -> auction count
    ::std::vector<uint32> listingDurations;
    ::std::vector<float> undercutAmounts;

    // Iterate through all auction houses (Alliance, Horde, Neutral)
    for (uint32 auctionHouseId = 1; auctionHouseId <= 3; ++auctionHouseId)
    {
        AuctionHouseObject* auctionHouse = auctionHouseMgr->GetAuctionsById(auctionHouseId);
        if (!auctionHouse)
            continue;

        // Iterate through all auctions using proper TrinityCore API
    for (auto itr = auctionHouse->GetAuctionsBegin(); itr != auctionHouse->GetAuctionsEnd(); ++itr)
        {
            const AuctionPosting& auction = itr->second;

            // Check if this auction contains items with the target itemId
    for (Item* item : auction.Items)
            {
                if (item && item->GetTemplate()->GetId() == itemId)
                {
                    sellerCounts[auction.Owner.GetCounter()]++;
                    // Calculate listing duration from StartTime to EndTime
                    auto duration = ::std::chrono::duration_cast<::std::chrono::hours>(auction.EndTime - auction.StartTime);
                    listingDurations.push_back(static_cast<uint32>(duration.count()));
                }
            }
        }
    }

    // Calculate market concentration
    if (!sellerCounts.empty())
    {
        uint32 totalListings = 0;
        uint32 topSellerListings = 0;

        for (const auto& pair : sellerCounts)
        {
            totalListings += pair.second;
            analysis.majorSellers.push_back(pair.first);
        }

        // Sort by listing count
        ::std::sort(analysis.majorSellers.begin(), analysis.majorSellers.end(),
            [&sellerCounts](uint32 a, uint32 b) {
                return sellerCounts[a] > sellerCounts[b];
            });

        // Calculate market share for top sellers
    for (size_t i = 0; i < ::std::min(size_t(5), analysis.majorSellers.size()); ++i)
        {
            uint32 sellerId = analysis.majorSellers[i];
            float marketShare = float(sellerCounts[sellerId]) / float(totalListings);
            analysis.sellerMarketShare[sellerId] = marketShare;

            if (i < 3) // Top 3 sellers
                topSellerListings += sellerCounts[sellerId];
        }

        analysis.marketConcentration = float(topSellerListings) / float(totalListings);
    }

    // Calculate average listing duration
    if (!listingDurations.empty())
    {
        analysis.averageListingDuration = ::std::accumulate(listingDurations.begin(),
                                                         listingDurations.end(), 0u) / listingDurations.size();
    }

    return analysis;
}

::std::vector<uint32> MarketAnalysis::GetTopSellers(uint32 itemId, uint32 count)
{
    CompetitorAnalysis analysis = AnalyzeCompetition(itemId);

    if (analysis.majorSellers.size() <= count)
        return analysis.majorSellers;

    ::std::vector<uint32> topSellers(analysis.majorSellers.begin(),
                                  analysis.majorSellers.begin() + count);
    return topSellers;
}

float MarketAnalysis::GetSellerReputationScore(uint32 sellerGuid)
{
    // =========================================================================
    // Full seller reputation scoring system for market trust analysis
    // Returns score in range [0.0, 1.0] where:
    //   0.0-0.3: Poor reputation (avoid transactions)
    //   0.3-0.5: Below average (caution advised)
    //   0.5-0.7: Average/neutral (new or limited history)
    //   0.7-0.9: Good reputation (reliable seller)
    //   0.9-1.0: Excellent reputation (trusted seller)
    // =========================================================================

    if (sellerGuid == 0)
        return 0.5f;  // Neutral for invalid GUID

    ::std::lock_guard lock(_marketMutex);

    // Check if we have reputation data for this seller
    auto sellerIt = _sellerReputations.find(sellerGuid);
    if (sellerIt == _sellerReputations.end())
    {
        // Initialize new seller reputation tracking
        SellerReputation newRep;
        newRep.sellerGuid = sellerGuid;
        newRep.firstSeenTime = GameTime::GetGameTimeMS();
        newRep.lastActivityTime = newRep.firstSeenTime;
        newRep.reputationScore = 0.5f;  // Neutral starting score
        _sellerReputations[sellerGuid] = newRep;
        return 0.5f;
    }

    SellerReputation& rep = sellerIt->second;

    // =========================================================================
    // COMPONENT 1: Auction Success Rate (25% weight)
    // Measures ratio of completed vs expired auctions
    // =========================================================================
    float successRateScore = 0.5f;
    uint32 totalAuctions = rep.completedAuctions + rep.expiredAuctions + rep.cancelledAuctions;
    if (totalAuctions > 0)
    {
        float successRate = float(rep.completedAuctions) / float(totalAuctions);
        // Scale to reputation score: 100% = 1.0, 50% = 0.5, 0% = 0.0
        successRateScore = successRate;

        // Penalty for high cancellation rate (possible price manipulation)
        float cancelRate = float(rep.cancelledAuctions) / float(totalAuctions);
        if (cancelRate > 0.3f)
            successRateScore *= (1.0f - (cancelRate - 0.3f));  // Reduce score for high cancels
    }

    // =========================================================================
    // COMPONENT 2: Pricing Consistency (20% weight)
    // Measures how close to market value seller prices items
    // =========================================================================
    float pricingScore = 0.5f;
    if (rep.pricingDeviationSum > 0.0f && rep.totalPricingChecks > 0)
    {
        float avgDeviation = rep.pricingDeviationSum / float(rep.totalPricingChecks);
        // Convert deviation to score: 0% deviation = 1.0, 50%+ deviation = 0.0
        pricingScore = ::std::max(0.0f, 1.0f - (avgDeviation / 0.5f));
    }

    // =========================================================================
    // COMPONENT 3: Sale Velocity (15% weight)
    // Faster sales indicate fair pricing and reliable goods
    // =========================================================================
    float velocityScore = 0.5f;
    if (rep.totalSaleTimeHours > 0 && rep.completedAuctions > 0)
    {
        float avgSaleTime = float(rep.totalSaleTimeHours) / float(rep.completedAuctions);
        // Ideal sale time < 12 hours = 1.0, 48+ hours = 0.3
        if (avgSaleTime <= 12.0f)
            velocityScore = 1.0f;
        else if (avgSaleTime <= 48.0f)
            velocityScore = 1.0f - ((avgSaleTime - 12.0f) / 36.0f * 0.7f);  // Scale down to 0.3
        else
            velocityScore = 0.3f;
    }

    // =========================================================================
    // COMPONENT 4: Transaction Volume Experience (15% weight)
    // More transactions = more reliable reputation data
    // =========================================================================
    float volumeScore = 0.5f;
    // Scale: 0 transactions = 0.3, 10 = 0.5, 50 = 0.8, 100+ = 1.0
    if (totalAuctions >= 100)
        volumeScore = 1.0f;
    else if (totalAuctions >= 50)
        volumeScore = 0.8f + (float(totalAuctions - 50) / 250.0f);
    else if (totalAuctions >= 10)
        volumeScore = 0.5f + (float(totalAuctions - 10) / 133.0f);
    else
        volumeScore = 0.3f + (float(totalAuctions) / 50.0f);

    // =========================================================================
    // COMPONENT 5: Market Manipulation Detection (15% weight)
    // Detects suspicious patterns like pump-and-dump schemes
    // =========================================================================
    float manipulationScore = 1.0f;  // Start with perfect score, deduct for violations

    // Check for rapid price swings (pump and dump indicator)
    if (rep.largestPriceSwing > 0.5f)  // >50% price swing detected
        manipulationScore -= 0.3f;

    // Check for coordinated listing patterns (bot indicator)
    if (rep.suspiciousTimingPatterns > 5)
        manipulationScore -= 0.2f;

    // Check for market cornering attempts
    if (rep.monopolyAttempts > 0)
        manipulationScore -= 0.3f * ::std::min(3u, rep.monopolyAttempts);

    manipulationScore = ::std::max(0.0f, manipulationScore);

    // =========================================================================
    // COMPONENT 6: Time-Weighted Recency (10% weight)
    // Recent activity counts more than old activity
    // =========================================================================
    float recencyScore = 0.5f;
    uint32 currentTime = GameTime::GetGameTimeMS();
    uint32 daysSinceActivity = (currentTime - rep.lastActivityTime) / (24 * 60 * 60 * 1000);

    if (daysSinceActivity == 0)
        recencyScore = 1.0f;
    else if (daysSinceActivity <= 7)
        recencyScore = 0.9f;
    else if (daysSinceActivity <= 30)
        recencyScore = 0.7f;
    else if (daysSinceActivity <= 90)
        recencyScore = 0.5f;
    else
        recencyScore = 0.3f;  // Stale seller data

    // =========================================================================
    // FINAL SCORE CALCULATION
    // Weighted combination of all components
    // =========================================================================
    float finalScore =
        successRateScore * 0.25f +      // 25% - Auction success rate
        pricingScore * 0.20f +          // 20% - Pricing consistency
        velocityScore * 0.15f +         // 15% - Sale velocity
        volumeScore * 0.15f +           // 15% - Transaction volume
        manipulationScore * 0.15f +     // 15% - Market manipulation detection
        recencyScore * 0.10f;           // 10% - Recency bonus

    // Apply reputation decay for inactive sellers
    if (daysSinceActivity > 30)
    {
        float decayFactor = ::std::max(0.5f, 1.0f - (float(daysSinceActivity - 30) / 180.0f));
        // Decay towards neutral (0.5) over time
        finalScore = 0.5f + (finalScore - 0.5f) * decayFactor;
    }

    // Clamp final score to valid range
    finalScore = ::std::clamp(finalScore, 0.0f, 1.0f);

    // Update cached reputation score
    rep.reputationScore = finalScore;
    rep.lastCalculationTime = currentTime;

    return finalScore;
}

bool MarketAnalysis::IsMarketDominated(uint32 itemId, float threshold)
{
    CompetitorAnalysis analysis = AnalyzeCompetition(itemId);
    return analysis.marketConcentration > threshold;
}

float MarketAnalysis::GetSeasonalityFactor(uint32 itemId, uint32 timestamp)
{
    if (timestamp == 0)
        timestamp = GameTime::GetGameTimeMS();

    // Calculate seasonal adjustments based on time of year, events, etc.
    // This is a simplified implementation

    return 1.0f; // No seasonal adjustment by default
}

MarketTrend MarketAnalysis::GetSegmentTrend(MarketSegment segment)
{
    auto segmentIt = _segmentItems.find(segment);
    if (segmentIt == _segmentItems.end())
        return MarketTrend::STABLE;

    // Analyze trends across all items in the segment
    ::std::vector<MarketTrend> itemTrends;
    for (uint32 itemId : segmentIt->second)
    {
        itemTrends.push_back(GetMarketTrend(itemId, 7));
    }

    // Determine overall segment trend
    int risingCount = 0, fallingCount = 0, stableCount = 0;

    for (MarketTrend trend : itemTrends)
    {
        switch (trend)
        {
            case MarketTrend::RISING:
            case MarketTrend::BULLISH:
                risingCount++;
                break;
            case MarketTrend::FALLING:
            case MarketTrend::BEARISH:
                fallingCount++;
                break;
            default:
                stableCount++;
                break;
        }
    }

    float total = float(itemTrends.size());
    if (risingCount / total > 0.6f)
        return MarketTrend::RISING;
    else if (fallingCount / total > 0.6f)
        return MarketTrend::FALLING;
    else
        return MarketTrend::STABLE;
}

void MarketAnalysis::SetAnalysisDepth(float depth)
{
    _analysisDepth = ::std::clamp(depth, 0.0f, 1.0f);
}

void MarketAnalysis::UpdatePredictionAccuracy(uint32 itemId, float predictedPrice, float actualPrice)
{
    if (predictedPrice <= 0.0f || actualPrice <= 0.0f)
        return;

    float error = ::std::abs(predictedPrice - actualPrice) / actualPrice;
    bool accurate = error < 0.1f; // 10% tolerance
    if (accurate)
        _metrics.accuratePredictions++;

    // Update model accuracy for this item
    auto modelIt = _predictionModels.find(itemId);
    if (modelIt != _predictionModels.end())
    {
        PredictionModel& model = modelIt->second;
        model.accuracy = (model.accuracy * 0.9f) + (accurate ? 0.1f : 0.0f);
        model.trainingSamples++;
    }
}

void MarketAnalysis::LearnFromMarketEvents()
{
    // Implement machine learning to improve predictions
    // Adjust model weights based on prediction accuracy
    // Update trend detection algorithms
}

void MarketAnalysis::InitializeSegmentMappings()
{
    // Initialize item categorization into market segments
    // This would typically be loaded from database or configuration
}

MarketSegment MarketAnalysis::DetermineItemSegment(uint32 itemId)
{
    const ItemTemplate* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
    if (!itemTemplate)
        return MarketSegment::TRADE_GOODS;

    // Categorize based on item class and subclass
    switch (itemTemplate->GetClass())
    {
        case ITEM_CLASS_WEAPON:
        case ITEM_CLASS_ARMOR:
            return MarketSegment::EQUIPMENT;
        case ITEM_CLASS_CONSUMABLE:
            return MarketSegment::CONSUMABLES;
        case ITEM_CLASS_TRADE_GOODS:
            return MarketSegment::CRAFTING;
        case ITEM_CLASS_GEM:
            return MarketSegment::GEMS;
        case ITEM_CLASS_MISCELLANEOUS:
            return MarketSegment::COLLECTIBLES;
        case ITEM_CLASS_QUEST:
            return MarketSegment::QUEST_ITEMS;
        default:
            return MarketSegment::TRADE_GOODS;
    }
}

void MarketAnalysis::UpdateMovingAverages(uint32 itemId)
{
    auto historyIt = _priceHistory.find(itemId);
    if (historyIt == _priceHistory.end())
        return;

    const auto& history = historyIt->second;
    if (history.size() < 7) // Need at least 7 data points
        return;

    ::std::vector<float> prices;
    for (const auto& snapshot : history)
        prices.push_back(snapshot.averagePrice);

    // Calculate moving averages for different windows
    for (uint32 window : MOVING_AVERAGE_WINDOWS)
    {
        auto ma = CalculateMovingAverage(prices, window);
        // Store moving averages in item metrics
    }
}

float MarketAnalysis::CalculateStandardDeviation(const ::std::vector<float>& prices)
{
    if (prices.empty())
        return 0.0f;

    float mean = ::std::accumulate(prices.begin(), prices.end(), 0.0f) / prices.size();

    float sumSquaredDiffs = 0.0f;
    for (float price : prices)
    {
        float diff = price - mean;
        sumSquaredDiffs += diff * diff;
    }

    return ::std::sqrt(sumSquaredDiffs / prices.size());
}

float MarketAnalysis::CalculateCorrelation(const ::std::vector<float>& series1, const ::std::vector<float>& series2)
{
    if (series1.size() != series2.size() || series1.empty())
        return 0.0f;

    float mean1 = ::std::accumulate(series1.begin(), series1.end(), 0.0f) / series1.size();
    float mean2 = ::std::accumulate(series2.begin(), series2.end(), 0.0f) / series2.size();

    float numerator = 0.0f;
    float sumSq1 = 0.0f;
    float sumSq2 = 0.0f;

    for (size_t i = 0; i < series1.size(); ++i)
    {
        float diff1 = series1[i] - mean1;
        float diff2 = series2[i] - mean2;

        numerator += diff1 * diff2;
        sumSq1 += diff1 * diff1;
        sumSq2 += diff2 * diff2;
    }

    float denominator = ::std::sqrt(sumSq1 * sumSq2);
    return (denominator > 0.0f) ? (numerator / denominator) : 0.0f;
}

::std::vector<float> MarketAnalysis::CalculateMovingAverage(const ::std::vector<float>& prices, uint32 window)
{
    ::std::vector<float> movingAverages;

    if (prices.size() < window)
        return movingAverages;

    for (size_t i = window - 1; i < prices.size(); ++i)
    {
        float sum = 0.0f;
        for (size_t j = i - window + 1; j <= i; ++j)
            sum += prices[j];

        movingAverages.push_back(sum / window);
    }

    return movingAverages;
}

float MarketAnalysis::CalculateVolatility(const ::std::vector<float>& prices)
{
    if (prices.size() < 2)
        return 0.0f;

    ::std::vector<float> returns;
    for (size_t i = 1; i < prices.size(); ++i)
    {
        if (prices[i-1] > 0.0f)
            returns.push_back((prices[i] - prices[i-1]) / prices[i-1]);
    }

    return CalculateStandardDeviation(returns);
}

MarketTrend MarketAnalysis::AnalyzeTrendDirection(const ::std::vector<float>& prices)
{
    if (prices.size() < 3)
        return MarketTrend::STABLE;

    float trendStrength = CalculateTrendStrength(prices);

    // Calculate overall direction
    float firstHalf = ::std::accumulate(prices.begin(), prices.begin() + prices.size()/2, 0.0f) / (prices.size()/2);
    float secondHalf = ::std::accumulate(prices.begin() + prices.size()/2, prices.end(), 0.0f) / (prices.size() - prices.size()/2);

    float change = (secondHalf - firstHalf) / firstHalf;

    if (::std::abs(change) < TREND_THRESHOLD)
        return MarketTrend::STABLE;

    if (change > 0)
    {
        return (trendStrength > 0.7f) ? MarketTrend::BULLISH : MarketTrend::RISING;
    }
    else
    {
        return (trendStrength > 0.7f) ? MarketTrend::BEARISH : MarketTrend::FALLING;
    }
}

float MarketAnalysis::CalculateTrendStrength(const ::std::vector<float>& prices)
{
    if (prices.size() < 2)
        return 0.0f;

    // Calculate linear regression slope as trend strength indicator
    float n = static_cast<float>(prices.size());
    float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumX2 = 0.0f;

    for (size_t i = 0; i < prices.size(); ++i)
    {
        float x = static_cast<float>(i);
        float y = prices[i];

        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }

    float slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    float avgPrice = sumY / n;

    // Normalize slope by average price to get relative trend strength
    return ::std::abs(slope) / avgPrice;
}

bool MarketAnalysis::DetectTrendReversal(const ::std::vector<float>& prices)
{
    if (prices.size() < 10)
        return false;

    // Look for significant change in trend direction
    size_t midPoint = prices.size() / 2;

    ::std::vector<float> firstHalf(prices.begin(), prices.begin() + midPoint);
    ::std::vector<float> secondHalf(prices.begin() + midPoint, prices.end());

    MarketTrend firstTrend = AnalyzeTrendDirection(firstHalf);
    MarketTrend secondTrend = AnalyzeTrendDirection(secondHalf);

    // Check for trend reversal
    bool reversal = (firstTrend == MarketTrend::RISING && secondTrend == MarketTrend::FALLING) ||
                   (firstTrend == MarketTrend::FALLING && secondTrend == MarketTrend::RISING) ||
                   (firstTrend == MarketTrend::BULLISH && secondTrend == MarketTrend::BEARISH) ||
                   (firstTrend == MarketTrend::BEARISH && secondTrend == MarketTrend::BULLISH);

    return reversal;
}

float MarketAnalysis::CalculateMomentum(const ::std::vector<float>& prices)
{
    if (prices.size() < 5)
        return 0.0f;

    // Calculate rate of change over recent periods
    size_t period = ::std::min(size_t(5), prices.size());
    float recent = prices[prices.size() - 1];
    float past = prices[prices.size() - period];

    if (past > 0.0f)
        return (recent - past) / past;

    return 0.0f;
}

float MarketAnalysis::PredictLinearRegression(uint32 itemId, uint32 hoursAhead)
{
    auto historyIt = _priceHistory.find(itemId);
    if (historyIt == _priceHistory.end() || historyIt->second.size() < MIN_SAMPLES_FOR_PREDICTION)
        return 0.0f;

    const auto& history = historyIt->second;
    ::std::vector<float> prices;
    for (const auto& snapshot : history)
        prices.push_back(snapshot.averagePrice);

    // Perform linear regression
    float n = static_cast<float>(prices.size());
    float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumX2 = 0.0f;

    for (size_t i = 0; i < prices.size(); ++i)
    {
        float x = static_cast<float>(i);
        float y = prices[i];

        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }

    float slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    float intercept = (sumY - slope * sumX) / n;

    // Predict future price
    float futureX = n + (hoursAhead / 24.0f); // Convert hours to days
    return slope * futureX + intercept;
}

float MarketAnalysis::PredictMovingAverage(uint32 itemId, uint32 hoursAhead)
{
    auto historyIt = _priceHistory.find(itemId);
    if (historyIt == _priceHistory.end() || historyIt->second.size() < MIN_SAMPLES_FOR_PREDICTION)
        return 0.0f;

    const auto& history = historyIt->second;
    ::std::vector<float> prices;
    for (const auto& snapshot : history)
        prices.push_back(snapshot.averagePrice);

    // Use exponential moving average for prediction
    float alpha = 0.3f; // Smoothing factor
    float ema = prices[0];

    for (size_t i = 1; i < prices.size(); ++i)
    {
        ema = alpha * prices[i] + (1.0f - alpha) * ema;
    }

    return ema; // Simple prediction - EMA tends to continue
}

float MarketAnalysis::PredictSeasonalAdjusted(uint32 itemId, uint32 hoursAhead)
{
    float basePrediction = PredictLinearRegression(itemId, hoursAhead);
    float seasonalFactor = GetSeasonalityFactor(itemId, GameTime::GetGameTimeMS() + hoursAhead * 3600000);

    return basePrediction * seasonalFactor;
}

void MarketAnalysis::TrainPredictionModel(uint32 itemId)
{
    // Implement model training for better predictions
    // This would use machine learning techniques in a full implementation
}

::std::vector<MarketOpportunity> MarketAnalysis::ScanForPriceDiscrepancies()
{
    ::std::vector<MarketOpportunity> opportunities;

    // Scan for items priced significantly below fair value
    for (const auto& historyPair : _priceHistory)
    {
        uint32 itemId = historyPair.first;
        MarketSnapshot snapshot = GetMarketSnapshot(itemId);

        if (snapshot.activeListings == 0)
            continue;

        float fairValue = CalculateFairValue(itemId);
        if (fairValue > 0.0f && snapshot.averagePrice < fairValue * 0.8f)
        {
            MarketOpportunity opp;
            opp.itemId = itemId;
            opp.currentPrice = snapshot.averagePrice;
            opp.targetPrice = fairValue;
            opp.potentialProfit = fairValue - snapshot.averagePrice;
            opp.confidence = 0.8f;
            opp.reason = "Price discrepancy - below fair value";

            if (ValidateOpportunity(opp))
                opportunities.push_back(opp);
        }
    }

    return opportunities;
}

::std::vector<MarketOpportunity> MarketAnalysis::ScanForTrendBreakouts()
{
    ::std::vector<MarketOpportunity> opportunities;

    // Look for items breaking out of established trends
    for (const auto& historyPair : _priceHistory)
    {
        uint32 itemId = historyPair.first;
        const auto& history = historyPair.second;

        if (history.size() < 20) // Need sufficient history
            continue;

        ::std::vector<float> prices;
        for (const auto& snapshot : history)
            prices.push_back(snapshot.averagePrice);

        if (DetectTrendReversal(prices))
        {
            MarketSnapshot current = GetMarketSnapshot(itemId);
            float prediction = GetPricePrediction(itemId, 48); // 48 hours
    if (prediction > current.averagePrice * 1.1f) // 10% upside
            {
                MarketOpportunity opp;
                opp.itemId = itemId;
                opp.currentPrice = current.averagePrice;
                opp.targetPrice = prediction;
                opp.potentialProfit = prediction - current.averagePrice;
                opp.confidence = 0.6f;
                opp.timeToTarget = 48;
                opp.reason = "Trend breakout detected";

                if (ValidateOpportunity(opp))
                    opportunities.push_back(opp);
            }
        }
    }

    return opportunities;
}

::std::vector<MarketOpportunity> MarketAnalysis::ScanForMeanReversion()
{
    ::std::vector<MarketOpportunity> opportunities;

    // Look for items that have deviated significantly from their mean and may revert
    for (const auto& historyPair : _priceHistory)
    {
        uint32 itemId = historyPair.first;
        const auto& history = historyPair.second;

        if (history.size() < 30) // Need sufficient history for mean reversion
            continue;

        ::std::vector<float> prices;
        for (const auto& snapshot : history)
            prices.push_back(snapshot.averagePrice);

        float mean = ::std::accumulate(prices.begin(), prices.end(), 0.0f) / prices.size();
        float stdDev = CalculateStandardDeviation(prices);
        float currentPrice = prices.back();

        // Check if current price is more than 2 standard deviations from mean
    if (::std::abs(currentPrice - mean) > 2.0f * stdDev)
        {
            MarketOpportunity opp;
            opp.itemId = itemId;
            opp.currentPrice = currentPrice;
            opp.targetPrice = mean;
            opp.potentialProfit = ::std::abs(mean - currentPrice);
            opp.confidence = 0.7f;
            opp.timeToTarget = 72; // 3 days for mean reversion
            opp.reason = "Mean reversion opportunity";

            if (ValidateOpportunity(opp))
                opportunities.push_back(opp);
        }
    }

    return opportunities;
}

bool MarketAnalysis::ValidateOpportunity(const MarketOpportunity& opportunity)
{
    // Validate that the opportunity meets minimum criteria
    if (opportunity.potentialProfit < 10.0f) // Minimum profit threshold
        return false;

    if (opportunity.confidence < 0.5f) // Minimum confidence threshold
        return false;

    if (opportunity.currentPrice <= 0.0f || opportunity.targetPrice <= 0.0f)
        return false;

    return true;
}

void MarketAnalysis::Update(uint32 diff)
{
    static uint32 lastUpdate = 0;
    uint32 currentTime = GameTime::GetGameTimeMS();

    if (currentTime - lastUpdate < ANALYSIS_UPDATE_INTERVAL)
        return;

    lastUpdate = currentTime;

    // Update trend analysis
    UpdateTrendAnalysis();

    // Clean up old data
    CleanupOldData();

    // Recalibrate models periodically
    static uint32 lastRecalibration = 0;
    if (currentTime - lastRecalibration > MODEL_TRAINING_INTERVAL)
    {
        RecalibrateModels();
        lastRecalibration = currentTime;
    }
}

void MarketAnalysis::UpdateTrendAnalysis()
{
    // Update trend analysis for all tracked items
    for (auto& historyPair : _priceHistory)
    {
        uint32 itemId = historyPair.first;
        auto& metrics = _itemMetrics[itemId];

        // Update trend information
    if (!historyPair.second.empty())
        {
            ::std::vector<float> prices;
            for (const auto& snapshot : historyPair.second)
                prices.push_back(snapshot.averagePrice);

            metrics.currentTrend = AnalyzeTrendDirection(prices);
            metrics.momentum = CalculateMomentum(prices);
        }
    }
}

void MarketAnalysis::CleanupOldData()
{
    ::std::lock_guard lock(_marketMutex);

    uint32 currentTime = GameTime::GetGameTimeMS();
    uint32 cutoffTime = currentTime - (_maxHistoryDays * 24 * 60 * 60 * 1000);

    // Clean up old price history
    for (auto& historyPair : _priceHistory)
    {
        auto& history = historyPair.second;
        history.erase(
            ::std::remove_if(history.begin(), history.end(),
                [cutoffTime](const MarketSnapshot& snapshot) {
                    return snapshot.timestamp < cutoffTime;
                }),
            history.end()
        );
    }

    // Remove items with no recent data
    for (auto it = _priceHistory.begin(); it != _priceHistory.end();)
    {
        if (it->second.empty())
            it = _priceHistory.erase(it);
        else
            ++it;
    }
}

void MarketAnalysis::RecalibrateModels()
{
    // Recalibrate prediction models based on recent performance
    for (auto& modelPair : _predictionModels)
    {
        uint32 itemId = modelPair.first;
        RecalibrateModel(itemId);
    }
}

void MarketAnalysis::RecalibrateModel(uint32 itemId)
{
    // Recalibrate prediction model for specific item
    auto modelIt = _predictionModels.find(itemId);
    if (modelIt == _predictionModels.end())
    {
        _predictionModels[itemId] = PredictionModel();
        modelIt = _predictionModels.find(itemId);
    }

    PredictionModel& model = modelIt->second;
    model.lastTraining = GameTime::GetGameTimeMS();

    // Update model parameters based on recent performance
    // This would involve more sophisticated machine learning in a full implementation
}

} // namespace Playerbot