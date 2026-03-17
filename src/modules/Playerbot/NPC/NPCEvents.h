/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_NPC_EVENTS_H
#define PLAYERBOT_NPC_EVENTS_H

#include "Define.h"
#include "ObjectGuid.h"
#include <chrono>
#include <string>
#include <vector>

namespace Playerbot
{

enum class NPCEventType : uint8
{
    GOSSIP_MENU_RECEIVED = 0,
    GOSSIP_COMPLETE,
    VENDOR_LIST_RECEIVED,
    TRAINER_LIST_RECEIVED,
    TRAINER_SERVICE_RESULT,
    BANK_OPENED,
    SPIRIT_HEALER_CONFIRM,
    PETITION_LIST_RECEIVED,
    MAX_NPC_EVENT
};

enum class NPCEventPriority : uint8
{
    CRITICAL = 0,
    HIGH = 1,
    MEDIUM = 2,
    LOW = 3,
    BATCH = 4
};

struct GossipMenuItem
{
    uint32 menuId;
    uint32 optionIndex;
    std::string text;
    uint32 icon;
};

struct TrainerSpell
{
    uint32 spellId;
    uint8 reqLevel;
    uint32 reqSkill;
    uint32 cost;
};

struct NPCEvent
{
    using EventType = NPCEventType;
    using Priority = NPCEventPriority;

    NPCEventType type;
    NPCEventPriority priority;
    ObjectGuid playerGuid;
    ObjectGuid npcGuid;
    std::chrono::steady_clock::time_point timestamp;
    std::chrono::steady_clock::time_point expiryTime;

    // Gossip fields
    uint32 menuId = 0;
    std::vector<GossipMenuItem> gossipItems;
    std::string gossipText;

    // Vendor fields
    std::vector<uint32> vendorItems;
    uint32 vendorEntry = 0;

    // Trainer fields
    std::vector<TrainerSpell> trainerSpells;
    uint32 trainerEntry = 0;
    std::string trainerGreeting;

    // Service result fields
    uint32 serviceResult = 0;
    std::string errorMessage;

    // Petition fields
    std::vector<uint32> petitionIds;

    bool IsValid() const;
    bool IsExpired() const
    {
        return std::chrono::steady_clock::now() >= expiryTime;
    }
    std::string ToString() const;

    bool operator<(NPCEvent const& other) const
    {
        return priority > other.priority;
    }

    // Helper constructors
    static NPCEvent GossipMenuReceived(ObjectGuid player, ObjectGuid npc, uint32 menuId, std::vector<GossipMenuItem> items, std::string text);
    static NPCEvent GossipComplete(ObjectGuid player, ObjectGuid npc);
    static NPCEvent VendorListReceived(ObjectGuid player, ObjectGuid npc, uint32 vendorEntry, std::vector<uint32> items);
    static NPCEvent TrainerListReceived(ObjectGuid player, ObjectGuid npc, uint32 trainerEntry, std::vector<TrainerSpell> spells, std::string greeting);
    static NPCEvent TrainerServiceResult(ObjectGuid player, ObjectGuid npc, uint32 result, std::string error);
    static NPCEvent BankOpened(ObjectGuid player, ObjectGuid npc);
    static NPCEvent SpiritHealerConfirm(ObjectGuid player, ObjectGuid npc);
    static NPCEvent PetitionListReceived(ObjectGuid player, ObjectGuid npc, std::vector<uint32> petitions);
};

} // namespace Playerbot

#endif // PLAYERBOT_NPC_EVENTS_H
