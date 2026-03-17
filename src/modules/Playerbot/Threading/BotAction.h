/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_BOT_ACTION_H
#define PLAYERBOT_BOT_ACTION_H

#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <string>
#include <variant>

namespace Playerbot
{

/**
 * @brief Action types that bots can queue for main thread execution
 *
 * CRITICAL DESIGN:
 * Worker threads make decisions using snapshot data and queue actions.
 * Main thread executes actions with full Map access (thread-safe by design).
 * This follows TrinityCore's async I/O pattern: work → queue → main thread execution.
 */
enum class BotActionType : uint8
{
    // Combat actions
    ATTACK_TARGET,          // Start attacking a target
    CAST_SPELL,             // Cast a spell on target
    STOP_ATTACK,            // Stop attacking current target

    // Movement actions
    MOVE_TO_POSITION,       // Move to specific position
    FOLLOW_TARGET,          // Follow a target
    STOP_MOVEMENT,          // Stop moving

    // Interaction actions
    INTERACT_OBJECT,        // Interact with GameObject
    INTERACT_NPC,           // Talk to NPC
    LOOT_OBJECT,            // Loot creature/GameObject

    // Vehicle actions
    ENTER_VEHICLE,          // Enter a vehicle (creature with VehicleKit)
    EXIT_VEHICLE,           // Exit current vehicle

    // Group actions
    ACCEPT_GROUP_INVITE,    // Accept group invitation
    LEAVE_GROUP,            // Leave current group

    // Quest actions
    ACCEPT_QUEST,           // Accept quest from NPC
    TURN_IN_QUEST,          // Turn in completed quest

    // Item actions
    USE_ITEM,               // Use item from inventory
    EQUIP_ITEM,             // Equip item

    // Social actions
    SEND_CHAT_MESSAGE,      // Send chat message
    EMOTE,                  // Perform emote

    // Special
    CUSTOM                  // Custom action with string data
};

/**
 * @brief Immutable action data queued by worker threads
 *
 * POD structure containing all data needed to execute action on main thread.
 * No pointers, no references - only GUIDs and primitive data.
 */
struct BotAction
{
    BotActionType type;
    ObjectGuid botGuid;         // Bot performing the action

    // Action targets (optional, depends on action type)
    ObjectGuid targetGuid;      // Target unit/object GUID
    uint32 spellId{0};          // For CAST_SPELL
    uint32 itemEntry{0};        // For USE_ITEM, EQUIP_ITEM
    uint32 questId{0};          // For ACCEPT_QUEST, TURN_IN_QUEST

    // Position data (optional)
    Position position;          // For MOVE_TO_POSITION

    // Text data (optional)
    ::std::string text;           // For SEND_CHAT_MESSAGE, CUSTOM

    // Priority (higher = more urgent)
    uint8 priority{0};

    // Timestamp when action was queued
    uint32 queuedTime{0};

    // Validation
    bool IsValid() const { return !botGuid.IsEmpty(); }

    // Factory methods for common actions
    static BotAction AttackTarget(ObjectGuid bot, ObjectGuid target, uint32 timestamp)
    {
        BotAction action;
        action.type = BotActionType::ATTACK_TARGET;
        action.botGuid = bot;
        action.targetGuid = target;
        action.priority = 10;  // Combat is high priority
        action.queuedTime = timestamp;
        return action;
    }

    static BotAction CastSpell(uint32 spell, ObjectGuid target, uint32 timestamp, ObjectGuid bot)
    {
        BotAction action;
        action.type = BotActionType::CAST_SPELL;
        action.botGuid = bot;
        action.targetGuid = target;
        action.spellId = spell;
        action.priority = 10;  // Combat is high priority
        action.queuedTime = timestamp;
        return action;
    }

    static BotAction MoveToPosition(ObjectGuid bot, Position const& pos, uint32 timestamp)
    {
        BotAction action;
        action.type = BotActionType::MOVE_TO_POSITION;
        action.botGuid = bot;
        action.position = pos;
        action.priority = 5;   // Movement is medium priority
        action.queuedTime = timestamp;
        return action;
    }

    static BotAction StopAttack(ObjectGuid bot, uint32 timestamp)
    {
        BotAction action;
        action.type = BotActionType::STOP_ATTACK;
        action.botGuid = bot;
        action.priority = 10;  // Combat state changes are high priority
        action.queuedTime = timestamp;
        return action;
    }

    static BotAction InteractNPC(ObjectGuid bot, ObjectGuid npc, uint32 timestamp)
    {
        BotAction action;
        action.type = BotActionType::INTERACT_NPC;
        action.botGuid = bot;
        action.targetGuid = npc;
        action.priority = 5;
        action.queuedTime = timestamp;
        return action;
    }

    static BotAction LootObject(ObjectGuid bot, ObjectGuid object, uint32 timestamp)
    {
        BotAction action;
        action.type = BotActionType::LOOT_OBJECT;
        action.botGuid = bot;
        action.targetGuid = object;
        action.priority = 7;
        action.queuedTime = timestamp;
        return action;
    }

    static BotAction SendChatMessage(ObjectGuid bot, ::std::string const& message, uint32 timestamp)
    {
        BotAction action;
        action.type = BotActionType::SEND_CHAT_MESSAGE;
        action.botGuid = bot;
        action.text = message;
        action.priority = 1;   // Chat is low priority
        action.queuedTime = timestamp;
        return action;
    }

    static BotAction InteractObject(ObjectGuid bot, ObjectGuid object, uint32 timestamp)
    {
        BotAction action;
        action.type = BotActionType::INTERACT_OBJECT;
        action.botGuid = bot;
        action.targetGuid = object;
        action.priority = 8;   // BG objective interaction is high priority
        action.queuedTime = timestamp;
        return action;
    }

    static BotAction EnterVehicle(ObjectGuid bot, ObjectGuid vehicleGuid, int8 seatId, uint32 timestamp)
    {
        BotAction action;
        action.type = BotActionType::ENTER_VEHICLE;
        action.botGuid = bot;
        action.targetGuid = vehicleGuid;
        action.spellId = static_cast<uint32>(seatId);  // Reuse spellId field for seat ID
        action.priority = 8;   // Vehicle boarding is high priority
        action.queuedTime = timestamp;
        return action;
    }

    static BotAction ExitVehicle(ObjectGuid bot, uint32 timestamp)
    {
        BotAction action;
        action.type = BotActionType::EXIT_VEHICLE;
        action.botGuid = bot;
        action.priority = 8;
        action.queuedTime = timestamp;
        return action;
    }
};

/**
 * @brief Result of action execution on main thread
 */
struct BotActionResult
{
    bool success{false};
    ::std::string errorMessage;

    static BotActionResult Success()
    {
        BotActionResult result;
        result.success = true;
        return result;
    }

    static BotActionResult Failure(::std::string const& error)
    {
        BotActionResult result;
        result.success = false;
        result.errorMessage = error;
        return result;
    }
};

} // namespace Playerbot

#endif // PLAYERBOT_BOT_ACTION_H
