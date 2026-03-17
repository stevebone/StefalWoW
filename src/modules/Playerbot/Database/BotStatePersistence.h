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
 */

#ifndef _PLAYERBOT_BOT_STATE_PERSISTENCE_H
#define _PLAYERBOT_BOT_STATE_PERSISTENCE_H

#include "Common.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <functional>
#include <vector>

class Player;
class Item;

namespace Playerbot
{
    /**
     * @enum PersistenceResult
     * @brief Result codes for persistence operations
     */
    enum class PersistenceResult
    {
        SUCCESS = 0,                    ///< Operation completed successfully
        DATABASE_ERROR,                 ///< Database query failed
        PLAYER_INVALID,                 ///< Player is null or invalid
        STATE_NOT_FOUND,                ///< Bot state not found in database
        INVENTORY_FULL,                 ///< Cannot save more inventory items
        ITEM_INVALID,                   ///< Item is null or invalid
        ASYNC_PENDING,                  ///< Async operation in progress
        TRANSACTION_FAILED              ///< Database transaction failed
    };

    /**
     * @struct BotStateSnapshot
     * @brief Complete snapshot of bot state for persistence
     */
    struct BotStateSnapshot
    {
        ObjectGuid botGuid;             ///< Bot GUID
        Position position;              ///< Current position
        float orientation;              ///< Current facing direction
        uint32 mapId;                   ///< Current map ID
        uint32 zoneId;                  ///< Current zone ID
        uint64 goldCopper;              ///< Gold in copper
        uint32 health;                  ///< Current health
        uint32 mana;                    ///< Current mana/energy/rage
        uint32 level;                   ///< Character level

        BotStateSnapshot()
            : orientation(0.0f), mapId(0), zoneId(0), goldCopper(0),
              health(0), mana(0), level(0) {}
    };

    /**
     * @struct InventoryItemSnapshot
     * @brief Single inventory item snapshot
     */
    struct InventoryItemSnapshot
    {
        ObjectGuid botGuid;             ///< Bot GUID
        uint8 bag;                      ///< Bag index (0-4)
        uint8 slot;                     ///< Slot index within bag
        uint32 itemId;                  ///< Item entry ID
        ObjectGuid itemGuid;            ///< Item instance GUID
        uint32 stackCount;              ///< Stack size
        ::std::string enchantments;       ///< Enchantment data (serialized)
        uint32 durability;              ///< Item durability

        InventoryItemSnapshot()
            : bag(0), slot(0), itemId(0), stackCount(0), durability(0) {}
    };

    /**
     * @struct EquipmentItemSnapshot
     * @brief Single equipment slot snapshot
     */
    struct EquipmentItemSnapshot
    {
        ObjectGuid botGuid;             ///< Bot GUID
        uint8 slot;                     ///< Equipment slot (EQUIPMENT_SLOT_*)
        uint32 itemId;                  ///< Item entry ID
        ObjectGuid itemGuid;            ///< Item instance GUID
        ::std::string enchantments;       ///< Enchantment data (serialized)
        ::std::string gems;               ///< Gem data (serialized)
        uint32 durability;              ///< Item durability

        EquipmentItemSnapshot()
            : slot(0), itemId(0), durability(0) {}
    };

    /**
     * @class BotStatePersistence
     * @brief High-performance persistence system for bot state, inventory, and equipment
     *
     * Purpose:
     * - Save and restore bot state across server restarts
     * - Track bot inventory and equipment changes
     * - Provide async database operations for minimal server impact
     * - Enable bot state analytics and debugging
     *
     * Features:
     * - Async persistence (non-blocking)
     * - Batched updates (multiple bots per transaction)
     * - Incremental snapshots (only changed data)
     * - Thread-safe operations
     * - Automatic retry on transient failures
     *
     * Performance Targets:
     * - State save: < 1ms (async)
     * - State load: < 5ms (blocking, infrequent)
     * - Inventory save: < 2ms per 100 items (async)
     * - Equipment save: < 1ms (async)
     *
     * Quality Standards:
     * - NO shortcuts - Full prepared statement integration
     * - Complete error handling and logging
     * - Production-ready code (no TODOs)
     *
     * @code
     * // Example usage:
     * BotStatePersistence persistence;
     *
     * // Save bot state asynchronously
     * persistence.SaveBotStateAsync(bot, [](PersistenceResult result) {
     *     if (result == PersistenceResult::SUCCESS)
     *         TC_LOG_DEBUG("playerbot.persistence", "Bot state saved");
     * });
     *
     * // Load bot state synchronously (on login)
     * BotStateSnapshot snapshot;
     * if (persistence.LoadBotState(botGuid, snapshot) == PersistenceResult::SUCCESS)
     * {
     *     bot->Relocate(snapshot.position);
     *     bot->SetMoney(snapshot.goldCopper);
     * }
     * @endcode
     */
    class BotStatePersistence
    {
    public:
        BotStatePersistence() = default;
        ~BotStatePersistence() = default;

        /// Delete copy/move (singleton pattern recommended)
        BotStatePersistence(BotStatePersistence const&) = delete;
        BotStatePersistence(BotStatePersistence&&) = delete;
        BotStatePersistence& operator=(BotStatePersistence const&) = delete;
        BotStatePersistence& operator=(BotStatePersistence&&) = delete;

        /**
         * @brief Saves complete bot state asynchronously
         *
         * Workflow:
         * 1. Capture bot state snapshot (position, gold, health, mana)
         * 2. Queue async database operation
         * 3. Invoke callback on completion (optional)
         *
         * @param player Bot to save
         * @param callback Optional completion callback
         * @return Result code (SUCCESS or ASYNC_PENDING for async operation)
         *
         * Performance: < 1ms (non-blocking)
         * Thread-safety: Thread-safe (async queue)
         */
        [[nodiscard]] PersistenceResult SaveBotStateAsync(
            Player const* player,
            ::std::function<void(PersistenceResult)> callback = nullptr);

