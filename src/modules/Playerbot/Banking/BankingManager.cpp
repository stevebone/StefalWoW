/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * **Phase 5.1: BankingManager - Per-Bot Instance Implementation**
 *
 * Converted from singleton to per-bot instance pattern.
 * Each bot has its own BankingManager owned by GameSystemsManager.
 */

#include "BankingManager.h"
#include "../Config/PlayerbotConfig.h"
#include "../Professions/ProfessionManager.h"
#include "../Professions/GatheringMaterialsBridge.h"
#include "../Professions/ProfessionAuctionBridge.h"
#include "../AI/BotAI.h"
#include "../Session/BotSession.h"
#include "ObjectMgr.h"
#include "ItemTemplate.h"
#include "Player.h"
#include "Item.h"
#include "Bag.h"
#include "Log.h"
#include "World.h"
#include "WorldSession.h"
#include "GameTime.h"
#include "Creature.h"
#include "Map.h"
#include "Unit.h"
#include "MotionMaster.h"
#include <algorithm>

namespace Playerbot
{

// ============================================================================
// STATIC MEMBER INITIALIZATION
// ============================================================================

std::vector<BankingRule> BankingManager::_defaultRules;
BankingStatistics BankingManager::_globalStatistics;
bool BankingManager::_defaultRulesInitialized = false;

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

BankingManager::BankingManager(Player* bot)
    : BehaviorManager(bot, static_cast<BotSession*>(bot->GetSession())->GetAI(), 1000, "BankingManager")
    , _bot(bot)
    , _lastBankAccessTime(0)
    , _currentlyBanking(false)
    , _enabled(true)
{
    // CRITICAL: Do NOT access _bot->GetName() in constructor!
    // Bot may not be fully in world yet during GameSystemsManager::Initialize(),
    // and Player::m_name is not initialized, causing ACCESS_VIOLATION.
}

BankingManager::~BankingManager()
{
    // CRITICAL: Do NOT call _bot->GetName() in destructor!
    // During destruction, _bot may be in invalid state where GetName() returns
    // garbage data, causing ACCESS_VIOLATION or std::bad_alloc.
    // IsInWorld() guard is NOT reliable during destruction sequence.
}

// ============================================================================
// LIFECYCLE (BehaviorManager override)
// ============================================================================

void BankingManager::Update(uint32 diff)
{
    if (!_bot)
        return;

    if (!_enabled)
        return;

    // Throttle updates
    uint32 now = GameTime::GetGameTimeMS();
    if (now - _lastBankAccessTime < _profile.bankCheckInterval)
        return;

    // Check if already banking
    if (_currentlyBanking)
        return;

    // Check if near banker
    if (!IsNearBanker())
    {
        // Check if we should travel to banker
        bool needsBank = false;

        if (_profile.autoDepositGold && ShouldDepositGold())
            needsBank = true;

        if (_profile.autoDepositMaterials)
        {
            // Count free bag slots in player's inventory
            uint32 freeSlots = 0;

            // Count free slots in backpack (main inventory)
            for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
            {
                if (!_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                    ++freeSlots;
            }

            // Count free slots in equipped bags
            for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
            {
                if (Bag* pBag = _bot->GetBagByPos(bag))
                {
                    for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
                    {
                        if (!pBag->GetItemByPos(slot))
                            ++freeSlots;
                    }
                }
            }

            if (freeSlots < 10)
                needsBank = true;
        }

        if (needsBank && _profile.travelToBankerWhenNeeded)
        {
            TravelToNearestBanker();
        }

        return;
    }

    // Near banker, perform banking operations
    _currentlyBanking = true;

    // Auto-deposit gold
    if (_profile.autoDepositGold && ShouldDepositGold())
    {
        uint32 amount = GetRecommendedGoldDeposit();
        if (amount > 0)
            DepositGold(amount);
    }

    // Auto-deposit materials
    if (_profile.autoDepositMaterials)
        DepositExcessItems();

    // Auto-withdraw for crafting
    if (_profile.autoWithdrawForCrafting)
        WithdrawMaterialsForCrafting();

    _lastBankAccessTime = now;
    _currentlyBanking = false;
}

// OnShutdown removed - not in interface

// ============================================================================
// CORE BANKING OPERATIONS
// ============================================================================

void BankingManager::SetEnabled(bool enabled)
{
    _enabled = enabled;
}

bool BankingManager::IsEnabled() const
{
    return _enabled;
}

void BankingManager::SetBankingProfile(BotBankingProfile const& profile)
{
    _profile = profile;
}

BotBankingProfile BankingManager::GetBankingProfile() const
{
    return _profile;
}

void BankingManager::AddBankingRule(BankingRule const& rule)
{
    _profile.customRules.push_back(rule);
}

void BankingManager::RemoveBankingRule(uint32 itemId)
{
    _profile.customRules.erase(
        std::remove_if(_profile.customRules.begin(), _profile.customRules.end(),
            [itemId](BankingRule const& rule) { return rule.itemId == itemId; }),
        _profile.customRules.end());
}

// ============================================================================
// GOLD MANAGEMENT
// ============================================================================

bool BankingManager::DepositGold(uint32 amount)
{
    if (!_bot || amount == 0)
        return false;

    if (_bot->GetMoney() < amount)
        amount = _bot->GetMoney();

    if (amount == 0)
        return false;

    // In WoW, the player's bank doesn't actually store gold separately from inventory
    // Gold is simply player money. The "bank gold" concept is handled via a custom
    // tracking system for bots. We store the "banked gold" amount in the bot profile.
    // For actual banking functionality, we simply log the transaction and track it.
    //
    // Note: Real WoW banks just store items, not gold. Guild banks store gold.
    // For bot economy simulation, we track gold "deposited" conceptually.
    //
    // If you want actual gold removal from inventory (simulating deposit):
    // _bot->ModifyMoney(-static_cast<int64>(amount));
    //
    // For now, we just track the transaction for statistics without actually
    // moving gold (since bank gold isn't a real WoW concept for players).

    TC_LOG_DEBUG("playerbot", "BankingManager: Bot {} recorded gold deposit of {} copper ({} gold)",
        _bot->GetName(), amount, amount / 10000);

    // Record transaction
    BankingTransaction transaction;
    transaction.type = BankingTransaction::Type::DEPOSIT_GOLD;
    transaction.timestamp = GameTime::GetGameTimeMS();
    transaction.goldAmount = amount;
    transaction.reason = "Auto-deposit gold";
    RecordTransaction(transaction);

    // Update statistics
    _statistics.totalDeposits++;
    _statistics.goldDeposited += amount;
    _globalStatistics.totalDeposits++;
    _globalStatistics.goldDeposited += amount;

    return true;
}

bool BankingManager::WithdrawGold(uint32 amount)
{
    if (!_bot || amount == 0)
        return false;

    // In WoW, player banks don't store gold - only items. Gold is tracked as player money.
    // For bot economy simulation, we conceptually track "banked gold" but actual gold
    // is stored in player money. See DepositGold() comments for full explanation.
    //
    // If implementing actual gold withdrawal (simulating withdrawal):
    // _bot->ModifyMoney(static_cast<int64>(amount));

    TC_LOG_DEBUG("playerbot", "BankingManager: Bot {} recorded gold withdrawal of {} copper ({} gold)",
        _bot->GetName(), amount, amount / 10000);

    // Record transaction
    BankingTransaction transaction;
    transaction.type = BankingTransaction::Type::WITHDRAW_GOLD;
    transaction.timestamp = GameTime::GetGameTimeMS();
    transaction.goldAmount = amount;
    transaction.reason = "Auto-withdraw gold";
    RecordTransaction(transaction);

    // Update statistics
    _statistics.totalWithdrawals++;
    _statistics.goldWithdrawn += amount;
    _globalStatistics.totalWithdrawals++;
    _globalStatistics.goldWithdrawn += amount;

    return true;
}

bool BankingManager::ShouldDepositGold()
{
    if (!_bot)
        return false;

    uint64 currentGold = _bot->GetMoney();
    return currentGold > _profile.maxGoldInInventory;
}

bool BankingManager::ShouldWithdrawGold()
{
    if (!_bot)
        return false;

    uint64 currentGold = _bot->GetMoney();
    return currentGold < _profile.minGoldInInventory;
}

uint32 BankingManager::GetRecommendedGoldDeposit()
{
    if (!_bot)
        return 0;

    uint64 currentGold = _bot->GetMoney();
    if (currentGold <= _profile.maxGoldInInventory)
        return 0;

    // Deposit excess gold, keep maxGoldInInventory
    uint32 excess = static_cast<uint32>(currentGold - _profile.maxGoldInInventory);
    return excess;
}

// ============================================================================
// ITEM MANAGEMENT
// ============================================================================

bool BankingManager::DepositItem(uint32 itemGuid, uint32 quantity)
{
    if (!_bot)
        return false;

    // Find the item in player's inventory by GUID counter
    Item* itemToDeposit = nullptr;
    uint16 srcPos = 0;

    // Search backpack
    for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (item && item->GetGUID().GetCounter() == itemGuid)
        {
            itemToDeposit = item;
            srcPos = (INVENTORY_SLOT_BAG_0 << 8) | i;
            break;
        }
    }

    // Search bags if not found in backpack
    if (!itemToDeposit)
    {
        for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
        {
            if (Bag* pBag = _bot->GetBagByPos(bag))
            {
                for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
                {
                    Item* item = pBag->GetItemByPos(slot);
                    if (item && item->GetGUID().GetCounter() == itemGuid)
                    {
                        itemToDeposit = item;
                        srcPos = (bag << 8) | slot;
                        break;
                    }
                }
            }
            if (itemToDeposit)
                break;
        }
    }

    if (!itemToDeposit)
    {
        TC_LOG_DEBUG("playerbot", "BankingManager: Bot {} failed to deposit item {} - not found",
            _bot->GetName(), itemGuid);
        return false;
    }

    // Find a free bank slot to deposit into
    // First try the main bank slots (character bank tabs)
    uint16 dstPos = 0;
    bool foundBankSlot = false;
    uint8 numBankTabs = _bot->GetCharacterBankTabCount();

    // Iterate through bank bags
    for (uint8 bankBagIdx = 0; bankBagIdx < numBankTabs && bankBagIdx < (BANK_SLOT_BAG_END - BANK_SLOT_BAG_START); ++bankBagIdx)
    {
        uint8 bankSlot = BANK_SLOT_BAG_START + bankBagIdx;
        if (Bag* bankBag = _bot->GetBagByPos(bankSlot))
        {
            for (uint32 slot = 0; slot < bankBag->GetBagSize(); ++slot)
            {
                if (!bankBag->GetItemByPos(slot))
                {
                    dstPos = (bankSlot << 8) | slot;
                    foundBankSlot = true;
                    break;
                }
            }
            if (foundBankSlot)
                break;
        }
    }

    if (!foundBankSlot)
    {
        TC_LOG_DEBUG("playerbot", "BankingManager: Bot {} failed to deposit item {} - no bank space",
            _bot->GetName(), itemGuid);
        return false;
    }

    // Perform the swap from inventory to bank using TrinityCore's SwapItem
    _bot->SwapItem(srcPos, dstPos);

    TC_LOG_DEBUG("playerbot", "BankingManager: Bot {} deposited item {} to bank",
        _bot->GetName(), itemGuid);

    // Record transaction
    BankingTransaction transaction;
    transaction.type = BankingTransaction::Type::DEPOSIT_ITEM;
    transaction.timestamp = GameTime::GetGameTimeMS();
    transaction.itemId = itemGuid;
    transaction.quantity = quantity;
    transaction.reason = "Auto-deposit item";
    RecordTransaction(transaction);

    // Update statistics
    _statistics.totalDeposits++;
    _statistics.itemsDeposited += quantity;
    _globalStatistics.totalDeposits++;
    _globalStatistics.itemsDeposited += quantity;

    return true;
}

bool BankingManager::WithdrawItem(uint32 itemId, uint32 quantity)
{
    if (!_bot)
        return false;

    // Find the item in player's bank by item ID
    Item* itemToWithdraw = nullptr;
    uint16 srcPos = 0;
    uint32 totalWithdrawn = 0;

    // Get number of unlocked bank tabs
    uint8 numBankTabs = _bot->GetCharacterBankTabCount();

    // Search through bank bags for the item
    for (uint8 bankBagIdx = 0; bankBagIdx < numBankTabs && bankBagIdx < (BANK_SLOT_BAG_END - BANK_SLOT_BAG_START); ++bankBagIdx)
    {
        uint8 bankSlot = BANK_SLOT_BAG_START + bankBagIdx;
        Bag* bankBag = _bot->GetBagByPos(bankSlot);
        if (!bankBag)
            continue;

        for (uint32 slot = 0; slot < bankBag->GetBagSize(); ++slot)
        {
            Item* item = bankBag->GetItemByPos(slot);
            if (!item || item->GetEntry() != itemId)
                continue;

            // Found matching item in bank
            itemToWithdraw = item;
            srcPos = (bankSlot << 8) | slot;

            // Find a free inventory slot to place the item
            uint16 dstPos = 0;
            bool foundInvSlot = false;

            // First try backpack
            for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
            {
                if (!_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                {
                    dstPos = (INVENTORY_SLOT_BAG_0 << 8) | i;
                    foundInvSlot = true;
                    break;
                }
            }

            // Try equipped bags if backpack is full
            if (!foundInvSlot)
            {
                for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
                {
                    Bag* invBag = _bot->GetBagByPos(bag);
                    if (!invBag)
                        continue;

                    for (uint32 invSlot = 0; invSlot < invBag->GetBagSize(); ++invSlot)
                    {
                        if (!invBag->GetItemByPos(invSlot))
                        {
                            dstPos = (bag << 8) | invSlot;
                            foundInvSlot = true;
                            break;
                        }
                    }
                    if (foundInvSlot)
                        break;
                }
            }

            if (!foundInvSlot)
            {
                TC_LOG_DEBUG("playerbot", "BankingManager: Bot {} failed to withdraw item {} - no inventory space",
                    _bot->GetName(), itemId);
                return totalWithdrawn > 0;
            }

            // Perform the swap from bank to inventory
            _bot->SwapItem(srcPos, dstPos);

            uint32 itemCount = item->GetCount();
            totalWithdrawn += itemCount;

            TC_LOG_DEBUG("playerbot", "BankingManager: Bot {} withdrew {} x{} from bank",
                _bot->GetName(), itemId, itemCount);

            // Check if we have enough
            if (totalWithdrawn >= quantity)
                break;
        }

        if (totalWithdrawn >= quantity)
            break;
    }

    if (totalWithdrawn == 0)
    {
        TC_LOG_DEBUG("playerbot", "BankingManager: Bot {} failed to withdraw item {} - not found in bank",
            _bot->GetName(), itemId);
        return false;
    }

    // Record transaction
    BankingTransaction transaction;
    transaction.type = BankingTransaction::Type::WITHDRAW_ITEM;
    transaction.timestamp = GameTime::GetGameTimeMS();
    transaction.itemId = itemId;
    transaction.quantity = totalWithdrawn;
    transaction.reason = "Auto-withdraw item";
    RecordTransaction(transaction);

    // Update statistics
    _statistics.totalWithdrawals++;
    _statistics.itemsWithdrawn += totalWithdrawn;
    _globalStatistics.totalWithdrawals++;
    _globalStatistics.itemsWithdrawn += totalWithdrawn;

    return true;
}

bool BankingManager::ShouldDepositItem(uint32 itemId, uint32 currentCount)
{
    BankingRule const* rule = FindBankingRule(itemId);
    if (!rule)
        return false;

    if (!rule->enabled)
        return false;

    if (rule->priority == BankingPriority::NEVER_BANK)
        return false;

    // Check if count exceeds max inventory threshold
    if (currentCount > rule->maxInInventory)
        return true;

    // Check priority level
    if (rule->priority == BankingPriority::CRITICAL)
        return true;

    return false;
}

BankingPriority BankingManager::GetItemBankingPriority(uint32 itemId)
{
    BankingRule const* rule = FindBankingRule(itemId);
    if (rule)
        return rule->priority;

    return CalculateItemPriority(itemId);
}

void BankingManager::DepositExcessItems()
{
    if (!_bot)
        return;

    std::vector<DepositCandidate> candidates = GetDepositCandidates();

    // Sort by priority (highest first)
    std::sort(candidates.begin(), candidates.end(),
        [](DepositCandidate const& a, DepositCandidate const& b) {
            return static_cast<uint8>(a.priority) > static_cast<uint8>(b.priority);
        });

    // Deposit items
    for (auto const& candidate : candidates)
    {
        if (!HasBankSpace())
            break;

        DepositItem(candidate.itemGuid, candidate.quantity);
    }
}

void BankingManager::WithdrawMaterialsForCrafting()
{
    if (!_bot)
        return;

    std::vector<WithdrawRequest> requests = GetWithdrawRequests();

    for (auto const& request : requests)
    {
        WithdrawItem(request.itemId, request.quantity);
    }
}

// ============================================================================
// BANK SPACE ANALYSIS
// ============================================================================

BankSpaceInfo BankingManager::GetBankSpaceInfo()
{
    BankSpaceInfo info;

    if (!_bot)
        return info;

    // Get number of unlocked bank tabs
    uint8 numBankTabs = _bot->GetCharacterBankTabCount();

    // Count total slots and used slots across all bank bags
    uint32 totalSlots = 0;
    uint32 usedSlots = 0;

    for (uint8 bankBagIdx = 0; bankBagIdx < numBankTabs && bankBagIdx < (BANK_SLOT_BAG_END - BANK_SLOT_BAG_START); ++bankBagIdx)
    {
        uint8 bankSlot = BANK_SLOT_BAG_START + bankBagIdx;
        Bag* bankBag = _bot->GetBagByPos(bankSlot);
        if (!bankBag)
            continue;

        uint32 bagSize = bankBag->GetBagSize();
        totalSlots += bagSize;

        for (uint32 slot = 0; slot < bagSize; ++slot)
        {
            if (bankBag->GetItemByPos(slot))
                ++usedSlots;
        }
    }

    // Default bank size if no bank tabs unlocked (base character bank)
    // WoW standard bank has 28 base slots + additional bags
    if (totalSlots == 0)
        totalSlots = 28;  // Base bank slots

    info.totalSlots = totalSlots;
    info.usedSlots = usedSlots;
    info.freeSlots = totalSlots - usedSlots;

    return info;
}

bool BankingManager::HasBankSpace(uint32 slotsNeeded)
{
    BankSpaceInfo info = GetBankSpaceInfo();
    return info.freeSlots >= slotsNeeded;
}

uint32 BankingManager::GetItemCountInBank(uint32 itemId)
{
    if (!_bot)
        return 0;

    uint32 totalCount = 0;

    // Get number of unlocked bank tabs
    uint8 numBankTabs = _bot->GetCharacterBankTabCount();

    // Search through all bank bags for the item
    for (uint8 bankBagIdx = 0; bankBagIdx < numBankTabs && bankBagIdx < (BANK_SLOT_BAG_END - BANK_SLOT_BAG_START); ++bankBagIdx)
    {
        uint8 bankSlot = BANK_SLOT_BAG_START + bankBagIdx;
        Bag* bankBag = _bot->GetBagByPos(bankSlot);
        if (!bankBag)
            continue;

        for (uint32 slot = 0; slot < bankBag->GetBagSize(); ++slot)
        {
            Item* item = bankBag->GetItemByPos(slot);
            if (item && item->GetEntry() == itemId)
                totalCount += item->GetCount();
        }
    }

    return totalCount;
}

bool BankingManager::IsItemInBank(uint32 itemId)
{
    return GetItemCountInBank(itemId) > 0;
}

void BankingManager::OptimizeBankSpace()
{
    if (!_bot)
        return;

    // Get number of unlocked bank tabs
    uint8 numBankTabs = _bot->GetCharacterBankTabCount();

    // Build a map of stackable items and their locations
    struct StackInfo
    {
        uint32 itemId;
        uint8 bagSlot;
        uint32 slotInBag;
        uint32 count;
        uint32 maxStack;
    };

    std::vector<StackInfo> stackableItems;

    // First pass: collect all stackable items
    for (uint8 bankBagIdx = 0; bankBagIdx < numBankTabs && bankBagIdx < (BANK_SLOT_BAG_END - BANK_SLOT_BAG_START); ++bankBagIdx)
    {
        uint8 bankSlot = BANK_SLOT_BAG_START + bankBagIdx;
        Bag* bankBag = _bot->GetBagByPos(bankSlot);
        if (!bankBag)
            continue;

        for (uint32 slot = 0; slot < bankBag->GetBagSize(); ++slot)
        {
            Item* item = bankBag->GetItemByPos(slot);
            if (!item)
                continue;

            ItemTemplate const* itemTemplate = item->GetTemplate();
            if (!itemTemplate || itemTemplate->GetMaxStackSize() <= 1)
                continue;

            // Item is stackable and not at max stack
            if (item->GetCount() < itemTemplate->GetMaxStackSize())
            {
                StackInfo info;
                info.itemId = item->GetEntry();
                info.bagSlot = bankSlot;
                info.slotInBag = slot;
                info.count = item->GetCount();
                info.maxStack = itemTemplate->GetMaxStackSize();
                stackableItems.push_back(info);
            }
        }
    }

    // Second pass: consolidate stacks of the same item
    for (size_t i = 0; i < stackableItems.size(); ++i)
    {
        StackInfo& src = stackableItems[i];
        if (src.count == 0)  // Already merged
            continue;

        for (size_t j = i + 1; j < stackableItems.size(); ++j)
        {
            StackInfo& dst = stackableItems[j];
            if (dst.count == 0 || src.itemId != dst.itemId)
                continue;

            // Try to merge src into dst
            uint32 spaceInDst = dst.maxStack - dst.count;
            if (spaceInDst == 0)
                continue;

            uint32 toMove = std::min(src.count, spaceInDst);

            // Get the actual items
            Bag* srcBag = _bot->GetBagByPos(src.bagSlot);
            Bag* dstBag = _bot->GetBagByPos(dst.bagSlot);
            if (!srcBag || !dstBag)
                continue;

            Item* srcItem = srcBag->GetItemByPos(src.slotInBag);
            Item* dstItem = dstBag->GetItemByPos(dst.slotInBag);
            if (!srcItem || !dstItem)
                continue;

            // Perform the stack combination using SwapItem
            // SwapItem handles stack combining automatically
            uint16 srcPos = (src.bagSlot << 8) | src.slotInBag;
            uint16 dstPos = (dst.bagSlot << 8) | dst.slotInBag;
            _bot->SwapItem(srcPos, dstPos);

            // Update tracking
            dst.count += toMove;
            src.count -= toMove;

            if (src.count == 0)
                break;  // Source item is exhausted
        }
    }

    TC_LOG_DEBUG("playerbot", "BankingManager: Bot {} optimized bank space, consolidated {} stackable item types",
        _bot->GetName(), stackableItems.size());
}

// ============================================================================
// BANKER ACCESS
// ============================================================================

bool BankingManager::IsNearBanker()
{
    if (!_bot)
        return false;

    float distance = GetDistanceToNearestBanker();
    return distance <= _profile.maxDistanceToBanker;
}

float BankingManager::GetDistanceToNearestBanker()
{
    if (!_bot)
        return 999999.0f;

    Map* map = _bot->GetMap();
    if (!map)
        return 999999.0f;

    float closestDist = 999999.0f;

    // Search for bankers using UNIT_NPC_FLAG_BANKER
    // Use FindNearestCreatureWithOptions or iterate nearby creatures
    // Search radius of 100 yards initially
    float searchRadius = 100.0f;

    // Try to find a banker nearby using cell-based search
    // TrinityCore provides various ways to find creatures:
    // - ObjectAccessor::GetCreature() if we know GUID
    // - Map::GetCreatureBySpawnId() if we know spawn ID
    // - Cell-based iteration for nearby creatures

    // For performance, use a simple distance check against known banker positions
    // First, check if there's a banker in nearby cells
    Creature* nearestBanker = nullptr;

    // INTEGRATION REQUIRED: Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck API usage
    // Needs: Research proper Trinity 12.0 API for grid-based creature searches
    // Current: Using Map::GetCreatureBySpawnIdStore() iteration (works but not optimal)
    // For now, use a simpler approach by iterating nearby creatures

    // Find nearest banker by iterating nearby creatures manually
    // TrinityCore 11.x: Use simple creature search approach
    float minDist = searchRadius;

    // Check map for creatures within range - using TrinityCore spawn id store
    Map* botMap = _bot->GetMap();
    if (botMap)
    {
        // Use TrinityCore's creature spawn store to find bankers
        for (auto const& [spawnId, creature] : botMap->GetCreatureBySpawnIdStore())
        {
            if (creature && creature->IsAlive() &&
                creature->IsWithinDistInMap(_bot, searchRadius) &&
                creature->HasNpcFlag(NPCFlags(UNIT_NPC_FLAG_BANKER)))
            {
                float dist = _bot->GetDistance(creature);
                if (dist < minDist)
                {
                    minDist = dist;
                    nearestBanker = creature;
                }
            }
        }
    }

    if (nearestBanker)
    {
        closestDist = _bot->GetDistance(nearestBanker);
    }

    // Store the nearest banker for TravelToNearestBanker
    _cachedNearestBanker = nearestBanker;

    return closestDist;
}

bool BankingManager::TravelToNearestBanker()
{
    if (!_bot)
        return false;

    // Get distance to update cached banker
    float distance = GetDistanceToNearestBanker();

    if (!_cachedNearestBanker || distance >= 999999.0f)
    {
        TC_LOG_DEBUG("playerbot", "BankingManager: Bot {} cannot find banker to travel to", _bot->GetName());
        return false;
    }

    // Get banker position
    Position const& bankerPos = _cachedNearestBanker->GetPosition();

    // Use the bot's movement system to travel to the banker
    // The BotAI should have a movement manager we can use
    BotSession* session = static_cast<BotSession*>(_bot->GetSession());
    if (!session || !session->GetAI())
    {
        TC_LOG_DEBUG("playerbot", "BankingManager: Bot {} has no AI for movement", _bot->GetName());
        return false;
    }

    // Set movement target to banker position
    // The AI will handle pathfinding and movement
    // Note: MotionMaster is forward-declared in Unit.h, need to include it
    if (auto* motionMaster = _bot->GetMotionMaster())
    {
        motionMaster->MovePoint(0, bankerPos);
    }
    else
    {
        TC_LOG_DEBUG("playerbot", "BankingManager: Bot {} has no MotionMaster", _bot->GetName());
        return false;
    }

    TC_LOG_DEBUG("playerbot", "BankingManager: Bot {} traveling to banker at ({}, {}, {})",
        _bot->GetName(), bankerPos.GetPositionX(), bankerPos.GetPositionY(), bankerPos.GetPositionZ());

    _statistics.bankTrips++;
    _globalStatistics.bankTrips++;

    return true;
}

// ============================================================================
// TRANSACTION HISTORY
// ============================================================================

std::vector<BankingTransaction> BankingManager::GetRecentTransactions(uint32 count)
{
    std::vector<BankingTransaction> result;

    size_t start = _transactionHistory.size() > static_cast<size_t>(count) ? _transactionHistory.size() - static_cast<size_t>(count) : 0;
    result.assign(_transactionHistory.begin() + start, _transactionHistory.end());

    return result;
}

void BankingManager::RecordTransaction(BankingTransaction const& transaction)
{
    _transactionHistory.push_back(transaction);

    // Keep only last MAX_TRANSACTION_HISTORY transactions
    if (_transactionHistory.size() > MAX_TRANSACTION_HISTORY)
    {
        _transactionHistory.erase(_transactionHistory.begin(),
            _transactionHistory.begin() + (_transactionHistory.size() - MAX_TRANSACTION_HISTORY));
    }
}

// ============================================================================
// STATISTICS
// ============================================================================

BankingStatistics const& BankingManager::GetStatistics() const
{
    return _statistics;
}

BankingStatistics const& BankingManager::GetGlobalStatistics()
{
    return _globalStatistics;
}

void BankingManager::ResetStatistics()
{
    _statistics.Reset();
}

// ============================================================================
// INITIALIZATION HELPERS
// ============================================================================

void BankingManager::InitializeDefaultRules()
{
    // Copy default rules to this bot's profile
    _profile.customRules = _defaultRules;
}

void BankingManager::LoadBankingRules()
{
    // Initialize default banking rules (shared across all bots)
    _defaultRules.clear();

    // Rule: Never bank equipped items
    // (Handled by quest item check, no rule needed)

    // Rule: Bank excess trade goods (keep 40 in inventory)
    BankingRule tradeGoodsRule;
    tradeGoodsRule.itemClass = 7;  // ITEM_CLASS_TRADE_GOODS
    tradeGoodsRule.priority = BankingPriority::HIGH;
    tradeGoodsRule.keepInInventory = 40;
    tradeGoodsRule.maxInInventory = 80;
    _defaultRules.push_back(tradeGoodsRule);

    // Rule: Bank crafted items (keep 20 in inventory for selling)
    BankingRule craftedRule;
    craftedRule.itemClass = 1;  // ITEM_CLASS_CONSUMABLE or crafted
    tradeGoodsRule.priority = BankingPriority::MEDIUM;
    craftedRule.keepInInventory = 20;
    craftedRule.maxInInventory = 60;
    _defaultRules.push_back(craftedRule);

    TC_LOG_DEBUG("playerbot", "BankingManager: Loaded {} default banking rules", _defaultRules.size());
}

// ============================================================================
// BANKING LOGIC HELPERS
// ============================================================================

BankingRule const* BankingManager::FindBankingRule(uint32 itemId)
{
    // Check custom rules first
    for (auto const& rule : _profile.customRules)
    {
        if (ItemMatchesRule(itemId, rule))
            return &rule;
    }

    return nullptr;
}

BankingPriority BankingManager::CalculateItemPriority(uint32 itemId)
{
    if (!_bot)
        return BankingPriority::LOW;

    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
    if (!itemTemplate)
        return BankingPriority::LOW;

    // Quest items - never bank
    if (itemTemplate->GetStartQuest() != 0 || itemTemplate->FoodType != 0)
        return BankingPriority::NEVER_BANK;

    // Profession materials - check if needed
    if (IsNeededForProfessions(itemId))
        return BankingPriority::LOW;  // Keep in inventory

    // Default priority based on quality
    if (itemTemplate->GetQuality() >= 4)  // Epic+
        return BankingPriority::HIGH;
    else if (itemTemplate->GetQuality() >= 3)  // Rare
        return BankingPriority::MEDIUM;
    else
        return BankingPriority::LOW;
}

bool BankingManager::ItemMatchesRule(uint32 itemId, BankingRule const& rule)
{
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
    if (!itemTemplate)
        return false;

    // Specific item ID match
    if (rule.itemId != 0 && rule.itemId == itemId)
        return true;

    // Item class match
    if (rule.itemClass != 0)
    {
        if (itemTemplate->GetClass() != rule.itemClass)
            return false;

        // Item subclass match (if specified)
        if (rule.itemSubClass != 0 && itemTemplate->GetSubClass() != rule.itemSubClass)
            return false;

        // Quality match (if specified)
        if (rule.itemQuality != 0 && itemTemplate->GetQuality() != rule.itemQuality)
            return false;

        return true;
    }

    return false;
}

std::vector<BankingManager::DepositCandidate> BankingManager::GetDepositCandidates()
{
    std::vector<DepositCandidate> candidates;

    if (!_bot)
        return candidates;

    // Scan inventory for items to deposit
    for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (!item)
            continue;

        uint32 itemId = item->GetEntry();
        uint32 count = item->GetCount();

        if (ShouldDepositItem(itemId, count))
        {
            DepositCandidate candidate;
            candidate.itemGuid = item->GetGUID().GetCounter();
            candidate.itemId = itemId;
            candidate.quantity = count;
            candidate.priority = GetItemBankingPriority(itemId);
            candidates.push_back(candidate);
        }
    }

    // Scan bags
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        Bag* pBag = _bot->GetBagByPos(bag);
        if (!pBag)
            continue;

        for (uint32 slot = 0; slot < pBag->GetBagSize(); ++slot)
        {
            Item* item = pBag->GetItemByPos(slot);
            if (!item)
                continue;

            uint32 itemId = item->GetEntry();
            uint32 count = item->GetCount();

            if (ShouldDepositItem(itemId, count))
            {
                DepositCandidate candidate;
                candidate.itemGuid = item->GetGUID().GetCounter();
                candidate.itemId = itemId;
                candidate.quantity = count;
                candidate.priority = GetItemBankingPriority(itemId);
                candidates.push_back(candidate);
            }
        }
    }

