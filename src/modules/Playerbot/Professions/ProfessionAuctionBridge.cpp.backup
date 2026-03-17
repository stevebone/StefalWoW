/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ProfessionAuctionBridge.h"
#include "ProfessionManager.h"
#include "../Social/AuctionHouse.h"
#include "Player.h"
#include "Item.h"
#include "Bag.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot
{

// Singleton instance
ProfessionAuctionBridge* ProfessionAuctionBridge::instance()
{
    static ProfessionAuctionBridge instance;
    return &instance;
}

ProfessionAuctionBridge::ProfessionAuctionBridge()
    : _auctionHouse(nullptr)
{
}

// ============================================================================
// CORE BRIDGE MANAGEMENT
// ============================================================================

void ProfessionAuctionBridge::Initialize()
{
    TC_LOG_INFO("playerbots", "ProfessionAuctionBridge: Initializing profession-auction bridge...");

    // Get reference to existing AuctionHouse singleton
    _auctionHouse = AuctionHouse::instance();

    if (!_auctionHouse)
    {
        TC_LOG_ERROR("playerbots", "ProfessionAuctionBridge: Failed to get AuctionHouse instance");
        return;
    }

    LoadDefaultStockpileConfigs();

    TC_LOG_INFO("playerbots", "ProfessionAuctionBridge: Initialized (bridge to existing AuctionHouse)");
}

void ProfessionAuctionBridge::Update(::Player* player, uint32 diff)
{
    if (!player || !IsEnabled(player))
        return;

    uint32 playerGuid = player->GetGUID().GetCounter();
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return;
    }
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return;
    }
    uint32 currentTime = getMSTime();

    std::lock_guard<std::recursive_mutex> lock(_mutex);

    // Check if enough time passed since last auction check
    if (currentTime - _lastAuctionCheckTimes[playerGuid] < AUCTION_CHECK_INTERVAL)
        return;

    _lastAuctionCheckTimes[playerGuid] = currentTime;

    ProfessionAuctionProfile const& profile = GetAuctionProfile(playerGuid);

    // Sell excess materials
    if (profile.autoSellEnabled && profile.strategy != ProfessionAuctionStrategy::NONE)
    {
        SellExcessMaterials(player);
        SellCraftedItems(player);
    }

    // Buy materials for leveling
    if (profile.buyMaterialsForLeveling)
    {
        // Check all professions for leveling needs
        auto professions = ProfessionManager::instance()->GetPlayerProfessions(player);
        for (auto const& profInfo : professions)
        {
            if (profInfo.currentSkill < profInfo.maxSkill)
                BuyMaterialsForLeveling(player, profInfo.profession);
        }
    }
}

void ProfessionAuctionBridge::SetEnabled(::Player* player, bool enabled)
if (!player)
{
    TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
    return nullptr;
}
{
    if (!player)
        return;

    std::lock_guard<std::recursive_mutex> lock(_mutex);
    uint32 playerGuid = player->GetGUID().GetCounter();
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return;
    }

    if (_profiles.find(playerGuid) == _profiles.end())
        _profiles[playerGuid] = ProfessionAuctionProfile();
if (!player)
{
    TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
    return nullptr;
}

    _profiles[playerGuid].autoSellEnabled = enabled;
}

bool ProfessionAuctionBridge::IsEnabled(::Player* player) const
{
    if (!player)
        return false;

    std::lock_guard<std::recursive_mutex> lock(_mutex);
    uint32 playerGuid = player->GetGUID().GetCounter();
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return;
    }

    auto it = _profiles.find(playerGuid);
    if (it == _profiles.end())
        return false;

    return it->second.autoSellEnabled;
}

void ProfessionAuctionBridge::SetAuctionProfile(uint32 playerGuid, ProfessionAuctionProfile const& profile)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _profiles[playerGuid] = profile;
}

ProfessionAuctionProfile ProfessionAuctionBridge::GetAuctionProfile(uint32 playerGuid) const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    auto it = _profiles.find(playerGuid);
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return;
    }
    if (it != _profiles.end())
        return it->second;

    return ProfessionAuctionProfile();
}

// ============================================================================
// MATERIAL AUCTION AUTOMATION
// ============================================================================

