/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "AuctionHouse.h"
#include "GameTime.h"
#include <algorithm>
#include <cmath>

namespace Playerbot
{

// Static member definitions
std::atomic<uint32> AuctionHouse::_nextSessionId{1};
std::unordered_map<uint32, AuctionHouse::MarketData> AuctionHouse::_marketData;
std::unordered_map<uint32, std::vector<AuctionItem>> AuctionHouse::_auctionCache;
std::unordered_map<uint32, AuctionHouse::CompetitorProfile> AuctionHouse::_competitors;
AuctionMetrics AuctionHouse::_globalMetrics;
Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::TRADE_MANAGER> AuctionHouse::_marketMutex;

// Constructor
AuctionHouse::AuctionHouse(Player* bot)
    : _bot(bot)
{
    if (!_bot)
    {
        TC_LOG_ERROR("playerbot.auction", "AuctionHouse: Attempted to create with null bot!");
        return;
    }

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Created for bot {} ({})",
                 _bot->GetName(), _bot->GetGUID().ToString());
}

// Destructor
AuctionHouse::~AuctionHouse()
{
    // CRITICAL: Do NOT access _bot->GetName() or GetGUID().ToString() during destructor!
    // During bot destruction, the Player's internal string data may already be
    // freed even if the pointer is valid. This causes ACCESS_VIOLATION in
    // the string formatting code (VCRUNTIME140.dll).
    //
    // If logging is needed, use GUID counter instead:
    // if (_bot)
    //     TC_LOG_DEBUG("playerbot.auction",
    //         "AuctionHouse destroyed for bot {}", _bot->GetGUID().GetCounter());
}

// ============================================================================
// Core auction house operations using TrinityCore's AuctionHouseMgr
// ============================================================================

void AuctionHouse::SearchAuctionHouse(const AuctionSearchQuery& query)
{
    if (!_bot || !_auctionHouseEnabled)
        return;

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Bot {} searching auction house", _bot->GetName());

    AuctionHouseObject* auctionHouse = GetAuctionHouseForPlayer();
    if (!auctionHouse)
    {
        TC_LOG_ERROR("playerbot.auction", "AuctionHouse: No auction house available for bot {}", _bot->GetName());
        return;
    }

    std::vector<AuctionItem> results;
    std::lock_guard<decltype(_marketMutex)> lock(_marketMutex);

    // Iterate through all auctions
    for (auto itr = auctionHouse->GetAuctionsBegin(); itr != auctionHouse->GetAuctionsEnd(); ++itr)
    {
        AuctionPosting* auction = &itr->second;
        if (!auction || auction->Items.empty())
            continue;

        Item* item = auction->Items.front();
        if (!item)
            continue;

        AuctionItem auctionItem;
        auctionItem.auctionId = auction->Id;
        auctionItem.itemId = item->GetEntry();
        auctionItem.itemGuid = item->GetGUID().GetCounter();
        auctionItem.stackCount = auction->GetTotalItemCount();
        auctionItem.currentBid = auction->BidAmount;
        auctionItem.buyoutPrice = auction->BuyoutOrUnitPrice;
        auctionItem.sellerGuid = auction->Owner.GetCounter();
        auctionItem.quality = item->GetQuality();
        auctionItem.itemLevel = item->GetItemLevel(_bot);
        auctionItem.lastSeen = GameTime::GetGameTimeMS();

        if (MatchesSearchCriteria(auctionItem, query))
            results.push_back(auctionItem);

        if (results.size() >= MAX_SEARCH_RESULTS)
            break;
    }

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Found {} results for bot {}", results.size(), _bot->GetName());
    _metrics.marketScans++;
    _globalMetrics.marketScans++;
}

bool AuctionHouse::PlaceAuctionBid(uint32 auctionId, uint32 bidAmount)
{
    if (!_bot || !_auctionHouseEnabled)
        return false;

    AuctionHouseObject* auctionHouse = GetAuctionHouseForPlayer();
    if (!auctionHouse)
        return false;

    AuctionPosting* auction = auctionHouse->GetAuction(auctionId);
    if (!auction)
    {
        TC_LOG_ERROR("playerbot.auction", "AuctionHouse: Auction {} not found", auctionId);
        return false;
    }

    if (_bot->GetMoney() < bidAmount)
    {
        TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Bot {} lacks funds for bid (has: {}, needs: {})",
            _bot->GetName(), _bot->GetMoney(), bidAmount);
        HandleInsufficientFunds(bidAmount);
        return false;
    }

    if (auction->Owner == _bot->GetGUID())
    {
        TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Bot {} cannot bid on own auction", _bot->GetName());
        return false;
    }

    if (bidAmount < auction->BidAmount + auction->CalculateMinIncrement())
    {
        TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Bid amount {} too low for auction {}", bidAmount, auctionId);
        return false;
    }

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Bot {} placing bid {} on auction {}",
        _bot->GetName(), bidAmount, auctionId);

    _bot->ModifyMoney(-static_cast<int64>(bidAmount));
    UpdateAuctionMetrics(_bot->GetGUID().GetCounter(), AuctionActionType::UPDATE_BID, bidAmount, true);

    return true;
}

bool AuctionHouse::BuyoutAuction(uint32 auctionId)
{
    if (!_bot || !_auctionHouseEnabled)
        return false;

    AuctionHouseObject* auctionHouse = GetAuctionHouseForPlayer();
    if (!auctionHouse)
        return false;

    AuctionPosting* auction = auctionHouse->GetAuction(auctionId);
    if (!auction)
    {
        TC_LOG_ERROR("playerbot.auction", "AuctionHouse: Auction {} not found", auctionId);
        return false;
    }

    uint64 buyoutPrice = auction->BuyoutOrUnitPrice;
    if (buyoutPrice == 0)
    {
        TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Auction {} has no buyout price", auctionId);
        return false;
    }

    if (_bot->GetMoney() < buyoutPrice)
    {
        HandleInsufficientFunds(buyoutPrice);
        return false;
    }

    if (auction->Owner == _bot->GetGUID())
    {
        TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Bot {} cannot buy own auction", _bot->GetName());
        return false;
    }

    TC_LOG_INFO("playerbot.auction", "AuctionHouse: Bot {} buying out auction {} for {}",
        _bot->GetName(), auctionId, buyoutPrice);

    _bot->ModifyMoney(-static_cast<int64>(buyoutPrice));
    _metrics.itemsPurchased++;
    _metrics.totalGoldSpent += buyoutPrice;
    _globalMetrics.itemsPurchased++;
    _globalMetrics.totalGoldSpent += buyoutPrice;

    UpdateAuctionMetrics(_bot->GetGUID().GetCounter(), AuctionActionType::BUY_ITEM, buyoutPrice, true);

    return true;
}

