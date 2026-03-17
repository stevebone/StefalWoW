/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ReputationGrindManager.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "ReputationMgr.h"
#include "DB2Stores.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot
{

ReputationGrindManager::ReputationGrindManager(Player* bot, BotAI* ai)
    : BehaviorManager(bot, ai, 5000, "ReputationGrindManager")  // 5 second update
{
}

bool ReputationGrindManager::OnInitialize()
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return false;

    RefreshFactionData();
    _lastRefresh = std::chrono::steady_clock::now();

    return true;
}

void ReputationGrindManager::OnShutdown()
{
    if (_currentSession.isActive)
    {
        StopSession("Shutdown");
    }

    _factionCache.clear();
}

void ReputationGrindManager::OnUpdate(uint32 /*elapsed*/)
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return;

    // Refresh faction data periodically
    auto now = std::chrono::steady_clock::now();
    auto timeSinceRefresh = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastRefresh).count();
    if (timeSinceRefresh >= REFRESH_INTERVAL_MS)
    {
        RefreshFactionData();
        _lastRefresh = now;
    }

    // Update session progress
    if (_currentSession.isActive)
    {
        UpdateSessionProgress();
        CheckStandingChanges();
    }
}

void ReputationGrindManager::RefreshFactionData()
{
    if (!GetBot())
        return;

    // Clear and rebuild cache from player's reputation data
    _factionCache.clear();

    // TrinityCore stores reputation via ReputationMgr
    // Iterate through known factions and cache their data

    // Note: Implementation would use Player's ReputationMgr to get actual data
    // For each faction the player has interacted with:
    // - Get current standing
    // - Get reputation value
    // - Check at-war status
    // - Count available rewards

    TC_LOG_DEBUG("module.playerbot.reputation",
        "ReputationGrindManager: Refreshed faction data for bot {}",
        GetBot()->GetName());
}

void ReputationGrindManager::UpdateSessionProgress()
{
    if (!GetBot() || !_currentSession.isActive)
        return;

    // Get current reputation for target faction
    int32 currentRep = GetReputation(_currentSession.activeGoal.factionId);
    int32 repChange = currentRep - _currentSession.startRep;

    if (repChange > _currentSession.repGained)
    {
        int32 newGain = repChange - _currentSession.repGained;
        _currentSession.repGained = repChange;
        _statistics.totalRepGained += newGain;
    }

    // Update goal progress
    _currentSession.activeGoal.currentRep = currentRep;
    _currentSession.activeGoal.currentStanding = GetStanding(_currentSession.activeGoal.factionId);

    // Check if goal completed
    if (_currentSession.activeGoal.IsComplete())
    {
        TC_LOG_DEBUG("module.playerbot.reputation",
            "ReputationGrindManager: Bot {} reached {} with faction {}",
            GetBot()->GetName(),
            static_cast<uint32>(_currentSession.activeGoal.targetStanding),
            _currentSession.activeGoal.factionId);

        if (_currentSession.activeGoal.currentStanding == ReputationStanding::EXALTED)
        {
            _statistics.factionsExalted++;
        }

        StopSession("Goal achieved");
    }
}

void ReputationGrindManager::CheckStandingChanges()
{
    if (!GetBot())
        return;

    for (auto& [factionId, info] : _factionCache)
    {
        ReputationStanding currentStanding = GetStanding(factionId);

        if (currentStanding != info.standing)
        {
            ReputationStanding oldStanding = info.standing;
            info.standing = currentStanding;

            NotifyCallback(factionId, currentStanding);

            TC_LOG_DEBUG("module.playerbot.reputation",
                "ReputationGrindManager: Bot {} standing changed with faction {}: {} -> {}",
                GetBot()->GetName(), factionId,
                static_cast<uint32>(oldStanding), static_cast<uint32>(currentStanding));
        }
    }
}

ReputationStanding ReputationGrindManager::GetStanding(uint32 factionId) const
{
    int32 rep = GetReputation(factionId);
    return GetStandingFromRep(rep);
}

