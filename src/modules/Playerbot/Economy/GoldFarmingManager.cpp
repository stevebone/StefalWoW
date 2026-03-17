/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "GoldFarmingManager.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "GameTime.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot
{

GoldFarmingManager::GoldFarmingManager(Player* bot, BotAI* ai)
    : BehaviorManager(bot, ai, 5000, "GoldFarmingManager")  // 5 second update
{
    // Enable all methods by default except auction flipping (needs market knowledge)
    for (uint8 i = 0; i < static_cast<uint8>(GoldFarmingMethod::MAX_METHOD); ++i)
    {
        if (static_cast<GoldFarmingMethod>(i) != GoldFarmingMethod::AUCTION_FLIPPING)
        {
            _enabledMethods.insert(static_cast<GoldFarmingMethod>(i));
        }
    }
}

bool GoldFarmingManager::OnInitialize()
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return false;

    _lastKnownGold = GetCurrentGold();
    _lastGoldCheck = std::chrono::steady_clock::now();

    return true;
}

void GoldFarmingManager::OnShutdown()
{
    if (_currentSession.isActive)
    {
        StopSession("Shutdown");
    }

    _transactions.clear();
    _incomeByMethod.clear();
}

void GoldFarmingManager::OnUpdate(uint32 /*elapsed*/)
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return;

    // Check for gold changes periodically
    auto now = std::chrono::steady_clock::now();
    auto timeSinceCheck = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastGoldCheck).count();
    if (timeSinceCheck >= GOLD_CHECK_INTERVAL_MS)
    {
        UpdateGoldTracking();
        _lastGoldCheck = now;
    }

    // Update session progress
    if (_currentSession.isActive)
    {
        UpdateSessionProgress();
    }
}

void GoldFarmingManager::UpdateGoldTracking()
{
    uint64 currentGold = GetCurrentGold();

    if (currentGold != _lastKnownGold)
    {
        int64 change = static_cast<int64>(currentGold) - static_cast<int64>(_lastKnownGold);

        if (change > 0)
        {
            // Gold increased - likely from loot or other untracked source
            // Only record if we're in a session and this wasn't already tracked
            if (_currentSession.isActive)
            {
                // This might be double-counting if we already recorded the transaction
                // In practice, we'd need to coordinate with other systems
            }
        }

        _lastKnownGold = currentGold;
    }
}

void GoldFarmingManager::UpdateSessionProgress()
{
    if (!_currentSession.isActive)
        return;

    // Update current gold in goal
    _currentSession.goal.currentGold = GetCurrentGold();

    // Check if goal is complete
    if (_currentSession.goal.targetGold > 0 && _currentSession.goal.IsComplete())
    {
        TC_LOG_DEBUG("module.playerbot.economy",
            "GoldFarmingManager: Bot {} reached gold target of {}",
            GetBot() ? GetBot()->GetName() : "unknown",
            _currentSession.goal.targetGold);

        StopSession("Goal achieved");
    }

    // Update best GPH if applicable
    uint32 currentGph = _currentSession.GetGoldPerHour();
    if (currentGph > _statistics.bestGphAchieved.load())
    {
        _statistics.bestGphAchieved.store(currentGph);
    }
}

uint64 GoldFarmingManager::GetCurrentGold() const
{
    if (!GetBot())
        return 0;

    return GetBot()->GetMoney();
}

void GoldFarmingManager::RecordTransaction(int64 amount, GoldFarmingMethod source,
    uint32 itemId, std::string const& description)
{
    GoldTransaction tx;
    tx.amount = amount;
    tx.source = source;
    tx.itemId = itemId;
    tx.timestamp = GameTime::GetGameTimeMS();
    tx.description = description;

    _transactions.push_back(tx);
    TrimTransactionHistory();

    // Update income by method
    if (amount > 0)
    {
        _incomeByMethod[source] += amount;
        _statistics.totalIncome += amount;

        if (_currentSession.isActive)
        {
            _currentSession.goldGained += amount;
        }
    }
    else
    {
        _statistics.totalExpenses += std::abs(amount);

        if (_currentSession.isActive)
        {
            _currentSession.goldSpent += std::abs(amount);
        }
    }

    NotifyCallback(amount, source);

    TC_LOG_DEBUG("module.playerbot.economy",
        "GoldFarmingManager: Bot {} {} {} copper via {}",
        GetBot() ? GetBot()->GetName() : "unknown",
        amount > 0 ? "gained" : "spent",
        std::abs(amount),
        static_cast<uint32>(source));
}