void ProfessionAuctionBridge::SellExcessMaterials(::Player* player)
{
    if (!player || !_auctionHouse || !CanAccessAuctionHouse(player))
        return;

    uint32 playerGuid = player->GetGUID().GetCounter();
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return;
    }
    ProfessionAuctionProfile const& profile = GetAuctionProfile(playerGuid);

    // Get profession items in inventory
    auto items = GetProfessionItemsInInventory(player, true); // Materials only
        if (!player)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
            return nullptr;
        }

    for (auto const& itemInfo : items)
    {
        // Check if item has stockpile config
        auto configIt = profile.materialConfigs.find(itemInfo.itemId);
        if (configIt == profile.materialConfigs.end())
            continue;

        MaterialStockpileConfig const& config = configIt->second;

        // Check if should sell
        if (ShouldSellMaterial(player, itemInfo.itemId, itemInfo.stackCount))
        {
            ListMaterialOnAuction(player, itemInfo.itemGuid, config);
        }
    }
}

bool ProfessionAuctionBridge::ShouldSellMaterial(::Player* player, uint32 itemId, uint32 currentCount) const
{
    if (!player)
        return false;

    ProfessionAuctionProfile const& profile = GetAuctionProfile(player->GetGUID().GetCounter());
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return nullptr;
    }

    auto configIt = profile.materialConfigs.find(itemId);
    if (configIt == profile.materialConfigs.end())
        return false;

    MaterialStockpileConfig const& config = configIt->second;

    // Sell if current count exceeds maxStackSize
    return currentCount > config.maxStackSize;
}

bool ProfessionAuctionBridge::ListMaterialOnAuction(::Player* player, uint32 itemGuid, MaterialStockpileConfig const& config)
if (!player)
{
    TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
    return;
}
{
    if (!player || !_auctionHouse)
        return false;

    // Get optimal price from AuctionHouse
    uint32 marketPrice = GetOptimalMaterialPrice(player, config.itemId, config.auctionStackSize);
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
        return 0;
    }

    if (marketPrice == 0)
    {
        TC_LOG_DEBUG("playerbots", "ProfessionAuctionBridge: No market price found for item {}", config.itemId);
        return false;
    }

    // Calculate bid and buyout prices
    uint32 bidPrice = static_cast<uint32>(marketPrice * 0.95f); // 95% of market for bid
    uint32 buyoutPrice = config.preferBuyout ? marketPrice : 0;

    // Delegate to existing AuctionHouse
    bool success = _auctionHouse->CreateAuction(player, itemGuid, config.auctionStackSize,
        bidPrice, buyoutPrice, DEFAULT_AUCTION_DURATION);

    if (success)
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        uint32 playerGuid = player->GetGUID().GetCounter();
        if (!player)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
            return;
        }
        if (!player)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
            return;
        }
            if (!player)
            {
                TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
                return;
            }

        _playerStatistics[playerGuid].materialsListedCount++;
        _globalStatistics.materialsListedCount++;

        TC_LOG_INFO("playerbots", "ProfessionAuctionBridge: Listed material {} for player {} (price: {})",
            config.itemId, player->GetName(), marketPrice);
    }

    return success;
}

uint32 ProfessionAuctionBridge::GetOptimalMaterialPrice(::Player* player, uint32 itemId, uint32 stackSize) const
{
    if (!player || !_auctionHouse)
        return 0;

    // Delegate to existing AuctionHouse price calculation
    return _auctionHouse->CalculateOptimalListingPrice(player, itemId, stackSize);
}

// ============================================================================
// CRAFTED ITEM AUCTION AUTOMATION
// ============================================================================

void ProfessionAuctionBridge::SellCraftedItems(::Player* player)
{
    if (!player || !_auctionHouse || !CanAccessAuctionHouse(player))
        return;

    uint32 playerGuid = player->GetGUID().GetCounter();
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return;
    }
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return;
    }
    ProfessionAuctionProfile const& profile = GetAuctionProfile(playerGuid);

    // Get crafted items in inventory
    auto items = GetProfessionItemsInInventory(player, false); // All profession items

    for (auto const& itemInfo : items)
    {
        ProfessionType profession;
        if (!IsCraftedItem(itemInfo.itemId, profession))
            continue;

        // Check if item has auction config
        auto configIt = profile.craftedItemConfigs.find(itemInfo.itemId);
        if (configIt == profile.craftedItemConfigs.end())
            continue;

        CraftedItemAuctionConfig const& config = configIt->second;

        // Calculate material cost
        uint32 materialCost = CalculateMaterialCost(player, itemInfo.itemId);

        // Check if should sell
        if (ShouldSellCraftedItem(player, itemInfo.itemId, materialCost))
        {
            ListCraftedItemOnAuction(player, itemInfo.itemGuid, config);
        }
    }
}

