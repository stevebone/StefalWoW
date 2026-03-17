/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Extended Bot Action Types for Lock-Free Architecture
 * Adds quest and gathering specific actions to support refactored systems
 */

#ifndef PLAYERBOT_BOT_ACTION_EXTENDED_H
#define PLAYERBOT_BOT_ACTION_EXTENDED_H

#include "BotAction.h"
#include "ObjectGuid.h"
#include "Position.h"

namespace Playerbot
{

/**
 * Extended action types for lock-free quest and gathering systems
 */
enum class BotActionTypeExtended : uint8
{
    // Base actions (from BotActionType)
    ATTACK_TARGET = 0,
    CAST_SPELL,
    STOP_ATTACK,
    MOVE_TO_POSITION,
    FOLLOW_TARGET,
    STOP_MOVEMENT,
    INTERACT_OBJECT,
    INTERACT_NPC,
    LOOT_OBJECT,
    ACCEPT_GROUP_INVITE,
    LEAVE_GROUP,
    ACCEPT_QUEST,
    TURN_IN_QUEST,
    USE_ITEM,
    EQUIP_ITEM,
    SEND_CHAT_MESSAGE,
    EMOTE,
    CUSTOM,

    // Quest-specific actions (new)
    KILL_QUEST_TARGET = 20,     // Attack creature for quest objective
    TALK_TO_QUEST_NPC,          // Interact with NPC for quest dialogue
    INTERACT_QUEST_OBJECT,      // Use GameObject for quest
    ESCORT_NPC,                 // Follow and protect escort NPC
    DEFEND_ESCORT,              // Attack enemies threatening escort

    // Gathering actions (new)
    SKIN_CREATURE = 30,         // Cast skinning on corpse
    GATHER_OBJECT,              // Mine/herb from GameObject
    LOOT_GATHERING_NODE,        // Loot after gathering complete

    // Advanced combat actions (new)
    ASSIST_PLAYER = 40,         // Help another player in combat
    FLEE_FROM_COMBAT,           // Run away from overwhelming odds
    USE_CONSUMABLE,             // Use potion/food/bandage

    // Social actions (new)
    TRADE_WITH_PLAYER = 50,     // Initiate trade
    ACCEPT_TRADE,               // Accept trade offer
    DECLINE_TRADE,              // Decline trade offer

    // Movement refinements (new)
    MOVE_TO_CORPSE = 60,        // Move to corpse for resurrection
    PATROL_PATH,                // Follow patrol waypoints
    EXPLORE_AREA,               // Random exploration

    // Crafting actions (new)
    CRAFT_ITEM = 70,            // Create item via profession
    ENCHANT_ITEM,               // Apply enchantment
    USE_PROFESSION_SKILL,       // Generic profession action
};

/**
 * Extended bot action with additional quest/gathering data
 */
struct BotActionExtended : public BotAction
{
    // Additional quest-specific data
    uint32 objectiveIndex{0};      // Which objective in the quest
    uint32 objectiveProgress{0};   // Current progress
    bool isQuestComplete{false};   // Quest ready for turn-in

    // Additional gathering data
    uint32 nodeEntry{0};           // GameObject/Creature entry
    uint32 gatheringSkillId{0};    // Required skill (mining/herb/skinning)
    uint32 skillLevel{0};          // Required skill level

    // Combat enhancements
    float threatLevel{0.0f};       // Threat percentage
    uint8 comboPoints{0};          // For rogue combos
    bool isElite{false};           // Target is elite

    // Extended factory methods
    static BotActionExtended KillQuestTarget(
        ObjectGuid bot, ObjectGuid target,
        uint32 questId, uint32 objectiveIndex,
        uint32 timestamp)
    {
        BotActionExtended action;
        action.type = static_cast<BotActionType>(BotActionTypeExtended::KILL_QUEST_TARGET);
        action.botGuid = bot;
        action.targetGuid = target;
        action.questId = questId;
        action.objectiveIndex = objectiveIndex;
        action.priority = 8;  // Quest combat is important
        action.queuedTime = timestamp;
        return action;
    }

    static BotActionExtended TalkToQuestNPC(
        ObjectGuid bot, ObjectGuid npc,
        uint32 questId, uint32 timestamp)
    {
        BotActionExtended action;
        action.type = static_cast<BotActionType>(BotActionTypeExtended::TALK_TO_QUEST_NPC);
        action.botGuid = bot;
        action.targetGuid = npc;
        action.questId = questId;
        action.priority = 6;
        action.queuedTime = timestamp;
        return action;
    }

    static BotActionExtended InteractQuestObject(
        ObjectGuid bot, ObjectGuid object,
        uint32 questId, uint32 objectiveIndex,
        uint32 timestamp)
    {
        BotActionExtended action;
        action.type = static_cast<BotActionType>(BotActionTypeExtended::INTERACT_QUEST_OBJECT);
        action.botGuid = bot;
        action.targetGuid = object;
        action.questId = questId;
        action.objectiveIndex = objectiveIndex;
        action.priority = 6;
        action.queuedTime = timestamp;
        return action;
    }

    static BotActionExtended EscortNPC(
        ObjectGuid bot, ObjectGuid escort,
        uint32 questId, uint32 timestamp)
    {
        BotActionExtended action;
        action.type = static_cast<BotActionType>(BotActionTypeExtended::ESCORT_NPC);
        action.botGuid = bot;
        action.targetGuid = escort;
        action.questId = questId;
        action.priority = 7;
        action.queuedTime = timestamp;
        return action;
    }