bool AuctionHouse::CreateAuction(uint32 itemGuid, uint32 stackCount, uint32 bid, uint32 buyout, uint32 duration)
{
    if (!_bot || !_auctionHouseEnabled)
        return false;

    Item* item = _bot->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGuid));
    if (!item)
    {
        TC_LOG_ERROR("playerbot.auction", "AuctionHouse: Bot {} item {} not found", _bot->GetName(), itemGuid);
        return false;
    }

    if (stackCount > item->GetCount())
    {
        TC_LOG_ERROR("playerbot.auction", "AuctionHouse: Stack count {} exceeds item count {}", stackCount, item->GetCount());
        return false;
    }

    AuctionHouseObject* auctionHouse = GetAuctionHouseForPlayer();
    if (!auctionHouse)
        return false;

    uint32 auctionTime = duration * MINUTE;
    uint64 deposit = AuctionHouseMgr::GetItemAuctionDeposit(_bot, item, Minutes(duration));

    if (_bot->GetMoney() < deposit)
    {
        HandleInsufficientFunds(deposit);
        return false;
    }

    TC_LOG_INFO("playerbot.auction", "AuctionHouse: Bot {} creating auction for item {} (bid: {}, buyout: {}, duration: {})",
        _bot->GetName(), item->GetEntry(), bid, buyout, duration);

    _bot->ModifyMoney(-static_cast<int64>(deposit));
    _metrics.auctionsCreated++;
    _globalMetrics.auctionsCreated++;

    UpdateAuctionMetrics(_bot->GetGUID().GetCounter(), AuctionActionType::SELL_ITEM, deposit, true);

    return true;
}

bool AuctionHouse::CancelAuction(uint32 auctionId)
{
    if (!_bot || !_auctionHouseEnabled)
        return false;

    AuctionHouseObject* auctionHouse = GetAuctionHouseForPlayer();
    if (!auctionHouse)
        return false;

    AuctionPosting* auction = auctionHouse->GetAuction(auctionId);
    if (!auction)
    {
        TC_LOG_ERROR("playerbot.auction", "AuctionHouse: Auction {} not found", auctionId);
        return false;
    }

    if (auction->Owner != _bot->GetGUID())
    {
        TC_LOG_ERROR("playerbot.auction", "AuctionHouse: Bot {} does not own auction {}", _bot->GetName(), auctionId);
        return false;
    }

    TC_LOG_INFO("playerbot.auction", "AuctionHouse: Bot {} cancelling auction {}", _bot->GetName(), auctionId);

    _metrics.auctionsCancelled++;
    _globalMetrics.auctionsCancelled++;

    UpdateAuctionMetrics(_bot->GetGUID().GetCounter(), AuctionActionType::CANCEL_AUCTION, 0, true);

    return true;
}

// ============================================================================
// Intelligent auction strategies
// ============================================================================

void AuctionHouse::ExecuteAuctionStrategy(AuctionHouseStrategy strategy)
{
    if (!_bot || !_auctionHouseEnabled)
        return;

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Bot {} executing strategy {}", _bot->GetName(), static_cast<uint8>(strategy));

    uint32 sessionId = StartAuctionSession(AuctionActionType::SEARCH_MARKET);
    AuctionSession& session = _activeSessions[sessionId];

    switch (strategy)
    {
        case AuctionHouseStrategy::CONSERVATIVE:
            ExecuteConservativeStrategy(session);
            break;
        case AuctionHouseStrategy::AGGRESSIVE:
            ExecuteAggressiveStrategy(session);
            break;
        case AuctionHouseStrategy::OPPORTUNISTIC:
            ExecuteOpportunisticStrategy(session);
            break;
        case AuctionHouseStrategy::MARKET_MAKER:
            ExecuteMarketMakerStrategy(session);
            break;
        case AuctionHouseStrategy::COLLECTOR:
            ExecuteCollectorStrategy(session);
            break;
        case AuctionHouseStrategy::PROFIT_FOCUSED:
            ExecuteProfitFocusedStrategy(session);
            break;
        default:
            break;
    }

    CompleteAuctionSession(sessionId);
}

void AuctionHouse::ScanForBargains()
{
    if (!_bot || !_auctionHouseEnabled)
        return;

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Bot {} scanning for bargains", _bot->GetName());

    std::vector<AuctionItem> bargains = FindBargainAuctions(BARGAIN_THRESHOLD);

    for (const auto& auction : bargains)
    {
        if (IsWorthBuying(auction) && _bot->GetMoney() >= auction.buyoutPrice)
        {
            if (BuyoutAuction(auction.auctionId))
            {
                _metrics.bargainsFound++;
                _globalMetrics.bargainsFound++;
                TC_LOG_INFO("playerbot.auction", "AuctionHouse: Bot {} found bargain: item {} for {}",
                    _bot->GetName(), auction.itemId, auction.buyoutPrice);
            }
        }
    }
}

void AuctionHouse::AutoSellItems(const std::vector<uint32>& itemGuids)
{
    if (!_bot || !_auctionHouseEnabled)
        return;

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Bot {} auto-selling {} items", _bot->GetName(), itemGuids.size());

    for (uint32 itemGuid : itemGuids)
    {
        Item* item = _bot->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGuid));
        if (!item)
            continue;

        uint32 itemId = item->GetEntry();
        uint32 stackCount = item->GetCount();
        uint32 optimalPrice = CalculateOptimalListingPrice(itemId, stackCount);

        if (optimalPrice > 0)
        {
            uint32 bid = static_cast<uint32>(optimalPrice * 0.8f);
            CreateAuction(itemGuid, stackCount, bid, optimalPrice, 24 * 60);
        }
    }
}

void AuctionHouse::AutoBuyNeededItems()
{
    if (!_bot || !_auctionHouseEnabled)
        return;

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Bot {} auto-buying needed items", _bot->GetName());

    for (uint32 itemId : _profile.watchList)
    {
        std::vector<AuctionItem> auctions = GetSimilarAuctions(itemId, 5);

        for (const auto& auction : auctions)
        {
            if (IsWorthBuying(auction) && _bot->GetMoney() >= auction.buyoutPrice)
            {
                if (BuyoutAuction(auction.auctionId))
                    break;
            }
        }
    }
}

void AuctionHouse::ManageActiveAuctions()
{
    if (!_bot || !_auctionHouseEnabled)
        return;

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Bot {} managing active auctions", _bot->GetName());

    AuctionHouseObject* auctionHouse = GetAuctionHouseForPlayer();
    if (!auctionHouse)
        return;

    uint32 activeCount = 0;
    SystemTimePoint now = SystemTimePoint::clock::now();

    for (auto itr = auctionHouse->GetAuctionsBegin(); itr != auctionHouse->GetAuctionsEnd(); ++itr)
    {
        AuctionPosting* auction = &itr->second;
        if (!auction || auction->Owner != _bot->GetGUID())
            continue;

        activeCount++;

        if (auction->Items.empty())
            continue;

        Item* item = auction->Items.front();
        if (!item)
            continue;

        uint32 itemId = item->GetEntry();
        float marketPrice = GetMarketPrice(itemId, auction->GetTotalItemCount());

        if (marketPrice > 0 && auction->BuyoutOrUnitPrice > marketPrice * 1.5f)
        {
            if (_profile.autoRelist && now > auction->EndTime - std::chrono::hours(2))
            {
                TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Considering relisting auction {} for better price", auction->Id);
            }
        }
    }

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Bot {} has {} active auctions", _bot->GetName(), activeCount);
}

// ============================================================================
// Market analysis and price discovery
// ============================================================================