bool ProfessionAuctionBridge::ShouldSellCraftedItem(::Player* player, uint32 itemId, uint32 materialCost) const
{
    if (!player || !_auctionHouse)
        return false;

    ProfessionAuctionProfile const& profile = GetAuctionProfile(player->GetGUID().GetCounter());
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return nullptr;
    }
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
        return;
    }
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return nullptr;
    }

    auto configIt = profile.craftedItemConfigs.find(itemId);
    if (configIt == profile.craftedItemConfigs.end())
        return false;

    CraftedItemAuctionConfig const& config = configIt->second;

    // Get market price
    uint32 marketPrice = _auctionHouse->GetMarketPrice(itemId, 1);
    if (marketPrice == 0)
        return false;

    // Calculate profit margin
    float profitMargin = CalculateProfitMargin(player, itemId, marketPrice, materialCost);

    // Sell if profit margin exceeds minimum
    uint32 minProfitCopper = config.minProfitMargin;
    uint32 actualProfitCopper = (marketPrice > materialCost) ? (marketPrice - materialCost) : 0;

    return actualProfitCopper >= minProfitCopper;
}

bool ProfessionAuctionBridge::ListCraftedItemOnAuction(::Player* player, uint32 itemGuid, CraftedItemAuctionConfig const& config)
{
    if (!player || !_auctionHouse)
        return false;

    // Get market price
    uint32 marketPrice = _auctionHouse->GetMarketPrice(config.itemId, 1);
    if (marketPrice == 0)
        return false;

    // Apply undercut if configured
    uint32 listingPrice = marketPrice;
    if (config.undercutCompetition)
        listingPrice = static_cast<uint32>(marketPrice * (1.0f - config.undercutRate));

    uint32 bidPrice = static_cast<uint32>(listingPrice * 0.95f);

    // Delegate to existing AuctionHouse
    bool success = _auctionHouse->CreateAuction(player, itemGuid, 1, // Single item
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return;
    }
        bidPrice, listingPrice, config.maxListingDuration);

    if (success)
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        uint32 playerGuid = player->GetGUID().GetCounter();
if (!player)
{
    TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
    return nullptr;
}
        if (!player)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
            return;
        }
            if (!player)
            {
                TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
                return;
            }

        _playerStatistics[playerGuid].craftedsListedCount++;
        _globalStatistics.craftedsListedCount++;

        TC_LOG_INFO("playerbots", "ProfessionAuctionBridge: Listed crafted item {} for player {} (price: {})",
            config.itemId, player->GetName(), listingPrice);
    }

    return success;
}

float ProfessionAuctionBridge::CalculateProfitMargin(::Player* player, uint32 itemId, uint32 marketPrice, uint32 materialCost) const
{
    if (materialCost == 0)
        return 0.0f;

    if (marketPrice <= materialCost)
        return 0.0f;

    uint32 profit = marketPrice - materialCost;
    return static_cast<float>(profit) / static_cast<float>(materialCost);
}

// ============================================================================
// MATERIAL PURCHASING AUTOMATION
// ============================================================================

void ProfessionAuctionBridge::BuyMaterialsForLeveling(::Player* player, ProfessionType profession)
{
    if (!player || !_auctionHouse || !CanAccessAuctionHouse(player))
        return;

    // Get needed materials from ProfessionManager
    auto neededMaterials = GetNeededMaterialsForLeveling(player, profession);

    if (neededMaterials.empty())
        return;

    ProfessionAuctionProfile const& profile = GetAuctionProfile(player->GetGUID().GetCounter());
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return nullptr;
    }
    uint32 budgetRemaining = profile.auctionBudget;

    for (auto const& [itemId, quantity] : neededMaterials)
    {
        // Get market price
        float marketPrice = _auctionHouse->GetMarketPrice(itemId, quantity);
        if (marketPrice == 0.0f)
            continue;

        // Calculate max price willing to pay (110% of market)
        uint32 maxPricePerUnit = static_cast<uint32>(marketPrice * 1.1f);

        // Check if available at good price
        if (IsMaterialAvailableForPurchase(player, itemId, quantity, maxPricePerUnit))
        if (!player)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
            return nullptr;
        }
        {
            uint32 totalCost = maxPricePerUnit * quantity;
            if (totalCost <= budgetRemaining)
            {
                if (PurchaseMaterial(player, itemId, quantity, maxPricePerUnit))
                {
                    budgetRemaining -= totalCost;

                    std::lock_guard<std::recursive_mutex> lock(_mutex);
                    uint32 playerGuid = player->GetGUID().GetCounter();
                    if (!player)
                    {
                        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
                        return;
                    }

                    _playerStatistics[playerGuid].materialsBought += quantity;
                    _playerStatistics[playerGuid].goldSpentOnMaterials += totalCost;
                    _globalStatistics.materialsBought += quantity;
                    _globalStatistics.goldSpentOnMaterials += totalCost;
                }
            }
        }
    }
}

