/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * REAGENT MANAGER Implementation
 *
 * Auto-purchase consumables, reagents, and class materials from vendors.
 * Tracks inventory levels and plans vendor visits during non-combat time.
 *
 * Key Design Decisions:
 * - Singleton pattern: No per-bot state stored; all queries are stateless.
 *   This eliminates memory overhead for hundreds of bots.
 * - Level-bracketed item tables: Food, water, and bandages are selected
 *   based on bot level to match what vendor NPCs typically sell.
 * - Vendor NPC search uses TrinityCore's FindNearestCreature with NPC
 *   flag filtering, not hardcoded creature entries.
 * - Purchase execution delegates to Player::BuyItemFromVendorSlot() which
 *   handles all server-side validation (gold, bag space, level req, etc.)
 * - Repair uses Player::DurabilityRepairAll() after verifying the vendor
 *   has the UNIT_NPC_FLAG_REPAIR capability.
 */

#include "ReagentManager.h"
#include "Creature.h"
#include "CreatureData.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SharedDefines.h"
#include "Timer.h"
#include "UnitDefines.h"
#include <algorithm>
#include <cmath>

namespace Playerbot
{

// ============================================================================
// STATIC ITEM DATA TABLES
// ============================================================================

// Food items by level bracket - common vendor-sold food across expansions.
// These are basic food items that restore health out of combat.
// Listed from lowest level to highest; lookup returns the best match.
std::vector<LevelBracketItem> const& ReagentManager::GetFoodTable()
{
    static const std::vector<LevelBracketItem> table = {
        {  1,   5,  4540, "Tough Hunk of Bread" },
        {  5,  15,  4541, "Freshly Baked Bread" },
        { 15,  25,  4542, "Moist Cornbread" },
        { 25,  35,  4544, "Mulgore Spice Bread" },
        { 35,  45,  4601, "Soft Banana Bread" },
        { 45,  55,  8950, "Homemade Cherry Pie" },
        { 55,  65, 33449, "Crusty Flatbread" },
        { 65,  75, 35950, "Sweet Potato Bread" },
        { 75,  80, 44608, "Grilled Sculpin" },
        { 80,  85, 62290, "Buttered Wheat Roll" },
        { 85,  90, 74649, "Pandaren Treasure Noodle Soup" },
        { 90, 100,116453, "Frosty Stew" },
        {100, 110,133574, "Dried Mackerel Strips" },
        {110, 120,154882, "Sailor's Pie" },
        {120, 130,172043, "Feast of Gluttonous Hedonism" },
        {130, 140,197784, "Fated Fortune Cookie" },
        {140, 150,222728, "Sizzling Honey Roast" },
    };
    return table;
}

// Water/drink items by level bracket - restore mana out of combat.
// Only relevant for mana-using classes.
std::vector<LevelBracketItem> const& ReagentManager::GetWaterTable()
{
    static const std::vector<LevelBracketItem> table = {
        {  1,   5,   159, "Refreshing Spring Water" },
        {  5,  15,  1179, "Ice Cold Milk" },
        { 15,  25,  1205, "Melon Juice" },
        { 25,  35,  1708, "Sweet Nectar" },
        { 35,  45,  1645, "Moonberry Juice" },
        { 45,  55,  8766, "Morning Glory Dew" },
        { 55,  65, 33445, "Honeymint Tea" },
        { 65,  75, 35954, "Sweetened Goat's Milk" },
        { 75,  80, 44610, "Pungent Seal Whey" },
        { 80,  85, 62289, "Highland Water" },
        { 85,  90, 74650, "Pandaren Fruit Juice" },
        { 90, 100,116449, "Blackrock Coffee" },
        {100, 110,133575, "Dried Bilberries" },
        {110, 120,154884, "Coastal Healing Potion" },
        {120, 130,171270, "Potion of Spectral Healing" },
        {130, 140,197786, "Aromatic Seafood Platter" },
        {140, 150,222730, "Tender Twilight Jerky" },
    };
    return table;
}

// Bandage items by level bracket - first aid bandages for self-healing.
std::vector<LevelBracketItem> const& ReagentManager::GetBandageTable()
{
    static const std::vector<LevelBracketItem> table = {
        {  1,  10,  1251, "Linen Bandage" },
        { 10,  20,  2581, "Heavy Linen Bandage" },
        { 20,  30,  3530, "Wool Bandage" },
        { 30,  40,  3531, "Heavy Wool Bandage" },
        { 40,  50, 14529, "Mageweave Bandage" },
        { 50,  60, 14530, "Heavy Mageweave Bandage" },
        { 60,  70, 21990, "Netherweave Bandage" },
        { 70,  80, 21991, "Heavy Netherweave Bandage" },
        { 80,  85, 53049, "Frostweave Bandage" },
        { 85,  90, 53050, "Heavy Frostweave Bandage" },
        { 90, 110, 72986, "Windwool Bandage" },
        {110, 130,133942, "Silkweave Bandage" },
        {130, 150,172072, "Shrouded Cloth Bandage" },
    };
    return table;
}

// Class-specific reagent items.
// In WoW 12.0, most reagent systems are simplified, but some classes
// still benefit from carrying specific materials.
std::unordered_map<uint8, std::vector<uint32>> const& ReagentManager::GetClassReagentTable()
{
    static const std::unordered_map<uint8, std::vector<uint32>> table = {
        // Rogue: Poisons and utility powders
        { CLASS_ROGUE, {
            5140,   // Flash Powder
            5530,   // Blinding Powder
        }},
        // Warlock: Ritual materials (historical, mostly removed in modern WoW)
        { CLASS_WARLOCK, {
            // Most warlock reagents removed in modern expansions
        }},
        // Mage: Arcane materials (historical, most removed)
        { CLASS_MAGE, {
            // Arcane Powder removed in Cataclysm; mages no longer need vendor reagents
        }},
        // Priest: Light materials (historical, removed in Cataclysm+)
        { CLASS_PRIEST, {
            // Holy Candle removed in modern WoW
        }},
        // Shaman: Ankh reagents (removed post-Cataclysm)
        { CLASS_SHAMAN, {
            // Ankh reagent removed; Reincarnation no longer needs reagent
        }},
        // Paladin: Symbol of Kings etc. (removed in Cataclysm+)
        { CLASS_PALADIN, {
            // All paladin reagents removed in modern WoW
        }},
        // Druid: Wild materials (removed in Cataclysm+)
        { CLASS_DRUID, {
            // All druid reagents removed in modern WoW
        }},
    };
    return table;
}

// ============================================================================
// SINGLETON
// ============================================================================

ReagentManager::ReagentManager()
{
    TC_LOG_DEBUG("module.playerbot", "ReagentManager: Initialized with default configuration "
        "(minFood={}, minWater={}, repairThreshold={:.0f}%%)",
        _config.minFoodCount, _config.minWaterCount, _config.repairThresholdPct);
}

ReagentManager* ReagentManager::instance()
{
    static ReagentManager inst;
    return &inst;
}

// ============================================================================
// CORE RESTOCK QUERIES
// ============================================================================

bool ReagentManager::NeedsRestock(Player* bot) const
{
    if (!bot || !bot->IsInWorld() || !bot->IsAlive())
        return false;

    // Don't check while in combat
    if (bot->IsInCombat())
        return false;

    ReagentManagerConfig config = GetConfig();

    if (NeedsFoodRestock(bot, config))
        return true;

    if (NeedsWaterRestock(bot, config))
        return true;

    if (NeedsReagentRestock(bot, config))
        return true;

    if (NeedsBandageRestock(bot, config))
        return true;

    if (NeedsRepair(bot))
        return true;

    return false;
}

RestockPlan ReagentManager::GetRestockList(Player* bot) const
{
    RestockPlan plan;

    if (!bot || !bot->IsInWorld())
        return plan;

    ReagentManagerConfig config = GetConfig();
    uint32 botLevel = bot->GetLevel();
    uint8 botClass = bot->GetClass();

    // ====================================================================
    // FOOD
    // ====================================================================
    uint32 foodId = GetFoodForLevel(botLevel);
    if (foodId != 0)
    {
        uint32 currentFood = bot->GetItemCount(foodId);
        if (currentFood < config.minFoodCount)
        {
            RestockItem item;
            item.itemId = foodId;
            item.currentCount = currentFood;
            item.desiredCount = config.targetFoodCount;
            item.purchaseQuantity = config.targetFoodCount - currentFood;
            item.category = RestockCategory::FOOD;

            ItemTemplate const* tmpl = sObjectMgr->GetItemTemplate(foodId);
            if (tmpl)
                item.name = tmpl->GetName(LOCALE_enUS);
            else
                item.name = "Food (ID: " + std::to_string(foodId) + ")";

            plan.items.push_back(item);
        }
    }

    // ====================================================================
    // WATER (mana-using classes only)
    // ====================================================================
    if (ClassUsesMana(botClass))
    {
        uint32 waterId = GetWaterForLevel(botLevel);
        if (waterId != 0)
        {
            uint32 currentWater = bot->GetItemCount(waterId);
            if (currentWater < config.minWaterCount)
            {
                RestockItem item;
                item.itemId = waterId;
                item.currentCount = currentWater;
                item.desiredCount = config.targetWaterCount;
                item.purchaseQuantity = config.targetWaterCount - currentWater;
                item.category = RestockCategory::WATER;

                ItemTemplate const* tmpl = sObjectMgr->GetItemTemplate(waterId);
                if (tmpl)
                    item.name = tmpl->GetName(LOCALE_enUS);
                else
                    item.name = "Water (ID: " + std::to_string(waterId) + ")";

                plan.items.push_back(item);
            }
        }
    }

    // ====================================================================
    // CLASS REAGENTS
    // ====================================================================
    std::vector<uint32> reagents = GetClassReagents(botClass);
    for (uint32 reagentId : reagents)
    {
        if (reagentId == 0)
            continue;

        uint32 currentCount = bot->GetItemCount(reagentId);
        if (currentCount < config.minReagentCount)
        {
            RestockItem item;
            item.itemId = reagentId;
            item.currentCount = currentCount;
            item.desiredCount = config.targetReagentCount;
            item.purchaseQuantity = config.targetReagentCount - currentCount;
            item.category = RestockCategory::CLASS_REAGENT;

            ItemTemplate const* tmpl = sObjectMgr->GetItemTemplate(reagentId);
            if (tmpl)
                item.name = tmpl->GetName(LOCALE_enUS);
            else
                item.name = "Reagent (ID: " + std::to_string(reagentId) + ")";

            plan.items.push_back(item);
        }
    }

    // ====================================================================
    // BANDAGES
    // ====================================================================
    uint32 bandageId = GetBandageForLevel(botLevel);
    if (bandageId != 0)
    {
        uint32 currentBandages = bot->GetItemCount(bandageId);
        if (currentBandages < config.minBandageCount)
        {
            RestockItem item;
            item.itemId = bandageId;
            item.currentCount = currentBandages;
            item.desiredCount = config.targetBandageCount;
            item.purchaseQuantity = config.targetBandageCount - currentBandages;
            item.category = RestockCategory::BANDAGE;

            ItemTemplate const* tmpl = sObjectMgr->GetItemTemplate(bandageId);
            if (tmpl)
                item.name = tmpl->GetName(LOCALE_enUS);
            else
                item.name = "Bandage (ID: " + std::to_string(bandageId) + ")";

            plan.items.push_back(item);
        }
    }

    // ====================================================================
    // REPAIR STATUS
    // ====================================================================
    plan.lowestDurabilityPct = GetLowestDurabilityPct(bot);
    plan.needsRepair = plan.lowestDurabilityPct < config.repairThresholdPct;
    if (plan.needsRepair)
    {
        plan.estimatedRepairCost = EstimateRepairCost(bot);
    }

    // ====================================================================
    // ESTIMATE TOTAL COST
    // ====================================================================
    // Rough estimate based on item buy prices
    for (auto const& item : plan.items)
    {
        ItemTemplate const* tmpl = sObjectMgr->GetItemTemplate(item.itemId);
        if (tmpl)
        {
            uint64 buyPrice = tmpl->GetBuyPrice();
            uint32 buyCount = tmpl->GetBuyCount();
            if (buyCount == 0)
                buyCount = 1;

            plan.estimatedCost += (buyPrice / buyCount) * item.purchaseQuantity;
        }
    }
    plan.estimatedCost += plan.estimatedRepairCost;

    TC_LOG_DEBUG("module.playerbot", "ReagentManager: Bot {} restock plan: {} items to buy, "
        "repair={}, estimatedCost={}c, lowestDurability={:.1f}%%",
        bot->GetName(), plan.items.size(), plan.needsRepair,
        plan.estimatedCost, plan.lowestDurabilityPct);

    return plan;
}

uint32 ReagentManager::AttemptPurchase(Player* bot, Creature* vendor)
{
    if (!bot || !vendor)
        return 0;

    if (!bot->IsInWorld() || !bot->IsAlive())
        return 0;

    // Verify vendor is actually a vendor
    if (!vendor->IsVendor())
    {
        TC_LOG_DEBUG("module.playerbot", "ReagentManager: Creature {} ({}) is not a vendor",
            vendor->GetName(), vendor->GetEntry());
        return 0;
    }

    // Verify interaction range
    if (!bot->IsWithinDistInMap(vendor, 10.0f))
    {
        TC_LOG_DEBUG("module.playerbot", "ReagentManager: Bot {} is too far from vendor {} ({:.1f} yards)",
            bot->GetName(), vendor->GetName(),
            bot->GetDistance(vendor));
        return 0;
    }

    // Get the restock plan
    RestockPlan plan = GetRestockList(bot);
    if (plan.IsEmpty())
    {
        TC_LOG_DEBUG("module.playerbot", "ReagentManager: Bot {} has no restock needs",
            bot->GetName());
        return 0;
    }

    ReagentManagerConfig config = GetConfig();
    uint32 purchasedCount = 0;

    // Calculate available budget (total gold minus reserve)
    uint64 availableGold = bot->GetMoney();
    if (availableGold <= config.goldReserve)
    {
        TC_LOG_DEBUG("module.playerbot", "ReagentManager: Bot {} gold ({}) is at or below reserve ({}), skipping purchases",
            bot->GetName(), availableGold, config.goldReserve);

        // Still attempt repair if needed (repair is higher priority than reserve)
        if (plan.needsRepair && vendor->IsArmorer())
        {
            if (bot->HasEnoughMoney(static_cast<int64>(plan.estimatedRepairCost)))
            {
                bot->DurabilityRepairAll(true, 0.0f, false);
                TC_LOG_DEBUG("module.playerbot", "ReagentManager: Bot {} repaired equipment at vendor {}",
                    bot->GetName(), vendor->GetName());
            }
        }
        return 0;
    }
    uint64 spendableBudget = availableGold - config.goldReserve;

    // Reserve gold for repair first
    if (plan.needsRepair && plan.estimatedRepairCost > 0)
    {
        if (plan.estimatedRepairCost < spendableBudget)
            spendableBudget -= plan.estimatedRepairCost;
        else
            spendableBudget = 0;
    }

    // Purchase items in order (food/water first as they're most critical for gameplay)
    for (auto const& restockItem : plan.items)
    {
        if (restockItem.purchaseQuantity == 0)
            continue;

        // Check budget
        uint64 itemCost = CalculateItemCost(bot, vendor, restockItem.itemId, restockItem.purchaseQuantity);
        if (itemCost > spendableBudget)
        {
            // Try buying fewer items to fit budget
            uint32 reducedQty = restockItem.purchaseQuantity;
            while (reducedQty > 0)
            {
                itemCost = CalculateItemCost(bot, vendor, restockItem.itemId, reducedQty);
                if (itemCost <= spendableBudget)
                    break;
                reducedQty = reducedQty / 2; // Halve until affordable
            }

            if (reducedQty == 0)
            {
                TC_LOG_DEBUG("module.playerbot", "ReagentManager: Bot {} cannot afford {} (budget={}c)",
                    bot->GetName(), restockItem.name, spendableBudget);
                continue;
            }

            // Attempt reduced purchase
            if (TryPurchaseItem(bot, vendor, restockItem.itemId, reducedQty))
            {
                purchasedCount += reducedQty;
                spendableBudget -= itemCost;
                TC_LOG_DEBUG("module.playerbot", "ReagentManager: Bot {} bought {}x {} (reduced from {})",
                    bot->GetName(), reducedQty, restockItem.name, restockItem.purchaseQuantity);
            }
        }
        else
        {
            // Full purchase
            if (TryPurchaseItem(bot, vendor, restockItem.itemId, restockItem.purchaseQuantity))
            {
                purchasedCount += restockItem.purchaseQuantity;
                spendableBudget -= itemCost;
                TC_LOG_DEBUG("module.playerbot", "ReagentManager: Bot {} bought {}x {}",
                    bot->GetName(), restockItem.purchaseQuantity, restockItem.name);
            }
            else
            {
                TC_LOG_DEBUG("module.playerbot", "ReagentManager: Bot {} failed to buy {} from vendor {}",
                    bot->GetName(), restockItem.name, vendor->GetName());
            }
        }
    }

    // Repair equipment if vendor is an armorer
    if (plan.needsRepair && vendor->IsArmorer())
    {
        if (bot->HasEnoughMoney(static_cast<int64>(plan.estimatedRepairCost)))
        {
            bot->DurabilityRepairAll(true, 0.0f, false);
            TC_LOG_DEBUG("module.playerbot", "ReagentManager: Bot {} repaired all equipment at vendor {} "
                "(durability was {:.1f}%%)",
                bot->GetName(), vendor->GetName(), plan.lowestDurabilityPct);
        }
        else
        {
            TC_LOG_DEBUG("module.playerbot", "ReagentManager: Bot {} cannot afford repair (need {}c, have {}c)",
                bot->GetName(), plan.estimatedRepairCost, bot->GetMoney());
        }
    }

    if (purchasedCount > 0)
    {
        TC_LOG_DEBUG("module.playerbot", "ReagentManager: Bot {} completed restocking: {} items purchased at vendor {}",
            bot->GetName(), purchasedCount, vendor->GetName());
    }

    return purchasedCount;
}

// ============================================================================
// VENDOR PLANNING
// ============================================================================

bool ReagentManager::ShouldVisitVendor(Player* bot) const
{
    if (!bot || !bot->IsInWorld() || !bot->IsAlive())
        return false;

    // Don't visit vendors while in combat or battlegrounds
    if (bot->IsInCombat() || bot->InBattleground() || bot->InArena())
        return false;

    ReagentManagerConfig config = GetConfig();

    // Urgency checks: return true only when supplies are critically low

    // Food critically low (below half the threshold)
    uint32 foodId = GetFoodForLevel(bot->GetLevel());
    if (foodId != 0)
    {
        uint32 foodCount = bot->GetItemCount(foodId);
        if (foodCount < config.minFoodCount / 2)
            return true;
    }

    // Water critically low for mana users
    if (ClassUsesMana(bot->GetClass()))
    {
        uint32 waterId = GetWaterForLevel(bot->GetLevel());
        if (waterId != 0)
        {
            uint32 waterCount = bot->GetItemCount(waterId);
            if (waterCount < config.minWaterCount / 2)
                return true;
        }
    }

    // Class reagents depleted
    std::vector<uint32> reagents = GetClassReagents(bot->GetClass());
    for (uint32 reagentId : reagents)
    {
        if (reagentId == 0)
            continue;
        if (bot->GetItemCount(reagentId) == 0)
            return true;
    }

    // Equipment durability critically low
    if (NeedsRepair(bot))
    {
        float lowestDura = GetLowestDurabilityPct(bot);
        // Only visit vendor if durability is dangerously low (below threshold)
        if (lowestDura < config.repairThresholdPct)
            return true;
    }

    return false;
}

VendorSearchResult ReagentManager::GetNearestVendor(Player* bot) const
{
    VendorSearchResult result;

    if (!bot || !bot->IsInWorld())
        return result;

    ReagentManagerConfig config = GetConfig();
    float searchRange = config.vendorSearchRange;

    float bestDistance = searchRange + 1.0f;
    Creature* bestVendor = nullptr;
    bool bestCanRepair = false;
    bool bestCanSellFood = false;
    bool bestCanSellReagents = false;

    // Use GetCreatureListWithEntryInGrid with entry=0 to get ALL creatures in range,
    // then filter by NPC vendor flags. Entry=0 skips the entry check in the
    // AllCreaturesOfEntryInRange predicate, returning all creatures within range.
    std::list<Creature*> creatures;
    bot->GetCreatureListWithEntryInGrid(creatures, 0, searchRange);

    for (Creature* creature : creatures)
    {
        if (!creature || !creature->IsAlive())
            continue;

        // Check for any vendor NPC flag
        bool isVendor = creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR) ||
                        creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR_FOOD) ||
                        creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR_REAGENT) ||
                        creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR_AMMO) ||
                        creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR_POISON);

        if (!isVendor)
            continue;

        float distance = bot->GetDistance(creature);
        if (distance >= bestDistance)
            continue;

        // Check capabilities
        bool canRepair = creature->HasNpcFlag(UNIT_NPC_FLAG_REPAIR);
        bool canSellFood = creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR_FOOD) ||
                           creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR);
        bool canSellReagents = creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR_REAGENT) ||
                               creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR);

        // Prefer vendors that can both sell and repair
        // Score: repair capability is highly valued when we need repair
        bool preferThisVendor = false;
        if (bestVendor == nullptr)
        {
            preferThisVendor = true;
        }
        else if (distance < bestDistance * 0.8f)
        {
            // Significantly closer, always prefer
            preferThisVendor = true;
        }
        else if (canRepair && !bestCanRepair)
        {
            // Can repair and current best cannot
            preferThisVendor = true;
        }
        else if (distance < bestDistance)
        {
            // Closer and same capabilities
            preferThisVendor = true;
        }

        if (preferThisVendor)
        {
            bestVendor = creature;
            bestDistance = distance;
            bestCanRepair = canRepair;
            bestCanSellFood = canSellFood;
            bestCanSellReagents = canSellReagents;
        }
    }

    if (bestVendor)
    {
        result.vendor = bestVendor;
        result.distance = bestDistance;
        result.canRepair = bestCanRepair;
        result.canSellFood = bestCanSellFood;
        result.canSellReagents = bestCanSellReagents;

        TC_LOG_DEBUG("module.playerbot", "ReagentManager: Found vendor {} ({}) at {:.1f} yards "
            "(repair={}, food={}, reagents={})",
            bestVendor->GetName(), bestVendor->GetEntry(), bestDistance,
            bestCanRepair, bestCanSellFood, bestCanSellReagents);
    }
    else
    {
        TC_LOG_DEBUG("module.playerbot", "ReagentManager: No vendor found within {:.0f} yards of bot {}",
            searchRange, bot->GetName());
    }

    return result;
}

