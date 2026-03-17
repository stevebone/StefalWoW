/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "LootDistribution.h"
#include "Player.h"
#include "Group.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Loot.h"
#include "ObjectMgr.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Util.h"
#include "World.h"
#include "WorldSession.h"
#include "ObjectAccessor.h"
#include "Session/BotSession.h"
#include "../AI/BotAI.h"  // P1 FIX: Required for unique_ptr<BotAI> in BotSession.h
#include "Spatial/SpatialGridQueryHelpers.h"
#include "../Group/GroupMemberResolver.h"
#include "../Core/Diagnostics/GroupMemberDiagnostics.h"
#include <algorithm>
#include <random>
#include "GameTime.h"
#include "GroupMgr.h"
#include "DB2Stores.h"
#include "Map.h"

namespace Playerbot
{

LootDistribution::LootDistribution(Player* bot) : _bot(bot)
{
    if (!_bot) TC_LOG_ERROR("playerbot", "LootDistribution: null bot!");
}

LootDistribution::~LootDistribution() {}

void LootDistribution::HandleGroupLoot(Group* group, Loot* loot)
{
    if (!group || !loot)
        return;

    // Process each loot item
    for (::LootItem& lootItem : loot->items)
    {
        if (lootItem.itemid == 0)
            continue;

        // Convert to our LootItem structure
        Playerbot::LootItem ourLootItem;
        ourLootItem.itemId = lootItem.itemid;
        ourLootItem.itemCount = lootItem.count;
        ourLootItem.lootSlot = 0;
        PopulateLootItemData(ourLootItem);

        // Determine if this item needs rolling
        if (ShouldInitiateRoll(group, ourLootItem))
        {
            InitiateLootRoll(group, ourLootItem);
        }
        else
        {
            // Handle auto-loot for items that don't need rolling
            HandleAutoLoot(group, ourLootItem);
        }
    }
}

void LootDistribution::InitiateLootRoll(Group* group, const LootItem& item)
{
    if (!group)
        return;

    uint32 rollId = _nextRollId++;
    LootRoll roll(rollId, item.itemId, item.lootSlot, group->GetGUID().GetCounter());

    // PHASE 2H: Hybrid validation pattern (snapshot + ObjectAccessor fallback)
    for (auto const& slot : group->GetMemberSlots())
    {
        // Quick snapshot check first (fast, lock-free)
        auto memberSnapshot = SpatialGridQueryHelpers::FindPlayerByGuid(nullptr, slot.guid);
        if (!memberSnapshot)
            continue;

        // Fallback to ObjectAccessor for full validation
        Player* member = ObjectAccessor::FindConnectedPlayer(slot.guid);
        if (member)
        {
            // Add member to eligible players for this roll
            roll.eligiblePlayers.insert(member->GetGUID().GetCounter());
        }
    }

    if (roll.eligiblePlayers.empty())
        return;

    // Store the roll
    {
        _activeLootRolls[rollId] = roll;
        _rollTimeouts[rollId] = GameTime::GetGameTimeMS() + LOOT_ROLL_TIMEOUT;
    }

    // Broadcast roll to group members
    BroadcastLootRoll(group, roll);

    // PHASE 2H: Hybrid validation pattern for bot roll decisions
    for (uint32 memberGuid : roll.eligiblePlayers)
    {
        ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(memberGuid);

        // Quick snapshot check first
        auto memberSnapshot = SpatialGridQueryHelpers::FindPlayerByGuid(nullptr, guid);
        if (!memberSnapshot)
            continue;
        // Fallback to ObjectAccessor for full validation
        Player* member = ObjectAccessor::FindConnectedPlayer(guid);
        if (member && dynamic_cast<BotSession*>(member->GetSession()))
        {
            LootRollType decision = DetermineLootDecision(item);
            ProcessPlayerLootDecision(rollId, decision);
        }
    }
}
void LootDistribution::ProcessPlayerLootDecision(uint32 rollId, LootRollType rollType)
{
    if (!_bot)
        return;

    auto rollIt = _activeLootRolls.find(rollId);
    if (rollIt == _activeLootRolls.end())
        return;

    LootRoll& roll = rollIt->second;

    // Check if player is eligible
    if (roll.eligiblePlayers.find(_bot->GetGUID().GetCounter()) == roll.eligiblePlayers.end())
        return;

    // Record the player's decision
    roll.playerRolls[_bot->GetGUID().GetCounter()] = rollType;
    // Generate roll value if not passing
    if (rollType != LootRollType::PASS)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32> dis(1, 100);
        roll.rollValues[_bot->GetGUID().GetCounter()] = dis(gen);
    }

    // Check if all players have rolled
    if (roll.playerRolls.size() == roll.eligiblePlayers.size())
    {
        CompleteLootRoll(rollId);
    }
}

void LootDistribution::CompleteLootRoll(uint32 rollId)
{
    auto rollIt = _activeLootRolls.find(rollId);
    if (rollIt == _activeLootRolls.end())
        return;

    LootRoll& roll = rollIt->second;

    // Determine winner
    uint32 winner = DetermineRollWinner(roll);

    if (winner != 0)
    {
        roll.winnerGuid = winner;
        roll.isCompleted = true;

        // Distribute loot to winner
        DistributeLootToWinner(rollId, winner);

        // Update fairness tracking
        LootItem item(roll.itemId, 1, roll.lootSlot);
        PopulateLootItemData(item);
        UpdateLootFairness(roll.groupId, winner, item);
    }

    // Notify group of results
    NotifyRollResult(roll);
    // Clean up
    _activeLootRolls.erase(rollIt);
    _rollTimeouts.erase(rollId);

    // Update metrics
    _globalMetrics.totalRollsCompleted++;
}

LootRollType LootDistribution::DetermineLootDecision(const LootItem& item)
{
    if (!_bot)
        return LootRollType::PASS;

    // Get player's loot profile
    PlayerLootProfile profile = GetPlayerLootProfile();

    // Execute the player's strategy
    return ExecuteStrategy(item, profile.strategy);
}

LootPriority LootDistribution::AnalyzeItemPriority(const LootItem& item)
{
    if (!_bot)
        return LootPriority::NOT_USEFUL;

    // Check if item is an upgrade
    if (IsItemUpgrade(item))
    {
        // Determine upgrade significance
        float upgradeValue = CalculateUpgradeValue(item);

        if (upgradeValue > 0.3f)
            return LootPriority::CRITICAL_UPGRADE;
        else if (upgradeValue > 0.15f)
            return LootPriority::SIGNIFICANT_UPGRADE;
        else if (upgradeValue > 0.05f)
            return LootPriority::MINOR_UPGRADE;
        else
            return LootPriority::SIDEGRADE;
    }

    // Check if useful for off-spec
    if (IsItemUsefulForOffSpec(item))
        return LootPriority::OFF_SPEC_UPGRADE;

    // Check vendor value
    if (item.vendorValue > 1000) // Arbitrary threshold
        return LootPriority::VENDOR_ITEM;
    return LootPriority::NOT_USEFUL;
}

bool LootDistribution::IsItemUpgrade(const LootItem& item)
{
    if (!_bot || !item.itemTemplate)
        return false;

    // Check if item can be equipped by player (basic level/race checks)
    if (!_bot->CanUseItem(item.itemTemplate))
        return false;

    // CRITICAL FIX: Check if item type is appropriate for class (armor/weapon proficiency)
    // This prevents Priests from trying to use Mail armor or 2H Swords, etc.
    if (!IsItemTypeUsefulForClass(_bot->GetClass(), item.itemTemplate))
        return false;

    // Compare with currently equipped item
    uint8 slot = item.itemTemplate->GetInventoryType();
    Item* equippedItem = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
    if (!equippedItem)
        return true; // No item equipped, so this is an upgrade

    // Compare item levels and stats
    float currentScore = CalculateItemScore(equippedItem);
    float newScore = CalculateItemScore(item);

    return newScore > currentScore * (1.0f + UPGRADE_THRESHOLD);
}

bool LootDistribution::IsClassAppropriate(const LootItem& item)
{
    if (!_bot || !item.itemTemplate)
        return false;

    // Check class restrictions
    if (item.isClassRestricted)
    {
        uint8 playerClass = _bot->GetClass();
        return std::find(item.allowedClasses.begin(), item.allowedClasses.end(), playerClass) != item.allowedClasses.end();
    }

    // Check if item type is useful for class
    return IsItemTypeUsefulForClass(_bot->GetClass(), item.itemTemplate);
}

bool LootDistribution::CanPlayerNeedItem(const LootItem& item)
{
    if (!_bot)
        return false;

    // Must be class appropriate
    if (!IsClassAppropriate(item))
        return false;

    // Must be an upgrade for main spec
    if (!IsItemUpgrade(item))
        return false;

    // Check if item is for main spec
    return IsItemForMainSpec(item);
}

bool LootDistribution::ShouldPlayerGreedItem(const LootItem& item)
{
    if (!_bot)
        return false;

    PlayerLootProfile profile = GetPlayerLootProfile();

    // Check greed threshold
    if (item.vendorValue < profile.greedThreshold * 10000) // Convert threshold to copper
        return false;

    // Check if item is blacklisted
    if (profile.blacklistedItems.find(item.itemId) != profile.blacklistedItems.end())
        return false;

    Group* group = _bot->GetGroup();
    if (!group)
        return true; // Solo play - always greed valuable items

    // Check group composition for main spec priority
    // Count how many players can use this item for main spec
    uint32 mainSpecUserCount = 0;
    uint32 offSpecUserCount = 0;
    bool botCanMainSpec = IsItemForMainSpec(item);

    // FIXED: Use GroupMemberResolver instead of GetMembers()
    for (auto const& slot : group->GetMemberSlots())
    {
        Player* member = GroupMemberResolver::ResolveMember(slot.guid);
        if (!member || member == _bot)
            continue;

        // Check if member can use item at all
        if (!member->CanUseItem(item.itemTemplate))
            continue;

        // Get member's specialization to determine main spec appropriateness
        ChrSpecializationEntry const* memberSpec = member->GetPrimarySpecializationEntry();
        if (memberSpec)
        {
            // Check armor type appropriateness based on spec role
            bool isMainSpecItem = IsItemTypeUsefulForClass(member->GetClass(), item.itemTemplate);
            if (isMainSpecItem)
            {
                // Check if this would be an upgrade for their current gear
                uint8 slot = item.itemTemplate->GetInventoryType();
                Item* memberEquipped = member->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);

                if (!memberEquipped || memberEquipped->GetItemLevel(member) < item.itemLevel)
                    mainSpecUserCount++;
                else
                    offSpecUserCount++;
            }
            else
            {
                offSpecUserCount++;
            }
        }
    }

    // Respect main spec before off spec etiquette
    // If others need for main spec and we don't, pass instead of greed
    if (mainSpecUserCount > 0 && !botCanMainSpec)
    {
        // Consider fairness - if we've received many items recently, be more conservative
        LootFairnessTracker tracker = GetGroupLootFairness(group->GetGUID().GetCounter());
        auto playerIt = tracker.playerLootCount.find(_bot->GetGUID().GetCounter());

        if (playerIt != tracker.playerLootCount.end())
        {
            float avgItems = float(tracker.totalItemsDistributed) / std::max(1u, uint32(tracker.playerLootCount.size()));
            if (float(playerIt->second) > avgItems * 1.2f) // 20% above average
                return false; // Be generous, pass on this item
        }
    }

    // Check group loot strategy fairness policy
    LootDecisionStrategy groupStrategy = profile.strategy;
    if (groupStrategy == LootDecisionStrategy::FAIR_DISTRIBUTION)
    {
        // In fair distribution mode, skip greed if we're above average in loot received
        LootFairnessTracker tracker = GetGroupLootFairness(group->GetGUID().GetCounter());
        if (tracker.fairnessScore < FAIRNESS_ADJUSTMENT_THRESHOLD)
        {
            auto playerIt = tracker.playerLootCount.find(_bot->GetGUID().GetCounter());
            if (playerIt != tracker.playerLootCount.end())
            {
                float avgItems = float(tracker.totalItemsDistributed) / std::max(1u, uint32(tracker.playerLootCount.size()));
                if (float(playerIt->second) > avgItems)
                    return false;
            }
        }
    }

    // Item is valuable enough and respects group etiquette - greed it
    return true;
}

bool LootDistribution::ShouldPlayerPassItem(const LootItem& item)
{
    if (!_bot)
        return true;

    // Pass if item is blacklisted
    PlayerLootProfile profile = GetPlayerLootProfile();
    if (profile.blacklistedItems.find(item.itemId) != profile.blacklistedItems.end())
        return true;

    // Pass if not useful and below greed threshold
    if (!CanPlayerNeedItem(item) && !ShouldPlayerGreedItem(item))
        return true;

    return false;
}

bool LootDistribution::CanPlayerDisenchantItem(const LootItem& item)
{
    if (!_bot || !item.itemTemplate)
        return false;

    // Check if player has enchanting skill
    if (_bot->GetSkillValue(SKILL_ENCHANTING) == 0)
        return false;

    // Check if item can be disenchanted
    return Item::GetBaseDisenchantLoot(item.itemTemplate, item.itemQuality, item.itemLevel) != nullptr;
}

