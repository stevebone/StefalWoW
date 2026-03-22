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
#include "CreatureAI.h"
#include "CreatureAIImpl.h"
#include "EventMap.h"
#include "Map.h"
#include "MotionMaster.h"
#include "Player.h"
#include "Position.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "TemporarySummon.h"
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

struct npc_custom_eastvale_lumberjack : public ScriptedAI
{
    npc_custom_eastvale_lumberjack(Creature* creature) : ScriptedAI(creature) { }

    Position HomePos;
    EventMap events;
    uint32 currentSpot = 0;

    void Reset() override
    {
        HomePos = me->GetHomePosition();
        me->SetWalk(true);
        events.ScheduleEvent(EVENT_GO_TO_WORK_SPOT, std::chrono::milliseconds(urand(3000, 8000)));
    }

    void JustEnteredCombat(Unit* who) override
    {
        events.Reset();
        me->GetMotionMaster()->Clear();
        me->HandleEmoteCommand(EMOTE_STATE_NONE);

        Creature* attacker = who->ToCreature();
        Creature* marshall = me->FindNearestCreature(NPC_EASTVALE_MARSHAL_PETTERSON, 200.f);

        if (attacker && attacker->GetEntry() == NPC_EASTVALE_PROWLER)
        {
            uint32 idx = urand(0, uint32(EastvalePanicLines.size() - 1));
            me->Yell(EastvalePanicLines[idx], LANG_UNIVERSAL);
            me->SetWalk(false);
            if (marshall)
            {
                Position pos = marshall->GetPosition();
                me->GetMotionMaster()->MovePoint(4, pos.GetPositionX() + frand(-2.f, 2.f), pos.GetPositionY() + frand(-2.f, 2.f), pos.GetPositionZ());
            }
        }
    }

    void JustExitedCombat() override
    {
        me->HandleEmoteCommand(EMOTE_STATE_NONE);
        me->GetMotionMaster()->Clear();
        me->SetWalk(true);
        me->GetMotionMaster()->MoveTargetedHome();

        // Restart routine after a short delay
        events.ScheduleEvent(EVENT_IDLE, 5s);
    }

    void JustDied(Unit* /*killer*/) override // Runs once when creature dies
    {
        events.Reset();
    }

