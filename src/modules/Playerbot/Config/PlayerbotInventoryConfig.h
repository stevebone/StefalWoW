/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
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
 */

#ifndef TRINITY_PLAYERBOT_INVENTORY_CONFIG_H
#define TRINITY_PLAYERBOT_INVENTORY_CONFIG_H

#include "PlayerbotConfig.h"

/**
 * @class PlayerbotInventoryConfig
 * @brief Extension methods for PlayerbotConfig specific to inventory management
 *
 * This header provides convenient inline accessors for inventory-related
 * configuration values. These methods are implemented as free functions
 * to avoid modifying the core PlayerbotConfig class.
 */

namespace Playerbot
{
namespace Config
{

/**
 * @brief Check if inventory auto-loot is enabled
 * @return true if bots should automatically loot nearby corpses
 */
inline bool IsInventoryAutoLootEnabled()
{
    return PlayerbotConfig::instance()->GetBool("Playerbot.Inventory.AutoLoot", true);
}

/**
 * @brief Check if inventory auto-equip is enabled
 * @return true if bots should automatically equip upgrades
 */
inline bool IsInventoryAutoEquipEnabled()
{
    return PlayerbotConfig::instance()->GetBool("Playerbot.Inventory.AutoEquip", true);
}

/**
 * @brief Check if inventory auto-sell is enabled
 * @return true if bots should automatically sell vendor trash
 */
inline bool IsInventoryAutoSellEnabled()
{
    return PlayerbotConfig::instance()->GetBool("Playerbot.Inventory.AutoSell", true);
}

/**
 * @brief Get inventory update interval
 * @return Update interval in milliseconds
 */
inline uint32 GetInventoryUpdateInterval()
{
    return PlayerbotConfig::instance()->GetUInt("Playerbot.Inventory.UpdateInterval", 2000);
}

/**
 * @brief Get minimum free bag slots to maintain
 * @return Number of slots that should be kept free
 */
inline uint32 GetInventoryMinFreeSlots()
{
    return PlayerbotConfig::instance()->GetUInt("Playerbot.Inventory.MinFreeSlots", 5);
}

/**
 * @brief Get minimum item quality to loot
 * @return Minimum quality (0=grey, 1=white, 2=green, etc.)
 */
inline uint32 GetInventoryMinLootQuality()
{
    return PlayerbotConfig::instance()->GetUInt("Playerbot.Inventory.MinLootQuality", 0);
}

/**
 * @brief Get maximum loot range
 * @return Maximum distance to search for lootable objects
 */
inline float GetInventoryLootRange()
{
    return PlayerbotConfig::instance()->GetFloat("Playerbot.Inventory.LootRange", 30.0f);
}

/**
 * @brief Check if bots should loot quest items
 * @return true if quest items should be looted
 */
inline bool IsInventoryLootQuestItems()
{
    return PlayerbotConfig::instance()->GetBool("Playerbot.Inventory.LootQuestItems", true);
}

/**
 * @brief Check if bots should destroy items for space
 * @return true if bots can destroy low value items when bags are full
 */
inline bool IsInventoryDestroyTrashEnabled()
{
    return PlayerbotConfig::instance()->GetBool("Playerbot.Inventory.DestroyTrash", true);
}

/**
 * @brief Get equipment scan interval
 * @return Interval between equipment optimization scans in milliseconds
 */
inline uint32 GetInventoryEquipScanInterval()
{
    return PlayerbotConfig::instance()->GetUInt("Playerbot.Inventory.EquipScanInterval", 5000);
}

/**
 * @brief Get bag organization interval
 * @return Interval between bag organization operations in milliseconds
 */
inline uint32 GetInventoryOrganizeInterval()
{
    return PlayerbotConfig::instance()->GetUInt("Playerbot.Inventory.OrganizeInterval", 30000);
}

} // namespace Config
} // namespace Playerbot

#endif // TRINITY_PLAYERBOT_INVENTORY_CONFIG_H