float AuctionHouse::GetMarketPrice(uint32 itemId, uint32 stackSize)
{
    std::lock_guard<decltype(_marketMutex)> lock(_marketMutex);

    auto it = _marketData.find(itemId);
    if (it != _marketData.end() && it->second.averagePrice > 0.0f)
    {
        return it->second.averagePrice * stackSize;
    }

    UpdateItemMarketData(itemId);

    it = _marketData.find(itemId);
    if (it != _marketData.end())
        return it->second.averagePrice * stackSize;

    return 0.0f;
}

float AuctionHouse::GetPriceHistory(uint32 itemId, uint32 days)
{
    std::lock_guard<decltype(_marketMutex)> lock(_marketMutex);

    auto it = _marketData.find(itemId);
    if (it == _marketData.end() || it->second.priceHistory.empty())
        return 0.0f;

    uint32 cutoffTime = GameTime::GetGameTimeMS() - (days * 24 * 60 * 60 * 1000);
    uint64 totalPrice = 0;
    uint32 count = 0;

    for (const auto& [price, timestamp] : it->second.priceHistory)
    {
        if (timestamp >= cutoffTime)
        {
            totalPrice += price;
            count++;
        }
    }

    return count > 0 ? static_cast<float>(totalPrice) / count : 0.0f;
}

std::vector<AuctionItem> AuctionHouse::GetSimilarAuctions(uint32 itemId, uint32 maxResults)
{
    std::vector<AuctionItem> results;

    AuctionHouseObject* auctionHouse = GetAuctionHouseForPlayer();
    if (!auctionHouse)
        return results;

    std::lock_guard<decltype(_marketMutex)> lock(_marketMutex);

    for (auto itr = auctionHouse->GetAuctionsBegin(); itr != auctionHouse->GetAuctionsEnd() && results.size() < maxResults; ++itr)
    {
        AuctionPosting* auction = &itr->second;
        if (!auction || auction->Items.empty())
            continue;

        Item* item = auction->Items.front();
        if (!item || item->GetEntry() != itemId)
            continue;

        AuctionItem auctionItem;
        auctionItem.auctionId = auction->Id;
        auctionItem.itemId = itemId;
        auctionItem.itemGuid = item->GetGUID().GetCounter();
        auctionItem.stackCount = auction->GetTotalItemCount();
        auctionItem.currentBid = auction->BidAmount;
        auctionItem.buyoutPrice = auction->BuyoutOrUnitPrice;
        auctionItem.sellerGuid = auction->Owner.GetCounter();
        auctionItem.quality = item->GetQuality();
        auctionItem.itemLevel = item->GetItemLevel(_bot);
        auctionItem.lastSeen = GameTime::GetGameTimeMS();
        auctionItem.pricePerItem = auctionItem.stackCount > 0 ? static_cast<float>(auctionItem.buyoutPrice) / auctionItem.stackCount : 0.0f;

        results.push_back(auctionItem);
    }

    return results;
}

bool AuctionHouse::IsPriceBelowMarket(uint32 itemId, uint32 price, float threshold)
{
    float marketPrice = GetMarketPrice(itemId, 1);
    if (marketPrice <= 0.0f)
        return false;

    return price <= (marketPrice * threshold);
}

void AuctionHouse::UpdateMarketData()
{
    if (!_bot)
        return;

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Updating market data for bot {}", _bot->GetName());

    AuctionHouseObject* auctionHouse = GetAuctionHouseForPlayer();
    if (!auctionHouse)
        return;

    std::lock_guard<decltype(_marketMutex)> lock(_marketMutex);
    std::unordered_map<uint32, std::vector<uint64>> pricesByItem;

    for (auto itr = auctionHouse->GetAuctionsBegin(); itr != auctionHouse->GetAuctionsEnd(); ++itr)
    {
        AuctionPosting* auction = &itr->second;
        if (!auction || auction->Items.empty())
            continue;

        Item* item = auction->Items.front();
        if (!item)
            continue;

        uint32 itemId = item->GetEntry();
        uint64 pricePerItem = auction->GetTotalItemCount() > 0 ? auction->BuyoutOrUnitPrice / auction->GetTotalItemCount() : 0;

        if (pricePerItem > 0)
            pricesByItem[itemId].push_back(pricePerItem);
    }

    for (const auto& [itemId, prices] : pricesByItem)
    {
        if (prices.empty())
            continue;

        uint64 totalPrice = 0;
        for (uint64 price : prices)
            totalPrice += price;

        float avgPrice = static_cast<float>(totalPrice) / prices.size();

        auto& marketData = _marketData[itemId];
        marketData.itemId = itemId;
        marketData.averagePrice = avgPrice;
        marketData.activeListings = prices.size();
        marketData.lastAnalysisTime = GameTime::GetGameTimeMS();

        std::vector<uint64> sortedPrices = prices;
        std::sort(sortedPrices.begin(), sortedPrices.end());
        marketData.medianPrice = sortedPrices[sortedPrices.size() / 2];
    }

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Updated market data for {} items", pricesByItem.size());
}

// ============================================================================
// Advanced auction features
// ============================================================================

void AuctionHouse::SetAuctionProfile(const AuctionProfile& profile)
{
    _profile = profile;
    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Updated auction profile for bot {}", _bot ? _bot->GetName() : "unknown");
}

AuctionProfile AuctionHouse::GetAuctionProfile()
{
    return _profile;
}

// ============================================================================
// Auction monitoring and automation
// ============================================================================

uint32 AuctionHouse::StartAuctionSession(AuctionActionType primaryAction)
{
    if (!_bot)
        return 0;

    uint32 sessionId = _nextSessionId++;

    AuctionSession session(sessionId, primaryAction, GameTime::GetGameTimeMS());
    session.isActive = true;

    _activeSessions[sessionId] = session;

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Started session {} for bot {} (action: {})",
        sessionId, _bot->GetName(), static_cast<uint8>(primaryAction));

    return sessionId;
}

void AuctionHouse::UpdateAuctionSession(uint32 sessionId)
{
    auto it = _activeSessions.find(sessionId);
    if (it == _activeSessions.end())
        return;

    AuctionSession& session = it->second;

    if (!session.isActive)
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - session.sessionStartTime > SESSION_TIMEOUT)
    {
        TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Session {} timed out", sessionId);
        CompleteAuctionSession(sessionId);
        return;
    }

    ProcessActionQueue(session);
}

void AuctionHouse::CompleteAuctionSession(uint32 sessionId)
{
    auto it = _activeSessions.find(sessionId);
    if (it == _activeSessions.end())
        return;

    AuctionSession& session = it->second;
    session.isActive = false;

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Completed session {} (bought: {}, sold: {}, budget used: {})",
        sessionId, session.itemsBought, session.itemsSold, session.budgetUsed);

    _activeSessions.erase(it);
}

AuctionSession AuctionHouse::GetAuctionSession(uint32 sessionId)
{
    auto it = _activeSessions.find(sessionId);
    if (it != _activeSessions.end())
        return it->second;

    return AuctionSession();
}

// ============================================================================
// Price optimization and profit calculation
// ============================================================================

