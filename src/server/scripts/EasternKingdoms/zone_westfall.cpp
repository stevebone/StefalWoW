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

void AddSC_westfall()
{
    using namespace Scripts::EasternKingdoms::Westfall;

    // Creature
    RegisterCreatureAI(npc_westfall_overloaded_harvest_golem);
    //RegisterCreatureAI(npc_westplains_drifter);
    new npc_westplains_drifter();

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