int32 ReputationGrindManager::GetReputation(uint32 factionId) const
{
    if (!GetBot())
        return 0;

    // TrinityCore uses ReputationMgr for reputation tracking
    // Would call: GetBot()->GetReputationMgr().GetReputation(factionId)
    auto it = _factionCache.find(factionId);
    if (it != _factionCache.end())
    {
        return it->second.currentRep;
    }

    return 0;
}

bool ReputationGrindManager::IsExalted(uint32 factionId) const
{
    return GetStanding(factionId) == ReputationStanding::EXALTED;
}

std::vector<FactionInfo> ReputationGrindManager::GetAllFactions() const
{
    std::vector<FactionInfo> result;
    result.reserve(_factionCache.size());

    for (auto const& [id, info] : _factionCache)
    {
        result.push_back(info);
    }

    return result;
}

std::vector<FactionInfo> ReputationGrindManager::GetFactionsWithRewards() const
{
    std::vector<FactionInfo> result;

    for (auto const& [id, info] : _factionCache)
    {
        if (info.rewards > 0 && info.standing < ReputationStanding::EXALTED)
        {
            result.push_back(info);
        }
    }

    // Sort by number of rewards (descending)
    std::sort(result.begin(), result.end(),
        [](FactionInfo const& a, FactionInfo const& b) {
            return a.rewards > b.rewards;
        });

    return result;
}

std::vector<FactionInfo> ReputationGrindManager::GetNearestStandingFactions(uint32 maxCount) const
{
    std::vector<FactionInfo> result;

    for (auto const& [id, info] : _factionCache)
    {
        if (info.standing < ReputationStanding::EXALTED)
        {
            result.push_back(info);
        }
    }

    // Sort by progress to next standing (descending)
    std::sort(result.begin(), result.end(),
        [](FactionInfo const& a, FactionInfo const& b) {
            return a.GetStandingProgress() > b.GetStandingProgress();
        });

    if (result.size() > maxCount)
    {
        result.resize(maxCount);
    }

    return result;
}

std::vector<ReputationGoal> ReputationGrindManager::GetSuggestedFactions(uint32 maxCount) const
{
    std::vector<ReputationGoal> suggestions;

    // Priority factions first
    for (uint32 factionId : _priorityFactions)
    {
        auto it = _factionCache.find(factionId);
        if (it != _factionCache.end() && it->second.standing < ReputationStanding::EXALTED)
        {
            ReputationGoal goal;
            goal.factionId = factionId;
            goal.factionName = it->second.name;
            goal.currentStanding = it->second.standing;
            goal.currentRep = it->second.currentRep;
            goal.targetStanding = _defaultTarget;
            goal.preferredMethod = GetBestGrindMethod(factionId);
            suggestions.push_back(goal);
        }
    }

    // Then factions with rewards
    auto factionsWithRewards = GetFactionsWithRewards();
    for (auto const& info : factionsWithRewards)
    {
        // Skip if already added
        bool alreadyAdded = false;
        for (auto const& existing : suggestions)
        {
            if (existing.factionId == info.factionId)
            {
                alreadyAdded = true;
                break;
            }
        }

        if (!alreadyAdded && suggestions.size() < maxCount)
        {
            ReputationGoal goal;
            goal.factionId = info.factionId;
            goal.factionName = info.name;
            goal.currentStanding = info.standing;
            goal.currentRep = info.currentRep;
            goal.targetStanding = _defaultTarget;
            goal.preferredMethod = GetBestGrindMethod(info.factionId);
            suggestions.push_back(goal);
        }
    }

    // Fill with nearest-to-standing factions
    auto nearestFactions = GetNearestStandingFactions(maxCount);
    for (auto const& info : nearestFactions)
    {
        bool alreadyAdded = false;
        for (auto const& existing : suggestions)
        {
            if (existing.factionId == info.factionId)
            {
                alreadyAdded = true;
                break;
            }
        }

        if (!alreadyAdded && suggestions.size() < maxCount)
        {
            ReputationGoal goal;
            goal.factionId = info.factionId;
            goal.factionName = info.name;
            goal.currentStanding = info.standing;
            goal.currentRep = info.currentRep;
            goal.targetStanding = _defaultTarget;
            goal.preferredMethod = GetBestGrindMethod(info.factionId);
            suggestions.push_back(goal);
        }
    }

    return suggestions;
}

