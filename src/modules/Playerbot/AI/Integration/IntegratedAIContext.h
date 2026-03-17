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

#ifndef TRINITYCORE_INTEGRATED_AI_CONTEXT_H
#define TRINITYCORE_INTEGRATED_AI_CONTEXT_H

#include "AI/BehaviorTree/BehaviorTree.h"
#include "AI/Blackboard/SharedBlackboard.h"
#include "AI/Coordination/RaidOrchestrator.h"
#include "AI/Coordination/ZoneOrchestrator.h"
#include "Define.h"
#include "ObjectGuid.h"
#include <memory>

namespace Playerbot
{

class BotAI;

// Forward declarations from Advanced namespace
namespace Advanced
{
    class TacticalCoordinator;
}

// Forward declarations from Coordination namespace
namespace Coordination
{
    class RaidOrchestrator;
    class ZoneOrchestrator;
}

/**
 * @brief Integrated AI Context
 * Provides unified access to all AI systems for bot decision-making
 *
 * This context is passed to Behavior Tree nodes, giving them access to:
 * - Local bot blackboard (BTBlackboard)
 * - Shared blackboards (Group, Raid, Zone)
 * - Coordination systems (Group, Raid, Zone)
 * - Bot AI reference
 */
class TC_GAME_API IntegratedAIContext
{
public:
    IntegratedAIContext(BotAI* bot, BTBlackboard* localBlackboard);
    ~IntegratedAIContext() = default;

    /**
     * @brief Get bot AI
     */
    BotAI* GetBotAI() const { return _bot; }

    /**
     * @brief Get local blackboard
     */
    BTBlackboard* GetLocalBlackboard() const { return _localBlackboard; }

    /**
     * @brief Get group shared blackboard
     */
    SharedBlackboard* GetGroupBlackboard() const;

    /**
     * @brief Get raid shared blackboard
     */
    SharedBlackboard* GetRaidBlackboard() const;

    /**
     * @brief Get zone shared blackboard
     */
    SharedBlackboard* GetZoneBlackboard() const;

    /**
     * @brief Get tactical coordinator (for combat coordination)
     */
    Advanced::TacticalCoordinator* GetTacticalCoordinator() const;

    /**
     * @brief Get raid orchestrator
     */
    Coordination::RaidOrchestrator* GetRaidOrchestrator() const;

    /**
     * @brief Get zone orchestrator
     */
    Coordination::ZoneOrchestrator* GetZoneOrchestrator() const;

    /**
     * @brief Propagate local data to group
     * @param key Key to propagate (empty = all)
     */
    void PropagateToGroup(::std::string const& key = "");

    /**
     * @brief Pull group data to local
     * @param key Key to pull (empty = all)
     */
    void PullFromGroup(::std::string const& key = "");

    /**
     * @brief Check if bot is in group
     */
    bool IsInGroup() const;

    /**
     * @brief Check if bot is in raid
     */
    bool IsInRaid() const;

    /**
     * @brief Get bot GUID
     */
    ObjectGuid GetBotGuid() const;

    /**
     * @brief Get group ID
     */
    uint32 GetGroupId() const;

    /**
     * @brief Get raid ID
     */
    uint32 GetRaidId() const;

    /**
     * @brief Get zone ID
     */
    uint32 GetZoneId() const;

private:
    BotAI* _bot;
    BTBlackboard* _localBlackboard;

    // Cached references (initialized on first access)
    mutable SharedBlackboard* _cachedGroupBlackboard = nullptr;
    mutable SharedBlackboard* _cachedRaidBlackboard = nullptr;
    mutable SharedBlackboard* _cachedZoneBlackboard = nullptr;
    mutable Advanced::TacticalCoordinator* _cachedTacticalCoordinator = nullptr;
    mutable Coordination::RaidOrchestrator* _cachedRaidOrchestrator = nullptr;
    mutable Coordination::ZoneOrchestrator* _cachedZoneOrchestrator = nullptr;
};

/**
 * @brief Coordination-Aware BT Node
 * Base class for BT nodes that need coordination system access
 */
class TC_GAME_API CoordinationBTNode : public BTNode
{
public:
    CoordinationBTNode(::std::string const& name) : BTNode(name) {}

    /**
     * @brief Tick with integrated context
     * @param context Integrated AI context
     * @return Execution status
     */
    virtual BTStatus TickWithContext(IntegratedAIContext& context) = 0;

    /**
     * @brief Standard tick (delegates to TickWithContext)
     */
    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override final
    {
        IntegratedAIContext context(ai, &blackboard);
        return TickWithContext(context);
    }
};

/**
 * @brief Check Group Focus Target Node
 * Reads focus target from group blackboard
 */
class TC_GAME_API BTCheckGroupFocusTarget : public CoordinationBTNode
{
public:
    BTCheckGroupFocusTarget() : CoordinationBTNode("CheckGroupFocusTarget") {}

    BTStatus TickWithContext(IntegratedAIContext& context);
};

/**
 * @brief Follow Group Strategy Node
 * Executes behavior based on group coordinator directives
 */
class TC_GAME_API BTFollowGroupStrategy : public CoordinationBTNode
{
public:
    BTFollowGroupStrategy() : CoordinationBTNode("FollowGroupStrategy") {}

    BTStatus TickWithContext(IntegratedAIContext& context);
};

/**
 * @brief Use Coordinated Cooldown Node
 * Uses cooldown based on raid coordinator rotation
 */
class TC_GAME_API BTUseCoordinatedCooldown : public CoordinationBTNode
{
public:
    BTUseCoordinatedCooldown(::std::string const& cooldownType)
        : CoordinationBTNode("UseCoordinatedCooldown")
        , _cooldownType(cooldownType)
    {
    }

    BTStatus TickWithContext(IntegratedAIContext& context);

private:
    ::std::string _cooldownType;
};

/**
 * @brief Attack Group Focus Target Node
 * Attacks the target designated by group coordinator
 */
class TC_GAME_API BTAttackGroupFocusTarget : public CoordinationBTNode
{
public:
    BTAttackGroupFocusTarget() : CoordinationBTNode("AttackGroupFocusTarget") {}

    BTStatus TickWithContext(IntegratedAIContext& context);
};

/**
 * @brief Share Threat Information Node
 * Shares current threat information to group blackboard
 */
class TC_GAME_API BTShareThreatInfo : public CoordinationBTNode
{
public:
    BTShareThreatInfo() : CoordinationBTNode("ShareThreatInfo") {}

    BTStatus TickWithContext(IntegratedAIContext& context);
};

/**
 * @brief Request Group Assistance Node
 * Requests help from group when in danger
 */
class TC_GAME_API BTRequestGroupAssistance : public CoordinationBTNode
{
public:
    BTRequestGroupAssistance() : CoordinationBTNode("RequestGroupAssistance") {}

    BTStatus TickWithContext(IntegratedAIContext& context);
};

/**
 * @brief Respond to Zone Objective Node
 * Moves to zone objective if assigned
 */
class TC_GAME_API BTRespondToZoneObjective : public CoordinationBTNode
{
public:
    BTRespondToZoneObjective() : CoordinationBTNode("RespondToZoneObjective") {}

    BTStatus TickWithContext(IntegratedAIContext& context);
};

} // namespace Playerbot

#endif // TRINITYCORE_INTEGRATED_AI_CONTEXT_H