    void MovementInform(uint32 /*type*/, uint32 id) override
    {
        switch (id)
        {
        case 1:
            events.ScheduleEvent(EVENT_CHOP_WOOD, 0s);
            break;
        case 2:
            events.ScheduleEvent(EVENT_IDLE, 4s);
            break;

        case 3:
        {
            int8 outcome = urand(0, 1);
            std::string request = "Hey boss, can we have a break please?";
            me->Say(request, LANG_UNIVERSAL);

            switch (outcome)
            {
            case 0:
                events.ScheduleEvent(EVENT_BREAK_NOT_ALLOWED, 3s, 5s);
                break;
            case 1:
                events.ScheduleEvent(EVENT_BREAK_ALLOWED, 3s, 5s);
                break;
            default:
                break;
            }
            break;
        }

        case 4:
        {
            if (!me->getAttackers().empty())
            {
                Unit* victim = *me->getAttackers().begin();
                if (victim)
                {
                    me->AI()->AttackStart(victim);

                    if (Creature* marshall = me->FindNearestCreature(NPC_EASTVALE_MARSHAL_PETTERSON, 30.f))
                        marshall->AI()->AttackStart(victim);
                }
            }
            break;
        }
        default:
            break;
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->isDead())
            return;

        // Threat detection: wolves nearby
        if (!me->IsInCombat())
        {
            Creature* wolf = me->FindNearestCreature(NPC_EASTVALE_PROWLER, 10.f);
            if (wolf && !wolf->IsInCombat())
            {
                // Force wolf to attack this worker
                wolf->AI()->AttackStart(me);

                return;
            }
        }

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_GO_TO_WORK_SPOT:
            {
                currentSpot = urand(0, EastvaleWorkSpots.size() - 1);
                me->GetMotionMaster()->MovePoint(1, EastvaleWorkSpots[currentSpot]);
                break;
            }

            case EVENT_CHOP_WOOD:
            {
                me->SetFacingTo(EastvaleWorkSpots[currentSpot].GetOrientation());
                me->HandleEmoteCommand(EMOTE_STATE_WORK_CHOPWOOD);

                // Say something
                if (!EastvaleLumberjackWorkLines.empty())
                {
                    uint32 idx = urand(0, uint32(EastvaleLumberjackWorkLines.size() - 1));
                    me->Say(EastvaleLumberjackWorkLines[idx], LANG_UNIVERSAL);
                }

                events.ScheduleEvent(EVENT_RETURN_HOME, std::chrono::milliseconds(urand(28000, 45000)));
                break;
            }

            case EVENT_RETURN_HOME:
            {
                me->HandleEmoteCommand(EMOTE_STATE_NONE);
                me->GetMotionMaster()->MovePoint(2, HomePos);
                break;
            }

            case EVENT_IDLE:
            {
                uint8 event = RAND(EVENT_GO_TO_WORK_SPOT, EVENT_GO_TO_SUPERVISOR, EVENT_GO_TO_WORK_SPOT);

                events.ScheduleEvent(event, std::chrono::milliseconds(urand(30000, 45000)));
                break;
            }

            case EVENT_GO_TO_SUPERVISOR:
            {
                uint32 supervisorEntry = 0;
                if (me->GetEntry() == NPC_EASTVALE_PEASANT)
                    supervisorEntry = NPC_EASTVALE_RAELEN;
                else supervisorEntry = NPC_EASTVALE_PERRY;

                Creature* supervisor = me->FindNearestCreature(supervisorEntry, 30.f);
                if (supervisor)
                {
                    me->GetMotionMaster()->MoveCloserAndStop(3, supervisor, frand(3.f, 5.f));
                }
                else
                {
                    events.ScheduleEvent(EVENT_GO_TO_WORK_SPOT, 2s);
                }
                break;
            }

            case EVENT_BREAK_NOT_ALLOWED:
            {
                uint32 supervisorEntry = 0;
                if (me->GetEntry() == NPC_EASTVALE_PEASANT)
                    supervisorEntry = NPC_EASTVALE_RAELEN;
                else supervisorEntry = NPC_EASTVALE_PERRY;

                Creature* supervisor = me->FindNearestCreature(supervisorEntry, 10.f);
                if (supervisor)
                {
                    if(supervisorEntry == NPC_EASTVALE_RAELEN)
                        supervisor->Say("We need to get this wagon filled by the end of the day. So back to work with you. Chop, chop!", LANG_UNIVERSAL);
                    else supervisor->Say("The big boss is here and we cannot be seen slacking!", LANG_UNIVERSAL);
                    events.ScheduleEvent(EVENT_GO_TO_WORK_SPOT, 3s, 5s);
                }
                break;
            }

            case EVENT_BREAK_ALLOWED:
            {
                uint32 supervisorEntry = 0;
                if (me->GetEntry() == NPC_EASTVALE_PEASANT)
                    supervisorEntry = NPC_EASTVALE_RAELEN;
                else supervisorEntry = NPC_EASTVALE_PERRY;

                Creature* supervisor = me->FindNearestCreature(supervisorEntry, 10.f);
                if (supervisor)
                {
                    if (supervisorEntry == NPC_EASTVALE_RAELEN)
                        supervisor->Say("This better be the last one today!", LANG_UNIVERSAL);
                    else supervisor->Say("Just a few moments, okay?", LANG_UNIVERSAL);
                    events.ScheduleEvent(EVENT_RETURN_HOME, 3s, 5s);
                }
                break;
            }
            }
        }
    }
};