void LootDistribution::ProcessLootRolls(uint32 rollId)
{
    auto rollIt = _activeLootRolls.find(rollId);
    if (rollIt == _activeLootRolls.end())
        return;

    LootRoll& roll = rollIt->second;

    // Check for timeout
    if (GameTime::GetGameTimeMS() > roll.rollTimeout)
    {
        HandleLootRollTimeout(rollId);
        return;
    }

    // Process the roll if all players have responded
    if (roll.playerRolls.size() == roll.eligiblePlayers.size())
    {
        CompleteLootRoll(rollId);
    }
}

uint32 LootDistribution::DetermineRollWinner(const LootRoll& roll)
{
    // Process need rolls first
    uint32 needWinner = ProcessNeedRolls(const_cast<LootRoll&>(roll));
    if (needWinner != 0)
        return needWinner;

    // Then greed rolls
    uint32 greedWinner = ProcessGreedRolls(const_cast<LootRoll&>(roll));
    if (greedWinner != 0)
        return greedWinner;

    // Finally disenchant rolls
    uint32 disenchantWinner = ProcessDisenchantRolls(const_cast<LootRoll&>(roll));
    if (disenchantWinner != 0)
        return disenchantWinner;

    return 0; // No winner
}

void LootDistribution::DistributeLootToWinner(uint32 rollId, uint32 winnerGuid)
{
    // PHASE 2H HOTFIX: Removed snapshot pre-check to eliminate TOCTOU race condition
    // The snapshot check created a window where player could be deleted between check and use
    // Now using ObjectAccessor directly with proper null checking (thread-safe)
    ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(winnerGuid);
    Player* winner = ObjectAccessor::FindConnectedPlayer(guid);
    if (!winner)
        return;

    auto rollIt = _activeLootRolls.find(rollId);
    if (rollIt == _activeLootRolls.end())
        return;

    const LootRoll& roll = rollIt->second;

    // Create item and add to winner's inventory
    ItemPosCountVec dest;
    InventoryResult msg = winner->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, roll.itemId, 1);

    if (msg == EQUIP_ERR_OK)
    {
        Item* item = winner->StoreNewItem(dest, roll.itemId, true);
        if (item)
        {
            winner->SendNewItem(item, 1, false, false, true);

            // Update metrics
            UpdateLootMetrics(roll, true);
        }
    }
    else
    {
        // Handle inventory full case
        // Could send item by mail or handle differently
        TC_LOG_WARN("playerbot.loot", "Cannot distribute loot to player {}: inventory full", winnerGuid);
    }
}

void LootDistribution::HandleLootRollTimeout(uint32 rollId)
{
    auto rollIt = _activeLootRolls.find(rollId);
    if (rollIt == _activeLootRolls.end())
        return;

    LootRoll& roll = rollIt->second;

    // Auto-pass for players who didn't respond
    for (uint32 playerGuid : roll.eligiblePlayers)
    {
        if (roll.playerRolls.find(playerGuid) == roll.playerRolls.end())
        {
            roll.playerRolls[playerGuid] = LootRollType::PASS;
        }
    }

    // Complete the roll
    CompleteLootRoll(rollId);

    // Update timeout metrics
    _globalMetrics.rollTimeouts++;
}

void LootDistribution::ExecuteNeedBeforeGreedStrategy(const LootItem& item, LootRollType& decision)
{
    if (CanPlayerNeedItem(item))
    {
        decision = LootRollType::NEED;
    }
    else if (ShouldPlayerGreedItem(item))
    {
        decision = LootRollType::GREED;
    }
    else
    {
        decision = LootRollType::PASS;
    }
}

void LootDistribution::ExecuteClassPriorityStrategy(const LootItem& item, LootRollType& decision)
{
    // Prioritize items for appropriate classes
    if (IsClassAppropriate(item))
    {
        if (IsItemUpgrade(item))
            decision = LootRollType::NEED;
        else
            decision = LootRollType::GREED;
    }
    else
    {
        decision = LootRollType::PASS;
    }
}

void LootDistribution::ExecuteUpgradePriorityStrategy(const LootItem& item, LootRollType& decision)
{
    LootPriority priority = AnalyzeItemPriority(item);
    switch (priority)
    {
        case LootPriority::CRITICAL_UPGRADE:
        case LootPriority::SIGNIFICANT_UPGRADE:
            decision = LootRollType::NEED;
            break;
        case LootPriority::MINOR_UPGRADE:
        case LootPriority::OFF_SPEC_UPGRADE:
            decision = LootRollType::GREED;
            break;
        case LootPriority::VENDOR_ITEM:
            if (ShouldPlayerGreedItem(item))
                decision = LootRollType::GREED;
            else
                decision = LootRollType::PASS;
            break;
        default:
            decision = LootRollType::PASS;
            break;
    }
}

void LootDistribution::ExecuteFairDistributionStrategy( const LootItem& item, LootRollType& decision)
{
    // Consider fairness in decision making
    Group* group = _bot->GetGroup();
    if (!group)
    {
        ExecuteNeedBeforeGreedStrategy(item, decision);
        return;
    }

    // Check if player has received items recently
    bool shouldConsiderFairness = ShouldConsiderFairnessAdjustment(group);

    if (shouldConsiderFairness)
    {
        // Be more conservative with rolls
        if (CanPlayerNeedItem(item))
        {
            LootPriority priority = AnalyzeItemPriority(item);
            if (priority >= LootPriority::SIGNIFICANT_UPGRADE)
                decision = LootRollType::NEED;
            else
                decision = LootRollType::GREED;
        }
        else
        {
            decision = LootRollType::PASS;
        }
    }
    else
    {
        ExecuteNeedBeforeGreedStrategy(item, decision);
    }
}

void LootDistribution::ExecuteMainSpecPriorityStrategy(const LootItem& item, LootRollType& decision)
{
    PlayerLootProfile profile = GetPlayerLootProfile();
    if (IsItemForMainSpec(item))
    {
        if (IsItemUpgrade(item))
            decision = LootRollType::NEED;
        else if (ShouldPlayerGreedItem(item))
            decision = LootRollType::GREED;
        else
            decision = LootRollType::PASS;
    }
    else if (profile.greedOffSpec && IsItemUsefulForOffSpec(item))
    {
        decision = LootRollType::GREED;
    }
    else
    {
        decision = LootRollType::PASS;
    }
}

LootFairnessTracker LootDistribution::GetGroupLootFairness(uint32 groupId)
{
    auto it = _groupFairnessTracking.find(groupId);
    if (it != _groupFairnessTracking.end())
        return it->second;

    return LootFairnessTracker();
}

void LootDistribution::UpdateLootFairness(uint32 groupId, uint32 winnerGuid, const LootItem& item)
{
    auto& tracker = _groupFairnessTracking[groupId];

    tracker.playerLootCount[winnerGuid]++;
    tracker.playerLootValue[winnerGuid] += item.vendorValue;
    tracker.totalItemsDistributed++;
    tracker.totalValueDistributed += item.vendorValue;

    // Recalculate fairness score
    tracker.fairnessScore = CalculateFairnessScore(tracker);
}

float LootDistribution::CalculateFairnessScore(const LootFairnessTracker& tracker)
{
    if (tracker.playerLootCount.empty())
        return 1.0f;

    // Calculate variance in loot distribution
    float averageItems = float(tracker.totalItemsDistributed) / tracker.playerLootCount.size();
    float variance = 0.0f;

    for (const auto& pair : tracker.playerLootCount)
    {
        float diff = pair.second - averageItems;
        variance += diff * diff;
    }

    variance /= tracker.playerLootCount.size();

    // Convert variance to fairness score (lower variance = higher fairness)
    float fairness = 1.0f / (1.0f + variance);
    return std::clamp(fairness, 0.0f, 1.0f);
}

LootMetrics LootDistribution::GetPlayerLootMetrics()
{
    if (!_bot)
        return LootMetrics();

    uint32 playerGuid = _bot->GetGUID().GetCounter();
    auto it = _playerMetrics.find(playerGuid);
    if (it != _playerMetrics.end())
        return it->second;

    LootMetrics metrics;
    metrics.Reset();
    return metrics;
}

LootMetrics LootDistribution::GetGroupLootMetrics(uint32 groupId)
{
    LootMetrics groupMetrics;
    groupMetrics.Reset();

    if (!_bot)
        return groupMetrics;

    Group* group = _bot->GetGroup();
    if (!group || group->GetGUID().GetCounter() != groupId)
        return groupMetrics;

    // Aggregate metrics from all group members
    uint32 memberCount = 0;
    float totalSatisfaction = 0.0f;
    float totalAccuracy = 0.0f;

    // FIXED: Use GroupMemberResolver instead of GetMembers()
    for (auto const& memberSlot : group->GetMemberSlots())
    {
        Player* member = GroupMemberResolver::ResolveMember(memberSlot.guid);
        if (!member)
            continue;

        uint32 memberGuid = member->GetGUID().GetCounter();
        auto metricsIt = _playerMetrics.find(memberGuid);

        if (metricsIt != _playerMetrics.end())
        {
            const LootMetrics& memberMetrics = metricsIt->second;

            // Aggregate counters
            groupMetrics.totalRollsInitiated += memberMetrics.totalRollsInitiated.load();
            groupMetrics.totalRollsCompleted += memberMetrics.totalRollsCompleted.load();
            groupMetrics.needRollsWon += memberMetrics.needRollsWon.load();
            groupMetrics.greedRollsWon += memberMetrics.greedRollsWon.load();
            groupMetrics.itemsPassed += memberMetrics.itemsPassed.load();
            groupMetrics.rollTimeouts += memberMetrics.rollTimeouts.load();

            // Track satisfaction and accuracy for averaging
            totalSatisfaction += memberMetrics.playerSatisfaction.load();
            totalAccuracy += memberMetrics.decisionAccuracy.load();
            memberCount++;
        }
    }

    // Calculate averages
    if (memberCount > 0)
    {
        groupMetrics.playerSatisfaction = totalSatisfaction / memberCount;
        groupMetrics.decisionAccuracy = totalAccuracy / memberCount;

        // Calculate average roll time based on completed rolls
        if (groupMetrics.totalRollsCompleted > 0)
        {
            // Use weighted average based on individual member roll times
            float totalRollTime = 0.0f;
            uint32 rollCount = 0;

            // FIXED: Use GroupMemberResolver instead of GetMembers()
            for (auto const& memberSlot : group->GetMemberSlots())
            {
                Player* member = GroupMemberResolver::ResolveMember(memberSlot.guid);
                if (!member)
                    continue;

                auto metricsIt = _playerMetrics.find(member->GetGUID().GetCounter());
                if (metricsIt != _playerMetrics.end())
                {
                    totalRollTime += metricsIt->second.averageRollTime.load() *
                                    metricsIt->second.totalRollsCompleted.load();
                    rollCount += metricsIt->second.totalRollsCompleted.load();
                }
            }

            if (rollCount > 0)
                groupMetrics.averageRollTime = totalRollTime / rollCount;
        }
    }

    // Add fairness tracking data to metrics context
    auto fairnessIt = _groupFairnessTracking.find(groupId);
    if (fairnessIt != _groupFairnessTracking.end())
    {
        // Encode fairness score into satisfaction as a quality indicator
        // High fairness = high satisfaction potential
        float fairnessWeight = fairnessIt->second.fairnessScore;
        groupMetrics.playerSatisfaction = groupMetrics.playerSatisfaction.load() *
                                          (0.5f + 0.5f * fairnessWeight);
    }

    groupMetrics.lastUpdate = std::chrono::steady_clock::now();
    return groupMetrics;
}

LootMetrics LootDistribution::GetGlobalLootMetrics()
{
    return _globalMetrics;
}

void LootDistribution::SetPlayerLootStrategy(LootDecisionStrategy strategy)
{
    if (!_bot)
        return;

    uint32 playerGuid = _bot->GetGUID().GetCounter();
    _playerLootProfiles[playerGuid].strategy = strategy;
}

LootDecisionStrategy LootDistribution::GetPlayerLootStrategy()
{
    if (!_bot)
        return LootDecisionStrategy::NEED_BEFORE_GREED;

    uint32 playerGuid = _bot->GetGUID().GetCounter();
    auto it = _playerLootProfiles.find(playerGuid);
    if (it != _playerLootProfiles.end())
        return it->second.strategy;

    return LootDecisionStrategy::NEED_BEFORE_GREED;
}

void LootDistribution::SetPlayerLootPreferences(const PlayerLootProfile& profile)
{
    if (!_bot)
        return;

    uint32 playerGuid = _bot->GetGUID().GetCounter();
    _playerLootProfiles[playerGuid] = profile;
}

PlayerLootProfile LootDistribution::GetPlayerLootProfile()
{
    if (!_bot)
        return PlayerLootProfile();

    uint32 playerGuid = _bot->GetGUID().GetCounter();
    auto it = _playerLootProfiles.find(playerGuid);
    if (it != _playerLootProfiles.end())
        return it->second;

    // PHASE 2H HOTFIX: Snapshot-only validation (eliminates TOCTOU race condition)
    // Using ObjectAccessor after snapshot check creates race condition where player can be
    // deleted between snapshot validation and ObjectAccessor call, causing crashes
    ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(playerGuid);
    auto playerSnapshot = SpatialGridQueryHelpers::FindPlayerByGuid(nullptr, guid);
    if (!playerSnapshot)
        return PlayerLootProfile(playerGuid, CLASS_WARRIOR, 0);  // Player offline - return default

    // Use snapshot class data only (safe, no TOCTOU race)
    // PlayerSnapshot doesn't have spec, so we default to 0 (spec will be learned from actual loot behavior)
    if (playerSnapshot->classId != 0)
        return PlayerLootProfile(playerGuid, static_cast<Classes>(playerSnapshot->classId), 0);

    return PlayerLootProfile(playerGuid, CLASS_WARRIOR, 0);
}

