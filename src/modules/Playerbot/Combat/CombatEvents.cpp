/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CombatEvents.h"
#include <sstream>

namespace Playerbot
{

CombatEvent CombatEvent::SpellCastStart(ObjectGuid caster, ObjectGuid target, uint32 spellId, uint32 castTime)
{
    CombatEvent event;
    event.type = CombatEventType::SPELL_CAST_START;
    event.priority = CombatEventPriority::CRITICAL;
    event.casterGuid = caster;
    event.targetGuid = target;
    event.victimGuid = target;
    event.spellId = spellId;
    event.amount = static_cast<int32>(castTime);
    event.schoolMask = 0;
    event.flags = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(5000);
    return event;
}

CombatEvent CombatEvent::SpellCastGo(ObjectGuid caster, ObjectGuid target, uint32 spellId)
{
    CombatEvent event;
    event.type = CombatEventType::SPELL_CAST_GO;
    event.priority = CombatEventPriority::HIGH;
    event.casterGuid = caster;
    event.targetGuid = target;
    event.victimGuid = target;
    event.spellId = spellId;
    event.amount = 0;
    event.schoolMask = 0;
    event.flags = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(5000);
    return event;
}

CombatEvent CombatEvent::SpellDamage(ObjectGuid caster, ObjectGuid victim, uint32 spellId, int32 damage, uint32 school)
{
    CombatEvent event;
    event.type = CombatEventType::SPELL_DAMAGE_DEALT;
    event.priority = CombatEventPriority::HIGH;
    event.casterGuid = caster;
    event.targetGuid = victim;
    event.victimGuid = victim;
    event.spellId = spellId;
    event.amount = damage;
    event.schoolMask = school;
    event.flags = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(5000);
    return event;
}

CombatEvent CombatEvent::SpellHeal(ObjectGuid caster, ObjectGuid target, uint32 spellId, int32 heal)
{
    CombatEvent event;
    event.type = CombatEventType::SPELL_HEAL_DEALT;
    event.priority = CombatEventPriority::HIGH;
    event.casterGuid = caster;
    event.targetGuid = target;
    event.victimGuid = target;
    event.spellId = spellId;
    event.amount = heal;
    event.schoolMask = 0;
    event.flags = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(5000);
    return event;
}

CombatEvent CombatEvent::SpellInterrupt(ObjectGuid interrupter, ObjectGuid victim, uint32 interruptedSpell, uint32 interruptSpell)
{
    CombatEvent event;
    event.type = CombatEventType::SPELL_INTERRUPTED;
    event.priority = CombatEventPriority::CRITICAL;
    event.casterGuid = interrupter;
    event.targetGuid = victim;
    event.victimGuid = victim;
    event.spellId = interruptedSpell;
    event.amount = static_cast<int32>(interruptSpell);
    event.schoolMask = 0;
    event.flags = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(5000);
    return event;
}

CombatEvent CombatEvent::AttackStart(ObjectGuid attacker, ObjectGuid victim)
{
    CombatEvent event;
    event.type = CombatEventType::ATTACK_START;
    event.priority = CombatEventPriority::HIGH;
    event.casterGuid = attacker;
    event.targetGuid = victim;
    event.victimGuid = victim;
    event.spellId = 0;
    event.amount = 0;
    event.schoolMask = 0;
    event.flags = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(10000);
    return event;
}

CombatEvent CombatEvent::AttackStop(ObjectGuid attacker, ObjectGuid victim, bool nowDead)
{
    CombatEvent event;
    event.type = CombatEventType::ATTACK_STOP;
    event.priority = CombatEventPriority::HIGH;
    event.casterGuid = attacker;
    event.targetGuid = victim;
    event.victimGuid = victim;
    event.spellId = 0;
    event.amount = nowDead ? 1 : 0;
    event.schoolMask = 0;
    event.flags = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(5000);
    return event;
}

CombatEvent CombatEvent::ThreatUpdate(ObjectGuid unit, ObjectGuid victim, int32 threatChange)
{
    CombatEvent event;
    event.type = CombatEventType::THREAT_UPDATE;
    event.priority = CombatEventPriority::MEDIUM;
    event.casterGuid = unit;
    event.targetGuid = victim;
    event.victimGuid = victim;
    event.spellId = 0;
    event.amount = threatChange;
    event.schoolMask = 0;
    event.flags = 0;
    event.timestamp = std::chrono::steady_clock::now();
    event.expiryTime = event.timestamp + std::chrono::milliseconds(5000);
    return event;
}

bool CombatEvent::IsValid() const
{
    if (type >= CombatEventType::MAX_COMBAT_EVENT)
        return false;
    if (timestamp.time_since_epoch().count() == 0)
        return false;
    if (casterGuid.IsEmpty() && targetGuid.IsEmpty())
        return false;
    return true;
}

std::string CombatEvent::ToString() const
{
    std::ostringstream oss;
    oss << "[CombatEvent] Type: " << static_cast<uint32>(type)
        << ", Caster: " << casterGuid.ToString()
        << ", Target: " << targetGuid.ToString()
        << ", Spell: " << spellId
        << ", Amount: " << amount;
    return oss.str();
}

} // namespace Playerbot
