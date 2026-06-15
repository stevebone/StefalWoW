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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "SpellInfo.h"
#include "SpellAuras.h"

#include "Custom_Instance_Deadmines.h"

namespace Scripts::EasternKingdoms::Deadmines
{
    // 43778 - Foe Reaper 5000
    struct boss_foe_reaper_5000 : public BossAI
    {
        boss_foe_reaper_5000(Creature* creature) : BossAI(creature, DataTypes::BOSS_FOE_REAPER_5000), _summons(creature)
        {
            ApplyCrowdControlImmunities(creature);
            me->AddAura(Spells::Offline, me);
            me->SetUnitFlag(UnitFlags(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_STUNNED));
        }

        void Reset() override
        {
            BossAI::Reset();

            _events.Reset();
            _summons.DespawnAll();
            _safetyOfflineCast = false;
            _harvestActive = false;
            instance->SetData(Misc::FoeReaperAddCounter, 0); // Reset counter for starting encounter

            if (IsHeroic())
                me->SummonCreature(Creatures::PrototypeReaper, Positions::PrototypeSpawn, TEMPSUMMON_MANUAL_DESPAWN);

            me->SummonCreature(Creatures::DefiasWatcher, Positions::FoeReaper5000AddSpawns[0], TEMPSUMMON_MANUAL_DESPAWN);
            me->SummonCreature(Creatures::DefiasReaper, Positions::FoeReaper5000AddSpawns[1], TEMPSUMMON_MANUAL_DESPAWN);
            me->SummonCreature(Creatures::DefiasWatcher, Positions::FoeReaper5000AddSpawns[2], TEMPSUMMON_MANUAL_DESPAWN);
            me->SummonCreature(Creatures::DefiasReaper, Positions::FoeReaper5000AddSpawns[3], TEMPSUMMON_MANUAL_DESPAWN);
        }

        void JustSummoned(Creature* summon) override
        {
            _summons.Summon(summon);

            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
                summon->AI()->AttackStart(target);
        }

        void JustReachedHome() override
        {
            me->AddAura(Spells::Offline, me);
            me->RemoveAurasDueToSpell(Spells::RedEyes);
            me->SetUnitFlag(UnitFlags(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_STUNNED));
        }

        void DoAction(int32 action) override
        {
            if (action == 1)
            {
                _events.ScheduleEvent(Events::FoeReaperInit, 500ms);
                _events.ScheduleEvent(Events::FoeReaperAggro, 4s);
            }
        }

