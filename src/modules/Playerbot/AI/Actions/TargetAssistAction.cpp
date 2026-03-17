/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "TargetAssistAction.h"
#include "BotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "Log.h"

namespace Playerbot
{

TargetAssistAction::TargetAssistAction(::std::string const& name)
    : CombatAction(name)
{
    // IMPLEMENTATION COMPLETE - Full group-based target assistance
    // Features:
    // - Leader target prioritization (always assist leader first)
    // - Group member target aggregation with priority scoring
    // - Distance-based, health-based, and attacker-count prioritization
    // - Target validation (alive, hostile, LOS, range)
    // - Smart target switching (avoids thrashing on low-health targets)
    // - Class-aware assist range (melee vs ranged)
}

ActionResult TargetAssistAction::Execute(BotAI* ai, ActionContext const& context)
{
    if (!ai)
        return ActionResult::FAILED;

    Player* bot = ai->GetBot();
    if (!bot || !bot->IsAlive())
        return ActionResult::FAILED;
    // CRITICAL FIX: Implement actual target assistance logic
    TC_LOG_INFO("module.playerbot.combat", "TargetAssistAction::Execute for bot {}", bot->GetName());
    // Check if we have a target from context
    Unit* targetFromContext = dynamic_cast<Unit*>(context.target);
    if (targetFromContext)
    {
        TC_LOG_INFO("module.playerbot.combat", "Using target from context: {} for bot {}",
                    targetFromContext->GetName(), bot->GetName());

        // Engage the target
    if (EngageTarget(bot, targetFromContext))
        {
            TC_LOG_INFO("module.playerbot.combat", "SUCCESS: Bot {} now attacking {}",
                        bot->GetName(), targetFromContext->GetName());
            return ActionResult::SUCCESS;
        }
    }
    // Get group
    Group* group = bot->GetGroup();
    if (!group)
    {
        TC_LOG_DEBUG("module.playerbot.combat", "Bot {} not in group for assist action", bot->GetName());
        return ActionResult::FAILED;
    }

    // Find best target to assist
    Unit* bestTarget = GetBestAssistTarget(bot, group);
    if (!bestTarget)
    {
        TC_LOG_DEBUG("module.playerbot.combat", "No valid assist target found for bot {}", bot->GetName());
        return ActionResult::FAILED;
    }

    // Check if we should switch to this target
    if (!ShouldSwitchTarget(bot, bestTarget))
    {
        TC_LOG_DEBUG("module.playerbot.combat", "Bot {} keeping current target", bot->GetName());
        return ActionResult::IN_PROGRESS;
    }

    // Engage the new target
    if (EngageTarget(bot, bestTarget))
    {
        TC_LOG_INFO("module.playerbot.combat", "SUCCESS: Bot {} now assisting by attacking {}",
                    bot->GetName(), bestTarget->GetName());
        return ActionResult::SUCCESS;
    }
    TC_LOG_DEBUG("module.playerbot.combat", "Failed to engage target for bot {}", bot->GetName());
    return ActionResult::FAILED;
}
bool TargetAssistAction::IsPossible(BotAI* ai) const
{
    if (!ai)
        return false;

    Player* bot = ai->GetBot();
    if (!bot || !bot->IsAlive())
        return false;

    return true;
}
bool TargetAssistAction::IsUseful(BotAI* ai) const
{
    if (!ai)
        return false;

    Player* bot = ai->GetBot();
    if (!bot || !bot->IsAlive())
        return false;

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    // Check if group members are in combat
    for (GroupReference const& itr : group->GetMembers())
    {
        if (Player* member = itr.GetSource())
        {
            if (member != bot && member->IsInCombat())
                return true;
        }
    }

    return false;
}

Unit* TargetAssistAction::GetBestAssistTarget(Player* bot, Group* group) const
{
    if (!bot || !group)
        return nullptr;

    Unit* bestTarget = nullptr;
    float bestPriority = 0.0f;

    // First check leader's target
    if (Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID()))
    {
        if (leader != bot && leader->IsInCombat())
        {
            if (Unit* leaderTarget = leader->GetVictim())
            {
                if (IsValidAssistTarget(leaderTarget, bot))
                {
                    return leaderTarget; // Always prioritize leader
                }
            }
        }
    }

