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
#include "Core/Events/GenericEventBus.h"
#include "ProfessionEvents.h"
#include "ProfessionEvents.h"
#include "../Social/AuctionHouse.h"
#include "../AI/BotAI.h"
#include "../Session/BotSession.h"
#include "../Core/Managers/GameSystemsManager.h"
#include "../Core/PlayerBotHelpers.h"  // GetBotAI, GetGameSystems
#include "Player.h"
#include "Item.h"
#include "Bag.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "ItemTemplate.h"
#include <algorithm>
#include "GameTime.h"

namespace Playerbot
{

// ============================================================================
// STATIC MEMBER INITIALIZATION
// ============================================================================

// NOTE: AuctionHouse is now per-bot (Phase 7), access via GetGameSystems(_bot)->GetAuctionHouse()
// Temporary: Re-enabled for linker compatibility until full refactoring is complete
AuctionHouse* ProfessionAuctionBridge::_auctionHouse = nullptr;
ProfessionAuctionStatistics ProfessionAuctionBridge::_globalStatistics;
bool ProfessionAuctionBridge::_sharedDataInitialized = false;

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

ProfessionAuctionBridge::ProfessionAuctionBridge(Player* bot)
    : _bot(bot)
{
    // CRITICAL: Do NOT call _bot->GetName() in constructor!
    // Bot may not be fully in world yet during GameSystemsManager::Initialize(),
    // and Player::m_name is not initialized, causing ACCESS_VIOLATION.
    // Logging with bot identity deferred to first Update() call.
}

ProfessionAuctionBridge::~ProfessionAuctionBridge()
{
    // CRITICAL: No logging in destructors - can throw std::bad_alloc during memory pressure
    // Event bus unsubscription handled automatically by ProfessionEventBus
}

// ============================================================================
// CORE BRIDGE MANAGEMENT
// ============================================================================

void ProfessionAuctionBridge::Initialize()
{
    if (!_bot)
        return;

    // Load shared data once (thread-safe via static initialization)
    if (!_sharedDataInitialized)
    {
        // NOTE: AuctionHouse is per-bot (Phase 7). Access via GetGameSystems(_bot)->GetAuctionHouse()
        // in per-instance methods. No AH calls needed during shared initialization.
        LoadDefaultStockpileConfigs();
        _sharedDataInitialized = true;
        TC_LOG_INFO("playerbot", "ProfessionAuctionBridge::Initialize - Loaded shared data (stockpile configs)");
    }

    // Subscribe to ProfessionEventBus for event-driven reactivity (Phase 2)
    EventBus<ProfessionEvent>::instance()->SubscribeCallback(
        [this](ProfessionEvent const& event) { HandleProfessionEvent(event); },
        {
            ProfessionEventType::CRAFTING_COMPLETED,
            ProfessionEventType::ITEM_BANKED
        }
    );

    // CRITICAL: Do NOT call _bot->GetName() in Initialize()!
    // Bot may not be fully in world yet during GameSystemsManager::Initialize(),
    // and Player::m_name is not initialized, causing ACCESS_VIOLATION.
    // Logging with bot identity deferred to first Update() call.
}

void ProfessionAuctionBridge::Update(::Player* player, uint32 /*diff*/)
{
    if (!_bot || !IsEnabled(player))
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();

    // Check if enough time passed since last auction check
    if (currentTime - _lastAuctionCheckTime < AUCTION_CHECK_INTERVAL)
        return;

    _lastAuctionCheckTime = currentTime;

    // Sell excess materials
    if (_profile.autoSellEnabled && _profile.strategy != ProfessionAuctionStrategy::NONE)
    {
        SellExcessMaterials(player);
        SellCraftedItems(player);
    }

    // Buy materials for leveling
    if (_profile.buyMaterialsForLeveling)
    {
        // Check all professions for leveling needs
        ProfessionManager* profMgr = GetProfessionManager();
        if (profMgr)
        {
            auto professions = profMgr->GetPlayerProfessions();
            for (auto const& profInfo : professions)
            {
                if (profInfo.currentSkill < profInfo.maxSkill)
                    BuyMaterialsForLeveling(player, profInfo.profession);
            }
        }
    }
}

void ProfessionAuctionBridge::SetEnabled(::Player* /*player*/, bool enabled)
{
    if (!_bot)
        return;

    if (enabled)
    {
        // Initialize with default profile if needed
        if (!_profile.autoSellEnabled)
            _profile = ProfessionAuctionProfile();
    }
    else
    {
        _profile.autoSellEnabled = false;
        _activeAuctionIds.clear();
    }
    _profile.autoSellEnabled = enabled;
}

bool ProfessionAuctionBridge::IsEnabled(::Player* /*player*/) const
{
    return _bot && _profile.autoSellEnabled;
}

void ProfessionAuctionBridge::SetAuctionProfile(uint32 /*playerGuid*/, ProfessionAuctionProfile const& profile)
{
    _profile = profile;
}

ProfessionAuctionProfile ProfessionAuctionBridge::GetAuctionProfile(uint32 /*playerGuid*/) const
{
    return _profile;
}

// ============================================================================
// MATERIAL AUCTION AUTOMATION
// ============================================================================

void ProfessionAuctionBridge::SellExcessMaterials(::Player* player)
{
    if (!_bot || !_auctionHouse || !CanAccessAuctionHouse())
        return;

    // Get profession items in inventory
    auto items = GetProfessionItemsInInventory(true); // Materials only

    for (auto const& itemInfo : items)
    {
        // Check if item has stockpile config
        auto configIt = _profile.materialConfigs.find(itemInfo.itemId);
        if (configIt == _profile.materialConfigs.end())
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
    if (!_bot)
        return false;

    auto configIt = _profile.materialConfigs.find(itemId);
    if (configIt == _profile.materialConfigs.end())
        return false;

    MaterialStockpileConfig const& config = configIt->second;

    // Sell if current count exceeds maxStackSize
    return currentCount > config.maxStackSize;
}

bool ProfessionAuctionBridge::ListMaterialOnAuction(::Player* player, uint32 itemGuid, MaterialStockpileConfig const& config)
{
    if (!_bot || !_auctionHouse)
        return false;

    // Get optimal price from AuctionHouse
    uint32 marketPrice = GetOptimalMaterialPrice(player, config.itemId, config.auctionStackSize);
    if (marketPrice == 0)
    {
        TC_LOG_DEBUG("playerbots", "ProfessionAuctionBridge: No market price found for item {}", config.itemId);
        return false;
    }

    // Calculate bid and buyout prices
    uint32 bidPrice = static_cast<uint32>(marketPrice * 0.95f); // 95% of market for bid
    uint32 buyoutPrice = config.preferBuyout ? marketPrice : 0;

    // Delegate to existing AuctionHouse
    bool success = _auctionHouse->CreateAuction(itemGuid, config.auctionStackSize,
        bidPrice, buyoutPrice, DEFAULT_AUCTION_DURATION);

    if (success)
    {
        _statistics.materialsListedCount++;
        _globalStatistics.materialsListedCount++;

        TC_LOG_INFO("playerbots", "ProfessionAuctionBridge: Listed material {} for bot {} (price: {})",
            config.itemId, _bot->GetName(), marketPrice);
    }

    return success;
}

uint32 ProfessionAuctionBridge::GetOptimalMaterialPrice(::Player* player, uint32 itemId, uint32 stackSize) const
{
    if (!_bot || !_auctionHouse)
        return 0;

    // Delegate to existing AuctionHouse price calculation
    return _auctionHouse->CalculateOptimalListingPrice(itemId, stackSize);
}

// ============================================================================
// CRAFTED ITEM AUCTION AUTOMATION
// ============================================================================

void ProfessionAuctionBridge::SellCraftedItems(::Player* player)
{
    if (!_bot || !_auctionHouse || !CanAccessAuctionHouse())
        return;

    // Get crafted items in inventory
    auto items = GetProfessionItemsInInventory(false); // All profession items

    for (auto const& itemInfo : items)
    {
        ProfessionType profession;
        if (!IsCraftedItem(itemInfo.itemId, profession))
            continue;

        // Check if item has auction config
        auto configIt = _profile.craftedItemConfigs.find(itemInfo.itemId);
        if (configIt == _profile.craftedItemConfigs.end())
            continue;

        CraftedItemAuctionConfig const& config = configIt->second;

        // Calculate material cost
        uint32 materialCost = CalculateMaterialCost(itemInfo.itemId);

        // Check if should sell
        if (ShouldSellCraftedItem(player, itemInfo.itemId, materialCost))
        {
            ListCraftedItemOnAuction(player, itemInfo.itemGuid, config);
        }
    }
}

bool ProfessionAuctionBridge::ShouldSellCraftedItem(::Player* player, uint32 itemId, uint32 materialCost) const
{
    if (!_bot || !_auctionHouse)
        return false;

    auto configIt = _profile.craftedItemConfigs.find(itemId);
    if (configIt == _profile.craftedItemConfigs.end())
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
    if (!_bot || !_auctionHouse)
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
    bool success = _auctionHouse->CreateAuction(itemGuid, 1, // Single item
        bidPrice, listingPrice, config.maxListingDuration);

    if (success)
    {
        _statistics.craftedsListedCount++;
        _globalStatistics.craftedsListedCount++;

        TC_LOG_INFO("playerbots", "ProfessionAuctionBridge: Listed crafted item {} for bot {} (price: {})",
            config.itemId, _bot->GetName(), listingPrice);
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
    if (!_bot || !_auctionHouse || !CanAccessAuctionHouse())
        return;

    // Get needed materials from ProfessionManager
    auto neededMaterials = GetNeededMaterialsForLeveling(player, profession);

    if (neededMaterials.empty())
        return;

    uint32 budgetRemaining = _profile.auctionBudget;

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
        {
            uint32 totalCost = maxPricePerUnit * quantity;
            if (totalCost <= budgetRemaining)
            {
                if (PurchaseMaterial(player, itemId, quantity, maxPricePerUnit))
                {
                    budgetRemaining -= totalCost;

                    _statistics.materialsBought += quantity;
                    _statistics.goldSpentOnMaterials += totalCost;
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

    if (!_bot)
        return materials;

    // Get optimal leveling recipe from ProfessionManager via facade
    ProfessionManager* profMgr = const_cast<ProfessionAuctionBridge*>(this)->GetProfessionManager();
    if (!profMgr)
        return materials;

    RecipeInfo const* recipe = profMgr->GetOptimalLevelingRecipe(profession);
    if (!recipe)
        return materials;

    // Get missing materials for recipe
    return profMgr->GetMissingMaterials(*recipe);
}

bool ProfessionAuctionBridge::IsMaterialAvailableForPurchase(::Player* player, uint32 itemId, uint32 quantity, uint32 maxPricePerUnit) const
{
    if (!_bot || !_auctionHouse)
        return false;

    // Delegate to existing AuctionHouse
    float marketPrice = _auctionHouse->GetMarketPrice(itemId, quantity);
    if (marketPrice == 0.0f)
        return false;

    // Check if price is below our maximum
    return _auctionHouse->IsPriceBelowMarket(itemId, maxPricePerUnit);
}

bool ProfessionAuctionBridge::PurchaseMaterial(::Player* player, uint32 itemId, uint32 quantity, uint32 maxPricePerUnit)
{
    if (!_bot || !_auctionHouse)
        return false;

    // Get similar auctions for this item
    std::vector<AuctionItem> auctions = _auctionHouse->GetSimilarAuctions(itemId, 50);
    if (auctions.empty())
    {
        TC_LOG_DEBUG("playerbots", "ProfessionAuctionBridge: No auctions found for item {}", itemId);
        return false;
    }

    // Sort by price per item (lowest first)
    std::sort(auctions.begin(), auctions.end(),
        [](const AuctionItem& a, const AuctionItem& b) {
            return a.pricePerItem < b.pricePerItem;
        });

    uint32 totalBought = 0;
    uint32 totalGoldSpent = 0;
    std::vector<uint32> boughtAuctionIds;

    for (const AuctionItem& auction : auctions)
    {
        if (totalBought >= quantity)
            break;

        // Skip if price too high
        if (auction.buyoutPrice > 0)
        {
            uint32 pricePerUnit = auction.buyoutPrice / auction.stackCount;
            if (pricePerUnit > maxPricePerUnit)
                continue;
        }
        else
        {
            // No buyout, skip (we only use buyout for automation)
            continue;
        }

        // Calculate how many we can buy from this auction
        uint32 buyAmount = std::min(quantity - totalBought, auction.stackCount);
        uint32 cost = (auction.buyoutPrice / auction.stackCount) * buyAmount;

        // Check if bot has enough gold
        if (_bot->GetMoney() < totalGoldSpent + cost)
        {
            TC_LOG_DEBUG("playerbots", "ProfessionAuctionBridge: Bot {} insufficient gold for material purchase",
                _bot->GetName());
            break;
        }

        // Buyout the auction
        if (_auctionHouse->BuyoutAuction(auction.auctionId))
        {
            totalBought += buyAmount;
            totalGoldSpent += cost;
            boughtAuctionIds.push_back(auction.auctionId);

            TC_LOG_INFO("playerbots", "ProfessionAuctionBridge: Bot {} bought {} x {} for {} gold (auction {})",
                _bot->GetName(), buyAmount, itemId, cost, auction.auctionId);
        }
        else
        {
            TC_LOG_WARN("playerbots", "ProfessionAuctionBridge: Failed to buyout auction {} for bot {}",
                auction.auctionId, _bot->GetName());
        }
    }

    if (totalBought > 0)
    {
        TC_LOG_INFO("playerbots", "ProfessionAuctionBridge: Bot {} purchased total {} x {} for {} gold from {} auctions",
            _bot->GetName(), totalBought, itemId, totalGoldSpent, boughtAuctionIds.size());
        return true;
    }

    return false;
}

// ============================================================================
// STOCKPILE MANAGEMENT
// ============================================================================

void ProfessionAuctionBridge::SetMaterialStockpile(uint32 playerGuid, uint32 itemId, MaterialStockpileConfig const& config)
{
    _profile.materialConfigs[itemId] = config;
}

void ProfessionAuctionBridge::SetCraftedItemAuction(uint32 playerGuid, uint32 itemId, CraftedItemAuctionConfig const& config)
{
    _profile.craftedItemConfigs[itemId] = config;
}

uint32 ProfessionAuctionBridge::GetCurrentStockpile(::Player* player, uint32 itemId) const
{
    if (!_bot)
        return 0;

    return _bot->GetItemCount(itemId);
}

bool ProfessionAuctionBridge::IsStockpileTargetMet(::Player* player, uint32 itemId) const
{
    if (!_bot)
        return false;

    auto configIt = _profile.materialConfigs.find(itemId);
    if (configIt == _profile.materialConfigs.end())
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

ProfessionAuctionStatistics const& ProfessionAuctionBridge::GetGlobalStatistics() const
{
    return _globalStatistics;
}

void ProfessionAuctionBridge::ResetStatistics(uint32 playerGuid)
{
    _statistics.Reset();
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

std::vector<ProfessionAuctionBridge::ItemInfo> ProfessionAuctionBridge::GetProfessionItemsInInventory(bool materialsOnly) const
{
    std::vector<ItemInfo> items;

    if (!_bot)
        return items;

    // Scan all bags
    for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (::Bag* bag = _bot->GetBagByPos(i))
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
        if (::Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
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

uint32 ProfessionAuctionBridge::CalculateMaterialCost(uint32 itemId) const
{
    if (!_bot || !_auctionHouse)
        return 0;

    // Search all production professions for recipe that creates this item
    static const std::vector<ProfessionType> productionProfessions = {
        ProfessionType::ALCHEMY,
        ProfessionType::BLACKSMITHING,
        ProfessionType::ENCHANTING,
        ProfessionType::ENGINEERING,
        ProfessionType::INSCRIPTION,
        ProfessionType::JEWELCRAFTING,
        ProfessionType::LEATHERWORKING,
        ProfessionType::TAILORING
    };

    // Get ProfessionManager via facade
    ProfessionManager* profMgr = const_cast<ProfessionAuctionBridge*>(this)->GetProfessionManager();
    if (!profMgr)
        return 0;

    for (ProfessionType profession : productionProfessions)
    {
        // Get all recipes for this profession
        std::vector<RecipeInfo> recipes = profMgr->GetRecipesForProfession(profession);

        for (const RecipeInfo& recipe : recipes)
        {
            if (recipe.productItemId == itemId)
            {
                // Found the recipe! Calculate total material cost
                uint32 totalCost = 0;

                for (const RecipeInfo::Reagent& reagent : recipe.reagents)
                {
                    // Get market price for reagent
                    uint32 reagentPrice = static_cast<uint32>(_auctionHouse->GetMarketPrice(reagent.itemId, reagent.quantity));
                    totalCost += reagentPrice;
                }

                TC_LOG_DEBUG("playerbots", "ProfessionAuctionBridge: Calculated material cost for item {}: {} gold (from {} reagents)",
                    itemId, totalCost, recipe.reagents.size());

                return totalCost;
            }
        }
    }

    // No recipe found for this item - might be gathered or vendor-bought
    TC_LOG_DEBUG("playerbots", "ProfessionAuctionBridge: No recipe found for item {} (not a crafted item)",
        itemId);

    return 0;
}

bool ProfessionAuctionBridge::IsProfessionMaterial(uint32 itemId) const
{
    // Get item template to check class/subclass
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
    if (!itemTemplate)
        return false;

    // Check if item is a profession material based on item class
    uint32 itemClass = itemTemplate->GetClass();
    uint32 itemSubClass = itemTemplate->GetSubClass();

    // Trade goods are profession materials
    if (itemClass == ITEM_CLASS_TRADE_GOODS)
        return true;

    // Reagents are profession materials
    if (itemClass == ITEM_CLASS_REAGENT)
        return true;

    // Some consumables are used for professions (e.g., vials, threads)
    if (itemClass == ITEM_CLASS_CONSUMABLE)
    {
        // Check specific subclasses that are profession-related
        // ITEM_SUBCLASS_CONSUMABLE (various crafting reagents like thread, vials, etc.)
        return true; // Conservative: include all consumables
    }

    // Quest items sometimes used in profession quests
    if (itemClass == ITEM_CLASS_QUEST)
        return false; // Don't auto-sell quest items

    // Not a profession material
    return false;
}

bool ProfessionAuctionBridge::IsCraftedItem(uint32 itemId, ProfessionType& outProfession) const
{
    // Search all production professions for recipe that creates this item
    static const std::vector<ProfessionType> productionProfessions = {
        ProfessionType::ALCHEMY,
        ProfessionType::BLACKSMITHING,
        ProfessionType::ENCHANTING,
        ProfessionType::ENGINEERING,
        ProfessionType::INSCRIPTION,
        ProfessionType::JEWELCRAFTING,
        ProfessionType::LEATHERWORKING,
        ProfessionType::TAILORING,
        ProfessionType::COOKING // Cooking also creates items
    };

    for (ProfessionType profession : productionProfessions)
    {
        // LIMITATION: Recipe lookup requires per-bot ProfessionManager context (GameSystemsManager).
        // This method is called without bot context, so recipe iteration is a no-op.
        // Enhancement: Pass Player* parameter to enable GameSystems()->GetProfessionManager() lookup.
        std::vector<RecipeInfo> recipes;

        for (const RecipeInfo& recipe : recipes)
        {
            if (recipe.productItemId == itemId)
            {
                outProfession = profession;
                return true;
            }
        }
    }

    // Not a crafted item
    outProfession = ProfessionType::NONE;
    return false;
}

bool ProfessionAuctionBridge::CanAccessAuctionHouse() const
{
    if (!_bot)
        return false;

    // For automated bot trading, we'll use a relaxed check:
    // 1. Bot must be in a city (safe zone)
    // 2. Or have recently interacted with auction house

    // LIMITATION: Proper rest area check (REST_FLAG_IN_CITY) would restrict to cities only.
    // Currently assumes bot has access if alive — acceptable for economy simulation.
    if (_bot->IsAlive())
    {
        TC_LOG_DEBUG("playerbots", "ProfessionAuctionBridge: Bot {} has auction access",
            _bot->GetName());
        return true;
    }

    // Alternative: Check if bot is in a zone with an auction house
    // Major cities with auction houses:
    // - Stormwind, Ironforge, Darnassus (Alliance)
    // - Orgrimmar, Undercity, Thunder Bluff (Horde)
    // - Neutral: Booty Bay, Gadgetzan, Everlook, etc.
    uint32 zoneId = _bot->GetZoneId();

    // Alliance cities
    if (zoneId == 1519 ||  // Stormwind
        zoneId == 1537 ||  // Ironforge
        zoneId == 1657)    // Darnassus
    {
        return true;
    }

    // Horde cities
    if (zoneId == 1637 ||  // Orgrimmar
        zoneId == 1497 ||  // Undercity
        zoneId == 1638)    // Thunder Bluff
    {
        return true;
    }

    // Neutral auction houses
    if (zoneId == 33 ||    // Booty Bay
        zoneId == 1938 ||  // Gadgetzan
        zoneId == 2057 ||  // Everlook
        zoneId == 3487)    // Shattrath
    {
        return true;
    }

    // For bots, allow access from anywhere (they can teleport/travel)
    // In a real implementation, you might want to trigger bot travel to AH
    TC_LOG_DEBUG("playerbots", "ProfessionAuctionBridge: Bot {} not near auction house (zone {}), allowing anyway for automation",
        _bot->GetName(), zoneId);

    return true; // Allow for automation purposes
}

// ============================================================================
// PROFESSION MANAGER ACCESS
// ============================================================================

ProfessionManager* ProfessionAuctionBridge::GetProfessionManager()
{
    if (!_bot)
        return nullptr;

    BotSession* session = static_cast<BotSession*>(_bot->GetSession());
    if (!session || !session->GetAI())
        return nullptr;

    return session->GetAI()->GetGameSystems()->GetProfessionManager();
}

// ============================================================================
// EVENT HANDLING (Phase 2)
// ============================================================================

void ProfessionAuctionBridge::HandleProfessionEvent(ProfessionEvent const& event)
{
    // PHASE 4.3: Filter events for this bot only
    if (!_bot)
        return;

    if (event.playerGuid != _bot->GetGUID())
        return;

    switch (event.type)
    {
        case ProfessionEventType::CRAFTING_COMPLETED:
        {
            // When crafting completes, consider listing crafted items for sale on AH
            TC_LOG_DEBUG("playerbot.events.profession",
                "ProfessionAuctionBridge: CRAFTING_COMPLETED event - Item {} x{} crafted from recipe {}",
                event.itemId, event.quantity, event.recipeId);

            // Check if enabled for profession-auction automation
            if (!IsEnabled(_bot))
                return;

            // Sell crafted items automatically (implementation would call SellCraftedItems)
            TC_LOG_INFO("playerbots",
                "ProfessionAuctionBridge: Processing crafted item {} x{} for potential AH listing",
                event.itemId, event.quantity);
            break;
        }

        case ProfessionEventType::ITEM_BANKED:
        {
            // When items are banked, update inventory tracking and stockpile management
            TC_LOG_DEBUG("playerbot.events.profession",
                "ProfessionAuctionBridge: ITEM_BANKED event - Item {} x{} banked",
                event.itemId, event.quantity);

            // Check if this is a profession material we track
            if (IsProfessionMaterial(event.itemId))
            {
                // Recalculate sellable materials now that inventory changed
                TC_LOG_DEBUG("playerbots",
                    "ProfessionAuctionBridge: Profession material {} x{} banked, recalculating sellable materials",
                    event.itemId, event.quantity);

                // Future enhancement: Trigger sellable materials recalculation
                // This would analyze current stockpiles vs configured thresholds
            }
            break;
        }

        default:
            // Ignore other event types
            break;
    }
}

ProfessionAuctionStatistics const& ProfessionAuctionBridge::GetPlayerStatistics(uint32 playerGuid) const
{
    // Since this is now per-bot instance, return the bot's statistics
    // The playerGuid parameter is for interface compatibility but we use _bot's stats
    if (!_bot)
    {
        TC_LOG_ERROR("playerbot.professionauction", "ProfessionAuctionBridge::GetPlayerStatistics: null bot!");
        return _globalStatistics; // Return global stats as fallback
    }

    // Return bot's local statistics
    return _statistics;
}

} // namespace Playerbot