    return candidates;
}

std::vector<BankingManager::WithdrawRequest> BankingManager::GetWithdrawRequests()
{
    std::vector<WithdrawRequest> requests;

    if (!_bot)
        return requests;

    // Get material needs from ProfessionManager
    ProfessionManager* profMgr = GetProfessionManager();
    if (!profMgr)
        return requests;

    // Query ProfessionManager for needed materials for leveling
    // Check each profession the bot has and get materials needed for optimal leveling recipe
    std::vector<ProfessionSkillInfo> botProfessions = profMgr->GetPlayerProfessions();

    for (auto const& professionInfo : botProfessions)
    {
        if (professionInfo.profession == ProfessionType::NONE)
            continue;

        // Skip gathering professions - they don't need materials
        ProfessionCategory category = profMgr->GetProfessionCategory(professionInfo.profession);
        if (category == ProfessionCategory::GATHERING)
            continue;

        // Get the optimal leveling recipe for this profession
        RecipeInfo const* optimalRecipe = profMgr->GetOptimalLevelingRecipe(professionInfo.profession);
        if (!optimalRecipe)
            continue;

        // Check if we're missing materials for this recipe
        if (profMgr->HasMaterialsForRecipe(*optimalRecipe))
            continue;

        // Get missing materials
        auto missingMaterials = profMgr->GetMissingMaterials(*optimalRecipe);

        for (auto const& material : missingMaterials)
        {
            uint32 itemId = material.first;
            uint32 neededQty = material.second;

            // Check if we have any of this item in the bank
            uint32 bankCount = GetItemCountInBank(itemId);
            if (bankCount == 0)
                continue;

            // Create withdraw request for what we have in bank (up to needed amount)
            uint32 withdrawQty = std::min(bankCount, neededQty);

            WithdrawRequest request;
            request.itemId = itemId;
            request.quantity = withdrawQty;
            request.reason = "Crafting material for profession leveling";
            requests.push_back(request);
        }
    }

    return requests;
}

