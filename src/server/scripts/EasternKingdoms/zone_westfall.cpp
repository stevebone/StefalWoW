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
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "PhasingHandler.h"
#include "SharedDefines.h"
#include "TemporarySummon.h"


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

    void OnCharmed(bool apply) override
    {
        // Prevent PossessedAI from replacing this AI
        if (apply)
            me->SetControlled(false, UNIT_STATE_POSSESSED);
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
        if (roll_chance(clueGainChance))
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
                    if (roll_chance(50)) // prevent spam
                    {
                        if (roll_chance(70)) // chance to trigger simple talk event
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
                                    Ripsnarl->RemoveAurasDueToSpell(1);
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

        //QUEST_A_VISION_OF_THE_PAST
        if (player->GetQuestStatus(0) != QUEST_STATUS_INCOMPLETE)
            return;

        // Complete the quest directly - temp fix
        player->CompleteQuest(0);
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
        if ((newZone == 40 && newArea == 108 && (player->GetQuestStatus(QUEST_RISE_OF_THE_BROTHERHOOD) == QUEST_STATUS_COMPLETE)) || ((player->GetQuestStatus(QUEST_RISE_OF_THE_BROTHERHOOD) == QUEST_STATUS_REWARDED && player->GetQuestStatus(26761) != QUEST_STATUS_REWARDED)))
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
