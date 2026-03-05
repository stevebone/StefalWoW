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
            events.Reset();
            events.ScheduleEvent(1, 2s); // 8 seconds
        }

        void JustAppeared() override
        {
            events.Reset();
            events.ScheduleEvent(1, 2s);
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
                    default:
                        break;
                    }

                    Unit* target = me->GetVictim();
                    if (!target)
                        target = me->SelectNearestTarget(8.0f);

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