struct npc_custom_hogger : public ScriptedAI
{
    npc_custom_hogger(Creature* creature) : ScriptedAI(creature) { }

    bool _minionsSummoned = false;
    bool _endingSceneActive = false;
    bool _alreadyEaten = false;
    bool _isEating = false;

    void Reset() override
    {
        _events.Reset();

        _minionsSummoned = false;
        _endingSceneActive = false;
        _alreadyEaten = false;
        _isEating = false;

        me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
        me->SetReactState(REACT_AGGRESSIVE);
        me->SetWalk(false);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (_endingSceneActive)
        {
            damage = 0;
            return;
        }

        if (!_minionsSummoned && me->HealthBelowPctDamaged(50, damage))
            SummonMinions();

        if (!_alreadyEaten && me->HealthBelowPctDamaged(30, damage))
            MoveToEatingPosition();


        if (me->GetHealth() <= damage)
        {
            damage = 0;
            me->SetHealth(1);
            StartEndingScene();
            RewardPlayers();
        }

        if (_isEating)
        {
            _isEating = false;
            me->SetReactState(REACT_AGGRESSIVE);
            Talk(SAY_STUNNED);
        }
    }

    void JustEnteredCombat(Unit* /*who*/) override
    {
        Talk(SAY_HOGGER_AGGRO);
        _events.ScheduleEvent(EVENT_VICIOUS_SLICE, 3s);
    }

