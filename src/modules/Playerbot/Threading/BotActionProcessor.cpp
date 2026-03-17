/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotActionProcessor.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "MotionMaster.h"
#include "Loot.h"
#include "QuestDef.h"
#include "SharedDefines.h"
#include "Log.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "DBCEnums.h"
#include "StringFormat.h"
#include "Vehicle.h"
#include "Core/PlayerBotHelpers.h"
#include "AI/BotAI.h"

namespace Playerbot
{

uint32 BotActionProcessor::ProcessActions(uint32 maxActionsPerFrame)
{
    _actionsThisFrame = 0;
    BotAction action;

    // Process actions until queue empty or frame budget exhausted
    while (_actionsThisFrame < maxActionsPerFrame && _queue.Pop(action))
    {
        // Execute action on main thread with full Map access
        BotActionResult result = ExecuteAction(action);

        // Update statistics
        _queue.IncrementProcessed();
        if (!result.success)
        {
            _queue.IncrementFailed();
            TC_LOG_DEBUG("playerbot.action",
                "Action failed for bot {}: {} (type {})",
                action.botGuid.ToString(),
                result.errorMessage,
                static_cast<uint8>(action.type));
        }

        ++_actionsThisFrame;
    }

    if (_actionsThisFrame > 0)
    {
        TC_LOG_TRACE("playerbot.action",
            "Processed {} bot actions this frame", _actionsThisFrame);
    }

    return _actionsThisFrame;
}

BotActionResult BotActionProcessor::ExecuteAction(BotAction const& action)
{
    // Get bot player object (all actions require valid bot)
    Player* bot = GetBot(action.botGuid);
    if (!bot)
    {
        return BotActionResult::Failure("Bot not found or not in world");
    }

    // Dispatch to action-specific executor
    switch (action.type)
    {
        case BotActionType::ATTACK_TARGET:
            return ExecuteAttackTarget(bot, action);
        case BotActionType::CAST_SPELL:
            return ExecuteCastSpell(bot, action);
        case BotActionType::STOP_ATTACK:
            return ExecuteStopAttack(bot, action);
        case BotActionType::MOVE_TO_POSITION:
            return ExecuteMoveToPosition(bot, action);
        case BotActionType::FOLLOW_TARGET:
            return ExecuteFollowTarget(bot, action);
        case BotActionType::STOP_MOVEMENT:
            return ExecuteStopMovement(bot, action);
        case BotActionType::INTERACT_OBJECT:
            return ExecuteInteractObject(bot, action);
        case BotActionType::INTERACT_NPC:
            return ExecuteInteractNPC(bot, action);
        case BotActionType::LOOT_OBJECT:
            return ExecuteLootObject(bot, action);
        case BotActionType::ACCEPT_QUEST:
            return ExecuteAcceptQuest(bot, action);
        case BotActionType::TURN_IN_QUEST:
            return ExecuteTurnInQuest(bot, action);
        case BotActionType::USE_ITEM:
            return ExecuteUseItem(bot, action);
        case BotActionType::EQUIP_ITEM:
            return ExecuteEquipItem(bot, action);
        case BotActionType::SEND_CHAT_MESSAGE:
            return ExecuteSendChatMessage(bot, action);
        case BotActionType::ENTER_VEHICLE:
            return ExecuteEnterVehicle(bot, action);
        case BotActionType::EXIT_VEHICLE:
            return ExecuteExitVehicle(bot, action);
        default:
            return BotActionResult::Failure("Unknown action type");
    }
}

// ============================================================================
// ACTION EXECUTORS
// ============================================================================

BotActionResult BotActionProcessor::ExecuteAttackTarget(Player* bot, BotAction const& action)
{
    Unit* target = GetUnit(bot, action.targetGuid);
    if (!target)
        return BotActionResult::Failure("Target not found");

    if (!target->IsInWorld())
        return BotActionResult::Failure("Target not in world");

    if (!target->IsAlive())
        return BotActionResult::Failure("Target is dead");

    if (!bot->IsHostileTo(target))
        return BotActionResult::Failure("Target not hostile");

    // Start attacking
    bot->Attack(target, true);
    TC_LOG_TRACE("playerbot.action",
        "Bot {} started attacking {} at distance {:.1f}",
        bot->GetName(), target->GetName(), bot->GetDistance(target));
    return BotActionResult::Success();
}

BotActionResult BotActionProcessor::ExecuteCastSpell(Player* bot, BotAction const& action)
{
    // Validate spell
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(action.spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return BotActionResult::Failure("Spell not found");

    // Get target if specified
    Unit* target = nullptr;
    if (!action.targetGuid.IsEmpty())
    {
        target = GetUnit(bot, action.targetGuid);
        if (!target || !target->IsInWorld())
            return BotActionResult::Failure("Spell target not found");
    }

    // Cast spell
    bot->CastSpell(CastSpellTargetArg(target ? target : bot), action.spellId);

    TC_LOG_TRACE("playerbot.action",
        "Bot {} cast spell {} on {}",
        bot->GetName(), action.spellId, target ? target->GetName() : "self");

    return BotActionResult::Success();
}

BotActionResult BotActionProcessor::ExecuteStopAttack(Player* bot, BotAction const& action)
{
    bot->AttackStop();

    TC_LOG_TRACE("playerbot.action",
        "Bot {} stopped attacking", bot->GetName());

    return BotActionResult::Success();
}

BotActionResult BotActionProcessor::ExecuteMoveToPosition(Player* bot, BotAction const& action)
{
    // Validate position
    if (!action.position.IsPositionValid())
        return BotActionResult::Failure("Invalid position");

    // Use validated pathfinding for bot movement
    if (BotAI* ai = GetBotAI(bot))
    {
        if (!ai->MoveTo(action.position, true))
        {
            // Fallback to legacy if validation fails
            bot->GetMotionMaster()->MovePoint(0, action.position);
        }
    }
    else
    {
        // Non-bot player - use standard movement
        bot->GetMotionMaster()->MovePoint(0, action.position);
    }

    TC_LOG_TRACE("playerbot.action",
        "Bot {} moving to position ({:.1f}, {:.1f}, {:.1f})",
        bot->GetName(), action.position.GetPositionX(),
        action.position.GetPositionY(), action.position.GetPositionZ());

    return BotActionResult::Success();
}

BotActionResult BotActionProcessor::ExecuteFollowTarget(Player* bot, BotAction const& action)
{
    Unit* target = GetUnit(bot, action.targetGuid);
    if (!target)
        return BotActionResult::Failure("Follow target not found");

    if (!target->IsInWorld())
        return BotActionResult::Failure("Follow target not in world");

    // Use validated pathfinding for bot follow movement
    if (BotAI* ai = GetBotAI(bot))
    {
        if (!ai->MoveToUnit(target, 3.0f))
        {
            // Fallback to legacy if validation fails
            bot->GetMotionMaster()->MoveFollow(target, 3.0f, 0.0f);
        }
    }
    else
    {
        // Non-bot player - use standard movement
        bot->GetMotionMaster()->MoveFollow(target, 3.0f, 0.0f);
    }

    TC_LOG_TRACE("playerbot.action",
        "Bot {} following {}", bot->GetName(), target->GetName());

    return BotActionResult::Success();
}

BotActionResult BotActionProcessor::ExecuteStopMovement(Player* bot, BotAction const& action)
{
    bot->StopMoving();

    TC_LOG_TRACE("playerbot.action",
        "Bot {} stopped moving", bot->GetName());

    return BotActionResult::Success();
}

BotActionResult BotActionProcessor::ExecuteInteractObject(Player* bot, BotAction const& action)
{
    GameObject* object = GetGameObject(bot, action.targetGuid);
    if (!object)
        return BotActionResult::Failure("GameObject not found");

    if (!object->IsInWorld())
        return BotActionResult::Failure("GameObject not in world");

    // Use the object
    object->Use(bot);

    TC_LOG_TRACE("playerbot.action",
        "Bot {} interacted with GameObject {}",
        bot->GetName(), object->GetEntry());
    return BotActionResult::Success();
}

BotActionResult BotActionProcessor::ExecuteInteractNPC(Player* bot, BotAction const& action)
{
    Creature* npc = GetCreature(bot, action.targetGuid);
    if (!npc)
        return BotActionResult::Failure("NPC not found");

    if (!npc->IsInWorld())
        return BotActionResult::Failure("NPC not in world");

    // Interact with NPC (opens gossip menu)
    // Get the first gossip menu for this creature
    ::std::vector<uint32> const& gossipMenuIds = npc->GetCreatureTemplate()->GossipMenuIds;
    uint32 menuId = gossipMenuIds.empty() ? 0 : gossipMenuIds[0];

    // Prepare gossip menu with quest options enabled
    bot->PrepareGossipMenu(npc, menuId, true);

    // Send the prepared gossip to the bot
    bot->SendPreparedGossip(npc);

    TC_LOG_TRACE("playerbot.action",
        "Bot {} opened gossip menu {} with NPC {}",
        bot->GetName(), menuId, npc->GetName());

    return BotActionResult::Success();
}

BotActionResult BotActionProcessor::ExecuteLootObject(Player* bot, BotAction const& action)
{
    // Can be Creature or GameObject
    if (Creature* creature = GetCreature(bot, action.targetGuid))
    {
        if (!creature->IsInWorld() || creature->IsAlive())
            return BotActionResult::Failure("Cannot loot living creature");

        // SendLoot expects Loot& and aeLooting flag
        Loot* loot = creature->GetLootForPlayer(bot);
        if (loot)
            bot->SendLoot(*loot, false);

        return BotActionResult::Success();
    }

    if (GameObject* object = GetGameObject(bot, action.targetGuid))
    {
        if (!object->IsInWorld())
            return BotActionResult::Failure("GameObject not in world");

        // SendLoot expects Loot& and aeLooting flag
        Loot* loot = object->GetLootForPlayer(bot);
        if (loot)
            bot->SendLoot(*loot, false);
        return BotActionResult::Success();
    }

    return BotActionResult::Failure("Loot target not found");
}

BotActionResult BotActionProcessor::ExecuteAcceptQuest(Player* bot, BotAction const& action)
{
    // Get quest from database
    Quest const* quest = sObjectMgr->GetQuestTemplate(action.questId);
    if (!quest)
        return BotActionResult::Failure("Quest not found");

    // Check if player can take the quest
    if (bot->CanTakeQuest(quest, false))
    {
        // Get quest giver (NPC or GameObject)
        WorldObject* questGiver = nullptr;
        if (!action.targetGuid.IsEmpty())
        {
            if (Creature* npc = GetCreature(bot, action.targetGuid))
                questGiver = npc;
            else if (GameObject* obj = GetGameObject(bot, action.targetGuid))
                questGiver = obj;
        }

        // Add quest to player's quest log
        bot->AddQuestAndCheckCompletion(quest, questGiver);

        TC_LOG_TRACE("playerbot.action",
            "Bot {} accepted quest {} ({})",
            bot->GetName(), action.questId, quest->GetLogTitle());

        return BotActionResult::Success();
    }

    return BotActionResult::Failure("Cannot take quest (requirements not met or quest log full)");
}

BotActionResult BotActionProcessor::ExecuteTurnInQuest(Player* bot, BotAction const& action)
{
    // Get quest from player's quest log
    Quest const* quest = sObjectMgr->GetQuestTemplate(action.questId);
    if (!quest)
        return BotActionResult::Failure("Quest not found");

    // Check if player has the quest (FindQuestSlot returns MAX_QUEST_LOG_SIZE if not found)
    uint16 questSlot = bot->FindQuestSlot(action.questId);
    if (questSlot >= MAX_QUEST_LOG_SIZE)
        return BotActionResult::Failure("Player does not have this quest");

    // Check if quest is complete
    if (!bot->CanCompleteQuest(action.questId))
        return BotActionResult::Failure("Quest not complete");

    // Get quest giver
    WorldObject* questGiver = nullptr;
    if (!action.targetGuid.IsEmpty())
    {
        if (Creature* npc = GetCreature(bot, action.targetGuid))
            questGiver = npc;
        else if (GameObject* obj = GetGameObject(bot, action.targetGuid))
            questGiver = obj;
    }

    // Complete the quest (LootItemType rewardType, uint32 rewardId)
    // Use LOOT_ITEM_TYPE_ITEM with ID 0 for auto-select
    bot->RewardQuest(quest, LootItemType::Item, 0, questGiver, false);

    TC_LOG_TRACE("playerbot.action",
        "Bot {} turned in quest {} ({})",
        bot->GetName(), action.questId, quest->GetLogTitle());

    return BotActionResult::Success();
}

BotActionResult BotActionProcessor::ExecuteUseItem(Player* bot, BotAction const& action)
{
    // Find item in inventory by entry
    Item* item = bot->GetItemByEntry(action.itemEntry);
    if (!item)
        return BotActionResult::Failure("Item not found in inventory");

    // Get item template to check usage
    ItemTemplate const* itemTemplate = item->GetTemplate();
    if (!itemTemplate)
        return BotActionResult::Failure("Invalid item template");

    // Check if item has usable effects
    if (itemTemplate->Effects.empty())
        return BotActionResult::Failure("Item has no usable effects");

    // Use the item (casts the item's spell)
    // CastItemUseSpell signature: (Item* item, SpellCastTargets const& targets, ObjectGuid castCount, const std::array<int32, 3>& misc)
    // WoW 12.0: Misc array expanded from 2 to 3 elements
    SpellCastTargets targets;
    std::array<int32, 3> misc = { 0, 0, 0 };
    bot->CastItemUseSpell(item, targets, ObjectGuid::Empty, misc);

    TC_LOG_TRACE("playerbot.action",
        "Bot {} used item {} ({})",
        bot->GetName(), action.itemEntry, itemTemplate->GetName(LOCALE_enUS));

    return BotActionResult::Success();
}

BotActionResult BotActionProcessor::ExecuteEquipItem(Player* bot, BotAction const& action)
{
    // Find item in inventory
    Item* item = bot->GetItemByEntry(action.itemEntry);
    if (!item)
        return BotActionResult::Failure("Item not found in inventory");

    // Get item template
    ItemTemplate const* itemTemplate = item->GetTemplate();
    if (!itemTemplate)
        return BotActionResult::Failure("Invalid item template");

    // Check if item is equippable
    if (itemTemplate->GetInventoryType() == INVTYPE_NON_EQUIP)
        return BotActionResult::Failure("Item is not equippable");

    // Find appropriate equipment slot
    // FindEquipSlot signature: (Item const* item, uint8 slot, bool swap)
    uint8 slot = bot->FindEquipSlot(item, NULL_SLOT, true);
    if (slot >= INVENTORY_SLOT_ITEM_END)
        return BotActionResult::Failure("No valid equipment slot found");

    // Equip the item
    // EquipItem signature: (uint16 pos, Item* pItem, bool update) returns Item*
    uint16 position = (INVENTORY_SLOT_BAG_0 << 8) | slot;
    Item* equippedItem = bot->EquipItem(position, item, true);
    if (!equippedItem)
    {
        return BotActionResult::Failure("Failed to equip item");
    }

    TC_LOG_TRACE("playerbot.action",
        "Bot {} equipped item {} ({}) to slot {}",
        bot->GetName(), action.itemEntry, itemTemplate->GetName(LOCALE_enUS), slot);

    return BotActionResult::Success();
}

BotActionResult BotActionProcessor::ExecuteSendChatMessage(Player* bot, BotAction const& action)
{
    if (action.text.empty())
        return BotActionResult::Failure("Empty chat message");

    // Send chat message
    bot->Say(action.text, LANG_UNIVERSAL);
    TC_LOG_TRACE("playerbot.action",
        "Bot {} said: {}", bot->GetName(), action.text);

    return BotActionResult::Success();
}

// ============================================================================
// HELPER METHODS
// ============================================================================

Player* BotActionProcessor::GetBot(ObjectGuid guid)
{
    // Use FindPlayer (HashMapHolder::Find + IsInWorld check).
    // Do NOT use GetPlayer(Map*, guid) — it compares player->GetMap() == m,
    // and nullptr never matches any BG map, causing 100% lookup failure.
    Player* player = ObjectAccessor::FindPlayer(guid);
    if (!player)
        return nullptr;

    return player;
}

Unit* BotActionProcessor::GetUnit(Player* bot, ObjectGuid guid)
{
    // SAFE: This is called from MAIN THREAD ONLY
    // ObjectAccessor::GetUnit calls Map::GetCreature which is NOT thread-safe,
    // but it's safe when called from main thread (Map owner)
    Unit* unit = ObjectAccessor::GetUnit(*bot, guid);
    if (!unit || !unit->IsInWorld())
        return nullptr;

    return unit;
}

Creature* BotActionProcessor::GetCreature(Player* bot, ObjectGuid guid)
{
    // SAFE: Main thread only
    Creature* creature = ObjectAccessor::GetCreature(*bot, guid);
    if (!creature || !creature->IsInWorld())
        return nullptr;

    return creature;
}

GameObject* BotActionProcessor::GetGameObject(Player* bot, ObjectGuid guid)
{
    // SAFE: Main thread only
    GameObject* object = ObjectAccessor::GetGameObject(*bot, guid);
    if (!object || !object->IsInWorld())
        return nullptr;

    return object;
}

// ============================================================================
// VEHICLE ACTION EXECUTORS
// ============================================================================

BotActionResult BotActionProcessor::ExecuteEnterVehicle(Player* bot, BotAction const& action)
{
    // Already in a vehicle?
    if (bot->GetVehicle())
        return BotActionResult::Failure("Bot already in a vehicle");

    Creature* vehicleCreature = GetCreature(bot, action.targetGuid);
    if (!vehicleCreature)
        return BotActionResult::Failure("Vehicle creature not found");

    if (!vehicleCreature->IsAlive())
        return BotActionResult::Failure("Vehicle creature is dead");

    Vehicle* vehicle = vehicleCreature->GetVehicleKit();
    if (!vehicle)
        return BotActionResult::Failure("Creature has no VehicleKit");

    int8 seatId = static_cast<int8>(action.spellId);  // Reused field

    // Enter vehicle (seatId -1 = first available seat)
    bot->EnterVehicle(vehicleCreature, seatId);

    TC_LOG_TRACE("playerbot.action",
        "Bot {} entered vehicle {} (entry {}, seat {})",
        bot->GetName(), vehicleCreature->GetName(),
        vehicleCreature->GetEntry(), seatId);
    return BotActionResult::Success();
}

BotActionResult BotActionProcessor::ExecuteExitVehicle(Player* bot, BotAction const& action)
{
    if (!bot->GetVehicle())
        return BotActionResult::Failure("Bot not in a vehicle");

    bot->ExitVehicle();

    TC_LOG_TRACE("playerbot.action",
        "Bot {} exited vehicle", bot->GetName());
    return BotActionResult::Success();
}

} // namespace Playerbot
