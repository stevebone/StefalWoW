#include "Log.h"

#include "Followship_bots_ai_base.h"
#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

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
                if (cls == FSB_Class::Shaman && bot->HasAura(SPELL_SHAMAN_GHOST_WOLF))
                    bot->RemoveAurasDueToSpell(SPELL_SHAMAN_GHOST_WOLF);

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

    void BotHandleReturnMovement(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return;

        Player* owner = FSBMgr::Get()->GetBotOwner(bot);

        if (!baseAI->botHired || !owner)
        {
            bot->GetMotionMaster()->Clear();
            bot->ClearUnitState(UNIT_STATE_FOLLOW | UNIT_STATE_CHASE | UNIT_STATE_ROAMING);
            bot->GetMotionMaster()->MoveIdle();

            TC_LOG_DEBUG("scripts.fsb.movement", "FSB: BotHandleReturnMovement bot {} does not have owner so it is set to Idle", bot->GetName());
            return;
        }

        if (owner && baseAI->botMoveState == FSB_MOVE_STATE_FOLLOWING)
        {
            if (bot->HasUnitState(UNIT_STATE_CHASE))
                bot->GetMotionMaster()->Remove(CHASE_MOTION_TYPE);

            bot->GetMotionMaster()->Clear();
            bot->GetMotionMaster()->MoveFollow(owner, baseAI->botFollowDistance, baseAI->botFollowAngle);

            TC_LOG_DEBUG("scripts.fsb.movement", "FSB: BotHandleReturnMovement bot {} has owner so it is set to Follow", bot->GetName());

        }
    }

    bool EnsureInRange(Creature* bot, Unit* target)
    {
        if (!bot || !target || !target->IsAlive())
            return false;

        float dist = bot->GetDistance(target);
        float requiredRange = FSBCombatUtils::GetBotChaseDistance(bot);

        // Already in range
        if (dist <= requiredRange)
            return false;

        MotionMaster* mm = bot->GetMotionMaster();
        if (!mm)
            return false;

        // Avoid restarting the same chase every tick
        if (mm->GetCurrentMovementGeneratorType() == CHASE_MOTION_TYPE)
            return true;

        mm->Clear();
        bot->ClearUnitState(UNIT_STATE_FOLLOW | UNIT_STATE_CHASE);

        // Bots with ranged attacks should not care about the chase angle at all.
        float angle = requiredRange == 2.f ? float(M_PI) : frand(-2.f, 2.f);
        float tolerance = requiredRange == 0.f ? float(M_PI_4) : float(M_PI * 2);
        mm->MoveChase(target, ChaseRange(0.f, requiredRange), ChaseAngle(angle, tolerance));       

        TC_LOG_DEBUG("scripts.fsb.movement", "FSB: EnsureInRange Bot {} chasing {} to {:.1f} yards", bot->GetName(), target->GetName(), requiredRange);

        return true;
    }

    bool EnsureInRange(Creature* bot, Unit* target, float range)
    {
        if (!bot || !target || !target->IsAlive())
            return false;

        float dist = bot->GetDistance(target);

        // Already in range
        if (dist <= range)
            return false;

        MotionMaster* mm = bot->GetMotionMaster();
        if (!mm)
            return false;

        // Avoid restarting the same chase every tick
        if (mm->GetCurrentMovementGeneratorType() == CHASE_MOTION_TYPE)
            return true;

        mm->Clear();
        bot->ClearUnitState(UNIT_STATE_FOLLOW | UNIT_STATE_CHASE);

        // Bots with ranged attacks should not care about the chase angle at all.
        float angle = range == 2.f ? float(M_PI) : frand(-2.f, 2.f);
        float tolerance = range == 0.f ? float(M_PI_4) : float(M_PI * 2);
        mm->MoveChase(target, ChaseRange(0.f, range), ChaseAngle(angle, tolerance));

        TC_LOG_DEBUG("scripts.fsb.movement", "FSB: EnsureInRange for spell Bot {} chasing {} to {:.1f} yards", bot->GetName(), target->GetName(), range);

        return true;
    }
}