void GoldFarmingManager::RecordLootGold(uint64 amount)
{
    RecordTransaction(static_cast<int64>(amount), GoldFarmingMethod::MOB_FARMING, 0, "Loot");
    _currentSession.itemsLooted++;
    _statistics.itemsLooted++;
}

void GoldFarmingManager::RecordQuestGold(uint64 amount, uint32 questId)
{
    RecordTransaction(static_cast<int64>(amount), GoldFarmingMethod::QUESTING, questId, "Quest reward");
}

void GoldFarmingManager::RecordVendorSale(uint64 amount, uint32 itemId, uint32 count)
{
    std::string desc = "Sold " + std::to_string(count) + "x item";
    RecordTransaction(static_cast<int64>(amount), GoldFarmingMethod::MOB_FARMING, itemId, desc);
    _currentSession.itemsSold += count;
    _statistics.itemsSold += count;
}

void GoldFarmingManager::RecordAuctionSale(uint64 amount, uint32 itemId)
{
    RecordTransaction(static_cast<int64>(amount), GoldFarmingMethod::AUCTION_FLIPPING, itemId, "AH sale");
    _statistics.auctionsSold++;
}

void GoldFarmingManager::RecordExpense(uint64 amount, GoldFarmingMethod source, std::string const& description)
{
    RecordTransaction(-static_cast<int64>(amount), source, 0, description);
}

std::vector<GoldTransaction> GoldFarmingManager::GetRecentTransactions(uint32 maxCount) const
{
    if (_transactions.size() <= maxCount)
        return _transactions;

    return std::vector<GoldTransaction>(
        _transactions.end() - maxCount,
        _transactions.end());
}

std::vector<GoldFarmingMethod> GoldFarmingManager::GetSuggestedMethods(uint32 maxCount) const
{
    std::vector<std::pair<GoldFarmingMethod, uint32>> methodsWithGph;

    for (uint8 i = 1; i < static_cast<uint8>(GoldFarmingMethod::MAX_METHOD); ++i)
    {
        GoldFarmingMethod method = static_cast<GoldFarmingMethod>(i);
        if (!IsMethodEnabled(method))
            continue;

        uint32 gph = GetEstimatedGph(method);
        methodsWithGph.push_back({method, gph});
    }

    // Sort by GPH descending
    std::sort(methodsWithGph.begin(), methodsWithGph.end(),
        [](auto const& a, auto const& b) { return a.second > b.second; });

    std::vector<GoldFarmingMethod> result;
    for (auto const& [method, gph] : methodsWithGph)
    {
        result.push_back(method);
        if (result.size() >= maxCount)
            break;
    }

    return result;
}

std::vector<FarmingSpot> GoldFarmingManager::GetSuggestedSpots(
    GoldFarmingMethod method, uint32 maxCount) const
{
    std::vector<FarmingSpot> spots;

    // Would query a database of known farming spots
    // For now, return empty - real implementation would have predefined spots

    if (spots.size() > maxCount)
    {
        spots.resize(maxCount);
    }

    return spots;
}

uint32 GoldFarmingManager::GetEstimatedGph(GoldFarmingMethod method) const
{
    // Base estimates (copper per hour) - would be refined based on actual performance
    switch (method)
    {
        case GoldFarmingMethod::QUESTING:
            return 500000;  // 50g/hour
        case GoldFarmingMethod::MOB_FARMING:
            return 300000;  // 30g/hour
        case GoldFarmingMethod::GATHERING:
            return 800000;  // 80g/hour
        case GoldFarmingMethod::CRAFTING:
            return 600000;  // 60g/hour
        case GoldFarmingMethod::AUCTION_FLIPPING:
            return 1000000; // 100g/hour (high risk)
        case GoldFarmingMethod::DUNGEON_RUNS:
            return 700000;  // 70g/hour
        case GoldFarmingMethod::OLD_RAIDS:
            return 1500000; // 150g/hour (weekly limited)
        case GoldFarmingMethod::FISHING:
            return 200000;  // 20g/hour
        case GoldFarmingMethod::TREASURE_HUNTING:
            return 400000;  // 40g/hour
        default:
            return 0;
    }
}

std::unordered_map<GoldFarmingMethod, int64> GoldFarmingManager::GetIncomeBreakdown() const
{
    return _incomeByMethod;
}