    void MoveToEatingPosition()
    {
        _alreadyEaten = true;

        if (Creature* target = me->FindNearestCreature(NPC_EATING_TARGET, 100.0f))
        {
            me->SetReactState(REACT_PASSIVE);
            me->GetMotionMaster()->MovePoint(0, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), true);
            _events.ScheduleEvent(EVENT_CHECK_EAT_RANGE, 200ms);
        }
    }

    void StartEndingScene()
    {
        _endingSceneActive = true;
        _events.Reset();

        me->SetReactState(REACT_PASSIVE);
        me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
        me->StopMoving();
        me->AttackStop();

        Talk(SAY_BEG);

        SummonGeneralHammondClay();
        SummonAndromath();
        SummonDumas();

        _events.ScheduleEvent(EVENT_HAMMOND_GROUP_START_WALKING, 1s);
    }

    void RewardPlayers()
    {
        for (auto const& ref : me->GetThreatManager().GetUnsortedThreatList())
        {
            if (!ref->IsOnline())
                continue;

            if (Unit* unit = ref->GetVictim())
            {
                if (Player* player = unit->ToPlayer())
                    player->RewardPlayerAndGroupAtEvent(NPC_HOGGER, me);
            }
        }
    }

    void SummonGeneralHammondClay()
    {
        if (TempSummon* hammond = me->SummonCreature(NPC_GENERAL_HAMMOND_CLAY, generalHammondClayPositions[0]))
        {
            _generalHammondGUID = hammond->GetGUID();
            hammond->CastSpell(hammond, SPELL_TELEPORT_VISUAL_ONLY_1, true);
            hammond->Mount(DISPLAYID_GENERAL_HAMMOND_CLAYS_MOUNT);
            hammond->AI()->Talk(YELL_OPENING);
        }
        // summon ragamuffins and do text
        if (TempSummon* ragamuffin1 = me->SummonCreature(NPC_RAGAMUFFIN, ragamuffinPositions[0], TEMPSUMMON_TIMED_DESPAWN, 5s))
        {
            ragamuffin1->SetFacingToObject(GetHammond());
            ragamuffin1->AI()->Talk(SAY_CLAY);
        }
        if (TempSummon* ragamuffin2 = me->SummonCreature(NPC_RAGAMUFFIN, ragamuffinPositions[1], TEMPSUMMON_TIMED_DESPAWN, 5s))
        {
            ragamuffin2->SetFacingToObject(GetHammond());
            ragamuffin2->AI()->Talk(SAY_WOW);
        }
    }

    void MoveGeneralHammondClay()
    {
        if (GetHammond())
        {
            GetHammond()->SetWalk(true);
            GetHammond()->GetMotionMaster()->MovePoint(0, generalHammondClayPositions[2], true);
            _events.ScheduleEvent(EVENT_DISMOUNT_HAMMOND_CLAY, 8500ms);
        }
    }

    void SummonAndromath()
    {
        TempSummon* andromath = me->SummonCreature(NPC_ANDROMATH, andromathPositions[0]);
        if (andromath)
        {
            _andromathGUID = andromath->GetGUID();
            andromath->CastSpell(andromath, SPELL_TELEPORT_VISUAL_ONLY_1, true);
        }
    }

    void MoveAndromath()
    {
        if (GetAndromath())
        {
            GetAndromath()->SetWalk(true);
            GetAndromath()->GetMotionMaster()->MovePoint(0, andromathPositions[2], true);
        }
    }

    void SummonDumas()
    {
        TempSummon* dumas = me->SummonCreature(NPC_DUMAS, dumasPositions[0]);
        if (dumas)
        {
            _dumasGUID = dumas->GetGUID();
            dumas->CastSpell(dumas, SPELL_TELEPORT_VISUAL_ONLY_1, true);
        }
    }

    void MoveDumas()
    {
        if (GetDumas())
        {
            GetDumas()->SetWalk(true);
            GetDumas()->GetMotionMaster()->MovePoint(0, dumasPositions[2], true);
        }
    }

    void SummonMinions()
    {
        me->CastStop();
        Talk(SAY_HOGGER_SUMMON_MINIONS);

        for (float distance : { 0.5f, 1.5f, 2.5f })
        {
            // Build offset
            Position offset;
            offset.m_positionX = distance;  // forward
            offset.m_positionY = 0.0f;      // no sideways offset
            offset.m_positionZ = 0.0f;

            // Apply offset to Hogger's current position
            Position pos = me->GetPositionWithOffset(offset);

            // Fix Z height
            pos.m_positionZ = me->GetMap()->GetHeight(
                me->GetPhaseShift(),
                pos.GetPositionX(),
                pos.GetPositionY(),
                pos.GetPositionZ()
            );

            // Summon minion
            me->SummonCreature(NPC_HOGGER_MINION, pos);
        }

        _minionsSummoned = true;
    }

    void MoveHoggerToFinalPosition()
    {
        me->SetWalk(true);
        me->GetMotionMaster()->MovePoint(0, hoggerPositions[0]);
    }

    void TeleportBack()
    {
        if (GetHammond() && GetAndromath() && GetDumas())
        {
            GetHammond()->CastSpell(GetHammond(), SPELL_TELEPORT_VISUAL_ONLY_2, true);
            GetAndromath()->CastSpell(GetAndromath(), SPELL_TELEPORT_VISUAL_ONLY_2, true);
            GetDumas()->CastSpell(GetDumas(), SPELL_TELEPORT_VISUAL_ONLY_2, true);
            DoCastSelf(SPELL_TELEPORT_VISUAL_ONLY_2, true);

            me->DisappearAndDie();
            GetHammond()->DisappearAndDie();
            GetAndromath()->DisappearAndDie();
            GetDumas()->DisappearAndDie();
        }
    }

    Creature* GetHammond()
    {
        return me->GetMap()->GetCreature(_generalHammondGUID);
    }

    Creature* GetAndromath()
    {
        return me->GetMap()->GetCreature(_andromathGUID);
    }

    Creature* GetDumas()
    {
        return me->GetMap()->GetCreature(_dumasGUID);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim() && !_endingSceneActive)
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_VICIOUS_SLICE:
                DoCastVictim(SPELL_VICIOUS_SLICE);
                _events.Repeat(3s);
                break;

            case EVENT_HAMMOND_GROUP_START_WALKING:
                MoveGeneralHammondClay();
                MoveAndromath();
                MoveDumas();
                MoveHoggerToFinalPosition();
                break;

            case EVENT_DISMOUNT_HAMMOND_CLAY:
                if (GetHammond())
                {
                    me->SetFacingToObject(GetHammond());

                    GetHammond()->Dismount();
                    GetHammond()->GetMotionMaster()->MovePoint(0, generalHammondClayPositions[3], true);

                    _events.ScheduleEvent(EVENT_HOGGER_SAY_GRR, 3s);
                }
                break;

            case EVENT_HOGGER_SAY_GRR:
                Talk(SAY_RAND);
                _events.ScheduleEvent(EVENT_CLAYS_EXPLAINATION, 3s);
                break;

            case EVENT_CLAYS_EXPLAINATION:
                if (GetHammond())
                    GetHammond()->AI()->Talk(SAY_EXPLAINATION);
                _events.ScheduleEvent(EVENT_CLAY_SAYS_TAKE_HIM, 4500ms);
                break;

            case EVENT_CLAY_SAYS_TAKE_HIM:
                if (GetHammond())
                    GetHammond()->AI()->Talk(SAY_TAKE_HIM);
                _events.ScheduleEvent(EVENT_HOGGER_SAYS_NOO, 2s);
                break;

            case EVENT_HOGGER_SAYS_NOO:
                Talk(SAY_FINAL);
                _events.ScheduleEvent(EVENT_CLAY_SPEAKS_TO_ANDROMATH, 3s);
                break;

            case EVENT_CLAY_SPEAKS_TO_ANDROMATH:
                if (GetHammond() && GetAndromath())
                {
                    GetHammond()->SetFacingToObject(GetAndromath());
                    GetAndromath()->SetFacingToObject(GetHammond());
                    GetHammond()->AI()->Talk(SAY_TO_ANDROMATH);
                }
                _events.ScheduleEvent(EVENT_TELEPORT_BACK, 4s);
                break;

            case EVENT_TELEPORT_BACK:
                TeleportBack();
                break;

            case EVENT_CHECK_EAT_RANGE:
                if (!me->FindNearestCreature(NPC_EATING_TARGET, 3.0f))
                {
                    _events.ScheduleEvent(EVENT_CHECK_EAT_RANGE, 200ms);
                    break;
                }
                else
                {
                    DoCast(SPELL_EATING);
                    Talk(SAY_EATING);
                    _events.ScheduleEvent(EVENT_BLOODY_STRIKE, 100ms);
                    _isEating = true;
                }
                break;

            case EVENT_BLOODY_STRIKE:
                if (_isEating)
                {
                    if (Creature* dummy = me->FindNearestCreature(NPC_EATING_TARGET, 10.0f))
                        DoCast(dummy, SPELL_BLOODY_STRIKE, true);
                    _events.ScheduleEvent(EVENT_BLOODY_STRIKE, 1s);
                }
                break;

            default:
                break;
            }
        }

        if (!_endingSceneActive)
            me->DoMeleeAttackIfReady();
    }

private:
    EventMap _events;

    ObjectGuid _generalHammondGUID;
    ObjectGuid _andromathGUID;
    ObjectGuid _dumasGUID;
};

struct npc_custom_hogger_minion : public ScriptedAI
{
    npc_custom_hogger_minion(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        me->SetReactState(REACT_AGGRESSIVE);

        if (Creature* hogger = me->FindNearestCreature(NPC_HOGGER, 35.0f, true))
            me->CastSpell(hogger, SPELL_ADVENTURERS_RUSH, true);
    }
};

void AddSC_custom_elwynn_forest_npcs()
{
    new npc_custom_stormwind_infantry();
    RegisterCreatureAI(npc_custom_brother_paxton);
    RegisterCreatureAI(npc_custom_eastvale_lumberjack);
    RegisterCreatureAI(npc_custom_hogger);
    RegisterCreatureAI(npc_custom_hogger_minion);
}
