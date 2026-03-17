/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 */

#include "CombatEvent.h"
#include "GameTime.h"
#include "SpellInfo.h"
#include "SpellAuras.h"

namespace Playerbot {

CombatEvent CombatEvent::CreateDamageTaken(ObjectGuid victim, ObjectGuid attacker,
                                            uint32 damage, uint32 overkill,
                                            const SpellInfo* spell) {
    CombatEvent event;
    event.type = CombatEventType::DAMAGE_TAKEN;
    event.timestamp = GameTime::GetGameTimeMS();
    event.target = victim;
    event.source = attacker;
    event.amount = damage;
    event.overkill = overkill;
    event.spellInfo = spell;
    event.spellId = spell ? spell->Id : 0;
    return event;
}

CombatEvent CombatEvent::CreateDamageDealt(ObjectGuid attacker, ObjectGuid victim,
                                            uint32 damage, const SpellInfo* spell) {
    CombatEvent event;
    event.type = CombatEventType::DAMAGE_DEALT;
    event.timestamp = GameTime::GetGameTimeMS();
    event.source = attacker;
    event.target = victim;
    event.amount = damage;
    event.spellInfo = spell;
    event.spellId = spell ? spell->Id : 0;
    return event;
}

CombatEvent CombatEvent::CreateHealingDone(ObjectGuid healer, ObjectGuid target,
                                            uint32 healing, uint32 overheal,
                                            const SpellInfo* spell) {
    CombatEvent event;
    event.type = CombatEventType::HEALING_DONE;
    event.timestamp = GameTime::GetGameTimeMS();
    event.source = healer;
    event.target = target;
    event.amount = healing;
    event.overkill = overheal;  // Using overkill field for overheal
    event.spellInfo = spell;
    event.spellId = spell ? spell->Id : 0;
    return event;
}

CombatEvent CombatEvent::CreateSpellCastStart(ObjectGuid caster, const SpellInfo* spell,
                                               ObjectGuid target) {
    CombatEvent event;
    event.type = CombatEventType::SPELL_CAST_START;
    event.timestamp = GameTime::GetGameTimeMS();
    event.source = caster;
    event.target = target;
    event.spellInfo = spell;
    event.spellId = spell ? spell->Id : 0;
    return event;
}

CombatEvent CombatEvent::CreateSpellCastSuccess(ObjectGuid caster, const SpellInfo* spell) {
    CombatEvent event;
    event.type = CombatEventType::SPELL_CAST_SUCCESS;
    event.timestamp = GameTime::GetGameTimeMS();
    event.source = caster;
    event.spellInfo = spell;
    event.spellId = spell ? spell->Id : 0;
    return event;
}

CombatEvent CombatEvent::CreateSpellInterrupted(ObjectGuid caster, const SpellInfo* spell,
                                                 ObjectGuid interrupter) {
    CombatEvent event;
    event.type = CombatEventType::SPELL_INTERRUPTED;
    event.timestamp = GameTime::GetGameTimeMS();
    event.source = interrupter;
    event.target = caster;  // Target is the caster whose spell was interrupted
    event.spellInfo = spell;
    event.spellId = spell ? spell->Id : 0;
    return event;
}

CombatEvent CombatEvent::CreateAuraApplied(ObjectGuid target, const Aura* aura,
                                            ObjectGuid caster) {
    CombatEvent event;
    event.type = CombatEventType::AURA_APPLIED;
    event.timestamp = GameTime::GetGameTimeMS();
    event.target = target;
    event.source = caster;
    event.aura = aura;
    if (aura) {
        event.spellId = aura->GetId();
        event.auraStacks = aura->GetStackAmount();
        event.auraDuration = aura->GetDuration();
    }
    return event;
}

CombatEvent CombatEvent::CreateAuraRemoved(ObjectGuid target, const Aura* aura) {
    CombatEvent event;
    event.type = CombatEventType::AURA_REMOVED;
    event.timestamp = GameTime::GetGameTimeMS();
    event.target = target;
    event.aura = aura;
    if (aura) {
        event.spellId = aura->GetId();
        event.source = aura->GetCasterGUID();
    }
    return event;
}

CombatEvent CombatEvent::CreateThreatChanged(ObjectGuid unit, ObjectGuid target,
                                              float oldThreat, float newThreat) {
    CombatEvent event;
    event.type = CombatEventType::THREAT_CHANGED;
    event.timestamp = GameTime::GetGameTimeMS();
    event.source = unit;
    event.target = target;
    event.oldThreat = oldThreat;
    event.newThreat = newThreat;
    event.threatDelta = newThreat - oldThreat;
    return event;
}

CombatEvent CombatEvent::CreateUnitDied(ObjectGuid unit, ObjectGuid killer) {
    CombatEvent event;
    event.type = CombatEventType::UNIT_DIED;
    event.timestamp = GameTime::GetGameTimeMS();
    event.target = unit;
    event.source = killer;
    return event;
}

CombatEvent CombatEvent::CreateCombatStarted(ObjectGuid unit) {
    CombatEvent event;
    event.type = CombatEventType::COMBAT_STARTED;
    event.timestamp = GameTime::GetGameTimeMS();
    event.source = unit;
    return event;
}

CombatEvent CombatEvent::CreateCombatEnded(ObjectGuid unit) {
    CombatEvent event;
    event.type = CombatEventType::COMBAT_ENDED;
    event.timestamp = GameTime::GetGameTimeMS();
    event.source = unit;
    return event;
}

CombatEvent CombatEvent::CreateEncounterStart(uint32 encounterId) {
    CombatEvent event;
    event.type = CombatEventType::ENCOUNTER_START;
    event.timestamp = GameTime::GetGameTimeMS();
    event.encounterId = encounterId;
    return event;
}

CombatEvent CombatEvent::CreateEncounterEnd(uint32 encounterId) {
    CombatEvent event;
    event.type = CombatEventType::ENCOUNTER_END;
    event.timestamp = GameTime::GetGameTimeMS();
    event.encounterId = encounterId;
    return event;
}

CombatEvent CombatEvent::CreateBossPhaseChanged(uint32 encounterId, uint8 phase) {
    CombatEvent event;
    event.type = CombatEventType::BOSS_PHASE_CHANGED;
    event.timestamp = GameTime::GetGameTimeMS();
    event.encounterId = encounterId;
    event.encounterPhase = phase;
    return event;
}

} // namespace Playerbot