bool GoldFarmingManager::StartSession(uint64 targetGold, GoldFarmingMethod method)
{
    if (_currentSession.isActive)
    {
        TC_LOG_DEBUG("module.playerbot.economy",
            "GoldFarmingManager: Session already active for bot {}",
            GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    _currentSession.Reset();
    _currentSession.isActive = true;
    _currentSession.startTime = std::chrono::steady_clock::now();
    _currentSession.startGold = GetCurrentGold();

    // Set up goal
    _currentSession.goal.targetGold = targetGold;
    _currentSession.goal.currentGold = _currentSession.startGold;
    _currentSession.goal.startGold = _currentSession.startGold;

    // Select method
    if (method != GoldFarmingMethod::NONE)
    {
        _currentSession.activeMethod = method;
        _currentSession.goal.preferredMethod = method;
    }
    else
    {
        _currentSession.activeMethod = AutoSelectMethod();
        _currentSession.goal.preferredMethod = _currentSession.activeMethod;
    }

    TC_LOG_DEBUG("module.playerbot.economy",
        "GoldFarmingManager: Started session for bot {}, target: {}, method: {}",
        GetBot() ? GetBot()->GetName() : "unknown",
        targetGold,
        static_cast<uint32>(_currentSession.activeMethod));

    return true;
}

void GoldFarmingManager::StopSession(std::string const& reason)
{
    if (!_currentSession.isActive)
        return;

    _statistics.totalFarmingTimeMs += _currentSession.GetElapsedMs();

    TC_LOG_DEBUG("module.playerbot.economy",
        "GoldFarmingManager: Stopped session for bot {}, reason: {}, net: {} copper, GPH: {}",
        GetBot() ? GetBot()->GetName() : "unknown",
        reason.empty() ? "none" : reason.c_str(),
        _currentSession.GetNetGold(),
        _currentSession.GetGoldPerHour());

    _currentSession.Reset();
}

bool GoldFarmingManager::ChangeMethod(GoldFarmingMethod method)
{
    if (!_currentSession.isActive)
        return false;

    if (!IsMethodEnabled(method))
        return false;

    _currentSession.activeMethod = method;
    _currentSession.goal.preferredMethod = method;

    TC_LOG_DEBUG("module.playerbot.economy",
        "GoldFarmingManager: Bot {} changed method to {}",
        GetBot() ? GetBot()->GetName() : "unknown",
        static_cast<uint32>(method));

    return true;
}

bool GoldFarmingManager::GoToFarmingSpot(FarmingSpot const& spot)
{
    if (!GetBot() || !GetAI())
        return false;

    if (!spot.IsValid())
        return false;

    _currentSession.activeSpot = spot;

    // Move to the spot
    GetAI()->MoveTo(
        spot.position.GetPositionX(),
        spot.position.GetPositionY(),
        spot.position.GetPositionZ());

    return true;
}

void GoldFarmingManager::SetMethodEnabled(GoldFarmingMethod method, bool enabled)
{
    if (enabled)
    {
        _enabledMethods.insert(method);
    }
    else
    {
        _enabledMethods.erase(method);
    }
}

bool GoldFarmingManager::IsMethodEnabled(GoldFarmingMethod method) const
{
    return _enabledMethods.count(method) > 0;
}

GoldFarmingMethod GoldFarmingManager::AutoSelectMethod() const
{
    // Prefer the method with highest estimated GPH among enabled methods
    auto suggested = GetSuggestedMethods(1);
    if (!suggested.empty())
        return suggested[0];

    // Fallback to preferred method
    if (_preferredMethod != GoldFarmingMethod::NONE && IsMethodEnabled(_preferredMethod))
        return _preferredMethod;

    // Fallback to questing
    if (IsMethodEnabled(GoldFarmingMethod::QUESTING))
        return GoldFarmingMethod::QUESTING;

    // Last resort
    return GoldFarmingMethod::MOB_FARMING;
}

void GoldFarmingManager::TrimTransactionHistory()
{
    if (_transactions.size() > MAX_TRANSACTION_HISTORY)
    {
        _transactions.erase(
            _transactions.begin(),
            _transactions.begin() + (_transactions.size() - MAX_TRANSACTION_HISTORY));
    }
}

void GoldFarmingManager::NotifyCallback(int64 amount, GoldFarmingMethod source)
{
    if (_callback)
    {
        _callback(amount, source);
    }
}

} // namespace Playerbot