// ============================================================================
// INTEGRATION HELPERS
// ============================================================================

bool BankingManager::IsNeededForProfessions(uint32 itemId)
{
    ProfessionManager* profMgr = GetProfessionManager();
    if (!profMgr)
        return false;

    // Check if item is needed for any of the bot's professions
    // by examining the crafting recipes that require this material
    std::vector<ProfessionSkillInfo> botProfessions = profMgr->GetPlayerProfessions();

    for (auto const& professionInfo : botProfessions)
    {
        if (professionInfo.profession == ProfessionType::NONE)
            continue;

        // Skip gathering professions - they don't use materials
        ProfessionCategory category = profMgr->GetProfessionCategory(professionInfo.profession);
        if (category == ProfessionCategory::GATHERING)
            continue;

        // Get all craftable recipes for this profession
        std::vector<RecipeInfo> craftableRecipes = profMgr->GetCraftableRecipes(professionInfo.profession);

        // Check if any recipe uses this item as a reagent
        for (auto const& recipe : craftableRecipes)
        {
            for (auto const& reagent : recipe.reagents)
            {
                if (reagent.itemId == itemId)
                    return true;  // Item is needed for this recipe
            }
        }

        // Also check the optimal leveling recipe
        RecipeInfo const* optimalRecipe = profMgr->GetOptimalLevelingRecipe(professionInfo.profession);
        if (optimalRecipe)
        {
            for (auto const& reagent : optimalRecipe->reagents)
            {
                if (reagent.itemId == itemId)
                    return true;
            }
        }
    }

    return false;  // Item not needed for any profession
}

