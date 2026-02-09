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
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "PhasingHandler.h"
#include "SharedDefines.h"


namespace Scripts::EasternKingdoms::Westfall
{
namespace Creatures
{
    static constexpr uint32 EnergizedHarvestReaper = 42342;
    static constexpr uint32 OverloadedHarvestGolem = 42601;
    static constexpr uint32 JangolodeMineGlubtok = 42492;
    static constexpr uint32 JangolodeMineFigure = 42515;
    static constexpr uint32 HomelessStormwindCitizen1 = 42386;
    static constexpr uint32 HomelessStormwindCitizen2 = 42384;
    static constexpr uint32 WestPlainDrifter = 42391;
    static constexpr uint32 Transient = 42383;
}

namespace Quests
{
    static constexpr uint32 MurderWasTheCaseThatTheyGaveMe = 26209;
}

namespace Spells
{
    static constexpr uint32 JangolodeMineSummonFigure = 79265;
    static constexpr uint32 JangolodeMineSummonGlubtok = 79263;
    static constexpr uint32 HoboInformation1 = 79181;
    static constexpr uint32 HoboInformation2 = 79182;
    static constexpr uint32 HoboInformation3 = 79183;
    static constexpr uint32 HoboInformation4 = 79184;
    static constexpr uint32 SummonRagamuffinLooter = 79169;
    static constexpr uint32 SummonRagamuffinLooter1 = 79170;
    static constexpr uint32 SummonRagamuffinLooter2 = 79171;
    static constexpr uint32 SummonRagamuffinLooter3 = 79172;
    static constexpr uint32 SummonRagamuffinLooter4 = 79173;
    static constexpr uint32 HoboAggro = 79168;
}

namespace Gossip
{
    namespace GossipOption
    {
        static constexpr uint32 GossipOption0 = 0;
        static constexpr uint32 GossipOption1 = 1;
    }
}

namespace Events
{
    namespace ItsAlive
    {
        static constexpr uint32 CheckArea = 1;
        static constexpr uint32 DespawnHarvester = 2;
    }

    namespace MurderWasTheCaseThatTheyGaveMe
    {
        static constexpr uint32 HoboTalk = 1;
        static constexpr uint32 JackpotIntro = 2;
        static constexpr uint32 HoboCry = 3;
        static constexpr uint32 PropertyRage = 4;
        static constexpr uint32 JackpotMiddle = 5;
        static constexpr uint32 JackpotEnd = 6;
        static constexpr uint32 ResumeMove = 7;
        static constexpr uint32 GroupOOC = 1;
    }
}

namespace Actions
{
    namespace MurderWasTheCaseThatTheyGaveMe
    {
        static constexpr uint32 HoboAggroAction = 1;
        static constexpr uint32 HoboAggroActionDone = 2;
    }
}

namespace Text
{
    namespace HarvestGolem
    {
        static constexpr uint32 AnnounceOutOfArea = 0;
    }

    namespace HoboText
    {
        static constexpr uint32 HoboSayClue1 = 0;
        static constexpr uint32 HoboSayClue2 = 1;
        static constexpr uint32 HoboSayClue3 = 2;
        static constexpr uint32 HoboSayClue4 = 3;
        static constexpr uint32 HoboAggroBribe = 4;
        static constexpr uint32 HoboAggroConvince = 5;
        static constexpr uint32 HoboEvent = 6;
        static constexpr uint32 HoboJackpotIntro = 7;
        static constexpr uint32 HoboJackpotEnd = 8;
        static constexpr uint32 HoboPropertyRage = 9;
        static constexpr uint32 HoboFlee = 10;
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

// 42386 - Homeless Stormwind Citizen
// 42384 - Homeless Stormwind Citizen
// 42384 - West Plain Drifter
// 42383 - Transient
struct npc_westfall_hobo_witness : public ScriptedAI
{
    npc_westfall_hobo_witness(Creature* creature) : ScriptedAI(creature), _bribeFailed(false), _hoboRage(false), _flee(false) {}

    struct HoboClueData
    {
        uint32 SpellID;
        uint32 TextID;
    };

    static constexpr std::array<HoboClueData, 4> HoboClues =
    { {
        { Spells::HoboInformation1, Text::HoboText::HoboSayClue1 },
        { Spells::HoboInformation2, Text::HoboText::HoboSayClue2 },
        { Spells::HoboInformation3, Text::HoboText::HoboSayClue3 },
        { Spells::HoboInformation4, Text::HoboText::HoboSayClue4 },
    } };

    bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
    {
        uint8 clueGainChance = 0;
        switch (gossipListId)
        {
            case Gossip::GossipOption::GossipOption0:
            {
                clueGainChance = 25;
                break;
            }
            case Gossip::GossipOption::GossipOption1:
            {
                clueGainChance = 75;
                _bribeFailed = true;
                break;
            }
            default:
                return true;
        }

        me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);
        CloseGossipMenuFor(player);
        if (roll_chance_i(clueGainChance))
            GiveClue(player);
        else
        {
            me->SetImmuneToPC(false);
            AttackStart(player);
        }

        return false;
    }

    void GiveClue(Player* player)
    {
        _events.CancelEventGroup(Events::MurderWasTheCaseThatTheyGaveMe::GroupOOC);
        me->SetFacingToObject(player);

        uint16 slot = player->FindQuestSlot(Quests::MurderWasTheCaseThatTheyGaveMe);

        for (uint8 i = 0; i < HoboClues.size(); i++)
        {
            if (player->GetQuestSlotCounter(slot, i) == 0)
            {
                HoboClueData const& data = HoboClues[i];

                player->CastSpell(player, data.SpellID);
                Talk(data.TextID, player);

                me->DespawnOrUnsummon(12s);
                return;
            }
        }
    }

    void JustEngagedWith(Unit* who) override
    {
        me->SetAIAnimKitId(0);
        _events.CancelEventGroup(Events::MurderWasTheCaseThatTheyGaveMe::GroupOOC);

        if (!who->IsPlayer())
            return;

        Talk(_bribeFailed ? Text::HoboText::HoboAggroBribe : Text::HoboText::HoboAggroConvince, who);
    }

