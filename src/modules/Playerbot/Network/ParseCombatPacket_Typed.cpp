/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotPacketSniffer.h"
#include "WorldSession.h"
#include "Player.h"
#include "Core/Events/GenericEventBus.h"
#include "Combat/CombatEvents.h"
#include "SpellPackets.h"
#include "CombatPackets.h"
#include "CombatLogPackets.h"
#include "Log.h"

namespace Playerbot
{

// ================================================================================================
// TYPED PACKET HANDLERS - COMBAT CATEGORY
// These handlers receive TYPED packet objects before serialization (WoW 12.0 Solution)
// ================================================================================================

/**
 * Spell Cast Start - Typed Handler
 * Critical for interrupt detection
 */
void ParseTypedSpellStart(WorldSession* session, WorldPackets::Spells::SpellStart const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    // WoW 12.0: TargetGUID removed, extract from Target.Unit or HitTargets[0]
    ObjectGuid targetGuid = packet.Cast.Target.Unit;
    if (targetGuid.IsEmpty() && !packet.Cast.HitTargets.empty())
        targetGuid = packet.Cast.HitTargets[0];

    CombatEvent event = CombatEvent::SpellCastStart(
        packet.Cast.CasterGUID,
        targetGuid,
        packet.Cast.SpellID,
        packet.Cast.CastTime
    );

    EventBus<CombatEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received SPELL_START (typed): caster={}, target={}, spell={}, castTime={}ms",
        bot->GetName(), packet.Cast.CasterGUID.ToString(), targetGuid.ToString(),
        packet.Cast.SpellID, packet.Cast.CastTime);
}
/**
 * Spell Cast Go - Typed Handler
 * Spell completes successfully
 */
void ParseTypedSpellGo(WorldSession* session, WorldPackets::Spells::SpellGo const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    // WoW 12.0: TargetGUID removed, extract from Target.Unit or HitTargets[0]
    ObjectGuid targetGuid = packet.Cast.Target.Unit;
    if (targetGuid.IsEmpty() && !packet.Cast.HitTargets.empty())
        targetGuid = packet.Cast.HitTargets[0];

    CombatEvent event = CombatEvent::SpellCastGo(
        packet.Cast.CasterGUID,
        targetGuid,
        packet.Cast.SpellID
    );

    EventBus<CombatEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received SPELL_GO (typed): caster={}, target={}, spell={}",
        bot->GetName(), packet.Cast.CasterGUID.ToString(), targetGuid.ToString(), packet.Cast.SpellID);
}

/**
 * Spell Failure - Typed Handler
 */
void ParseTypedSpellFailure(WorldSession* session, WorldPackets::Spells::SpellFailure const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    CombatEvent event;
    event.type = CombatEventType::SPELL_CAST_FAILED;
    event.priority = CombatEventPriority::MEDIUM;
    event.casterGuid = packet.CasterUnit;
    event.targetGuid = ObjectGuid::Empty;
    event.victimGuid = ObjectGuid::Empty;
    event.spellId = packet.SpellID;
    event.amount = static_cast<int32>(packet.Reason);
    event.schoolMask = 0;
    event.flags = 0;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::milliseconds(5000);

    EventBus<CombatEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received SPELL_FAILURE (typed): caster={}, spell={}, reason={}",
        bot->GetName(), packet.CasterUnit.ToString(), packet.SpellID, static_cast<uint32>(packet.Reason));
}

/**
 * Spell Failed Other - Typed Handler
 */
void ParseTypedSpellFailedOther(WorldSession* session, WorldPackets::Spells::SpellFailedOther const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    CombatEvent event;
    event.type = CombatEventType::SPELL_CAST_FAILED;
    event.priority = CombatEventPriority::LOW;
    event.casterGuid = packet.CasterUnit;
    event.targetGuid = ObjectGuid::Empty;
    event.victimGuid = ObjectGuid::Empty;
    event.spellId = packet.SpellID;
    event.amount = static_cast<int32>(packet.Reason);
    event.schoolMask = 0;
    event.flags = 0;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::milliseconds(5000);
    EventBus<CombatEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received SPELL_FAILED_OTHER (typed): caster={}, spell={}",
        bot->GetName(), packet.CasterUnit.ToString(), packet.SpellID);
}

/**
 * Spell Energize - Typed Handler
 */
void ParseTypedSpellEnergize(WorldSession* session, WorldPackets::CombatLog::SpellEnergizeLog const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    CombatEvent event;
    event.type = CombatEventType::SPELL_ENERGIZE;
    event.priority = CombatEventPriority::MEDIUM;
    event.casterGuid = packet.CasterGUID;
    event.targetGuid = packet.TargetGUID;
    event.victimGuid = packet.TargetGUID;
    event.spellId = packet.SpellID;
    event.amount = packet.Amount;
    event.schoolMask = static_cast<uint32>(packet.Type);
    event.flags = 0;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::milliseconds(5000);

    EventBus<CombatEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received SPELL_ENERGIZE (typed): caster={}, target={}, spell={}, amount={}, type={}",
        bot->GetName(), packet.CasterGUID.ToString(), packet.TargetGUID.ToString(),
        packet.SpellID, packet.Amount, static_cast<uint32>(packet.Type));
}

/**
 * Spell Interrupt - Typed Handler
 * CRITICAL for interrupt coordination
 */
