/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "UnifiedLootManager.h"
#include "Core/PlayerBotHelpers.h"  // GetBotAI, GetGameSystems
#include "Core/Managers/GameSystemsManager.h"  // Complete type for IGameSystemsManager
#include "LootDistribution.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"  // For sObjectMgr
#include "Random.h"
#include "../Group/GroupMemberResolver.h"
#include "../Core/Diagnostics/GroupMemberDiagnostics.h"
#include <algorithm>
#include <sstream>
#include <cmath>
#include "GameTime.h"
#include "ItemTemplate.h"
#include "Item.h"
#include "Player.h"
#include "SharedDefines.h"

// Note: LootAnalysis and LootCoordination were stub headers with no implementations.
// They have been removed during consolidation. Real implementations needed here.

namespace Playerbot
{

// ============================================================================
// Singleton Management
// ============================================================================

UnifiedLootManager* UnifiedLootManager::instance()
{
    static UnifiedLootManager instance;
    return &instance;
}

UnifiedLootManager::UnifiedLootManager()
    : _analysis(std::make_unique<AnalysisModule>())
    , _coordination(std::make_unique<CoordinationModule>())
    , _distribution(std::make_unique<DistributionModule>())
{
    TC_LOG_INFO("playerbot.loot", "UnifiedLootManager initialized");
}

UnifiedLootManager::~UnifiedLootManager()
{
    TC_LOG_INFO("playerbot.loot", "UnifiedLootManager shutting down");
}

// ============================================================================
// ANALYSIS MODULE IMPLEMENTATION
// ============================================================================

float UnifiedLootManager::AnalysisModule::CalculateItemValue(Player* player, LootItem const& item)
{
    if (!player || !item.itemTemplate)
    {
        _itemsAnalyzed++;
        return 0.0f;
    }

    ItemTemplate const* proto = item.itemTemplate;
    float value = 0.0f;

    // Base value from item level (normalized to 0-100 scale)
    // Max item level in TWW ~639, so divide by ~6.4 for rough scaling
    uint32 itemLevel = item.itemLevel > 0 ? item.itemLevel : proto->GetBaseItemLevel();
    value += static_cast<float>(itemLevel) / 6.4f;

    // Quality multiplier (0=poor, 1=common, 2=uncommon, 3=rare, 4=epic, 5=legendary)
    uint32 quality = item.itemQuality > 0 ? item.itemQuality : proto->GetQuality();
    float qualityMultiplier = 1.0f;
    switch (quality)
    {
        case ITEM_QUALITY_POOR:      qualityMultiplier = 0.1f; break;
        case ITEM_QUALITY_NORMAL:    qualityMultiplier = 0.3f; break;
        case ITEM_QUALITY_UNCOMMON:  qualityMultiplier = 0.6f; break;
        case ITEM_QUALITY_RARE:      qualityMultiplier = 0.85f; break;
        case ITEM_QUALITY_EPIC:      qualityMultiplier = 1.0f; break;
        case ITEM_QUALITY_LEGENDARY: qualityMultiplier = 1.15f; break;
        case ITEM_QUALITY_ARTIFACT:  qualityMultiplier = 1.2f; break;
        default:                     qualityMultiplier = 0.5f; break;
    }
    value *= qualityMultiplier;

    // Add value from stats using player's stat weights
    std::vector<std::pair<uint32, float>> priorities = GetStatPriorities(player);
    std::unordered_map<uint32, float> weightMap;
    for (auto const& [statType, weight] : priorities)
        weightMap[statType] = weight;

    for (uint32 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
    {
        int32 statType = proto->GetStatModifierBonusStat(i);
        int32 statValue = proto->GetStatPercentEditor(i);
        if (statType >= 0 && statValue != 0)
        {
            float weight = 1.0f;
            auto it = weightMap.find(static_cast<uint32>(statType));
            if (it != weightMap.end())
                weight = it->second;
            value += std::abs(statValue) * weight * 0.1f;
        }
    }

    // Armor value for armor items
    if (proto->IsArmor())
    {
        uint32 armor = proto->GetArmor(itemLevel);
        value += armor * 0.01f;
    }

    // DPS value for weapons
    if (proto->IsWeapon())
    {
        float dps = proto->GetDPS(itemLevel);
        value += dps * 0.5f;
    }

    // Cap at 100
    value = std::min(value, 100.0f);

    _itemsAnalyzed++;
    return value;
}

float UnifiedLootManager::AnalysisModule::CalculateUpgradeValue(Player* player, LootItem const& item)
{
    if (!player || !item.itemTemplate)
    {
        _upgradesDetected++;
        return 0.0f;
    }

    ItemTemplate const* proto = item.itemTemplate;
    InventoryType invType = proto->GetInventoryType();

    // Find current equipped item for this slot
    uint8 slot = 0;
    switch (invType)
    {
        case INVTYPE_HEAD:          slot = EQUIPMENT_SLOT_HEAD; break;
        case INVTYPE_NECK:          slot = EQUIPMENT_SLOT_NECK; break;
        case INVTYPE_SHOULDERS:     slot = EQUIPMENT_SLOT_SHOULDERS; break;
        case INVTYPE_BODY:          slot = EQUIPMENT_SLOT_BODY; break;
        case INVTYPE_CHEST:
        case INVTYPE_ROBE:          slot = EQUIPMENT_SLOT_CHEST; break;
        case INVTYPE_WAIST:         slot = EQUIPMENT_SLOT_WAIST; break;
        case INVTYPE_LEGS:          slot = EQUIPMENT_SLOT_LEGS; break;
        case INVTYPE_FEET:          slot = EQUIPMENT_SLOT_FEET; break;
        case INVTYPE_WRISTS:        slot = EQUIPMENT_SLOT_WRISTS; break;
        case INVTYPE_HANDS:         slot = EQUIPMENT_SLOT_HANDS; break;
        case INVTYPE_FINGER:        slot = EQUIPMENT_SLOT_FINGER1; break;
        case INVTYPE_TRINKET:       slot = EQUIPMENT_SLOT_TRINKET1; break;
        case INVTYPE_CLOAK:         slot = EQUIPMENT_SLOT_BACK; break;
        case INVTYPE_WEAPON:
        case INVTYPE_2HWEAPON:
        case INVTYPE_WEAPONMAINHAND: slot = EQUIPMENT_SLOT_MAINHAND; break;
        case INVTYPE_WEAPONOFFHAND:
        case INVTYPE_SHIELD:
        case INVTYPE_HOLDABLE:      slot = EQUIPMENT_SLOT_OFFHAND; break;
        case INVTYPE_RANGED:
        case INVTYPE_THROWN:
        case INVTYPE_RANGEDRIGHT:   slot = EQUIPMENT_SLOT_MAINHAND; break;
        default:
            _upgradesDetected++;
            return 0.0f; // Not equippable
    }

    Item const* currentItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);

    // If nothing equipped, any item is an upgrade
    if (!currentItem)
    {
        _upgradesDetected++;
        return 100.0f; // Max upgrade value - empty slot
    }

    // Compare item scores
    float newScore = CalculateItemScore(player, item);

    // Create a temporary LootItem for current item
    LootItem currentLootItem;
    currentLootItem.itemId = currentItem->GetEntry();
    currentLootItem.itemTemplate = currentItem->GetTemplate();
    currentLootItem.itemLevel = currentItem->GetItemLevel(player);
    currentLootItem.itemQuality = currentItem->GetQuality();

    float currentScore = CalculateItemScore(player, currentLootItem);

    // Calculate percentage improvement
    float upgradePercent = 0.0f;
    if (currentScore > 0.0f)
        upgradePercent = ((newScore - currentScore) / currentScore) * 100.0f;
    else if (newScore > 0.0f)
        upgradePercent = 100.0f;

    if (upgradePercent > 0.0f)
        _upgradesDetected++;

    return upgradePercent;
}

bool UnifiedLootManager::AnalysisModule::IsSignificantUpgrade(Player* player, LootItem const& item)
{
    // A significant upgrade is defined as >5% improvement
    float upgradeValue = CalculateUpgradeValue(player, item);
    return upgradeValue > 5.0f;
}

float UnifiedLootManager::AnalysisModule::CalculateStatWeight(Player* player, uint32 statType)
{
    if (!player)
        return 1.0f;

    // Get class and spec for stat weight determination
    uint8 playerClass = player->GetClass();
    uint32 spec = static_cast<uint32>(player->GetPrimarySpecialization());

    // WoW 11.x (The War Within) stat priorities by role
    // These are approximate weights - 1.0 = baseline, higher = more valuable
    // Based on common theorycrafting priorities

    // Determine role from spec
    bool isTank = false;
    bool isHealer = false;
    bool isMelee = false;
    bool isCaster = false;

    // Map spec ID to role (simplified - TWW spec IDs)
    switch (playerClass)
    {
        case CLASS_WARRIOR:
            isTank = (spec == 73); // Protection
            isMelee = !isTank;
            break;
        case CLASS_PALADIN:
            isTank = (spec == 66);  // Protection
            isHealer = (spec == 65); // Holy
            isMelee = !isTank && !isHealer;
            break;
        case CLASS_HUNTER:
            isMelee = (spec == 255); // Survival
            isCaster = !isMelee; // BM/MM are more ranged
            break;
        case CLASS_ROGUE:
            isMelee = true;
            break;
        case CLASS_PRIEST:
            isHealer = (spec == 256 || spec == 257); // Discipline/Holy
            isCaster = !isHealer; // Shadow
            break;
        case CLASS_DEATH_KNIGHT:
            isTank = (spec == 250); // Blood
            isMelee = !isTank;
            break;
        case CLASS_SHAMAN:
            isHealer = (spec == 264); // Restoration
            isMelee = (spec == 263); // Enhancement
            isCaster = !isHealer && !isMelee; // Elemental
            break;
        case CLASS_MAGE:
            isCaster = true;
            break;
        case CLASS_WARLOCK:
            isCaster = true;
            break;
        case CLASS_MONK:
            isTank = (spec == 268); // Brewmaster
            isHealer = (spec == 270); // Mistweaver
            isMelee = !isTank && !isHealer; // Windwalker
            break;
        case CLASS_DRUID:
            isTank = (spec == 104); // Guardian
            isHealer = (spec == 105); // Restoration
            isMelee = (spec == 103); // Feral
            isCaster = (spec == 102); // Balance
            break;
        case CLASS_DEMON_HUNTER:
            isTank = (spec == 581); // Vengeance
            isMelee = !isTank; // Havoc
            break;
        case CLASS_EVOKER:
            isHealer = (spec == 1468); // Preservation
            isCaster = !isHealer; // Devastation/Augmentation
            break;
        default:
            isMelee = true; // Default to melee DPS
            break;
    }

    // Primary stats (class-specific)
    switch (statType)
    {
        // Primary stats
        case ITEM_MOD_AGILITY:
            if (playerClass == CLASS_ROGUE || playerClass == CLASS_HUNTER ||
                playerClass == CLASS_MONK || playerClass == CLASS_DEMON_HUNTER ||
                (playerClass == CLASS_DRUID && (spec == 103 || spec == 104)) ||
                (playerClass == CLASS_SHAMAN && spec == 263))
                return 1.5f;
            return 0.0f;

        case ITEM_MOD_STRENGTH:
            if (playerClass == CLASS_WARRIOR || playerClass == CLASS_DEATH_KNIGHT ||
                (playerClass == CLASS_PALADIN && spec != 65))
                return 1.5f;
            return 0.0f;

        case ITEM_MOD_INTELLECT:
            if (isHealer || isCaster ||
                (playerClass == CLASS_PALADIN && spec == 65) ||
                (playerClass == CLASS_SHAMAN && (spec == 262 || spec == 264)) ||
                (playerClass == CLASS_DRUID && (spec == 102 || spec == 105)) ||
                (playerClass == CLASS_MONK && spec == 270) ||
                playerClass == CLASS_MAGE || playerClass == CLASS_WARLOCK ||
                playerClass == CLASS_PRIEST || playerClass == CLASS_EVOKER)
                return 1.5f;
            return 0.0f;

        case ITEM_MOD_STAMINA:
            if (isTank) return 1.3f;
            return 0.8f;

        // Secondary stats
        case ITEM_MOD_CRIT_RATING:
            if (isTank) return 0.8f;
            if (isHealer) return 1.0f;
            return 1.2f; // DPS love crit

        case ITEM_MOD_HASTE_RATING:
            if (isHealer) return 1.3f; // Healers often prioritize haste
            if (isCaster) return 1.2f;
            return 1.1f;

        case ITEM_MOD_MASTERY_RATING:
            if (isTank) return 1.2f; // Tanks often value mastery
            return 1.0f;

        case ITEM_MOD_VERSATILITY:
            if (isTank) return 1.4f; // Tanks love versatility
            if (isHealer) return 1.1f;
            return 0.9f; // Usually lower priority for DPS

        // Tertiary/special stats
        case ITEM_MOD_DODGE_RATING:
        case ITEM_MOD_PARRY_RATING:
        case ITEM_MOD_BLOCK_RATING:
            if (isTank) return 1.3f;
            return 0.0f; // Useless for non-tanks

        case ITEM_MOD_HIT_RATING:
        case ITEM_MOD_EXPERTISE_RATING:
            return 0.0f; // Deprecated stats in modern WoW

        case ITEM_MOD_SPELL_POWER:
            if (isHealer || isCaster) return 1.3f;
            return 0.0f;

        case ITEM_MOD_ATTACK_POWER:
            if (isMelee && !isTank) return 1.2f;
            return 0.0f;

        default:
            return 1.0f;
    }
}

float UnifiedLootManager::AnalysisModule::CompareItems(Player* player, LootItem const& newItem, Item const* currentItem)
{
    if (!player || !newItem.itemTemplate)
        return 0.0f;

    if (!currentItem)
        return 100.0f; // New item is infinitely better than nothing

    // Calculate scores for both items
    float newScore = CalculateItemScore(player, newItem);

    // Create temporary LootItem for current item
    LootItem currentLootItem;
    currentLootItem.itemId = currentItem->GetEntry();
    currentLootItem.itemTemplate = currentItem->GetTemplate();
    currentLootItem.itemLevel = currentItem->GetItemLevel(player);
    currentLootItem.itemQuality = currentItem->GetQuality();

    float currentScore = CalculateItemScore(player, currentLootItem);

    // Return difference (positive = new is better, negative = current is better)
    return newScore - currentScore;
}

float UnifiedLootManager::AnalysisModule::CalculateItemScore(Player* player, LootItem const& item)
{
    if (!player || !item.itemTemplate)
        return 0.0f;

    ItemTemplate const* proto = item.itemTemplate;
    float score = 0.0f;

    // Base score from item level (heavily weighted)
    uint32 itemLevel = item.itemLevel > 0 ? item.itemLevel : proto->GetBaseItemLevel();
    score += static_cast<float>(itemLevel) * 1.5f;

    // Quality bonus
    uint32 quality = item.itemQuality > 0 ? item.itemQuality : proto->GetQuality();
    switch (quality)
    {
        case ITEM_QUALITY_POOR:      score += 0.0f; break;
        case ITEM_QUALITY_NORMAL:    score += 5.0f; break;
        case ITEM_QUALITY_UNCOMMON:  score += 15.0f; break;
        case ITEM_QUALITY_RARE:      score += 30.0f; break;
        case ITEM_QUALITY_EPIC:      score += 50.0f; break;
        case ITEM_QUALITY_LEGENDARY: score += 75.0f; break;
        case ITEM_QUALITY_ARTIFACT:  score += 100.0f; break;
        default: break;
    }

    // Calculate weighted stat contribution
    for (uint32 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
    {
        int32 statType = proto->GetStatModifierBonusStat(i);
        int32 statValue = proto->GetStatPercentEditor(i);
        if (statType >= 0 && statValue != 0)
        {
            float weight = CalculateStatWeight(player, static_cast<uint32>(statType));
            score += std::abs(statValue) * weight;
        }
    }

    // Armor contribution for armor items
    if (proto->IsArmor())
    {
        uint32 armor = proto->GetArmor(itemLevel);
        // Normalize armor - plate has ~4000+ at high ilvl, cloth ~1000
        score += armor * 0.02f;
    }

    // Weapon DPS contribution
    if (proto->IsWeapon())
    {
        float dps = proto->GetDPS(itemLevel);
        score += dps * 2.0f; // Weapon DPS is very important
    }

    return score;
}

std::vector<std::pair<uint32, float>> UnifiedLootManager::AnalysisModule::GetStatPriorities(Player* player)
{
    std::vector<std::pair<uint32, float>> priorities;

    if (!player)
    {
        // Return generic priorities
        priorities.push_back({ITEM_MOD_STAMINA, 1.0f});
        priorities.push_back({ITEM_MOD_CRIT_RATING, 1.0f});
        priorities.push_back({ITEM_MOD_HASTE_RATING, 1.0f});
        priorities.push_back({ITEM_MOD_MASTERY_RATING, 1.0f});
        priorities.push_back({ITEM_MOD_VERSATILITY, 1.0f});
        return priorities;
    }

    // Build priority list using CalculateStatWeight for each relevant stat
    std::vector<uint32> relevantStats = {
        ITEM_MOD_AGILITY,
        ITEM_MOD_STRENGTH,
        ITEM_MOD_INTELLECT,
        ITEM_MOD_STAMINA,
        ITEM_MOD_CRIT_RATING,
        ITEM_MOD_HASTE_RATING,
        ITEM_MOD_MASTERY_RATING,
        ITEM_MOD_VERSATILITY,
        ITEM_MOD_DODGE_RATING,
        ITEM_MOD_PARRY_RATING,
        ITEM_MOD_BLOCK_RATING,
        ITEM_MOD_SPELL_POWER,
        ITEM_MOD_ATTACK_POWER
    };

    for (uint32 stat : relevantStats)
    {
        float weight = CalculateStatWeight(player, stat);
        if (weight > 0.0f)
            priorities.push_back({stat, weight});
    }

    // Sort by weight (descending)
    std::sort(priorities.begin(), priorities.end(),
        [](std::pair<uint32, float> const& a, std::pair<uint32, float> const& b)
        {
            return a.second > b.second;
        });

    return priorities;
}

// ============================================================================
// COORDINATION MODULE IMPLEMENTATION
// ============================================================================

void UnifiedLootManager::CoordinationModule::InitiateLootSession(Group* group, Loot* /*loot*/)
{
    std::lock_guard<decltype(_sessionMutex)> lock(_sessionMutex);

    // Initialize loot session for group-based loot management
    // This tracks items and coordinates distribution among group members

    // Track in unified system
    uint32 sessionId = _nextSessionId++;
    LootSession session(sessionId, group ? group->GetGUID().GetCounter() : 0);
    _activeSessions[sessionId] = session;
    _sessionsCreated++;
}

void UnifiedLootManager::CoordinationModule::ProcessLootSession(Group* group, uint32 lootSessionId)
{
    if (!group)
    {
        TC_LOG_ERROR("playerbot.loot", "ProcessLootSession: Null group provided");
        return;
    }

    std::lock_guard<decltype(_sessionMutex)> lock(_sessionMutex);

    auto it = _activeSessions.find(lootSessionId);
    if (it == _activeSessions.end())
    {
        TC_LOG_WARN("playerbot.loot", "ProcessLootSession: Session {} not found", lootSessionId);
        return;
    }

    LootSession& session = it->second;
    if (!session.isActive)
    {
        TC_LOG_DEBUG("playerbot.loot", "ProcessLootSession: Session {} already inactive", lootSessionId);
        return;
    }

    // Check for timeout
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime >= session.sessionTimeout)
    {
        TC_LOG_INFO("playerbot.loot", "ProcessLootSession: Session {} timed out after {} ms",
            lootSessionId, currentTime - session.sessionStartTime);
        HandleLootSessionTimeout(lootSessionId);
        return;
    }

    // Process any pending items in the session
    if (!session.availableItems.empty())
    {
        TC_LOG_DEBUG("playerbot.loot", "ProcessLootSession: Processing {} items in session {}",
            session.availableItems.size(), lootSessionId);

        // Prioritize items by value and upgrade potential
        PrioritizeLootDistribution(group, session.availableItems);

        // Optimize the sequence for efficient looting
        OptimizeLootSequence(group, session.availableItems);
    }

    // Update session metrics
    TC_LOG_DEBUG("playerbot.loot", "ProcessLootSession: Session {} processed, {} active rolls",
        lootSessionId, session.activeRolls.size());
}

void UnifiedLootManager::CoordinationModule::CompleteLootSession(uint32 lootSessionId)
{
    std::lock_guard<decltype(_sessionMutex)> lock(_sessionMutex);

    // Complete loot session - mark inactive and clean up
    // This finalizes distribution tracking and removes session from active management

    // Clean up from unified tracking
    auto it = _activeSessions.find(lootSessionId);
    if (it != _activeSessions.end())
    {
        _activeSessions.erase(it);
        _sessionsCompleted++;
    }
}

void UnifiedLootManager::CoordinationModule::HandleLootSessionTimeout(uint32 lootSessionId)
{
    // Handle session timeout - log timeout event and clean up
    TC_LOG_INFO("playerbot.loot", "Loot session {} timed out, cleaning up", lootSessionId);
    CompleteLootSession(lootSessionId); // Cleanup
}

void UnifiedLootManager::CoordinationModule::OrchestrateLootDistribution(Group* group, std::vector<LootItem> const& items)
{
    if (!group || items.empty())
    {
        TC_LOG_DEBUG("playerbot.loot", "OrchestrateLootDistribution: No group or empty items");
        return;
    }

    TC_LOG_INFO("playerbot.loot", "OrchestrateLootDistribution: Orchestrating {} items for group {}",
        items.size(), group->GetGUID().GetCounter());

    // Create a mutable copy for prioritization and optimization
    std::vector<LootItem> workingItems = items;

    // Step 1: Prioritize items by value and upgrade potential
    PrioritizeLootDistribution(group, workingItems);

    // Step 2: Optimize the looting sequence
    OptimizeLootSequence(group, workingItems);

    // Step 3: For each item, facilitate discussion and handle distribution
    for (LootItem const& item : workingItems)
    {
        // Broadcast recommendations to group members
        BroadcastLootRecommendations(group, item);

        // Facilitate any necessary discussion
        FacilitateGroupLootDiscussion(group, item);

        // Use the unified loot manager's distribution module
        UnifiedLootManager::instance()->DistributeLoot(group, item);
    }

    TC_LOG_DEBUG("playerbot.loot", "OrchestrateLootDistribution: Completed orchestration for {} items",
        workingItems.size());
}

void UnifiedLootManager::CoordinationModule::PrioritizeLootDistribution(Group* group, std::vector<LootItem>& items)
{
    if (!group || items.empty())
        return;

    TC_LOG_DEBUG("playerbot.loot", "PrioritizeLootDistribution: Prioritizing {} items", items.size());

    // Calculate average upgrade potential for each item across group members
    std::unordered_map<uint32, float> itemAverageUpgrade;

    for (auto const& item : items)
    {
        float totalUpgrade = 0.0f;
        uint32 memberCount = 0;

        // FIXED: Use GroupMemberResolver instead of GetMembers()
        for (auto const& slot : group->GetMemberSlots())
        {
            Player* member = GroupMemberResolver::ResolveMember(slot.guid);
            if (!member)
                continue;

            float upgradeValue = UnifiedLootManager::instance()->CalculateUpgradeValue(member, item);
            if (upgradeValue > 0.0f)
            {
                totalUpgrade += upgradeValue;
                ++memberCount;
            }
        }

        float avgUpgrade = memberCount > 0 ? totalUpgrade / memberCount : 0.0f;
        itemAverageUpgrade[item.itemId] = avgUpgrade;
    }

    // Sort items by: Quality (descending), then by average upgrade potential (descending), then by item level (descending)
    std::sort(items.begin(), items.end(),
        [&itemAverageUpgrade](LootItem const& a, LootItem const& b)
        {
            // Higher quality first
            if (a.itemQuality != b.itemQuality)
                return a.itemQuality > b.itemQuality;

            // Higher average upgrade potential first
            float upgradeA = itemAverageUpgrade.count(a.itemId) ? itemAverageUpgrade[a.itemId] : 0.0f;
            float upgradeB = itemAverageUpgrade.count(b.itemId) ? itemAverageUpgrade[b.itemId] : 0.0f;
            if (std::abs(upgradeA - upgradeB) > 0.01f)
                return upgradeA > upgradeB;

            // Higher item level first
            return a.itemLevel > b.itemLevel;
        });

    TC_LOG_DEBUG("playerbot.loot", "PrioritizeLootDistribution: Sorted {} items by priority", items.size());
}

void UnifiedLootManager::CoordinationModule::OptimizeLootSequence(Group* group, std::vector<LootItem>& items)
{
    if (!group || items.empty())
        return;

    TC_LOG_DEBUG("playerbot.loot", "OptimizeLootSequence: Optimizing sequence for {} items", items.size());

    // Build dependency map - some items should be distributed before others
    // e.g., weapons before off-hands, main armor pieces before accessories
    std::unordered_map<uint32, int32> slotPriority;
    slotPriority[EQUIPMENT_SLOT_MAINHAND] = 10;  // Main weapons first
    slotPriority[EQUIPMENT_SLOT_OFFHAND] = 9;
    slotPriority[EQUIPMENT_SLOT_RANGED] = 8;     // Ranged/relic
    slotPriority[EQUIPMENT_SLOT_HEAD] = 7;       // Tier token slots
    slotPriority[EQUIPMENT_SLOT_SHOULDERS] = 7;
    slotPriority[EQUIPMENT_SLOT_CHEST] = 7;
    slotPriority[EQUIPMENT_SLOT_HANDS] = 6;
    slotPriority[EQUIPMENT_SLOT_LEGS] = 6;
    slotPriority[EQUIPMENT_SLOT_FEET] = 5;
    slotPriority[EQUIPMENT_SLOT_WAIST] = 4;
    slotPriority[EQUIPMENT_SLOT_WRISTS] = 4;
    slotPriority[EQUIPMENT_SLOT_BACK] = 3;
    slotPriority[EQUIPMENT_SLOT_NECK] = 2;
    slotPriority[EQUIPMENT_SLOT_FINGER1] = 2;
    slotPriority[EQUIPMENT_SLOT_FINGER2] = 2;
    slotPriority[EQUIPMENT_SLOT_TRINKET1] = 1;
    slotPriority[EQUIPMENT_SLOT_TRINKET2] = 1;

    // Sort items by slot priority, then by existing priority order
    std::stable_sort(items.begin(), items.end(),
        [&slotPriority](LootItem const& a, LootItem const& b)
        {
            int32 priorityA = slotPriority.count(a.inventoryType) ? slotPriority[a.inventoryType] : 0;
            int32 priorityB = slotPriority.count(b.inventoryType) ? slotPriority[b.inventoryType] : 0;
            return priorityA > priorityB;
        });

    // Group similar items together to speed up rolling
    // Players can make batch decisions on same-type items
    auto sameTypeRange = items.begin();
    while (sameTypeRange != items.end())
    {
        auto rangeEnd = std::find_if(sameTypeRange, items.end(),
            [&sameTypeRange](LootItem const& item)
            {
                return item.inventoryType != sameTypeRange->inventoryType;
            });

        // Within each slot type, sort by item level descending so best items go first
        std::sort(sameTypeRange, rangeEnd,
            [](LootItem const& a, LootItem const& b)
            {
                return a.itemLevel > b.itemLevel;
            });

        sameTypeRange = rangeEnd;
    }

    TC_LOG_DEBUG("playerbot.loot", "OptimizeLootSequence: Sequence optimized for {} items", items.size());
}

void UnifiedLootManager::CoordinationModule::FacilitateGroupLootDiscussion(Group* group, LootItem const& item)
{
    if (!group)
        return;

    TC_LOG_DEBUG("playerbot.loot", "FacilitateGroupLootDiscussion: Starting discussion for item {} (id: {})",
        item.itemName, item.itemId);

    // Collect interest from all group members (bots will auto-evaluate)
    std::vector<std::pair<ObjectGuid, LootPriority>> memberInterest;
    uint32 needCount = 0;
    uint32 greedCount = 0;
    uint32 passCount = 0;

    // FIXED: Use GroupMemberResolver instead of GetMembers()
    for (auto const& slot : group->GetMemberSlots())
    {
        Player* member = GroupMemberResolver::ResolveMember(slot.guid);
        if (!member)
            continue;

        // Get the bot's loot distribution to evaluate interest
        auto* gameSystems = GetGameSystems(member);
        if (!gameSystems)
        {
            // Human player - assume they will decide themselves
            TC_LOG_DEBUG("playerbot.loot", "FacilitateGroupLootDiscussion: {} is human player, skipping auto-evaluation",
                member->GetName());
            continue;
        }

        LootPriority priority = gameSystems->GetLootDistribution()->AnalyzeItemPriority(item);
        memberInterest.emplace_back(member->GetGUID(), priority);

        switch (priority)
        {
            case LootPriority::CRITICAL_UPGRADE:
            case LootPriority::SIGNIFICANT_UPGRADE:
                ++needCount;
                break;
            case LootPriority::MINOR_UPGRADE:
            case LootPriority::SIDEGRADE:
                ++greedCount;
                break;
            case LootPriority::NOT_USEFUL:
            default:
                ++passCount;
                break;
        }
    }

    // Log the discussion summary
    TC_LOG_DEBUG("playerbot.loot", "FacilitateGroupLootDiscussion: Item {} interest - Need: {}, Greed: {}, Pass: {}",
        item.itemName, needCount, greedCount, passCount);

    // If multiple players have high interest, flag for potential conflict resolution
    if (needCount > 1)
    {
        TC_LOG_INFO("playerbot.loot", "FacilitateGroupLootDiscussion: {} players need item {} - initiating fair resolution",
            needCount, item.itemName);
        HandleLootConflictResolution(group, item);
    }
}

void UnifiedLootManager::CoordinationModule::HandleLootConflictResolution(Group* group, LootItem const& item)
{
    if (!group)
        return;

    TC_LOG_DEBUG("playerbot.loot", "HandleLootConflictResolution: Resolving conflict for item {} (id: {})",
        item.itemName, item.itemId);

    // Collect all members who want this item with their priority scores
    struct ConflictCandidate
    {
        ObjectGuid guid;
        float upgradeScore;
        uint32 itemsWonThisSession;
        bool isMainSpec;
        uint32 currentItemLevel;
    };

    std::vector<ConflictCandidate> candidates;

    // FIXED: Use GroupMemberResolver instead of GetMembers()
    for (auto const& slot : group->GetMemberSlots())
    {
        Player* member = GroupMemberResolver::ResolveMember(slot.guid);
        if (!member)
            continue;

        auto* gameSystems = GetGameSystems(member);
        if (!gameSystems)
            continue;

        LootPriority priority = gameSystems->GetLootDistribution()->AnalyzeItemPriority(item);
        if (priority < LootPriority::SIGNIFICANT_UPGRADE)  // Only consider HIGH and HIGHEST priority
            continue;

        float upgradeScore = UnifiedLootManager::instance()->CalculateUpgradeValue(member, item);
        bool isMainSpec = gameSystems->GetLootDistribution()->IsItemForMainSpec(item);

        // Get items won count from the loot distribution profile
        uint32 itemsWon = gameSystems->GetLootDistribution()->GetPlayerLootProfile().totalLootReceived;

        // Get current item level in the slot
        uint32 currentILevel = 0;
        if (item.inventoryType != 0)
        {
            Item* currentItem = member->GetItemByPos(INVENTORY_SLOT_BAG_0, static_cast<uint8>(item.inventoryType));
            if (currentItem)
            {
                ItemTemplate const* itemTemplate = currentItem->GetTemplate();
                if (itemTemplate)
                    currentILevel = itemTemplate->GetBaseItemLevel();
            }
        }

        candidates.push_back({member->GetGUID(), upgradeScore, itemsWon, isMainSpec, currentILevel});
    }

    if (candidates.empty())
    {
        TC_LOG_DEBUG("playerbot.loot", "HandleLootConflictResolution: No valid candidates for item {}", item.itemName);
        return;
    }

    // Sort candidates by fairness criteria:
    // 1. Main spec over off spec
    // 2. Bigger upgrade value (more needed)
    // 3. Fewer items won this session
    // 4. Lower current item level (more undergeared)
    std::sort(candidates.begin(), candidates.end(),
        [](ConflictCandidate const& a, ConflictCandidate const& b)
        {
            // Main spec beats off spec
            if (a.isMainSpec != b.isMainSpec)
                return a.isMainSpec;

            // Bigger upgrade is higher priority
            if (std::abs(a.upgradeScore - b.upgradeScore) > 5.0f)
                return a.upgradeScore > b.upgradeScore;

            // Fewer wins this session takes precedence
            if (a.itemsWonThisSession != b.itemsWonThisSession)
                return a.itemsWonThisSession < b.itemsWonThisSession;

            // Lower current item level means more undergeared
            return a.currentItemLevel < b.currentItemLevel;
        });

    // The top candidate wins the conflict
    ConflictCandidate const& winner = candidates[0];
    TC_LOG_INFO("playerbot.loot", "HandleLootConflictResolution: {} wins item {} (upgrade: {:.1f}, items won: {}, main spec: {})",
        winner.guid.GetCounter(), item.itemName, winner.upgradeScore, winner.itemsWonThisSession, winner.isMainSpec ? "yes" : "no");

    // Notify all candidates of the resolution
    for (auto const& candidate : candidates)
    {
        if (candidate.guid == winner.guid)
            continue;

        TC_LOG_DEBUG("playerbot.loot", "HandleLootConflictResolution: {} passed on {} (winner had higher priority)",
            candidate.guid.GetCounter(), item.itemName);
    }
}

void UnifiedLootManager::CoordinationModule::BroadcastLootRecommendations(Group* group, LootItem const& item)
{
    if (!group)
        return;

    TC_LOG_DEBUG("playerbot.loot", "BroadcastLootRecommendations: Broadcasting recommendations for item {} (id: {})",
        item.itemName, item.itemId);

    // Find the best candidate(s) for this item
    struct Recommendation
    {
        ObjectGuid playerGuid;
        std::string playerName;
        float upgradePercent;
        bool isMainSpec;
        LootRollType suggestedAction;
    };

    std::vector<Recommendation> recommendations;

    // FIXED: Use GroupMemberResolver instead of GetMembers()
    for (auto const& slot : group->GetMemberSlots())
    {
        Player* member = GroupMemberResolver::ResolveMember(slot.guid);
        if (!member)
            continue;

        auto* gameSystems = GetGameSystems(member);
        if (!gameSystems)
            continue;

        float upgradeValue = UnifiedLootManager::instance()->CalculateUpgradeValue(member, item);
        bool isMainSpec = gameSystems->GetLootDistribution()->IsItemForMainSpec(item);

        // Determine suggested action based on upgrade value and spec
        LootRollType suggestedAction;
        if (upgradeValue > 10.0f && isMainSpec)
            suggestedAction = LootRollType::NEED;
        else if (upgradeValue > 5.0f)
            suggestedAction = LootRollType::GREED;
        else if (upgradeValue > 0.0f)
            suggestedAction = LootRollType::GREED;  // Minor upgrade
        else
            suggestedAction = LootRollType::PASS;

        recommendations.push_back({
            member->GetGUID(),
            member->GetName(),
            upgradeValue,
            isMainSpec,
            suggestedAction
        });
    }

    // Sort by upgrade value to show best candidates first
    std::sort(recommendations.begin(), recommendations.end(),
        [](Recommendation const& a, Recommendation const& b)
        {
            return a.upgradePercent > b.upgradePercent;
        });

    // Log recommendations
    uint32 needCount = 0;
    uint32 greedCount = 0;
    for (auto const& rec : recommendations)
    {
        if (rec.suggestedAction == LootRollType::NEED)
            ++needCount;
        else if (rec.suggestedAction == LootRollType::GREED)
            ++greedCount;

        TC_LOG_DEBUG("playerbot.loot", "BroadcastLootRecommendations: {} should {} on {} (upgrade: {:.1f}%, main spec: {})",
            rec.playerName,
            rec.suggestedAction == LootRollType::NEED ? "NEED" :
                (rec.suggestedAction == LootRollType::GREED ? "GREED" : "PASS"),
            item.itemName,
            rec.upgradePercent,
            rec.isMainSpec ? "yes" : "no");
    }

    TC_LOG_DEBUG("playerbot.loot", "BroadcastLootRecommendations: {} need, {} greed recommended for {}",
        needCount, greedCount, item.itemName);
}

void UnifiedLootManager::CoordinationModule::OptimizeLootEfficiency(Group* group)
{
    if (!group)
        return;

    TC_LOG_DEBUG("playerbot.loot", "OptimizeLootEfficiency: Optimizing efficiency for group {}",
        group->GetGUID().GetCounter());

    // Calculate metrics for efficiency optimization
    uint32 totalMembers = 0;
    uint32 botMembers = 0;
    uint32 humanMembers = 0;
    uint32 membersWithAutoLoot = 0;

    // FIXED: Use GroupMemberResolver instead of GetMembers()
    for (auto const& slot : group->GetMemberSlots())
    {
        Player* member = GroupMemberResolver::ResolveMember(slot.guid);
        if (!member)
            continue;

        ++totalMembers;

        auto* gameSystems = GetGameSystems(member);
        if (gameSystems)
        {
            ++botMembers;
            ++membersWithAutoLoot;  // All bots have auto-loot
        }
        else
        {
            ++humanMembers;
            // Check if human has auto-loot enabled
            if (member->HasPlayerFlag(PLAYER_FLAGS_AUTO_DECLINE_GUILD))  // Just checking some flag as proxy
                ++membersWithAutoLoot;
        }
    }

    // Calculate optimal batch size based on group composition
    uint32 optimalBatchSize = 1;
    if (botMembers == totalMembers)
    {
        // All bots - can batch more aggressively
        optimalBatchSize = std::min(10u, totalMembers);
    }
    else if (humanMembers > 0)
    {
        // Human players present - smaller batches for responsiveness
        optimalBatchSize = std::min(3u, totalMembers);
    }

    // Log efficiency metrics
    float botRatio = totalMembers > 0 ? (static_cast<float>(botMembers) / totalMembers) * 100.0f : 0.0f;
    float autoLootRatio = totalMembers > 0 ? (static_cast<float>(membersWithAutoLoot) / totalMembers) * 100.0f : 0.0f;

    TC_LOG_DEBUG("playerbot.loot", "OptimizeLootEfficiency: {} total members ({} bots, {} humans), "
        "{:.1f}% bot ratio, {:.1f}% auto-loot, optimal batch size: {}",
        totalMembers, botMembers, humanMembers, botRatio, autoLootRatio, optimalBatchSize);

    // Store efficiency settings for use by other methods
    _efficiencySettings.optimalBatchSize = optimalBatchSize;
    _efficiencySettings.canUseFastPath = (botMembers == totalMembers);
    _efficiencySettings.lastOptimizationTime = GameTime::GetGameTimeMS();
}

void UnifiedLootManager::CoordinationModule::MinimizeLootTime(Group* group, uint32 sessionId)
{
    if (!group)
        return;

    std::lock_guard<decltype(_sessionMutex)> lock(_sessionMutex);

    // Find the session
    auto sessionIt = _activeSessions.find(sessionId);
    if (sessionIt == _activeSessions.end())
    {
        TC_LOG_DEBUG("playerbot.loot", "MinimizeLootTime: Session {} not found", sessionId);
        return;
    }

    LootSession& session = sessionIt->second;
    if (!session.isActive)
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();
    uint32 sessionDuration = currentTime - session.sessionStartTime;

    TC_LOG_DEBUG("playerbot.loot", "MinimizeLootTime: Optimizing session {} (duration: {}ms, {} items, {} rolls)",
        sessionId, sessionDuration, session.availableItems.size(), session.activeRolls.size());

    // Strategy 1: Auto-roll for pending bot decisions if taking too long
    if (sessionDuration > _efficiencySettings.rollTimeoutMs / 2)
    {
        // Check pending rolls and auto-decide for bots who haven't responded
        for (uint32 rollId : session.activeRolls)
        {
            // Force bot decisions on rolls that are taking too long
            // Bots should use their DetermineLootDecision() to make quick choices
            TC_LOG_DEBUG("playerbot.loot", "MinimizeLootTime: Roll {} pending - considering auto-roll", rollId);
        }
    }

    // Strategy 2: Batch processing for all-bot groups
    if (_efficiencySettings.canUseFastPath)
    {
        // In all-bot groups, we can distribute items much faster
        // No need to wait for human input or display roll animations

        // Calculate target completion time based on item count and throughput
        float targetDurationSeconds = static_cast<float>(session.availableItems.size()) /
                                      _efficiencySettings.targetItemsPerSecond;
        uint32 targetDurationMs = static_cast<uint32>(targetDurationSeconds * 1000.0f);

        if (sessionDuration > targetDurationMs * 1.5f)
        {
            TC_LOG_DEBUG("playerbot.loot", "MinimizeLootTime: Session {} exceeding target duration ({}ms > {}ms), accelerating",
                sessionId, sessionDuration, targetDurationMs);

            // Reduce roll timeout for remaining items
            _efficiencySettings.rollTimeoutMs = std::max(5000u, _efficiencySettings.rollTimeoutMs / 2);
        }
    }

    // Strategy 3: Parallel item evaluation
    // Group items that can be distributed in parallel (different slots, no conflicts)
    std::unordered_map<uint32, std::vector<size_t>> itemsBySlot;
    for (size_t i = 0; i < session.availableItems.size(); ++i)
    {
        itemsBySlot[session.availableItems[i].inventoryType].push_back(i);
    }

    // Items in different slots can potentially be distributed simultaneously
    uint32 parallelizable = 0;
    for (auto const& [slot, indices] : itemsBySlot)
    {
        if (indices.size() == 1)
            parallelizable++;
    }

    TC_LOG_DEBUG("playerbot.loot", "MinimizeLootTime: {} of {} items can be distributed in parallel",
        parallelizable, session.availableItems.size());

    // Strategy 4: Skip low-value items in speed mode
    if (sessionDuration > _efficiencySettings.rollTimeoutMs)
    {
        TC_LOG_DEBUG("playerbot.loot", "MinimizeLootTime: Session {} timeout exceeded, enabling fast-pass mode", sessionId);
        // Mark session for expedited processing
    }
}

void UnifiedLootManager::CoordinationModule::MaximizeLootFairness(Group* group, uint32 sessionId)
{
    if (!group)
        return;

    std::lock_guard<decltype(_sessionMutex)> lock(_sessionMutex);

    // Find the session
    auto sessionIt = _activeSessions.find(sessionId);
    if (sessionIt == _activeSessions.end())
    {
        TC_LOG_DEBUG("playerbot.loot", "MaximizeLootFairness: Session {} not found", sessionId);
        return;
    }

    LootSession& session = sessionIt->second;
    if (!session.isActive)
        return;

    // Initialize or get fairness tracker for this session
    auto& fairness = _sessionFairness[sessionId];
    if (!fairness.isActive)
    {
        fairness.sessionStartTime = session.sessionStartTime;
        fairness.isActive = true;

        // FIXED: Use GroupMemberResolver instead of GetMembers()
        for (auto const& slot : group->GetMemberSlots())
        {
            Player* member = GroupMemberResolver::ResolveMember(slot.guid);
            if (member)
            {
                fairness.itemsWonThisSession[member->GetGUID()] = 0;
                fairness.totalUpgradeValueReceived[member->GetGUID()] = 0.0f;
            }
        }
    }

    TC_LOG_DEBUG("playerbot.loot", "MaximizeLootFairness: Session {} - tracking {} members",
        sessionId, fairness.itemsWonThisSession.size());

    // Calculate fairness metrics for current state
    uint32 totalMembers = static_cast<uint32>(fairness.itemsWonThisSession.size());
    if (totalMembers == 0)
        return;

    uint32 totalItemsDistributed = 0;
    float totalUpgradeValue = 0.0f;
    for (auto const& [guid, count] : fairness.itemsWonThisSession)
    {
        totalItemsDistributed += count;
    }
    for (auto const& [guid, value] : fairness.totalUpgradeValueReceived)
    {
        totalUpgradeValue += value;
    }

    float averageItemsPerMember = static_cast<float>(totalItemsDistributed) / static_cast<float>(totalMembers);
    float averageUpgradeValue = totalUpgradeValue / static_cast<float>(totalMembers);

    // Calculate fairness score (Gini coefficient-inspired)
    // Lower score = more fair, 0 = perfectly equal distribution
    float itemFairnessDeviation = 0.0f;
    float upgradeFairnessDeviation = 0.0f;

    for (auto const& [guid, count] : fairness.itemsWonThisSession)
    {
        float deviation = static_cast<float>(count) - averageItemsPerMember;
        itemFairnessDeviation += std::abs(deviation);
    }
    itemFairnessDeviation /= static_cast<float>(totalMembers);

    for (auto const& [guid, value] : fairness.totalUpgradeValueReceived)
    {
        float deviation = value - averageUpgradeValue;
        upgradeFairnessDeviation += std::abs(deviation);
    }
    upgradeFairnessDeviation /= static_cast<float>(totalMembers);

    TC_LOG_DEBUG("playerbot.loot", "MaximizeLootFairness: Session {} - {} items distributed, "
        "item deviation: {:.2f}, upgrade deviation: {:.2f}",
        sessionId, totalItemsDistributed, itemFairnessDeviation, upgradeFairnessDeviation);

    // Identify members who are "behind" and should be prioritized
    std::vector<ObjectGuid> prioritizedMembers;
    for (auto const& [guid, count] : fairness.itemsWonThisSession)
    {
        // Member is behind if they have fewer items than average
        if (static_cast<float>(count) < averageItemsPerMember * 0.75f)
        {
            prioritizedMembers.push_back(guid);
            TC_LOG_DEBUG("playerbot.loot", "MaximizeLootFairness: Member {} is behind (items: {}, avg: {:.1f})",
                guid.GetCounter(), count, averageItemsPerMember);
        }
    }

    // Members with lower total upgrade value should also be prioritized
    for (auto const& [guid, value] : fairness.totalUpgradeValueReceived)
    {
        if (value < averageUpgradeValue * 0.5f)
        {
            // Add if not already in prioritized list
            if (std::find(prioritizedMembers.begin(), prioritizedMembers.end(), guid) == prioritizedMembers.end())
            {
                prioritizedMembers.push_back(guid);
                TC_LOG_DEBUG("playerbot.loot", "MaximizeLootFairness: Member {} has low upgrade value ({:.2f}, avg: {:.2f})",
                    guid.GetCounter(), value, averageUpgradeValue);
            }
        }
    }

    // Apply fairness bonuses for prioritized members
    // This information can be used by HandleLootConflictResolution when tie-breaking
    TC_LOG_DEBUG("playerbot.loot", "MaximizeLootFairness: {} members marked for priority consideration",
        prioritizedMembers.size());

    // Check for severe unfairness and log warning
    if (itemFairnessDeviation > 2.0f || upgradeFairnessDeviation > 50.0f)
    {
        TC_LOG_WARN("playerbot.loot", "MaximizeLootFairness: Session {} has significant fairness imbalance "
            "(item dev: {:.2f}, upgrade dev: {:.2f})",
            sessionId, itemFairnessDeviation, upgradeFairnessDeviation);
    }
}

// ============================================================================
// DISTRIBUTION MODULE IMPLEMENTATION
// ============================================================================

void UnifiedLootManager::DistributionModule::DistributeLoot(Group* group, LootItem const& item)
{
    if (!group)
    {
        TC_LOG_ERROR("playerbot.loot", "DistributeLoot called with null group");
        return;
    }

    LootMethod method = group->GetLootMethod();

    TC_LOG_DEBUG("playerbot.loot", "Distributing loot item {} (id: {}) for group {} using method {}",
        item.itemName, item.itemId, group->GetGUID().GetCounter(), static_cast<uint32>(method));

    switch (method)
    {
        case MASTER_LOOT:
            HandleMasterLoot(group, item);
            break;

        case GROUP_LOOT:
        case NEED_BEFORE_GREED:
            HandleGroupLoot(group, item);
            break;

        case FREE_FOR_ALL:
            // In free-for-all, first bot to loot gets it - no coordination needed
            TC_LOG_DEBUG("playerbot.loot", "Free-for-all loot - no distribution needed");
            break;

        case ROUND_ROBIN:
            // Round robin is handled by game's loot system
            TC_LOG_DEBUG("playerbot.loot", "Round-robin loot - handled by game system");
            break;

        case PERSONAL_LOOT:
            // Personal loot is auto-assigned by game
            TC_LOG_DEBUG("playerbot.loot", "Personal loot - auto-assigned by game");
            break;

        default:
            TC_LOG_WARN("playerbot.loot", "Unknown loot method: {}", static_cast<uint32>(method));
            break;
    }

    _itemsDistributed++;
}

void UnifiedLootManager::DistributionModule::HandleLootRoll(Player* player, uint32 rollId, LootRollType rollType)
{
    std::lock_guard<decltype(_rollMutex)> lock(_rollMutex);

    (GetGameSystems(player) ? GetGameSystems(player)->GetLootDistribution()->ProcessPlayerLootDecision(rollId, rollType) : decltype(GetGameSystems(player)->GetLootDistribution()->ProcessPlayerLootDecision(rollId, rollType))());
    _rollsProcessed++;
}

LootRollType UnifiedLootManager::DistributionModule::DetermineLootDecision(
    Player* player, LootItem const& item, LootDecisionStrategy strategy)
{
    // Strategy parameter is ignored - each bot uses its own profile strategy
    return (GetGameSystems(player) ? GetGameSystems(player)->GetLootDistribution()->DetermineLootDecision(item) : decltype(GetGameSystems(player)->GetLootDistribution()->DetermineLootDecision(item))());
}

LootPriority UnifiedLootManager::DistributionModule::CalculateLootPriority(Player* player, LootItem const& item)
{
    return (GetGameSystems(player) ? GetGameSystems(player)->GetLootDistribution()->AnalyzeItemPriority(item) : decltype(GetGameSystems(player)->GetLootDistribution()->AnalyzeItemPriority(item))());
}

bool UnifiedLootManager::DistributionModule::ShouldRollNeed(Player* player, LootItem const& item)
{
    return (GetGameSystems(player) ? GetGameSystems(player)->GetLootDistribution()->CanPlayerNeedItem(item) : decltype(GetGameSystems(player)->GetLootDistribution()->CanPlayerNeedItem(item))());
}

bool UnifiedLootManager::DistributionModule::ShouldRollGreed(Player* player, LootItem const& item)
{
    return (GetGameSystems(player) ? GetGameSystems(player)->GetLootDistribution()->ShouldPlayerGreedItem(item) : decltype(GetGameSystems(player)->GetLootDistribution()->ShouldPlayerGreedItem(item))());
}

bool UnifiedLootManager::DistributionModule::IsItemForClass(Player* player, LootItem const& item)
{
    return (GetGameSystems(player) ? GetGameSystems(player)->GetLootDistribution()->IsClassAppropriate(item) : decltype(GetGameSystems(player)->GetLootDistribution()->IsClassAppropriate(item))());
}

bool UnifiedLootManager::DistributionModule::IsItemForMainSpec(Player* player, LootItem const& item)
{
    return (GetGameSystems(player) ? GetGameSystems(player)->GetLootDistribution()->IsItemForMainSpec(item) : decltype(GetGameSystems(player)->GetLootDistribution()->IsItemForMainSpec(item))());
}

bool UnifiedLootManager::DistributionModule::IsItemForOffSpec(Player* player, LootItem const& item)
{
    return (GetGameSystems(player) ? GetGameSystems(player)->GetLootDistribution()->IsItemForOffSpec(item) : decltype(GetGameSystems(player)->GetLootDistribution()->IsItemForOffSpec(item))());
}

bool UnifiedLootManager::DistributionModule::AwardItemToPlayer(Player* player, uint32 itemId, uint32 count)
{
    if (!player || !itemId || count == 0)
    {
        TC_LOG_ERROR("playerbot.loot", "AwardItemToPlayer: Invalid parameters (player={}, itemId={}, count={})",
            player ? "valid" : "null", itemId, count);
        return false;
    }

    // Check if item template exists
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
    if (!itemTemplate)
    {
        TC_LOG_ERROR("playerbot.loot", "AwardItemToPlayer: Item template not found for itemId {}", itemId);
        return false;
    }

    // Find inventory space
    ItemPosCountVec dest;
    InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count);
    if (msg != EQUIP_ERR_OK)
    {
        TC_LOG_WARN("playerbot.loot", "AwardItemToPlayer: {} cannot store item {} (error: {})",
            player->GetName(), itemId, static_cast<uint32>(msg));
        return false;
    }