        /**
         * @brief Loads bot state synchronously
         *
         * Used during bot login/spawn to restore previous state.
         *
         * @param botGuid Bot GUID to load
         * @param[out] snapshot Loaded state snapshot
         * @return Result code (SUCCESS if found, STATE_NOT_FOUND if new bot)
         *
         * Performance: < 5ms (blocking database query)
         * Thread-safety: Not thread-safe (main thread only)
         */
        [[nodiscard]] PersistenceResult LoadBotState(
            ObjectGuid botGuid,
            BotStateSnapshot& snapshot);

        /**
         * @brief Saves bot inventory asynchronously
         *
         * @param player Bot to save inventory for
         * @param callback Optional completion callback
         * @return Result code
         *
         * Performance: < 2ms per 100 items (async)
         * Thread-safety: Thread-safe
         */
        [[nodiscard]] PersistenceResult SaveInventoryAsync(
            Player const* player,
            ::std::function<void(PersistenceResult)> callback = nullptr);

        /**
         * @brief Loads bot inventory synchronously
         *
         * @param botGuid Bot GUID to load
         * @param[out] items Loaded inventory items
         * @return Result code
         *
         * Performance: < 10ms per 100 items (blocking)
         * Thread-safety: Not thread-safe (main thread only)
         */
        [[nodiscard]] PersistenceResult LoadInventory(
            ObjectGuid botGuid,
            ::std::vector<InventoryItemSnapshot>& items);

        /**
         * @brief Saves bot equipment asynchronously
         *
         * @param player Bot to save equipment for
         * @param callback Optional completion callback
         * @return Result code
         *
         * Performance: < 1ms (async)
         * Thread-safety: Thread-safe
         */
        [[nodiscard]] PersistenceResult SaveEquipmentAsync(
            Player const* player,
            ::std::function<void(PersistenceResult)> callback = nullptr);

        /**
         * @brief Loads bot equipment synchronously
         *
         * @param botGuid Bot GUID to load
         * @param[out] equipment Loaded equipment items
         * @return Result code
         *
         * Performance: < 5ms (blocking)
         * Thread-safety: Not thread-safe (main thread only)
         */
        [[nodiscard]] PersistenceResult LoadEquipment(
            ObjectGuid botGuid,
            ::std::vector<EquipmentItemSnapshot>& equipment);

        /**
         * @brief Saves all bot data (state + inventory + equipment) in single transaction
         *
         * @param player Bot to save
         * @param callback Optional completion callback
         * @return Result code
         *
         * Performance: < 5ms (async)
         * Thread-safety: Thread-safe
         */
        [[nodiscard]] PersistenceResult SaveCompleteSnapshot(
            Player const* player,
            ::std::function<void(PersistenceResult)> callback = nullptr);

        /**
         * @brief Updates only bot position (fast frequent update)
         *
         * Lightweight update for position tracking without full state save.
         *
         * @param player Bot to update position for
         * @return Result code
         *
         * Performance: < 0.5ms (async)
         * Thread-safety: Thread-safe
         */
        [[nodiscard]] PersistenceResult UpdateBotPositionAsync(
            Player const* player);

        /**
         * @brief Updates only bot gold (fast currency update)
         *
         * @param player Bot to update gold for
         * @return Result code
         *
         * Performance: < 0.5ms (async)
         * Thread-safety: Thread-safe
         */
        [[nodiscard]] PersistenceResult UpdateBotGoldAsync(
            Player const* player);

        /**
         * @brief Deletes all persisted data for bot
         *
         * Used when bot is permanently deleted.
         *
         * @param botGuid Bot GUID to delete
         * @return Result code
         *
         * Performance: < 2ms (async)
         * Thread-safety: Thread-safe
         */
        [[nodiscard]] PersistenceResult DeleteBotData(ObjectGuid botGuid);

        /**
         * @brief Gets human-readable error message for result code
         *
         * @param result Result code
         * @return String description
         */
        [[nodiscard]] static char const* GetResultString(PersistenceResult result);

    private:
        /**
         * @brief Captures current bot state snapshot
         *
         * @param player Bot to snapshot
         * @param[out] snapshot Captured state
         * @return true if successful, false if player invalid
         */
        [[nodiscard]] static bool CaptureStateSnapshot(
            Player const* player,
            BotStateSnapshot& snapshot);

        /**
         * @brief Captures bot inventory snapshot
         *
         * @param player Bot to snapshot
         * @param[out] items Captured inventory items
         * @return true if successful, false if player invalid
         */
        [[nodiscard]] static bool CaptureInventorySnapshot(
            Player const* player,
            ::std::vector<InventoryItemSnapshot>& items);

        /**
         * @brief Captures bot equipment snapshot
         *
         * @param player Bot to snapshot
         * @param[out] equipment Captured equipment items
         * @return true if successful, false if player invalid
         */
        [[nodiscard]] static bool CaptureEquipmentSnapshot(
            Player const* player,
            ::std::vector<EquipmentItemSnapshot>& equipment);

        /**
         * @brief Serializes item enchantments to string
         *
         * @param item Item to serialize
         * @return Serialized enchantment data
         */
        [[nodiscard]] static ::std::string SerializeEnchantments(Item const* item);

        /**
         * @brief Serializes item gems to string
         *
         * @param item Item to serialize
         * @return Serialized gem data
         */
        [[nodiscard]] static ::std::string SerializeGems(Item const* item);
    };

} // namespace Playerbot

#endif // _PLAYERBOT_BOT_STATE_PERSISTENCE_H