    static BotActionExtended SkinCreature(
        ObjectGuid bot, ObjectGuid creature,
        uint32 spellId, uint32 skillRequired,
        uint32 timestamp)
    {
        BotActionExtended action;
        action.type = static_cast<BotActionType>(BotActionTypeExtended::SKIN_CREATURE);
        action.botGuid = bot;
        action.targetGuid = creature;
        action.spellId = spellId;
        action.skillLevel = skillRequired;
        action.gatheringSkillId = 393;  // SKILL_SKINNING
        action.priority = 4;
        action.queuedTime = timestamp;
        return action;
    }

    static BotActionExtended GatherObject(
        ObjectGuid bot, ObjectGuid object,
        uint32 spellId, uint32 skillId,
        uint32 skillRequired, uint32 timestamp)
    {
        BotActionExtended action;
        action.type = static_cast<BotActionType>(BotActionTypeExtended::GATHER_OBJECT);
        action.botGuid = bot;
        action.targetGuid = object;
        action.spellId = spellId;
        action.gatheringSkillId = skillId;
        action.skillLevel = skillRequired;
        action.priority = 5;
        action.queuedTime = timestamp;
        return action;
    }

    static BotActionExtended DefendEscort(
        ObjectGuid bot, ObjectGuid attacker,
        ObjectGuid escort, uint32 questId,
        uint32 timestamp)
    {
        BotActionExtended action;
        action.type = static_cast<BotActionType>(BotActionTypeExtended::DEFEND_ESCORT);
        action.botGuid = bot;
        action.targetGuid = attacker;  // Attack the threat
        action.questId = questId;
        action.priority = 9;  // High priority to save escort
        action.queuedTime = timestamp;
        return action;
    }

    static BotActionExtended AssistPlayer(
        ObjectGuid bot, ObjectGuid playerToAssist,
        ObjectGuid enemy, uint32 timestamp)
    {
        BotActionExtended action;
        action.type = static_cast<BotActionType>(BotActionTypeExtended::ASSIST_PLAYER);
        action.botGuid = bot;
        action.targetGuid = enemy;
        action.priority = 8;
        action.queuedTime = timestamp;
        return action;
    }

    static BotActionExtended FleeFromCombat(
        ObjectGuid bot, Position const& safePosition,
        uint32 timestamp)
    {
        BotActionExtended action;
        action.type = static_cast<BotActionType>(BotActionTypeExtended::FLEE_FROM_COMBAT);
        action.botGuid = bot;
        action.position = safePosition;
        action.priority = 10;  // Maximum priority for survival
        action.queuedTime = timestamp;
        return action;
    }

    static BotActionExtended UseConsumable(
        ObjectGuid bot, uint32 itemId,
        uint32 timestamp)
    {
        BotActionExtended action;
        action.type = static_cast<BotActionType>(BotActionTypeExtended::USE_CONSUMABLE);
        action.botGuid = bot;
        action.itemEntry = itemId;
        action.priority = 9;  // High priority for healing/mana
        action.queuedTime = timestamp;
        return action;
    }
};

/**
 * Action validation helper
 */
class BotActionValidator
{
public:
    static bool ValidateAction(BotActionExtended const& action)
    {
        // Basic validation
        if (action.botGuid.IsEmpty())
            return false;

        // Type-specific validation
        switch (static_cast<BotActionTypeExtended>(action.type))
        {
            case BotActionTypeExtended::KILL_QUEST_TARGET:
            case BotActionTypeExtended::TALK_TO_QUEST_NPC:
            case BotActionTypeExtended::INTERACT_QUEST_OBJECT:
                return action.questId != 0 && !action.targetGuid.IsEmpty();

            case BotActionTypeExtended::ESCORT_NPC:
            case BotActionTypeExtended::DEFEND_ESCORT:
                return action.questId != 0 && !action.targetGuid.IsEmpty();

            case BotActionTypeExtended::SKIN_CREATURE:
                return !action.targetGuid.IsEmpty() && action.spellId != 0;

            case BotActionTypeExtended::GATHER_OBJECT:
                return !action.targetGuid.IsEmpty() &&
                       action.spellId != 0 &&
                       action.gatheringSkillId != 0;

            case BotActionTypeExtended::FLEE_FROM_COMBAT:
                return action.position.IsPositionValid();

            case BotActionTypeExtended::USE_CONSUMABLE:
                return action.itemEntry != 0;

            default:
                return true;  // Basic actions assumed valid
        }
    }

    static uint8 GetActionPriority(BotActionTypeExtended type)
    {
        switch (type)
        {
            case BotActionTypeExtended::FLEE_FROM_COMBAT:
                return 10;  // Survival highest

            case BotActionTypeExtended::DEFEND_ESCORT:
            case BotActionTypeExtended::USE_CONSUMABLE:
                return 9;   // Critical actions

            case BotActionTypeExtended::KILL_QUEST_TARGET:
            case BotActionTypeExtended::ASSIST_PLAYER:
                return 8;   // Important combat

            case BotActionTypeExtended::ESCORT_NPC:
                return 7;   // Quest critical

            case BotActionTypeExtended::TALK_TO_QUEST_NPC:
            case BotActionTypeExtended::INTERACT_QUEST_OBJECT:
                return 6;   // Quest progress

            case BotActionTypeExtended::GATHER_OBJECT:
                return 5;   // Gathering medium

            case BotActionTypeExtended::SKIN_CREATURE:
                return 4;   // Post-combat gathering

            default:
                return 3;   // Default priority
        }
    }
};

} // namespace Playerbot

#endif // PLAYERBOT_BOT_ACTION_EXTENDED_H