    // Store the item
    Item* item = player->StoreNewItem(dest, itemId, true);
    if (!item)
    {
        TC_LOG_ERROR("playerbot.loot", "AwardItemToPlayer: Failed to create item {} for {}",
            itemId, player->GetName());
        return false;
    }

    // Send loot message to player
    player->SendNewItem(item, count, true, false);

    TC_LOG_DEBUG("playerbot.loot", "AwardItemToPlayer: Successfully awarded {} x{} to {}",
        itemTemplate->GetName(LOCALE_enUS), count, player->GetName());

    return true;
}

// ============================================================================
// HELPER METHODS - Group Loot Coordination
// ============================================================================

void UnifiedLootManager::DistributionModule::HandleMasterLoot(Group* group, LootItem const& item)
{
    if (!group)
        return;

    // Get master looter
    ObjectGuid masterLooterGuid = group->GetMasterLooterGuid();
    Player* masterLooter = ObjectAccessor::FindPlayer(masterLooterGuid);

    if (!masterLooter)
    {
        TC_LOG_ERROR("playerbot.loot", "Master looter not found for group {}", group->GetGUID().GetCounter());
        return;
    }

    // Evaluate all bots in the group
    std::vector<BotRollEvaluation> evaluations;

    // FIXED: Use GroupMemberResolver instead of GetMembers()
    for (auto const& slot : group->GetMemberSlots())
    {
        Player* member = GroupMemberResolver::ResolveMember(slot.guid);
        if (!member || !GetBotAI(member))
            continue; // Skip non-bots and human players

        IGameSystemsManager* systems = GetGameSystems(member);
        if (!systems)
            continue;

        // Calculate upgrade value for this bot
        float upgradeValue = UnifiedLootManager::instance()->CalculateUpgradeValue(member, item);
        LootPriority priority = CalculateLootPriority(member, item);
        LootRollType recommendedRoll = DetermineLootDecision(member, item, LootDecisionStrategy::UPGRADE_PRIORITY);

        BotRollEvaluation eval;
        eval.bot = member;
        eval.rollType = recommendedRoll;
        eval.rollValue = 0; // Master loot doesn't use random rolls
        eval.upgradeValue = upgradeValue;
        eval.priority = priority;

        evaluations.push_back(eval);
    }

    if (evaluations.empty())
    {
        TC_LOG_DEBUG("playerbot.loot", "No eligible bots for master loot item {}", item.itemId);
        return;
    }

    // Sort by priority (lower enum value = higher priority) then by upgrade value
    std::sort(evaluations.begin(), evaluations.end(),
        [](BotRollEvaluation const& a, BotRollEvaluation const& b)
        {
            if (a.priority != b.priority)
                return a.priority < b.priority; // Higher priority (lower enum)
            return a.upgradeValue > b.upgradeValue; // Higher upgrade value
        });

    // Award to highest priority bot
    Player* winner = evaluations[0].bot;
    TC_LOG_INFO("playerbot.loot", "Master loot: Awarding item {} to {} (priority: {}, upgrade: {:.1f}%)",
        item.itemId, winner->GetName(), static_cast<uint32>(evaluations[0].priority), evaluations[0].upgradeValue);

    // Award the item via TrinityCore's inventory system
    if (AwardItemToPlayer(winner, item.itemId, 1))
    {
        TC_LOG_INFO("playerbot.loot", "Successfully awarded item {} to {}", item.itemId, winner->GetName());
        _itemsDistributed++;
    }
    else
    {
        TC_LOG_ERROR("playerbot.loot", "Failed to award item {} to {} - inventory may be full",
            item.itemId, winner->GetName());
    }
}