// ============================================================================
// PERIODIC UPDATE
// ============================================================================

void ReagentManager::Update(Player* bot, uint32 diff)
{
    if (!bot || !bot->IsInWorld() || !bot->IsAlive())
        return;

    // Don't process during combat or in battlegrounds/arenas
    if (bot->IsInCombat() || bot->InBattleground() || bot->InArena())
        return;

    ReagentManagerConfig config = GetConfig();
    uint64 botGuidRaw = bot->GetGUID().GetCounter();

    // Update per-bot timer
    {
        std::lock_guard<std::mutex> lock(_timerMutex);
        auto& timer = _updateTimers[botGuidRaw];
        timer += diff;

        if (timer < config.updateIntervalMs)
            return;

        timer = 0;
    }

    // Check if restock is needed
    if (!NeedsRestock(bot))
        return;

    // Look for a nearby vendor
    VendorSearchResult vendorResult = GetNearestVendor(bot);
    if (!vendorResult.vendor)
        return;

    // Only auto-purchase if within interaction range
    if (vendorResult.distance > 10.0f)
    {
        TC_LOG_DEBUG("module.playerbot", "ReagentManager: Bot {} needs restock but nearest vendor {} is {:.1f} yards away",
            bot->GetName(), vendorResult.vendor->GetName(), vendorResult.distance);
        return;
    }

    // Attempt purchase
    uint32 purchased = AttemptPurchase(bot, vendorResult.vendor);
    if (purchased > 0)
    {
        TC_LOG_DEBUG("module.playerbot", "ReagentManager: Bot {} auto-restocked {} items at {}",
            bot->GetName(), purchased, vendorResult.vendor->GetName());
    }
}