std::vector<std::pair<uint32, uint32>> ProfessionAuctionBridge::GetNeededMaterialsForLeveling(::Player* player, ProfessionType profession) const
{
    std::vector<std::pair<uint32, uint32>> materials;

    if (!player)
        return materials;

    // Get optimal leveling recipe from ProfessionManager
    RecipeInfo const* recipe = ProfessionManager::instance()->GetOptimalLevelingRecipe(player, profession);
    if (!recipe)
        return materials;

    // Get missing materials for recipe
    return ProfessionManager::instance()->GetMissingMaterials(player, *recipe);
}

bool ProfessionAuctionBridge::IsMaterialAvailableForPurchase(::Player* player, uint32 itemId, uint32 quantity, uint32 maxPricePerUnit) const
if (!player)
{
    TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
    return;
}
{
    if (!player || !_auctionHouse)
        return false;

    // Delegate to existing AuctionHouse
    float marketPrice = _auctionHouse->GetMarketPrice(itemId, quantity);
    if (marketPrice == 0.0f)
        return false;

    // Check if price is below our maximum
    return _auctionHouse->IsPriceBelowMarket(itemId, maxPricePerUnit);
}

bool ProfessionAuctionBridge::PurchaseMaterial(::Player* player, uint32 itemId, uint32 quantity, uint32 maxPricePerUnit)
        if (!player)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
            return;
        }
{
    if (!player || !_auctionHouse)
        return false;

    // In full implementation, search for specific auctions and buyout
    // For now, this is a placeholder - actual purchase would use AuctionHouse::BuyoutAuction

    TC_LOG_INFO("playerbots", "ProfessionAuctionBridge: Purchased {} x {} for player {}",
        quantity, itemId, player->GetName());

    return true; // Simplified for now
}

// ============================================================================
// STOCKPILE MANAGEMENT
// ============================================================================

void ProfessionAuctionBridge::SetMaterialStockpile(uint32 playerGuid, uint32 itemId, MaterialStockpileConfig const& config)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    if (_profiles.find(playerGuid) == _profiles.end())
        _profiles[playerGuid] = ProfessionAuctionProfile();

    _profiles[playerGuid].materialConfigs[itemId] = config;
}

void ProfessionAuctionBridge::SetCraftedItemAuction(uint32 playerGuid, uint32 itemId, CraftedItemAuctionConfig const& config)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    if (_profiles.find(playerGuid) == _profiles.end())
        _profiles[playerGuid] = ProfessionAuctionProfile();

    _profiles[playerGuid].craftedItemConfigs[itemId] = config;
}

uint32 ProfessionAuctionBridge::GetCurrentStockpile(::Player* player, uint32 itemId) const
{
    if (!player)
        return 0;

    return player->GetItemCount(itemId);
}

bool ProfessionAuctionBridge::IsStockpileTargetMet(::Player* player, uint32 itemId) const
{
    if (!player)
        return false;

    ProfessionAuctionProfile const& profile = GetAuctionProfile(player->GetGUID().GetCounter());
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return nullptr;
    }

    auto configIt = profile.materialConfigs.find(itemId);
    if (configIt == profile.materialConfigs.end())
        return false;

    uint32 currentCount = GetCurrentStockpile(player, itemId);
    return currentCount >= configIt->second.minStackSize;
}

// ============================================================================
// INTEGRATION WITH EXISTING AUCTION HOUSE
// ============================================================================

AuctionHouse* ProfessionAuctionBridge::GetAuctionHouse() const
{
    return _auctionHouse;
}

void ProfessionAuctionBridge::SynchronizeWithAuctionHouse(::Player* player)
{
    // Synchronization logic with existing auction house
    // In full implementation, check active auctions, update statistics, etc.
}