uint32 AuctionHouse::CalculateOptimalListingPrice(uint32 itemId, uint32 stackSize)
{
    float marketPrice = GetMarketPrice(itemId, 1);
    if (marketPrice <= 0.0f)
        return 0;

    float undercutRate = DEFAULT_UNDERCUT_RATE;

    switch (_profile.primaryStrategy)
    {
        case AuctionHouseStrategy::AGGRESSIVE:
            undercutRate = 0.10f;
            break;
        case AuctionHouseStrategy::QUICK_SALE:
            undercutRate = 0.20f;
            break;
        case AuctionHouseStrategy::CONSERVATIVE:
            undercutRate = 0.01f;
            break;
        case AuctionHouseStrategy::PREMIUM:
            undercutRate = -0.05f;
            break;
        default:
            break;
    }

    float optimalPrice = marketPrice * (1.0f - undercutRate) * stackSize;
    return static_cast<uint32>(std::max(1.0f, optimalPrice));
}

uint32 AuctionHouse::CalculateMaxBidAmount(uint32 itemId, uint32 stackSize)
{
    float marketPrice = GetMarketPrice(itemId, stackSize);
    if (marketPrice <= 0.0f)
        return 0;

    float maxBidRatio = _profile.bargainThreshold;
    uint32 maxBid = static_cast<uint32>(marketPrice * maxBidRatio);

    uint32 availableFunds = std::min(_bot->GetMoney(), static_cast<uint64>(_profile.maxBiddingBudget));

    return std::min(maxBid, static_cast<uint32>(availableFunds));
}

float AuctionHouse::CalculatePotentialProfit(const AuctionItem& auction, uint32 resellPrice)
{
    if (auction.buyoutPrice == 0 || resellPrice <= auction.buyoutPrice)
        return 0.0f;

    uint32 profit = resellPrice - auction.buyoutPrice;
    float auctionCut = resellPrice * 0.05f;

    float netProfit = profit - auctionCut;
    float profitMargin = netProfit / auction.buyoutPrice;

    return profitMargin;
}

bool AuctionHouse::IsWorthBuying(const AuctionItem& auction)
{
    if (auction.buyoutPrice == 0)
        return false;

    if (_profile.blackList.find(auction.itemId) != _profile.blackList.end())
        return false;

    if (!_bot || _bot->GetMoney() < auction.buyoutPrice)
        return false;

    float marketPrice = GetMarketPrice(auction.itemId, auction.stackCount);
    if (marketPrice <= 0.0f)
        return true;

    return auction.buyoutPrice <= (marketPrice * _profile.bargainThreshold);
}

bool AuctionHouse::ShouldUndercut(uint32 itemId, uint32 currentLowest)
{
    if (currentLowest == 0)
        return false;

    float marketPrice = GetMarketPrice(itemId, 1);
    if (marketPrice <= 0.0f)
        return false;

    float lowestRatio = currentLowest / marketPrice;

    if (lowestRatio < 0.7f)
        return false;

    return true;
}

// ============================================================================
// Market intelligence and learning
// ============================================================================

void AuctionHouse::TrackPriceMovement(uint32 itemId, uint32 price, uint32 timestamp)
{
    std::lock_guard<decltype(_marketMutex)> lock(_marketMutex);

    auto& marketData = _marketData[itemId];
    marketData.itemId = itemId;
    marketData.priceHistory.push_back({price, timestamp});

    if (marketData.priceHistory.size() > 100)
        marketData.priceHistory.erase(marketData.priceHistory.begin());

    uint32 cutoffTime = GameTime::GetGameTimeMS() - (PRICE_HISTORY_DAYS * 24 * 60 * 60 * 1000);
    marketData.priceHistory.erase(
        std::remove_if(marketData.priceHistory.begin(), marketData.priceHistory.end(),
            [cutoffTime](const auto& p) { return p.second < cutoffTime; }),
        marketData.priceHistory.end()
    );
}

void AuctionHouse::AnalyzeMarketTrends(uint32 itemId)
{
    std::lock_guard<decltype(_marketMutex)> lock(_marketMutex);

    auto it = _marketData.find(itemId);
    if (it == _marketData.end() || it->second.priceHistory.size() < 10)
        return;

    MarketData& data = it->second;
    data.volatility = CalculateMarketVolatility(data.priceHistory);

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Market volatility for item {}: {}", itemId, data.volatility);
}

void AuctionHouse::LearnFromAuctionOutcome(uint32 auctionId, bool wasSuccessful)
{
    if (wasSuccessful)
    {
        _metrics.auctionsSold++;
        _globalMetrics.auctionsSold++;
    }

    UpdateSuccessRates(wasSuccessful, AuctionActionType::SELL_ITEM);

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Learning from auction {} outcome: {}",
        auctionId, wasSuccessful ? "success" : "failure");
}

void AuctionHouse::AdaptAuctionBehavior()
{
    if (!_bot)
        return;

    AdaptPricingStrategy();

    float roi = _metrics.GetROI();
    if (roi < 0.9f)
    {
        if (_profile.primaryStrategy == AuctionHouseStrategy::AGGRESSIVE)
            _profile.primaryStrategy = AuctionHouseStrategy::CONSERVATIVE;
    }
    else if (roi > 1.5f)
    {
        if (_profile.primaryStrategy == AuctionHouseStrategy::CONSERVATIVE)
            _profile.primaryStrategy = AuctionHouseStrategy::AGGRESSIVE;
    }

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Adapted behavior for bot {} (ROI: {})", _bot->GetName(), roi);
}

// ============================================================================
// Specialized auction services
// ============================================================================

void AuctionHouse::HandleConsumableAutomation()
{
    if (!_bot || !_profile.autoBuyConsumables)
        return;

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Handling consumable automation for bot {}", _bot->GetName());

    std::vector<uint32> consumableItems = {858, 2512, 3385};

    for (uint32 itemId : consumableItems)
    {
        std::vector<AuctionItem> auctions = GetSimilarAuctions(itemId, 3);

        for (const auto& auction : auctions)
        {
            if (IsWorthBuying(auction) && _bot->GetMoney() >= auction.buyoutPrice)
            {
                BuyoutAuction(auction.auctionId);
                break;
            }
        }
    }
}

void AuctionHouse::HandleEquipmentUpgrades()
{
    if (!_bot)
        return;

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Checking equipment upgrades for bot {}", _bot->GetName());

    AuctionSearchQuery query;
    query.usableOnly = true;
    query.minItemLevel = _bot->GetAverageItemLevel() + 5;
    query.maxPrice = _bot->GetMoney() / 10;

    SearchAuctionHouse(query);
}

void AuctionHouse::HandleCraftingMaterials()
{
    if (!_bot)
        return;

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Handling crafting materials for bot {}", _bot->GetName());

    AuctionSearchQuery query;
    query.itemClasses = {7};
    query.maxPrice = _bot->GetMoney() / 20;

    SearchAuctionHouse(query);
}

void AuctionHouse::HandleCollectibleTrading()
{
    if (!_bot)
        return;

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Handling collectible trading for bot {}", _bot->GetName());

    std::vector<AuctionItem> collectibles = FindFlipOpportunities();

    for (const auto& auction : collectibles)
    {
        if (IsWorthBuying(auction))
            BuyoutAuction(auction.auctionId);
    }
}

void AuctionHouse::HandleBulkItemTrading()
{
    if (!_bot)
        return;

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Handling bulk item trading for bot {}", _bot->GetName());

    AuctionSearchQuery query;
    query.minQuality = 1;
    query.maxPrice = _bot->GetMoney() / 5;

    SearchAuctionHouse(query);
}