uint32 BankingManager::GetMaterialPriorityFromProfessions(uint32 itemId)
{
    ProfessionManager* profMgr = GetProfessionManager();
    if (!profMgr)
        return 0;

    // Get material priority from ProfessionManager
    // Priority is based on:
    // 1. Whether item is needed for current optimal leveling recipe (highest priority = 100)
    // 2. Whether item is needed for any craftable recipe (medium priority = 50)
    // 3. Whether item is a profession material at all (low priority = 25)
    // 0 = not a profession material

    uint32 maxPriority = 0;
    std::vector<ProfessionSkillInfo> botProfessions = profMgr->GetPlayerProfessions();

    for (auto const& professionInfo : botProfessions)
    {
        if (professionInfo.profession == ProfessionType::NONE)
            continue;

        // Skip gathering professions - they produce materials, don't consume them
        ProfessionCategory category = profMgr->GetProfessionCategory(professionInfo.profession);
        if (category == ProfessionCategory::GATHERING)
            continue;

        // Check if needed for optimal leveling recipe (highest priority)
        RecipeInfo const* optimalRecipe = profMgr->GetOptimalLevelingRecipe(professionInfo.profession);
        if (optimalRecipe)
        {
            for (auto const& reagent : optimalRecipe->reagents)
            {
                if (reagent.itemId == itemId)
                {
                    // Needed for current optimal leveling - highest priority
                    return 100;  // Maximum priority
                }
            }
        }

        // Check if needed for any craftable recipe (medium priority)
        std::vector<RecipeInfo> craftableRecipes = profMgr->GetCraftableRecipes(professionInfo.profession);
        for (auto const& recipe : craftableRecipes)
        {
            for (auto const& reagent : recipe.reagents)
            {
                if (reagent.itemId == itemId)
                {
                    // Calculate priority based on recipe skill-up chance
                    float skillUpChance = profMgr->GetSkillUpChance(recipe);
                    uint32 recipePriority = 25 + static_cast<uint32>(skillUpChance * 50);  // 25-75 range
                    maxPriority = std::max(maxPriority, recipePriority);
                }
            }
        }

        // Also check all recipes for this profession (not just craftable)
        // to determine if it's a profession material at all
        std::vector<RecipeInfo> allRecipes = profMgr->GetRecipesForProfession(professionInfo.profession);
        for (auto const& recipe : allRecipes)
        {
            for (auto const& reagent : recipe.reagents)
            {
                if (reagent.itemId == itemId && maxPriority == 0)
                {
                    // It's a profession material, but not currently needed
                    maxPriority = 25;  // Base priority for profession materials
                }
            }
        }
    }

    return maxPriority;
}

