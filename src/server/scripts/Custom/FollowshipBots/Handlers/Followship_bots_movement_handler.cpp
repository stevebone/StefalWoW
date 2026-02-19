#include "Followship_bots_ai_base.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

#include "Followship_bots_druid.h"
#include "Followship_bots_paladin.h"
#include "Followship_bots_warlock.h"

#include "Followship_bots_events_handler.h"
#include "Followship_bots_movement_handler.h"
#include "Followship_bots_spells_handler.h"

namespace FSBMovement
{
    void StopFollow(Creature* bot)
    {
        if (!bot)
            return;

        if (bot->HasUnitState(UNIT_STATE_FOLLOW))
            bot->GetMotionMaster()->Remove(FOLLOW_MOTION_TYPE);

        SetBotMoveState(bot, FSB_MOVE_STATE_STAY);

        bot->StopMoving();
        bot->GetMotionMaster()->Clear();
    }

    void ResumeFollow(Creature* bot, float followDistance, float followAngle)
    {
        Player* player = FSBMgr::Get()->GetBotOwner(bot);

        if (!player)
            return;

        SetBotMoveState(bot, FSB_MOVE_STATE_FOLLOWING);

        bot->GetMotionMaster()->MoveFollow(player, followDistance, followAngle);
    }

    FSB_MovementStates GetBotMoveState(Creature* bot)
    {
        if (!bot || !bot->IsBot())
            return FSB_MOVE_STATE_IDLE;


        if (FSB_BaseAI* ai = dynamic_cast<FSB_BaseAI*>(bot->AI()))
            return ai->botMoveState;


        return FSB_MOVE_STATE_IDLE;
    }

    void SetBotMoveState(Creature* bot, FSB_MovementStates moveState)
    {
        if (!bot || !bot->IsBot())
            return;


        if (FSB_BaseAI* ai = dynamic_cast<FSB_BaseAI*>(bot->AI()))
            ai->botMoveState = moveState;

    }

    MovementGeneratorType GetMovementType(Unit* me)
    {
        if (!me)
            return IDLE_MOTION_TYPE;

        MotionMaster* mm = me->GetMotionMaster();
        if (!mm)
            return IDLE_MOTION_TYPE;

        return mm->GetCurrentMovementGeneratorType();
    }

    void BotSetMountedState(Creature* bot, bool& botMounted)
    {
        if (!bot || !bot->IsAlive())
            return;

        if (bot->HasUnitState(UNIT_STATE_CASTING))
            return;

        Player* player = FSBMgr::Get()->GetBotOwner(bot);

        if (!player)
            return;

        FSB_Class cls = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());

        if (player->HasAuraType(SPELL_AURA_MOUNTED) && !botMounted)
        {
            uint32 randomSpell = FSBSpellsUtils::GetRandomMountSpellForBot(bot);
            
            if (cls == FSB_Class::Warlock)
            {
                std::vector<uint32> warlockMounts =
                {
                    randomSpell,
                    SPELL_WARLOCK_FELSTEED,
                    SPELL_WARLOCK_DREADSTEED
                };

                uint32 spellId = warlockMounts[urand(0, warlockMounts.size() - 1)];
                if (FSBSpells::BotCastSpell(bot, spellId, bot))
                    botMounted = true;
            }
            else if (cls == FSB_Class::Paladin)
            {
                std::vector<uint32> paladinMounts =
                {
                    randomSpell,
                    SPELL_PALADIN_CHARGER,
                    SPELL_PALADIN_WARHORSE
                };

                uint32 spellId = paladinMounts[urand(0, paladinMounts.size() - 1)];
                if (FSBSpells::BotCastSpell(bot, spellId, bot))
                {
                    botMounted = true;
                    FSBEvents::ScheduleBotEvent(bot, FSB_EVENT_HIRED_MOUNT_AURA, 3s, 5s);
                }
            }
            else if (cls == FSB_Class::Druid)
            {
                std::vector<uint32> druidMounts =
                {
                    randomSpell,
                    SPELL_DRUID_TRAVEL
                };

                bot->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);

                uint32 spellId = druidMounts[urand(0, druidMounts.size() - 1)];
                if (FSBSpells::BotCastSpell(bot, spellId, bot))
                {
                    botMounted = true;
                    if (spellId == SPELL_DRUID_TRAVEL)
                    {
                        //bot->SetDisplayId(65133, false);
                    }
                }

                
            }
            else
            {
                if (FSBSpellsUtils::CastRandomMountLevelSpell(bot))
                    botMounted = true;
            } 
        }
        else if (!player->HasAuraType(SPELL_AURA_MOUNTED) && botMounted)
        {
            botMounted = false;
            bot->Dismount();
            bot->RemoveAurasByType(SPELL_AURA_MOUNTED);
            bot->RemoveAurasDueToSpell(SPELL_PALADIN_CRUSADER_AURA);
            if(cls == FSB_Class::Druid)
            {
                auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
                bot->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);
                bot->RemoveAurasDueToSpell(SPELL_DRUID_TRAVEL);
                baseAI->botRole = FSB_ROLE_NONE;

            }
        }
    }
}