// ============================================================================
// Competition analysis
// ============================================================================

void AuctionHouse::AnalyzeCompetition(uint32 itemId)
{
    std::lock_guard<decltype(_marketMutex)> lock(_marketMutex);

    AuctionHouseObject* auctionHouse = GetAuctionHouseForPlayer();
    if (!auctionHouse)
        return;

    std::unordered_map<uint32, uint32> sellerCounts;

    for (auto itr = auctionHouse->GetAuctionsBegin(); itr != auctionHouse->GetAuctionsEnd(); ++itr)
    {
        AuctionPosting* auction = &itr->second;
        if (!auction || auction->Items.empty())
            continue;

        Item* item = auction->Items.front();
        if (!item || item->GetEntry() != itemId)
            continue;

        uint32 sellerGuid = auction->Owner.GetCounter();
        sellerCounts[sellerGuid]++;
    }

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Analyzed {} sellers for item {}", sellerCounts.size(), itemId);
}

std::vector<uint32> AuctionHouse::GetFrequentSellers(uint32 itemId)
{
    std::vector<uint32> sellers;
    std::lock_guard<decltype(_marketMutex)> lock(_marketMutex);

    for (const auto& [guid, profile] : _competitors)
    {
        for (uint32 frequentItem : profile.frequentItems)
        {
            if (frequentItem == itemId)
            {
                sellers.push_back(guid);
                break;
            }
        }
    }

    return sellers;
}

float AuctionHouse::GetCompetitorUndercutRate(uint32 sellerGuid)
{
    std::lock_guard<decltype(_marketMutex)> lock(_marketMutex);

    auto it = _competitors.find(sellerGuid);
    if (it != _competitors.end())
        return it->second.averageUndercutRate;

    return DEFAULT_UNDERCUT_RATE;
}

void AuctionHouse::TrackCompetitorBehavior(uint32 sellerGuid, const AuctionItem& auction)
{
    std::lock_guard<decltype(_marketMutex)> lock(_marketMutex);

    auto& competitor = _competitors[sellerGuid];
    competitor.sellerGuid = sellerGuid;
    competitor.totalAuctions++;
    competitor.lastActivity = GameTime::GetGameTimeMS();

    bool found = false;
    for (uint32 itemId : competitor.frequentItems)
    {
        if (itemId == auction.itemId)
        {
            found = true;
            break;
        }
    }

    if (!found && competitor.frequentItems.size() < 10)
        competitor.frequentItems.push_back(auction.itemId);

    competitor.pricingHistory.push_back({auction.itemId, auction.buyoutPrice});
    if (competitor.pricingHistory.size() > 50)
        competitor.pricingHistory.erase(competitor.pricingHistory.begin());
}

// ============================================================================
// Performance monitoring
// ============================================================================

AuctionMetrics AuctionHouse::GetAuctionMetrics()
{
    return _metrics;  // Uses copy constructor to load atomic values
}

AuctionMetrics AuctionHouse::GetGlobalAuctionMetrics()
{
    return _globalMetrics;  // Uses copy constructor to load atomic values
}

// ============================================================================
// Auction house integration with TrinityCore
// ============================================================================

void AuctionHouse::LoadAuctionData()
{
    if (!_bot)
        return;

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Loading auction data for bot {}", _bot->GetName());

    SynchronizeWithAuctionHouseMgr();
    UpdateMarketData();
}

void AuctionHouse::SynchronizeWithAuctionHouseMgr()
{
    if (!_bot)
        return;

    AuctionHouseObject* auctionHouse = GetAuctionHouseForPlayer();
    if (!auctionHouse)
        return;

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Synchronized with auction house manager for bot {}", _bot->GetName());
}

AuctionHouseObject* AuctionHouse::GetAuctionHouseForPlayer()
{
    if (!_bot)
        return nullptr;

    uint32 factionTemplateId = _bot->GetFaction();
    return sAuctionMgr->GetAuctionsMap(factionTemplateId);
}

bool AuctionHouse::ValidateAuctionAccess(uint32 auctionHouseId)
{
    if (!_bot)
        return false;

    return GetAuctionHouseForPlayer() != nullptr;
}

// ============================================================================
// Configuration and customization
// ============================================================================

void AuctionHouse::SetAuctionHouseEnabled(bool enabled)
{
    _auctionHouseEnabled = enabled;
    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: {} for bot {}",
        enabled ? "Enabled" : "Disabled", _bot ? _bot->GetName() : "unknown");
}

void AuctionHouse::SetMaxConcurrentAuctions(uint32 maxAuctions)
{
    _profile.maxAuctionsActive = maxAuctions;
    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Set max concurrent auctions to {} for bot {}",
        maxAuctions, _bot ? _bot->GetName() : "unknown");
}

void AuctionHouse::SetAuctionBudget(uint32 budget)
{
    _profile.maxBiddingBudget = budget;
    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Set auction budget to {} for bot {}",
        budget, _bot ? _bot->GetName() : "unknown");
}

void AuctionHouse::AddToWatchList(uint32 itemId)
{
    // Only add if not already in the list
    auto it = std::find(_profile.watchList.begin(), _profile.watchList.end(), itemId);
    if (it == _profile.watchList.end())
        _profile.watchList.push_back(itemId);
    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Added item {} to watch list for bot {}",
        itemId, _bot ? _bot->GetName() : "unknown");
}

void AuctionHouse::RemoveFromWatchList(uint32 itemId)
{
    auto it = std::find(_profile.watchList.begin(), _profile.watchList.end(), itemId);
    if (it != _profile.watchList.end())
        _profile.watchList.erase(it);
    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Removed item {} from watch list for bot {}",
        itemId, _bot ? _bot->GetName() : "unknown");
}

// ============================================================================
// Error handling and recovery
// ============================================================================

void AuctionHouse::HandleAuctionError(uint32 sessionId, const std::string& error)
{
    TC_LOG_ERROR("playerbot.auction", "AuctionHouse: Session {} error: {}", sessionId, error);

    auto it = _activeSessions.find(sessionId);
    if (it != _activeSessions.end())
        it->second.isActive = false;
}

void AuctionHouse::RecoverFromAuctionFailure(uint32 sessionId)
{
    auto it = _activeSessions.find(sessionId);
    if (it == _activeSessions.end())
        return;

    AuctionSession& session = it->second;

    while (!session.actionQueue.empty())
        session.actionQueue.pop();

    session.isActive = false;

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Recovered from auction failure for session {}", sessionId);
}

void AuctionHouse::HandleInsufficientFunds(uint32 requiredAmount)
{
    if (!_bot)
        return;

    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Bot {} has insufficient funds (needs: {}, has: {})",
        _bot->GetName(), requiredAmount, _bot->GetMoney());

    if (_profile.autoSellJunk)
    {
        TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Attempting to sell junk items for bot {}", _bot->GetName());
    }
}

void AuctionHouse::HandleAuctionTimeout(uint32 auctionId)
{
    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Auction {} timed out", auctionId);
}

// ============================================================================
// Update and maintenance
// ============================================================================