    void DamageTaken(Unit* /*killer*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (!_flee && me->HealthBelowPctDamaged(20, damage))
        {
            _flee = true;
            me->DoFleeToGetAssistance();
            Talk(Text::HoboText::HoboFlee);
        }
    }

    void JustDied(Unit* who) override
    {
        if (Creature* creature = who->ToCreature())
            creature->AI()->DoAction(Actions::MurderWasTheCaseThatTheyGaveMe::HoboAggroActionDone);
        DoCastSelf(Spells::SummonRagamuffinLooter);
    }

    void Reset() override
    {
        ScriptedAI::Reset();
        _events.Reset();
        _bribeFailed = false;
        _hoboRage = false;
        _flee = false;
        me->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
        me->SetImmuneToPC(true);
        _events.ScheduleEvent(Events::MurderWasTheCaseThatTheyGaveMe::ResumeMove, 100ms, Events::MurderWasTheCaseThatTheyGaveMe::GroupOOC);
    }

    void DoAction(int32 action) override
    {
        switch (action)
        {
            case Actions::MurderWasTheCaseThatTheyGaveMe::HoboAggroAction:
                if (Unit* target = ObjectAccessor::GetUnit(*me, _targetGUID))
                {
                    _hoboRage = true;
                    me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);
                    me->PauseMovement();
                    me->SetFacingToObject(target);
                    AttackStart(target);
                }
                break;
            case Actions::MurderWasTheCaseThatTheyGaveMe::HoboAggroActionDone:
                _hoboRage = false;
                break;
            default:
                return;
        }
    }

    void SetGUID(ObjectGuid const& guid, int32 /*id*/) override
    {
        _targetGUID = guid;
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);
        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case Events::MurderWasTheCaseThatTheyGaveMe::HoboTalk:
                    me->PauseMovement();
                    Talk(Text::HoboText::HoboEvent);
                    _events.ScheduleEvent(Events::MurderWasTheCaseThatTheyGaveMe::ResumeMove, 6s, Events::MurderWasTheCaseThatTheyGaveMe::GroupOOC);
                    break;
                case Events::MurderWasTheCaseThatTheyGaveMe::JackpotIntro:
                    me->PauseMovement();
                    Talk(Text::HoboText::HoboJackpotIntro);
                    _events.ScheduleEvent(Events::MurderWasTheCaseThatTheyGaveMe::JackpotMiddle, 2s + 500ms, Events::MurderWasTheCaseThatTheyGaveMe::GroupOOC);
                    break;
                case Events::MurderWasTheCaseThatTheyGaveMe::JackpotMiddle:
                    me->SetAIAnimKitId(648);
                    Talk(Text::HoboText::HoboJackpotEnd);
                    _events.ScheduleEvent(Events::MurderWasTheCaseThatTheyGaveMe::JackpotEnd, 6s, Events::MurderWasTheCaseThatTheyGaveMe::GroupOOC);
                    break;
                case Events::MurderWasTheCaseThatTheyGaveMe::JackpotEnd:
                    me->SetAIAnimKitId(0);
                    _events.ScheduleEvent(Events::MurderWasTheCaseThatTheyGaveMe::ResumeMove, 4s, Events::MurderWasTheCaseThatTheyGaveMe::GroupOOC);
                    break;
                case Events::MurderWasTheCaseThatTheyGaveMe::HoboCry:
                    me->HandleEmoteCommand(EMOTE_ONESHOT_CRY);
                    me->PauseMovement();
                    _events.ScheduleEvent(Events::MurderWasTheCaseThatTheyGaveMe::ResumeMove, 2s, Events::MurderWasTheCaseThatTheyGaveMe::GroupOOC);
                    break;
                case Events::MurderWasTheCaseThatTheyGaveMe::PropertyRage:
                {
                    uint32 creatureId = RAND(Creatures::HomelessStormwindCitizen1, Creatures::HomelessStormwindCitizen2, Creatures::Transient, Creatures::WestPlainDrifter);
                    if (Creature* creature = GetClosestCreatureWithEntry(me, creatureId, 25.0f))
                    {
                        if (!creature->IsAlive() || creature->IsInCombat())
                        {
                            _events.ScheduleEvent(Events::MurderWasTheCaseThatTheyGaveMe::ResumeMove, 100ms, Events::MurderWasTheCaseThatTheyGaveMe::GroupOOC);
                            return;
                        }
                        _hoboRage = true;
                        Talk(Text::HoboText::HoboPropertyRage);
                        me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);
                        me->PauseMovement();
                        me->CastSpell(creature, Spells::HoboAggro, true);
                        me->SetFacingToObject(creature);
                    }
                    else
                        _events.ScheduleEvent(Events::MurderWasTheCaseThatTheyGaveMe::ResumeMove, 100ms, Events::MurderWasTheCaseThatTheyGaveMe::GroupOOC);
                    break;
                }
                case Events::MurderWasTheCaseThatTheyGaveMe::ResumeMove:
                    me->ResumeMovement();
                    if (roll_chance_i(50)) // prevent spam
                    {
                        if (roll_chance_i(70)) // chance to trigger simple talk event
                            _events.ScheduleEvent(Events::MurderWasTheCaseThatTheyGaveMe::HoboTalk, 30s, 80s, Events::MurderWasTheCaseThatTheyGaveMe::GroupOOC);
                        else
                            _events.ScheduleEvent(urand(Events::MurderWasTheCaseThatTheyGaveMe::JackpotIntro, Events::MurderWasTheCaseThatTheyGaveMe::PropertyRage), 30s, 80s, Events::MurderWasTheCaseThatTheyGaveMe::GroupOOC);
                    }
                    else
                        _events.Repeat(30s, 80s);
                    break;
                default:
                    break;
            }
        }

        if (!_hoboRage)
            if (!UpdateVictim())
                return;
    }

private:
    EventMap _events;
    ObjectGuid _targetGUID;
    bool _bribeFailed;
    bool _hoboRage;
    bool _flee;
};

