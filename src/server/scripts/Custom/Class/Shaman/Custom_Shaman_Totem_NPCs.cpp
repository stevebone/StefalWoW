#include "ScriptMgr.h"
#include "Creature.h"
#include "CreatureAI.h"
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
            //events.Reset();
            //events.ScheduleEvent(EVENT_PERIODIC_SPELL_CAST, 0s); // 8 seconds
        }

        void JustAppeared() override
        {
            events.Reset();
            events.ScheduleEvent(EVENT_PERIODIC_SPELL_CAST, 0s);
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_PERIODIC_SPELL_CAST)
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
                    default:
                        break;
                    }

                    Unit* target = nullptr;

                    switch (spellId)
                    {
                    case SPELL_EARTHGRAB_TOTEM:
                        target = me->GetVictim();
                        if (!target)
                            target = me->SelectNearestTarget(8.0f);
                        break;
                    case SPELL_MANA_TIDE_TOTEM:
                        target = me->GetOwner();
                        if (target && target->HasAura(SPELL_MANA_TIDE_TOTEM))
                        {
                            target = target->GetOwner();
                            if (target && target->HasAura(SPELL_MANA_TIDE_TOTEM))
                            {
                                Trinity::AnyFriendlyUnitInObjectRangeCheck check(me, me, 20.0f);
                                Trinity::UnitLastSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(me, target, check);
                                Cell::VisitAllObjects(me, searcher, 20.0f);
                            }
                        }
                        break;
                    case NPC_HEALING_TIDE_TOTEM:
                        target = me;
                        break;
                    default:
                        break;
                    }                    

                    if (target)
                        me->CastSpell(target, spellId, false);

                    events.ScheduleEvent(EVENT_PERIODIC_SPELL_CAST, 2s);
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
