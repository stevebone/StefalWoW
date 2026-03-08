#include "Log.h"

#include "Followship_bots_mgr.h"

#include "Followship_bots_combat_handler.h"
#include "Followship_bots_group_handler.h"
#include "Followship_bots_movement_handler.h"

namespace FSBCombat
{
    void EvaluateAttackNeeded(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return;

        if (bot->HasAura(SPELL_SPECIAL_GHOST))
            return;

        Unit* victim = bot->GetVictim();
        // Prevent bot from disengaging from current target
        if (victim)
        {
            if (!victim || !victim->IsInWorld() || victim->IsDuringRemoveFromWorld() || !victim->IsAlive())
            {
                bot->AttackStop();
                return;
            }            

            // Bot must still consider this its real victim
            Unit* ensured = bot->EnsureVictim();
            if (!ensured || ensured != victim || !ensured->IsAlive() || !ensured->IsInWorld() || ensured->IsDuringRemoveFromWorld())
                return;

            // Owner may be null OR on another map
            Player* owner = FSBMgr::Get()->GetBotOwner(bot);
            if (!owner || !owner->IsInWorld() || owner->IsDuringRemoveFromWorld())
                return;

            // Owner must be on same map for combat checks
            if (owner->GetMapId() != victim->GetMapId())
                return;

            if (owner)
            {
                // Now safe to check combat state
                if ((!bot->IsValidAttackTarget(victim) && owner->IsInCombatWith(victim)) ||
                    (!bot->IsHostileTo(victim) && owner->IsInCombatWith(victim)))
                {
                    // Do not remove this or bots cant cast spells on neutral mobs
                    victim->SetFaction(14);
                    bot->GetCombatManager().SetInCombatWith(victim);
                    victim->GetCombatManager().SetInCombatWith(bot);
                }
            }

            return;
        }

        Unit* target = GetNextAttackTarget(bot);

        if (!target || !target->IsInWorld() || target->IsDuringRemoveFromWorld())
            return;

        if (target && !BotCanAttack(bot, target))
            return;

        if (target)
            BotDoAttack(bot, target);
        else
        {
            if (bot->IsInCombat())
            {
                bot->RemoveUnitFlag(UNIT_FLAG_IN_COMBAT);
                bot->AttackStop();
                bot->InterruptNonMeleeSpells(false);
                bot->CombatStop(true);
                bot->ClearInCombat();
                bot->GetMotionMaster()->Clear();
                FSBMovement::BotHandleReturnMovement(bot);
            }
        }
    }

    void BotDoAttack(Creature* bot, Unit* target)
    {
        if (!bot || !bot->IsInWorld() || bot->IsDuringRemoveFromWorld() || !bot->IsAlive())
            return;

        // Handles attack with or without chase and also resets flags
        // for next update / creature kill

        if (!target || !target->IsInWorld() || target->IsDuringRemoveFromWorld() || !target->IsAlive())
            return;

        if (bot->Attack(target, true))
        {
            bot->SetUnitFlag(UNIT_FLAG_IN_COMBAT); // on player bots, this flag indicates we're actively going after a target - that's what we're doing, so set it

            bot->GetMotionMaster()->Clear();

            if (bot->HasUnitState(UNIT_STATE_FOLLOW))
                bot->GetMotionMaster()->Remove(FOLLOW_MOTION_TYPE);

            if (FSBMovement::EnsureInRange(bot, target))
            {
                // say some text here
            }
        }
    }

    // result false means bot cannot attack
    bool BotCanAttack(Creature* bot, Unit* target)
    {
        // Evaluates wether a bot can attack a specific target based on MoveState, ReactState and other flags
        // IMPORTANT: The order in which things are checked is important, be careful if you add or remove checks

        // Hmmm...
        if (!bot->IsAlive())
            return false;

        if (!target || !target->IsAlive())
            return false;

        // Passive - passive bots cannot attack
        if (bot->HasReactState(REACT_PASSIVE))
            return false;

        if (target->HasBreakableByDamageCrowdControlAura())
            return false;

        // Stay - can attack if target is within range or commanded to
        if (auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI()))
        {
            if (baseAI->botMoveState == FSB_MOVE_STATE_STAY)
                return bot->IsWithinMeleeRange(target) || bot->IsWithinCombatRange(target, FSBCombatUtils::GetBotChaseDistance(bot));
        }

