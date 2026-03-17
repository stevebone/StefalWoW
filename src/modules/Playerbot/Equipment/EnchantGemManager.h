/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * EnchantGemManager: Applies optimal enchants and gems to bot equipment.
 * Uses EnchantGemDatabase for recommendations and TrinityCore Item/Player APIs
 * for application.
 *
 * Design: Static utility class (no per-bot state needed).
 * All methods take Player* and operate on equipped items.
 */

#pragma once

#include "Define.h"
#include "EnchantGemDatabase.h"
#include <vector>

class Item;
class Player;

namespace Playerbot
{

/// Result of an enchant/gem application operation
struct EnchantGemResult
{
    uint32 enchantsApplied{0};
    uint32 gemsApplied{0};
    uint32 enchantsSkipped{0};   // Already enchanted or no recommendation
    uint32 gemsSkipped{0};       // Already gemmed or no sockets
    uint32 errors{0};            // Failed applications

    uint32 TotalApplied() const { return enchantsApplied + gemsApplied; }
    bool HasErrors() const { return errors > 0; }
};

// ============================================================================
// EnchantGemManager - Static Utility
// ============================================================================

class TC_GAME_API EnchantGemManager
{
public:
    // ========================================================================
    // Main Operations
    // ========================================================================

    /// Apply optimal enchants to all equipped items for a bot.
    /// Skips items that already have a permanent enchant.
    /// @param bot         The bot player
    /// @param overwrite   If true, replace existing enchants with better ones
    /// @return Result with counts of applied/skipped/errors
    static EnchantGemResult ApplyOptimalEnchants(Player* bot, bool overwrite = false);

    /// Apply optimal gems to all socket slots on equipped items.
    /// Skips sockets that already have gems.
    /// @param bot         The bot player
    /// @param overwrite   If true, replace existing gems with better ones
    /// @return Result with counts of applied/skipped/errors
    static EnchantGemResult ApplyOptimalGems(Player* bot, bool overwrite = false);

    /// Apply both enchants and gems in one pass.
    /// @param bot         The bot player
    /// @param overwrite   If true, replace existing enchants/gems
    /// @return Combined result
    static EnchantGemResult ApplyAll(Player* bot, bool overwrite = false);

    // ========================================================================
    // Per-Item Operations
    // ========================================================================

    /// Apply the best enchant for a single item.
    /// @param bot     The bot player (for class/spec lookup)
    /// @param item    The item to enchant
    /// @param slot    Equipment slot the item is in
    /// @param overwrite If true, replace existing enchant
    /// @return true if enchant was applied
    static bool ApplyBestEnchant(Player* bot, ::Item* item, uint8 equipSlot, bool overwrite = false);

    /// Apply the best gems for a single item's sockets.
    /// @param bot     The bot player (for class/spec lookup)
    /// @param item    The item to gem
    /// @param overwrite If true, replace existing gems
    /// @return Number of gems applied
    static uint32 ApplyBestGems(Player* bot, ::Item* item, bool overwrite = false);

    // ========================================================================
    // Query Helpers
    // ========================================================================

    /// Check if an item has a permanent enchant.
    static bool HasPermanentEnchant(::Item* item);

    /// Check if an item has any empty gem sockets.
    static bool HasEmptyGemSockets(::Item* item);

    /// Get the number of gem sockets on an item.
    static uint32 GetGemSocketCount(::Item* item);

    /// Get the number of filled gem sockets on an item.
    static uint32 GetFilledGemSocketCount(::Item* item);

    /// Convert equipment slot index to EnchantSlotType.
    /// Returns MAX_SLOT if the equipment slot doesn't support enchants.
    static EnchantSlotType EquipSlotToEnchantSlot(uint8 equipSlot);

    /// Convert SocketColor bitmask to GemColor enum.
    static GemColor SocketColorToGemColor(uint32 socketColorMask);

private:
    EnchantGemManager() = delete;  // Static utility class

    /// Get bot's class ID.
    static uint8 GetBotClassId(Player* bot);

    /// Get bot's active spec index.
    static uint8 GetBotSpecId(Player* bot);
};

} // namespace Playerbot
