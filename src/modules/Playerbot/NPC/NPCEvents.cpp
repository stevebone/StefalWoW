/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "NPCEvents.h"
#include <sstream>

namespace Playerbot
{

NPCEvent NPCEvent::GossipMenuReceived(ObjectGuid player, ObjectGuid npc, uint32 menuId, std::vector<GossipMenuItem> items, std::string text)
{
    NPCEvent event;
    event.type = NPCEventType::GOSSIP_MENU_RECEIVED;
    event.priority = NPCEventPriority::HIGH;
    event.playerGuid = player;
    event.npcGuid = npc;
    event.menuId = menuId;
    event.gossipItems = std::move(items);
    event.gossipText = std::move(text);
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(30000);
    return event;
}

NPCEvent NPCEvent::GossipComplete(ObjectGuid player, ObjectGuid npc)
{
    NPCEvent event;
    event.type = NPCEventType::GOSSIP_COMPLETE;
    event.priority = NPCEventPriority::LOW;
    event.playerGuid = player;
    event.npcGuid = npc;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(5000);
    return event;
}

NPCEvent NPCEvent::VendorListReceived(ObjectGuid player, ObjectGuid npc, uint32 vendorEntry, std::vector<uint32> items)
{
    NPCEvent event;
    event.type = NPCEventType::VENDOR_LIST_RECEIVED;
    event.priority = NPCEventPriority::HIGH;
    event.playerGuid = player;
    event.npcGuid = npc;
    event.vendorEntry = vendorEntry;
    event.vendorItems = std::move(items);
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(30000);
    return event;
}

NPCEvent NPCEvent::TrainerListReceived(ObjectGuid player, ObjectGuid npc, uint32 trainerEntry, std::vector<TrainerSpell> spells, std::string greeting)
{
    NPCEvent event;
    event.type = NPCEventType::TRAINER_LIST_RECEIVED;
    event.priority = NPCEventPriority::HIGH;
    event.playerGuid = player;
    event.npcGuid = npc;
    event.trainerEntry = trainerEntry;
    event.trainerSpells = std::move(spells);
    event.trainerGreeting = std::move(greeting);
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(30000);
    return event;
}

NPCEvent NPCEvent::TrainerServiceResult(ObjectGuid player, ObjectGuid npc, uint32 result, std::string error)
{
    NPCEvent event;
    event.type = NPCEventType::TRAINER_SERVICE_RESULT;
    event.priority = result == 0 ? NPCEventPriority::MEDIUM : NPCEventPriority::HIGH;
    event.playerGuid = player;
    event.npcGuid = npc;
    event.serviceResult = result;
    event.errorMessage = std::move(error);
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(10000);
    return event;
}

NPCEvent NPCEvent::BankOpened(ObjectGuid player, ObjectGuid npc)
{
    NPCEvent event;
    event.type = NPCEventType::BANK_OPENED;
    event.priority = NPCEventPriority::MEDIUM;
    event.playerGuid = player;
    event.npcGuid = npc;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(60000);
    return event;
}

NPCEvent NPCEvent::SpiritHealerConfirm(ObjectGuid player, ObjectGuid npc)
{
    NPCEvent event;
    event.type = NPCEventType::SPIRIT_HEALER_CONFIRM;
    event.priority = NPCEventPriority::CRITICAL;
    event.playerGuid = player;
    event.npcGuid = npc;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(15000);
    return event;
}

NPCEvent NPCEvent::PetitionListReceived(ObjectGuid player, ObjectGuid npc, std::vector<uint32> petitions)
{
    NPCEvent event;
    event.type = NPCEventType::PETITION_LIST_RECEIVED;
    event.priority = NPCEventPriority::MEDIUM;
    event.playerGuid = player;
    event.npcGuid = npc;
    event.petitionIds = std::move(petitions);
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(30000);
    return event;
}

bool NPCEvent::IsValid() const
{
    if (type >= NPCEventType::MAX_NPC_EVENT)
        return false;
    if (timestamp.time_since_epoch().count() == 0)
        return false;
    if (playerGuid.IsEmpty())
        return false;
    if (npcGuid.IsEmpty())
        return false;
    return true;
}

std::string NPCEvent::ToString() const
{
    std::ostringstream oss;
    oss << "[NPCEvent] Type: " << static_cast<uint32>(type)
        << ", Player: " << playerGuid.ToString()
        << ", NPC: " << npcGuid.ToString();

    if (type == NPCEventType::GOSSIP_MENU_RECEIVED)
        oss << ", MenuId: " << menuId << ", Items: " << gossipItems.size();
    else if (type == NPCEventType::VENDOR_LIST_RECEIVED)
        oss << ", Vendor: " << vendorEntry << ", Items: " << vendorItems.size();
    else if (type == NPCEventType::TRAINER_LIST_RECEIVED)
        oss << ", Trainer: " << trainerEntry << ", Spells: " << trainerSpells.size();
    else if (type == NPCEventType::TRAINER_SERVICE_RESULT)
        oss << ", Result: " << serviceResult;

    return oss.str();
}

} // namespace Playerbot