void ParseTypedSpellInterrupt(WorldSession* session, WorldPackets::CombatLog::SpellInterruptLog const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    CombatEvent event = CombatEvent::SpellInterrupt(
        packet.Caster,
        packet.Victim,
        packet.InterruptedSpellID,
        packet.SpellID  // WoW 12.0: Field is SpellID, not InterruptingSpellID
    );

    EventBus<CombatEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received SPELL_INTERRUPT (typed): interrupter={}, victim={}, interruptedSpell={}, interruptSpell={}",
        bot->GetName(), packet.Caster.ToString(), packet.Victim.ToString(),
        packet.InterruptedSpellID, packet.SpellID);
}

/**
 * Spell Dispel - Typed Handler
 */
void ParseTypedSpellDispel(WorldSession* session, WorldPackets::CombatLog::SpellDispellLog const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    // Publish event for each dispelled aura
    for (auto const& dispellData : packet.DispellData)
    {
        CombatEvent event;
        event.type = CombatEventType::SPELL_DISPELLED;
        event.priority = CombatEventPriority::HIGH;
        event.casterGuid = packet.CasterGUID;  // WoW 12.0: Field is CasterGUID, not DispellerGUID
        event.targetGuid = packet.TargetGUID;
        event.victimGuid = packet.TargetGUID;
        event.spellId = packet.DispelledBySpellID;
        event.amount = static_cast<int32>(dispellData.SpellID);
        event.schoolMask = 0;
        event.flags = 0;
        event.timestamp = ::std::chrono::steady_clock::now();
        event.expiryTime = event.timestamp + ::std::chrono::milliseconds(5000);

        EventBus<CombatEvent>::instance()->PublishEvent(event);
    }

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received SPELL_DISPEL (typed): dispeller={}, target={}, dispelSpell={}, count={}",
        bot->GetName(), packet.CasterGUID.ToString(), packet.TargetGUID.ToString(),
        packet.DispelledBySpellID, packet.DispellData.size());
}

/**
 * Attack Start - Typed Handler
 */
void ParseTypedAttackStart(WorldSession* session, WorldPackets::Combat::AttackStart const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    CombatEvent event = CombatEvent::AttackStart(
        packet.Attacker,
        packet.Victim
    );

    EventBus<CombatEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received ATTACK_START (typed): attacker={}, victim={}",
        bot->GetName(), packet.Attacker.ToString(), packet.Victim.ToString());
}

/**
 * Attack Stop - Typed Handler
 * WoW 12.0: Server packet is SAttackStop, not AttackStop (which is client packet)
 */
void ParseTypedAttackStop(WorldSession* session, WorldPackets::Combat::SAttackStop const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    // WoW 12.0: SAttackStop only has Attacker and Victim, no NowDead field
    CombatEvent event = CombatEvent::AttackStop(
        packet.Attacker,
        packet.Victim,
        false  // NowDead field doesn't exist in WoW 12.0
    );

    EventBus<CombatEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received ATTACK_STOP (typed): attacker={}, victim={}",
        bot->GetName(), packet.Attacker.ToString(), packet.Victim.ToString());
}

/**
 * AI Reaction - Typed Handler
 * NPC aggro changes
 */
void ParseTypedAIReaction(WorldSession* session, WorldPackets::Combat::AIReaction const& packet)
{
    if (!session)
        return;

    Player* bot = session->GetPlayer();
    if (!bot)
        return;

    CombatEvent event;
    event.type = CombatEventType::AI_REACTION;
    event.priority = CombatEventPriority::HIGH;
    event.casterGuid = packet.UnitGUID;
    event.targetGuid = ObjectGuid::Empty;
    event.victimGuid = ObjectGuid::Empty;
    event.spellId = 0;
    event.amount = static_cast<int32>(packet.Reaction);
    event.schoolMask = 0;
    event.flags = 0;
    event.timestamp = ::std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + ::std::chrono::milliseconds(5000);

    EventBus<CombatEvent>::instance()->PublishEvent(event);

    TC_LOG_DEBUG("playerbot.packets", "Bot {} received AI_REACTION (typed): unit={}, reaction={}",
        bot->GetName(), packet.UnitGUID.ToString(), static_cast<uint32>(packet.Reaction));
}

// ================================================================================================
// HANDLER REGISTRATION
// Called from PlayerbotPacketSniffer::Initialize()
// ================================================================================================

void RegisterCombatPacketHandlers()
{
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Spells::SpellStart>(&ParseTypedSpellStart);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Spells::SpellGo>(&ParseTypedSpellGo);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Spells::SpellFailure>(&ParseTypedSpellFailure);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Spells::SpellFailedOther>(&ParseTypedSpellFailedOther);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::CombatLog::SpellEnergizeLog>(&ParseTypedSpellEnergize);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::CombatLog::SpellInterruptLog>(&ParseTypedSpellInterrupt);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::CombatLog::SpellDispellLog>(&ParseTypedSpellDispel);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Combat::AttackStart>(&ParseTypedAttackStart);
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Combat::SAttackStop>(&ParseTypedAttackStop);  // WoW 12.0: SAttackStop is server packet
    PlayerbotPacketSniffer::RegisterTypedHandler<WorldPackets::Combat::AIReaction>(&ParseTypedAIReaction);

    TC_LOG_INFO("playerbot", "PlayerbotPacketSniffer: Registered {} Combat packet typed handlers", 10);
}

} // namespace Playerbot