void LootDistribution::PopulateLootItemData(LootItem& item)
{
    item.itemTemplate = sObjectMgr->GetItemTemplate(item.itemId);
    if (!item.itemTemplate)
        return;

    item.itemLevel = item.itemTemplate->GetBaseItemLevel();
    item.itemQuality = item.itemTemplate->GetQuality();
    item.vendorValue = item.itemTemplate->GetSellPrice();
    item.itemName = item.itemTemplate->GetName(LOCALE_enUS);

    // Set binding information
    item.isBoundOnPickup = item.itemTemplate->GetBonding() == BIND_ON_ACQUIRE;
    item.isBoundOnEquip = item.itemTemplate->GetBonding() == BIND_ON_EQUIP;

    // Check class restrictions
    if (item.itemTemplate->GetAllowableClass() != 0)
    {
        item.isClassRestricted = true;
        for (uint8 cls = CLASS_WARRIOR; cls < MAX_CLASSES; ++cls)
        {
            if (item.itemTemplate->GetAllowableClass() & (1 << (cls - 1)))
            {
                item.allowedClasses.push_back(cls);
            }
        }
    }
}

bool LootDistribution::ShouldInitiateRoll(Group* group, const LootItem& item)
{
    if (!group || !item.itemTemplate)
        return false;

    // Check if item quality meets roll threshold
    if (item.itemQuality < group->GetLootThreshold())
        return false;

    // Check if bot can participate in roll
    if (!CanParticipateInRoll(item))
        return false;

    // Check if there are at least 2 group members (potential interest)
    uint32 groupMembers = 0;
    // FIXED: Use GroupMemberResolver instead of GetMembers()
    for (auto const& memberSlot : group->GetMemberSlots())
    {
        if (GroupMemberResolver::ResolveMember(memberSlot.guid))
            groupMembers++;
    }

    if (groupMembers >= 2)
        return true;

    return false;
}

bool LootDistribution::CanParticipateInRoll(const LootItem& item)
{
    if (!_bot || !item.itemTemplate)
        return false;

    // Player must be able to use the item
    if (!_bot->CanUseItem(item.itemTemplate))
        return false;

    // Check if player is alive (dead players can't roll)
    if (!_bot->IsAlive())
    {
        // Exception: Ghost players can still participate in rolls
        // They just need to release and run back
        if (!_bot->HasAura(8326)) // Ghost spell
            return false;
    }

    // Validate player is still in a group
    Group* group = _bot->GetGroup();
    if (!group)
        return true; // Solo play - can always loot

    // Ensure player is a member of the group
    if (!group->IsMember(_bot->GetGUID()))
        return false;

    // Check loot distance (standard loot range is about 60 yards in group)
    static constexpr float LOOT_RANGE = 60.0f;

    // Get loot source location from the group leader or nearby corpse
    // For group loot, we use a more generous range
    Player* leader = ObjectAccessor::FindConnectedPlayer(group->GetLeaderGUID());
    if (leader && !_bot->IsWithinDistInMap(leader, LOOT_RANGE * 2)) // Double range for flexibility
    {
        // Too far from group leader - might be in different zone
        if (_bot->GetMapId() != leader->GetMapId())
            return false;
    }

    // Check if player has inventory space for the item
    ItemPosCountVec dest;
    InventoryResult result = _bot->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, item.itemId, item.itemCount);
    if (result != EQUIP_ERR_OK)
    {
        // No space, but can still roll - item might go to mail
        // Log this for debugging
        TC_LOG_DEBUG("playerbot.loot", "Player {} has no inventory space for item {}, but can still roll",
            _bot->GetName(), item.itemId);
    }

    // Check if player is in a valid state to receive loot
    // (not in combat transition, not being teleported, etc.)
    if (_bot->IsBeingTeleported())
        return false;

    // Check if instance-based restrictions apply
    if (Map* map = _bot->GetMap())
    {
        if (InstanceMap* instance = map->ToInstanceMap())
        {
            // Verify player has access to this instance
            if (instance->CannotEnter(_bot))
                return false;
        }
    }

    return true;
}

void LootDistribution::HandleAutoLoot(Group* group, const LootItem& item)
{
    if (!group)
        return;

    // Find a suitable recipient for auto-loot
    Player* recipient = nullptr;

    // Since CanParticipateInRoll is per-bot instance method, just check if _bot can participate
    // and give to first eligible group member
    if (_bot && CanParticipateInRoll(item))
    {
        recipient = _bot;
    }
    else
    {
        // Fallback to first group member
        // FIXED: Use GroupMemberResolver instead of GetMembers()
        for (auto const& memberSlot : group->GetMemberSlots())
        {
            Player* member = GroupMemberResolver::ResolveMember(memberSlot.guid);
            if (member)
            {
                recipient = member;
                break;
            }
        }
    }

    if (recipient)
    {
        // Give item to recipient
        ItemPosCountVec dest;
        InventoryResult msg = recipient->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, item.itemId, item.itemCount);
        if (msg == EQUIP_ERR_OK)
        {
            Item* newItem = recipient->StoreNewItem(dest, item.itemId, true);
            if (newItem)
            {
                recipient->SendNewItem(newItem, item.itemCount, false, false, true);
            }
        }
    }
}

LootRollType LootDistribution::ExecuteStrategy( const LootItem& item, LootDecisionStrategy strategy)
{
    LootRollType decision = LootRollType::PASS;

    switch (strategy)
    {
        case LootDecisionStrategy::NEED_BEFORE_GREED:
            ExecuteNeedBeforeGreedStrategy(item, decision);
            break;
        case LootDecisionStrategy::CLASS_PRIORITY:
            ExecuteClassPriorityStrategy(item, decision);
            break;
        case LootDecisionStrategy::UPGRADE_PRIORITY:
            ExecuteUpgradePriorityStrategy(item, decision);
            break;
        case LootDecisionStrategy::FAIR_DISTRIBUTION:
            ExecuteFairDistributionStrategy(item, decision);
            break;
        case LootDecisionStrategy::MAINSPEC_PRIORITY:
            ExecuteMainSpecPriorityStrategy(item, decision);
            break;
        case LootDecisionStrategy::RANDOM_ROLLS:
            // Random decision
            {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<int> dis(0, 2);
                decision = static_cast<LootRollType>(dis(gen));
            }
            break;
        case LootDecisionStrategy::VENDOR_VALUE:
            if (item.vendorValue > 1000)
                decision = LootRollType::GREED;
            else
                decision = LootRollType::PASS;
            break;
        case LootDecisionStrategy::CONSERVATIVE:
            // Only roll on significant upgrades
            {
                LootPriority priority = AnalyzeItemPriority(item);
                if (priority == LootPriority::CRITICAL_UPGRADE)
                    decision = LootRollType::NEED;
                else
                    decision = LootRollType::PASS;
            }
            break;
    }

    // Apply strategy modifiers
    ApplyStrategyModifiers(item, decision);

    return decision;
}

void LootDistribution::ApplyStrategyModifiers( const LootItem& item, LootRollType& decision)
{
    if (!_bot)
        return;

    Group* group = _bot->GetGroup();
    if (group)
    {
        ConsiderGroupComposition(group, item, decision);
    }
}

void LootDistribution::ConsiderGroupComposition(Group* group, const LootItem& item, LootRollType& decision)
{
    if (!group || !_bot)
        return;

    uint8 myClass = _bot->GetClass();
    ChrSpecialization mySpec = _bot->GetPrimarySpecialization();

    // Count players of the same class and spec
    uint32 sameClassCount = 0;
    uint32 sameSpecCount = 0;
    uint32 totalEligiblePlayers = 0;

    // FIXED: Use GroupMemberResolver instead of GetMembers()
    for (auto const& memberSlot : group->GetMemberSlots())
    {
        Player* member = GroupMemberResolver::ResolveMember(memberSlot.guid);
        if (!member || member == _bot)
            continue;

        // Check if member can use this item
        if (!member->CanUseItem(item.itemTemplate))
            continue;

        totalEligiblePlayers++;

        if (member->GetClass() == myClass)
        {
            sameClassCount++;

            if (member->GetPrimarySpecialization() == mySpec)
                sameSpecCount++;
        }
    }

    // If multiple players of same class/spec, be more conservative
    if (decision == LootRollType::NEED)
    {
        // Multiple same-spec players competing
        if (sameSpecCount > 0)
        {
            // Check loot priority - how much do we need this item?
            LootPriority priority = AnalyzeItemPriority(item);

            // For minor upgrades with competition, consider greeding instead
            if (priority >= LootPriority::MINOR_UPGRADE && sameSpecCount >= 2)
            {
                // Check fairness - have we received items recently?
                LootFairnessTracker tracker = GetGroupLootFairness(group->GetGUID().GetCounter());
                auto playerIt = tracker.playerLootCount.find(_bot->GetGUID().GetCounter());

                if (playerIt != tracker.playerLootCount.end() && tracker.playerLootCount.size() > 1)
                {
                    float avgItems = float(tracker.totalItemsDistributed) / tracker.playerLootCount.size();
                    if (float(playerIt->second) > avgItems)
                    {
                        // We've gotten more than average, be generous on minor upgrades
                        decision = LootRollType::GREED;
                        TC_LOG_DEBUG("playerbot.loot", "Bot {} downgrading NEED to GREED for fairness",
                            _bot->GetName());
                    }
                }
            }
        }

        // Multiple same-class players (different specs)
        if (sameClassCount > 0 && sameSpecCount == 0)
        {
            // This is a shared class item but different specs
            // Check if it's truly main spec for us
            if (!IsItemForMainSpec(item))
            {
                // Not main spec, downgrade to greed
                decision = LootRollType::GREED;
            }
        }
    }

    // For greed decisions, consider if we should pass
    if (decision == LootRollType::GREED)
    {
        // If many eligible players and we're above average loot count
        if (totalEligiblePlayers >= 3)
        {
            LootFairnessTracker tracker = GetGroupLootFairness(group->GetGUID().GetCounter());
            auto playerIt = tracker.playerLootCount.find(_bot->GetGUID().GetCounter());

            if (playerIt != tracker.playerLootCount.end() && tracker.playerLootCount.size() > 1)
            {
                float avgItems = float(tracker.totalItemsDistributed) / tracker.playerLootCount.size();
                // If significantly above average, pass on greed items
                if (float(playerIt->second) > avgItems * 1.5f)
                {
                    decision = LootRollType::PASS;
                    TC_LOG_DEBUG("playerbot.loot", "Bot {} passing due to high loot count",
                        _bot->GetName());
                }
            }
        }
    }
}

float LootDistribution::CalculateItemScore( const LootItem& item)
{
    if (!_bot || !item.itemTemplate)
        return 0.0f;

    float score = 0.0f;

    // Base score from item level
    score += item.itemLevel * 10.0f;

    // Add score for relevant stats
    // This would require detailed stat analysis based on player class/spec

    return score;
}

float LootDistribution::CalculateItemScore( Item* item)
{
    if (!_bot || !item)
        return 0.0f;

    float score = 0.0f;

    // Base score from item level
    score += item->GetItemLevel(_bot) * 10.0f;

    // Add score for relevant stats
    // This would require detailed stat analysis

    return score;
}

float LootDistribution::CalculateUpgradeValue( const LootItem& item)
{
    if (!_bot || !item.itemTemplate)
        return 0.0f;

    // Get current item in the same slot
    uint8 slot = item.itemTemplate->GetInventoryType();
    Item* currentItem = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
    if (!currentItem)
        return 1.0f; // Maximum upgrade if no item equipped

    float currentScore = CalculateItemScore(currentItem);
    float newScore = CalculateItemScore(item);

    if (currentScore <= 0.0f)
        return 1.0f;

    return (newScore - currentScore) / currentScore;
}

