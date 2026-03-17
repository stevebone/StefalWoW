/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * BotSaveController Implementation
 *
 * P6: Save frequency tiering based on AIBudgetTier
 * P3: Coarse differential saves — skip saves when persistent state unchanged
 */

#include "BotSaveController.h"
#include "Player.h"
#include "Item.h"
#include "QuestDef.h"
#include "Log.h"
#include "WorldSession.h"

namespace Playerbot
{

BotSaveController* BotSaveController::instance()
{
    static BotSaveController inst;
    return &inst;
}

void BotSaveController::OnBudgetTierChange(Player* bot, AIBudgetTier newTier)
{
    if (!bot || !bot->IsInWorld())
        return;

    // Only adjust save timers for bots
    WorldSession* session = bot->GetSession();
    if (!session || !session->IsBot())
        return;

    uint32 interval = GetSaveIntervalForTier(newTier);
    bot->SetSaveTimer(interval);

    _stats.tierChanges++;

    TC_LOG_DEBUG("module.playerbot",
        "BotSaveController: Bot {} tier -> {} save interval = {}s",
        bot->GetName(), static_cast<uint8>(newTier), interval / 1000);
}

uint32 BotSaveController::GetSaveIntervalForTier(AIBudgetTier tier) const
{
    switch (tier)
    {
        case AIBudgetTier::FULL:    return _fullInterval;
        case AIBudgetTier::REDUCED: return _reducedInterval;
        case AIBudgetTier::MINIMAL: return _minimalInterval;
        default:                    return _reducedInterval;
    }
}

bool BotSaveController::ShouldSave(Player* bot)
{
    if (!bot)
        return true;  // Safety: allow save if we can't check

    WorldSession* session = bot->GetSession();
    if (!session || !session->IsBot())
        return true;  // Not a bot — always allow normal save

    _stats.totalSaveChecks++;

    ObjectGuid guid = bot->GetGUID();
    BotSaveState current = CaptureState(bot);

    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _botStates.find(guid);
    if (it == _botStates.end() || !it->second.initialized)
    {
        // First save — always allow and store baseline
        _botStates[guid] = current;
        _stats.savesAllowed++;
        return true;
    }

    if (HasStateChanged(it->second, current))
    {
        // State changed — allow save
        _stats.savesAllowed++;
        return true;
    }

    // No meaningful state change — skip this save cycle
    // Defer by resetting the save timer to the current tier's interval
    // (The timer will count down again, effectively skipping this save)
    _stats.savesSkipped++;

    TC_LOG_DEBUG("module.playerbot",
        "BotSaveController: Skipping unchanged bot save for {} (skip rate: {:.1f}%)",
        bot->GetName(), _stats.GetSkipRate() * 100.0f);

    return false;
}

void BotSaveController::OnSaveCompleted(Player* bot)
{
    if (!bot)
        return;

    ObjectGuid guid = bot->GetGUID();
    BotSaveState current = CaptureState(bot);

    std::lock_guard<std::mutex> lock(_mutex);
    _botStates[guid] = current;
}

void BotSaveController::RemoveBot(ObjectGuid guid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _botStates.erase(guid);
}

// ============================================================================
// State Capture & Comparison
// ============================================================================

BotSaveController::BotSaveState BotSaveController::CaptureState(Player* bot)
{
    BotSaveState state;
    state.level = bot->GetLevel();
    state.xp = bot->GetXP();
    state.money = bot->GetMoney();
    state.zoneId = bot->GetZoneId();
    state.inventoryChecksum = ComputeInventoryChecksum(bot);
    state.questLogChecksum = ComputeQuestLogChecksum(bot);
    state.initialized = true;
    return state;
}

bool BotSaveController::HasStateChanged(BotSaveState const& stored, BotSaveState const& current)
{
    if (stored.level != current.level)
        return true;
    if (stored.xp != current.xp)
        return true;
    if (stored.money != current.money)
        return true;
    if (stored.zoneId != current.zoneId)
        return true;
    if (stored.inventoryChecksum != current.inventoryChecksum)
        return true;
    if (stored.questLogChecksum != current.questLogChecksum)
        return true;

    return false;
}

uint32 BotSaveController::ComputeInventoryChecksum(Player* bot)
{
    // Simple FNV-1a hash of equipped item entry IDs
    uint32 hash = 2166136261u;
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        if (Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            uint32 entry = item->GetEntry();
            hash ^= entry;
            hash *= 16777619u;
        }
    }
    return hash;
}

uint32 BotSaveController::ComputeQuestLogChecksum(Player* bot)
{
    // Simple FNV-1a hash of active quest IDs
    uint32 hash = 2166136261u;
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (questId != 0)
        {
            hash ^= questId;
            hash *= 16777619u;
        }
    }
    return hash;
}

} // namespace Playerbot