        void OnSpellCast(SpellInfo const* spellInfo) override
        {
            if (spellInfo && spellInfo->Id == Spells::Harvest)
                me->GetMotionMaster()->MovePoint(0, _harvestPos);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case Events::FoeReaperInit:
                    me->CastSpell(me, Spells::FoeEnergize, true);
                    me->RemoveAura(Spells::Offline);
                    me->AddAura(Spells::RedEyes, me);
                    me->RemoveUnitFlag(UnitFlags(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_STUNNED));
                    Talk(Texts::FoeReaperInit);
                    break;
                case Events::FoeReaperAggro:
                    Talk(Texts::FoeReaperAggro);
                    if (Unit* victim = SelectTarget(SelectTargetMethod::Random, 0, 150.f, false))
                        me->Attack(victim, false);
                    _events.ScheduleEvent(Events::FoeReaperReaperStrike, 8s);
                    if (IsHeroic())
                        _events.ScheduleEvent(Events::FoeReaperMoltenSlagWarning, 8s);
                    _events.ScheduleEvent(Events::FoeReaperOverdriveWarning, 4s);
                    _events.ScheduleEvent(Events::FoeReaperHarvest, 23s);
                    break;
                case Events::FoeReaperReaperStrike:
                    if (Unit* victim = me->GetVictim())
                        me->CastSpell(victim, Spells::ReaperStrike);
                    _events.RescheduleEvent(Events::FoeReaperReaperStrike, 5s, 7s);
                    break;
                case Events::FoeReaperMoltenSlagWarning:
                    Talk(Texts::FoeReaperMoltenSlag);
                    _events.ScheduleEvent(Events::FoeReaperMoltenSlag, 5s);
                    break;
                case Events::FoeReaperMoltenSlag:
                    if (IsHeroic())
                    {
                        uint32 randomIndex = urand(0, 3);
                        me->SummonCreature(Creatures::MoltenSlag, Positions::MoltenSlagSpawn[randomIndex], TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 30s);
                    }
                    _events.ScheduleEvent(Events::FoeReaperMoltenSlagWarning, 15s);
                    break;
                case Events::FoeReaperOverdriveWarning:
                {
                    // Re-using harvest warning for overdrive phase
                    if (instance)
                        instance->SetData(Misc::FoeReaper5000AOEWarning, true);
                    Talk(Texts::FoeReaperOverdriveWarning);
                    _events.ScheduleEvent(Events::FoeReaperOverdrive, 5s);
                    break;
                }
                case Events::FoeReaperOverdrive:
                    me->CastSpell(me, Spells::Overdrive, true);
                    Talk(Texts::FoeReaperOverdrive);
                    _events.ScheduleEvent(Events::FoeReaperOverdriveWarning, 25s);
                    _events.ScheduleEvent(Events::FoeReaperOverdriveSwitchTarget, 1500ms);
                    break;
                case Events::FoeReaperOverdriveSwitchTarget:
                    if (me->HasAura(Spells::Overdrive))
                    {
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 150.0f, false))
                            me->Attack(target, true);
                        _events.ScheduleEvent(Events::FoeReaperOverdriveSwitchTarget, 1500ms);
                    }
                    break;
                case Events::FoeReaperHarvest:
                    Talk(Texts::FoeReaperHarvest);
                    if (Unit* target = GetHarvestTarget())
                    {
                        _harvestActive = true;
                        _harvestPos = target->GetPosition();
                        me->SummonCreature(Creatures::HarvestTargetVisual, _harvestPos, TEMPSUMMON_TIMED_DESPAWN, 5s);
                        me->CastSpell(_harvestPos, Spells::Harvest);
                        if (instance)
                            instance->SetData(Misc::FoeReaper5000AOEWarning, true);
                    }
                    break;
                case Events::FoeReaperSafetyOfflineWarning:
                    Talk(Texts::FoeReaperSafetyOffWarning);
                    _events.ScheduleEvent(Events::FoeReaperSafetyOffline, 5s);
                    break;
                case Events::FoeReaperSafetyOffline:
                    me->CastSpell(me, Spells::SafetyOffline, true);
                    Talk(Texts::FoeReaperSafetyOff);
                    break;
                }
            }

            if (!UpdateVictim())
                return;

            // Add melee here if needed
        }

        void OnAuraApplied(AuraApplication const* aurApp) override
        {
            if (aurApp->GetBase()->GetId() == Spells::Overdrive)
                me->SetSpeedRate(MOVE_RUN, 2.0f);
        }

        void OnAuraRemoved(AuraApplication const* aurApp) override
        {
            if (aurApp->GetBase()->GetId() == Spells::Overdrive)
            {
                // Re-using the same warning for Overdrive
                if (instance)
                    instance->SetData(Misc::FoeReaper5000AOEWarning, false);
                me->SetSpeedRate(MOVE_RUN, 1.0f);
            }
        }

        void MovementInform(uint32 motionType, uint32 pointId) override
        {
            if (motionType == POINT_MOTION_TYPE && pointId == 0 && _harvestActive)
            {
                me->StopMoving();
                me->RemoveAurasDueToSpell(Spells::Harvest);
                me->CastSpell(me, Spells::HarvestSweep, false);
                Talk(Texts::FoeReaperHarvestSweep);
                _harvestActive = false;
                if (instance)
                    instance->SetData(Misc::FoeReaper5000AOEWarning, false);
                _events.ScheduleEvent(Events::FoeReaperHarvest, 30s);
            }
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->ToUnit() && victim->GetEntry() == Creatures::GeneralPurposeBunny)
                return;

            Talk(Texts::FoeReaperKill, victim);
        }

        void JustDied(Unit* killer) override
        {
            BossAI::JustDied(killer);
            _events.Reset();
            Talk(Texts::FoeReaperDeath);
            _summons.DespawnAll();
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (!_safetyOfflineCast && me->HealthBelowPctDamaged(25, damage))
            {
                _safetyOfflineCast = true;
                _events.ScheduleEvent(Events::FoeReaperSafetyOfflineWarning, 0s);
            }
        }

    private:
        bool _safetyOfflineCast = false;
        bool _harvestActive = false;
        EventMap _events;
        SummonList _summons;
        Position _harvestPos;

        Unit* GetHarvestTarget()
        {
            // 1) Try to select a target at least 10 yards away
            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, -10.0f, false))
            {
                if (me->GetDistance(target) >= 10.0f)
                    return target;
            }

            // 2) Fallback: find a specific NPC in the room
            if (Creature* fallback = me->FindNearestCreature(45979, 100.0f))
                return fallback;

            // 3) Last resort: return nullptr
            return nullptr;
        }
    };

    // 47403, 47404 - Defias Reaper & Watcher (Foe Reaper 5000 adds)
    struct npc_defias_foe_reaper_add : public ScriptedAI
    {
        npc_defias_foe_reaper_add(Creature* creature) : ScriptedAI(creature)
        {
            creature->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FIRE, true);
            me->RemoveNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);
        }

        void Reset() override
        {
            ScriptedAI::Reset();
            _events.Reset();
            _energizeCast = false;
            _onFireCast = false;
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            if (_onFireCast)
                return;
            ScriptedAI::EnterEvadeMode();
        }

        void IncrementFoeReaperAddCounter()
        {
            if (InstanceScript* instance = me->GetInstanceScript())
            {
                uint8 currentCount = instance->GetData(Misc::FoeReaperAddCounter);
                currentCount++;
                instance->SetData(Misc::FoeReaperAddCounter, currentCount);

                if (currentCount == 4)
                {
                    ObjectGuid bossGUID = instance->GetGuidData(DataTypes::BOSS_FOE_REAPER_5000);

                    if (Creature* boss = ObjectAccessor::GetCreature(*me, bossGUID))
                        boss->AI()->DoAction(1);
                }
            }
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (!_energizeCast && me->HealthBelowPctDamaged(60, damage))
            {
                _energizeCast = true;
                me->CastSpell(me, Spells::Energized, true);
            }

            if (!_onFireCast && me->HealthBelowPctDamaged(15, damage))
            {
                me->SetRegenerateHealth(false);
                damage = 0;
                me->SetHealth(me->GetMaxHealth() * 0.15);
                _onFireCast = true;
                IncrementFoeReaperAddCounter();
                me->CombatStop(true);
                me->ClearInCombat();
                me->RemoveAllAuras();
                me->CastSpell(me, Spells::OnFire, true);
                me->SetFaction(35);
                me->SetNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);

                if (Creature* foeReaper = me->FindNearestCreature(Creatures::FoeReaper5000, 100.0f))
                    me->CastSpell(foeReaper, Spells::Energize);

                _events.ScheduleEvent(Events::FoeReaperAddDeath, 30s);
            }
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.RescheduleEvent(Events::FoeReaperAddCleave, randtime(7s, 9s));
            _events.RescheduleEvent(Events::FoeReaperAddWatch, 4s);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case Events::FoeReaperAddCleave:
                {
                    if (!UpdateVictim())
                        return;

                    if (Unit* victim = me->GetVictim())
                        if (me->IsWithinDist(victim, 5.0f))
                            me->CastSpell(victim, Spells::Cleave, false);
                    _events.RescheduleEvent(Events::FoeReaperAddCleave, randtime(7s, 9s));
                    break;
                }
                case Events::FoeReaperAddWatch:
                {
                    if (!UpdateVictim())
                        return;

                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 0.0f, false))
                        me->CastSpell(target, Spells::Watch, false);
                    _events.RescheduleEvent(Events::FoeReaperAddWatch, randtime(12s, 14s));
                    break;
                }
                case Events::FoeReaperAddDeath:
                    me->KillSelf();
                    break;
                }
            }

            if (UpdateVictim())
                me->DoMeleeAttackIfReady();
        }

    private:
        bool _energizeCast = false;
        bool _onFireCast = false;
        EventMap _events;
    };
}

void AddSC_custom_deadmines_foe_reaper_5000()
{
    using namespace Scripts::EasternKingdoms::Deadmines;

    RegisterCreatureAI(boss_foe_reaper_5000);
    RegisterCreatureAI(npc_defias_foe_reaper_add);
}
