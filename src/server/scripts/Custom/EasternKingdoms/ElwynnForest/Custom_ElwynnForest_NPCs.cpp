/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Creature.h"
#include "CreatureAIImpl.h"
#include "MotionMaster.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "Unit.h"

#include "Custom_ElwynnForest_Defines.h"

// 49869 - Stormwind Infantry
class npc_custom_stormwind_infantry : public CreatureScript
{
public:
    npc_custom_stormwind_infantry() : CreatureScript("npc_custom_stormwind_infantry") { }

    struct npc_custom_stormwind_infantryAI : public ScriptedAI
    {
        npc_custom_stormwind_infantryAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 YellTimer = 0;
        uint32 EnemyScanTimer = 0;
        bool HelpCalled = false;

        void Reset() override
        {
            YellTimer = urand(30000, 45000);   // 30-45 sec
            EnemyScanTimer = urand(30000, 45000);   // 30-45 sec
            HelpCalled = false;
        }

        void JustEnteredCombat(Unit* /*who*/) override
        {
            // Nothing special here
        }

        void JustExitedCombat() override
        {
            HelpCalled = false;
        }

        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (!HelpCalled && me->HealthBelowPct(75))
            {
                HelpCalled = true;

                switch (urand(0, 9))
                {
                case 0: me->Say("HELP!", LANG_UNIVERSAL); break;
                case 1: me->Say("I could use a heal, brother!", LANG_UNIVERSAL); break;
                case 2: me->Say("I need a heal!", LANG_UNIVERSAL); break;
                case 3: me->Say("Make yourself useful and heal me, Paxton!", LANG_UNIVERSAL); break;
                case 4: me->Say("Paxton! I'm bleeding over here!", LANG_UNIVERSAL); break;
                case 5: me->Say("Light preserve me. I need aid!", LANG_UNIVERSAL); break;
                case 6: me->Say("Brother Paxton, don't let me fall!", LANG_UNIVERSAL); break;
                case 7: me->Say("A little help would be nice!", LANG_UNIVERSAL); break;
                case 8: me->Say("I'm not ready to die today!", LANG_UNIVERSAL); break;
                case 9: me->Say("Paxton! Patch me up before I drop!", LANG_UNIVERSAL); break;
                }
            }
        }

        // ----------------------------------------------------
        // Aura Hooks
        // ----------------------------------------------------
        void OnAuraApplied(AuraApplication const* aurApp) override
        {
            if (aurApp->GetBase()->GetId() == SPELL_PAXTON_FORTITUDE)
                ApplyFortitudeHPModifier(true);
        }

        void OnAuraRemoved(AuraApplication const* aurApp) override
        {
            if (aurApp->GetBase()->GetId() == SPELL_PAXTON_FORTITUDE)
                ApplyFortitudeHPModifier(false);
        }

        void UpdateAI(uint32 diff) override
        {
            Unit* victim = me->GetVictim();
            if(victim && victim->ToCreature() && victim->GetFaction() != 14)
                victim->SetFaction(14);

            // Yell every 30-45 seconds
            if (YellTimer <= diff)
            {
                // 30% chance to yell
                if (urand(1, 100) <= 10)
                {
                    switch (urand(0, 9))
                    {
                    case 0: me->Yell("Look alive, men! There are orcs and worgs about!", LANG_UNIVERSAL); break;
                    case 1: me->Yell("Your filthy dogs won't be enough!", LANG_UNIVERSAL); break;
                    case 2: me->Yell("We are here to protect the kingdom!", LANG_UNIVERSAL); break;
                    case 3: me->Yell("You puny blackrock are no match for us!", LANG_UNIVERSAL); break;
                    case 4: me->Yell("For the glory of Stormwind, stand your ground!", LANG_UNIVERSAL); break;
                    case 5: me->Yell("Drive them back, men! Show no fear!", LANG_UNIVERSAL); break;
                    case 6: me->Yell("The Light shields us - they cannot break our line!", LANG_UNIVERSAL); break;
                    case 7: me->Yell("Hold fast! Northshire will not fall today!", LANG_UNIVERSAL); break;
                    case 8: me->Yell("Steel yourselves! These beasts won't take another step!", LANG_UNIVERSAL); break;
                    case 9: me->Yell("Stand firm, brothers! Stormwind watches over us!", LANG_UNIVERSAL); break;
                    }
                }

                YellTimer = urand(30000, 45000);
            }
            else
                YellTimer -= diff;

            // Scan for enemies every 30 seconds
            if (EnemyScanTimer <= diff)
            {
                std::vector<uint32> enemyEntries = { NPC_NORTHSHIRE_ASSASSIN, NPC_NORTHSHIRE_SPY, NPC_NORTHSHIRE_WORG };

                for (uint32 entry : enemyEntries)
                {
                    if (Creature* enemy = me->FindNearestCreature(entry, 30.0f, true))
                    {
                        if (!me->IsInCombat())
                            AttackStart(enemy);
                        break;
                    }
                }

                EnemyScanTimer = urand(30000, 45000);
            }
            else
                EnemyScanTimer -= diff;

            // Standard melee AI
            if (!UpdateVictim())
                return;

            me->DoMeleeAttackIfReady();
        }