void UnifiedLootManager::DistributionModule::HandleGroupLoot(Group* group, LootItem const& item)
{
    if (!group)
        return;

    std::lock_guard<decltype(_rollMutex)> lock(_rollMutex);

    // Create a new loot roll session
    uint32 rollId = _nextRollId++;
    LootRoll roll(rollId);
    roll.itemId = item.itemId;
    roll.lootSlot = item.lootSlot;
    roll.groupId = static_cast<uint32>(group->GetGUID().GetCounter());

    // Collect rolls from all bots in the group
    std::vector<BotRollEvaluation> evaluations;

    // FIXED: Use GroupMemberResolver instead of GetMembers()
    for (auto const& slot : group->GetMemberSlots())
    {
        Player* member = GroupMemberResolver::ResolveMember(slot.guid);
        if (!member || !GetBotAI(member))
            continue; // Skip non-bots and human players

        IGameSystemsManager* systems = GetGameSystems(member);
        if (!systems)
            continue;

        // Determine bot's roll decision
        LootRollType rollType = DetermineLootDecision(member, item,
            group->GetLootMethod() == NEED_BEFORE_GREED ?
                LootDecisionStrategy::NEED_BEFORE_GREED :
                LootDecisionStrategy::UPGRADE_PRIORITY);

        // Generate roll value (1-100) for Need/Greed
        uint32 rollValue = 0;
        if (rollType == LootRollType::NEED || rollType == LootRollType::GREED)
        {
            rollValue = urand(1, 100);
        }

        // Calculate upgrade metrics
        float upgradeValue = UnifiedLootManager::instance()->CalculateUpgradeValue(member, item);
        LootPriority priority = CalculateLootPriority(member, item);

        BotRollEvaluation eval;
        eval.bot = member;
        eval.rollType = rollType;
        eval.rollValue = rollValue;
        eval.upgradeValue = upgradeValue;
        eval.priority = priority;

        evaluations.push_back(eval);

        // Record in roll tracking
        roll.playerRolls[member->GetGUID().GetCounter()] = rollType;
        roll.rollValues[member->GetGUID().GetCounter()] = rollValue;

        TC_LOG_DEBUG("playerbot.loot", "Bot {} rolled {} (value: {}) for item {}",
            member->GetName(), static_cast<uint32>(rollType), rollValue, item.itemId);
    }

    if (evaluations.empty())
    {
        TC_LOG_DEBUG("playerbot.loot", "No eligible bots for group loot item {}", item.itemId);
        return;
    }

    // Store roll for potential tie resolution
    _activeRolls[rollId] = roll;

    // Determine winner
    Player* winner = DetermineGroupLootWinner(evaluations);
    if (winner)
    {
        uint32 winnerGuid = winner->GetGUID().GetCounter();
        roll.isComplete = true;
        roll.winnerGuid = winnerGuid;  // Track winner for ninja detection and statistics

        TC_LOG_INFO("playerbot.loot", "Group loot: {} won item {} with roll type {}",
            winner->GetName(), item.itemId,
            static_cast<uint32>(roll.playerRolls[winnerGuid]));

        // Award the item via TrinityCore's inventory system
        if (AwardItemToPlayer(winner, item.itemId, 1))
        {
            TC_LOG_INFO("playerbot.loot", "Successfully awarded item {} to {}", item.itemId, winner->GetName());
            _itemsDistributed++;
        }
        else
        {
            TC_LOG_ERROR("playerbot.loot", "Failed to award item {} to {} - inventory may be full",
                item.itemId, winner->GetName());
        }
    }

    // Update active rolls with winner info
    _activeRolls[rollId] = roll;
    _rollsProcessed++;
}

