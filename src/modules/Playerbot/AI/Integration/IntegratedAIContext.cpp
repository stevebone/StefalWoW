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

#include "IntegratedAIContext.h"
#include "AI/BotAI.h"
#include "../../Advanced/TacticalCoordinator.h"
#include "Player.h"
#include "Group.h"
#include "Unit.h"
#include "ObjectAccessor.h"
#include "Log.h"

namespace Playerbot
{

// ============================================================================
// IntegratedAIContext
// ============================================================================

IntegratedAIContext::IntegratedAIContext(BotAI* bot, BTBlackboard* localBlackboard)
    : _bot(bot)
    , _localBlackboard(localBlackboard)
{
}

SharedBlackboard* IntegratedAIContext::GetGroupBlackboard() const
{
    if (!_cachedGroupBlackboard && IsInGroup())
    {
        _cachedGroupBlackboard = BlackboardManager::GetGroupBlackboard(GetGroupId());
    }
    return _cachedGroupBlackboard;
}

SharedBlackboard* IntegratedAIContext::GetRaidBlackboard() const
{
    if (!_cachedRaidBlackboard && IsInRaid())
    {
        _cachedRaidBlackboard = BlackboardManager::GetRaidBlackboard(GetRaidId());
    }
    return _cachedRaidBlackboard;
}

SharedBlackboard* IntegratedAIContext::GetZoneBlackboard() const
{
    if (!_cachedZoneBlackboard)
    {
        _cachedZoneBlackboard = BlackboardManager::GetZoneBlackboard(GetZoneId());
    }
    return _cachedZoneBlackboard;
}

Advanced::TacticalCoordinator* IntegratedAIContext::GetTacticalCoordinator() const
{
    if (!_cachedTacticalCoordinator && _bot)
    {
        _cachedTacticalCoordinator = _bot->GetTacticalCoordinator();
    }
    return _cachedTacticalCoordinator;
}

Coordination::RaidOrchestrator* IntegratedAIContext::GetRaidOrchestrator() const
{
    // Would retrieve from raid coordination manager
    // Placeholder implementation
    return _cachedRaidOrchestrator;
}

Coordination::ZoneOrchestrator* IntegratedAIContext::GetZoneOrchestrator() const
{
    if (!_cachedZoneOrchestrator)
    {
        _cachedZoneOrchestrator = Coordination::ZoneOrchestratorManager::GetOrchestrator(GetZoneId());
    }
    return _cachedZoneOrchestrator;
}

void IntegratedAIContext::PropagateToGroup(::std::string const& key)
{
    if (!IsInGroup())
        return;

    SharedBlackboard* groupBoard = GetGroupBlackboard();
    if (!groupBoard || !_localBlackboard)
        return;

    if (key.empty())
    {
        // Propagate keys that should be shared with the group
        // Use naming convention: keys prefixed with "group_" are propagated
        auto keys = _localBlackboard->GetKeys();
        uint32 propagatedCount = 0;

        for (auto const& k : keys)
        {
            // Only propagate keys intended for group sharing
            if (k.find("group_") == 0 || k.find("shared_") == 0)
            {
                ::std::any value;
                if (_localBlackboard->GetAny(k, value))
                {
                    groupBoard->SetAny(k, value);
                    ++propagatedCount;
                }
            }
        }

        TC_LOG_DEBUG("module.playerbot.blackboard",
            "PropagateToGroup: Propagated {} keys from bot {} to group {}",
            propagatedCount, GetBotGuid().GetCounter(), GetGroupId());
    }
    else
    {
        // Propagate specific key using type-erased transfer
        ::std::any value;
        if (_localBlackboard->GetAny(key, value))
        {
            groupBoard->SetAny(key, value);

            TC_LOG_DEBUG("module.playerbot.blackboard",
                "PropagateToGroup: Propagated key '{}' from bot {} to group {}",
                key, GetBotGuid().GetCounter(), GetGroupId());
        }
        else
        {
            TC_LOG_TRACE("module.playerbot.blackboard",
                "PropagateToGroup: Key '{}' not found in local blackboard",
                key);
        }
    }
}

void IntegratedAIContext::PullFromGroup(::std::string const& key)
{
    if (!IsInGroup())
        return;

    SharedBlackboard* groupBoard = GetGroupBlackboard();
    if (!groupBoard || !_localBlackboard)
        return;

    if (key.empty())
    {
        // Pull all keys from group
        auto keys = groupBoard->GetKeys();
        uint32 pulledCount = 0;

        for (auto const& k : keys)
        {
            ::std::any value;
            if (groupBoard->GetAny(k, value))
            {
                _localBlackboard->SetAny(k, value);
                ++pulledCount;
            }
        }

        TC_LOG_DEBUG("module.playerbot.blackboard",
            "PullFromGroup: Pulled {} keys from group {} to bot {}",
            pulledCount, GetGroupId(), GetBotGuid().GetCounter());
    }
    else
    {
        // Pull specific key
        ::std::any value;
        if (groupBoard->GetAny(key, value))
        {
            _localBlackboard->SetAny(key, value);

            TC_LOG_DEBUG("module.playerbot.blackboard",
                "PullFromGroup: Pulled key '{}' from group {} to bot {}",
                key, GetGroupId(), GetBotGuid().GetCounter());
        }
        else
        {
            TC_LOG_TRACE("module.playerbot.blackboard",
                "PullFromGroup: Key '{}' not found in group blackboard",
                key);
        }
    }
}

bool IntegratedAIContext::IsInGroup() const
{
    if (!_bot)
        return false;

    Player* player = _bot->GetBot();
    if (!player)
        return false;

    return player->GetGroup() != nullptr;
}

bool IntegratedAIContext::IsInRaid() const
{
    if (!_bot)
        return false;

    Player* player = _bot->GetBot();
    if (!player)
        return false;

    Group* group = player->GetGroup();
    return group && group->isRaidGroup();
}

ObjectGuid IntegratedAIContext::GetBotGuid() const
{
    if (!_bot)
        return ObjectGuid::Empty;

    Player* player = _bot->GetBot();
    if (!player)
        return ObjectGuid::Empty;

    return player->GetGUID();
}

uint32 IntegratedAIContext::GetGroupId() const
{
    if (!_bot)
        return 0;

    Player* player = _bot->GetBot();
    if (!player)
        return 0;

    Group* group = player->GetGroup();
    if (!group)
        return 0;

    return group->GetGUID().GetCounter();
}

uint32 IntegratedAIContext::GetRaidId() const
{
    return GetGroupId(); // Same as group ID for raids
}

uint32 IntegratedAIContext::GetZoneId() const
{
    if (!_bot)
        return 0;

    Player* player = _bot->GetBot();
    if (!player)
        return 0;

    return player->GetZoneId();
}

// ============================================================================
// Coordination-Aware BT Nodes
// ============================================================================

BTStatus BTCheckGroupFocusTarget::TickWithContext(IntegratedAIContext& context)
{
    SharedBlackboard* groupBoard = context.GetGroupBlackboard();
    if (!groupBoard)
        return BTStatus::FAILURE;

    ObjectGuid focusTarget;
    if (groupBoard->Get("focus_target", focusTarget))
    {
        // Store in local blackboard
        context.GetLocalBlackboard()->Set("current_target", focusTarget);
        return BTStatus::SUCCESS;
    }

    return BTStatus::FAILURE;
}

BTStatus BTFollowGroupStrategy::TickWithContext(IntegratedAIContext& context)
{
    Advanced::TacticalCoordinator* coordinator = context.GetTacticalCoordinator();
    if (!coordinator)
        return BTStatus::FAILURE;

    // Check for group directives
    ObjectGuid focusTarget = coordinator->GetFocusTarget();
    if (!focusTarget.IsEmpty())
    {
        context.GetLocalBlackboard()->Set("current_target", focusTarget);
        return BTStatus::SUCCESS;
    }

    return BTStatus::RUNNING;
}

BTStatus BTUseCoordinatedCooldown::TickWithContext(IntegratedAIContext& context)
{
    Coordination::RaidOrchestrator* raid = context.GetRaidOrchestrator();
    if (!raid)
        return BTStatus::FAILURE;

    // Check if it's our turn for this cooldown
    // This would integrate with RoleCoordinator cooldown rotation
    Coordination::HealerCoordinator* healers = raid->GetRoleCoordinatorManager()->GetHealerCoordinator();
    if (healers)
    {
        ObjectGuid nextHealer = healers->GetNextCooldownHealer(_cooldownType);
        if (nextHealer == context.GetBotGuid())
        {
            // It's our turn - use the cooldown
            context.GetLocalBlackboard()->Set("use_cooldown", _cooldownType);
            return BTStatus::SUCCESS;
        }
    }

    return BTStatus::FAILURE;
}

BTStatus BTAttackGroupFocusTarget::TickWithContext(IntegratedAIContext& context)
{
    SharedBlackboard* groupBoard = context.GetGroupBlackboard();
    if (!groupBoard)
        return BTStatus::FAILURE;

    ObjectGuid focusTarget;
    if (!groupBoard->Get("focus_target", focusTarget))
        return BTStatus::FAILURE;

    BotAI* ai = context.GetBotAI();
    if (!ai)
        return BTStatus::FAILURE;

    Player* bot = ai->GetBot();
    if (!bot)
        return BTStatus::FAILURE;

    Unit* target = ObjectAccessor::GetUnit(*bot, focusTarget);
    if (!target || !target->IsAlive())
        return BTStatus::FAILURE;

    // Attack the focus target
    bot->Attack(target, true);
    return BTStatus::SUCCESS;
}

BTStatus BTShareThreatInfo::TickWithContext(IntegratedAIContext& context)
{
    SharedBlackboard* groupBoard = context.GetGroupBlackboard();
    if (!groupBoard)
        return BTStatus::FAILURE;

    BotAI* ai = context.GetBotAI();
    if (!ai)
        return BTStatus::FAILURE;

    Player* bot = ai->GetBot();
    if (!bot)
        return BTStatus::FAILURE;

    // Share current threat level
    Unit* threat = bot->GetSelectedUnit();
    if (threat && bot->GetThreatManager().GetThreat(threat) > 0.0f)
    {
        float threatPct = bot->GetThreatManager().GetThreat(threat);
        groupBoard->Set("bot_" + context.GetBotGuid().ToString() + "_threat", threatPct);
        return BTStatus::SUCCESS;
    }

    return BTStatus::FAILURE;
}

BTStatus BTRequestGroupAssistance::TickWithContext(IntegratedAIContext& context)
{
    SharedBlackboard* groupBoard = context.GetGroupBlackboard();
    if (!groupBoard)
        return BTStatus::FAILURE;

    BotAI* ai = context.GetBotAI();
    if (!ai)
        return BTStatus::FAILURE;

    Player* bot = ai->GetBot();
    if (!bot)
        return BTStatus::FAILURE;

    // Request assistance if health is critical
    if (bot->GetHealthPct() < 30.0f)
    {
        groupBoard->Set("assistance_request", context.GetBotGuid());
        groupBoard->Set("assistance_priority", 100 - static_cast<int>(bot->GetHealthPct()));

        TC_LOG_DEBUG("playerbot.coordination", "Bot {} requesting group assistance ({}% health)",
            bot->GetName(), bot->GetHealthPct());

        return BTStatus::SUCCESS;
    }

    return BTStatus::FAILURE;
}

BTStatus BTRespondToZoneObjective::TickWithContext(IntegratedAIContext& context)
{
    Coordination::ZoneOrchestrator* zone = context.GetZoneOrchestrator();
    if (!zone)
        return BTStatus::FAILURE;

    auto objectives = zone->GetActiveObjectives();
    if (objectives.empty())
        return BTStatus::FAILURE;

    // Get highest priority objective
    auto const& objective = objectives[0];

    // Store objective position in blackboard
    context.GetLocalBlackboard()->Set("objective_position", objective.targetPosition);
    context.GetLocalBlackboard()->Set("objective_type", objective.objectiveType);

    return BTStatus::SUCCESS;
}

} // namespace Playerbot
