/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * EnchantGemManager: Applies optimal enchants and gems to bot equipment.
 */

#include "EnchantGemManager.h"
#include "EnchantGemDatabase.h"
#include "Player.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "ItemDefines.h"
#include "ObjectMgr.h"
#include "DB2Stores.h"
#include "Log.h"

namespace Playerbot
{

// ============================================================================
// Equipment Slot Mapping
// ============================================================================

// Maps EQUIPMENT_SLOT_* indices to EnchantSlotType
// Only slots that support enchants are mapped; others map to MAX_SLOT
EnchantSlotType EnchantGemManager::EquipSlotToEnchantSlot(uint8 equipSlot)
{
    switch (equipSlot)
    {
        case EQUIPMENT_SLOT_HEAD:     return EnchantSlotType::HEAD;
        case EQUIPMENT_SLOT_SHOULDERS: return EnchantSlotType::SHOULDER;
        case EQUIPMENT_SLOT_BACK:     return EnchantSlotType::BACK;
        case EQUIPMENT_SLOT_CHEST:    return EnchantSlotType::CHEST;
        case EQUIPMENT_SLOT_WRISTS:   return EnchantSlotType::WRIST;
        case EQUIPMENT_SLOT_HANDS:    return EnchantSlotType::HANDS;
        case EQUIPMENT_SLOT_LEGS:     return EnchantSlotType::LEGS;
        case EQUIPMENT_SLOT_FEET:     return EnchantSlotType::FEET;
        case EQUIPMENT_SLOT_MAINHAND: return EnchantSlotType::MAIN_HAND;
        case EQUIPMENT_SLOT_OFFHAND:  return EnchantSlotType::OFF_HAND;
        case EQUIPMENT_SLOT_FINGER1:  return EnchantSlotType::RING_1;
        case EQUIPMENT_SLOT_FINGER2:  return EnchantSlotType::RING_2;
        default:                      return EnchantSlotType::MAX_SLOT;
    }
}

GemColor EnchantGemManager::SocketColorToGemColor(uint32 socketColorMask)
{
    if (socketColorMask & SOCKET_COLOR_META)
        return GemColor::META;
    if (socketColorMask & SOCKET_COLOR_RED)
        return GemColor::RED;
    if (socketColorMask & SOCKET_COLOR_YELLOW)
        return GemColor::YELLOW;
    if (socketColorMask & SOCKET_COLOR_BLUE)
        return GemColor::BLUE;
    if (socketColorMask & SOCKET_COLOR_COGWHEEL)
        return GemColor::COGWHEEL;
    // Default: treat as prismatic (any color fits)
    return GemColor::PRISMATIC;
}

// ============================================================================
// Helper Methods
// ============================================================================

uint8 EnchantGemManager::GetBotClassId(Player* bot)
{
    return bot ? bot->GetClass() : 0;
}

uint8 EnchantGemManager::GetBotSpecId(Player* bot)
{
    if (!bot)
        return 0;

    // GetPrimarySpecializationEntry() returns ChrSpecializationEntry const*
    // We need the spec index (0-3) within the class
    if (auto const* specEntry = bot->GetPrimarySpecializationEntry())
        return static_cast<uint8>(specEntry->OrderIndex);

    return 0;
}

bool EnchantGemManager::HasPermanentEnchant(::Item* item)
{
    return item && item->GetEnchantmentId(PERM_ENCHANTMENT_SLOT) != 0;
}

bool EnchantGemManager::HasEmptyGemSockets(::Item* item)
{
    if (!item)
        return false;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return false;

    for (uint32 i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i)
    {
        SocketColor color = proto->GetSocketColor(i);
        if (color != SocketColor(0))
        {
            // Socket exists; check if it's filled
            EnchantmentSlot gemSlot = static_cast<EnchantmentSlot>(SOCK_ENCHANTMENT_SLOT + i);
            if (item->GetEnchantmentId(gemSlot) == 0)
                return true;
        }
    }

    return false;
}

uint32 EnchantGemManager::GetGemSocketCount(::Item* item)
{
    if (!item)
        return 0;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return 0;

    uint32 count = 0;
    for (uint32 i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i)
    {
        if (proto->GetSocketColor(i) != SocketColor(0))
            ++count;
    }

    return count;
}

uint32 EnchantGemManager::GetFilledGemSocketCount(::Item* item)
{
    if (!item)
        return 0;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return 0;

    uint32 count = 0;
    for (uint32 i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i)
    {
        if (proto->GetSocketColor(i) != SocketColor(0))
        {
            EnchantmentSlot gemSlot = static_cast<EnchantmentSlot>(SOCK_ENCHANTMENT_SLOT + i);
            if (item->GetEnchantmentId(gemSlot) != 0)
                ++count;
        }
    }

    return count;
}

// ============================================================================
// Per-Item Operations
// ============================================================================

bool EnchantGemManager::ApplyBestEnchant(Player* bot, ::Item* item, uint8 equipSlot, bool overwrite)
{
    if (!bot || !item)
        return false;

    // Check if item already has a permanent enchant
    if (!overwrite && HasPermanentEnchant(item))
        return false;

    // Map equipment slot to enchant slot type
    EnchantSlotType enchantSlot = EquipSlotToEnchantSlot(equipSlot);
    if (enchantSlot == EnchantSlotType::MAX_SLOT)
        return false;

    uint8 classId = GetBotClassId(bot);
    uint8 specId = GetBotSpecId(bot);
    uint32 itemLevel = item->GetItemLevel(bot);

    // Look up the best enchant recommendation
    EnchantRecommendation const* rec = EnchantGemDatabase::GetBestEnchant(classId, specId, enchantSlot, itemLevel);
    if (!rec || rec->enchantId == 0)
        return false;

    // If overwriting, check if the current enchant is already the recommended one
    if (overwrite && item->GetEnchantmentId(PERM_ENCHANTMENT_SLOT) == rec->enchantId)
        return false; // Already has the best enchant

    // Validate the enchant ID exists in DBC
    SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(rec->enchantId);
    if (!enchantEntry)
    {
        TC_LOG_WARN("module.playerbot", "EnchantGemManager: Invalid enchant ID {} ({}) for player {}",
            rec->enchantId, rec->enchantName, bot->GetName());
        return false;
    }

    // Remove old enchant stat effects if overwriting
    if (overwrite && HasPermanentEnchant(item))
        bot->ApplyEnchantment(item, PERM_ENCHANTMENT_SLOT, false);

    // Apply the enchant
    item->SetEnchantment(PERM_ENCHANTMENT_SLOT, rec->enchantId, 0, 0);

    // Apply enchant stat effects
    bot->ApplyEnchantment(item, PERM_ENCHANTMENT_SLOT, true);

    TC_LOG_DEBUG("module.playerbot", "EnchantGemManager: Applied enchant '{}' (ID {}) to {} slot {} for player {}",
        rec->enchantName, rec->enchantId, item->GetTemplate()->GetDefaultLocaleName(),
        equipSlot, bot->GetName());

    return true;
}

uint32 EnchantGemManager::ApplyBestGems(Player* bot, ::Item* item, bool overwrite)
{
    if (!bot || !item)
        return 0;

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
        return 0;

    uint8 classId = GetBotClassId(bot);
    uint8 specId = GetBotSpecId(bot);
    uint32 gemsApplied = 0;

    for (uint32 i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i)
    {
        SocketColor socketColor = proto->GetSocketColor(i);
        if (socketColor == SocketColor(0))
            continue; // No socket in this slot

        EnchantmentSlot gemSlot = static_cast<EnchantmentSlot>(SOCK_ENCHANTMENT_SLOT + i);

        // Check if socket already has a gem
        if (!overwrite && item->GetEnchantmentId(gemSlot) != 0)
            continue;

        // Convert socket color to our GemColor enum
        GemColor color = SocketColorToGemColor(static_cast<uint32>(socketColor));

        // Look up the best gem recommendation
        GemRecommendation const* rec = EnchantGemDatabase::GetBestGem(classId, specId, color);
        if (!rec || rec->gemItemId == 0)
        {
            // Try prismatic as fallback (fits any socket)
            rec = EnchantGemDatabase::GetBestGem(classId, specId, GemColor::PRISMATIC);
            if (!rec || rec->gemItemId == 0)
                continue;
        }

        // Gems are applied as enchantments using the gem's enchant ID
        // Look up the gem item template to find its GemProperties -> enchant ID
        ItemTemplate const* gemProto = sObjectMgr->GetItemTemplate(rec->gemItemId);
        if (!gemProto)
        {
            TC_LOG_WARN("module.playerbot", "EnchantGemManager: Invalid gem item ID {} ({}) for player {}",
                rec->gemItemId, rec->gemName, bot->GetName());
            continue;
        }

        // Get gem enchant ID from GemProperties DBC
        uint32 gemEnchantId = 0;
        if (uint32 gemPropertiesId = gemProto->GetGemProperties())
        {
            if (GemPropertiesEntry const* gemProps = sGemPropertiesStore.LookupEntry(gemPropertiesId))
                gemEnchantId = gemProps->EnchantId;
        }

        if (gemEnchantId == 0)
        {
            TC_LOG_WARN("module.playerbot", "EnchantGemManager: Gem item {} ({}) has no enchant ID for player {}",
                rec->gemItemId, rec->gemName, bot->GetName());
            continue;
        }

        // If overwriting, check if already has same gem
        if (overwrite && item->GetEnchantmentId(gemSlot) == gemEnchantId)
            continue;

        // Remove old gem effects if overwriting
        if (overwrite && item->GetEnchantmentId(gemSlot) != 0)
            bot->ApplyEnchantment(item, gemSlot, false);

        // Apply the gem enchant
        item->SetEnchantment(gemSlot, gemEnchantId, 0, 0);

        // Apply gem stat effects
        bot->ApplyEnchantment(item, gemSlot, true);

        TC_LOG_DEBUG("module.playerbot", "EnchantGemManager: Applied gem '{}' (item {}, enchant {}) to socket {} of {} for player {}",
            rec->gemName, rec->gemItemId, gemEnchantId, i,
            proto->GetDefaultLocaleName(), bot->GetName());

        ++gemsApplied;
    }

    return gemsApplied;
}

// ============================================================================
// Batch Operations
// ============================================================================

EnchantGemResult EnchantGemManager::ApplyOptimalEnchants(Player* bot, bool overwrite)
{
    EnchantGemResult result;

    if (!bot || !bot->IsInWorld())
        return result;

    TC_LOG_DEBUG("module.playerbot", "EnchantGemManager: Applying optimal enchants for player {} (class {}, spec {})",
        bot->GetName(), GetBotClassId(bot), GetBotSpecId(bot));

    // Iterate all equipment slots that support enchants
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        ::Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            continue;

        EnchantSlotType enchantSlot = EquipSlotToEnchantSlot(slot);
        if (enchantSlot == EnchantSlotType::MAX_SLOT)
            continue; // This slot doesn't support enchants

        if (ApplyBestEnchant(bot, item, slot, overwrite))
            ++result.enchantsApplied;
        else
            ++result.enchantsSkipped;
    }

