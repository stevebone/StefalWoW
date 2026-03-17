/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * ACTIVITY TYPE DEFINITIONS
 *
 * Phase 3: Humanization Core
 *
 * Defines all possible activities a bot can engage in.
 * Used by HumanizationManager and ActivitySessionManager
 * to track what bots are doing and for how long.
 */

#pragma once

#include "Define.h"
#include <string>
#include <unordered_map>

namespace Playerbot
{
namespace Humanization
{

/**
 * @brief Categories of activities for high-level tracking
 */
enum class ActivityCategory : uint8
{
    IDLE = 0,           // Not actively doing anything
    COMBAT,             // Engaged in combat
    QUESTING,           // Doing quests
    GATHERING,          // Gathering professions (mining, herbing, skinning)
    CRAFTING,           // Crafting professions
    SOCIAL,             // Trading, grouping, chatting
    TRAVELING,          // Moving between locations
    CITY_LIFE,          // Activities in cities
    DUNGEONS,           // Running dungeons
    RAIDS,              // Raiding
    PVP,                // Battlegrounds, arenas, world PvP
    EXPLORATION,        // Exploring new areas
    FARMING,            // Farming mobs/gold
    MAINTENANCE,        // Vendor, repair, bank, etc.
    AFK,                // Away from keyboard simulation

    MAX_CATEGORY
};

/**
 * @brief Specific activity types for detailed tracking
 *
 * Each activity has associated behaviors, durations, and transitions.
 */
enum class ActivityType : uint8
{
    // ========================================================================
    // IDLE ACTIVITIES
    // ========================================================================
    NONE = 0,               // No activity
    STANDING_IDLE,          // Just standing around
    SITTING_IDLE,           // Sitting down
    EMOTING,                // Playing emotes

    // ========================================================================
    // COMBAT ACTIVITIES
    // ========================================================================
    SOLO_COMBAT,            // Fighting mobs alone
    GROUP_COMBAT,           // Fighting with group
    DUNGEON_COMBAT,         // Dungeon encounters
    RAID_COMBAT,            // Raid encounters
    PVP_COMBAT,             // PvP combat

    // ========================================================================
    // QUESTING ACTIVITIES
    // ========================================================================
    QUEST_PICKUP,           // Picking up quests
    QUEST_OBJECTIVE,        // Working on quest objectives
    QUEST_TURNIN,           // Turning in quests
    QUEST_TRAVEL,           // Traveling for quests

    // ========================================================================
    // GATHERING ACTIVITIES
    // ========================================================================
    MINING,                 // Mining ore nodes
    HERBALISM,              // Gathering herbs
    SKINNING,               // Skinning creatures
    FISHING,                // Fishing
    ARCHAEOLOGY,            // Archaeology digging

    // ========================================================================
    // CRAFTING ACTIVITIES
    // ========================================================================
    CRAFTING_SESSION,       // Active crafting
    LEARNING_RECIPES,       // Learning new recipes
    DISENCHANTING,          // Disenchanting items

    // ========================================================================
    // SOCIAL ACTIVITIES
    // ========================================================================
    TRADING,                // Trading with players
    GROUP_FORMING,          // Forming/joining groups
    CHATTING,               // Engaging in chat
    GUILD_ACTIVITY,         // Guild-related activities

    // ========================================================================
    // CITY LIFE ACTIVITIES
    // ========================================================================
    AUCTION_BROWSING,       // Browsing auction house
    AUCTION_POSTING,        // Posting auctions
    MAILBOX_CHECK,          // Checking mail
    BANK_VISIT,             // Using bank
    VENDOR_VISIT,           // Visiting vendors
    TRAINER_VISIT,          // Visiting trainers
    INN_REST,               // Resting at inn
    CITY_WANDERING,         // Wandering around city
    TRANSMOG_BROWSING,      // Looking at transmog

    // ========================================================================
    // DUNGEON ACTIVITIES
    // ========================================================================
    DUNGEON_QUEUE,          // Queued for dungeon
    DUNGEON_RUN,            // Running a dungeon
    DUNGEON_LOOT,           // Looting in dungeon

    // ========================================================================
    // RAID ACTIVITIES
    // ========================================================================
    RAID_PREP,              // Preparing for raid
    RAID_ENCOUNTER,         // In raid encounter
    RAID_BREAK,             // Break during raid