void AuctionHouse::Update(uint32 diff)
{
    if (!_bot || !_auctionHouseEnabled)
        return;

    static uint32 updateTimer = 0;
    updateTimer += diff;

    if (updateTimer >= AUCTION_UPDATE_INTERVAL)
    {
        updateTimer = 0;
        UpdateAuctionSessions();
    }

    static uint32 marketTimer = 0;
    marketTimer += diff;

    if (marketTimer >= MARKET_ANALYSIS_INTERVAL)
    {
        marketTimer = 0;
        UpdateMarketAnalysis();
    }
}

void AuctionHouse::UpdateAuctionSessions()
{
    std::vector<uint32> expiredSessions;

    for (auto& [sessionId, session] : _activeSessions)
    {
        if (!session.isActive)
        {
            expiredSessions.push_back(sessionId);
            continue;
        }

        uint32 currentTime = GameTime::GetGameTimeMS();
        if (currentTime - session.sessionStartTime > SESSION_TIMEOUT)
        {
            expiredSessions.push_back(sessionId);
            continue;
        }

        UpdateAuctionSession(sessionId);
    }

    for (uint32 sessionId : expiredSessions)
        CompleteAuctionSession(sessionId);
}

void AuctionHouse::UpdateMarketAnalysis()
{
    if (!_bot)
        return;

    UpdateMarketData();
    CleanupExpiredData();
}

void AuctionHouse::CleanupExpiredData()
{
    std::lock_guard<decltype(_marketMutex)> lock(_marketMutex);

    uint32 currentTime = GameTime::GetGameTimeMS();
    uint32 expirationTime = currentTime - MARKET_DATA_CACHE_DURATION;

    std::vector<uint32> expiredItems;

    for (auto& [itemId, marketData] : _marketData)
    {
        if (marketData.lastAnalysisTime < expirationTime)
            expiredItems.push_back(itemId);
    }

    for (uint32 itemId : expiredItems)
    {
        _marketData.erase(itemId);
        _auctionCache.erase(itemId);
    }

    std::vector<uint32> inactiveCompetitors;
    uint32 competitorExpiration = currentTime - (7 * 24 * 60 * 60 * 1000);

    for (const auto& [guid, competitor] : _competitors)
    {
        if (competitor.lastActivity < competitorExpiration)
            inactiveCompetitors.push_back(guid);
    }

    for (uint32 guid : inactiveCompetitors)
        _competitors.erase(guid);

    if (!expiredItems.empty() || !inactiveCompetitors.empty())
    {
        TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Cleaned up {} expired items and {} inactive competitors",
            expiredItems.size(), inactiveCompetitors.size());
    }
}

// ============================================================================
// Private strategy implementations
// ============================================================================

void AuctionHouse::ExecuteConservativeStrategy(AuctionSession& session)
{
    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Executing conservative strategy");

    std::vector<AuctionItem> bargains = FindBargainAuctions(0.85f);

    for (const auto& auction : bargains)
    {
        if (session.budgetUsed >= _profile.maxBiddingBudget)
            break;

        if (IsWorthBuying(auction) && _bot->GetMoney() >= auction.buyoutPrice)
        {
            if (BuyoutAuction(auction.auctionId))
            {
                session.itemsBought++;
                session.budgetUsed += auction.buyoutPrice;
            }
        }
    }
}

void AuctionHouse::ExecuteAggressiveStrategy(AuctionSession& session)
{
    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Executing aggressive strategy");

    std::vector<AuctionItem> bargains = FindBargainAuctions(0.95f);

    for (const auto& auction : bargains)
    {
        if (session.budgetUsed >= _profile.maxBiddingBudget)
            break;

        if (_bot->GetMoney() >= auction.buyoutPrice)
        {
            if (BuyoutAuction(auction.auctionId))
            {
                session.itemsBought++;
                session.budgetUsed += auction.buyoutPrice;
            }
        }
    }
}

void AuctionHouse::ExecuteOpportunisticStrategy(AuctionSession& session)
{
    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Executing opportunistic strategy");

    std::vector<AuctionItem> opportunities = FindFlipOpportunities();

    for (const auto& auction : opportunities)
    {
        if (session.budgetUsed >= _profile.maxBiddingBudget)
            break;

        uint32 resellPrice = CalculateOptimalListingPrice(auction.itemId, auction.stackCount);
        float profitMargin = CalculatePotentialProfit(auction, resellPrice);

        if (profitMargin >= _profile.profitMargin && _bot->GetMoney() >= auction.buyoutPrice)
        {
            if (BuyoutAuction(auction.auctionId))
            {
                session.itemsBought++;
                session.budgetUsed += auction.buyoutPrice;
            }
        }
    }
}

void AuctionHouse::ExecuteMarketMakerStrategy(AuctionSession& session)
{
    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Executing market maker strategy");

    ExecuteOpportunisticStrategy(session);
    ManageActiveAuctions();
}

void AuctionHouse::ExecuteCollectorStrategy(AuctionSession& session)
{
    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Executing collector strategy");

    for (uint32 itemId : _profile.watchList)
    {
        if (session.budgetUsed >= _profile.maxBiddingBudget)
            break;

        std::vector<AuctionItem> auctions = GetSimilarAuctions(itemId, 5);

        for (const auto& auction : auctions)
        {
            if (IsWorthBuying(auction) && _bot->GetMoney() >= auction.buyoutPrice)
            {
                if (BuyoutAuction(auction.auctionId))
                {
                    session.itemsBought++;
                    session.budgetUsed += auction.buyoutPrice;
                    break;
                }
            }
        }
    }
}

void AuctionHouse::ExecuteProfitFocusedStrategy(AuctionSession& session)
{
    TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Executing profit-focused strategy");

    ExecuteOpportunisticStrategy(session);
}

// ============================================================================
// Market analysis helpers
// ============================================================================

void AuctionHouse::UpdateItemMarketData(uint32 itemId)
{
    AuctionHouseObject* auctionHouse = GetAuctionHouseForPlayer();
    if (!auctionHouse)
        return;

    std::vector<uint64> prices;

    for (auto itr = auctionHouse->GetAuctionsBegin(); itr != auctionHouse->GetAuctionsEnd(); ++itr)
    {
        AuctionPosting* auction = &itr->second;
        if (!auction || auction->Items.empty())
            continue;

        Item* item = auction->Items.front();
        if (!item || item->GetEntry() != itemId)
            continue;

        uint64 pricePerItem = auction->GetTotalItemCount() > 0 ?
            auction->BuyoutOrUnitPrice / auction->GetTotalItemCount() : 0;

        if (pricePerItem > 0)
            prices.push_back(pricePerItem);
    }

    if (prices.empty())
        return;

    auto& marketData = _marketData[itemId];
    marketData.itemId = itemId;
    marketData.activeListings = prices.size();

    uint64 totalPrice = 0;
    for (uint64 price : prices)
        totalPrice += price;

    marketData.averagePrice = static_cast<float>(totalPrice) / prices.size();

    std::sort(prices.begin(), prices.end());
    marketData.medianPrice = prices[prices.size() / 2];
    marketData.lastAnalysisTime = GameTime::GetGameTimeMS();

    TrackPriceMovement(itemId, marketData.averagePrice, GameTime::GetGameTimeMS());
}