// 79169 - Summon Ragamuffin Looter
class spell_westfall_summon_ragamuffin_looter : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo(
            {
                Spells::SummonRagamuffinLooter1,
                Spells::SummonRagamuffinLooter2,
                Spells::SummonRagamuffinLooter3,
                Spells::SummonRagamuffinLooter4
            });
    }

    void HandleScript(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        caster->CastSpell(caster, Spells::SummonRagamuffinLooter1, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
        caster->CastSpell(caster, Spells::SummonRagamuffinLooter2, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
        caster->CastSpell(caster, Spells::SummonRagamuffinLooter3, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
        caster->CastSpell(caster, Spells::SummonRagamuffinLooter4, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
    }

    void Register() override
    {
        OnEffectLaunch += SpellEffectFn(spell_westfall_summon_ragamuffin_looter::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// 79168 - Hobo Aggro
class spell_westfall_aggro_hobo : public SpellScript
{
    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        if (Creature* creature = GetHitCreature())
        {
            creature->AI()->SetGUID(GetCaster()->GetGUID(), 0);
            creature->AI()->DoAction(Actions::MurderWasTheCaseThatTheyGaveMe::HoboAggroAction);
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_westfall_aggro_hobo::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
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
            events.ScheduleEvent(EVENT_KILL_SHOT, 1s,2s);
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
                //TC_LOG_DEBUG("entities.unit", "We have no target");
                return;
            }

            Player* player = who->GetCharmerOrOwnerPlayerOrPlayerItself();
            if (!player)
            {
                //TC_LOG_DEBUG("entities.unit", "We have no player");
                return;
            }

            if (player->GetQuestStatus(QUEST_SECRETS_OF_THE_TOWER) != QUEST_STATUS_INCOMPLETE)
            {
                //TC_LOG_DEBUG("entities.unit", "We have no quest");
                return;
            }

            Creature* agent = me->FindNearestCreature(NPC_KEARNEN, 150.0f, true);
            if (!agent)
            {
                //TC_LOG_DEBUG("entities.unit", "We have no agent");
                return;
            }

            //TC_LOG_DEBUG("entities.unit", "We passed all checks");

            if (auto* ai = CAST_AI(npc_agent_kearnen::npc_agent_kearnenAI, agent->AI()))
            {
                ai->Assist(player, me);
                //TC_LOG_DEBUG("entities.unit", "We passed the signal");
            }

            AssistTriggered = true;
            //TC_LOG_DEBUG("entities.unit", "ASSIST TRIGGERED");
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_mortwake_tower_eliteAI(creature);
    }
};

enum eShadowy2
{
    QUEST_THE_DAWNING_OF_NEW_DAY = 26297,
    QUEST_A_VISION_OF_THE_PAST = 26320,
    NPC_SHADOWY2 = 42680,
    NPC_TRIGGER2 = 43515,
    NPC_LISTENER = 42383
};

class npc_shadowy_trigger : public CreatureScript
{
public:
    npc_shadowy_trigger() : CreatureScript("npc_shadowy_trigger") {}

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_shadowy_triggerAI(creature);
    }

    struct npc_shadowy_triggerAI : public ScriptedAI
    {
        npc_shadowy_triggerAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 SummonTimer;
        ObjectGuid PlayerGUID;
        ObjectGuid Shadowy2GUID;
        ObjectGuid Witness1GUID;
        ObjectGuid Witness2GUID;
        ObjectGuid Witness3GUID;
        ObjectGuid Witness4GUID;
        ObjectGuid Witness5GUID;
        ObjectGuid Witness6GUID;
        ObjectGuid Witness7GUID;
        ObjectGuid Witness8GUID;
        ObjectGuid Witness9GUID;
        ObjectGuid Witness10GUID;
        ObjectGuid Witness11GUID;
        ObjectGuid Witness12GUID;
        ObjectGuid Witness13GUID;
        ObjectGuid Witness14GUID;
        ObjectGuid Witness15GUID;
        ObjectGuid Witness16GUID;
        ObjectGuid Witness17GUID;
        ObjectGuid Witness18GUID;
        ObjectGuid Witness19GUID;
        ObjectGuid Witness20GUID;
        ObjectGuid Witness21GUID;
        ObjectGuid Witness22GUID;
        ObjectGuid Witness23GUID;
        ObjectGuid Witness24GUID;
        ObjectGuid Witness25GUID;
        ObjectGuid Witness26GUID;
        ObjectGuid Witness27GUID;

        uint8 Phase;

        bool bSummoned;
        bool bSummoned1;
        bool bText;

        void Reset() override
        {
            Phase = 0;
            bSummoned = false;
            bSummoned1 = false;
            bText = false;
            SummonTimer = 2000;
        }

        void MoveInLineOfSight(Unit* who) override
        {
            ScriptedAI::MoveInLineOfSight(who);

            if (who->GetTypeId() != TYPEID_PLAYER)
                return;

            if (who->ToPlayer()->GetQuestStatus(QUEST_THE_DAWNING_OF_NEW_DAY) == QUEST_STATUS_INCOMPLETE)
            {
                if (who->IsWithinDistInMap(me, 20.0f) && !bSummoned)
                {
                    PlayerGUID = who->GetGUID();
                    StartSpeech();
                }
            }
            else
                if (who->ToPlayer()->GetQuestStatus(QUEST_A_VISION_OF_THE_PAST) == QUEST_STATUS_INCOMPLETE)
                {
                    if (who->IsWithinDistInMap(me, 2.0f) && !bSummoned)
                    {
                        PlayerGUID = who->GetGUID();
                        me->TextEmote("Follow the trail of homeless to the Deadmines dungeon entrance.", NULL, true);
                    }
                }
        }

        void StartSpeech()
        {
            if (!bSummoned)
            {
                if (Creature* Shadowy2 = me->SummonCreature(NPC_SHADOWY2, -11016.31f, 1478.82f, 47.80f, 2.016f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                {
                    Shadowy2GUID = Shadowy2->GetGUID();
                    bSummoned = true;
                    SummonTimer = 2000;
                    SummonCrowd();
                }
            }
        }

        void SummonCrowd()
        {
            if (!bSummoned1)
            {
                if (Creature* Witness1 = me->SummonCreature(NPC_LISTENER, -11009.036f, 1490.47f, 43.58f, 4.16f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                    if (Creature* Witness2 = me->SummonCreature(NPC_LISTENER, -11010.76f, 1488.21f, 43.57f, 4.33f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                        if (Creature* Witness3 = me->SummonCreature(NPC_LISTENER, -11017.39f, 1491.76f, 43.19f, 4.78f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                            if (Creature* Witness4 = me->SummonCreature(NPC_LISTENER, -11021.73f, 1493.054f, 43.184f, 5.09f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                if (Creature* Witness5 = me->SummonCreature(NPC_LISTENER, -11025.74f, 1487.70f, 43.17f, 5.45f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                    if (Creature* Witness6 = me->SummonCreature(NPC_LISTENER, -11025.25f, 1482.23f, 43.03f, 6.04f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                        if (Creature* Witness7 = me->SummonCreature(NPC_LISTENER, -11029.68f, 1481.255f, 43.185f, 6.20f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                            if (Creature* Witness8 = me->SummonCreature(NPC_LISTENER, -11024.46f, 1473.88f, 43.02f, 0.43f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                if (Creature* Witness9 = me->SummonCreature(NPC_LISTENER, -11019.49f, 1471.70f, 43.21f, 1.09f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                    if (Creature* Witness10 = me->SummonCreature(NPC_LISTENER, -11021.56f, 1497.053f, 43.20f, 5.00f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                        if (Creature* Witness11 = me->SummonCreature(NPC_LISTENER, -11012.29f, 1488.14f, 43.77f, 4.16f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                            if (Creature* Witness12 = me->SummonCreature(NPC_LISTENER, -11014.67f, 1493.14f, 43.23f, 4.60f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                                if (Creature* Witness13 = me->SummonCreature(NPC_LISTENER, -11019.8f, 1494.25f, 43.2f, 4.77f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                                    if (Creature* Witness14 = me->SummonCreature(NPC_LISTENER, -11023.6f, 1489.35f, 43.17f, 4.77f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                                        if (Creature* Witness15 = me->SummonCreature(NPC_LISTENER, -11023.1f, 1482.51f, 43.07f, 6.25f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                                            if (Creature* Witness16 = me->SummonCreature(NPC_LISTENER, -11027.2f, 1494.37f, 43.17f, 5.15f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                                                if (Creature* Witness17 = me->SummonCreature(NPC_LISTENER, -11029.6f, 1488.29f, 43.19f, 5.64f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                                                    if (Creature* Witness18 = me->SummonCreature(NPC_LISTENER, -11030.8f, 1485.13f, 43.31f, 5.94f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                                                        if (Creature* Witness19 = me->SummonCreature(NPC_LISTENER, -11026.2f, 1478.62f, 42.94f, 6.17f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                                                            if (Creature* Witness20 = me->SummonCreature(NPC_LISTENER, -11013.2f, 1497.81f, 43.31f, 4.44f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                                                                if (Creature* Witness21 = me->SummonCreature(NPC_LISTENER, -11011.6f, 1492.61f, 43.39f, 4.44f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                                                                    if (Creature* Witness22 = me->SummonCreature(NPC_LISTENER, -11015.4f, 1489.06f, 43.28f, 4.73f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                                                                        if (Creature* Witness23 = me->SummonCreature(NPC_LISTENER, -11020.2f, 1490.15f, 43.19f, 5.07f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                                                                            if (Creature* Witness24 = me->SummonCreature(NPC_LISTENER, -11009.9f, 1483.52f, 44.06f, 3.88f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                                                                                if (Creature* Witness25 = me->SummonCreature(NPC_LISTENER, -11020.1f, 1484.87f, 43.18f, 5.33f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                                                                                    if (Creature* Witness26 = me->SummonCreature(NPC_LISTENER, -11017.7f, 1487.42f, 43.24f, 4.89f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                                                                                        if (Creature* Witness27 = me->SummonCreature(NPC_LISTENER, -11017.9f, 1498.24f, 43.20f, 5.04f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s))
                                                                                                                        {
                                                                                                                            Witness1GUID = Witness1->GetGUID();
                                                                                                                            Witness2GUID = Witness2->GetGUID();
                                                                                                                            Witness3GUID = Witness3->GetGUID();
                                                                                                                            Witness4GUID = Witness4->GetGUID();
                                                                                                                            Witness5GUID = Witness5->GetGUID();
                                                                                                                            Witness6GUID = Witness6->GetGUID();
                                                                                                                            Witness7GUID = Witness7->GetGUID();
                                                                                                                            Witness8GUID = Witness8->GetGUID();
                                                                                                                            Witness9GUID = Witness9->GetGUID();
                                                                                                                            Witness10GUID = Witness10->GetGUID();
                                                                                                                            Witness11GUID = Witness11->GetGUID();
                                                                                                                            Witness12GUID = Witness12->GetGUID();
                                                                                                                            Witness13GUID = Witness13->GetGUID();
                                                                                                                            Witness14GUID = Witness14->GetGUID();
                                                                                                                            Witness15GUID = Witness15->GetGUID();
                                                                                                                            Witness16GUID = Witness16->GetGUID();
                                                                                                                            Witness17GUID = Witness17->GetGUID();
                                                                                                                            Witness18GUID = Witness18->GetGUID();
                                                                                                                            Witness19GUID = Witness19->GetGUID();
                                                                                                                            Witness20GUID = Witness20->GetGUID();
                                                                                                                            Witness21GUID = Witness21->GetGUID();
                                                                                                                            Witness22GUID = Witness22->GetGUID();
                                                                                                                            Witness23GUID = Witness23->GetGUID();
                                                                                                                            Witness24GUID = Witness24->GetGUID();
                                                                                                                            Witness25GUID = Witness25->GetGUID();
                                                                                                                            Witness26GUID = Witness26->GetGUID();
                                                                                                                            Witness27GUID = Witness27->GetGUID();

                                                                                                                            bSummoned1 = true;
                                                                                                                        }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (SummonTimer < diff)
            {
                if (bSummoned)
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                        if (Creature* Shadowy2 = ObjectAccessor::GetCreature(*me, Shadowy2GUID))
                        {
                            switch (Phase)
                            {
                            case 0:
                            {
                                me->TextEmote("The rally is about to begin!", NULL, true);
                                SummonTimer = 1500;
                                Phase++;
                                break;
                            }
                            case 1:
                            {
                                Shadowy2->Yell("Gather, brothers and sisters! Come, all, and listen!", LANG_UNIVERSAL);
                                SummonTimer = 3000;
                                Phase++;
                                break;
                            }
                            case 2:
                            {
                                Shadowy2->Yell("Brothers. Sisters. We are ABANDONED - the orphaned children of Stormwind.", LANG_UNIVERSAL);
                                SummonTimer = 5500;
                                Phase++;
                                break;
                            }
                            case 3:
                            {
                                Shadowy2->Yell("Our 'king' sits atop his throne made of gold and shrugs at our plight!", LANG_UNIVERSAL);
                                SummonTimer = 4000;
                                Phase++;
                                break;
                            }
                            case 4:
                            {
                                Shadowy2->Yell("Meanwhile, our children die of starvation on these very streets!", LANG_UNIVERSAL);
                                SummonTimer = 4500;
                                Phase++;
                                break;
                            }
                            case 5:
                            {
                                Shadowy2->Yell("HIS war, not ours, cost us our livelihood. WE paid for the Alliance's victories with our blood and the blood of our loved ones!", LANG_UNIVERSAL);
                                SummonTimer = 4500;
                                Phase++;
                                break;
                            }
                            case 6:
                            {
                                Shadowy2->Yell("The time has come, brothers and sisters, to stop this injustice!", LANG_UNIVERSAL);
                                SummonTimer = 4500;
                                Phase++;
                                break;
                            }
                            case 7:
                            {
                                Shadowy2->Yell("The government of Stormwind, of the ALLIANCE, must be made accountable for what it has done to us!", LANG_UNIVERSAL); SummonTimer = 4500;
                                Phase++;
                                break;
                            }
                            case 8:
                            {
                                Shadowy2->Yell("Today, we are reborn! Today, we take a stand as men and women, not nameless, faceless numbers!", LANG_UNIVERSAL);
                                SummonTimer = 4500;
                                Phase++;
                                break;
                            }
                            case 9:
                            {
                                if (Creature* listener = me->FindNearestCreature(NPC_LISTENER, 35.0f, true))
                                {
                                    if (!bText)
                                    {
                                        me->TextEmote("<homeless people applaud and cheer>", NULL, true);
                                        listener->TextEmote(TEXT_EMOTE_APPLAUD, ObjectAccessor::GetPlayer(*me, PlayerGUID), false);
                                        bText = true;
                                    }
                                    listener->HandleEmoteCommand(EMOTE_ONESHOT_APPLAUD);
                                    player->KilledMonsterCredit(42680, PlayerGUID);
                                    Shadowy2->CastSpell(me, 64446, true);
                                    Shadowy2->DespawnOrUnsummon(1s);
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

enum Ripsnarl
{
    AURA_IN_STOCKS = 69196,
};

//42635
struct npc_ripsnarl : public ScriptedAI
{
    npc_ripsnarl(Creature* c) : ScriptedAI(c) { }

    void Reset() override
    {
        ScriptedAI::Reset();
        me->SetFaction(7);
        me->SetReactState(REACT_PASSIVE);
        me->AddAura(AURA_IN_STOCKS, me);
        me->SetUnitFlag(UnitFlags(UNIT_FLAG_NON_ATTACKABLE));
    }
};

enum eEnd
{
    SPELL_TRANSFORM_HUMAN = 79745,
    SPELL_ADMIRAL_HAT = 79750,
    SPELL_TRANSFORM_VANEESA = 79709,
    SPELL_SUMMON_BLACKGUARD = 79712,
    SPELL_TIED_UP_GOOD_GUYS = 79723,
    SPELL_TIED_UP_VISUAL = 79724,
    SPELL_DEFIAS_FINALE_EVENT = 79758,
    SPELL_SUMMON_GLUBTOK = 79762,
    SPELL_SUMMON_HELIX = 79763,
    SPELL_TOSS_TORCH = 79778,
    SPELL_TOSS_TORCH_TRIGGER = 79779,
    SPELL_SMOKE = 67690,
    SPELL_HIT_ME = 65600,

    NPC_HOPE = 42575,
    NPC_GRYAN = 42750,
    NPC_RIPSNARL = 42635, // 42748
    NPC_INVESTIGATOR = 42745,
    NPC_INVESTIGATOR3 = 46614,
    NPC_THIEF = 42769,
    NPC_GUARD = 878,
    NPC_HORATIO2 = 42744,
    NPC_DANUVIN = 821,
    NPC_GRYAN_INIT = 234,

    QUEST_RISE_OF_THE_BROTHERHOOD = 26322,

    DATA_START_EVENT = 2,
    DATA_PLAYER_GUID = 1
};

class player_rise_of_the_brotherhood : public PlayerScript
{
public:
    player_rise_of_the_brotherhood() : PlayerScript("player_rise_of_the_brotherhood") { }

    void OnQuestStatusChange(Player* player, uint32 questId) override
    {
        if (questId != QUEST_RISE_OF_THE_BROTHERHOOD)
            return;

        QuestStatus status = player->GetQuestStatus(QUEST_RISE_OF_THE_BROTHERHOOD);

        // Only react when quest is newly accepted
        if (status != QUEST_STATUS_INCOMPLETE)
            return;

        //TC_LOG_DEBUG("scripts.ai.core", "RISE BR Quest status incomplete detected");

        // Find the NPC that runs the event
        if (Creature* riseNpc = player->FindNearestCreature(NPC_GRYAN_INIT, 50.0f, true))
        {
            //TC_LOG_DEBUG("scripts.ai.core", "RISE BR We found Gryan");
            if (CreatureAI* ai = riseNpc->AI())
            {
                ai->SetGUID(player->GetGUID(), DATA_PLAYER_GUID);
                ai->SetData(DATA_START_EVENT, 0);
                //TC_LOG_DEBUG("scripts.ai.core", "RISE BR We sent signal");
                player->RemoveAurasDueToSpell(79341); // remove detect quest invis 2 so the murder scene of Lou is no longer visible
            }
        }
    }
};

class npc_rise_br : public CreatureScript
{
public:
    npc_rise_br() : CreatureScript("npc_rise_br") {}

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_rise_brAI(creature);
    }

    struct npc_rise_brAI : public ScriptedAI
    {
        npc_rise_brAI(Creature* creature) : ScriptedAI(creature) {}

        ObjectGuid PlayerGUID;
        ObjectGuid Glubok4GUID;
        ObjectGuid Helix2GUID;
        ObjectGuid Thief1GUID;
        ObjectGuid Thief2GUID;
        ObjectGuid Thief3GUID;
        ObjectGuid Thief4GUID;
        ObjectGuid Thief5GUID;
        ObjectGuid Thief6GUID;
        ObjectGuid Thief7GUID;
        ObjectGuid Thief8GUID;
        ObjectGuid Thief9GUID;
        ObjectGuid Thief10GUID;
        ObjectGuid Fire1GUID;
        ObjectGuid Fire2GUID;
        ObjectGuid Fire3GUID;
        ObjectGuid Fire4GUID;
        ObjectGuid Fire5GUID;
        ObjectGuid Fire6GUID;
        ObjectGuid Fire7GUID;
        ObjectGuid Fire8GUID;
        ObjectGuid Fire9GUID;
        ObjectGuid Fire10GUID;
        ObjectGuid Fire11GUID;
        ObjectGuid Fire12GUID;
        ObjectGuid Fire13GUID;

        uint32 EventTimer;

        uint8 Phase;

        char playername;

        bool EventStarted;

        void SetGUID(ObjectGuid const& guid, int32 id) override
        {
            if (id == DATA_PLAYER_GUID)
                PlayerGUID = guid;
        }

        void SetData(uint32 id, uint32 /*value*/) override
        {
            if (id == DATA_START_EVENT && !EventStarted)
            {
                //TC_LOG_DEBUG("scripts.ai", "RISE BR: DATA_START_EVENT received");
                EventStarted = true;
                Phase = 0;
                EventTimer = 2000;
            }
        }

        void Reset() override
        {
            if (EventStarted)
                return;

            Phase = 0;
            EventTimer = 2000;
            me->RemoveAllAuras();
        }

        void SummonBrotherHood()
        {
            if (Creature* Thief1 = me->SummonCreature(NPC_THIEF, -10500.37f, 1042.65f, 60.51f, 3.06f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 100s))
                if (Creature* Thief2 = me->SummonCreature(NPC_THIEF, -10500.99f, 1046.73f, 60.517f, 3.29f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 100s))
                    if (Creature* Thief3 = me->SummonCreature(NPC_THIEF, -10505.202f, 1040.46f, 60.51f, 1.88f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 100s))
                        if (Creature* Thief4 = me->SummonCreature(NPC_THIEF, -10507.89f, 1039.52f, 60.51f, 1.95f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 100s))
                            if (Creature* Thief5 = me->SummonCreature(NPC_THIEF, -10513.54f, 1038.66f, 60.51f, 0.51f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 100s))
                                if (Creature* Thief6 = me->SummonCreature(NPC_THIEF, -10514.930f, 1042.012f, 60.51f, 0.399f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 100s))
                                    if (Creature* Thief7 = me->SummonCreature(NPC_THIEF, -10516.797f, 1048.61f, 59.95f, 5.32f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 100s))
                                        if (Creature* Thief8 = me->SummonCreature(NPC_THIEF, -10514.032f, 1049.80f, 59.92f, 5.11f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 100s))
                                            if (Creature* Thief9 = me->SummonCreature(NPC_THIEF, -10509.012f, 1051.92f, 59.85f, 5.11f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 100s))
                                                if (Creature* Thief10 = me->SummonCreature(NPC_THIEF, -10504.77f, 1053.57f, 59.86f, 4.84f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 100s))
                                                    if (Creature* eGlub = me->SummonCreature(42755, -10506.12f, 1053.13f, 59.10f, 4.92f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120s))
                                                        if (Creature* eHelix = me->SummonCreature(42753, -10509.368f, 1057.28f, 57.86f, 4.95f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120s))
                                                        {
                                                            Thief1->SetReactState(REACT_PASSIVE);
                                                            Thief2->SetReactState(REACT_PASSIVE);
                                                            Thief3->SetReactState(REACT_PASSIVE);
                                                            Thief4->SetReactState(REACT_PASSIVE);
                                                            Thief5->SetReactState(REACT_PASSIVE);
                                                            Thief6->SetReactState(REACT_PASSIVE);
                                                            Thief7->SetReactState(REACT_PASSIVE);
                                                            Thief8->SetReactState(REACT_PASSIVE);
                                                            Thief9->SetReactState(REACT_PASSIVE);
                                                            Thief10->SetReactState(REACT_PASSIVE);
                                                            eGlub->SetReactState(REACT_PASSIVE);
                                                            eHelix->SetReactState(REACT_PASSIVE);
                                                            Thief1GUID = Thief1->GetGUID();
                                                            Thief2GUID = Thief2->GetGUID();
                                                            Thief3GUID = Thief3->GetGUID();
                                                            Thief4GUID = Thief4->GetGUID();
                                                            Thief5GUID = Thief5->GetGUID();
                                                            Thief6GUID = Thief6->GetGUID();
                                                            Thief7GUID = Thief7->GetGUID();
                                                            Thief8GUID = Thief8->GetGUID();
                                                            Thief9GUID = Thief9->GetGUID();
                                                            Thief10GUID = Thief10->GetGUID();
                                                        }
        }

        void RemoveStun()
        {
            if (Creature* Horatio = me->FindNearestCreature(NPC_HORATIO2, 50.0f, true))
                if (Creature* Danuvin = me->FindNearestCreature(NPC_DANUVIN, 50.0f, true))
                    if (Creature* Guard = me->FindNearestCreature(NPC_GUARD, 50.0f, true))
                        if (Creature* Investigator = me->FindNearestCreature(NPC_INVESTIGATOR, 50.0f, true))
                            if (Creature* Investigator2 = me->FindNearestCreature(NPC_INVESTIGATOR3, 50.0f, true))
                            {
                                Horatio->RemoveAurasDueToSpell(79724);
                                Danuvin->RemoveAurasDueToSpell(79724);
                                Guard->RemoveAurasDueToSpell(79724);
                                Investigator->RemoveAurasDueToSpell(79724);
                                Investigator2->RemoveAurasDueToSpell(79724);
                            }
            me->RemoveAurasDueToSpell(79724);
        }

        void DoStun()
        {
            if (Creature* Horatio = me->FindNearestCreature(NPC_HORATIO2, 50.0f, true))
                if (Creature* Danuvin = me->FindNearestCreature(NPC_DANUVIN, 50.0f, true))
                    if (Creature* Guard = me->FindNearestCreature(NPC_GUARD, 50.0f, true))
                        if (Creature* Investigator = me->FindNearestCreature(NPC_INVESTIGATOR, 50.0f, true))
                            if (Creature* Investigator2 = me->FindNearestCreature(NPC_INVESTIGATOR3, 50.0f, true))
                            {
                                Horatio->CastSpell(Horatio, 79724, true);
                                Danuvin->CastSpell(Danuvin, 79724, true);
                                Guard->CastSpell(Guard, 79724, true);
                                Investigator->CastSpell(Investigator, 79724, true);
                                Investigator2->CastSpell(Investigator2, 79724, true);
                            }
        }

        void SummonFireTrigger()
        {
            if (Creature* Fire1 = me->SummonCreature(45937, -10507.5f, 1050.6f, 60.5189f, 3.06f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 50s))
                if (Creature* Fire2 = me->SummonCreature(45937, -10502.0f, 1051.81f, 67.7637f, 3.29f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 50s))
                    if (Creature* Fire3 = me->SummonCreature(45937, -10504.9f, 1038.74f, 68.2617f, 1.88f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 50s))
                        if (Creature* Fire4 = me->SummonCreature(45937, -10507.4f, 1039.3f, 68.1306f, 1.95f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 50s))
                            if (Creature* Fire5 = me->SummonCreature(45937, -10512.0f, 1040.95f, 60.5179f, 0.51f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 50s))
                                if (Creature* Fire6 = me->SummonCreature(45937, -10516.2f, 1043.94f, 70.1173f, 0.399f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 50s))
                                    if (Creature* Fire7 = me->SummonCreature(45937, -10501.6f, 1039.39f, 71.2453f, 5.32f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 50s))
                                        if (Creature* Fire8 = me->SummonCreature(45937, -10520.0f, 1053.13f, 57.8726f, 5.11f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 50s))
                                            if (Creature* Fire9 = me->SummonCreature(45937, -10504.0f, 1063.19f, 56.481f, 5.11f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 50s))
                                                if (Creature* Fire10 = me->SummonCreature(45937, -10492.4f, 1048.5f, 67.612f, 4.84f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 50s))
                                                    if (Creature* Fire11 = me->SummonCreature(45937, -10503.8f, 1033.96f, 60.6086f, 4.92f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 50s))
                                                        if (Creature* Fire12 = me->SummonCreature(45937, -10502.4f, 1037.98f, 102.67f, 4.95f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 50s))
                                                            if (Creature* Fire13 = me->SummonCreature(45937, -10528.7f, 1038.45f, 56.4513f, 4.95f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 50s))
                                                            {
                                                                Fire1GUID = Fire1->GetGUID();
                                                                Fire2GUID = Fire2->GetGUID();
                                                                Fire3GUID = Fire3->GetGUID();
                                                                Fire4GUID = Fire4->GetGUID();
                                                                Fire5GUID = Fire5->GetGUID();
                                                                Fire6GUID = Fire6->GetGUID();
                                                                Fire7GUID = Fire7->GetGUID();
                                                                Fire8GUID = Fire8->GetGUID();
                                                                Fire9GUID = Fire9->GetGUID();
                                                                Fire10GUID = Fire10->GetGUID();
                                                                Fire11GUID = Fire11->GetGUID();
                                                                Fire12GUID = Fire12->GetGUID();
                                                                Fire13GUID = Fire13->GetGUID();
                                                            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (EventStarted)
            {
                if (EventTimer < diff)
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, PlayerGUID))
                        if (Creature* Hope = me->FindNearestCreature(NPC_HOPE, 50.0f, true))
                            if (Creature* Ripsnarl = me->FindNearestCreature(NPC_RIPSNARL, 50.0f, true))
                            {
                                switch (Phase)
                                {
                                case 0:
                                {
                                    me->SummonCreature(NPC_HORATIO2, -10508.0f, 1047.54f, 60.518f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 100s);
                                    me->SummonCreature(NPC_DANUVIN, -10510.0f, 1040.67f, 60.518f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 100s);
                                    me->SummonCreature(NPC_GUARD, -10504.7f, 1042.87f, 60.518f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 100s);
                                    me->SummonCreature(NPC_INVESTIGATOR, -10509.1f, 1048.56f, 60.518f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 100s);
                                    me->SummonCreature(NPC_INVESTIGATOR3, -10506.3f, 1047.7f, 60.518f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 100s);
                                    std::string l_Msg = "I don't like this, " + std::string(player->GetName()) + ". Stay alert!";
                                    me->Say(l_Msg, LANG_UNIVERSAL);
                                    EventTimer = 4000;
                                    Phase++;
                                    break;
                                }
                                case 1:
                                {
                                    me->SetFacingToObject(Hope);
                                    Hope->AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
                                    Hope->GetMotionMaster()->MovePoint(0, -10507.65f, 1042.81f, 60.51f);
                                    EventTimer = 8000;
                                    Phase++;
                                    break;
                                }
                                case 2:
                                {
                                    Hope->Say("You bastards will burn for what you did.", LANG_UNIVERSAL);
                                    EventTimer = 4000;
                                    Phase++;
                                    break;
                                }
                                case 3:
                                {
                                    me->Say("Hope! Wha...", LANG_UNIVERSAL);
                                    EventTimer = 1500;
                                    Phase++;
                                    break;
                                }
                                case 4:
                                {
                                    Hope->Say("Hope? Is that what I was supposed to feel when I saw my father decapitated by your henchmen?", LANG_UNIVERSAL);
                                    EventTimer = 12000;
                                    Phase++;
                                    break;
                                }
                                case 5:
                                {
                                    Hope->Say("Hope is a cruel joke, played upon us by a harsh and uncaring world. There is no Hope, there is only Vanessa, Vanessa VanCleef.", LANG_UNIVERSAL);
                                    EventTimer = 9500;
                                    Phase++;
                                    break;
                                }
                                case 6:
                                {
                                    Hope->CastSpell(Hope, SPELL_TRANSFORM_VANEESA, true);
                                    EventTimer = 2500;
                                    Phase++;
                                    break;
                                }
                                case 7:
                                {
                                    Hope->Yell("RISE UP BROTHERHOOD! THE DAWNING DAY IS UPON US!", LANG_UNIVERSAL);
                                    SummonBrotherHood();
                                    EventTimer = 4000;
                                    Phase++;
                                    break;
                                }
                                case 8:
                                {
                                    Hope->Say("Tie them up.", LANG_UNIVERSAL);
                                    EventTimer = 1500;
                                    Phase++;
                                    break;
                                }
                                case 9:
                                {
                                    DoCast(me, 79724);
                                    DoStun();
                                    EventTimer = 6000;
                                    Phase++;
                                    break;
                                }
                                case 10:
                                {
                                    Hope->GetMotionMaster()->MovePoint(1, -10512.36f, 1044.36f, 60.518f);
                                    EventTimer = 3000;
                                    Phase++;
                                    break;
                                }
                                case 11:
                                {
                                    Hope->CastSpell(Hope, SPELL_HIT_ME, true);
                                    EventTimer = 1500;
                                    Phase++;
                                    break;
                                }
                                case 12:
                                {
                                    Ripsnarl->RemoveAurasDueToSpell(AURA_IN_STOCKS);
                                    Ripsnarl->CastSpell(Ripsnarl, SPELL_TRANSFORM_HUMAN, true);
                                    Hope->Say("Admiral, your hat.", LANG_UNIVERSAL);
                                    EventTimer = 4000;
                                    Phase++;
                                    break;
                                }
                                case 13:
                                {
                                    Ripsnarl->Say("Thank you, my dear.", LANG_UNIVERSAL);
                                    Ripsnarl->CastSpell(Ripsnarl, SPELL_ADMIRAL_HAT, true);
                                    EventTimer = 4000;
                                    Phase++;
                                    break;
                                }
                                case 14:
                                {
                                    Ripsnarl->AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
                                    Ripsnarl->GetMotionMaster()->MovePoint(2, -10513.41f, 1041.11f, 60.518f);
                                    EventTimer = 2500;
                                    Phase++;
                                    break;
                                }
                                case 15:
                                {
                                    Ripsnarl->GetMotionMaster()->MovePoint(3, -10511.34f, 1042.46f, 60.5172f);
                                    EventTimer = 1500;
                                    Phase++;
                                    break;
                                }
                                case 16:
                                {
                                    Hope->SetFacingToObject(player);
                                    std::string l_Msg = "And you, " + std::string(player->GetName()) + ".  I will spare your life. You have done much to help our cause, albeit unwittingly, but the next time we meet it will be as enemies.";
                                    Hope->Say(l_Msg, LANG_UNIVERSAL);
                                    Ripsnarl->GetMotionMaster()->MovePoint(4, -10516.64f, 1064.78f, 55.362f);
                                    EventTimer = 8000;
                                    Phase++;
                                    break;
                                }
                                case 17:
                                {
                                    Ripsnarl->SetFacingToObject(me);
                                    Hope->GetMotionMaster()->MovePoint(5, -10513.37f, 1056.48f, 57.605f);
                                    me->Say("Why'd you have the Furlbrows killed?", LANG_UNIVERSAL);
                                    EventTimer = 4500;
                                    Phase++;
                                    break;
                                }
                                case 18:
                                {
                                    Hope->SetFacingToObject(me);
                                    Hope->Say("I had no choice, lieutenant. They recognized me. The only people in the world who even knew I existed, recognized my face from when I was an infant.", LANG_UNIVERSAL);
                                    EventTimer = 7000;
                                    Phase++;
                                    break;
                                }
                                case 19:
                                {
                                    Hope->Say("I took no pleasure in their deaths.", LANG_UNIVERSAL);
                                    //SummonFireTrigger(); // I cannot get this to work properly, leaving code in cae someone else works it out
                                    EventTimer = 5000;
                                    Phase++;
                                    break;
                                }
                                case 20:
                                {
                                    Hope->Yell("Leave nothing but ashes in your wake, brothers! Burn Sentinel Hill in the ground!", LANG_UNIVERSAL);
                                    EventTimer = 4000;
                                    Phase++;
                                    break;
                                }
                                case 21:
                                {
                                    std::string l_Msg = std::string(player->GetName()) + ", get to Stormwind. Tell King Wrynn everything, EVERYTHING! GO NOW!";
                                    me->Say(l_Msg, LANG_UNIVERSAL);
                                    Hope->GetMotionMaster()->MovePoint(6, -10518.38f, 1067.99f, 54.84f);
                                    Ripsnarl->SetFacingToObject(Hope);

                                    if (Creature* Helix2 = me->FindNearestCreature(42753, 30.0f, true))
                                    {
                                        Helix2->AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
                                        Helix2->GetMotionMaster()->MovePoint(7, -10520.01f, 1062.31f, 55.386f);
                                    }

                                    if (Creature* Glubtok4 = me->FindNearestCreature(42755, 30.0f, true))
                                    {
                                        Glubtok4->AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
                                        Glubtok4->GetMotionMaster()->MovePoint(8, -10511.79f, 1065.78f, 55.085f);
                                    }

                                    std::list<Player*> players;
                                    me->GetPlayerListInGrid(players, 50.0f);

                                    for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                                        (*itr)->GroupEventHappens(26322, me);

                                    EventTimer = 40000;
                                    Phase++;
                                    break;
                                }
                                case 22:
                                {
                                    me->GetMotionMaster()->MoveTargetedHome();
                                    Hope->GetMotionMaster()->MoveTargetedHome();
                                    Hope->RemoveAurasDueToSpell(79709);
                                    //Ripsnarl->GetMotionMaster()->MoveTargetedHome();
                                    //Ripsnarl->RemoveAurasDueToSpell(79745);
                                    //Ripsnarl->RemoveAurasDueToSpell(79750);
                                    Ripsnarl->DespawnOrUnsummon(3s);
                                    me->AI()->Reset();
                                    RemoveStun();
                                    EventTimer = 100;
                                    PhasingHandler::AddPhase(player, 226, true);
                                    if (Creature* Helix2 = me->FindNearestCreature(42753, 40.0f, true))
                                    {
                                        Helix2->DespawnOrUnsummon();
                                    }

                                    if (Creature* Glubtok4 = me->FindNearestCreature(42755, 40.0f, true))
                                    {
                                        Glubtok4->DespawnOrUnsummon();
                                    }
                                    Phase++;
                                }
                                break;
                                default:
                                    break;
                                }
                            }
                }
                else EventTimer -= diff;
            }
        }
    };
};

/*##########
##npc_black_thief
############*/

class npc_defias_blackguard : public CreatureScript
{
public:
    npc_defias_blackguard() : CreatureScript("npc_defias_blackguard") {}

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_defias_blackguardAI(creature);
    }

    struct npc_defias_blackguardAI : public ScriptedAI
    {
        npc_defias_blackguardAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 timer_DoFire;
        uint32 timer_DoTheStun;
        uint32 ThrowTimer;
        bool Defias_DoFire;
        bool Defias_DoTheStun;

        void Reset() override
        {
            Defias_DoFire = false;
            timer_DoFire = 53000;
            me->CastSpell(me, SPELL_SMOKE, true);
            me->CastSpell(me, 22766, true);
            ThrowTimer = 4000;
        }

        void UpdateAI(uint32 diff) override
        {
            if (timer_DoFire)
            {
                if (timer_DoFire > diff)
                    timer_DoFire -= diff;
                else
                    Defias_DoFire = true;
            }

            if (Defias_DoFire)
            {
                if (Creature* pTarget = me->FindNearestCreature(45937, 30.0f, true))
                {
                    if (ThrowTimer < diff)
                    {
                        DoCast(pTarget, 79778, true);
                        ThrowTimer = 4000;
                        me->SetWalk(true);
                        me->GetMotionMaster()->MovePoint(0, me->GetPositionX() - 5, me->GetPositionY() + 5, me->GetPositionZ());
                    }
                    else ThrowTimer -= diff;
                }
            }
        }
    };
};

// Temp player script to complete quest until whole event is scripted
enum IncenseBurner
{
    SPELL_INCENSE_BURNER = 79586,   // ? spell the player casts
};

class player_spell_incense_burner : public PlayerScript
{
public:
    player_spell_incense_burner() : PlayerScript("player_spell_incense_burner") { }

    void OnSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (!player || !spell)
            return;

        if (spell->GetSpellInfo()->Id != SPELL_INCENSE_BURNER)
            return;

        if (player->GetQuestStatus(QUEST_A_VISION_OF_THE_PAST) != QUEST_STATUS_INCOMPLETE)
            return;

        // Complete the quest directly - temp fix
        player->CompleteQuest(QUEST_A_VISION_OF_THE_PAST);
    }
};

// add phase 226 if player still has the quest 26322 completed
class player_login_phase_rise : public PlayerScript
{
public:
    player_login_phase_rise() : PlayerScript("player_login_phase_rise") { }

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        // Check if player has completed the quest but not yet rewarded or did not advance further than
        // 26761 Threat to the kingdom
        if (player->GetQuestStatus(QUEST_RISE_OF_THE_BROTHERHOOD) == QUEST_STATUS_COMPLETE || (player->GetQuestStatus(QUEST_RISE_OF_THE_BROTHERHOOD) == QUEST_STATUS_REWARDED && player->GetQuestStatus(26761) != QUEST_STATUS_REWARDED))
        {
            // Apply the phase
            PhasingHandler::AddPhase(player, 226, true);
            //TC_LOG_DEBUG("scripts.ai", "Applied phase 226 to player %s on login.", player->GetName().c_str());
        }
    }

    void OnUpdateZone(Player* player, uint32 newZone, uint32 newArea) override
    {
        if (newZone == 40 && newArea == 108 && (player->GetQuestStatus(QUEST_RISE_OF_THE_BROTHERHOOD) == QUEST_STATUS_COMPLETE) || (player->GetQuestStatus(QUEST_RISE_OF_THE_BROTHERHOOD) == QUEST_STATUS_REWARDED && player->GetQuestStatus(26761) != QUEST_STATUS_REWARDED))
        {
            PhasingHandler::AddPhase(player, 226, true);
        }
    }
};

void AddSC_westfall()
{
    using namespace Scripts::EasternKingdoms::Westfall;

    // Creature
    RegisterCreatureAI(npc_westfall_overloaded_harvest_golem);
    RegisterCreatureAI(npc_ripsnarl);

    new npc_westplains_drifter();
    new npc_lous_parting_thoughts_trigger();
    new npc_lous_parting_thoughts_thug();
    new npc_hungry_hobo();
    new npc_shadowy_tower();
    new npc_agent_kearnen();
    new npc_mortwake_tower_elite();
    new npc_shadowy_trigger();
    new player_rise_of_the_brotherhood();
    new npc_rise_br();
    new npc_defias_blackguard();

    new player_spell_incense_burner();
    new player_login_phase_rise();
    RegisterCreatureAI(npc_westfall_hobo_witness);

    // Spells
    RegisterSpellScript(spell_westfall_unbound_energy);
    RegisterSpellScript(spell_westfall_reaping_blows);
    RegisterSpellScript(spell_westfall_wake_harvest_golem);
    RegisterSpellScript(spell_westfall_summon_lous_house);
    RegisterSpellScript(spell_westfall_quest_credit_jangolode_event);
    RegisterSpellScript(spell_westfall_despawn_jangolode_actor);
    RegisterSpellScript(spell_westfall_livin_the_life_ping_glubtok);
    RegisterSpellScript(spell_westfall_livin_the_life_ping_figure);
    RegisterSpellScript(spell_westfall_summon_ragamuffin_looter);
    RegisterSpellScript(spell_westfall_aggro_hobo);
}