    // ========================================================================
    // PVP ACTIVITIES
    // ========================================================================
    BG_QUEUE,               // Queued for battleground
    BATTLEGROUND,           // In battleground
    ARENA,                  // In arena
    WORLD_PVP,              // World PvP

    // ========================================================================
    // TRAVEL ACTIVITIES
    // ========================================================================
    WALKING,                // Walking travel
    MOUNTED_TRAVEL,         // Mounted travel
    FLYING,                 // Flying travel
    FLIGHT_PATH,            // Using flight paths
    PORTAL_TRAVEL,          // Using portals

    // ========================================================================
    // MAINTENANCE ACTIVITIES
    // ========================================================================
    REPAIRING,              // Repairing gear
    RESTOCKING,             // Buying supplies
    VENDORING,              // Selling items
    TALENT_MANAGEMENT,      // Managing talents

    // ========================================================================
    // FARMING ACTIVITIES
    // ========================================================================
    MOB_FARMING,            // Farming mobs
    GOLD_FARMING,           // Gold-focused farming
    REP_FARMING,            // Reputation farming
    TRANSMOG_FARMING,       // Transmog farming
    MOUNT_FARMING,          // Mount farming

    // ========================================================================
    // EXPLORATION
    // ========================================================================
    ZONE_EXPLORATION,       // Exploring a zone
    ACHIEVEMENT_HUNTING,    // Working on achievements

    // ========================================================================
    // AFK SIMULATION
    // ========================================================================
    AFK_SHORT,              // Short AFK (1-5 min)
    AFK_MEDIUM,             // Medium AFK (5-15 min)
    AFK_LONG,               // Long AFK (15-30 min)
    AFK_BIO_BREAK,          // Bio break simulation

    MAX_ACTIVITY_TYPE
};

/**
 * @brief Activity information structure
 */
struct ActivityInfo
{
    ActivityType type;
    ActivityCategory category;
    std::string name;
    std::string description;
    uint32 minDurationMs;       // Minimum session duration
    uint32 maxDurationMs;       // Maximum session duration
    float interruptionChance;   // Chance to be interrupted (0.0-1.0)
    bool canBeInterrupted;      // Can this activity be interrupted
    bool requiresLocation;      // Does this require a specific location
    bool isGroupActivity;       // Is this a group activity

    ActivityInfo()
        : type(ActivityType::NONE)
        , category(ActivityCategory::IDLE)
        , minDurationMs(60000)
        , maxDurationMs(300000)
        , interruptionChance(0.1f)
        , canBeInterrupted(true)
        , requiresLocation(false)
        , isGroupActivity(false)
    {}
};

/**
 * @brief Activity transition probability
 */
struct ActivityTransition
{
    ActivityType fromActivity;
    ActivityType toActivity;
    float probability;          // Probability of this transition (0.0-1.0)
    uint32 cooldownMs;          // Cooldown before this transition can happen again

    ActivityTransition()
        : fromActivity(ActivityType::NONE)
        , toActivity(ActivityType::NONE)
        , probability(0.0f)
        , cooldownMs(0)
    {}

