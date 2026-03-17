/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Playerbot Spell Fixes - Override buggy TrinityCore spell scripts with null-safe versions
 *
 * These overrides fix crashes caused by race conditions where units are removed
 * from ObjectAccessor while AreaTriggers are iterating over their inside units list.
 *
 * IMPORTANT: To use these fixes, apply the SQL patch:
 *   sql/playerbot/fixes/01_fix_binding_shot_crash.sql
 *
 * This changes the database to point to our fixed scripts instead of TrinityCore's.
 */

#include "AreaTrigger.h"
#include "AreaTriggerAI.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "TaskScheduler.h"
#include "Unit.h"

// Spell IDs for Hunter Binding Shot
enum BindingShotSpells
{
    SPELL_HUNTER_BINDING_SHOT           = 109248,
    SPELL_HUNTER_BINDING_SHOT_MARKER    = 117405,
    SPELL_HUNTER_BINDING_SHOT_STUN      = 117526,
    SPELL_HUNTER_BINDING_SHOT_IMMUNE    = 117553,
    SPELL_HUNTER_BINDING_SHOT_VISUAL    = 118306,
};

/*
 * FIX: Hunter Binding Shot AreaTrigger (spell_hunter.cpp at_hun_binding_shot)
 *
 * Original bug: ObjectAccessor::GetUnit() can return nullptr if unit was removed,
 * but the code immediately calls unit->HasAura() without null check, causing crash.
 *
 * This override adds proper null checks to prevent ACCESS_VIOLATION.
 *
 * Database must have ScriptName = 'at_hun_binding_shot_playerbot' for this to work.
 * Apply sql/playerbot/fixes/01_fix_binding_shot_crash.sql
 */
struct at_hun_binding_shot_playerbot : AreaTriggerAI
{
    at_hun_binding_shot_playerbot(AreaTrigger* at) : AreaTriggerAI(at) { }

    void OnInitialize() override
    {
        if (Unit* caster = at->GetCaster())
            for (AreaTrigger* other : caster->GetAreaTriggers(SPELL_HUNTER_BINDING_SHOT))
                other->SetDuration(0);
    }

    void OnCreate(Spell const* /*creatingSpell*/) override
    {
        _scheduler.Schedule(1s, [this](TaskContext task)
        {
            for (ObjectGuid const& guid : at->GetInsideUnits())
            {
                Unit* unit = ObjectAccessor::GetUnit(*at, guid);
                // FIX: Add null check to prevent crash when unit is removed during iteration
                if (!unit || !unit->HasAura(SPELL_HUNTER_BINDING_SHOT_MARKER))
                    continue;

                unit->CastSpell(at->GetPosition(), SPELL_HUNTER_BINDING_SHOT_VISUAL, TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
            }

            task.Repeat(1s);
        });
    }

    void OnUnitEnter(Unit* unit) override
    {
        if (Unit* caster = at->GetCaster())
        {
            // FIX: Add null check for unit (should always be valid here, but be defensive)
            if (unit && caster->IsValidAttackTarget(unit) && !unit->HasAura(SPELL_HUNTER_BINDING_SHOT_IMMUNE, caster->GetGUID()))
            {
                caster->CastSpell(unit, SPELL_HUNTER_BINDING_SHOT_MARKER, TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
                unit->CastSpell(at->GetPosition(), SPELL_HUNTER_BINDING_SHOT_VISUAL, TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
            }
        }
    }

    void OnUnitExit(Unit* unit, AreaTriggerExitReason /*reason*/) override
    {
        // FIX: Add null check for unit
        if (!unit)
            return;

        unit->RemoveAurasDueToSpell(SPELL_HUNTER_BINDING_SHOT_MARKER, at->GetCasterGuid());

        if (at->IsRemoved())
            return;

        if (Unit* caster = at->GetCaster())
        {
            if (caster->IsValidAttackTarget(unit) && !unit->HasAura(SPELL_HUNTER_BINDING_SHOT_IMMUNE, caster->GetGUID()))
            {
                caster->CastSpell(unit, SPELL_HUNTER_BINDING_SHOT_STUN, TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
                caster->CastSpell(unit, SPELL_HUNTER_BINDING_SHOT_IMMUNE, TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
            }
        }
    }

    void OnUpdate(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
};

// Register our fixed version with a unique name
// Database must be patched to use this name (see sql/playerbot/fixes/)
void AddSC_playerbot_spell_fixes()
{
    RegisterAreaTriggerAI(at_hun_binding_shot_playerbot);
}