    // Check all group members' targets
    for (GroupReference const& itr : group->GetMembers())
    {
        if (Player* member = itr.GetSource())
        {
            if (member != bot && member->IsInCombat())
            {
                if (Unit* target = member->GetVictim())
                {
                    if (IsValidAssistTarget(target, bot))
                    {
                        float priority = CalculateAssistPriority(bot, target, group);
                        if (priority > bestPriority)
                        {
                            bestPriority = priority;
                            bestTarget = target;
                        }
                    }
                }
            }
        }
    }

    return bestTarget;
}

bool TargetAssistAction::IsValidAssistTarget(Unit* target, Player* bot) const
{
    if (!target || !bot)
        return false;

    if (!target->IsAlive())
        return false;

    if (!bot->IsHostileTo(target))
        return false;

    if (!bot->IsWithinLOSInMap(target))
        return false;

    if (!IsInAssistRange(bot, target))
        return false;

    return true;
}

bool TargetAssistAction::ShouldSwitchTarget(Player* bot, Unit* newTarget) const
{
    if (!bot || !newTarget)
        return false;

    Unit* currentTarget = bot->GetVictim();
    if (!currentTarget)
        return true; // No current target, switch
    if (currentTarget == newTarget)
        return false; // Already targeting

    // Switch if current target is low health
    if (currentTarget->GetHealthPct() < _switchHealthThreshold)
        return false; // Keep attacking low health target

    return true;
}

bool TargetAssistAction::EngageTarget(Player* bot, Unit* target)
{
    if (!bot || !target)
        return false;

    // Set target and attack
    bot->SetTarget(target->GetGUID());
    bot->Attack(target, true);

    return true;
}

bool TargetAssistAction::IsInAssistRange(Player* bot, Unit* target) const
{
    if (!bot || !target)
        return false;

    float range = GetClassAssistRange(bot);
    float rangeSq = range * range;
    return bot->GetExactDistSq(target) <= rangeSq;
}

float TargetAssistAction::GetClassAssistRange(Player* bot) const
{
    if (!bot)
        return 30.0f;

    // Melee classes
    if (bot->GetClass() == CLASS_WARRIOR || bot->GetClass() == CLASS_ROGUE ||
        bot->GetClass() == CLASS_DEATH_KNIGHT || bot->GetClass() == CLASS_DEMON_HUNTER ||
        bot->GetClass() == CLASS_PALADIN || bot->GetClass() == CLASS_MONK)
    {
        return MELEE_ASSIST_RANGE;
    }

    // Ranged classes
    return RANGED_ASSIST_RANGE;
}

float TargetAssistAction::CalculateAssistPriority(Player* bot, Unit* target, Group* group) const
{
    if (!bot || !target || !group)
        return 0.0f;

    float priority = 0.0f;

    // Base priority
    priority += 10.0f;

    // Count attackers on this target
    uint32 attackers = CountAssistingMembers(target, group);
    priority += attackers * 5.0f;

    // Distance factor (closer = higher priority)
    float distance = ::std::sqrt(bot->GetExactDistSq(target)); // Calculate once from squared distance
    priority += ::std::max(0.0f, 30.0f - distance);

    // Health factor (lower health = higher priority)
    float healthPct = target->GetHealthPct();
    if (healthPct < 50.0f)
        priority += LOW_HEALTH_BONUS;

    return priority;
}

uint32 TargetAssistAction::CountAssistingMembers(Unit* target, Group* group) const
{
    if (!target || !group)
        return 0;

    uint32 count = 0;
    for (GroupReference const& itr : group->GetMembers())
    {
        if (Player* member = itr.GetSource())
        {
            if (member->GetVictim() == target)
                ++count;
        }
    }
    return count;
}

} // namespace Playerbot