    ActivityTransition(ActivityType from, ActivityType to, float prob, uint32 cooldown = 0)
        : fromActivity(from)
        , toActivity(to)
        , probability(prob)
        , cooldownMs(cooldown)
    {}
};

/**
 * @brief Get the category for an activity type
 */
inline ActivityCategory GetActivityCategory(ActivityType type)
{
    switch (type)
    {
        case ActivityType::NONE:
        case ActivityType::STANDING_IDLE:
        case ActivityType::SITTING_IDLE:
        case ActivityType::EMOTING:
            return ActivityCategory::IDLE;

        case ActivityType::SOLO_COMBAT:
        case ActivityType::GROUP_COMBAT:
        case ActivityType::DUNGEON_COMBAT:
        case ActivityType::RAID_COMBAT:
        case ActivityType::PVP_COMBAT:
            return ActivityCategory::COMBAT;

        case ActivityType::QUEST_PICKUP:
        case ActivityType::QUEST_OBJECTIVE:
        case ActivityType::QUEST_TURNIN:
        case ActivityType::QUEST_TRAVEL:
            return ActivityCategory::QUESTING;

        case ActivityType::MINING:
        case ActivityType::HERBALISM:
        case ActivityType::SKINNING:
        case ActivityType::FISHING:
        case ActivityType::ARCHAEOLOGY:
            return ActivityCategory::GATHERING;

        case ActivityType::CRAFTING_SESSION:
        case ActivityType::LEARNING_RECIPES:
        case ActivityType::DISENCHANTING:
            return ActivityCategory::CRAFTING;

        case ActivityType::TRADING:
        case ActivityType::GROUP_FORMING:
        case ActivityType::CHATTING:
        case ActivityType::GUILD_ACTIVITY:
            return ActivityCategory::SOCIAL;

        case ActivityType::AUCTION_BROWSING:
        case ActivityType::AUCTION_POSTING:
        case ActivityType::MAILBOX_CHECK:
        case ActivityType::BANK_VISIT:
        case ActivityType::VENDOR_VISIT:
        case ActivityType::TRAINER_VISIT:
        case ActivityType::INN_REST:
        case ActivityType::CITY_WANDERING:
        case ActivityType::TRANSMOG_BROWSING:
            return ActivityCategory::CITY_LIFE;

        case ActivityType::DUNGEON_QUEUE:
        case ActivityType::DUNGEON_RUN:
        case ActivityType::DUNGEON_LOOT:
            return ActivityCategory::DUNGEONS;

        case ActivityType::RAID_PREP:
        case ActivityType::RAID_ENCOUNTER:
        case ActivityType::RAID_BREAK:
            return ActivityCategory::RAIDS;

        case ActivityType::BG_QUEUE:
        case ActivityType::BATTLEGROUND:
        case ActivityType::ARENA:
        case ActivityType::WORLD_PVP:
            return ActivityCategory::PVP;

        case ActivityType::WALKING:
        case ActivityType::MOUNTED_TRAVEL:
        case ActivityType::FLYING:
        case ActivityType::FLIGHT_PATH:
        case ActivityType::PORTAL_TRAVEL:
            return ActivityCategory::TRAVELING;

        case ActivityType::REPAIRING:
        case ActivityType::RESTOCKING:
        case ActivityType::VENDORING:
        case ActivityType::TALENT_MANAGEMENT:
            return ActivityCategory::MAINTENANCE;

        case ActivityType::MOB_FARMING:
        case ActivityType::GOLD_FARMING:
        case ActivityType::REP_FARMING:
        case ActivityType::TRANSMOG_FARMING:
        case ActivityType::MOUNT_FARMING:
            return ActivityCategory::FARMING;

        case ActivityType::ZONE_EXPLORATION:
        case ActivityType::ACHIEVEMENT_HUNTING:
            return ActivityCategory::EXPLORATION;

        case ActivityType::AFK_SHORT:
        case ActivityType::AFK_MEDIUM:
        case ActivityType::AFK_LONG:
        case ActivityType::AFK_BIO_BREAK:
            return ActivityCategory::AFK;

        default:
            return ActivityCategory::IDLE;
    }
}

/**
 * @brief Get human-readable name for an activity type
 */
inline std::string GetActivityName(ActivityType type)
{
    static const std::unordered_map<ActivityType, std::string> names = {
        {ActivityType::NONE, "None"},
        {ActivityType::STANDING_IDLE, "Standing Idle"},
        {ActivityType::SITTING_IDLE, "Sitting"},
        {ActivityType::EMOTING, "Emoting"},
        {ActivityType::SOLO_COMBAT, "Solo Combat"},
        {ActivityType::GROUP_COMBAT, "Group Combat"},
        {ActivityType::DUNGEON_COMBAT, "Dungeon Combat"},
        {ActivityType::RAID_COMBAT, "Raid Combat"},
        {ActivityType::PVP_COMBAT, "PvP Combat"},
        {ActivityType::QUEST_PICKUP, "Quest Pickup"},
        {ActivityType::QUEST_OBJECTIVE, "Quest Objective"},
        {ActivityType::QUEST_TURNIN, "Quest Turn-in"},
        {ActivityType::QUEST_TRAVEL, "Quest Travel"},
        {ActivityType::MINING, "Mining"},
        {ActivityType::HERBALISM, "Herbalism"},
        {ActivityType::SKINNING, "Skinning"},
        {ActivityType::FISHING, "Fishing"},
        {ActivityType::ARCHAEOLOGY, "Archaeology"},
        {ActivityType::CRAFTING_SESSION, "Crafting"},
        {ActivityType::LEARNING_RECIPES, "Learning Recipes"},
        {ActivityType::DISENCHANTING, "Disenchanting"},
        {ActivityType::TRADING, "Trading"},
        {ActivityType::GROUP_FORMING, "Group Forming"},
        {ActivityType::CHATTING, "Chatting"},
        {ActivityType::GUILD_ACTIVITY, "Guild Activity"},
        {ActivityType::AUCTION_BROWSING, "Auction Browsing"},
        {ActivityType::AUCTION_POSTING, "Auction Posting"},
        {ActivityType::MAILBOX_CHECK, "Mailbox Check"},
        {ActivityType::BANK_VISIT, "Bank Visit"},
        {ActivityType::VENDOR_VISIT, "Vendor Visit"},
        {ActivityType::TRAINER_VISIT, "Trainer Visit"},
        {ActivityType::INN_REST, "Inn Rest"},
        {ActivityType::CITY_WANDERING, "City Wandering"},
        {ActivityType::TRANSMOG_BROWSING, "Transmog Browsing"},
        {ActivityType::DUNGEON_QUEUE, "Dungeon Queue"},
        {ActivityType::DUNGEON_RUN, "Dungeon Run"},
        {ActivityType::DUNGEON_LOOT, "Dungeon Loot"},
        {ActivityType::RAID_PREP, "Raid Prep"},
        {ActivityType::RAID_ENCOUNTER, "Raid Encounter"},
        {ActivityType::RAID_BREAK, "Raid Break"},
        {ActivityType::BG_QUEUE, "BG Queue"},
        {ActivityType::BATTLEGROUND, "Battleground"},
        {ActivityType::ARENA, "Arena"},
        {ActivityType::WORLD_PVP, "World PvP"},
        {ActivityType::WALKING, "Walking"},
        {ActivityType::MOUNTED_TRAVEL, "Mounted Travel"},
        {ActivityType::FLYING, "Flying"},
        {ActivityType::FLIGHT_PATH, "Flight Path"},
        {ActivityType::PORTAL_TRAVEL, "Portal Travel"},
        {ActivityType::REPAIRING, "Repairing"},
        {ActivityType::RESTOCKING, "Restocking"},
        {ActivityType::VENDORING, "Vendoring"},
        {ActivityType::TALENT_MANAGEMENT, "Talent Management"},
        {ActivityType::MOB_FARMING, "Mob Farming"},
        {ActivityType::GOLD_FARMING, "Gold Farming"},
        {ActivityType::REP_FARMING, "Rep Farming"},
        {ActivityType::TRANSMOG_FARMING, "Transmog Farming"},
        {ActivityType::MOUNT_FARMING, "Mount Farming"},
        {ActivityType::ZONE_EXPLORATION, "Zone Exploration"},
        {ActivityType::ACHIEVEMENT_HUNTING, "Achievement Hunting"},
        {ActivityType::AFK_SHORT, "Short AFK"},
        {ActivityType::AFK_MEDIUM, "Medium AFK"},
        {ActivityType::AFK_LONG, "Long AFK"},
        {ActivityType::AFK_BIO_BREAK, "Bio Break"}
    };

    auto it = names.find(type);
    return it != names.end() ? it->second : "Unknown";
}

/**
 * @brief Get human-readable name for an activity category
 */
inline std::string GetCategoryName(ActivityCategory category)
{
    static const std::unordered_map<ActivityCategory, std::string> names = {
        {ActivityCategory::IDLE, "Idle"},
        {ActivityCategory::COMBAT, "Combat"},
        {ActivityCategory::QUESTING, "Questing"},
        {ActivityCategory::GATHERING, "Gathering"},
        {ActivityCategory::CRAFTING, "Crafting"},
        {ActivityCategory::SOCIAL, "Social"},
        {ActivityCategory::TRAVELING, "Traveling"},
        {ActivityCategory::CITY_LIFE, "City Life"},
        {ActivityCategory::DUNGEONS, "Dungeons"},
        {ActivityCategory::RAIDS, "Raids"},
        {ActivityCategory::PVP, "PvP"},
        {ActivityCategory::EXPLORATION, "Exploration"},
        {ActivityCategory::FARMING, "Farming"},
        {ActivityCategory::MAINTENANCE, "Maintenance"},
        {ActivityCategory::AFK, "AFK"}
    };

    auto it = names.find(category);
    return it != names.end() ? it->second : "Unknown";
}

} // namespace Humanization
} // namespace Playerbot
