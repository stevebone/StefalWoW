/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "CombatAI.h"
#include "Containers.h"
#include "CreatureAIImpl.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"

namespace Scripts::EasternKingdoms::Westfall
{
namespace Creatures
{
    static constexpr uint32 EnergizedHarvestReaper = 42342;
    static constexpr uint32 OverloadedHarvestGolem = 42601;
    static constexpr uint32 JangolodeMineGlubtok = 42492;
    static constexpr uint32 JangolodeMineFigure = 42515;
}

namespace Spells
{
    static constexpr uint32 JangolodeMineSummonFigure = 79265;
    static constexpr uint32 JangolodeMineSummonGlubtok = 79263;
}

namespace Events
{
    namespace ItsAlive
    {
        static constexpr uint32 CheckArea = 1;
        static constexpr uint32 DespawnHarvester = 2;
    }
}

namespace Text
{
    namespace HarvestGolem
    {
        static constexpr uint32 AnnounceOutOfArea = 0;
    }
}

namespace Area
{
    static constexpr uint32 TheMoestFarm = 918;
}

// 79084 - Unbound Energy
class spell_westfall_unbound_energy : public SpellScript
{
    void FilterTargets(std::list<WorldObject*>& targets)
    {
        if (targets.empty())
            return;

        Unit* caster = GetCaster();
        targets.remove_if([caster](WorldObject const* target) -> bool
        {
            return caster->ToWorldObject() == target;
        });

        if (targets.size() > 1)
            Trinity::Containers::RandomResize(targets, 1);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_westfall_unbound_energy::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
    }
};

// 42601 - Overloaded Harvest Golem
struct npc_westfall_overloaded_harvest_golem : public ScriptedAI
{
    npc_westfall_overloaded_harvest_golem(Creature* creature) : ScriptedAI(creature) {}

    void JustAppeared() override
    {
        _events.ScheduleEvent(Events::ItsAlive::CheckArea, 1s);
    }

    void PassengerBoarded(Unit* /*passenger*/, int8 /*seatId*/, bool apply) override
    {
        if (!apply)
            me->DespawnOrUnsummon();
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);
        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case Events::ItsAlive::CheckArea:
                    if (me->GetAreaId() != Area::TheMoestFarm)
                    {
                        if (Unit* owner = me->GetCharmerOrOwner())
                            Talk(Text::HarvestGolem::AnnounceOutOfArea, owner);
                        _events.ScheduleEvent(Events::ItsAlive::DespawnHarvester, 8s);
                    }
                    else
                        _events.Repeat(1s);
                    break;
                case Events::ItsAlive::DespawnHarvester:
                    if (me->GetAreaId() != Area::TheMoestFarm)
                        me->DespawnOrUnsummon();
                    else
                        _events.ScheduleEvent(Events::ItsAlive::CheckArea, 1s);
                    break;
                default:
                    break;
            }
        }
    }

private:
    EventMap _events;
};

// 79425 - Reaping Blows
class spell_westfall_reaping_blows : public AuraScript
{
    bool Validate(SpellInfo const* spellInfo) override
    {
        return ValidateSpellEffect({ { spellInfo->Id, EFFECT_1 } }) && ValidateSpellInfo({ uint32(spellInfo->GetEffect(EFFECT_1).TriggerSpell) });
    }

    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        // HACK
        // periodic ticks are forcing to cast the spell onto himself instead of target
        // ref AuraEffect::HandlePeriodicTriggerSpellAuraTick
        PreventDefaultAction();
        if (Creature* reaper = GetTarget()->FindNearestCreature(Creatures::EnergizedHarvestReaper, 5.f, true))
            GetTarget()->CastSpell(reaper, GetSpellInfo()->GetEffect(EFFECT_1).TriggerSpell, true);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_westfall_reaping_blows::HandlePeriodic, EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

// 79436 - Wake Harvest Golem
class spell_westfall_wake_harvest_golem : public SpellScript
{
    SpellCastResult CheckTarget()
    {
        Unit* target = GetExplTargetUnit();
        if (!target || !target->IsCreature())
            return SPELL_FAILED_BAD_TARGETS;

        return SPELL_CAST_OK;
    }

    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster || !caster->IsPlayer())
            return;

