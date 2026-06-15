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

#include "CreatureAIImpl.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "MotionMaster.h"
#include "MoveSplineInit.h"
#include "ObjectMgr.h"
#include "PassiveAI.h"
#include "SpellAuras.h"
#include "SpellScript.h"
#include "Player.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Vehicle.h"
#include "GridNotifiers.h"
#include "Containers.h"
#include "DB2Stores.h"
#include "Creature.h"
#include "TemporarySummon.h"
#include "G3D/Vector3.h"
#include <list>

#include "Custom_Instance_Deadmines.h"

namespace Scripts::EasternKingdoms::Deadmines
{
    // 48230 - Ogre Henchman
    struct npc_ogre_henchman : public ScriptedAI
    {
        npc_ogre_henchman(Creature* creature) : ScriptedAI(creature)
        {
            ApplyCrowdControlImmunities(me);
        }

        void Reset() override
        {
            me->RemoveAllAuras();
            _hasCastMotivation = false;
            _isSleeping = false;
            _events.Reset();

            // Schedule OOC say event (initial spawn)
            _events.RescheduleEvent(Events::OgreHenchmanOOCSay, 30s, 45s);

            // Special event for spawn ID 375925 (initial spawn)
            if (me->GetSpawnId() == CreatureSpawns::OgreHenchmanSleeper)
                _events.RescheduleEvent(Events::OgreHenchmanSleepEvent, 60s);
        }

        void JustEngagedWith(Unit* who) override
        {
            ScriptedAI::JustEngagedWith(who);
            _events.RescheduleEvent(Events::OgreHenchmanUppercut, std::chrono::seconds(5));

            if (roll_chance(30))
                Talk(Texts::OgreHenchmanAggro, who);
        }

        void EnterEvadeMode(EvadeReason why) override
        {
            ScriptedAI::EnterEvadeMode(why);
            _events.RescheduleEvent(Events::OgreHenchmanOOCSay, 30s, 45s);
        }