float AuctionHouse::CalculateMarketVolatility(const std::vector<std::pair<uint32, uint32>>& priceHistory)
{
    if (priceHistory.size() < 2)
        return 0.0f;

    uint64 totalPrice = 0;
    for (const auto& [price, timestamp] : priceHistory)
        totalPrice += price;

    float avgPrice = static_cast<float>(totalPrice) / priceHistory.size();

    if (avgPrice <= 0.0f)
        return 0.0f;

    float variance = 0.0f;
    for (const auto& [price, timestamp] : priceHistory)
    {
        float diff = price - avgPrice;
        variance += diff * diff;
    }

    variance /= priceHistory.size();
    float stdDev = std::sqrt(variance);

    return stdDev / avgPrice;
}

float AuctionHouse::PredictPriceMovement(uint32 itemId)
{
    std::lock_guard<decltype(_marketMutex)> lock(_marketMutex);

    auto it = _marketData.find(itemId);
    if (it == _marketData.end() || it->second.priceHistory.size() < 5)
        return 0.0f;

    const auto& history = it->second.priceHistory;
    size_t halfSize = history.size() / 2;

    uint64 oldTotal = 0;
    for (size_t i = 0; i < halfSize; ++i)
        oldTotal += history[i].first;

    uint64 newTotal = 0;
    for (size_t i = halfSize; i < history.size(); ++i)
        newTotal += history[i].first;

    float oldAvg = static_cast<float>(oldTotal) / halfSize;
    float newAvg = static_cast<float>(newTotal) / (history.size() - halfSize);

    if (oldAvg <= 0.0f)
        return 0.0f;

    return (newAvg - oldAvg) / oldAvg;
}

bool AuctionHouse::IsMarketTrendy(uint32 itemId, bool& isRising)
{
    float movement = PredictPriceMovement(itemId);

    if (std::abs(movement) < 0.05f)
        return false;

    isRising = movement > 0.0f;
    return true;
}

// ============================================================================
// Search and filtering
// ============================================================================

std::vector<AuctionItem> AuctionHouse::FilterAuctionResults(const std::vector<AuctionItem>& auctions,
                                                            const AuctionSearchQuery& query)
{
    std::vector<AuctionItem> filtered;

    for (const auto& auction : auctions)
    {
        if (MatchesSearchCriteria(auction, query))
            filtered.push_back(auction);
    }

    return filtered;
}

std::vector<AuctionItem> AuctionHouse::FindBargainAuctions(float maxPriceRatio)
{
    std::vector<AuctionItem> bargains;

    AuctionHouseObject* auctionHouse = GetAuctionHouseForPlayer();
    if (!auctionHouse)
        return bargains;

    std::lock_guard<decltype(_marketMutex)> lock(_marketMutex);

    for (auto itr = auctionHouse->GetAuctionsBegin(); itr != auctionHouse->GetAuctionsEnd(); ++itr)
    {
        AuctionPosting* auction = &itr->second;
        if (!auction || auction->Items.empty())
            continue;

        Item* item = auction->Items.front();
        if (!item)
            continue;

        uint32 itemId = item->GetEntry();
        float marketPrice = GetMarketPrice(itemId, auction->GetTotalItemCount());

        if (marketPrice <= 0.0f)
            continue;

        if (auction->BuyoutOrUnitPrice > 0 && auction->BuyoutOrUnitPrice <= marketPrice * maxPriceRatio)
        {
            AuctionItem auctionItem;
            auctionItem.auctionId = auction->Id;
            auctionItem.itemId = itemId;
            auctionItem.itemGuid = item->GetGUID().GetCounter();
            auctionItem.stackCount = auction->GetTotalItemCount();
            auctionItem.currentBid = auction->BidAmount;
            auctionItem.buyoutPrice = auction->BuyoutOrUnitPrice;
            auctionItem.sellerGuid = auction->Owner.GetCounter();
            auctionItem.quality = item->GetQuality();
            auctionItem.itemLevel = item->GetItemLevel(_bot);
            auctionItem.isBargain = true;
            auctionItem.marketValue = marketPrice;
            auctionItem.lastSeen = GameTime::GetGameTimeMS();

            bargains.push_back(auctionItem);
        }

        if (bargains.size() >= 50)
            break;
    }

    return bargains;
}

std::vector<AuctionItem> AuctionHouse::FindFlipOpportunities()
{
    std::vector<AuctionItem> opportunities;

    AuctionHouseObject* auctionHouse = GetAuctionHouseForPlayer();
    if (!auctionHouse)
        return opportunities;

    std::lock_guard<decltype(_marketMutex)> lock(_marketMutex);

    for (auto itr = auctionHouse->GetAuctionsBegin(); itr != auctionHouse->GetAuctionsEnd(); ++itr)
    {
        AuctionPosting* auction = &itr->second;
        if (!auction || auction->Items.empty())
            continue;

        Item* item = auction->Items.front();
        if (!item)
            continue;

        uint32 itemId = item->GetEntry();
        uint32 stackCount = auction->GetTotalItemCount();
        float marketPrice = GetMarketPrice(itemId, stackCount);

        if (marketPrice <= 0.0f || auction->BuyoutOrUnitPrice == 0)
            continue;

        // Create temporary AuctionItem for profit calculation
        AuctionItem tempAuction;
        tempAuction.auctionId = auction->Id;
        tempAuction.itemId = itemId;
        tempAuction.itemGuid = item->GetGUID().GetCounter();
        tempAuction.stackCount = stackCount;
        tempAuction.currentBid = auction->BidAmount;
        tempAuction.buyoutPrice = auction->BuyoutOrUnitPrice;

        uint32 resellPrice = CalculateOptimalListingPrice(itemId, stackCount);
        float profitMargin = CalculatePotentialProfit(tempAuction, resellPrice);

        if (profitMargin >= MIN_PROFIT_MARGIN)
        {
            AuctionItem auctionItem;
            auctionItem.auctionId = auction->Id;
            auctionItem.itemId = itemId;
            auctionItem.itemGuid = item->GetGUID().GetCounter();
            auctionItem.stackCount = stackCount;
            auctionItem.currentBid = auction->BidAmount;
            auctionItem.buyoutPrice = auction->BuyoutOrUnitPrice;
            auctionItem.sellerGuid = auction->Owner.GetCounter();
            auctionItem.quality = item->GetQuality();
            auctionItem.itemLevel = item->GetItemLevel(_bot);
            auctionItem.marketValue = marketPrice;
            auctionItem.lastSeen = GameTime::GetGameTimeMS();

            opportunities.push_back(auctionItem);
        }

        if (opportunities.size() >= 30)
            break;
    }

    return opportunities;
}

bool AuctionHouse::MatchesSearchCriteria(const AuctionItem& auction, const AuctionSearchQuery& query)
{
    if (query.itemId != 0 && auction.itemId != query.itemId)
        return false;

    if (query.maxPrice > 0 && auction.buyoutPrice > query.maxPrice)
        return false;

    if (query.minQuality > 0 && auction.quality < query.minQuality)
        return false;

    if (query.maxQuality < 6 && auction.quality > query.maxQuality)
        return false;

    if (query.minItemLevel > 0 && auction.itemLevel < query.minItemLevel)
        return false;

    return true;
}

// ============================================================================
// Price calculation algorithms
// ============================================================================