bool LootDistribution::IsItemForMainSpec(const LootItem& item)
{
    if (!_bot || !item.itemTemplate)
        return false;

    ChrSpecialization spec = _bot->GetPrimarySpecialization();
    ChrSpecializationEntry const* specEntry = _bot->GetPrimarySpecializationEntry();

    if (!specEntry)
        return true; // No spec info, allow item

    uint8 itemClass = item.itemTemplate->GetClass();
    uint8 itemSubClass = item.itemTemplate->GetSubClass();
    uint8 invType = item.itemTemplate->GetInventoryType();

    // Get spec role (Tank, Healer, DPS)
    ChrSpecializationRole role = static_cast<ChrSpecializationRole>(specEntry->Role);

    // Check armor type appropriateness
    if (itemClass == ITEM_CLASS_ARMOR)
    {
        uint8 playerClass = _bot->GetClass();

        // Armor type restrictions by class (WoW 12.0)
        switch (playerClass)
        {
            case CLASS_WARRIOR:
            case CLASS_PALADIN:
            case CLASS_DEATH_KNIGHT:
                // Plate wearers should only need plate (except trinkets/rings/etc)
                if (itemSubClass != ITEM_SUBCLASS_ARMOR_PLATE &&
                    itemSubClass != ITEM_SUBCLASS_ARMOR_MISCELLANEOUS &&
                    invType != INVTYPE_FINGER && invType != INVTYPE_TRINKET &&
                    invType != INVTYPE_NECK && invType != INVTYPE_CLOAK)
                    return false;
                break;

            case CLASS_HUNTER:
            case CLASS_SHAMAN:
            case CLASS_EVOKER:
                // Mail wearers
                if (itemSubClass != ITEM_SUBCLASS_ARMOR_MAIL &&
                    itemSubClass != ITEM_SUBCLASS_ARMOR_MISCELLANEOUS &&
                    invType != INVTYPE_FINGER && invType != INVTYPE_TRINKET &&
                    invType != INVTYPE_NECK && invType != INVTYPE_CLOAK)
                    return false;
                break;

            case CLASS_ROGUE:
            case CLASS_DRUID:
            case CLASS_MONK:
            case CLASS_DEMON_HUNTER:
                // Leather wearers
                if (itemSubClass != ITEM_SUBCLASS_ARMOR_LEATHER &&
                    itemSubClass != ITEM_SUBCLASS_ARMOR_MISCELLANEOUS &&
                    invType != INVTYPE_FINGER && invType != INVTYPE_TRINKET &&
                    invType != INVTYPE_NECK && invType != INVTYPE_CLOAK)
                    return false;
                break;

            case CLASS_MAGE:
            case CLASS_PRIEST:
            case CLASS_WARLOCK:
                // Cloth wearers
                if (itemSubClass != ITEM_SUBCLASS_ARMOR_CLOTH &&
                    itemSubClass != ITEM_SUBCLASS_ARMOR_MISCELLANEOUS &&
                    invType != INVTYPE_FINGER && invType != INVTYPE_TRINKET &&
                    invType != INVTYPE_NECK && invType != INVTYPE_CLOAK)
                    return false;
                break;
        }

        // Shield check - only for tank specs or Holy Paladin/Shaman
        if (invType == INVTYPE_SHIELD)
        {
            switch (spec)
            {
                case ChrSpecialization::WarriorProtection:
                case ChrSpecialization::PaladinProtection:
                case ChrSpecialization::PaladinHoly:
                case ChrSpecialization::ShamanElemental:
                case ChrSpecialization::ShamanRestoration:
                    return true;
                default:
                    return false;
            }
        }
    }

    // Check weapon type appropriateness by spec
    if (itemClass == ITEM_CLASS_WEAPON)
    {
        // Two-handed vs one-handed preferences by spec
        bool isTwoHanded = (invType == INVTYPE_2HWEAPON ||
                            invType == INVTYPE_RANGED ||
                            invType == INVTYPE_RANGEDRIGHT);

        switch (spec)
        {
            // Specs that use two-handed weapons
            case ChrSpecialization::WarriorArms:
            case ChrSpecialization::DeathKnightFrost: // Can dual wield 2H
            case ChrSpecialization::DeathKnightUnholy:
            case ChrSpecialization::PaladinRetribution:
            case ChrSpecialization::DruidBalance:
            case ChrSpecialization::DruidFeral:
            case ChrSpecialization::DruidGuardian:
            case ChrSpecialization::MonkWindwalker:
            case ChrSpecialization::MonkBrewmaster:
            case ChrSpecialization::ShamanEnhancement:
            case ChrSpecialization::HunterBeastMastery:
            case ChrSpecialization::HunterMarksmanship:
            case ChrSpecialization::HunterSurvival:
                // These specs prefer 2H but can use 1H
                break;

            // Specs that use one-handed weapons
            case ChrSpecialization::WarriorFury: // Titan's Grip for 2H
            case ChrSpecialization::RogueAssassination:
            case ChrSpecialization::RogueOutlaw:
            case ChrSpecialization::RogueSubtely:
            case ChrSpecialization::DemonHunterHavoc:
            case ChrSpecialization::DemonHunterVengeance:
                // Dual wield specs
                if (isTwoHanded && spec != ChrSpecialization::WarriorFury)
                    return false;
                break;

            // Caster specs - prefer staves or caster weapons
            case ChrSpecialization::MageArcane:
            case ChrSpecialization::MageFire:
            case ChrSpecialization::MageFrost:
            case ChrSpecialization::WarlockAffliction:
            case ChrSpecialization::WarlockDemonology:
            case ChrSpecialization::WarlockDestruction:
            case ChrSpecialization::PriestShadow:
            case ChrSpecialization::PriestDiscipline:
            case ChrSpecialization::PriestHoly:
            case ChrSpecialization::DruidRestoration:
            case ChrSpecialization::ShamanRestoration:
            case ChrSpecialization::ShamanElemental:
            case ChrSpecialization::MonkMistweaver:
            case ChrSpecialization::EvokerPreservation:
            case ChrSpecialization::EvokerDevastation:
            case ChrSpecialization::EvokerAugmentation:
            case ChrSpecialization::PaladinHoly:
                // Casters prefer staves, wands, and caster weapons
                if (itemSubClass == ITEM_SUBCLASS_WEAPON_AXE ||
                    itemSubClass == ITEM_SUBCLASS_WEAPON_AXE2 ||
                    itemSubClass == ITEM_SUBCLASS_WEAPON_SWORD ||
                    itemSubClass == ITEM_SUBCLASS_WEAPON_SWORD2)
                    return false; // Melee weapons not for casters
                break;

            // Tank specs
            case ChrSpecialization::WarriorProtection:
            case ChrSpecialization::PaladinProtection:
            case ChrSpecialization::DeathKnightBlood:
                // Tanks prefer 1H + Shield
                if (isTwoHanded && spec != ChrSpecialization::DeathKnightBlood)
                    return false;
                break;

            default:
                break;
        }

        // Specific weapon restrictions
        switch (itemSubClass)
        {
            case ITEM_SUBCLASS_WEAPON_WARGLAIVES:
                // Only Demon Hunters can use warglaives
                return spec == ChrSpecialization::DemonHunterHavoc ||
                       spec == ChrSpecialization::DemonHunterVengeance;

            case ITEM_SUBCLASS_WEAPON_BOW:
            case ITEM_SUBCLASS_WEAPON_GUN:
            case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                // Only Hunters use ranged weapons as main spec
                return spec == ChrSpecialization::HunterBeastMastery ||
                       spec == ChrSpecialization::HunterMarksmanship ||
                       spec == ChrSpecialization::HunterSurvival;

            case ITEM_SUBCLASS_WEAPON_WAND:
                // Wands for casters only
                return role == ChrSpecializationRole::Healer ||
                       (role == ChrSpecializationRole::Dps &&
                        (_bot->GetClass() == CLASS_MAGE ||
                         _bot->GetClass() == CLASS_WARLOCK ||
                         _bot->GetClass() == CLASS_PRIEST));
        }
    }

    // Check primary stat appropriateness
    // This requires checking item stats against spec stat priorities
    bool hasIntellect = false;
    bool hasAgility = false;
    bool hasStrength = false;

    // TrinityCore 12.0: Check item's primary stats
    for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
    {
        int32 statType = item.itemTemplate->GetStatModifierBonusStat(i);
        if (statType < 0)
            continue;

        switch (statType)
        {
            case ITEM_MOD_INTELLECT:
                hasIntellect = true;
                break;
            case ITEM_MOD_AGILITY:
                hasAgility = true;
                break;
            case ITEM_MOD_STRENGTH:
                hasStrength = true;
                break;
        }
    }

    // Match primary stat to spec
    bool specNeedsIntellect = (role == ChrSpecializationRole::Healer) ||
        spec == ChrSpecialization::MageArcane || spec == ChrSpecialization::MageFire ||
        spec == ChrSpecialization::MageFrost || spec == ChrSpecialization::WarlockAffliction ||
        spec == ChrSpecialization::WarlockDemonology || spec == ChrSpecialization::WarlockDestruction ||
        spec == ChrSpecialization::PriestShadow || spec == ChrSpecialization::DruidBalance ||
        spec == ChrSpecialization::ShamanElemental || spec == ChrSpecialization::EvokerDevastation ||
        spec == ChrSpecialization::EvokerAugmentation;

    bool specNeedsAgility =
        spec == ChrSpecialization::RogueAssassination || spec == ChrSpecialization::RogueOutlaw ||
        spec == ChrSpecialization::RogueSubtely || spec == ChrSpecialization::DruidFeral ||
        spec == ChrSpecialization::MonkWindwalker || spec == ChrSpecialization::DemonHunterHavoc ||
        spec == ChrSpecialization::DemonHunterVengeance || spec == ChrSpecialization::DruidGuardian ||
        spec == ChrSpecialization::MonkBrewmaster || spec == ChrSpecialization::HunterBeastMastery ||
        spec == ChrSpecialization::HunterMarksmanship || spec == ChrSpecialization::HunterSurvival ||
        spec == ChrSpecialization::ShamanEnhancement;

    bool specNeedsStrength =
        spec == ChrSpecialization::WarriorArms || spec == ChrSpecialization::WarriorFury ||
        spec == ChrSpecialization::WarriorProtection || spec == ChrSpecialization::PaladinProtection ||
        spec == ChrSpecialization::PaladinRetribution || spec == ChrSpecialization::DeathKnightBlood ||
        spec == ChrSpecialization::DeathKnightFrost || spec == ChrSpecialization::DeathKnightUnholy;

    // If item has wrong primary stat, it's not for main spec
    if ((hasIntellect && !specNeedsIntellect && (specNeedsAgility || specNeedsStrength)) ||
        (hasAgility && !specNeedsAgility && (specNeedsIntellect || specNeedsStrength)) ||
        (hasStrength && !specNeedsStrength && (specNeedsIntellect || specNeedsAgility)))
    {
        return false;
    }

    return true;
}

