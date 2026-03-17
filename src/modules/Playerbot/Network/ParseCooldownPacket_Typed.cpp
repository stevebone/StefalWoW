/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#include "PlayerbotPacketSniffer.h"
#include "WorldSession.h"
#include "Player.h"
#include "Core/Events/GenericEventBus.h"
#include "Cooldown/CooldownEvents.h"
#include "SpellPackets.h"
#include "ItemPackets.h"
#include "Log.h"

namespace Playerbot
{

void ParseTypedSpellCooldown(WorldSession* session, WorldPackets::Spells::SpellCooldown const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    for (auto const& cooldownEntry : packet.SpellCooldowns)
    {
        CooldownEvent event;
        event.type = CooldownEventType::SPELL_COOLDOWN_START;
        event.casterGuid = packet.Caster;
        event.spellId = cooldownEntry.SrecID;  // WoW 12.0: Field is SrecID, not SpellID
        event.itemId = 0;
        event.category = 0;
        event.cooldownMs = cooldownEntry.ForcedCooldown;
        event.modRateMs = static_cast<int32>(cooldownEntry.ModRate);
        event.timestamp = ::std::chrono::steady_clock::now();
        EventBus<CooldownEvent>::instance()->PublishEvent(event);
    }

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received SPELL_COOLDOWN (typed): {} cooldowns",
        bot->GetName(), packet.SpellCooldowns.size());
}

void ParseTypedCooldownEvent(WorldSession* session, WorldPackets::Spells::CooldownEvent const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    CooldownEvent event;
    event.type = CooldownEventType::SPELL_COOLDOWN_START;
    event.casterGuid = bot->GetGUID();
    event.spellId = packet.SpellID;
    event.itemId = 0;
    event.category = 0;
    event.cooldownMs = 0;
    event.modRateMs = 0;
    event.timestamp = ::std::chrono::steady_clock::now();

    EventBus<CooldownEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received COOLDOWN_EVENT (typed): spell={}",
        bot->GetName(), packet.SpellID);
}

void ParseTypedClearCooldown(WorldSession* session, WorldPackets::Spells::ClearCooldown const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    CooldownEvent event;
    event.type = CooldownEventType::SPELL_COOLDOWN_CLEAR;
    event.casterGuid = bot->GetGUID();  // WoW 12.0: ClearCooldown has no CasterGUID field, use bot GUID
    event.spellId = static_cast<uint32>(packet.SpellID);
    event.itemId = 0;
    event.category = 0;
    event.cooldownMs = 0;
    event.modRateMs = 0;
    event.timestamp = ::std::chrono::steady_clock::now();

    EventBus<CooldownEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received CLEAR_COOLDOWN (typed): spell={}, isPet={}",
        bot->GetName(), packet.SpellID, packet.IsPet);
}

void ParseTypedClearCooldowns(WorldSession* session, WorldPackets::Spells::ClearCooldowns const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    for (int32 spellId : packet.SpellID)  // WoW 12.0: Field is SpellID (singular), not SpellIDs (plural)
    {
        CooldownEvent event;
        event.type = CooldownEventType::SPELL_COOLDOWN_CLEAR;
        event.casterGuid = bot->GetGUID();
        event.spellId = static_cast<uint32>(spellId);
        event.itemId = 0;
        event.category = 0;
        event.cooldownMs = 0;
        event.modRateMs = 0;
        event.timestamp = ::std::chrono::steady_clock::now();

        EventBus<CooldownEvent>::instance()->PublishEvent(event);
    }

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received CLEAR_COOLDOWNS (typed): {} spells",
        bot->GetName(), packet.SpellID.size());
}

void ParseTypedModifyCooldown(WorldSession* session, WorldPackets::Spells::ModifyCooldown const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    CooldownEvent event;
    event.type = CooldownEventType::SPELL_COOLDOWN_MODIFY;
    event.casterGuid = bot->GetGUID();
    event.spellId = packet.SpellID;
    event.itemId = 0;
    event.category = 0;
    event.cooldownMs = 0;
    event.modRateMs = packet.DeltaTime;
    event.timestamp = ::std::chrono::steady_clock::now();

    EventBus<CooldownEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received MODIFY_COOLDOWN (typed): spell={}, delta={}ms",
        bot->GetName(), packet.SpellID, packet.DeltaTime);
}

void RegisterCooldownPacketHandlers()
{
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Spells::SpellCooldown>(&ParseTypedSpellCooldown);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Spells::CooldownEvent>(&ParseTypedCooldownEvent);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Spells::ClearCooldown>(&ParseTypedClearCooldown);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Spells::ClearCooldowns>(&ParseTypedClearCooldowns);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Spells::ModifyCooldown>(&ParseTypedModifyCooldown);

    TC_LOG_INFO("playerbot", "PlayerbotPacketSniffer: Registered {} Cooldown packet typed handlers", 5);
}

} // namespace Playerbot