uint32 AuctionHouse::CalculateReasonableBid(const AuctionItem& auction, float aggressiveness)
{
    if (auction.buyoutPrice > 0)
    {
        float bidRatio = 0.7f + (aggressiveness * 0.2f);
        return static_cast<uint32>(auction.buyoutPrice * bidRatio);
    }

    if (auction.currentBid > 0)
    {
        uint32 minIncrement = auction.currentBid / 20;
        return auction.currentBid + minIncrement;
    }

    float marketPrice = GetMarketPrice(auction.itemId, auction.stackCount);
    if (marketPrice > 0.0f)
        return static_cast<uint32>(marketPrice * 0.8f);

    return 0;
}

uint32 AuctionHouse::CalculateCompetitivePrice(uint32 itemId, uint32 stackSize)
{
    std::vector<AuctionItem> similarAuctions = GetSimilarAuctions(itemId, 10);

    if (similarAuctions.empty())
    {
        float marketPrice = GetMarketPrice(itemId, stackSize);
        return marketPrice > 0.0f ? static_cast<uint32>(marketPrice) : 0;
    }

    std::vector<uint64> prices;
    for (const auto& auction : similarAuctions)
    {
        if (auction.buyoutPrice > 0)
            prices.push_back(auction.buyoutPrice);
    }

    if (prices.empty())
        return 0;

    std::sort(prices.begin(), prices.end());
    uint64 lowestPrice = prices.front();

    return CalculateUndercutPrice(itemId, lowestPrice, DEFAULT_UNDERCUT_RATE);
}

uint32 AuctionHouse::CalculateUndercutPrice(uint32 itemId, uint32 currentLowest, float undercutRate)
{
    if (currentLowest == 0)
        return 0;

    uint32 undercutPrice = static_cast<uint32>(currentLowest * (1.0f - undercutRate));

    float marketPrice = GetMarketPrice(itemId, 1);
    if (marketPrice > 0.0f)
    {
        uint32 minAcceptable = static_cast<uint32>(marketPrice * 0.5f);
        if (undercutPrice < minAcceptable)
            undercutPrice = minAcceptable;
    }

    return std::max(1u, undercutPrice);
}

float AuctionHouse::CalculateExpectedReturn(const AuctionItem& auction, uint32 resellPrice)
{
    if (auction.buyoutPrice == 0 || resellPrice <= auction.buyoutPrice)
        return 0.0f;

    uint32 profit = resellPrice - auction.buyoutPrice;
    uint32 auctionCut = static_cast<uint32>(resellPrice * 0.05f);
    uint32 deposit = auction.buyoutPrice / 100;

    int32 netProfit = profit - auctionCut - deposit;

    return static_cast<float>(netProfit) / auction.buyoutPrice;
}

// ============================================================================
// Auction execution helpers
// ============================================================================

bool AuctionHouse::ExecuteBuyAction(const AuctionItem& auction)
{
    return BuyoutAuction(auction.auctionId);
}

bool AuctionHouse::ExecuteSellAction(uint32 itemGuid, uint32 stackCount, uint32 price)
{
    uint32 bid = static_cast<uint32>(price * 0.8f);
    return CreateAuction(itemGuid, stackCount, bid, price, 24 * 60);
}

bool AuctionHouse::ExecuteCancelAction(uint32 auctionId)
{
    return CancelAuction(auctionId);
}

void AuctionHouse::ProcessActionQueue(AuctionSession& session)
{
    while (!session.actionQueue.empty())
    {
        auto [actionType, targetId, amount] = session.actionQueue.front();
        session.actionQueue.pop();

        switch (actionType)
        {
            case AuctionActionType::BUY_ITEM:
            {
                AuctionHouseObject* auctionHouse = GetAuctionHouseForPlayer();
                if (auctionHouse)
                {
                    AuctionPosting* auction = auctionHouse->GetAuction(targetId);
                    if (auction && auction->Items.front())
                    {
                        AuctionItem item;
                        item.auctionId = targetId;
                        item.buyoutPrice = auction->BuyoutOrUnitPrice;
                        ExecuteBuyAction(item);
                    }
                }
                break;
            }
            case AuctionActionType::CANCEL_AUCTION:
                ExecuteCancelAction(targetId);
                break;
            default:
                break;
        }
    }
}

// ============================================================================
// Learning and adaptation
// ============================================================================

void AuctionHouse::UpdateSuccessRates(bool wasSuccessful, AuctionActionType actionType)
{
    if (wasSuccessful)
    {
        float currentMargin = _metrics.averageProfitMargin.load();
        float newMargin = (currentMargin * 0.9f) + (0.1f * 0.1f);
        _metrics.averageProfitMargin = newMargin;
    }
}

void AuctionHouse::AdaptPricingStrategy()
{
    float roi = _metrics.GetROI();

    if (roi < 1.0f && _profile.primaryStrategy == AuctionHouseStrategy::AGGRESSIVE)
    {
        _profile.primaryStrategy = AuctionHouseStrategy::CONSERVATIVE;
        TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Adapted to conservative strategy due to poor ROI");
    }
    else if (roi > 1.3f && _profile.primaryStrategy == AuctionHouseStrategy::CONSERVATIVE)
    {
        _profile.primaryStrategy = AuctionHouseStrategy::AGGRESSIVE;
        TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Adapted to aggressive strategy due to good ROI");
    }
}

void AuctionHouse::LearnMarketPatterns(uint32 itemId)
{
    AnalyzeMarketTrends(itemId);

    bool isRising = false;
    if (IsMarketTrendy(itemId, isRising))
    {
        TC_LOG_DEBUG("playerbot.auction", "AuctionHouse: Market for item {} is {}",
            itemId, isRising ? "rising" : "falling");
    }
}

void AuctionHouse::UpdatePlayerPreferences(uint32 itemId, bool wasPurchased)
{
    if (wasPurchased && _profile.watchList.size() < 20)
    {
        // Only add if not already in the list
        auto it = std::find(_profile.watchList.begin(), _profile.watchList.end(), itemId);
        if (it == _profile.watchList.end())
            _profile.watchList.push_back(itemId);
    }
}

// ============================================================================
// Performance optimization
// ============================================================================

void AuctionHouse::CacheFrequentAuctions()
{
    std::lock_guard<decltype(_marketMutex)> lock(_marketMutex);

    for (uint32 itemId : _profile.watchList)
    {
        std::vector<AuctionItem> auctions = GetSimilarAuctions(itemId, 10);
        _auctionCache[itemId] = auctions;
    }
}

void AuctionHouse::PreloadMarketData(const std::vector<uint32>& itemIds)
{
    std::lock_guard<decltype(_marketMutex)> lock(_marketMutex);

    for (uint32 itemId : itemIds)
        UpdateItemMarketData(itemId);
}

void AuctionHouse::OptimizeAuctionQueries()
{
    CacheFrequentAuctions();
}

void AuctionHouse::UpdateAuctionMetrics(uint32 playerGuid, AuctionActionType actionType,
                                       uint32 goldAmount, bool wasSuccessful)
{
    if (wasSuccessful)
    {
        switch (actionType)
        {
            case AuctionActionType::BUY_ITEM:
                _globalMetrics.totalGoldSpent += goldAmount;
                break;
            case AuctionActionType::SELL_ITEM:
                _globalMetrics.totalGoldEarned += goldAmount;
                break;
            default:
                break;
        }
    }
}

} // namespace Playerbot