// ============================================================================
// STATISTICS
// ============================================================================

ProfessionAuctionStatistics const& ProfessionAuctionBridge::GetPlayerStatistics(uint32 playerGuid) const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    static ProfessionAuctionStatistics emptyStats;
    auto it = _playerStatistics.find(playerGuid);
    if (it != _playerStatistics.end())
        return it->second;

    return emptyStats;
}

ProfessionAuctionStatistics const& ProfessionAuctionBridge::GetGlobalStatistics() const
{
    return _globalStatistics;
}

void ProfessionAuctionBridge::ResetStatistics(uint32 playerGuid)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    auto it = _playerStatistics.find(playerGuid);
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetItemByPos");
        return;
    }
    if (it != _playerStatistics.end())
        it->second.Reset();
}

// ============================================================================
// INITIALIZATION HELPERS
// ============================================================================

void ProfessionAuctionBridge::LoadDefaultStockpileConfigs()
{
    InitializeMiningMaterials();
    InitializeHerbalismMaterials();
    InitializeSkinningMaterials();
    InitializeCraftedItemConfigs();
}

void ProfessionAuctionBridge::InitializeMiningMaterials()
{
    // Default configs would be loaded from database in full implementation
    TC_LOG_DEBUG("playerbots", "ProfessionAuctionBridge: Initialized mining material configs");
}

void ProfessionAuctionBridge::InitializeHerbalismMaterials()
{
    TC_LOG_DEBUG("playerbots", "ProfessionAuctionBridge: Initialized herbalism material configs");
}

void ProfessionAuctionBridge::InitializeSkinningMaterials()
{
    TC_LOG_DEBUG("playerbots", "ProfessionAuctionBridge: Initialized skinning material configs");
}

void ProfessionAuctionBridge::InitializeCraftedItemConfigs()
{
    TC_LOG_DEBUG("playerbots", "ProfessionAuctionBridge: Initialized crafted item configs");
}

// ============================================================================
// AUCTION HELPERS
// ============================================================================

std::vector<ProfessionAuctionBridge::ItemInfo> ProfessionAuctionBridge::GetProfessionItemsInInventory(::Player* player, bool materialsOnly) const
{
    std::vector<ItemInfo> items;

    if (!player)
        return items;

    // Scan all bags
    for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (::Bag* bag = player->GetBagByPos(i))
        {
            for (uint32 j = 0; j < bag->GetBagSize(); ++j)
            {
                if (::Item* item = bag->GetItemByPos(j))
                {
                    if (materialsOnly && !IsProfessionMaterial(item->GetEntry()))
                        continue;

                    ItemInfo info;
                    info.itemGuid = item->GetGUID().GetCounter();
                    info.itemId = item->GetEntry();
                    info.stackCount = item->GetCount();
                    info.quality = item->GetTemplate()->GetQuality();
                    items.push_back(info);
                }
            }
        }
    }

    // Scan main backpack
    for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        if (::Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        if (!player)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetItemByPos");
            return nullptr;
        }
        {
            if (materialsOnly && !IsProfessionMaterial(item->GetEntry()))
                continue;

            ItemInfo info;
            info.itemGuid = item->GetGUID().GetCounter();
            info.itemId = item->GetEntry();
            info.stackCount = item->GetCount();
            info.quality = item->GetTemplate()->GetQuality();
            items.push_back(info);
        }
    }

    return items;
}

uint32 ProfessionAuctionBridge::CalculateMaterialCost(::Player* player, uint32 itemId) const
{
    if (!player || !_auctionHouse)
        return 0;

    // In full implementation, get recipe from ProfessionManager
    // and calculate sum of reagent market prices
    return 0; // Simplified
}

bool ProfessionAuctionBridge::IsProfessionMaterial(uint32 itemId) const
{
    // In full implementation, check against profession material database
    // For now, simplified logic
    return true; // Assume all items could be materials
}

bool ProfessionAuctionBridge::IsCraftedItem(uint32 itemId, ProfessionType& outProfession) const
{
    // In full implementation, check against crafted item database
    outProfession = ProfessionType::NONE;
    return false; // Simplified
}

bool ProfessionAuctionBridge::CanAccessAuctionHouse(::Player* player) const
{
    if (!player)
        return false;

    // Check if player is near auction house
    // In full implementation, verify proximity to auctioneer NPC
    return true; // Simplified
}

} // namespace Playerbot