// ============================================================================
// ITEM KNOWLEDGE
// ============================================================================

uint32 ReagentManager::GetFoodForLevel(uint32 level) const
{
    auto const& table = GetFoodTable();
    uint32 bestMatch = 0;

    for (auto const& bracket : table)
    {
        if (level >= bracket.minLevel && level <= bracket.maxLevel)
        {
            bestMatch = bracket.itemId;
            // Don't break; continue to find the highest level match
            // (table is sorted ascending, so last match wins)
        }
    }

    // If level exceeds all brackets, use the last entry
    if (bestMatch == 0 && !table.empty())
    {
        bestMatch = table.back().itemId;
    }

    return bestMatch;
}

uint32 ReagentManager::GetWaterForLevel(uint32 level) const
{
    auto const& table = GetWaterTable();
    uint32 bestMatch = 0;

    for (auto const& bracket : table)
    {
        if (level >= bracket.minLevel && level <= bracket.maxLevel)
        {
            bestMatch = bracket.itemId;
        }
    }

    if (bestMatch == 0 && !table.empty())
    {
        bestMatch = table.back().itemId;
    }

    return bestMatch;
}

uint32 ReagentManager::GetBandageForLevel(uint32 level) const
{
    auto const& table = GetBandageTable();
    uint32 bestMatch = 0;

    for (auto const& bracket : table)
    {
        if (level >= bracket.minLevel && level <= bracket.maxLevel)
        {
            bestMatch = bracket.itemId;
        }
    }

    if (bestMatch == 0 && !table.empty())
    {
        bestMatch = table.back().itemId;
    }

    return bestMatch;
}