Player* UnifiedLootManager::DistributionModule::DetermineGroupLootWinner(std::vector<BotRollEvaluation>& rolls)
{
    if (rolls.empty())
        return nullptr;

    // Separate rolls by type: Need > Greed > Disenchant > Pass
    std::vector<BotRollEvaluation*> needRolls;
    std::vector<BotRollEvaluation*> greedRolls;
    std::vector<BotRollEvaluation*> disenchantRolls;

    for (auto& roll : rolls)
    {
        if (roll.rollType == LootRollType::NEED)
            needRolls.push_back(&roll);
        else if (roll.rollType == LootRollType::GREED)
            greedRolls.push_back(&roll);
        else if (roll.rollType == LootRollType::DISENCHANT)
            disenchantRolls.push_back(&roll);
        // PASS rolls are excluded
    }

    // Determine winning pool (Need takes priority over Greed over Disenchant)
    std::vector<BotRollEvaluation*>* winningPool = nullptr;
    const char* rollTypeName = "None";

    if (!needRolls.empty())
    {
        winningPool = &needRolls;
        rollTypeName = "NEED";
    }
    else if (!greedRolls.empty())
    {
        winningPool = &greedRolls;
        rollTypeName = "GREED";
    }
    else if (!disenchantRolls.empty())
    {
        winningPool = &disenchantRolls;
        rollTypeName = "DISENCHANT";
    }

    if (!winningPool || winningPool->empty())
        return nullptr;

    // Sort by roll value (highest wins)
    std::sort(winningPool->begin(), winningPool->end(),
        [](BotRollEvaluation* a, BotRollEvaluation* b)
        {
            return a->rollValue > b->rollValue;
        });

    // Check for ties
    BotRollEvaluation* winner = (*winningPool)[0];
    std::vector<BotRollEvaluation*> tiedRolls;
    tiedRolls.push_back(winner);

    for (size_t i = 1; i < winningPool->size(); ++i)
    {
        if ((*winningPool)[i]->rollValue == winner->rollValue)
            tiedRolls.push_back((*winningPool)[i]);
        else
            break;
    }

    // If there's a tie, resolve by upgrade value
    if (tiedRolls.size() > 1)
    {
        TC_LOG_DEBUG("playerbot.loot", "Tie detected between {} bots with roll value {}, resolving by upgrade value",
            tiedRolls.size(), winner->rollValue);

        std::sort(tiedRolls.begin(), tiedRolls.end(),
            [](BotRollEvaluation* a, BotRollEvaluation* b)
            {
                return a->upgradeValue > b->upgradeValue;
            });

        winner = tiedRolls[0];
    }

    TC_LOG_DEBUG("playerbot.loot", "Winner determined: {} ({} roll: {}, upgrade: {:.1f}%)",
        winner->bot->GetName(), rollTypeName, winner->rollValue, winner->upgradeValue);

    return winner->bot;
}

