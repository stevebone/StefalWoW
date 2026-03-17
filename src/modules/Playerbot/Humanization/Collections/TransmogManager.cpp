/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 */

#include "TransmogManager.h"
#include "Player.h"
#include "Item.h"
#include "CollectionMgr.h"
#include "GameTime.h"
#include "Log.h"
#include "DB2Stores.h"
#include "WorldSession.h"

namespace Playerbot
{

// ============================================================================
// TransmogManager - Per-Bot Implementation
// ============================================================================

TransmogManager::TransmogManager(Player* bot)
    : _bot(bot)
{
}

void TransmogManager::CollectAppearancesFromEquipment()
{
    if (!_bot || !_bot->GetSession())
        return;

    CollectionMgr* collection = _bot->GetSession()->GetCollectionMgr();
    if (!collection)
        return;

    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (item)
            collection->AddItemAppearance(item);
    }

    TC_LOG_DEBUG("module.playerbot", "TransmogManager: Bot {} collected appearances from equipment",
        _bot->GetName());
}

bool TransmogManager::SaveCurrentOutfit(std::string const& outfitName, TransmogTheme theme)
{
    if (!_bot || outfitName.empty())
        return false;

    TransmogOutfit outfit;
    outfit.name = outfitName;
    outfit.theme = theme;

    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            continue;

        TransmogSlot tSlot = EquipmentSlotToTransmogSlot(slot);
        if (tSlot == TransmogSlot::MAX)
            continue;

        uint32 appearanceId = GetItemAppearanceId(item);
        if (appearanceId > 0)
            outfit.slotAppearances[tSlot] = appearanceId;
    }

    if (outfit.slotAppearances.empty())
        return false;

    _savedOutfits[outfitName] = std::move(outfit);

    TC_LOG_DEBUG("module.playerbot", "TransmogManager: Bot {} saved outfit '{}' ({} slots)",
        _bot->GetName(), outfitName, _savedOutfits[outfitName].slotAppearances.size());

    return true;
}

bool TransmogManager::ApplyOutfit(std::string const& outfitName)
{
    if (!_bot)
        return false;

    auto it = _savedOutfits.find(outfitName);
    if (it == _savedOutfits.end())
        return false;

    const TransmogOutfit& outfit = it->second;
    uint32 applied = 0;

    for (const auto& [tSlot, appearanceId] : outfit.slotAppearances)
    {
        // Convert TransmogSlot back to equipment slot
        uint8 equipSlot = EQUIPMENT_SLOT_END;
        switch (tSlot)
        {
            case TransmogSlot::HEAD:       equipSlot = EQUIPMENT_SLOT_HEAD; break;
            case TransmogSlot::SHOULDER:   equipSlot = EQUIPMENT_SLOT_SHOULDERS; break;
            case TransmogSlot::CHEST:      equipSlot = EQUIPMENT_SLOT_CHEST; break;
            case TransmogSlot::WAIST:      equipSlot = EQUIPMENT_SLOT_WAIST; break;
            case TransmogSlot::LEGS:       equipSlot = EQUIPMENT_SLOT_LEGS; break;
            case TransmogSlot::FEET:       equipSlot = EQUIPMENT_SLOT_FEET; break;
            case TransmogSlot::WRIST:      equipSlot = EQUIPMENT_SLOT_WRISTS; break;
            case TransmogSlot::HANDS:      equipSlot = EQUIPMENT_SLOT_HANDS; break;
            case TransmogSlot::BACK:       equipSlot = EQUIPMENT_SLOT_BACK; break;
            case TransmogSlot::MAIN_HAND:  equipSlot = EQUIPMENT_SLOT_MAINHAND; break;
            case TransmogSlot::OFF_HAND:   equipSlot = EQUIPMENT_SLOT_OFFHAND; break;
            case TransmogSlot::TABARD:     equipSlot = EQUIPMENT_SLOT_TABARD; break;
            default: continue;
        }

        Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, equipSlot);
        if (!item)
            continue;

        // Apply the transmog appearance via item modifier
        // In WoW 12.0, transmog is stored as ITEM_MODIFIER_TRANSMOG_APPEARANCE_ALL_SPECS
        item->SetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_ALL_SPECS, appearanceId);
        item->SetState(ITEM_CHANGED, _bot);
        ++applied;
    }

    if (applied > 0)
    {
        // Force visual update for nearby players
        _bot->SetVisibleItemSlot(0, nullptr); // Trigger update
        TC_LOG_DEBUG("module.playerbot", "TransmogManager: Bot {} applied outfit '{}' ({} slots)",
            _bot->GetName(), outfitName, applied);
    }

    return applied > 0;
}

bool TransmogManager::HasSavedOutfit(std::string const& outfitName) const
{
    return _savedOutfits.contains(outfitName);
}

std::vector<std::string> TransmogManager::GetSavedOutfits() const
{
    std::vector<std::string> names;
    names.reserve(_savedOutfits.size());
    for (const auto& [name, _] : _savedOutfits)
        names.push_back(name);
    return names;
}

void TransmogManager::ClearTransmog()
{
    if (!_bot)
        return;

    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            continue;

        item->SetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_ALL_SPECS, 0);
        item->SetState(ITEM_CHANGED, _bot);
    }

    TC_LOG_DEBUG("module.playerbot", "TransmogManager: Bot {} cleared all transmog",
        _bot->GetName());
}