std::vector<uint32> ReagentManager::GetClassReagents(uint8 classId) const
{
    auto const& table = GetClassReagentTable();
    auto it = table.find(classId);
    if (it != table.end())
        return it->second;

    return {};
}

// ============================================================================
// DURABILITY QUERIES
// ============================================================================

float ReagentManager::GetLowestDurabilityPct(Player* bot) const
{
    if (!bot)
        return 100.0f;

    float lowestPct = 100.0f;
    bool hasAnyDurabilityItem = false;

    // Check all equipped items
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            continue;

        uint32 maxDurability = *item->m_itemData->MaxDurability;
        if (maxDurability == 0)
            continue; // Item has no durability (rings, trinkets, etc.)

        hasAnyDurabilityItem = true;
        uint32 curDurability = *item->m_itemData->Durability;

        float pct = (static_cast<float>(curDurability) / static_cast<float>(maxDurability)) * 100.0f;
        if (pct < lowestPct)
            lowestPct = pct;
    }

    if (!hasAnyDurabilityItem)
        return 100.0f;

    return lowestPct;
}

bool ReagentManager::NeedsRepair(Player* bot) const
{
    if (!bot)
        return false;

    ReagentManagerConfig config = GetConfig();
    return GetLowestDurabilityPct(bot) < config.repairThresholdPct;
}