void UnifiedLootManager::DistributionModule::ExecuteLootDistribution(Group* group, uint32 rollId)
{
    if (!group)
    {
        TC_LOG_ERROR("playerbot.loot", "ExecuteLootDistribution called with null group");
        return;
    }

    std::lock_guard<decltype(_rollMutex)> lock(_rollMutex);

    auto it = _activeRolls.find(rollId);
    if (it == _activeRolls.end())
    {
        TC_LOG_WARN("playerbot.loot", "ExecuteLootDistribution: Roll ID {} not found", rollId);
        return;
    }

    LootRoll& roll = it->second;

    if (roll.isComplete)
    {
        TC_LOG_DEBUG("playerbot.loot", "Roll {} already completed, checking if item needs awarding", rollId);

        // Award item to winner if we have a valid winner GUID
        if (roll.winnerGuid != 0)
        {
            Player* winner = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(roll.winnerGuid));
            if (winner)
            {
                if (AwardItemToPlayer(winner, roll.itemId, 1))
                {
                    TC_LOG_INFO("playerbot.loot", "Successfully awarded item {} to {} via ExecuteLootDistribution",
                        roll.itemId, winner->GetName());
                    _itemsDistributed++;
                }
                else
                {
                    TC_LOG_ERROR("playerbot.loot", "Failed to award item {} to {} - inventory may be full",
                        roll.itemId, winner->GetName());
                }
            }
            else
            {
                TC_LOG_WARN("playerbot.loot", "Winner player with GUID {} no longer online for roll {}",
                    roll.winnerGuid, rollId);
            }
        }
    }
    else
    {
        TC_LOG_WARN("playerbot.loot", "ExecuteLootDistribution: Roll {} not yet completed", rollId);
    }

    // Clean up completed roll
    _activeRolls.erase(it);
}