        if (Creature* target = GetHitCreature())
        {
            caster->ToPlayer()->KilledMonsterCredit(Creatures::OverloadedHarvestGolem);
            target->DespawnOrUnsummon(100ms);
        }
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_westfall_wake_harvest_golem::CheckTarget);
        OnEffectHitTarget += SpellEffectFn(spell_westfall_wake_harvest_golem::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// 79262 - Summon Lou's House
class spell_westfall_summon_lous_house : public SpellScript
{
    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        caster->CastSpell(nullptr, Spells::JangolodeMineSummonFigure, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
        caster->CastSpell(nullptr, Spells::JangolodeMineSummonGlubtok, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_westfall_summon_lous_house::HandleScriptEffect, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// 79275 - Quest Credit: Jangolde Event
class spell_westfall_quest_credit_jangolode_event : public SpellScript
{
    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        GetHitUnit()->ExitVehicle();
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_westfall_quest_credit_jangolode_event::HandleScriptEffect, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// 79290 - General Trigger 1: Glubtok
// 79292 - General Trigger 2: Glubtok
// 79294 - General Trigger 3: Glubtok
// 79297 - General Trigger 4: Glubtok
class spell_westfall_livin_the_life_ping_glubtok : public SpellScript
{
    void FilterTargets(std::list<WorldObject*>& targets)
    {
        Player* player = Object::ToPlayer(GetCaster());
        if (!player)
            return;

        targets.clear();
        if (Creature* creature = player->FindNearestCreatureWithOptions(50.0f, { .CreatureId = Creatures::JangolodeMineGlubtok, .OwnerGuid = player->GetGUID() }))
            targets.push_back(creature);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_westfall_livin_the_life_ping_glubtok::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
    }
};

// 79279 - General Trigger 1: Figure
// 79283 - General Trigger 2: Figure
// 79284 - General Trigger 3: Figure
// 79287 - General Trigger 4: Figure
class spell_westfall_livin_the_life_ping_figure : public SpellScript
{
    void FilterTargets(std::list<WorldObject*>& targets)
    {
        Player* player = Object::ToPlayer(GetCaster());
        if (!player)
            return;

        targets.clear();
        if (Creature* creature = player->FindNearestCreatureWithOptions(50.0f, { .CreatureId = Creatures::JangolodeMineFigure, .OwnerGuid = player->GetGUID() }))
            targets.push_back(creature);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_westfall_livin_the_life_ping_figure::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
    }
};

// 79273 - Despawn Jangolode Actor
class spell_westfall_despawn_jangolode_actor : public SpellScript
{
    void FilterTargets(std::list<WorldObject*>& targets)
    {
        Player* player = Object::ToPlayer(GetCaster());
        if (!player)
            return;

        targets.clear();
        if (Creature* creature = player->FindNearestCreatureWithOptions(50.0f, { .CreatureId = Creatures::JangolodeMineGlubtok, .OwnerGuid = player->GetGUID() }))
            targets.push_back(creature);

        if (Creature* creature = player->FindNearestCreatureWithOptions(50.0f, { .CreatureId = Creatures::JangolodeMineFigure, .OwnerGuid = player->GetGUID() }))
            targets.push_back(creature);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_westfall_despawn_jangolode_actor::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
    }
};
}

/*######
## npc_westplains_drifter
######*/

enum edrifter
{
    CREDIT_SAY1 = 42414,
    CREDIT_SAY2 = 42415,
    CREDIT_SAY3 = 42416,
    CREDIT_SAY4 = 42417,

    QUEST_MURDER_WAS_THE_CASE_THAT_THEY_GAVE_ME = 26209
};

#define GOSSIP_COST 2
#define GOSSIP_HELLO_DRIFTER1 "Did you see who killed the Furlbrows?"
#define GOSSIP_HELLO_DRIFTER2 "Maybe a couple copper will loosen your tongue. Now tell me, did you see who killed the Furlbrows?"

class npc_westplains_drifter : public CreatureScript
{
public:
    npc_westplains_drifter() : CreatureScript("npc_westplains_drifter") { }

    struct npc_westplains_drifterAI : public ScriptedAI
    {
        npc_westplains_drifterAI(Creature* creature) : ScriptedAI(creature) { }

        bool OnGossipHello(Player* player) override
        {
            if (player->GetQuestStatus(QUEST_MURDER_WAS_THE_CASE_THAT_THEY_GAVE_ME) == QUEST_STATUS_INCOMPLETE)
            {
                AddGossipItemFor(player, GossipOptionNpc::None,
                    GOSSIP_HELLO_DRIFTER1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

                AddGossipItemFor(player, GossipOptionNpc::None,
                    GOSSIP_HELLO_DRIFTER2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            }

            SendGossipMenuFor(player, player->GetGossipTextId(me), me->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
        {
            uint32 const action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
            player->PlayerTalkClass->ClearMenus();

            if (action == GOSSIP_ACTION_INFO_DEF + 1)
            {
                switch (rand() % 4)
                {
                case 0:
                    CloseGossipMenuFor(player);
                    me->Say("Listen, pal. I don't want any trouble, ok? I didn't see who murdered 'em, but I sure heard it! Lots of yelling. Human voices... you dig? Now get out of here before I change my mind about beating you up and takin' your shoes.", LANG_UNIVERSAL);
                    player->KilledMonsterCredit(CREDIT_SAY1);
                    me->SetStandState(UNIT_STAND_STATE_STAND);
                    me->DespawnOrUnsummon(5s);
                    break;

                case 1:
                    CloseGossipMenuFor(player);
                    me->Say("I didn't see who killed 'm, bub/sis, but I got a whiff. Smelled rich, kinda like you. Damn shame too. Furlbrows were a fixture around here. Nice people, always willin' to share a meal or a patch of dirt.", LANG_UNIVERSAL);
                    player->KilledMonsterCredit(CREDIT_SAY2);
                    me->SetStandState(UNIT_STAND_STATE_STAND);
                    me->DespawnOrUnsummon(5s);
                    break;

                case 2:
                    CloseGossipMenuFor(player);
                    me->Say("Who killed the Furlbrows? I'll tell you who killed the Furlbrows: KING VARIAN WRYNN. THAT'S WHO! And he's killin' the rest of us too. One bum at a time. The only thing I can tell you is that I saw some gnolls leavin' the place a few hours before the law arrived.", LANG_UNIVERSAL);
                    player->KilledMonsterCredit(CREDIT_SAY3);
                    me->SetStandState(UNIT_STAND_STATE_STAND);
                    me->DespawnOrUnsummon(5s);
                    break;

                case 3:
                    CloseGossipMenuFor(player);
                    me->Say("Between you, me, and the tree, murlocs killed the Furlbrows. Yep, saw 'em with my own two eyes. Think they'd been casin' the joint for days, maybe months. They left in a hurry once they got wind of 'Johnny Law' and the idiot brigade over there...", LANG_UNIVERSAL);
                    player->KilledMonsterCredit(CREDIT_SAY4);
                    me->SetStandState(UNIT_STAND_STATE_STAND);
                    me->DespawnOrUnsummon(5s);
                    break;
                }
            }


            if (action == GOSSIP_ACTION_INFO_DEF + 2)
            {
                if (!player->HasEnoughMoney(uint64(GOSSIP_COST)))
                {
                    player->SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
                    CloseGossipMenuFor(player);
                }
                else
                {
                    player->ModifyMoney(-(GOSSIP_COST));

                    switch (rand() % 8)
                    {
                    case 0:
                        CloseGossipMenuFor(player);
                        me->Say("Listen, pal. I don't want any trouble, ok? I didn't see who murdered 'em, but I sure heard it! Lots of yelling. Human voices... you dig? Now get out of here before I change my mind about beating you up and takin' your shoes.", LANG_UNIVERSAL);
                        player->KilledMonsterCredit(CREDIT_SAY1);
                        me->DespawnOrUnsummon(5s);
                        break;

                    case 1:
                        CloseGossipMenuFor(player);
                        me->Say("I didn't see who killed 'm, bub/sis, but I got a whiff. Smelled rich, kinda like you. Damn shame too. Furlbrows were a fixture around here. Nice people, always willin' to share a meal or a patch of dirt.", LANG_UNIVERSAL);
                        player->KilledMonsterCredit(CREDIT_SAY2);
                        me->DespawnOrUnsummon(5s);
                        break;

                    case 2:
                        CloseGossipMenuFor(player);
                        me->Say("Who killed the Furlbrows? I'll tell you who killed the Furlbrows: KING VARIAN WRYNN. THAT'S WHO! And he's killin' the rest of us too. One bum at a time. The only thing I can tell you is that I saw some gnolls leavin' the place a few hours before the law arrived.", LANG_UNIVERSAL);
                        player->KilledMonsterCredit(CREDIT_SAY3);
                        me->DespawnOrUnsummon(5s);
                        break;

                    case 3:
                        CloseGossipMenuFor(player);
                        me->Say("Between you, me, and the tree, murlocs killed the Furlbrows. Yep, saw 'em with my own two eyes. Think they'd been casin' the joint for days, maybe months. They left in a hurry once they got wind of 'Johnny Law' and the idiot brigade over there...", LANG_UNIVERSAL);
                        player->KilledMonsterCredit(CREDIT_SAY4);
                        me->DespawnOrUnsummon(5s);
                        break;
                    case 4:
                        CloseGossipMenuFor(player);
                        me->Say("I wonder if it's possible to eat rocks? Got plenty of rocks around here. Just imagine it! I'd be the richest person in the world for making that discovery!", LANG_UNIVERSAL);
                        me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);
                        me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->AI()->AttackStart(player);
                        break;
                    case 5:
                        CloseGossipMenuFor(player);
                        me->Say("Looks like I found us a savory and clean piece of dirt! Tonight we eat like kings, Mr. Penguin! Of course I'll share it with you! You're my best friend!", LANG_UNIVERSAL);
                        me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);
                        me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->AI()->AttackStart(player);
                        break;
                    case 6:
                        CloseGossipMenuFor(player);
                        me->Say("HAHAHAH! Good one, Mr. Penguin! GOOD ONE!", LANG_UNIVERSAL);
                        me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);
                        me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->AI()->AttackStart(player);
                        break;
                    case 7:
                        CloseGossipMenuFor(player);
                        me->Say("What happened to me? I used to be the king of Stormwind!", LANG_UNIVERSAL);
                        me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);
                        me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->AI()->AttackStart(player);
                        break;
                    }
                }
            }

            return true;
        }

    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_westplains_drifterAI(creature);
    }
};

/*######
## npc_lous_parting_thoughts_trigger
######*/

enum eThug
{
    QUEST_LOUS_PARTING_THOUGHTS = 26232,
    NPC_THUG = 42387,
    NPC_TRIGGER = 42562
};

static Position const ThugPositions[4] =
{
    { -9859.36f, 1332.42f, 41.985f, 2.49f },
    { -9862.51f, 1332.08f, 41.985f, 0.85f },
    { -9863.49f, 1335.49f, 41.985f, 5.63f },
    { -9860.42f, 1335.46f, 41.985f, 4.11f },
};

class npc_lous_parting_thoughts_trigger : public CreatureScript
{
public:
    npc_lous_parting_thoughts_trigger() : CreatureScript("npc_lous_parting_thoughts_trigger") { }

    struct npc_lous_parting_thoughts_triggerAI : public ScriptedAI
    {
        npc_lous_parting_thoughts_triggerAI(Creature* creature) : ScriptedAI(creature) { }

        ObjectGuid PlayerGUID;
        std::array<ObjectGuid, 4> ThugGUIDs;

        uint32 PhaseTimer = 0;
        uint8  Phase = 0;
        uint8  DeadThugs = 0;
        bool   EventStarted = false;
        bool   EventLocked = false;

        void Reset() override
        {
            Phase = 0;
            DeadThugs = 0;
            EventStarted = false;
            EventLocked = false;
            PhaseTimer = 1000;

            for (auto& guid : ThugGUIDs)
                guid.Clear();
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (EventStarted)
                return;

            Player* player = who->ToPlayer();
            if (!player)
                return;

            if (player->GetQuestStatus(QUEST_LOUS_PARTING_THOUGHTS) != QUEST_STATUS_INCOMPLETE)
                return;

            if (!who->IsWithinDistInMap(me, 20.0f))
                return;

            PlayerGUID = player->GetGUID();
            StartEvent();
        }

        void StartEvent()
        {
            EventStarted = true;

            for (uint8 i = 0; i < 4; ++i)
            {
                if (Creature* thug = me->SummonCreature(
                    NPC_THUG,
                    ThugPositions[i],
                    TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT,
                    90s))
                {
                    thug->SetReactState(REACT_PASSIVE);
                    thug->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC);

                    ThugGUIDs[i] = thug->GetGUID();
                }

            }

            PhaseTimer = 2000;
        }

        Creature* GetThug(uint8 index)
        {
            return ObjectAccessor::GetCreature(*me, ThugGUIDs[index]);
        }

        void SetData(uint32 id, uint32 /*value*/) override
        {
            if (id != 1)
                return;

            ++DeadThugs;

            if (DeadThugs >= 4)
            {
                if (Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                {
                    me->TextEmote("Hurry back to the Furlbrow's Cottage.", player, true);
                    player->KilledMonsterCredit(42417, PlayerGUID);
                    player->RemoveAurasDueToSpell(79229);
                    player->CastSpell(player, 79341);
                }


                Reset();
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!EventStarted || EventLocked)
                return;

            if (PhaseTimer <= diff)
            {
                switch (Phase)
                {
                case 0:
                    if (Creature* thug = GetThug(1))
                        thug->Say("Did you... did you meet her?", LANG_UNIVERSAL);
                    PhaseTimer = 3500;
                    break;

                case 1:
                    if (Creature* thug = GetThug(0))
                        thug->Say("Yep. She's for real?", LANG_UNIVERSAL);
                    PhaseTimer = 4000;
                    break;

                case 2:
                    if (Creature* thug = GetThug(0))
                        thug->Say("She wanted me to tell you that she appreciates the job that we did for her on the Furlbrows.", LANG_UNIVERSAL);
                    PhaseTimer = 7000;
                    break;

                case 3:
                    if (Creature* thug = GetThug(3))
                        thug->Say("See her face. It is really...", LANG_UNIVERSAL);
                    PhaseTimer = 4000;
                    break;

                case 4:
                    if (Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                        for (auto guid : ThugGUIDs)
                            if (Creature* thug = ObjectAccessor::GetCreature(*me, guid))
                                thug->SetFacingToObject(player);
                    PhaseTimer = 1000;
                    break;

                case 5:
                    if (Creature* thug = GetThug(2))
                        thug->Say("Whoa, what do we have here? Looks like we have ourselves an eavesdropper.", LANG_UNIVERSAL);
                    PhaseTimer = 4500;
                    break;

                case 6:
                    if (Creature* thug = GetThug(1))
                        thug->Say("Only one thing to do with an eavesdropper.", LANG_UNIVERSAL);
                    PhaseTimer = 4500;
                    break;

                case 7:
                    if (Creature* thug = GetThug(0))
                        thug->Say("DIE!!!", LANG_UNIVERSAL);
                    PhaseTimer = 2000;
                    break;

                case 8:
                    EventLocked = true;

                    for (auto guid : ThugGUIDs)
                    {
                        if (Creature* thug = ObjectAccessor::GetCreature(*me, guid))
                        {
                            thug->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC);
                            thug->SetFaction(14);
                            thug->SetReactState(REACT_AGGRESSIVE);
                        }
                    }
                    PhaseTimer = 1000;
                    break;

                case 9:
                    if (Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                        for (auto guid : ThugGUIDs)
                            if (Creature* thug = ObjectAccessor::GetCreature(*me, guid))
                                thug->AI()->AttackStart(player);
                    PhaseTimer = 0;
                    break;
                }

                ++Phase;
            }
            else
                PhaseTimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_lous_parting_thoughts_triggerAI(creature);
    }
};

/*######
## npc_lous_parting_thoughts_thug
######*/

class npc_lous_parting_thoughts_thug : public CreatureScript
{
public:
    npc_lous_parting_thoughts_thug() : CreatureScript("npc_lous_parting_thoughts_thug") { }

    struct npc_lous_parting_thoughts_thugAI : public ScriptedAI
    {
        npc_lous_parting_thoughts_thugAI(Creature* creature) : ScriptedAI(creature) { }

        void JustDied(Unit* /*killer*/) override
        {
            if (TempSummon* summon = me->ToTempSummon())
            {
                if (Creature* trigger = ObjectAccessor::GetCreature(*me, summon->GetSummonerGUID()))
                {
                    if (trigger->AI())
                        trigger->AI()->SetData(1, 1); // notify kill
                }
            }
        }

    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_lous_parting_thoughts_thugAI(creature);
    }
};

enum eHobo
{
    QUEST_FEEDING_THE_HUNGRY = 26271,
    STEW = 42617,
    SPELL_FULL_BELLY = 79451,
    HOMELESS_STORMWIND_CITIZEN = 42384,
    HOMELESS_STORMWIND_CITIZEN_FEMALE = 42386
    
};

class npc_hungry_hobo : public CreatureScript
{
public:
    npc_hungry_hobo() : CreatureScript("npc_hungry_hobo") {}

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_hungry_hoboAI(creature);
    }

    struct npc_hungry_hoboAI : public ScriptedAI
    {
        npc_hungry_hoboAI(Creature* creature) : ScriptedAI(creature) {}

        uint8 count;
        uint32 Miam;

        void Reset() override
        {
            count = 0;
            Miam = 2000;
        }

        void Eat()
        {
            me->CastSpell(me, SPELL_FULL_BELLY, true);
            me->SetStandState(UNIT_STAND_STATE_SIT);
        }

        void UpdateAI(uint32 diff) override
        {
            if (Miam < diff)
            {
                if (Creature* stew = me->FindNearestCreature(STEW, 10.0f, true))
                {
                    if (me->HasAura(SPELL_FULL_BELLY) && count == 0)
                        return;

                    switch (count)
                    {
                    case 0:
                    {
                        static constexpr uint32 SPELL_SLEEP = 78677; // replace with real spell ID

                        if (me->HasAura(SPELL_SLEEP))
                        {
                            // Creature is sleeping so we remove the aura
                            me->RemoveAurasDueToSpell(SPELL_SLEEP);
                        }
                        me->SetStandState(UNIT_STAND_STATE_STAND);
                        Miam = 1000;
                        count++;
                        break;
                    }
                    case 1:
                    {
                        Eat();
                        Miam = 2000;
                        me->SetStandState(UNIT_STAND_STATE_SIT);
                        count++;
                        break;
                    }
                    case 2:
                    {
                        if (stew->ToTempSummon())
                            if (WorldObject* player = stew->ToTempSummon()->GetSummoner())
                                player->ToPlayer()->KilledMonsterCredit(42617);
                        Miam = 25000;
                        count++;
                        break;
                    }
                    default:
                        break;
                    }
                }
                else Miam = 3000;

                if (!me->HasAura(SPELL_FULL_BELLY))
                    me->SetStandState(UNIT_STAND_STATE_STAND);

                if (count == 3)
                    Reset();
            }
            else Miam -= diff;
        }
    };
};

enum eTower
{
    SPELL_POTION_SHROUDING = 79528,
    QUEST_SECRETS_OF_THE_TOWER = 26290,

    NPC_ELITE_TARGET = 42656,
    NPC_KEARNEN = 7024,
    SPELL_KILL_SHOT = 79526,

    EVENT_KILL_SHOT = 1,
    DATA_ASSIST_REQUEST = 1
};

class npc_shadowy_tower : public CreatureScript
{
public:
    npc_shadowy_tower() : CreatureScript("npc_shadowy_tower") {}

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_shadowy_towerAI(creature);
    }

    struct npc_shadowy_towerAI : public ScriptedAI
    {
        npc_shadowy_towerAI(Creature* creature) : ScriptedAI(creature) {}

        uint8 Phase;
        uint32 SummonTimer;
        ObjectGuid PlayerGUID;
        ObjectGuid Glubtok3GUID;
        ObjectGuid Shadowy3GUID;

        bool bSumm;
        bool bSumm1;
        bool bSumm2;
        bool bExit;

        void Reset() override
        {
            Phase = 0;
            SummonTimer = 2000;
            bSumm = false;
            bSumm1 = false;
            bSumm2 = false;
            bExit = false;
        }

        void MoveInLineOfSight(Unit* who) override
        {
            ScriptedAI::MoveInLineOfSight(who);

            if (who->GetTypeId() != TYPEID_PLAYER)
                return;
            //if (who->HasAura(79528))
            // return true;

            if (who->ToPlayer()->GetQuestStatus(QUEST_SECRETS_OF_THE_TOWER) == QUEST_STATUS_INCOMPLETE)
            {
                if (who->IsWithinDistInMap(me, 10.0f) && !bSumm)
                {
                    PlayerGUID = who->GetGUID();
                    StartEvent();
                }
            }
        }

        void StartEvent()
        {
            if (!bSumm)
            {
                if (!bSumm1)
                {
                    if (Creature* Shadowy3 = me->SummonCreature(42662, -11138.659f, 545.20f, 70.30f, 0.19f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                    {
                        Shadowy3GUID = Shadowy3->GetGUID();
                        Shadowy3->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
                        Shadowy3->SetWalk(true);
                        Shadowy3->GetMotionMaster()->MovePoint(0, -11131.710f, 546.810f, 70.380f);
                        bSumm1 = true;
                    }
                }

                if (!bSumm2)
                {
                    if (Creature* Glubtok3 = me->SummonCreature(42492, -11128.11f, 547.52f, 70.41f, 3.32f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                    {
                        Glubtok3GUID = Glubtok3->GetGUID();
                        Glubtok3->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
                        bSumm2 = true;
                    }
                }

                if (bSumm1 && bSumm2)
                {
                    bSumm = true;
                    SummonTimer = 2000;
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (SummonTimer < diff)
            {
                if (bSumm)
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                        if (Creature* Glubtok3 = ObjectAccessor::GetCreature(*me, Glubtok3GUID))
                            if (Creature* Shadowy3 = ObjectAccessor::GetCreature(*me, Shadowy3GUID))
                            {
                                switch (Phase)
                                {
                                case 0:
                                {
                                    Glubtok3->SetInFront(me);
                                    SummonTimer = 1000;
                                    Phase++;
                                    break;
                                }
                                case 1:
                                {
                                    Glubtok3->Say("The gnolls have failed, mistress.", LANG_UNIVERSAL);
                                    SummonTimer = 6000;
                                    Phase++;
                                    break;
                                }
                                case 2:
                                {
                                    Shadowy3->Say("They provided the distraction I required. We continue as planned.", LANG_UNIVERSAL);
                                    SummonTimer = 6000;
                                    Phase++;
                                    break;
                                }
                                case 3:
                                {
                                    Glubtok3->Say("But mistress, the admiral is sti...", LANG_UNIVERSAL);
                                    SummonTimer = 6000;
                                    Phase++;
                                    break;
                                }
                                case 4:
                                {
                                    Shadowy3->Say("We will free the admiral during the dawning.", LANG_UNIVERSAL);
                                    SummonTimer = 6000;
                                    Phase++;
                                    break;
                                }
                                case 5:
                                {
                                    Glubtok3->Say("Yes, mistress.", LANG_UNIVERSAL);
                                    SummonTimer = 6000;
                                    Phase++;
                                    break;
                                }
                                case 6:
                                {
                                    Shadowy3->Say("Judgment day is soon upon us, Helix.", LANG_UNIVERSAL);
                                    SummonTimer = 6000;
                                    Phase++;
                                    break;
                                }
                                case 7:
                                {
                                    Shadowy3->Say("Call for the people. I wish to speak to them one last time before the dawning.", LANG_UNIVERSAL);
                                    SummonTimer = 6000;
                                    Phase++;
                                    break;
                                }
                                case 8:
                                {
                                    Glubtok3->Say("Moonbrook, mistress?", LANG_UNIVERSAL);
                                    SummonTimer = 6000;
                                    Phase++;
                                    break;
                                }
                                case 9:
                                {
                                    Shadowy3->Say("Aye. Tonight.", LANG_UNIVERSAL);
                                    SummonTimer = 2000;
                                    Phase++;
                                    break;
                                }
                                case 10:
                                {
                                    player->CastSpell(player, 79534, true);
                                    SummonTimer = 1000;
                                    Phase++;
                                    break;
                                }
                                case 11:
                                {
                                    if (!bExit)
                                    {
                                        Shadowy3->CastSpell(Shadowy3, 64446, true);
                                        Shadowy3->DespawnOrUnsummon(1s);
                                        Glubtok3->CastSpell(Shadowy3, 64446, true);
                                        Glubtok3->DespawnOrUnsummon(1s);
                                        bExit = true;
                                    }
                                }
                                break;
                                default:
                                    break;
                                }
                            }
                }
            }
            else SummonTimer -= diff;
        }
    };
};

class npc_agent_kearnen : public CreatureScript
{
public:
    npc_agent_kearnen() : CreatureScript("npc_agent_kearnen") { }

    struct npc_agent_kearnenAI : public ScriptedAI
    {
        npc_agent_kearnenAI(Creature* creature) : ScriptedAI(creature) { }

        EventMap events;
        ObjectGuid PlayerGUID;
        ObjectGuid TargetGUID;

        void Reset() override
        {
            events.Reset();
            PlayerGUID.Clear();
            TargetGUID.Clear();
        }

        void Assist(Player* player, Creature* target)
        {
            if (!player || !target)
                return;

            // Allow multiple elites - overwrite previous safely
            PlayerGUID = player->GetGUID();
            TargetGUID = target->GetGUID();

            events.Reset();
            events.ScheduleEvent(EVENT_KILL_SHOT, 1s,3s);
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_KILL_SHOT:
                {
                    Creature* target = ObjectAccessor::GetCreature(*me, TargetGUID);
                    Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID);

                    if (target && target->IsAlive() && player)
                    {
                        me->SetFacingToObject(target);
                        me->CastSpell(target, SPELL_KILL_SHOT, true);
                        Talk(0, player);
                    }

                    TargetGUID.Clear();
                    break;
                }
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_agent_kearnenAI(creature);
    }
};

class npc_mortwake_tower_elite : public CreatureScript
{
public:
    npc_mortwake_tower_elite() : CreatureScript("npc_mortwake_tower_elite") { }

    struct npc_mortwake_tower_eliteAI : public ScriptedAI
    {
        npc_mortwake_tower_eliteAI(Creature* creature) : ScriptedAI(creature) { }

        bool AssistTriggered = false;

        void Reset() override
        {
            AssistTriggered = false;
        }

        void JustEngagedWith(Unit* who) override
        {
            if (AssistTriggered)
                return;

            if (!who)
            {
                TC_LOG_DEBUG("entities.unit", "We have no target");
                return;
            }

            Player* player = who->GetCharmerOrOwnerPlayerOrPlayerItself();
            if (!player)
            {
                TC_LOG_DEBUG("entities.unit", "We have no player");
                return;
            }

            if (player->GetQuestStatus(QUEST_SECRETS_OF_THE_TOWER) != QUEST_STATUS_INCOMPLETE)
            {
                TC_LOG_DEBUG("entities.unit", "We have no quest");
                return;
            }

            Creature* agent = me->FindNearestCreature(NPC_KEARNEN, 150.0f, true);
            if (!agent)
            {
                TC_LOG_DEBUG("entities.unit", "We have no agent");
                return;
            }

            TC_LOG_DEBUG("entities.unit", "We passed all checks");

            if (auto* ai = CAST_AI(npc_agent_kearnen::npc_agent_kearnenAI, agent->AI()))
            {
                ai->Assist(player, me);
                TC_LOG_DEBUG("entities.unit", "We passed the signal");
            }

            AssistTriggered = true;
            TC_LOG_DEBUG("entities.unit", "ASSIST TRIGGERED");
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_mortwake_tower_eliteAI(creature);
    }
};

void AddSC_westfall()
{
    using namespace Scripts::EasternKingdoms::Westfall;

    // Creature
    RegisterCreatureAI(npc_westfall_overloaded_harvest_golem);
    //RegisterCreatureAI(npc_westplains_drifter);
    new npc_westplains_drifter();
    new npc_lous_parting_thoughts_trigger();
    new npc_lous_parting_thoughts_thug();
    new npc_hungry_hobo();
    new npc_shadowy_tower();
    new npc_agent_kearnen();
    new npc_mortwake_tower_elite();

    // Spells
    RegisterSpellScript(spell_westfall_unbound_energy);
    RegisterSpellScript(spell_westfall_reaping_blows);
    RegisterSpellScript(spell_westfall_wake_harvest_golem);
    RegisterSpellScript(spell_westfall_summon_lous_house);
    RegisterSpellScript(spell_westfall_quest_credit_jangolode_event);
    RegisterSpellScript(spell_westfall_despawn_jangolode_actor);
    RegisterSpellScript(spell_westfall_livin_the_life_ping_glubtok);
    RegisterSpellScript(spell_westfall_livin_the_life_ping_figure);
}