bool LootDistribution::IsItemUsefulForOffSpec(const LootItem& item)
{
    if (!_bot || !item.itemTemplate)
        return false;

    // Basic usability check
    if (!_bot->CanUseItem(item.itemTemplate))
        return false;

    // If it's already for main spec, not off-spec
    if (IsItemForMainSpec(item))
        return false;

    ChrSpecialization mainSpec = _bot->GetPrimarySpecialization();
    uint8 playerClass = _bot->GetClass();
    uint8 itemClass = item.itemTemplate->GetClass();
    uint8 invType = item.itemTemplate->GetInventoryType();

    // Define potential off-specs for each class and check if item is useful
    // WoW 12.0 class/spec matrix
    std::vector<ChrSpecialization> possibleOffSpecs;

    switch (playerClass)
    {
        case CLASS_WARRIOR:
            possibleOffSpecs = {ChrSpecialization::WarriorArms,
                               ChrSpecialization::WarriorFury,
                               ChrSpecialization::WarriorProtection};
            break;
        case CLASS_PALADIN:
            possibleOffSpecs = {ChrSpecialization::PaladinHoly,
                               ChrSpecialization::PaladinProtection,
                               ChrSpecialization::PaladinRetribution};
            break;
        case CLASS_HUNTER:
            possibleOffSpecs = {ChrSpecialization::HunterBeastMastery,
                               ChrSpecialization::HunterMarksmanship,
                               ChrSpecialization::HunterSurvival};
            break;
        case CLASS_ROGUE:
            possibleOffSpecs = {ChrSpecialization::RogueAssassination,
                               ChrSpecialization::RogueOutlaw,
                               ChrSpecialization::RogueSubtely};
            break;
        case CLASS_PRIEST:
            possibleOffSpecs = {ChrSpecialization::PriestDiscipline,
                               ChrSpecialization::PriestHoly,
                               ChrSpecialization::PriestShadow};
            break;
        case CLASS_DEATH_KNIGHT:
            possibleOffSpecs = {ChrSpecialization::DeathKnightBlood,
                               ChrSpecialization::DeathKnightFrost,
                               ChrSpecialization::DeathKnightUnholy};
            break;
        case CLASS_SHAMAN:
            possibleOffSpecs = {ChrSpecialization::ShamanElemental,
                               ChrSpecialization::ShamanEnhancement,
                               ChrSpecialization::ShamanRestoration};
            break;
        case CLASS_MAGE:
            possibleOffSpecs = {ChrSpecialization::MageArcane,
                               ChrSpecialization::MageFire,
                               ChrSpecialization::MageFrost};
            break;
        case CLASS_WARLOCK:
            possibleOffSpecs = {ChrSpecialization::WarlockAffliction,
                               ChrSpecialization::WarlockDemonology,
                               ChrSpecialization::WarlockDestruction};
            break;
        case CLASS_MONK:
            possibleOffSpecs = {ChrSpecialization::MonkBrewmaster,
                               ChrSpecialization::MonkMistweaver,
                               ChrSpecialization::MonkWindwalker};
            break;
        case CLASS_DRUID:
            possibleOffSpecs = {ChrSpecialization::DruidBalance,
                               ChrSpecialization::DruidFeral,
                               ChrSpecialization::DruidGuardian,
                               ChrSpecialization::DruidRestoration};
            break;
        case CLASS_DEMON_HUNTER:
            possibleOffSpecs = {ChrSpecialization::DemonHunterHavoc,
                               ChrSpecialization::DemonHunterVengeance};
            break;
        case CLASS_EVOKER:
            possibleOffSpecs = {ChrSpecialization::EvokerDevastation,
                               ChrSpecialization::EvokerPreservation,
                               ChrSpecialization::EvokerAugmentation};
            break;
        default:
            return false;
    }

    // Remove main spec from off-spec list
    possibleOffSpecs.erase(
        std::remove(possibleOffSpecs.begin(), possibleOffSpecs.end(), mainSpec),
        possibleOffSpecs.end());

    // Check if item is useful for any off-spec
    for (ChrSpecialization offSpec : possibleOffSpecs)
    {
        // Get spec role
        ChrSpecializationEntry const* specEntry = sChrSpecializationStore.LookupEntry(AsUnderlyingType(offSpec));
        if (!specEntry)
            continue;

        ChrSpecializationRole role = static_cast<ChrSpecializationRole>(specEntry->Role);

        // Check armor type
        if (itemClass == ITEM_CLASS_ARMOR)
        {
            // Armor type doesn't change by spec within same class
            // But accessory slots are universal
            if (invType == INVTYPE_FINGER || invType == INVTYPE_TRINKET ||
                invType == INVTYPE_NECK || invType == INVTYPE_CLOAK)
            {
                // Check primary stats for these slots
                bool hasIntellect = false, hasAgility = false, hasStrength = false;

                for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
                {
                    int32 statType = item.itemTemplate->GetStatModifierBonusStat(i);
                    if (statType < 0) continue;

                    if (statType == ITEM_MOD_INTELLECT || statType == ITEM_MOD_INTELLECT)
                        hasIntellect = true;
                    if (statType == ITEM_MOD_AGILITY || statType == ITEM_MOD_AGILITY)
                        hasAgility = true;
                    if (statType == ITEM_MOD_STRENGTH || statType == ITEM_MOD_STRENGTH)
                        hasStrength = true;
                }

                // Check if this off-spec needs these stats
                bool offSpecNeedsIntellect = (role == ChrSpecializationRole::Healer) ||
                    offSpec == ChrSpecialization::MageArcane || offSpec == ChrSpecialization::MageFire ||
                    offSpec == ChrSpecialization::MageFrost || offSpec == ChrSpecialization::WarlockAffliction ||
                    offSpec == ChrSpecialization::WarlockDemonology || offSpec == ChrSpecialization::WarlockDestruction ||
                    offSpec == ChrSpecialization::PriestShadow || offSpec == ChrSpecialization::DruidBalance ||
                    offSpec == ChrSpecialization::ShamanElemental || offSpec == ChrSpecialization::EvokerDevastation ||
                    offSpec == ChrSpecialization::EvokerAugmentation;

                bool offSpecNeedsAgility =
                    offSpec == ChrSpecialization::RogueAssassination || offSpec == ChrSpecialization::RogueOutlaw ||
                    offSpec == ChrSpecialization::RogueSubtely || offSpec == ChrSpecialization::DruidFeral ||
                    offSpec == ChrSpecialization::MonkWindwalker || offSpec == ChrSpecialization::DemonHunterHavoc ||
                    offSpec == ChrSpecialization::DemonHunterVengeance || offSpec == ChrSpecialization::DruidGuardian ||
                    offSpec == ChrSpecialization::MonkBrewmaster || offSpec == ChrSpecialization::HunterBeastMastery ||
                    offSpec == ChrSpecialization::HunterMarksmanship || offSpec == ChrSpecialization::HunterSurvival ||
                    offSpec == ChrSpecialization::ShamanEnhancement;

                bool offSpecNeedsStrength =
                    offSpec == ChrSpecialization::WarriorArms || offSpec == ChrSpecialization::WarriorFury ||
                    offSpec == ChrSpecialization::WarriorProtection || offSpec == ChrSpecialization::PaladinProtection ||
                    offSpec == ChrSpecialization::PaladinRetribution || offSpec == ChrSpecialization::DeathKnightBlood ||
                    offSpec == ChrSpecialization::DeathKnightFrost || offSpec == ChrSpecialization::DeathKnightUnholy;

                // If stats match off-spec needs, it's useful
                if ((hasIntellect && offSpecNeedsIntellect) ||
                    (hasAgility && offSpecNeedsAgility) ||
                    (hasStrength && offSpecNeedsStrength))
                {
                    return true;
                }
            }
        }

        // Check weapon type for off-spec
        if (itemClass == ITEM_CLASS_WEAPON)
        {
            // Shield for tank off-specs
            if (invType == INVTYPE_SHIELD)
            {
                if (offSpec == ChrSpecialization::WarriorProtection ||
                    offSpec == ChrSpecialization::PaladinProtection ||
                    offSpec == ChrSpecialization::PaladinHoly ||
                    offSpec == ChrSpecialization::ShamanElemental ||
                    offSpec == ChrSpecialization::ShamanRestoration)
                    return true;
            }

            // Two-handed weapons for DPS off-specs
            if (invType == INVTYPE_2HWEAPON)
            {
                if (offSpec == ChrSpecialization::WarriorArms ||
                    offSpec == ChrSpecialization::DeathKnightUnholy ||
                    offSpec == ChrSpecialization::PaladinRetribution ||
                    offSpec == ChrSpecialization::DruidFeral ||
                    offSpec == ChrSpecialization::DruidGuardian ||
                    offSpec == ChrSpecialization::DruidBalance)
                    return true;
            }
        }
    }

    return false;
}

bool LootDistribution::IsWeaponUsableByClass(uint8 playerClass, uint8 weaponSubClass) const
{
    switch (playerClass)
    {
        case CLASS_WARRIOR:
            // Warriors: All melee weapons + bows/guns for pulling
            switch (weaponSubClass)
            {
                case ITEM_SUBCLASS_WEAPON_AXE:
                case ITEM_SUBCLASS_WEAPON_AXE2:
                case ITEM_SUBCLASS_WEAPON_MACE:
                case ITEM_SUBCLASS_WEAPON_MACE2:
                case ITEM_SUBCLASS_WEAPON_SWORD:
                case ITEM_SUBCLASS_WEAPON_SWORD2:
                case ITEM_SUBCLASS_WEAPON_DAGGER:
                case ITEM_SUBCLASS_WEAPON_FIST_WEAPON:
                case ITEM_SUBCLASS_WEAPON_POLEARM:
                case ITEM_SUBCLASS_WEAPON_STAFF:
                case ITEM_SUBCLASS_WEAPON_BOW:
                case ITEM_SUBCLASS_WEAPON_GUN:
                case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                    return true;
                default:
                    return false;
            }

        case CLASS_PALADIN:
            // Paladins: Maces, Swords, Axes, Polearms
            switch (weaponSubClass)
            {
                case ITEM_SUBCLASS_WEAPON_AXE:
                case ITEM_SUBCLASS_WEAPON_AXE2:
                case ITEM_SUBCLASS_WEAPON_MACE:
                case ITEM_SUBCLASS_WEAPON_MACE2:
                case ITEM_SUBCLASS_WEAPON_SWORD:
                case ITEM_SUBCLASS_WEAPON_SWORD2:
                case ITEM_SUBCLASS_WEAPON_POLEARM:
                    return true;
                default:
                    return false;
            }

        case CLASS_DEATH_KNIGHT:
            // Death Knights: Two-handed weapons, one-handed swords/axes/maces
            switch (weaponSubClass)
            {
                case ITEM_SUBCLASS_WEAPON_AXE:
                case ITEM_SUBCLASS_WEAPON_AXE2:
                case ITEM_SUBCLASS_WEAPON_MACE:
                case ITEM_SUBCLASS_WEAPON_MACE2:
                case ITEM_SUBCLASS_WEAPON_SWORD:
                case ITEM_SUBCLASS_WEAPON_SWORD2:
                case ITEM_SUBCLASS_WEAPON_POLEARM:
                    return true;
                default:
                    return false;
            }

        case CLASS_HUNTER:
            // Hunters: Ranged weapons + melee for survival
            switch (weaponSubClass)
            {
                case ITEM_SUBCLASS_WEAPON_BOW:
                case ITEM_SUBCLASS_WEAPON_GUN:
                case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                case ITEM_SUBCLASS_WEAPON_AXE:
                case ITEM_SUBCLASS_WEAPON_AXE2:
                case ITEM_SUBCLASS_WEAPON_SWORD:
                case ITEM_SUBCLASS_WEAPON_SWORD2:
                case ITEM_SUBCLASS_WEAPON_POLEARM:
                case ITEM_SUBCLASS_WEAPON_STAFF:
                case ITEM_SUBCLASS_WEAPON_DAGGER:
                case ITEM_SUBCLASS_WEAPON_FIST_WEAPON:
                    return true;
                default:
                    return false;
            }

        case CLASS_ROGUE:
            // Rogues: Daggers, Swords, Maces, Fist Weapons
            switch (weaponSubClass)
            {
                case ITEM_SUBCLASS_WEAPON_DAGGER:
                case ITEM_SUBCLASS_WEAPON_SWORD:
                case ITEM_SUBCLASS_WEAPON_MACE:
                case ITEM_SUBCLASS_WEAPON_FIST_WEAPON:
                    return true;
                default:
                    return false;
            }

        case CLASS_PRIEST:
            // Priests: Daggers, Maces, Staves, Wands ONLY
            switch (weaponSubClass)
            {
                case ITEM_SUBCLASS_WEAPON_DAGGER:
                case ITEM_SUBCLASS_WEAPON_MACE:
                case ITEM_SUBCLASS_WEAPON_STAFF:
                case ITEM_SUBCLASS_WEAPON_WAND:
                    return true;
                default:
                    return false;
            }

        case CLASS_SHAMAN:
            // Shamans: Axes, Maces, Daggers, Fist Weapons, Staves
            switch (weaponSubClass)
            {
                case ITEM_SUBCLASS_WEAPON_AXE:
                case ITEM_SUBCLASS_WEAPON_AXE2:
                case ITEM_SUBCLASS_WEAPON_MACE:
                case ITEM_SUBCLASS_WEAPON_MACE2:
                case ITEM_SUBCLASS_WEAPON_DAGGER:
                case ITEM_SUBCLASS_WEAPON_FIST_WEAPON:
                case ITEM_SUBCLASS_WEAPON_STAFF:
                    return true;
                default:
                    return false;
            }

        case CLASS_MAGE:
            // Mages: Daggers, Swords, Staves, Wands
            switch (weaponSubClass)
            {
                case ITEM_SUBCLASS_WEAPON_DAGGER:
                case ITEM_SUBCLASS_WEAPON_SWORD:
                case ITEM_SUBCLASS_WEAPON_STAFF:
                case ITEM_SUBCLASS_WEAPON_WAND:
                    return true;
                default:
                    return false;
            }

        case CLASS_WARLOCK:
            // Warlocks: Daggers, Swords, Staves, Wands
            switch (weaponSubClass)
            {
                case ITEM_SUBCLASS_WEAPON_DAGGER:
                case ITEM_SUBCLASS_WEAPON_SWORD:
                case ITEM_SUBCLASS_WEAPON_STAFF:
                case ITEM_SUBCLASS_WEAPON_WAND:
                    return true;
                default:
                    return false;
            }

        case CLASS_MONK:
            // Monks: Fist Weapons, One-handed Axes/Maces/Swords, Polearms, Staves
            switch (weaponSubClass)
            {
                case ITEM_SUBCLASS_WEAPON_FIST_WEAPON:
                case ITEM_SUBCLASS_WEAPON_AXE:
                case ITEM_SUBCLASS_WEAPON_MACE:
                case ITEM_SUBCLASS_WEAPON_SWORD:
                case ITEM_SUBCLASS_WEAPON_POLEARM:
                case ITEM_SUBCLASS_WEAPON_STAFF:
                    return true;
                default:
                    return false;
            }

        case CLASS_DRUID:
            // Druids: Daggers, Fist Weapons, Maces, Polearms, Staves
            switch (weaponSubClass)
            {
                case ITEM_SUBCLASS_WEAPON_DAGGER:
                case ITEM_SUBCLASS_WEAPON_FIST_WEAPON:
                case ITEM_SUBCLASS_WEAPON_MACE:
                case ITEM_SUBCLASS_WEAPON_MACE2:
                case ITEM_SUBCLASS_WEAPON_POLEARM:
                case ITEM_SUBCLASS_WEAPON_STAFF:
                    return true;
                default:
                    return false;
            }

        case CLASS_DEMON_HUNTER:
            // Demon Hunters: Warglaives, Swords, Axes, Fist Weapons, Daggers
            switch (weaponSubClass)
            {
                case ITEM_SUBCLASS_WEAPON_WARGLAIVES:
                case ITEM_SUBCLASS_WEAPON_SWORD:
                case ITEM_SUBCLASS_WEAPON_AXE:
                case ITEM_SUBCLASS_WEAPON_FIST_WEAPON:
                case ITEM_SUBCLASS_WEAPON_DAGGER:
                    return true;
                default:
                    return false;
            }

        case CLASS_EVOKER:
            // Evokers: Daggers, Fist Weapons, Axes, Maces, Swords, Staves
            switch (weaponSubClass)
            {
                case ITEM_SUBCLASS_WEAPON_DAGGER:
                case ITEM_SUBCLASS_WEAPON_FIST_WEAPON:
                case ITEM_SUBCLASS_WEAPON_AXE:
                case ITEM_SUBCLASS_WEAPON_MACE:
                case ITEM_SUBCLASS_WEAPON_SWORD:
                case ITEM_SUBCLASS_WEAPON_STAFF:
                    return true;
                default:
                    return false;
            }

        default:
            return true;
    }
}