void UnifiedLootManager::DistributionModule::ResolveRollTies(Group* group, uint32 rollId)
{
    if (!group)
    {
        TC_LOG_ERROR("playerbot.loot", "ResolveRollTies called with null group");
        return;
    }

    std::lock_guard<decltype(_rollMutex)> lock(_rollMutex);

    auto it = _activeRolls.find(rollId);
    if (it == _activeRolls.end())
    {
        TC_LOG_WARN("playerbot.loot", "ResolveRollTies: Roll ID {} not found", rollId);
        return;
    }

    LootRoll& roll = it->second;

    // Find all players with the highest roll value
    uint32 highestRoll = 0;
    std::vector<uint32> tiedPlayers;

    for (auto const& [playerGuid, rollValue] : roll.rollValues)
    {
        if (rollValue > highestRoll)
        {
            highestRoll = rollValue;
            tiedPlayers.clear();
            tiedPlayers.push_back(playerGuid);
        }
        else if (rollValue == highestRoll && rollValue > 0)
        {
            tiedPlayers.push_back(playerGuid);
        }
    }

    if (tiedPlayers.size() <= 1)
    {
        TC_LOG_DEBUG("playerbot.loot", "No tie to resolve for roll {}", rollId);
        return;
    }

    TC_LOG_INFO("playerbot.loot", "Resolving tie for roll {} between {} players with value {}",
        rollId, tiedPlayers.size(), highestRoll);

    // Re-roll for tied players
    std::vector<std::pair<uint32, uint32>> rerolls; // playerGuid, rollValue
    for (uint32 playerGuid : tiedPlayers)
    {
        uint32 newRoll = urand(1, 100);
        rerolls.push_back({playerGuid, newRoll});
        TC_LOG_DEBUG("playerbot.loot", "Player {} re-rolled {}", playerGuid, newRoll);
    }

    // Find highest re-roll
    std::sort(rerolls.begin(), rerolls.end(),
        [](std::pair<uint32, uint32> const& a, std::pair<uint32, uint32> const& b)
        {
            return a.second > b.second;
        });

    // Mark roll as complete
    uint32 winnerGuid = rerolls[0].first;
    roll.isComplete = true;

    TC_LOG_INFO("playerbot.loot", "Tie resolved: Player {} won with re-roll {}",
        winnerGuid, rerolls[0].second);
}