FactionInfo ReputationGrindManager::GetFactionInfo(uint32 factionId) const
{
    auto it = _factionCache.find(factionId);
    if (it != _factionCache.end())
    {
        return it->second;
    }

    return FactionInfo();
}

bool ReputationGrindManager::StartSession(uint32 factionId, ReputationStanding targetStanding)
{
    if (_currentSession.isActive)
    {
        TC_LOG_DEBUG("module.playerbot.reputation",
            "ReputationGrindManager: Session already active for bot {}",
            GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    auto it = _factionCache.find(factionId);
    if (it == _factionCache.end())
    {
        TC_LOG_DEBUG("module.playerbot.reputation",
            "ReputationGrindManager: Faction {} not found for bot {}",
            factionId, GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    FactionInfo const& info = it->second;

    if (info.standing >= targetStanding)
    {
        TC_LOG_DEBUG("module.playerbot.reputation",
            "ReputationGrindManager: Already at or above target standing for faction {}",
            factionId);
        return false;
    }

    _currentSession.Reset();
    _currentSession.isActive = true;
    _currentSession.startTime = std::chrono::steady_clock::now();
    _currentSession.startRep = info.currentRep;

    _currentSession.activeGoal.factionId = factionId;
    _currentSession.activeGoal.factionName = info.name;
    _currentSession.activeGoal.currentStanding = info.standing;
    _currentSession.activeGoal.currentRep = info.currentRep;
    _currentSession.activeGoal.targetStanding = targetStanding;
    _currentSession.activeGoal.preferredMethod = GetBestGrindMethod(factionId);
    _currentSession.activeGoal.isActive = true;

    // Calculate target rep based on standing
    int32 targetMin, targetMax;
    GetStandingThresholds(targetStanding, targetMin, targetMax);
    _currentSession.activeGoal.targetRep = targetMin;

    TC_LOG_DEBUG("module.playerbot.reputation",
        "ReputationGrindManager: Started session for bot {}, faction {}, target {}",
        GetBot() ? GetBot()->GetName() : "unknown",
        info.name, static_cast<uint32>(targetStanding));

    return true;
}

void ReputationGrindManager::StopSession(std::string const& reason)
{
    if (!_currentSession.isActive)
        return;

    _statistics.totalGrindTimeMs += _currentSession.GetElapsedMs();
    _statistics.questsCompleted += _currentSession.questsCompleted;
    _statistics.mobsKilled += _currentSession.mobsKilled;
    _statistics.itemsTurnedIn += _currentSession.itemsTurnedIn;

    TC_LOG_DEBUG("module.playerbot.reputation",
        "ReputationGrindManager: Stopped session for bot {}, reason: {}, rep gained: {}",
        GetBot() ? GetBot()->GetName() : "unknown",
        reason.empty() ? "none" : reason.c_str(),
        _currentSession.repGained);

    _currentSession.Reset();
}

bool ReputationGrindManager::ChangeTargetFaction(uint32 factionId)
{
    if (!_currentSession.isActive)
        return false;

    StopSession("Changing target");
    return StartSession(factionId, _defaultTarget);
}

ReputationGrindMethod ReputationGrindManager::GetBestGrindMethod(uint32 factionId) const
{
    // Determine best method based on available options
    // Would check:
    // - Available quests for faction
    // - Mob kills that give rep
    // - Item turn-ins
    // - Dungeon options

    // Default to preferred method or quests
    if (_preferredMethod != ReputationGrindMethod::NONE)
        return _preferredMethod;

    // Check if there are available quests
    auto quests = GetReputationQuests(factionId);
    if (!quests.empty())
        return ReputationGrindMethod::QUESTS;

    // Check for mob kills
    auto mobs = GetReputationMobs(factionId);
    if (!mobs.empty())
        return ReputationGrindMethod::MOB_KILLS;

    // Check for turn-ins
    auto items = GetTurnInItems(factionId);
    if (!items.empty())
        return ReputationGrindMethod::ITEM_TURNINS;

    return ReputationGrindMethod::QUESTS;
}

std::vector<uint32> ReputationGrindManager::GetReputationQuests(uint32 /*factionId*/) const
{
    std::vector<uint32> quests;
    // Would query quest_template for quests that give rep for this faction
    return quests;
}

std::vector<uint32> ReputationGrindManager::GetReputationMobs(uint32 /*factionId*/) const
{
    std::vector<uint32> mobs;
    // Would query creature_onkill_reputation for mobs that give rep
    return mobs;
}

std::vector<uint32> ReputationGrindManager::GetTurnInItems(uint32 /*factionId*/) const
{
    std::vector<uint32> items;
    // Would query npc_vendor or quest templates for turn-in items
    return items;
}

void ReputationGrindManager::RecordRepGain(uint32 factionId, int32 amount, ReputationGrindMethod source)
{
    if (!_currentSession.isActive || _currentSession.activeGoal.factionId != factionId)
        return;

    _currentSession.repGained += amount;
    _statistics.totalRepGained += amount;

    switch (source)
    {
        case ReputationGrindMethod::QUESTS:
            _currentSession.questsCompleted++;
            break;
        case ReputationGrindMethod::MOB_KILLS:
            _currentSession.mobsKilled++;
            break;
        case ReputationGrindMethod::ITEM_TURNINS:
            _currentSession.itemsTurnedIn++;
            break;
        default:
            break;
    }
}

void ReputationGrindManager::AddPriorityFaction(uint32 factionId)
{
    _priorityFactions.insert(factionId);
}

void ReputationGrindManager::RemovePriorityFaction(uint32 factionId)
{
    _priorityFactions.erase(factionId);
}

ReputationStanding ReputationGrindManager::GetStandingFromRep(int32 rep) const
{
    if (rep >= STANDING_EXALTED_MIN)
        return ReputationStanding::EXALTED;
    if (rep >= STANDING_REVERED_MIN)
        return ReputationStanding::REVERED;
    if (rep >= STANDING_HONORED_MIN)
        return ReputationStanding::HONORED;
    if (rep >= STANDING_FRIENDLY_MIN)
        return ReputationStanding::FRIENDLY;
    if (rep >= STANDING_NEUTRAL_MIN)
        return ReputationStanding::NEUTRAL;
    if (rep >= STANDING_UNFRIENDLY_MIN)
        return ReputationStanding::UNFRIENDLY;
    if (rep >= STANDING_HOSTILE_MIN)
        return ReputationStanding::HOSTILE;

    return ReputationStanding::HATED;
}

void ReputationGrindManager::GetStandingThresholds(ReputationStanding standing, int32& min, int32& max) const
{
    switch (standing)
    {
        case ReputationStanding::HATED:
            min = STANDING_HATED_MIN;
            max = STANDING_HOSTILE_MIN - 1;
            break;
        case ReputationStanding::HOSTILE:
            min = STANDING_HOSTILE_MIN;
            max = STANDING_UNFRIENDLY_MIN - 1;
            break;
        case ReputationStanding::UNFRIENDLY:
            min = STANDING_UNFRIENDLY_MIN;
            max = STANDING_NEUTRAL_MIN - 1;
            break;
        case ReputationStanding::NEUTRAL:
            min = STANDING_NEUTRAL_MIN;
            max = STANDING_FRIENDLY_MIN - 1;
            break;
        case ReputationStanding::FRIENDLY:
            min = STANDING_FRIENDLY_MIN;
            max = STANDING_HONORED_MIN - 1;
            break;
        case ReputationStanding::HONORED:
            min = STANDING_HONORED_MIN;
            max = STANDING_REVERED_MIN - 1;
            break;
        case ReputationStanding::REVERED:
            min = STANDING_REVERED_MIN;
            max = STANDING_EXALTED_MIN - 1;
            break;
        case ReputationStanding::EXALTED:
            min = STANDING_EXALTED_MIN;
            max = 42999;
            break;
        default:
            min = 0;
            max = 0;
            break;
    }
}

void ReputationGrindManager::NotifyCallback(uint32 factionId, ReputationStanding newStanding)
{
    if (_callback)
    {
        _callback(factionId, newStanding);
    }
}

} // namespace Playerbot
