/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotPacketSniffer.h"
#include "Core/Events/GenericEventBus.h"
#include "Resource/ResourceEvents.h"
#include "CombatPackets.h"
#include "Player.h"
#include "WorldSession.h"
#include "ObjectAccessor.h"
#include "Log.h"

namespace Playerbot
{

/**
 * @brief SMSG_HEALTH_UPDATE - Unit health changed
 *
 * Fired when any unit's health changes (player, bot, NPC, creature).
 * Critical for healing priority and threat assessment.
 */
void ParseTypedHealthUpdate(WorldSession* session, WorldPackets::Combat::HealthUpdate const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    // Get the unit whose health changed
    Unit* unit = ObjectAccessor::GetUnit(*bot, packet.Guid);
    if (!unit)
        return;

    // Create health update event (manual population - no factory method)
    ResourceEvent event;
    event.type = ResourceEventType::HEALTH_UPDATE;
    event.priority = ResourceEventPriority::HIGH;
    event.playerGuid = packet.Guid;
    event.powerType = Powers::POWER_MANA;  // Not used for health updates
    event.amount = static_cast<int32>(packet.Health);
    event.maxAmount = static_cast<int32>(unit->GetMaxHealth());
    event.isRegen = false;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(5);

    EventBus<ResourceEvent>::instance()->PublishEvent(event);

    // Calculate health percent for logging
    float healthPercent = unit->GetMaxHealth() > 0 ? (static_cast<float>(packet.Health) / unit->GetMaxHealth() * 100.0f) : 0.0f;
        if (!unit)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: unit in method GetMaxHealth");
            return;
        }

    TC_LOG_TRACE("playerbot.packets", "Bot {} received HEALTH_UPDATE (typed): Unit {} health={}/{} ({:.1f}%)",
        bot->GetName(),
        packet.Guid.ToString(),
        packet.Health,
        unit->GetMaxHealth(),
        healthPercent);
}

/**
 * @brief SMSG_POWER_UPDATE - Unit power changed (mana, rage, energy, etc.)
 *
 * Fired when any unit's power changes. Supports multiple power types.
 * Critical for resource management and spell usage decisions.
 */
void ParseTypedPowerUpdate(WorldSession* session, WorldPackets::Combat::PowerUpdate const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    // Get the unit whose power changed
    Unit* unit = ObjectAccessor::GetUnit(*bot, packet.Guid);
    if (!unit)
        return;

    // PowerUpdate can contain multiple power types
    for (auto const& powerInfo : packet.Powers)
    {
        // Map TrinityCore Powers enum to Playerbot Powers enum
        // TrinityCore::Powers is int8, packet has uint8
        ::Powers tcPowerType = static_cast<::Powers>(powerInfo.Power);
        int32 maxPower = unit->GetMaxPower(tcPowerType);

        // Map to Playerbot::Powers enum (WoW 12.0: limited subset)
        Powers botPowerType = Powers::POWER_MANA;  // Default
    switch (powerInfo.Power)
        {
            case 0: botPowerType = Powers::POWER_MANA; break;
            case 1: botPowerType = Powers::POWER_RAGE; break;
            case 2: botPowerType = Powers::POWER_FOCUS; break;
            case 3: botPowerType = Powers::POWER_ENERGY; break;
            case 6: botPowerType = Powers::POWER_RUNIC_POWER; break;
            default: continue;  // Skip unsupported power types
        }

        // Create power update event (manual population - no factory method)
        ResourceEvent event;
        event.type = ResourceEventType::POWER_UPDATE;
        event.priority = ResourceEventPriority::MEDIUM;
        event.playerGuid = packet.Guid;
        event.powerType = botPowerType;
        event.amount = powerInfo.Power;
        event.maxAmount = maxPower;
        event.isRegen = false;
        event.timestamp = ::std::chrono::steady_clock::now();
        event.expiryTime = event.timestamp + ::std::chrono::seconds(5);

        EventBus<ResourceEvent>::instance()->PublishEvent(event);

        // Calculate power percent for logging
        float powerPercent = maxPower > 0 ? (static_cast<float>(powerInfo.Power) / maxPower * 100.0f) : 0.0f;

        TC_LOG_TRACE("playerbot.packets", "Bot {} received POWER_UPDATE (typed): Unit {} powerType={} power={}/{} ({:.1f}%)",
            bot->GetName(),
            packet.Guid.ToString(),
            uint32(powerInfo.Power),
            powerInfo.Power,
            maxPower,
            powerPercent);
    }
}

/**
 * @brief SMSG_BREAK_TARGET - Target broken/cleared
 *
 * Fired when a unit's target is forcibly cleared (e.g., stealth, vanish, fear).
 * Critical for combat state tracking and target validation.
 */
void ParseTypedBreakTarget(WorldSession* session, WorldPackets::Combat::BreakTarget const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    // Create break target event (manual population - no factory method)
    ResourceEvent event;
    event.type = ResourceEventType::BREAK_TARGET;
    event.priority = ResourceEventPriority::HIGH;
    event.playerGuid = packet.UnitGUID;
    event.powerType = Powers::POWER_MANA;  // Not used for break target
    event.amount = 0;
    event.maxAmount = 0;
    event.isRegen = false;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::seconds(5);

    EventBus<ResourceEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received BREAK_TARGET (typed): Unit {} target broken",
        bot->GetName(),
        packet.UnitGUID.ToString());
}

/**
 * @brief Register all resource packet typed handlers
 *
 * Called during PlayerbotPacketSniffer initialization to register
 * type-safe packet interceptors for resource-related packets.
 */
void RegisterResourcePacketHandlers()
{
    // Register health tracking
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Combat::HealthUpdate>(&ParseTypedHealthUpdate);

    // Register power tracking
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Combat::PowerUpdate>(&ParseTypedPowerUpdate);

    // Register target break tracking
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Combat::BreakTarget>(&ParseTypedBreakTarget);

    TC_LOG_INFO("playerbot", "PlayerbotPacketSniffer: Registered {} Resource packet typed handlers", 3);
}

} // namespace Playerbot