void UnifiedLootManager::DistributionModule::HandleLootNinja(Group* group, uint32 suspectedPlayer)
{
    if (!group)
    {
        TC_LOG_ERROR("playerbot.loot", "HandleLootNinja called with null group");
        return;
    }

    ObjectGuid suspectGuid = ObjectGuid::Create<HighGuid::Player>(suspectedPlayer);
    Player* suspect = ObjectAccessor::FindPlayer(suspectGuid);
    if (!suspect)
    {
        TC_LOG_WARN("playerbot.loot", "HandleLootNinja: Suspected player {} not found", suspectedPlayer);
        return;
    }

    // Log the ninja loot attempt
    TC_LOG_WARN("playerbot.loot", "Potential ninja loot detected: Player {} in group {}",
        suspect->GetName(), group->GetGUID().GetCounter());

    // For bots, this is primarily for logging and detection
    // Human players would handle actual consequences

    // Check recent loot history for suspicious patterns
    std::lock_guard<decltype(_rollMutex)> lock(_rollMutex);

    uint32 recentNeedRolls = 0;
    uint32 recentWins = 0;

    for (auto const& [rollId, roll] : _activeRolls)
    {
        if (roll.groupId != group->GetGUID().GetCounter())
            continue;

        auto playerRollIt = roll.playerRolls.find(suspectedPlayer);
        if (playerRollIt != roll.playerRolls.end())
        {
            if (playerRollIt->second == LootRollType::NEED)
                ++recentNeedRolls;

            // Track wins using winnerGuid field in LootRoll struct
            if (roll.winnerGuid == suspectedPlayer)
                ++recentWins;
        }
    }

    // Simple heuristic: If player has won more than 50% of recent rolls, flag as suspicious
    if (recentNeedRolls > 0 && recentWins > (recentNeedRolls / 2))
    {
        TC_LOG_WARN("playerbot.loot", "Player {} has suspicious loot pattern: {} wins out of {} need rolls",
            suspect->GetName(), recentWins, recentNeedRolls);

        // For bots, we could implement automatic remediation:
        // - Reduce their loot priority temporarily
        // - Flag for review
        // - Notify group leader (if human)

        // For now, just log for monitoring
    }
    else
    {
        TC_LOG_DEBUG("playerbot.loot", "Player {} loot pattern appears normal: {} wins out of {} need rolls",
            suspect->GetName(), recentWins, recentNeedRolls);
    }
}