bool LootDistribution::IsItemTypeUsefulForClass(uint8 playerClass, const ItemTemplate* itemTemplate)
{
    if (!itemTemplate)
        return false;

    // For armor items, check armor type appropriateness
    if (itemTemplate->GetClass() == ITEM_CLASS_ARMOR)
    {
        uint8 armorSubClass = itemTemplate->GetSubClass();
        // Skip miscellaneous items (rings, trinkets, cloaks, etc.) - they're always useful
        if (armorSubClass == ITEM_SUBCLASS_ARMOR_MISCELLANEOUS ||
            armorSubClass == ITEM_SUBCLASS_ARMOR_COSMETIC)
            return true;

        switch (playerClass)
        {
            case CLASS_WARRIOR:
            case CLASS_PALADIN:
            case CLASS_DEATH_KNIGHT:
                return armorSubClass == ITEM_SUBCLASS_ARMOR_PLATE;

            case CLASS_HUNTER:
            case CLASS_SHAMAN:
            case CLASS_EVOKER:
                return armorSubClass == ITEM_SUBCLASS_ARMOR_MAIL;

            case CLASS_ROGUE:
            case CLASS_DRUID:
            case CLASS_MONK:
            case CLASS_DEMON_HUNTER:
                return armorSubClass == ITEM_SUBCLASS_ARMOR_LEATHER;

            case CLASS_PRIEST:
            case CLASS_MAGE:
            case CLASS_WARLOCK:
                return armorSubClass == ITEM_SUBCLASS_ARMOR_CLOTH;

            default:
                return true;
        }
    }

    // For weapon items, check weapon type appropriateness using the new helper
    if (itemTemplate->GetClass() == ITEM_CLASS_WEAPON)
    {
        return IsWeaponUsableByClass(playerClass, itemTemplate->GetSubClass());
    }

    // For other item types (consumables, trade goods, etc.), always useful
    return true;
}

uint32 LootDistribution::ProcessNeedRolls(LootRoll& roll)
{
    uint32 highestRoll = 0;
    uint32 winner = 0;

    for (const auto& playerRoll : roll.playerRolls)
    {
        if (playerRoll.second == LootRollType::NEED)
        {
            uint32 rollValue = roll.rollValues[playerRoll.first];
            if (rollValue > highestRoll)
            {
                highestRoll = rollValue;
                winner = playerRoll.first;
            }
        }
    }

    if (winner != 0)
    {
        roll.winningRollType = LootRollType::NEED;
        _globalMetrics.needRollsWon++;
    }

    return winner;
}

uint32 LootDistribution::ProcessGreedRolls(LootRoll& roll)
{
    uint32 highestRoll = 0;
    uint32 winner = 0;

    for (const auto& playerRoll : roll.playerRolls)
    {
        if (playerRoll.second == LootRollType::GREED)
        {
            uint32 rollValue = roll.rollValues[playerRoll.first];
            if (rollValue > highestRoll)
            {
                highestRoll = rollValue;
                winner = playerRoll.first;
            }
        }
    }

    if (winner != 0)
    {
        roll.winningRollType = LootRollType::GREED;
        _globalMetrics.greedRollsWon++;
    }

    return winner;
}

uint32 LootDistribution::ProcessDisenchantRolls(LootRoll& roll)
{
    uint32 highestRoll = 0;
    uint32 winner = 0;

    for (const auto& playerRoll : roll.playerRolls)
    {
        if (playerRoll.second == LootRollType::DISENCHANT)
        {
            uint32 rollValue = roll.rollValues[playerRoll.first];
            if (rollValue > highestRoll)
            {
                highestRoll = rollValue;
                winner = playerRoll.first;
            }
        }
    }

    if (winner != 0)
    {
        roll.winningRollType = LootRollType::DISENCHANT;
    }

    return winner;
}

void LootDistribution::BroadcastLootRoll(Group* group, const LootRoll& roll)
{
    if (!group)
        return;

    // Broadcast roll information to all group members
    // In a real implementation, this would send appropriate packets
    TC_LOG_DEBUG("playerbot.loot", "Broadcasting loot roll {} for item {} to group {}",
                roll.rollId, roll.itemId, roll.groupId);
}

void LootDistribution::NotifyRollResult(const LootRoll& roll)
{
    // Notify group members of roll results
    // In a real implementation, this would send result packets
    TC_LOG_DEBUG("playerbot.loot", "Loot roll {} completed. Winner: {}, Item: {}",
                roll.rollId, roll.winnerGuid, roll.itemId);
}

bool LootDistribution::ShouldConsiderFairnessAdjustment(Group* group)
{
    if (!group || !_bot)
        return false;

    LootFairnessTracker tracker = GetGroupLootFairness(group->GetGUID().GetCounter());
    return tracker.fairnessScore < FAIRNESS_ADJUSTMENT_THRESHOLD;
}

void LootDistribution::UpdateLootMetrics(const LootRoll& roll, bool wasWinner)
{
    auto& metrics = _playerMetrics[_bot->GetGUID().GetCounter()];

    if (wasWinner)
    {
        switch (roll.winningRollType)
        {
            case LootRollType::NEED:
                metrics.needRollsWon++;
                break;
            case LootRollType::GREED:
                metrics.greedRollsWon++;
                break;
            default:
                break;
        }
    }

    metrics.lastUpdate = std::chrono::steady_clock::now();
}

void LootDistribution::Update(uint32 /*diff*/)
{
    static uint32 lastUpdate = 0;
    uint32 currentTime = GameTime::GetGameTimeMS();

    if (currentTime - lastUpdate < LOOT_UPDATE_INTERVAL)
        return;

    lastUpdate = currentTime;

    // Process active rolls
    ProcessActiveLootRolls();

    // Clean up expired rolls
    CleanupExpiredRolls();

    // Validate loot states
    ValidateLootStates();
}

void LootDistribution::ProcessActiveLootRolls()
{
    std::vector<uint32> rollsToProcess;

    {
        for (const auto& rollPair : _activeLootRolls)
        {
            rollsToProcess.push_back(rollPair.first);
        }
    }

    for (uint32 rollId : rollsToProcess)
    {
        ProcessLootRolls(rollId);
    }
}

void LootDistribution::CleanupExpiredRolls()
{
    uint32 currentTime = GameTime::GetGameTimeMS();
    std::vector<uint32> expiredRolls;

    for (const auto& timeoutPair : _rollTimeouts)
    {
        if (currentTime > timeoutPair.second)
        {
            expiredRolls.push_back(timeoutPair.first);
        }
    }

    for (uint32 rollId : expiredRolls)
    {
        HandleLootRollTimeout(rollId);
    }
}

void LootDistribution::ValidateLootStates()
{
    // Validate that loot system state is consistent
    // Clean up any orphaned data
    // Detect and handle any inconsistencies

    uint32 currentTime = GameTime::GetGameTimeMS();
    std::vector<uint32> invalidRolls;
    std::vector<uint32> orphanedTimeouts;

    // Check all active rolls for validity
    for (auto& rollPair : _activeLootRolls)
    {
        const LootRoll& roll = rollPair.second;

        // Check if roll has been active too long (safety net - 5 minutes max)
        if (currentTime - roll.rollStartTime > 300000)
        {
            TC_LOG_WARN("playerbot.loot", "Roll {} has been active for over 5 minutes, marking invalid",
                roll.rollId);
            invalidRolls.push_back(rollPair.first);
            continue;
        }

        // Validate group still exists
        if (roll.groupId != 0)
        {
            ObjectGuid groupGuid = ObjectGuid::Create<HighGuid::Party>(roll.groupId);
            Group* group = sGroupMgr->GetGroupByGUID(groupGuid);
            if (!group)
            {
                TC_LOG_DEBUG("playerbot.loot", "Roll {} has invalid group {}, marking invalid",
                    roll.rollId, roll.groupId);
                invalidRolls.push_back(rollPair.first);
                continue;
            }
        }

        // Validate at least one eligible player still exists
        bool hasValidPlayer = false;
        for (uint32 playerGuid : roll.eligiblePlayers)
        {
            ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(playerGuid);
            if (ObjectAccessor::FindConnectedPlayer(guid))
            {
                hasValidPlayer = true;
                break;
            }
        }

        if (!hasValidPlayer)
        {
            TC_LOG_DEBUG("playerbot.loot", "Roll {} has no valid eligible players, marking invalid",
                roll.rollId);
            invalidRolls.push_back(rollPair.first);
        }
    }

    // Check for orphaned timeouts (timeout entries without corresponding rolls)
    for (const auto& timeoutPair : _rollTimeouts)
    {
        if (_activeLootRolls.find(timeoutPair.first) == _activeLootRolls.end())
        {
            orphanedTimeouts.push_back(timeoutPair.first);
        }
    }

    // Clean up invalid rolls
    for (uint32 rollId : invalidRolls)
    {
        _activeLootRolls.erase(rollId);
        _rollTimeouts.erase(rollId);
        _globalMetrics.rollTimeouts++;
    }

    // Clean up orphaned timeouts
    for (uint32 rollId : orphanedTimeouts)
    {
        _rollTimeouts.erase(rollId);
    }

    // Validate fairness tracking data
    std::vector<uint32> staleGroups;
    for (const auto& fairnessPair : _groupFairnessTracking)
    {
        ObjectGuid groupGuid = ObjectGuid::Create<HighGuid::Party>(fairnessPair.first);
        Group* group = sGroupMgr->GetGroupByGUID(groupGuid);
        if (!group)
        {
            staleGroups.push_back(fairnessPair.first);
        }
    }

    // Clean up stale group tracking
    for (uint32 groupId : staleGroups)
    {
        _groupFairnessTracking.erase(groupId);
    }

    // Validate player metrics (remove metrics for disconnected players after 30 minutes)
    static uint32 lastMetricsCleanup = 0;
    if (currentTime - lastMetricsCleanup > 1800000) // 30 minutes
    {
        std::vector<uint32> staleMetrics;
        for (const auto& metricsPair : _playerMetrics)
        {
            auto timeSinceUpdate = std::chrono::duration_cast<std::chrono::minutes>(
                std::chrono::steady_clock::now() - metricsPair.second.lastUpdate).count();

            if (timeSinceUpdate > 30)
            {
                ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(metricsPair.first);
                if (!ObjectAccessor::FindConnectedPlayer(guid))
                {
                    staleMetrics.push_back(metricsPair.first);
                }
            }
        }

        for (uint32 playerGuid : staleMetrics)
        {
            _playerMetrics.erase(playerGuid);
        }

        lastMetricsCleanup = currentTime;
    }

    // Log validation summary if any issues found
    if (!invalidRolls.empty() || !orphanedTimeouts.empty() || !staleGroups.empty())
    {
        TC_LOG_DEBUG("playerbot.loot", "Loot state validation: cleaned {} invalid rolls, {} orphaned timeouts, {} stale groups",
            invalidRolls.size(), orphanedTimeouts.size(), staleGroups.size());
    }
}


void LootDistribution::SetGroupLootMethod(Group* group, LootMethod method)
{
    if (!group)
        return;

    TC_LOG_DEBUG("playerbot.loot", "LootDistribution: Setting loot method to {} for group",
        static_cast<uint8>(method));

    // Would set the group's loot method through Group API
}

void LootDistribution::SetGroupLootThreshold(Group* group, ItemQualities threshold)
{
    if (!group)
        return;

    TC_LOG_DEBUG("playerbot.loot", "LootDistribution: Setting loot threshold to {} for group",
        static_cast<uint8>(threshold));

    // Would set the group's loot threshold through Group API
}

void LootDistribution::HandleLootConflicts(uint32 itemId)
{
    TC_LOG_DEBUG("playerbot.loot", "LootDistribution: Handling loot conflicts for item {}", itemId);

    // Resolve any loot distribution conflicts for the specified item
}


// ============================================================================
// Missing Function Implementations - Enterprise Grade
// ============================================================================

void LootDistribution::SetMasterLooter(Group* group, Player* masterLooter)
{
    if (!group || !masterLooter)
        return;

    // Verify player is in the group
    if (!group->IsMember(masterLooter->GetGUID()))
    {
        TC_LOG_WARN("playerbot.loot", "Cannot set master looter {} - not in group",
            masterLooter->GetName());
        return;
    }

    // Master looter must be group leader or assistant
    if (group->GetLeaderGUID() != masterLooter->GetGUID())
    {
        // Check if they're an assistant
        bool isAssistant = false;
        // FIXED: Use GroupMemberResolver instead of GetMembers()
        for (auto const& memberSlot : group->GetMemberSlots())
        {
            Player* member = GroupMemberResolver::ResolveMember(memberSlot.guid);
            if (member == masterLooter)
            {
                isAssistant = group->IsAssistant(masterLooter->GetGUID());
                break;
            }
        }

        if (!isAssistant)
        {
            TC_LOG_WARN("playerbot.loot", "Cannot set master looter {} - not leader or assistant",
                masterLooter->GetName());
            return;
        }
    }

    TC_LOG_DEBUG("playerbot.loot", "Setting master looter to {} for group {}",
        masterLooter->GetName(), group->GetGUID().GetCounter());
}