        void JustReachedHome() override
        {
            _events.RescheduleEvent(Events::OgreHenchmanOOCSay, 30s, 45s);

            // Special event for spawn ID 375925
            if (me->GetSpawnId() == CreatureSpawns::OgreHenchmanSleeper)
                _events.RescheduleEvent(Events::OgreHenchmanSleepEvent, 60s);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (me->HealthBelowPctDamaged(25, damage) && !_hasCastMotivation)
            {
                _hasCastMotivation = true;
                uint32 spellId = (me->GetMap()->GetDifficultyID() == DIFFICULTY_HEROIC) ? Spells::OgrishMotivationHeroic : Spells::OgrishMotivationNormal;

                DoCastSelf(spellId);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case Events::OgreHenchmanOOCSay:
                        {
                            // Don't say OOC if sleeping (spawn ID 375925)
                            if (_isSleeping)
                            {
                                _events.RescheduleEvent(Events::OgreHenchmanOOCSay, 30s, 45s);
                                break;
                            }

                            uint32 sayId = (me->GetSpawnId() == CreatureSpawns::OgreHenchmanByCannon) ? Texts::OgreHenchmanCandle : Texts::OgreHenchmanOOC;
                            uint32 minTime = (me->GetSpawnId() == CreatureSpawns::OgreHenchmanByCannon) ? 45 : 30;
                            uint32 maxTime = (me->GetSpawnId() == CreatureSpawns::OgreHenchmanByCannon) ? 60 : 45;

                            if (roll_chance(30))
                                Talk(sayId);

                            _events.RescheduleEvent(Events::OgreHenchmanOOCSay, std::chrono::seconds(minTime), std::chrono::seconds(maxTime));
                            break;
                        }
                        case Events::OgreHenchmanSleepEvent:
                        {
                            if (me->GetSpawnId() == CreatureSpawns::OgreHenchmanSleeper)
                            {
                                _isSleeping = true;
                                Talk(Texts::OgreHenchmanSleep);
                                _events.RescheduleEvent(Events::OgreHenchmanSleepAura, 3s);
                            }
                            _events.RescheduleEvent(Events::OgreHenchmanSleepEvent, 90s);
                            break;
                        }
                        case Events::OgreHenchmanSleepAura:
                        {
                            DoCastSelf(Spells::AuraSleep);
                            _events.RescheduleEvent(Events::OgreHenchmanWakeUp, 30s);
                            break;
                        }
                        case Events::OgreHenchmanWakeUp:
                        {
                            me->RemoveAurasDueToSpell(Spells::AuraSleep);
                            _isSleeping = false;
                            break;
                        }
                        default:
                            break;
                    }
                }
                return;
            }

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::OgreHenchmanUppercut:
                        if (Unit* victim = me->GetVictim())
                            DoCast(victim, Spells::Uppercut);
                        _events.RescheduleEvent(Events::OgreHenchmanUppercut, 10s, 14s);
                        break;
                    default:
                        break;
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        bool _hasCastMotivation = false;
        bool _isSleeping = false;
    };

    // Ogre Bodyguard Dialogue Entry
    struct OgreBodyguardDialogueEntry
    {
        int8 talkId;
        Milliseconds delay;
        bool isFinal;
    };

    // 48262 - Ogre Bodyguard
    struct npc_ogre_bodyguard : public ScriptedAI
    {
        npc_ogre_bodyguard(Creature* creature) : ScriptedAI(creature), _dialogueIndex(0)
        {
            _hasCastFrenzy = false;
            _conversationInProgress = false;
        }

        void Reset() override
        {
            ScriptedAI::Reset();
            _hasCastFrenzy = false;
            _dialogueIndex = 0;
            _conversationInProgress = false;
            _events.Reset();

            // Schedule conversation start event (1-5s random)
            _events.RescheduleEvent(Events::OgreBodyguardConversation, 1s, 5s);
        }

        void JustEngagedWith(Unit* who) override
        {
            ScriptedAI::JustEngagedWith(who);

            if (roll_chance(50))
                Talk(Texts::OgreBodyguardAggro, who);

            _events.RescheduleEvent(Events::OgreBodyguardBonk, 4s, 8s);

            // Stop conversation on combat
            _events.CancelEvent(Events::OgreBodyguardConversation);
            _events.CancelEvent(Events::OgreBodyguardDialogueLine);
            _conversationInProgress = false;
        }

        void EnterEvadeMode(EvadeReason why) override
        {
            ScriptedAI::EnterEvadeMode(why);
            _conversationInProgress = false;
            _dialogueIndex = 0;
            _events.RescheduleEvent(Events::OgreBodyguardConversation, 1s, 5s);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (me->HealthBelowPctDamaged(30, damage) && !_hasCastFrenzy)
            {
                _hasCastFrenzy = true;
                DoCastSelf(Spells::Frenzy);
                Talk(Texts::OgreBodyguardFrenzy);
            }
        }

        void SetData(uint32 id, uint32 value) override
        {
            if (id == 1 && value == 1) // Cancel conversation start event
            {
                _events.CancelEvent(Events::OgreBodyguardConversation);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::OgreBodyguardConversation:
                        StartConversation();
                        break;
                    case Events::OgreBodyguardDialogueLine:
                        ProcessDialogueLine();
                        break;
                    case Events::OgreBodyguardBonk:
                        if (Unit* victim = me->GetVictim())
                            me->CastSpell(victim, Spells::Bonk);
                        _events.RescheduleEvent(Events::OgreBodyguardBonk, 14s, 17s);
                        break;
                    default:
                        break;
                }
            }

            if (!UpdateVictim())
                return;

            me->DoMeleeAttackIfReady();
        }

        void StartConversation()
        {
            if (_conversationInProgress)
                return;

            if (Creature* other = me->FindNearestCreature(Creatures::OgreBodyguard, 10.f, true))
            {
                if (other && other->IsAlive())
                {
                    // Only the ogre with the lower GUID initiates the conversation
                    if (me->GetGUID().GetCounter() < other->GetGUID().GetCounter())
                    {
                        _conversationInProgress = true;
                        other->AI()->SetData(1, 1); // Tell other to cancel their start event
                        _dialogueIndex = 0;
                        _events.RescheduleEvent(Events::OgreBodyguardDialogueLine, 1s);
                    }
                    return;
                }
            }
        }

        void ProcessDialogueLine()
        {
            if (_dialogueIndex >= _dialogueSequence.size())
            {
                _conversationInProgress = false;
                _dialogueIndex = 0;
                // Schedule next conversation cycle (45-60s)
                _events.RescheduleEvent(Events::OgreBodyguardConversation, 45s, 60s);
                return;
            }

            const OgreBodyguardDialogueEntry& entry = _dialogueSequence[_dialogueIndex];

            Creature* speaker = me;
            Creature* otherOgre = me->FindNearestCreature(Creatures::OgreBodyguard, 10.f, true);

            // Determine speaker based on dialogue index (alternating)
            if (speaker && otherOgre)
            {
                // Even indices: lower GUID speaks, Odd indices: higher GUID speaks
                if (_dialogueIndex % 2 == 0)
                {
                    if (speaker->GetGUID().GetCounter() > otherOgre->GetGUID().GetCounter())
                        std::swap(speaker, otherOgre);
                }
                else
                {
                    if (speaker->GetGUID().GetCounter() < otherOgre->GetGUID().GetCounter())
                        std::swap(speaker, otherOgre);
                }
            }

            if (speaker)
                speaker->AI()->Talk(entry.talkId);

            ++_dialogueIndex;

            if (entry.isFinal)
            {
                _conversationInProgress = false;
                _dialogueIndex = 0;
                _events.RescheduleEvent(Events::OgreBodyguardConversation, 45s, 60s);
            }
            else
            {
                _events.RescheduleEvent(Events::OgreBodyguardDialogueLine, entry.delay);
            }
        }

    private:
        EventMap _events;
        bool _hasCastFrenzy = false;
        size_t _dialogueIndex;
        bool _conversationInProgress;

        static const std::vector<OgreBodyguardDialogueEntry> _dialogueSequence;
    };

    const std::vector<OgreBodyguardDialogueEntry> npc_ogre_bodyguard::_dialogueSequence =
    {
        { Texts::OgreBodyguardConversation1, 3s, false },
        { Texts::OgreBodyguardConversation2, 5s, false },
        { Texts::OgreBodyguardConversation3, 4s, false },
        { Texts::OgreBodyguardConversation4, 3s, false },
        { Texts::OgreBodyguardConversation5, 5s, false },
        { Texts::OgreBodyguardConversation6, 4s, false },
        { Texts::OgreBodyguardConversation7, 3s, false },
        { Texts::OgreBodyguardConversation8, 5s, false },
        { Texts::OgreBodyguardConversation9, 4s, false },
        { Texts::OgreBodyguardConversation10, 3s, false },
        { Texts::OgreBodyguardConversation10, 3s, true } // Both say line 11
    };

    // 48284 - Mining Powder
    struct npc_mining_powder : public ScriptedAI
    {
        npc_mining_powder(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            ScriptedAI::Reset();
            _hasExploded = false;
            _attackerGUID.Clear();
            me->SetReactState(REACT_PASSIVE);
        }

        void DamageTaken(Unit* attacker, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (_hasExploded)
                return;

            _hasExploded = true;
            _attackerGUID = attacker->GetGUID();
            DoCastSelf(Spells::Explode);
            me->DespawnOrUnsummon(100ms);
        }

        ObjectGuid GetGUID(int32 id) const override
        {
            if (id == 1)
                return _attackerGUID;
            return ObjectGuid::Empty;
        }

    private:
        bool _hasExploded = false;
        ObjectGuid _attackerGUID;
    };

    // 48421 - Defias Overseer
    struct npc_defias_overseer : public ScriptedAI
    {
        npc_defias_overseer(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            ScriptedAI::Reset();
            _isController = false;
            _hasFled = false;
            _dialogueLine = 0;
            _events.Reset();

            // Delay controller determination to ensure both NPCs are spawned
            _events.RescheduleEvent(Events::OverseerInitController, 1s);
        }

        void JustEngagedWith(Unit* who) override
        {
            ScriptedAI::JustEngagedWith(who);
            _events.Reset();

            // Link combat with partner
            if (Creature* partner = me->FindNearestCreature(Creatures::DefiasOverseer, 20.0f, true))
                if (partner->IsAlive() && !partner->IsInCombat())
                    partner->AI()->AttackStart(who);
        }

        void JustReachedHome() override
        {
            ScriptedAI::JustReachedHome();

            // Check if partner is dead, if so respawn them
            if (Creature* partner = me->FindNearestCreature(Creatures::DefiasOverseer, 20.0f, false))
                if (!partner->IsAlive())
                    partner->Respawn(true);

            // Reschedule dialogue start after respawn
            _events.RescheduleEvent(Events::OverseerInitController, 10s);
        }

        void JustDied(Unit* killer) override
        {
            ScriptedAI::JustDied(killer);
            _events.Reset();
        }

        void DamageTaken(Unit* attacker, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (HealthBelowPct(15) && !_hasFled)
            {
                _hasFled = true;
                Talk(Texts::OverseerFlee);
                me->GetMotionMaster()->MoveFleeing(attacker, 10s);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case Events::OverseerInitController:
                            // Determine if this is the controller (lower GUID)
                            if (Creature* partner = me->FindNearestCreature(Creatures::DefiasOverseer, 20.0f, true))
                            {
                                _isController = (me->GetGUID().GetCounter() < partner->GetGUID().GetCounter());
                                if (_isController)
                                    _events.RescheduleEvent(Events::OverseerDialogueStart, 4s); // 1s already elapsed
                            }
                            break;
                        case Events::OverseerDialogueStart:
                            if (_isController)
                                StartDialogue();
                            break;
                        case Events::OverseerDialogueLine:
                            if (_isController)
                                SayNextDialogueLine();
                            break;
                        default:
                            break;
                    }
                }
                return;
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        void StartDialogue()
        {
            _dialogueLine = 0;
            SayNextDialogueLine();
        }

        void SayNextDialogueLine()
        {
            Creature* partner = me->FindNearestCreature(Creatures::DefiasOverseer, 20.0f, true);
            if (!partner || partner == me)
            {
                // Partner not available, restart dialogue after delay
                _events.RescheduleEvent(Events::OverseerDialogueStart, 30s, 45s);
                return;
            }

            if (_dialogueLine >= 10)
            {
                // Dialogue complete, restart after delay
                _dialogueLine = 0;
                _events.RescheduleEvent(Events::OverseerDialogueStart, 30s, 45s);
                return;
            }

            // Determine which overseer should speak (alternating)
            // Even lines (0, 2, 4, 6, 8) - this overseer speaks
            // Odd lines (1, 3, 5, 7, 9) - partner speaks
            Creature* speaker = (_dialogueLine % 2 == 0) ? me : partner;
            speaker->AI()->Talk(_dialogueLine);

            // Calculate delay based on text length
            uint32 delay = 3000; // default 3s
            switch (_dialogueLine)
            {
                case 0: delay = 3500; break; // "It's broken."
                case 1: delay = 3500; break; // "It's not broken!"
                case 2: delay = 5000; break; // "Why's it shooting steam..."
                case 3: delay = 5000; break; // "That's the...pressure release valve."
                case 4: delay = 4500; break; // "In the middle of the pipe?"
                case 5: delay = 4500; break; // "Er...backup release valve?"
                case 6: delay = 4000; break; // "We should tell the Admiral."
                case 7: delay = 3500; break; // "You tell him."
                case 8: delay = 4500; break; // "No way! He gives me the creeps!"
                case 9: delay = 7000; break; // "I know! The way he looks at you..."
                default: delay = 3000; break;
            }

            _dialogueLine++;
            _events.RescheduleEvent(Events::OverseerDialogueLine, std::chrono::milliseconds(delay));
        }

        bool _isController = false;
        int8 _dialogueLine = 0;
        bool _hasFled = false;
        EventMap _events;
    };

    // 48280, 48439 - Goblin Foundry Event (Engineer & Craftsman)
    struct npc_goblin_foundry_worker : public ScriptedAI
    {
        npc_goblin_foundry_worker(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            ScriptedAI::Reset();
            _events.Reset();
        }

        void SetData(uint32 id, uint32 value) override
        {
            if (id == Misc::GoblinFoundryEventNPC)
            {
                _npcRole = value;
                _isEventNPC = true;
            }
        }

        uint32 GetData(uint32 id) const override
        {
            if (id == Misc::GoblinFoundryEventNPC)
                return _npcRole;
            return 0;
        }

        void DoAction(int32 action) override
        {
            if (!_isEventNPC)
                return;

            if (action == 1)
                StartFirstMovement();
        }

        void MovementInform(uint32 motionType, uint32 pointId) override
        {
            if (motionType != POINT_MOTION_TYPE)
                return;

            if (!_isEventNPC)
                return;

            switch (pointId)
            {
                case 1: // Reached first movement point (Craftsman 1 only)
                    if (_npcRole == 1)
                    {
                        Talk(Texts::GoblinCraftsmanAlive);
                        _events.RescheduleEvent(Events::GoblinWorkerRunToFinal, 7s);
                    }
                    break;
                case 2: // Reached GoblinMoveTo
                    OnReachMoveTo();
                    break;
                case 3: // Reached final destination
                    // NPC stays here until instance reset
                    break;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!_isEventNPC)
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case Events::GoblinWorkerStartMovement: // Start first movement after delay
                    if (_npcRole == 1)
                        me->GetMotionMaster()->MovePoint(1, Positions::GoblinCraftsman1MoveTo);
                    else if (_npcRole == 2 || _npcRole == 3 || _npcRole == 4)
                        me->GetMotionMaster()->MovePoint(2, Positions::GoblinMoveTo);
                    break;
                case Events::GoblinWorkerRunToFinal: // Run to final destination
                    me->GetMotionMaster()->MovePoint(3, GetRunToPosition());
                    break;
                case Events::GoblinWorkerCraftsmanNo: // Craftsman 2 SAY GoblinCraftsmanNo
                    Talk(Texts::GoblinCraftsmanNo);
                    _events.RescheduleEvent(Events::GoblinWorkerRunToFinal, 3s);
                    break;
                }
            }
        }

    private:
        void StartFirstMovement()
        {
            switch (_npcRole)
            {
            case 1:
            case 2:
                _events.RescheduleEvent(Events::GoblinWorkerStartMovement, 1s);
                break;
            case 3:
                _events.RescheduleEvent(Events::GoblinWorkerStartMovement, 2s);
                break;
            case 4:
                _events.RescheduleEvent(Events::GoblinWorkerStartMovement, 6s);
                break;
            default:
                break;
            }
        }

        void OnReachMoveTo()
        {
            // Role 2: Craftsman 2 - SAY GoblinCraftsmanYellAlive, then SAY GoblinCraftsmanNo 2s later, then run
            // Role 3: Engineer 1 - SAY GoblinEngineerRun, then immediately run to GoblinRunTo
            // Role 4: Engineer 2 - Run to GoblinRunTo
            if (_npcRole == 2)
            {
                Talk(Texts::GoblinCraftsmanYellAlive);
                _events.RescheduleEvent(Events::GoblinWorkerCraftsmanNo, 2s);
            }
            else if (_npcRole == 3)
            {
                Talk(Texts::GoblinEngineerRun);
                me->GetMotionMaster()->MovePoint(3, GetRunToPosition());
            }
            else if (_npcRole == 4)
                me->GetMotionMaster()->MovePoint(3, GetRunToPosition());
        }

        Position GetRunToPosition()
        {
            // Assign one of the 4 GoblinRunTo positions
            static const Position runToPositions[] =
            {
                Positions::GoblinRunTo1,
                Positions::GoblinRunTo2,
                Positions::GoblinRunTo3,
                Positions::GoblinRunTo4
            };

            // Use a simple hash of GUID to determine position
            uint32 index = me->GetGUID().GetCounter() % 4;
            return runToPositions[index];
        }

        bool _isEventNPC = false;
        uint32 _npcRole = 0;
        EventMap _events;
    };

    // 48266 - Defis Cannon
    struct npc_defias_cannon : public ScriptedAI
    {
        npc_defias_cannon(Creature* creature) : ScriptedAI(creature)
        {
        }

        void Reset() override
        {
            ScriptedAI::Reset();
        }

        void SetData(uint32 id, uint32 value) override
        {
            if (id == 1 && value == 1) // Fire Cannon
            {
                if (InstanceScript* instance = me->GetInstanceScript())
                {
                    if (instance->GetData(Misc::DeadminesVersion) == Version::Classic)
                        return;

                    if (instance->GetData(CannonEvent::DATA_EVENT) != CannonEvent::STATE_DONE)
                        return;

                    DoFire();
                }
            }
        }

        static uint32 GetImpactBunnyForCannon(uint32 cannonSpawn)
        {
            for (auto const& pair : CannonsFiringEvent::CannonImpactMap)
                if (pair.cannonSpawnId == cannonSpawn)
                    return pair.impactSpawnId;

            return 0; // not found
        }

        void DoFire()
        {
            uint32 impactBunnySpawn = GetImpactBunnyForCannon(me->GetSpawnId());
            if (!impactBunnySpawn)
                return;

            if (Creature* bunny = me->GetMap()->GetCreatureBySpawnId(impactBunnySpawn))
                me->CastSpell(bunny->GetPosition(), Spells::DefiasCannonCannonballFire);
        }
    };

    // 48522 - Defias Pirate
    struct npc_defias_pirate : public ScriptedAI
    {
        npc_defias_pirate(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            ScriptedAI::Reset();
            _events.Reset();
            _cannonGUID.Clear();
            _events.ScheduleEvent(Events::DefiasPirateCannonCheck, 3s, 7s);
        }

        void JustEngagedWith(Unit* who) override
        {
            _events.CancelEvent(Events::DefiasPirateCannonCheck);
            _events.CancelEvent(Events::DefiasPirateCannonFire);

            if (who)
                me->CastSpell(who, Spells::LeapCleave, false);
        }

        void EnterEvadeMode(EvadeReason why) override
        {
            ScriptedAI::EnterEvadeMode(why);
            _events.ScheduleEvent(Events::DefiasPirateCannonCheck, 3s, 7s);
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
                    case Events::DefiasPirateCannonCheck:
                    {
                        InstanceScript* instance = me->GetInstanceScript();
                        if (instance && instance->GetData(Misc::DeadminesVersion) != Version::Classic &&
                            instance->GetData(CannonEvent::DATA_EVENT) == CannonEvent::STATE_DONE)
                        {
                            if (Creature* cannon = me->FindNearestCreature(Creatures::DefiasCannon, 5.0f, true))
                            {
                                _cannonGUID = cannon->GetGUID();
                                me->HandleEmoteCommand(Emote::EMOTE_ONESHOT_USE_STANDING);
                                _events.ScheduleEvent(Events::DefiasPirateCannonFire, 2s);
                            }
                        }
                        _events.ScheduleEvent(Events::DefiasPirateCannonCheck, 5s, 9s);
                        break;
                    }
                    case Events::DefiasPirateCannonFire:
                        if (Creature* cannon = ObjectAccessor::GetCreature(*me, _cannonGUID))
                            if (cannon->IsAlive())
                                cannon->AI()->SetData(1, 1);
                        break;
                }
            }

            if (UpdateVictim())
                me->DoMeleeAttackIfReady();
        }

    private:
        ObjectGuid _cannonGUID;
        EventMap _events;
    };
}

void AddSC_custom_deadmines_npcs()
{
    using namespace Scripts::EasternKingdoms::Deadmines;

    RegisterCreatureAI(npc_defias_cannon);
    RegisterCreatureAI(npc_defias_pirate);
    RegisterCreatureAI(npc_ogre_henchman);
    RegisterCreatureAI(npc_ogre_bodyguard);
    RegisterCreatureAI(npc_mining_powder);
    RegisterCreatureAI(npc_defias_overseer);
    RegisterCreatureAI(npc_goblin_foundry_worker);
}