uint64 ReagentManager::EstimateRepairCost(Player* bot) const
{
    if (!bot)
        return 0;

    uint64 totalCost = 0;

    // Sum up repair costs for all equipped items
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            continue;

        uint32 maxDurability = *item->m_itemData->MaxDurability;
        if (maxDurability == 0)
            continue;

        uint32 curDurability = *item->m_itemData->Durability;
        if (curDurability >= maxDurability)
            continue;

        // Use TrinityCore's built-in repair cost calculation
        // 0.0f discount = no reputation discount (conservative estimate)
        totalCost += item->CalculateDurabilityRepairCost(1.0f);
    }

    return totalCost;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

ReagentManagerConfig ReagentManager::GetConfig() const
{
    std::shared_lock<std::shared_mutex> lock(_configMutex);
    return _config;
}

void ReagentManager::SetConfig(ReagentManagerConfig const& config)
{
    std::unique_lock<std::shared_mutex> lock(_configMutex);
    _config = config;

    TC_LOG_DEBUG("module.playerbot", "ReagentManager: Configuration updated "
        "(minFood={}, minWater={}, repairThreshold={:.0f}%%)",
        _config.minFoodCount, _config.minWaterCount, _config.repairThresholdPct);
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

bool ReagentManager::NeedsFoodRestock(Player* bot, ReagentManagerConfig const& config) const
{
    uint32 foodId = GetFoodForLevel(bot->GetLevel());
    if (foodId == 0)
        return false;

    return bot->GetItemCount(foodId) < config.minFoodCount;
}

bool ReagentManager::NeedsWaterRestock(Player* bot, ReagentManagerConfig const& config) const
{
    if (!ClassUsesMana(bot->GetClass()))
        return false;

    uint32 waterId = GetWaterForLevel(bot->GetLevel());
    if (waterId == 0)
        return false;

    return bot->GetItemCount(waterId) < config.minWaterCount;
}

bool ReagentManager::NeedsReagentRestock(Player* bot, ReagentManagerConfig const& config) const
{
    std::vector<uint32> reagents = GetClassReagents(bot->GetClass());
    for (uint32 reagentId : reagents)
    {
        if (reagentId == 0)
            continue;
        if (bot->GetItemCount(reagentId) < config.minReagentCount)
            return true;
    }
    return false;
}

bool ReagentManager::NeedsBandageRestock(Player* bot, ReagentManagerConfig const& config) const
{
    uint32 bandageId = GetBandageForLevel(bot->GetLevel());
    if (bandageId == 0)
        return false;

    return bot->GetItemCount(bandageId) < config.minBandageCount;
}

bool ReagentManager::ClassUsesMana(uint8 classId) const
{
    switch (classId)
    {
        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_PRIEST:
        case CLASS_DRUID:
        case CLASS_SHAMAN:
        case CLASS_PALADIN:
        case CLASS_MONK:
        case CLASS_EVOKER:
            return true;
        case CLASS_HUNTER:
            // Hunters have focus in modern WoW, not mana
            return false;
        case CLASS_WARRIOR:
        case CLASS_ROGUE:
        case CLASS_DEATH_KNIGHT:
        case CLASS_DEMON_HUNTER:
            return false;
        default:
            return false;
    }
}

bool ReagentManager::TryPurchaseItem(Player* bot, Creature* vendor, uint32 itemId, uint32 quantity) const
{
    if (!bot || !vendor || itemId == 0 || quantity == 0)
        return false;

    // Find the item in vendor's inventory
    uint32 vendorSlot = FindVendorSlotForItem(vendor, itemId);
    if (vendorSlot == UINT32_MAX)
    {
        TC_LOG_DEBUG("module.playerbot", "ReagentManager: Item {} not found in vendor {} inventory",
            itemId, vendor->GetEntry());
        return false;
    }

    // Check if bot has bag space
    ItemPosCountVec dest;
    InventoryResult canStoreResult = bot->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, quantity);
    if (canStoreResult != EQUIP_ERR_OK)
    {
        TC_LOG_DEBUG("module.playerbot", "ReagentManager: Bot {} has no bag space for {}x item {}",
            bot->GetName(), quantity, itemId);
        return false;
    }

    // Calculate cost and verify gold
    uint64 cost = CalculateItemCost(bot, vendor, itemId, quantity);
    if (!bot->HasEnoughMoney(static_cast<int64>(cost)))
    {
        TC_LOG_DEBUG("module.playerbot", "ReagentManager: Bot {} cannot afford {}x item {} (need {}c, have {}c)",
            bot->GetName(), quantity, itemId, cost, bot->GetMoney());
        return false;
    }

    // Execute purchase via TrinityCore API
    // BuyItemFromVendorSlot handles all server-side validation and item creation
    bool success = bot->BuyItemFromVendorSlot(
        vendor->GetGUID(),
        vendorSlot,
        itemId,
        quantity,
        NULL_BAG,
        NULL_SLOT
    );

    if (success)
    {
        TC_LOG_DEBUG("module.playerbot", "ReagentManager: Bot {} purchased {}x item {} from vendor {} (slot {})",
            bot->GetName(), quantity, itemId, vendor->GetEntry(), vendorSlot);
    }
    else
    {
        TC_LOG_DEBUG("module.playerbot", "ReagentManager: BuyItemFromVendorSlot failed for bot {} - item {} from vendor {}",
            bot->GetName(), itemId, vendor->GetEntry());
    }

    return success;
}