uint32 TransmogManager::GetCollectedAppearanceCount() const
{
    if (!_bot || !_bot->GetSession())
        return 0;

    CollectionMgr* collection = _bot->GetSession()->GetCollectionMgr();
    if (!collection)
        return 0;

    return static_cast<uint32>(collection->GetAppearanceIds().size());
}

bool TransmogManager::ShouldVisitTransmogrifier() const
{
    if (!_bot)
        return false;

    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastTransmogVisitMs < TRANSMOG_VISIT_COOLDOWN_MS)
        return false;

    // Check if equipment changed since last visit
    uint32 currentHash = CalculateEquipmentHash();
    return currentHash != _lastEquipmentHash;
}

void TransmogManager::OnTransmogrifierVisit()
{
    if (!_bot)
        return;

    _lastTransmogVisitMs = GameTime::GetGameTimeMS();
    _lastEquipmentHash = CalculateEquipmentHash();

    // Collect appearances from current gear
    CollectAppearancesFromEquipment();

    // Save current look as "current"
    SaveCurrentOutfit("current", TransmogTheme::NONE);

    // If we have a preferred outfit, apply it
    if (HasSavedOutfit("preferred"))
        ApplyOutfit("preferred");

    TC_LOG_DEBUG("module.playerbot", "TransmogManager: Bot {} visited transmogrifier (collected: {})",
        _bot->GetName(), GetCollectedAppearanceCount());
}

bool TransmogManager::GenerateThemedOutfit(TransmogTheme /*theme*/, std::string const& outfitName)
{
    // For now, save current equipment as the themed outfit
    // Future: Select matching pieces from collection based on theme
    return SaveCurrentOutfit(outfitName, TransmogTheme::MATCHING);
}

void TransmogManager::Update(uint32 /*diff*/)
{
    if (!_bot)
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastUpdateMs < UPDATE_INTERVAL_MS)
        return;

    _lastUpdateMs = currentTime;

    // Detect equipment changes and auto-collect appearances
    uint32 currentHash = CalculateEquipmentHash();
    if (currentHash != _lastEquipmentHash)
    {
        CollectAppearancesFromEquipment();
        _lastEquipmentHash = currentHash;
    }
}

uint32 TransmogManager::CalculateEquipmentHash() const
{
    if (!_bot)
        return 0;

    uint32 hash = 0;
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = _bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (item)
            hash ^= item->GetEntry() + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash;
}

TransmogSlot TransmogManager::EquipmentSlotToTransmogSlot(uint8 equipSlot)
{
    switch (equipSlot)
    {
        case EQUIPMENT_SLOT_HEAD:      return TransmogSlot::HEAD;
        case EQUIPMENT_SLOT_SHOULDERS: return TransmogSlot::SHOULDER;
        case EQUIPMENT_SLOT_CHEST:     return TransmogSlot::CHEST;
        case EQUIPMENT_SLOT_WAIST:     return TransmogSlot::WAIST;
        case EQUIPMENT_SLOT_LEGS:      return TransmogSlot::LEGS;
        case EQUIPMENT_SLOT_FEET:      return TransmogSlot::FEET;
        case EQUIPMENT_SLOT_WRISTS:    return TransmogSlot::WRIST;
        case EQUIPMENT_SLOT_HANDS:     return TransmogSlot::HANDS;
        case EQUIPMENT_SLOT_BACK:      return TransmogSlot::BACK;
        case EQUIPMENT_SLOT_MAINHAND:  return TransmogSlot::MAIN_HAND;
        case EQUIPMENT_SLOT_OFFHAND:   return TransmogSlot::OFF_HAND;
        case EQUIPMENT_SLOT_TABARD:    return TransmogSlot::TABARD;
        default:                       return TransmogSlot::MAX;
    }
}

uint32 TransmogManager::GetItemAppearanceId(Item const* item)
{
    if (!item)
        return 0;

    // Get the item modified appearance entry for this item
    ItemModifiedAppearanceEntry const* appearance = item->GetItemModifiedAppearance();
    if (appearance)
        return appearance->ID;

    return 0;
}

// ============================================================================
// TransmogCoordinator - Singleton
// ============================================================================

TransmogManager* TransmogCoordinator::GetManager(Player* bot)
{
    if (!bot)
        return nullptr;

    ObjectGuid guid = bot->GetGUID();

    {
        std::shared_lock lock(_mutex);
        auto it = _managers.find(guid);
        if (it != _managers.end())
            return it->second.get();
    }

    std::unique_lock lock(_mutex);
    // Double-check after acquiring write lock
    auto it = _managers.find(guid);
    if (it != _managers.end())
        return it->second.get();

    auto [newIt, inserted] = _managers.emplace(guid, std::make_unique<TransmogManager>(bot));
    return newIt->second.get();
}

void TransmogCoordinator::RemoveManager(ObjectGuid botGuid)
{
    std::unique_lock lock(_mutex);
    _managers.erase(botGuid);
}

void TransmogCoordinator::CleanupExpired(uint32 /*currentTimeMs*/)
{
    // Managers are cleaned up when bots log out via RemoveManager
    // This is a safety net for leaked managers
    std::unique_lock lock(_mutex);

    std::erase_if(_managers, [](const auto& pair) {
        return !pair.second || !pair.second->GetCollectedAppearanceCount();
    });
}

} // namespace Playerbot
