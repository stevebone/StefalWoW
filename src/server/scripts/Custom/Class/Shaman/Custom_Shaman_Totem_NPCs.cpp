#include "Creature.h"
#include "CreatureAI.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "EventMap.h"

#include "Custom_Shaman_Totem_NPCs.h"

class custom_npc_totem_generic : public CreatureScript
{
public:
    custom_npc_totem_generic() : CreatureScript("custom_npc_totem_generic") { }

    struct custom_npc_totem_genericAI : public ScriptedAI
    {
        custom_npc_totem_genericAI(Creature* creature) : ScriptedAI(creature) { }

        EventMap events;

        void IsSummonedBy(WorldObject* /*summoner*/) override
        {
        }

        void JustAppeared() override
        {
            events.Reset();
            if (me->GetEntry() == NPC_FIRE_NOVA_TOTEM)
                events.ScheduleEvent(EVENT_TIMED_SPELL_CAST_NOVA, 5500ms);
            else if (me->GetEntry() == NPC_CAPACITATOR_TOTEM)
                events.ScheduleEvent(EVENT_TIMED_SPELL_CAST_CAPACITATOR, 2s);                
            else events.ScheduleEvent(EVENT_PERIODIC_SPELL_CAST, 0s);
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {

                case EVENT_PERIODIC_SPELL_CAST:
                {
                    uint32 spellId = 0;

                    switch (me->GetEntry())
                    {
                    case NPC_EARTHGRAB_TOTEM:
                        spellId = SPELL_EARTHGRAB_TOTEM;
                        break;
                    case NPC_MANA_TIDE_TOTEM:
                        spellId = SPELL_MANA_TIDE_TOTEM;
                        break;
                    case NPC_HEALING_TIDE_TOTEM:
                        spellId = SPELL_HEALING_TIDE_TOTEM;
                        break;
                    case NPC_SCORCHING_TOTEM:
                        spellId = SPELL_SCORCH_TOTEM;
                        break;
                    default:
                        break;
                    }

                    Unit* target = nullptr;

                    switch (spellId)
                    {
                    case SPELL_SCORCH_TOTEM:
                    case SPELL_EARTHGRAB_TOTEM:
                        target = me->GetVictim();
                        if (!target)
                            target = me->SelectNearestTarget(8.0f);
                        break;
                    case SPELL_MANA_TIDE_TOTEM:
                    {
                        target = me->GetOwner();
                        if (target && target->HasAura(SPELL_MANA_TIDE_TOTEM))
                        {
                            target = target->GetOwner();
                            if (target && target->HasAura(SPELL_MANA_TIDE_TOTEM))
                            {
                                Unit* found = nullptr;

                                struct FriendlyWithoutAuraCheck
                                {
                                    Unit* me;
                                    float range;
                                    uint32 aura;

                                    FriendlyWithoutAuraCheck(Unit* me, float range, uint32 aura)
                                        : me(me), range(range), aura(aura) {}

                                    bool operator()(Unit* u)
                                    {
                                        return u && u != me &&
                                            me->IsFriendlyTo(u) &&
                                            me->GetDistance(u) <= range &&
                                            !u->HasAura(aura);
                                    }
                                };

                                FriendlyWithoutAuraCheck check(me, 20.0f, SPELL_MANA_TIDE_TOTEM);
                                Trinity::UnitSearcher<FriendlyWithoutAuraCheck> searcher(me, found, check);
                                Cell::VisitAllObjects(me, searcher, 20.0f);

                                if (found)
                                    target = found;
                            }
                        }
                        break;
                    }
                    case SPELL_HEALING_TIDE_TOTEM:
                        target = me;
                        break;
                    default:
                        break;
                    }

                    if (target)
                        me->CastSpell(target, spellId, false);

                    events.ScheduleEvent(EVENT_PERIODIC_SPELL_CAST, 2s);

                    break;
                }
                case EVENT_TIMED_SPELL_CAST_NOVA:
                    me->CastSpell(me, SPELL_FIRE_NOVA_TOTEM, false);
                    break;

                case EVENT_TIMED_SPELL_CAST_CAPACITATOR:
                    me->CastSpell(me, SPELL_CAPACITATOR_TOTEM, false);
                    break;

                default:
                    break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new custom_npc_totem_genericAI(creature);
    }
};

void AddSC_custom_shaman_totem_npcs()
{
    new custom_npc_totem_generic();
}
