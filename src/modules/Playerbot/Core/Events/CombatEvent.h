/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 */

#ifndef TRINITYCORE_COMBAT_EVENT_H
#define TRINITYCORE_COMBAT_EVENT_H

#include "CombatEventType.h"
#include "Define.h"
#include "ObjectGuid.h"
#include <cstdint>

class SpellInfo;
class Aura;
class Unit;

namespace Playerbot {

/**
 * @struct CombatEvent
 * @brief Combat event data structure for event-driven combat system
 *
 * Phase 3 Architecture: Carries all data needed for combat event handling
 *
 * Design:
 * - Single struct for all event types (no inheritance overhead)
 * - Factory methods for type-safe construction
 * - Lightweight for high-frequency events
 *
 * Memory layout optimized for cache efficiency:
 * - Hot data (type, guids) at start
 * - Cold data (encounter info) at end
 */
struct CombatEvent {
    // ====================================================================
    // EVENT IDENTIFICATION (Hot path - always accessed)
    // ====================================================================

    CombatEventType type = CombatEventType::NONE;
    uint32 timestamp = 0;

    // ====================================================================
    // PARTICIPANTS (Hot path - usually accessed)
    // ====================================================================

    ObjectGuid source;      // Who caused the event (attacker, healer, caster)
    ObjectGuid target;      // Who was affected (victim, heal target, spell target)

    // ====================================================================
    // DAMAGE/HEALING DATA
    // ====================================================================

    uint32 amount = 0;      // Damage dealt or healing done
    uint32 overkill = 0;    // Overkill damage or overheal amount
    uint32 absorbed = 0;    // Absorbed amount
    uint32 resisted = 0;    // Resisted amount
    bool isCritical = false;

    // ====================================================================
    // SPELL DATA
    // ====================================================================

    uint32 spellId = 0;
    const SpellInfo* spellInfo = nullptr;

    // ====================================================================
    // AURA DATA
    // ====================================================================

    const Aura* aura = nullptr;
    uint8 auraStacks = 0;
    uint32 auraDuration = 0;

    // ====================================================================
    // THREAT DATA
    // ====================================================================

    float oldThreat = 0.0f;
    float newThreat = 0.0f;
    float threatDelta = 0.0f;

    // ====================================================================
    // ENCOUNTER DATA (Cold path - rarely accessed)
    // ====================================================================

    uint32 encounterId = 0;
    uint8 encounterPhase = 0;

    // ====================================================================
    // UTILITY METHODS
    // ====================================================================

    /**
     * @brief Check if event is valid
     */
    [[nodiscard]] bool IsValid() const { return type != CombatEventType::NONE; }

    /**
     * @brief Check if event is a damage event
     */
    [[nodiscard]] bool IsDamageEvent() const { return HasFlag(type, CombatEventType::ALL_DAMAGE); }

    /**
     * @brief Check if event is a healing event
     */
    [[nodiscard]] bool IsHealingEvent() const { return HasFlag(type, CombatEventType::ALL_HEALING); }

    /**
     * @brief Check if event is a spell event
     */
    [[nodiscard]] bool IsSpellEvent() const { return HasFlag(type, CombatEventType::ALL_SPELL); }

    /**
     * @brief Check if event is a threat event
     */
    [[nodiscard]] bool IsThreatEvent() const { return HasFlag(type, CombatEventType::ALL_THREAT); }

    /**
     * @brief Check if event is an aura event
     */
    [[nodiscard]] bool IsAuraEvent() const { return HasFlag(type, CombatEventType::ALL_AURA); }

    /**
     * @brief Check if event is a combat state event
     */
    [[nodiscard]] bool IsCombatStateEvent() const { return HasFlag(type, CombatEventType::ALL_COMBAT_STATE); }

    /**
     * @brief Check if event is a unit event
     */
    [[nodiscard]] bool IsUnitEvent() const { return HasFlag(type, CombatEventType::ALL_UNIT); }

    /**
     * @brief Check if event is an encounter event
     */
    [[nodiscard]] bool IsEncounterEvent() const { return HasFlag(type, CombatEventType::ALL_ENCOUNTER); }

    // ====================================================================
    // FACTORY METHODS - Type-safe event construction
    // ====================================================================