        // ----------------------------------------------------
        // Common method for applying/removing Fortitude scaling
        // ----------------------------------------------------
        void ApplyFortitudeHPModifier(bool apply)
        {
            if (apply)
            {
                uint64 OriginalMaxHP = me->GetMaxHealth();

                // Increase max HP by 10%
                float newMax = float(OriginalMaxHP) * 1.10f;
                me->SetMaxHealth(newMax);
            }
            else
            {
                uint64 currentMax = me->GetMaxHealth();
                uint32 reduction = currentMax * 0.1f;
                me->SetHealth(currentMax - reduction);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_custom_stormwind_infantryAI(creature);
    }
};

// 951 Brother Paxton
struct npc_custom_brother_paxton : public ScriptedAI
{
    npc_custom_brother_paxton(Creature* creature) : ScriptedAI(creature) { }

    uint32 HealScanTimer = 5000;
    uint32 BuffScanTimer = 300000;

    void Reset() override
    {
        HealScanTimer = 5000;
        BuffScanTimer = 300000;
    }

    void UpdateAI(uint32 diff) override
    {
        // Don't interrupt active casts
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        // -----------------------------------
        // HEAL LOGIC
        // -----------------------------------
        if (HealScanTimer <= diff)
        {
            Creature* target = nullptr;

            std::list<Creature*> allies;
            me->GetCreatureListWithEntryInGrid(allies, NPC_NORTHSHIRE_STORMWIND_INFANTRY, 100.0f);

            for (Creature* ally : allies)
            {
                if (ally->IsAlive() && ally->HealthBelowPct(75))
                {
                    target = ally;
                    break;
                }
            }

            if (target)
            {
                float dist = me->GetDistance(target);

                if (dist > 30.0f)
                {
                    if (!me->isMoving())
                    {
                        // Desired distance to stop from target
                        float stopDist = 20.0f;

                        // Direction vector from Paxton to target
                        float angle = me->GetRelativeAngle(target->GetPosition());

                        // Compute a point stopDist away from the target
                        float x = target->GetPositionX() + cos(angle) * stopDist;
                        float y = target->GetPositionY() + sin(angle) * stopDist;
                        float z = target->GetPositionZ();

                        me->GetMotionMaster()->MovePoint(1, x, y, z);
                    }
                }
                else
                {
                    uint32 spellId = RAND(
                        SPELL_PAXTON_FLASH_HEAL,
                        SPELL_PAXTON_PRAYER_OF_HEALING,
                        SPELL_PAXTON_RENEW
                    );

                    // Say a line when healing
                    if (urand(1, 100) <= 50)
                    {
                        switch (urand(0, 4))
                        {
                        case 0: me->Say("AND I LAY MY HANDS UPON YOU!", LANG_UNIVERSAL); break;
                        case 1: me->Say("BY THE LIGHT BE RENEWED!", LANG_UNIVERSAL); break;
                        case 2: me->Say("Be healed, brother!", LANG_UNIVERSAL); break;
                        case 3: me->Say("FIGHT ON, BROTHER!", LANG_UNIVERSAL); break;
                        case 4: me->Say("Let the Holy Light embrace you!", LANG_UNIVERSAL); break;
                        }
                    }

                    me->CastSpell(target, spellId, false);
                }
            }

            HealScanTimer = 2000;
        }
        else
            HealScanTimer -= diff;

        // -----------------------------------
        // BUFF LOGIC (Fortitude)
        // -----------------------------------
        if (BuffScanTimer <= diff)
        {
            std::list<Creature*> allies;
            me->GetCreatureListWithEntryInGrid(allies, NPC_NORTHSHIRE_STORMWIND_INFANTRY, 100.0f);

            for (Creature* ally : allies)
            {
                if (!ally->HasAura(SPELL_PAXTON_FORTITUDE))
                {
                    float dist = me->GetDistance(ally);

                    if (dist > 30.0f)
                    {
                        if (!me->isMoving())
                        {
                            // Desired distance to stop from target
                            float stopDist = 20.0f;

                            // Direction vector from Paxton to target
                            float angle = me->GetRelativeAngle(ally->GetPosition());

                            // Compute a point stopDist away from the target
                            float x = ally->GetPositionX() + cos(angle) * stopDist;
                            float y = ally->GetPositionY() + sin(angle) * stopDist;
                            float z = ally->GetPositionZ();

                            me->GetMotionMaster()->MovePoint(1, x, y, z);
                        }
                    }
                    else
                    {
                        me->CastSpell(ally, SPELL_PAXTON_FORTITUDE, false);
                    }

                    break; // Only buff ONE guard per cycle
                }
            }

            BuffScanTimer = 300000;
        }
        else
            BuffScanTimer -= diff;
    }
};

void AddSC_custom_elwynn_forest_npcs()
{
    new npc_custom_stormwind_infantry();
    RegisterCreatureAI(npc_custom_brother_paxton);
}
