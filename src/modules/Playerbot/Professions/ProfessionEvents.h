/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_PROFESSION_EVENTS_H
#define PLAYERBOT_PROFESSION_EVENTS_H

#include "Define.h"
#include "ObjectGuid.h"
#include "ProfessionManager.h"
#include <chrono>
#include <string>

namespace Playerbot
{

enum class ProfessionEventType : uint8
{
    RECIPE_LEARNED = 0,         // Bot learned new recipe
    SKILL_UP,                   // Profession skill increased
    CRAFTING_STARTED,           // Bot started crafting item
    CRAFTING_COMPLETED,         // Bot completed crafting item
    CRAFTING_FAILED,            // Crafting attempt failed
    MATERIALS_NEEDED,           // Bot needs materials for recipe
    MATERIAL_GATHERED,          // Bot gathered material from node
    MATERIAL_PURCHASED,         // Bot bought material from AH
    ITEM_BANKED,                // Bot deposited item to bank
    ITEM_WITHDRAWN,             // Bot withdrew item from bank
    GOLD_BANKED,                // Bot deposited gold to bank
    GOLD_WITHDRAWN,             // Bot withdrew gold from bank
    MAX_PROFESSION_EVENT
};

enum class ProfessionEventPriority : uint8
{
    CRITICAL = 0,
    HIGH = 1,
    MEDIUM = 2,
    LOW = 3,
    BATCH = 4
};

/**
 * @brief Profession event data structure
 *
 * **Phase 5 Template Compatibility:**
 * This struct now includes all fields required by GenericEventBus<TEvent>:
 * - type (EventType)
 * - priority (Priority)
 * - timestamp (creation time)
 * - expiryTime (when event becomes invalid)
 * - IsValid() (validation check)
 * - IsExpired() (expiry check)
 * - ToString() (debug/logging)
 * - operator< (priority queue ordering)
 */
struct ProfessionEvent
{
    using EventType = ProfessionEventType;
    using Priority = ProfessionEventPriority;

    // Core event fields
    ProfessionEventType type;
    ProfessionEventPriority priority;
    std::chrono::steady_clock::time_point timestamp;
    std::chrono::steady_clock::time_point expiryTime;

    // Profession-specific fields
    ObjectGuid playerGuid;
    ProfessionType profession;
    uint32 recipeId;
    uint32 itemId;
    uint32 quantity;
    uint32 skillBefore;
    uint32 skillAfter;
    uint32 goldAmount;              // For banking events
    std::string reason;             // Human-readable event reason

    // GenericEventBus interface requirements
    bool IsValid() const;
    bool IsExpired() const;
    std::string ToString() const;

    /**
     * Priority queue ordering: Higher priority first
     * CRITICAL(0) > HIGH(1) > MEDIUM(2) > LOW(3) > BATCH(4)
     * Same priority: newer events first (reverse timestamp order)
     */
    bool operator<(ProfessionEvent const& other) const
    {
        if (priority != other.priority)
            return priority > other.priority;  // Lower numeric value = higher priority
        return timestamp > other.timestamp;    // Newer events last in queue
    }

    // Factory methods for each event type
    static ProfessionEvent RecipeLearned(ObjectGuid playerGuid, ProfessionType profession, uint32 recipeId);
    static ProfessionEvent SkillUp(ObjectGuid playerGuid, ProfessionType profession, uint32 skillBefore, uint32 skillAfter);
    static ProfessionEvent CraftingStarted(ObjectGuid playerGuid, ProfessionType profession, uint32 recipeId, uint32 itemId);
    static ProfessionEvent CraftingCompleted(ObjectGuid playerGuid, ProfessionType profession, uint32 recipeId, uint32 itemId, uint32 quantity);
    static ProfessionEvent CraftingFailed(ObjectGuid playerGuid, ProfessionType profession, uint32 recipeId, std::string const& reason);
    static ProfessionEvent MaterialsNeeded(ObjectGuid playerGuid, ProfessionType profession, uint32 recipeId);
    static ProfessionEvent MaterialGathered(ObjectGuid playerGuid, ProfessionType profession, uint32 itemId, uint32 quantity);
    static ProfessionEvent MaterialPurchased(ObjectGuid playerGuid, uint32 itemId, uint32 quantity, uint32 goldSpent);
    static ProfessionEvent ItemBanked(ObjectGuid playerGuid, uint32 itemId, uint32 quantity);
    static ProfessionEvent ItemWithdrawn(ObjectGuid playerGuid, uint32 itemId, uint32 quantity);
    static ProfessionEvent GoldBanked(ObjectGuid playerGuid, uint32 goldAmount);
    static ProfessionEvent GoldWithdrawn(ObjectGuid playerGuid, uint32 goldAmount);
};

} // namespace Playerbot

#endif // PLAYERBOT_PROFESSION_EVENTS_H