    /**
     * @brief Create DAMAGE_TAKEN event
     *
     * @param victim Who took damage
     * @param attacker Who dealt damage
     * @param damage Amount of damage
     * @param overkill Overkill amount (0 if not lethal)
     * @param spell Spell that dealt damage (nullptr for melee)
     */
    static CombatEvent CreateDamageTaken(ObjectGuid victim, ObjectGuid attacker,
                                          uint32 damage, uint32 overkill = 0,
                                          const SpellInfo* spell = nullptr);

    /**
     * @brief Create DAMAGE_DEALT event
     *
     * @param attacker Who dealt damage
     * @param victim Who took damage
     * @param damage Amount of damage
     * @param spell Spell that dealt damage (nullptr for melee)
     */
    static CombatEvent CreateDamageDealt(ObjectGuid attacker, ObjectGuid victim,
                                          uint32 damage, const SpellInfo* spell = nullptr);

    /**
     * @brief Create HEALING_DONE event
     *
     * @param healer Who healed
     * @param target Who was healed
     * @param healing Amount healed
     * @param overheal Overheal amount
     * @param spell Spell used
     */
    static CombatEvent CreateHealingDone(ObjectGuid healer, ObjectGuid target,
                                          uint32 healing, uint32 overheal = 0,
                                          const SpellInfo* spell = nullptr);

    /**
     * @brief Create SPELL_CAST_START event
     *
     * @param caster Who is casting
     * @param spell Spell being cast
     * @param target Target of spell (Empty for no target)
     *
     * IMPORTANT: This event type should use Dispatch() not QueueEvent()
     * for immediate interrupt response
     */
    static CombatEvent CreateSpellCastStart(ObjectGuid caster, const SpellInfo* spell,
                                             ObjectGuid target = ObjectGuid::Empty);

    /**
     * @brief Create SPELL_CAST_SUCCESS event
     *
     * @param caster Who cast the spell
     * @param spell Spell that was cast
     */
    static CombatEvent CreateSpellCastSuccess(ObjectGuid caster, const SpellInfo* spell);

    /**
     * @brief Create SPELL_INTERRUPTED event
     *
     * @param caster Whose spell was interrupted
     * @param spell Spell that was interrupted
     * @param interrupter Who interrupted
     */
    static CombatEvent CreateSpellInterrupted(ObjectGuid caster, const SpellInfo* spell,
                                               ObjectGuid interrupter);

    /**
     * @brief Create AURA_APPLIED event
     *
     * @param target Who received the aura
     * @param aura The aura that was applied
     * @param caster Who cast the aura
     */
    static CombatEvent CreateAuraApplied(ObjectGuid target, const Aura* aura,
                                          ObjectGuid caster);

    /**
     * @brief Create AURA_REMOVED event
     *
     * @param target Who lost the aura
     * @param aura The aura that was removed
     */
    static CombatEvent CreateAuraRemoved(ObjectGuid target, const Aura* aura);

    /**
     * @brief Create THREAT_CHANGED event
     *
     * @param unit Unit whose threat changed
     * @param target Target of threat
     * @param oldThreat Previous threat value
     * @param newThreat New threat value
     */
    static CombatEvent CreateThreatChanged(ObjectGuid unit, ObjectGuid target,
                                            float oldThreat, float newThreat);

    /**
     * @brief Create UNIT_DIED event
     *
     * @param unit Unit that died
     * @param killer Who killed (Empty if environmental)
     */
    static CombatEvent CreateUnitDied(ObjectGuid unit, ObjectGuid killer = ObjectGuid::Empty);

    /**
     * @brief Create COMBAT_STARTED event
     *
     * @param unit Unit entering combat
     */
    static CombatEvent CreateCombatStarted(ObjectGuid unit);

    /**
     * @brief Create COMBAT_ENDED event
     *
     * @param unit Unit leaving combat
     */
    static CombatEvent CreateCombatEnded(ObjectGuid unit);

    /**
     * @brief Create ENCOUNTER_START event
     *
     * @param encounterId Encounter ID
     */
    static CombatEvent CreateEncounterStart(uint32 encounterId);

    /**
     * @brief Create ENCOUNTER_END event
     *
     * @param encounterId Encounter ID
     */
    static CombatEvent CreateEncounterEnd(uint32 encounterId);

    /**
     * @brief Create BOSS_PHASE_CHANGED event
     *
     * @param encounterId Encounter ID
     * @param phase New phase number
     */
    static CombatEvent CreateBossPhaseChanged(uint32 encounterId, uint8 phase);
};

} // namespace Playerbot

#endif // TRINITYCORE_COMBAT_EVENT_H