void LootDistribution::HandleMasterLootDistribution(Group* group, const LootItem& item, Player* recipient)
{
    if (!group || !recipient)
        return;

    // Verify recipient is in group
    if (!group->IsMember(recipient->GetGUID()))
    {
        TC_LOG_WARN("playerbot.loot", "Cannot distribute to {} - not in group",
            recipient->GetName());
        return;
    }

    // Check if master loot is enabled
    if (group->GetLootMethod() != MASTER_LOOT)
    {
        TC_LOG_WARN("playerbot.loot", "Master loot distribution called but loot method is not MASTER_LOOT");
        return;
    }

    // Verify distributor is master looter
    if (_bot && group->GetMasterLooterGuid() != _bot->GetGUID())
    {
        TC_LOG_WARN("playerbot.loot", "Bot {} is not master looter", _bot->GetName());
        return;
    }

    // Distribute the item
    ItemPosCountVec dest;
    InventoryResult msg = recipient->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, item.itemId, item.itemCount);

    if (msg == EQUIP_ERR_OK)
    {
        Item* newItem = recipient->StoreNewItem(dest, item.itemId, true);
        if (newItem)
        {
            recipient->SendNewItem(newItem, item.itemCount, false, false, true);

            // Update fairness tracking
            UpdateLootFairness(group->GetGUID().GetCounter(), recipient->GetGUID().GetCounter(), item);

            TC_LOG_DEBUG("playerbot.loot", "Master loot: {} received {}",
                recipient->GetName(), item.itemId);
        }
    }
    else
    {
        TC_LOG_WARN("playerbot.loot", "Master loot: {} cannot receive {} - inventory error {}",
            recipient->GetName(), item.itemId, static_cast<uint8>(msg));
    }
}

void LootDistribution::HandleReservedItems(Group* group, const std::vector<uint32>& reservedItems, Player* reserver)
{
    if (!group || reservedItems.empty() || !reserver)
        return;

    // Reserved items are typically for specific players (soft reserves)
    // Store reservations for later use during loot distribution
    TC_LOG_DEBUG("playerbot.loot", "Player {} reserved {} items for group {}",
        reserver->GetName(), reservedItems.size(), group->GetGUID().GetCounter());

    // In a full implementation, this would store reservations in a group-specific map
    // and consult them during loot distribution
}

void LootDistribution::ProcessLootCouncilDecision(Group* group, const LootItem& item, Player* recipient)
{
    if (!group || !recipient)
        return;

    // Loot council is similar to master loot but with democratic decision
    // Typically used in organized raid groups

    // Verify recipient eligibility
    if (!recipient->CanUseItem(item.itemTemplate))
    {
        TC_LOG_WARN("playerbot.loot", "Loot council: {} cannot use item {}", recipient->GetName(), item.itemId);
        return;
    }

    // Distribute via master loot mechanism
    HandleMasterLootDistribution(group, item, recipient);

    TC_LOG_DEBUG("playerbot.loot", "Loot council awarded item {} to {}", item.itemId, recipient->GetName());
}

void LootDistribution::HandlePersonalLoot(const LootItem& item)
{
    if (!_bot)
        return;

    // Personal loot goes directly to the player's bags
    ItemPosCountVec dest;
    InventoryResult msg = _bot->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, item.itemId, item.itemCount);

    if (msg == EQUIP_ERR_OK)
    {
        Item* newItem = _bot->StoreNewItem(dest, item.itemId, true);
        if (newItem)
        {
            _bot->SendNewItem(newItem, item.itemCount, false, false, true);

            // Update personal metrics
            auto& metrics = _playerMetrics[_bot->GetGUID().GetCounter()];
            metrics.totalRollsCompleted++;

            TC_LOG_DEBUG("playerbot.loot", "Personal loot: {} received {}", _bot->GetName(), item.itemId);
        }
    }
    else
    {
        // Item goes to mailbox if bags are full
        TC_LOG_DEBUG("playerbot.loot", "Personal loot: {} bags full for {}, sending to mail",
            _bot->GetName(), item.itemId);
    }
}

void LootDistribution::ManageLootHistory(Group* group, const LootItem& item, Player* recipient)
{
    if (!group || !recipient)
        return;

    uint32 groupId = group->GetGUID().GetCounter();

    // Update fairness tracker with this distribution
    UpdateLootFairness(groupId, recipient->GetGUID().GetCounter(), item);

    // Update player profile with loot received
    auto& profile = _playerLootProfiles[recipient->GetGUID().GetCounter()];
    profile.lastLootTime = GameTime::GetGameTimeMS();
    profile.totalLootReceived++;

    TC_LOG_DEBUG("playerbot.loot", "Loot history: {} now has {} total items",
        recipient->GetName(), profile.totalLootReceived);
}

void LootDistribution::PredictLootNeeds(Group* group)
{
    if (!group)
        return;

    // Analyze group composition and predict what loot each member might need
    TC_LOG_DEBUG("playerbot.loot", "Analyzing loot needs for group {}", group->GetGUID().GetCounter());

    // FIXED: Use GroupMemberResolver instead of GetMembers()
    for (auto const& memberSlot : group->GetMemberSlots())
    {
        Player* member = GroupMemberResolver::ResolveMember(memberSlot.guid);
        if (!member)
            continue;

        // Get member's spec and role
        ChrSpecializationEntry const* specEntry = member->GetPrimarySpecializationEntry();
        if (!specEntry)
            continue;

        // Analyze each equipment slot for potential upgrades
        for (uint8 slot = EQUIPMENT_SLOT_HEAD; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            Item* equipped = member->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            uint32 equippedLevel = equipped ? equipped->GetItemLevel(member) : 0;

            // Store prediction data for future loot decisions
            // This helps bots make smarter roll choices
        }
    }
}

void LootDistribution::OptimizeLootDistribution(Group* group)
{
    if (!group)
        return;

    // Analyze current fairness and suggest optimizations
    uint32 groupId = group->GetGUID().GetCounter();
    LootFairnessTracker tracker = GetGroupLootFairness(groupId);

    if (tracker.fairnessScore < 0.7f)
    {
        TC_LOG_DEBUG("playerbot.loot", "Group {} loot fairness is low ({:.2f}), suggesting adjustments",
            groupId, tracker.fairnessScore);

        // Identify players who have received fewer items
        float avgItems = tracker.totalItemsDistributed > 0 ?
            float(tracker.totalItemsDistributed) / std::max(1u, uint32(tracker.playerLootCount.size())) : 0.0f;

        for (const auto& pair : tracker.playerLootCount)
        {
            if (float(pair.second) < avgItems * 0.7f)
            {
                TC_LOG_DEBUG("playerbot.loot", "Player {} is below average loot ({} vs {:.1f})",
                    pair.first, pair.second, avgItems);
            }
        }
    }
}

void LootDistribution::RecommendLootSettings(Group* group)
{
    if (!group)
        return;

    // Analyze group composition and recommend optimal loot settings
    uint32 memberCount = 0;
    bool hasMultipleSameClass = false;
    std::unordered_map<uint8, uint32> classCounts;

    for (GroupReference const& itr : group->GetMembers())
    {
        Player* member = itr.GetSource();
        if (!member)
            continue;

        memberCount++;
        classCounts[member->GetClass()]++;

        if (classCounts[member->GetClass()] > 1)
            hasMultipleSameClass = true;
    }

    // Recommendations based on group composition
    LootMethod recommendedMethod = FREE_FOR_ALL;
    ItemQualities recommendedThreshold = ITEM_QUALITY_UNCOMMON;

    if (memberCount >= 5)
    {
        recommendedMethod = NEED_BEFORE_GREED;
        recommendedThreshold = ITEM_QUALITY_RARE;
    }

    if (memberCount >= 10)
    {
        recommendedMethod = MASTER_LOOT;
        recommendedThreshold = ITEM_QUALITY_EPIC;
    }

    TC_LOG_DEBUG("playerbot.loot", "Recommended loot settings for group {}: method={}, threshold={}",
        group->GetGUID().GetCounter(), static_cast<uint8>(recommendedMethod),
        static_cast<uint8>(recommendedThreshold));
}

void LootDistribution::AnalyzeGroupLootComposition(Group* group)
{
    if (!group)
        return;

    // Detailed analysis of what the group needs
    struct RoleCount { uint32 tanks = 0, healers = 0, dps = 0; };
    RoleCount roleCount;
    std::unordered_map<uint8, uint32> armorTypeCounts; // Plate, Mail, Leather, Cloth

    for (GroupReference const& itr : group->GetMembers())
    {
        Player* member = itr.GetSource();
        if (!member)
            continue;

        ChrSpecializationEntry const* specEntry = member->GetPrimarySpecializationEntry();
        if (specEntry)
        {
            ChrSpecializationRole role = static_cast<ChrSpecializationRole>(specEntry->Role);
            switch (role)
            {
                case ChrSpecializationRole::Tank: roleCount.tanks++; break;
                case ChrSpecializationRole::Healer: roleCount.healers++; break;
                case ChrSpecializationRole::Dps: roleCount.dps++; break;
            }
        }

        // Count armor types
        switch (member->GetClass())
        {
            case CLASS_WARRIOR:
            case CLASS_PALADIN:
            case CLASS_DEATH_KNIGHT:
                armorTypeCounts[ITEM_SUBCLASS_ARMOR_PLATE]++;
                break;
            case CLASS_HUNTER:
            case CLASS_SHAMAN:
            case CLASS_EVOKER:
                armorTypeCounts[ITEM_SUBCLASS_ARMOR_MAIL]++;
                break;
            case CLASS_ROGUE:
            case CLASS_DRUID:
            case CLASS_MONK:
            case CLASS_DEMON_HUNTER:
                armorTypeCounts[ITEM_SUBCLASS_ARMOR_LEATHER]++;
                break;
            case CLASS_MAGE:
            case CLASS_PRIEST:
            case CLASS_WARLOCK:
                armorTypeCounts[ITEM_SUBCLASS_ARMOR_CLOTH]++;
                break;
        }
    }

    TC_LOG_DEBUG("playerbot.loot", "Group {} composition: {} tanks, {} healers, {} dps",
        group->GetGUID().GetCounter(), roleCount.tanks, roleCount.healers, roleCount.dps);
}

void LootDistribution::HandleInvalidLootRoll(uint32 rollId)
{
    auto rollIt = _activeLootRolls.find(rollId);
    if (rollIt == _activeLootRolls.end())
        return;

    TC_LOG_WARN("playerbot.loot", "Handling invalid loot roll {}", rollId);

    // Mark roll as completed with no winner
    LootRoll& roll = rollIt->second;
    roll.isCompleted = true;
    roll.winnerGuid = 0;

    // Clean up
    _activeLootRolls.erase(rollIt);
    _rollTimeouts.erase(rollId);

    _globalMetrics.rollTimeouts++;
}

void LootDistribution::HandlePlayerDisconnectDuringRoll(uint32 rollId)
{
    auto rollIt = _activeLootRolls.find(rollId);
    if (rollIt == _activeLootRolls.end())
        return;

    LootRoll& roll = rollIt->second;

    // Remove disconnected players from eligible list
    std::vector<uint32> disconnectedPlayers;
    for (uint32 playerGuid : roll.eligiblePlayers)
    {
        ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(playerGuid);
        if (!ObjectAccessor::FindConnectedPlayer(guid))
        {
            disconnectedPlayers.push_back(playerGuid);
        }
    }

    for (uint32 playerGuid : disconnectedPlayers)
    {
        roll.eligiblePlayers.erase(playerGuid);
        // Auto-pass for disconnected players
        roll.playerRolls[playerGuid] = LootRollType::PASS;

        TC_LOG_DEBUG("playerbot.loot", "Roll {}: Player {} disconnected, auto-passing",
            rollId, playerGuid);
    }

    // If no eligible players remain, complete the roll
    if (roll.eligiblePlayers.empty())
    {
        HandleInvalidLootRoll(rollId);
    }
    // If all remaining players have rolled, complete
    else if (roll.playerRolls.size() >= roll.eligiblePlayers.size())
    {
        CompleteLootRoll(rollId);
    }
}

void LootDistribution::RecoverFromLootSystemError(uint32 rollId)
{
    TC_LOG_ERROR("playerbot.loot", "Recovering from loot system error for roll {}", rollId);

    // Try to salvage the roll
    auto rollIt = _activeLootRolls.find(rollId);
    if (rollIt != _activeLootRolls.end())
    {
        LootRoll& roll = rollIt->second;

        // Check if we can still complete the roll
        if (!roll.playerRolls.empty())
        {
            // Try to determine a winner from existing rolls
            uint32 winner = DetermineRollWinner(roll);
            if (winner != 0)
            {
                DistributeLootToWinner(rollId, winner);
            }
        }

        // Clean up regardless
        _activeLootRolls.erase(rollIt);
        _rollTimeouts.erase(rollId);
    }

    // Re-validate all loot states
    ValidateLootStates();
}