    if (result.enchantsApplied > 0)
    {
        TC_LOG_INFO("module.playerbot", "EnchantGemManager: Applied {} enchants for player {} ({} skipped)",
            result.enchantsApplied, bot->GetName(), result.enchantsSkipped);
    }

    return result;
}

EnchantGemResult EnchantGemManager::ApplyOptimalGems(Player* bot, bool overwrite)
{
    EnchantGemResult result;

    if (!bot || !bot->IsInWorld())
        return result;

    TC_LOG_DEBUG("module.playerbot", "EnchantGemManager: Applying optimal gems for player {} (class {}, spec {})",
        bot->GetName(), GetBotClassId(bot), GetBotSpecId(bot));

    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        ::Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            continue;

        uint32 applied = ApplyBestGems(bot, item, overwrite);
        result.gemsApplied += applied;

        if (applied == 0 && GetGemSocketCount(item) > 0)
            result.gemsSkipped += GetGemSocketCount(item) - GetFilledGemSocketCount(item);
    }

    if (result.gemsApplied > 0)
    {
        TC_LOG_INFO("module.playerbot", "EnchantGemManager: Applied {} gems for player {} ({} skipped)",
            result.gemsApplied, bot->GetName(), result.gemsSkipped);
    }

    return result;
}

EnchantGemResult EnchantGemManager::ApplyAll(Player* bot, bool overwrite)
{
    EnchantGemResult enchantResult = ApplyOptimalEnchants(bot, overwrite);
    EnchantGemResult gemResult = ApplyOptimalGems(bot, overwrite);

    EnchantGemResult combined;
    combined.enchantsApplied = enchantResult.enchantsApplied;
    combined.enchantsSkipped = enchantResult.enchantsSkipped;
    combined.gemsApplied = gemResult.gemsApplied;
    combined.gemsSkipped = gemResult.gemsSkipped;
    combined.errors = enchantResult.errors + gemResult.errors;

    if (combined.TotalApplied() > 0)
    {
        TC_LOG_INFO("module.playerbot", "EnchantGemManager: Applied {} enchants + {} gems for player {}",
            combined.enchantsApplied, combined.gemsApplied, bot->GetName());
    }

    return combined;
}

} // namespace Playerbot