        return true;
    }

    Unit* GetNextAttackTarget(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return nullptr;

        // 1. Check own victim
        Unit* victim = bot->GetVictim();
        if (victim && victim->IsAlive())
        {
            if (!victim || !victim->IsInWorld() || victim->IsDuringRemoveFromWorld() || !victim->IsAlive())
                return nullptr;

            TC_LOG_DEBUG("scripts.fsb.combat", "FSB: GetNextAttackTarget Bot {} next target is their victim {}", bot->GetName(), victim->GetName());
            return victim;
        }

        // 2. Check own attacker
        Unit* attacker = bot->getAttackerForHelper();
        if (attacker && attacker->IsAlive())
        {
            if (!attacker || !attacker->IsInWorld() || attacker->IsDuringRemoveFromWorld() || !attacker->IsAlive())
                return nullptr;

            TC_LOG_DEBUG("scripts.fsb.combat", "FSB: GetNextAttackTarget Bot {} next target is their attacker {}", bot->GetName(), attacker->GetName());
            return attacker;
        }

        // 3. Check owner victim & attacker
        Player* player = FSBMgr::Get()->GetBotOwner(bot);
        if (player)
        {
            Unit* ownerVictim = player->GetVictim();
            if (ownerVictim && ownerVictim->IsAlive())
            {
                if (!ownerVictim || !ownerVictim->IsInWorld() || ownerVictim->IsDuringRemoveFromWorld() || !ownerVictim->IsAlive())
                    return nullptr;

                TC_LOG_DEBUG("scripts.fsb.combat", "FSB: GetNextAttackTarget Bot {} next target is their owner victim {}", bot->GetName(), ownerVictim->GetName());
                return ownerVictim;
            }
            
            Unit* ownerAttacker = player->getAttackerForHelper();
            if (ownerAttacker && ownerAttacker->IsAlive())
            {
                if (!ownerAttacker || !ownerAttacker->IsInWorld() || ownerAttacker->IsDuringRemoveFromWorld() || !ownerAttacker->IsAlive())
                    return nullptr;

                TC_LOG_DEBUG("scripts.fsb.combat", "FSB: GetNextAttackTarget Bot {} next target is their owner attacker {}", bot->GetName(), ownerAttacker->GetName());
                return ownerAttacker;
            }
            
        }

        // 4. Group assist logic
        if (auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI()))
        {
            Unit* assistTarget = FSBGroup::BotGetFirstMemberToAssist(bot, baseAI->botLogicalGroup);
            if (assistTarget && assistTarget->IsAlive())
            {
                if (!assistTarget || !assistTarget->IsInWorld() || assistTarget->IsDuringRemoveFromWorld() || !assistTarget->IsAlive())
                    return nullptr;

                TC_LOG_DEBUG("scripts.fsb.combat", "FSB: GetNextAttackTarget Bot {} next target is their member attacker/victim {}", bot->GetName(), assistTarget->GetName());
                return assistTarget;
            }
        }

        return nullptr;
    }

    bool ShouldSwitchToMelee(Creature* bot)
    {
        if (!bot)
            return false;

        if (!bot->IsInCombat())
            return false;

        Unit* target = bot->GetVictim();
        if (!target)
            return false;

        // Mana threshold - tweak as needed
        uint32 mana = bot->GetPower(POWER_MANA);
        uint32 maxMana = bot->GetMaxPower(POWER_MANA);

        if (maxMana > 0 && mana > maxMana * 0.05f) // >5% mana
            return false;

        // Already in melee range?
        if (bot->IsWithinMeleeRange(target))
            return false;

        return true;
    }

    void EnterMeleeMode(Creature* bot)
    {
        if (!bot)
            return;

        Unit* target = bot->GetVictim();
        if (!target)
            return;

        // Stop casting
        bot->InterruptNonMeleeSpells(false);

        // Move into melee range
        bot->GetMotionMaster()->Clear();
        bot->GetMotionMaster()->MoveChase(target, 0.5f);

        // Enable auto-attack
        bot->Attack(target, true);

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: {} switching to melee mode (OOM)", bot->GetName());
    }
}