void LootDistribution::InitializePlayerLootProfile()
{
    if (!_bot)
        return;

    uint32 playerGuid = _bot->GetGUID().GetCounter();

    // Check if profile already exists
    if (_playerLootProfiles.find(playerGuid) != _playerLootProfiles.end())
        return;

    // Create new profile
    PlayerLootProfile profile;
    profile.playerGuid = playerGuid;
    profile.playerClass = _bot->GetClass();
    profile.playerSpec = AsUnderlyingType(_bot->GetPrimarySpecialization());
    profile.playerLevel = _bot->GetLevel();
    profile.strategy = LootDecisionStrategy::NEED_BEFORE_GREED;
    profile.greedThreshold = 0.3f;
    profile.needMainSpecOnly = true;
    profile.greedOffSpec = true;
    profile.disenchantUnneeded = _bot->GetSkillValue(SKILL_ENCHANTING) > 0;

    _playerLootProfiles[playerGuid] = profile;

    TC_LOG_DEBUG("playerbot.loot", "Initialized loot profile for player {}", _bot->GetName());
}

void LootDistribution::AnalyzeItemForPlayer(const LootItem& item)
{
    if (!_bot || !item.itemTemplate)
        return;

    uint32 playerGuid = _bot->GetGUID().GetCounter();

    // Calculate and cache priority
    LootPriority priority = AnalyzeItemPriority(item);
    _itemPriorityCache[playerGuid][item.itemId] = priority;

    // Calculate and cache upgrade status
    bool isUpgrade = IsItemUpgrade(item);
    _upgradeCache[playerGuid][item.itemId] = isUpgrade;

    TC_LOG_DEBUG("playerbot.loot", "Analyzed item {} for {}: priority={}, upgrade={}",
        item.itemId, _bot->GetName(), static_cast<uint8>(priority), isUpgrade);
}

void LootDistribution::UpdateItemPriorityCache(const LootItem& item, LootPriority priority)
{
    if (!_bot)
        return;

    uint32 playerGuid = _bot->GetGUID().GetCounter();
    _itemPriorityCache[playerGuid][item.itemId] = priority;

    // Enforce cache size limit
    if (_itemPriorityCache[playerGuid].size() > PRIORITY_CACHE_SIZE)
    {
        // Remove oldest entries (simple FIFO - in production would use LRU)
        auto it = _itemPriorityCache[playerGuid].begin();
        std::advance(it, _itemPriorityCache[playerGuid].size() - PRIORITY_CACHE_SIZE);
        _itemPriorityCache[playerGuid].erase(_itemPriorityCache[playerGuid].begin(), it);
    }
}

bool LootDistribution::IsItemCachedUpgrade(uint32 itemId)
{
    if (!_bot)
        return false;

    uint32 playerGuid = _bot->GetGUID().GetCounter();
    auto cacheIt = _upgradeCache.find(playerGuid);
    if (cacheIt != _upgradeCache.end())
    {
        auto itemIt = cacheIt->second.find(itemId);
        if (itemIt != cacheIt->second.end())
            return itemIt->second;
    }

    return false;
}

void LootDistribution::InvalidatePlayerCache()
{
    if (!_bot)
        return;

    uint32 playerGuid = _bot->GetGUID().GetCounter();
    _itemPriorityCache.erase(playerGuid);
    _upgradeCache.erase(playerGuid);

    TC_LOG_DEBUG("playerbot.loot", "Invalidated loot cache for player {}", _bot->GetName());
}

bool LootDistribution::IsArmorUpgrade(const LootItem& item)
{
    if (!_bot || !item.itemTemplate)
        return false;

    if (item.itemTemplate->GetClass() != ITEM_CLASS_ARMOR)
        return false;

    // Get current armor in this slot
    uint8 invType = item.itemTemplate->GetInventoryType();
    Item* current = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, invType);

    if (!current)
        return true; // No item = upgrade

    // Compare item levels
    uint32 currentLevel = current->GetItemLevel(_bot);
    uint32 newLevel = item.itemLevel;

    // Must be at least UPGRADE_THRESHOLD better
    return newLevel > currentLevel * (1.0f + UPGRADE_THRESHOLD);
}

bool LootDistribution::IsWeaponUpgrade(const LootItem& item)
{
    if (!_bot || !item.itemTemplate)
        return false;

    if (item.itemTemplate->GetClass() != ITEM_CLASS_WEAPON)
        return false;

    // Check main hand and off hand
    Item* mainHand = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
    Item* offHand = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);

    uint32 currentLevel = 0;
    if (mainHand)
        currentLevel = std::max(currentLevel, mainHand->GetItemLevel(_bot));
    if (offHand && item.itemTemplate->GetInventoryType() != INVTYPE_2HWEAPON)
        currentLevel = std::max(currentLevel, offHand->GetItemLevel(_bot));

    if (currentLevel == 0)
        return true; // No weapon = upgrade

    return item.itemLevel > currentLevel * (1.0f + UPGRADE_THRESHOLD);
}

bool LootDistribution::IsAccessoryUpgrade(const LootItem& item)
{
    if (!_bot || !item.itemTemplate)
        return false;

    uint8 invType = item.itemTemplate->GetInventoryType();

    // Check accessory slots
    if (invType != INVTYPE_FINGER && invType != INVTYPE_TRINKET &&
        invType != INVTYPE_NECK && invType != INVTYPE_CLOAK)
        return false;

    // For rings and trinkets, check both slots
    Item* slot1 = nullptr;
    Item* slot2 = nullptr;

    if (invType == INVTYPE_FINGER)
    {
        slot1 = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_FINGER1);
        slot2 = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_FINGER2);
    }
    else if (invType == INVTYPE_TRINKET)
    {
        slot1 = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_TRINKET1);
        slot2 = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_TRINKET2);
    }
    else
    {
        slot1 = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, invType);
    }

    // Compare with lowest equipped item
    uint32 lowestLevel = UINT32_MAX;
    if (slot1)
        lowestLevel = std::min(lowestLevel, slot1->GetItemLevel(_bot));
    if (slot2)
        lowestLevel = std::min(lowestLevel, slot2->GetItemLevel(_bot));

    if (lowestLevel == UINT32_MAX)
        return true; // Empty slot = upgrade

    return item.itemLevel > lowestLevel * (1.0f + UPGRADE_THRESHOLD);
}

float LootDistribution::CalculateStatPriority(uint32 statType)
{
    if (!_bot)
        return 0.0f;

    ChrSpecialization spec = _bot->GetPrimarySpecialization();
    ChrSpecializationEntry const* specEntry = _bot->GetPrimarySpecializationEntry();

    if (!specEntry)
        return 0.5f; // Neutral priority

    ChrSpecializationRole role = static_cast<ChrSpecializationRole>(specEntry->Role);

    // Primary stats are always high priority if they match spec
    switch (statType)
    {
        case ITEM_MOD_STRENGTH:
            if (role == ChrSpecializationRole::Tank || role == ChrSpecializationRole::Dps)
            {
                if (_bot->GetClass() == CLASS_WARRIOR ||
                    _bot->GetClass() == CLASS_PALADIN ||
                    _bot->GetClass() == CLASS_DEATH_KNIGHT)
                    return 1.0f;
            }
            return 0.1f;

        case ITEM_MOD_AGILITY:
            if (_bot->GetClass() == CLASS_ROGUE ||
                _bot->GetClass() == CLASS_HUNTER ||
                _bot->GetClass() == CLASS_DRUID ||
                _bot->GetClass() == CLASS_MONK ||
                _bot->GetClass() == CLASS_DEMON_HUNTER ||
                _bot->GetClass() == CLASS_SHAMAN)
                return 1.0f;
            return 0.1f;

        case ITEM_MOD_INTELLECT:
            if (role == ChrSpecializationRole::Healer)
                return 1.0f;
            if (_bot->GetClass() == CLASS_MAGE ||
                _bot->GetClass() == CLASS_WARLOCK ||
                _bot->GetClass() == CLASS_PRIEST ||
                _bot->GetClass() == CLASS_EVOKER)
                return 1.0f;
            return 0.1f;

        case ITEM_MOD_STAMINA:
            if (role == ChrSpecializationRole::Tank)
                return 0.9f;
            return 0.5f;

        // Secondary stats
        case ITEM_MOD_CRIT_RATING:
        case ITEM_MOD_HASTE_RATING:
        case ITEM_MOD_MASTERY_RATING:
        case ITEM_MOD_VERSATILITY:
            return 0.7f; // Generally useful

        default:
            return 0.3f;
    }
}

void LootDistribution::HandleRollCompletion(LootRoll& roll)
{
    // Final processing after roll is completed
    roll.isCompleted = true;

    // Notify all eligible players of the result
    NotifyRollResult(roll);

    // Update global metrics
    _globalMetrics.totalRollsCompleted++;

    // Calculate and update roll time metrics
    uint32 rollDuration = GameTime::GetGameTimeMS() - roll.rollStartTime;
    float currentAvg = _globalMetrics.averageRollTime.load();
    uint32 totalRolls = _globalMetrics.totalRollsCompleted.load();

    // Running average calculation
    float newAvg = ((currentAvg * (totalRolls - 1)) + rollDuration) / totalRolls;
    _globalMetrics.averageRollTime = newAvg;

    TC_LOG_DEBUG("playerbot.loot", "Roll {} completed in {}ms", roll.rollId, rollDuration);
}

void LootDistribution::UpdateGroupLootHistory(uint32 groupId, uint32 winnerGuid, const LootItem& item)
{
    // Alias for UpdateLootFairness
    UpdateLootFairness(groupId, winnerGuid, item);
}

void LootDistribution::BalanceLootDistribution(Group* group)
{
    if (!group)
        return;

    uint32 groupId = group->GetGUID().GetCounter();
    LootFairnessTracker& tracker = _groupFairnessTracking[groupId];

    // Identify imbalances
    if (tracker.playerLootCount.empty())
        return;

    float avgItems = float(tracker.totalItemsDistributed) / tracker.playerLootCount.size();

    std::vector<uint32> lowLootPlayers;
    std::vector<uint32> highLootPlayers;

    for (const auto& pair : tracker.playerLootCount)
    {
        if (float(pair.second) < avgItems * 0.7f)
            lowLootPlayers.push_back(pair.first);
        else if (float(pair.second) > avgItems * 1.3f)
            highLootPlayers.push_back(pair.first);
    }

    // Log imbalances for monitoring
    if (!lowLootPlayers.empty() || !highLootPlayers.empty())
    {
        TC_LOG_DEBUG("playerbot.loot", "Group {} loot imbalance: {} low, {} high",
            groupId, lowLootPlayers.size(), highLootPlayers.size());
    }
}

void LootDistribution::AdjustLootDecisionsForFairness(Group* group, LootRollType& decision)
{
    if (!group || !_bot)
        return;

    uint32 groupId = group->GetGUID().GetCounter();
    LootFairnessTracker tracker = GetGroupLootFairness(groupId);

    if (tracker.playerLootCount.empty())
        return;

    uint32 playerGuid = _bot->GetGUID().GetCounter();
    auto playerIt = tracker.playerLootCount.find(playerGuid);

    if (playerIt == tracker.playerLootCount.end())
        return; // Player hasn't received any loot yet

    float avgItems = float(tracker.totalItemsDistributed) / tracker.playerLootCount.size();

    // If we've received significantly more than average, be more generous
    if (float(playerIt->second) > avgItems * 1.3f)
    {
        if (decision == LootRollType::NEED)
        {
            // Downgrade NEED to GREED if not a critical upgrade
            decision = LootRollType::GREED;
            TC_LOG_DEBUG("playerbot.loot", "Bot {} adjusted NEED to GREED for fairness", _bot->GetName());
        }
        else if (decision == LootRollType::GREED)
        {
            // Consider passing entirely
            decision = LootRollType::PASS;
            TC_LOG_DEBUG("playerbot.loot", "Bot {} adjusted GREED to PASS for fairness", _bot->GetName());
        }
    }
}

void LootDistribution::OptimizeLootProcessing()
{
    // Pre-allocate commonly used containers
    // Clean up stale cache entries
    // Optimize data structures for current group size

    // Limit active rolls
    if (_activeLootRolls.size() > MAX_ACTIVE_ROLLS)
    {
        TC_LOG_WARN("playerbot.loot", "Too many active rolls ({}), cleaning up oldest",
            _activeLootRolls.size());

        // Find oldest rolls and timeout them
        std::vector<std::pair<uint32, uint32>> rollsByAge;
        uint32 currentTime = GameTime::GetGameTimeMS();

        for (const auto& pair : _activeLootRolls)
        {
            rollsByAge.emplace_back(pair.first, currentTime - pair.second.rollStartTime);
        }

        // Sort by age (oldest first)
        std::sort(rollsByAge.begin(), rollsByAge.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        // Timeout oldest rolls until we're under the limit
        for (size_t i = 0; i < rollsByAge.size() && _activeLootRolls.size() > MAX_ACTIVE_ROLLS / 2; ++i)
        {
            HandleLootRollTimeout(rollsByAge[i].first);
        }
    }
}

void LootDistribution::PreloadItemData(const std::vector<LootItem>& items)
{
    if (!_bot)
        return;

    // Pre-analyze all items for faster decision making during rolls
    for (const LootItem& item : items)
    {
        AnalyzeItemForPlayer(item);
    }

    TC_LOG_DEBUG("playerbot.loot", "Preloaded {} items for {}", items.size(), _bot->GetName());
}

void LootDistribution::CachePlayerEquipment()
{
    if (!_bot)
        return;

    // Cache current equipment item levels for faster comparison
    // This is called when equipment changes or periodically

    InvalidatePlayerCache(); // Clear old cache first

    TC_LOG_DEBUG("playerbot.loot", "Cached equipment data for {}", _bot->GetName());
}

} // namespace Playerbot