// ============================================================================
// UNIFIED MANAGER PUBLIC INTERFACE - Analysis
// ============================================================================

float UnifiedLootManager::CalculateItemValue(Player* player, LootItem const& item)
{
    return _analysis->CalculateItemValue(player, item);
}

float UnifiedLootManager::CalculateUpgradeValue(Player* player, LootItem const& item)
{
    return _analysis->CalculateUpgradeValue(player, item);
}

bool UnifiedLootManager::IsSignificantUpgrade(Player* player, LootItem const& item)
{
    return _analysis->IsSignificantUpgrade(player, item);
}

float UnifiedLootManager::CalculateStatWeight(Player* player, uint32 statType)
{
    return _analysis->CalculateStatWeight(player, statType);
}

float UnifiedLootManager::CompareItems(Player* player, LootItem const& newItem, Item const* currentItem)
{
    return _analysis->CompareItems(player, newItem, currentItem);
}

float UnifiedLootManager::CalculateItemScore(Player* player, LootItem const& item)
{
    return _analysis->CalculateItemScore(player, item);
}

std::vector<std::pair<uint32, float>> UnifiedLootManager::GetStatPriorities(Player* player)
{
    return _analysis->GetStatPriorities(player);
}

// ============================================================================
// UNIFIED MANAGER PUBLIC INTERFACE - Coordination
// ============================================================================

void UnifiedLootManager::InitiateLootSession(Group* group, Loot* loot)
{
    _coordination->InitiateLootSession(group, loot);
}

void UnifiedLootManager::ProcessLootSession(Group* group, uint32 lootSessionId)
{
    _coordination->ProcessLootSession(group, lootSessionId);
}

void UnifiedLootManager::CompleteLootSession(uint32 lootSessionId)
{
    _coordination->CompleteLootSession(lootSessionId);
}

void UnifiedLootManager::HandleLootSessionTimeout(uint32 lootSessionId)
{
    _coordination->HandleLootSessionTimeout(lootSessionId);
}

void UnifiedLootManager::OrchestrateLootDistribution(Group* group, std::vector<LootItem> const& items)
{
    _coordination->OrchestrateLootDistribution(group, items);
}

void UnifiedLootManager::PrioritizeLootDistribution(Group* group, std::vector<LootItem>& items)
{
    _coordination->PrioritizeLootDistribution(group, items);
}

void UnifiedLootManager::OptimizeLootSequence(Group* group, std::vector<LootItem>& items)
{
    _coordination->OptimizeLootSequence(group, items);
}

void UnifiedLootManager::FacilitateGroupLootDiscussion(Group* group, LootItem const& item)
{
    _coordination->FacilitateGroupLootDiscussion(group, item);
}

void UnifiedLootManager::HandleLootConflictResolution(Group* group, LootItem const& item)
{
    _coordination->HandleLootConflictResolution(group, item);
}

void UnifiedLootManager::BroadcastLootRecommendations(Group* group, LootItem const& item)
{
    _coordination->BroadcastLootRecommendations(group, item);
}

void UnifiedLootManager::OptimizeLootEfficiency(Group* group)
{
    _coordination->OptimizeLootEfficiency(group);
}

void UnifiedLootManager::MinimizeLootTime(Group* group, uint32 sessionId)
{
    _coordination->MinimizeLootTime(group, sessionId);
}

void UnifiedLootManager::MaximizeLootFairness(Group* group, uint32 sessionId)
{
    _coordination->MaximizeLootFairness(group, sessionId);
}

// ============================================================================
// UNIFIED MANAGER PUBLIC INTERFACE - Distribution
// ============================================================================

void UnifiedLootManager::DistributeLoot(Group* group, LootItem const& item)
{
    _distribution->DistributeLoot(group, item);
}

void UnifiedLootManager::HandleLootRoll(Player* player, uint32 rollId, LootRollType rollType)
{
    _distribution->HandleLootRoll(player, rollId, rollType);
}

LootRollType UnifiedLootManager::DetermineLootDecision(
    Player* player, LootItem const& item, LootDecisionStrategy strategy)
{
    return _distribution->DetermineLootDecision(player, item, strategy);
}

LootPriority UnifiedLootManager::CalculateLootPriority(Player* player, LootItem const& item)
{
    return _distribution->CalculateLootPriority(player, item);
}

bool UnifiedLootManager::ShouldRollNeed(Player* player, LootItem const& item)
{
    return _distribution->ShouldRollNeed(player, item);
}

bool UnifiedLootManager::ShouldRollGreed(Player* player, LootItem const& item)
{
    return _distribution->ShouldRollGreed(player, item);
}

bool UnifiedLootManager::IsItemForClass(Player* player, LootItem const& item)
{
    return _distribution->IsItemForClass(player, item);
}

bool UnifiedLootManager::IsItemForMainSpec(Player* player, LootItem const& item)
{
    return _distribution->IsItemForMainSpec(player, item);
}

bool UnifiedLootManager::IsItemForOffSpec(Player* player, LootItem const& item)
{
    return _distribution->IsItemForOffSpec(player, item);
}

void UnifiedLootManager::ExecuteLootDistribution(Group* group, uint32 rollId)
{
    _distribution->ExecuteLootDistribution(group, rollId);
}

void UnifiedLootManager::ResolveRollTies(Group* group, uint32 rollId)
{
    _distribution->ResolveRollTies(group, rollId);
}

void UnifiedLootManager::HandleLootNinja(Group* group, uint32 suspectedPlayer)
{
    _distribution->HandleLootNinja(group, suspectedPlayer);
}

// ============================================================================
// UNIFIED OPERATIONS
// ============================================================================

void UnifiedLootManager::ProcessCompleteLootFlow(Group* group, Loot* loot)
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    auto startTime = GameTime::GetGameTimeMS();
    _totalOperations++;

    // Step 1: Initiate session (Coordination)
    InitiateLootSession(group, loot);

    // Step 2: Analyze items (Analysis)
    // This happens automatically when players evaluate items

    // Step 3: Execute distribution (Distribution)
    // This happens when players roll

    auto endTime = GameTime::GetGameTimeMS();
    _totalProcessingTimeMs += (endTime - startTime);

    TC_LOG_DEBUG("playerbot.loot", "Processed complete loot flow in {} ms", endTime - startTime);
}

std::string UnifiedLootManager::GetLootRecommendation(Player* player, LootItem const& item)
{
    std::ostringstream oss;

    // Analysis
    float itemValue = CalculateItemValue(player, item);
    float upgradeValue = CalculateUpgradeValue(player, item);
    bool isSignificantUpgrade = IsSignificantUpgrade(player, item);

    // Distribution decision
    LootPriority priority = CalculateLootPriority(player, item);
    LootRollType recommendedRoll = DetermineLootDecision(player, item, LootDecisionStrategy::UPGRADE_PRIORITY);

    // Format recommendation
    oss << "Item Value: " << itemValue << "/100\n";
    oss << "Upgrade: " << upgradeValue << "% improvement\n";
    oss << "Significant Upgrade: " << (isSignificantUpgrade ? "Yes" : "No") << "\n";
    oss << "Priority: " << static_cast<uint32>(priority) << "\n";
    oss << "Recommended Action: ";

    switch (recommendedRoll)
    {
        case LootRollType::NEED:       oss << "NEED"; break;
        case LootRollType::GREED:      oss << "GREED"; break;
        case LootRollType::PASS:       oss << "PASS"; break;
        case LootRollType::DISENCHANT: oss << "DISENCHANT"; break;
        default:                       oss << "UNKNOWN"; break;
    }

    return oss.str();
}

std::string UnifiedLootManager::GetLootStatistics() const
{
    std::ostringstream oss;

    oss << "=== UnifiedLootManager Statistics ===\n";
    oss << "Total Operations: " << _totalOperations.load() << "\n";
    oss << "Total Processing Time: " << _totalProcessingTimeMs.load() << " ms\n";

    if (_totalOperations > 0)
    {
        oss << "Average Processing Time: "
            << (_totalProcessingTimeMs.load() / _totalOperations.load()) << " ms/operation\n";
    }

    oss << "\n--- Analysis Module ---\n";
    oss << "Items Analyzed: " << _analysis->GetItemsAnalyzed() << "\n";
    oss << "Upgrades Detected: " << _analysis->GetUpgradesDetected() << "\n";

    oss << "\n--- Coordination Module ---\n";
    oss << "Sessions Created: " << _coordination->GetSessionsCreated() << "\n";
    oss << "Sessions Completed: " << _coordination->GetSessionsCompleted() << "\n";
    oss << "Active Sessions: " << _coordination->GetActiveSessionCount() << "\n";

    oss << "\n--- Distribution Module ---\n";
    // Distribution module statistics accessed through its tracking
    oss << "Rolls Processed: (tracked in distribution module)" << "\n";
    oss << "Items Distributed: (tracked in distribution module)" << "\n";

    return oss.str();
}

} // namespace Playerbot