uint32 ReagentManager::FindVendorSlotForItem(Creature* vendor, uint32 itemId) const
{
    if (!vendor)
        return UINT32_MAX;

    VendorItemData const* vendorItems = vendor->GetVendorItems();
    if (!vendorItems || vendorItems->Empty())
        return UINT32_MAX;

    uint32 itemCount = vendorItems->GetItemCount();
    for (uint32 slot = 0; slot < itemCount; ++slot)
    {
        VendorItem const* vendorItem = vendorItems->GetItem(slot);
        if (vendorItem && vendorItem->item == itemId)
            return slot;
    }

    return UINT32_MAX;
}

uint64 ReagentManager::CalculateItemCost(Player* bot, Creature* vendor, uint32 itemId, uint32 quantity) const
{
    if (!bot || !vendor || itemId == 0 || quantity == 0)
        return 0;

    ItemTemplate const* tmpl = sObjectMgr->GetItemTemplate(itemId);
    if (!tmpl)
        return 0;

    uint64 basePrice = tmpl->GetBuyPrice();
    if (basePrice == 0)
        return 0;

    uint32 buyCount = tmpl->GetBuyCount();
    if (buyCount == 0)
        buyCount = 1;

    // Price per single item
    double pricePerItem = static_cast<double>(basePrice) / static_cast<double>(buyCount);

    // Apply reputation discount
    float discount = bot->GetReputationPriceDiscount(vendor);

    // Total cost
    uint64 totalCost = static_cast<uint64>(std::floor(pricePerItem * quantity * discount));

    // Ensure minimum 1 copper if item has a buy price
    if (basePrice > 0 && totalCost == 0)
        totalCost = 1;

    return totalCost;
}

} // namespace Playerbot