ProfessionManager* BankingManager::GetProfessionManager()
{
    if (!_bot)
        return nullptr;

    BotSession* session = static_cast<BotSession*>(_bot->GetSession());
    if (!session || !session->GetAI())
        return nullptr;

    return session->GetAI()->GetGameSystems()->GetProfessionManager();
}

bool BankingManager::Initialize()
{
    if (!_bot)
    {
        TC_LOG_ERROR("playerbot", "BankingManager::Initialize: null bot!");
        return false;
    }

    // CRITICAL: Do NOT call _bot->GetName() in Initialize()!
    // Bot may not be fully in world yet during GameSystemsManager::Initialize(),
    // and Player::m_name is not initialized, causing ACCESS_VIOLATION.
    // Logging with bot identity deferred to first Update() call.

    // Load configuration from PlayerbotConfig
    _enabled = sPlayerbotConfig->GetBool("Playerbot.Banking.Enable", true);
    _profile.bankCheckInterval = sPlayerbotConfig->GetInt("Playerbot.Banking.CheckInterval", 300000);
    _profile.autoDepositGold = sPlayerbotConfig->GetBool("Playerbot.Banking.AutoDeposit", true);
    _profile.minGoldInInventory = sPlayerbotConfig->GetInt("Playerbot.Banking.KeepGoldAmount", 10000);

    TC_LOG_DEBUG("playerbot", "BankingManager: Config loaded - Enable=%d, CheckInterval=%u, AutoDeposit=%d",
        _enabled, _profile.bankCheckInterval, _profile.autoDepositGold);

    if (!_enabled)
    {
        TC_LOG_DEBUG("playerbot", "BankingManager: Disabled via config");
        return true;  // Not an error, just disabled
    }

    // Initialize default rules if not already done
    if (!_defaultRulesInitialized)
    {
        InitializeDefaultRules();
        _defaultRulesInitialized = true;
    }

    // Load bot-specific rules
    LoadBankingRules();

    // Reset state
    _currentlyBanking = false;
    _lastBankAccessTime = 0;
    _enabled = true;

    // Logging with bot identity deferred - do not access _bot->GetName() here

    return true;
}

void BankingManager::Shutdown()
{
    if (!_bot)
    {
        TC_LOG_ERROR("playerbot", "BankingManager::Shutdown: null bot!");
        return;
    }

    // CRITICAL: Do NOT call _bot->GetName() in Shutdown()!
    // During destruction, _bot may be in invalid state where GetName() returns
    // garbage data, causing ACCESS_VIOLATION or std::bad_alloc.

    // Cancel any ongoing banking operation
    if (_currentlyBanking)
    {
        _currentlyBanking = false;
    }

    // Disable manager
    _enabled = false;
}

} // namespace Playerbot
