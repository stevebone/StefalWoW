/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_GROUP_EVENT_HANDLER_H
#define PLAYERBOT_GROUP_EVENT_HANDLER_H

#include "Core/Events/GenericEventBus.h"
#include "GroupEvents.h"
#include "Define.h"
#include <string>
#include <memory>

// Forward declarations
class Player;
class Group;

namespace Playerbot
{

class BotAI;

/**
 * @class GroupEventHandler
 * @brief Base class for all group event handlers
 *
 * This abstract class defines the interface for handling specific group events.
 * Each concrete handler implements the logic for responding to one type of
 * group event (e.g., member joined, loot changed, etc.).
 *
 * **Design Pattern**: Strategy Pattern
 * - Each handler encapsulates one specific event handling strategy
 * - Handlers are registered with GroupEventBus for specific event types
 * - BotAI can have multiple handlers for different aspects of group behavior
 *
 * **Handler Lifecycle**:
 * 1. Created when bot joins group (or group is created with bot)
 * 2. Registered with GroupEventBus for specific event types
 * 3. Receives events via HandleEvent() callback
 * 4. Unregistered when bot leaves group or is destroyed
 *
 * **Thread Safety**:
 * - HandleEvent() may be called from World update thread
 * - Handlers must be thread-safe if accessing shared state
 * - Use bot's internal synchronization mechanisms
 */
class TC_GAME_API GroupEventHandler
{
public:
    virtual ~GroupEventHandler() = default;

    /**
     * Handle a group event
     *
     * @param event The event to handle
     * @return true if event was handled successfully, false otherwise
     *
     * Implementation guidelines:
     * - Return false only for critical errors
     * - Log important actions at DEBUG level
     * - Keep processing time <1ms for performance
     * - Don't block on I/O or database operations
     */
    virtual bool HandleEvent(GroupEvent const& event) = 0;

    /**
     * Get handler name for debugging and logging
     * @return Human-readable handler name
     */
    virtual ::std::string GetHandlerName() const = 0;

    /**
     * Get event types this handler subscribes to
     * @return Vector of event types
     */
    virtual ::std::vector<GroupEventType> GetSubscribedEvents() const = 0;

    /**
     * Check if this handler can process a specific event type
     * @param type Event type to check
     * @return true if handler processes this event type
     */
    virtual bool CanHandle(GroupEventType type) const
    {
        auto subscribedEvents = GetSubscribedEvents();
        return ::std::find(subscribedEvents.begin(), subscribedEvents.end(), type) != subscribedEvents.end();
    }

    /**
     * Get the BotAI this handler belongs to
     * @return BotAI instance (may be null if handler is global)
     */
    BotAI* GetBotAI() const { return _botAI; }

    /**
     * Set the BotAI this handler belongs to
     * @param botAI BotAI instance
     */
    void SetBotAI(BotAI* botAI) { _botAI = botAI; }

protected:
    /**
     * Protected constructor - only derived classes can instantiate
     * @param botAI The BotAI this handler belongs to (optional, can be null for global handlers)
     */
    explicit GroupEventHandler(BotAI* botAI = nullptr) : _botAI(botAI) { }

    /**
     * Helper: Get bot's player object
     * @return Player* if bot AI is valid and bot is online, nullptr otherwise
     */
    Player* GetBotPlayer() const;

    /**
     * Helper: Get group the bot is in
     * @return Group* if bot is in a group, nullptr otherwise
     */
    Group* GetBotGroup() const;

    /**
     * Helper: Check if bot is in the event's group
     * @param event Event to check
     * @return true if bot is in the group
     */
    bool IsBotInEventGroup(GroupEvent const& event) const;

    /**
     * Helper: Log event handling
     * @param event Event being handled
     * @param action Action taken
     */
    void LogEventHandling(GroupEvent const& event, ::std::string const& action) const;

private:
    BotAI* _botAI; ///< The BotAI this handler belongs to
};

// ============================================================================
// CONCRETE EVENT HANDLERS
// ============================================================================

/**
 * @class MemberJoinedHandler
 * @brief Handles MEMBER_JOINED events
 *
 * Responsibilities:
 * - Update bot's group member cache
 * - Greet new member (if social features enabled)
 * - Adjust formation to accommodate new member
 * - Update role assignments if needed
 */
class TC_GAME_API MemberJoinedHandler : public GroupEventHandler
{
public:
    explicit MemberJoinedHandler(BotAI* botAI) : GroupEventHandler(botAI) { }

    bool HandleEvent(GroupEvent const& event);
    ::std::string GetHandlerName() const override { return "MemberJoinedHandler"; }
    ::std::vector<GroupEventType> GetSubscribedEvents() const override
    {
        return { GroupEventType::MEMBER_JOINED };
    }
};

/**
 * @class MemberLeftHandler
 * @brief Handles MEMBER_LEFT events
 *
 * Responsibilities:
 * - Update bot's group member cache
 * - Adjust formation after member leaves
 * - Take over roles if departed member had important role
 * - Disband group if too few members remain
 */
class TC_GAME_API MemberLeftHandler : public GroupEventHandler
{
public:
    explicit MemberLeftHandler(BotAI* botAI) : GroupEventHandler(botAI) { }

    bool HandleEvent(GroupEvent const& event);
    ::std::string GetHandlerName() const override { return "MemberLeftHandler"; }
    ::std::vector<GroupEventType> GetSubscribedEvents() const override
    {
        return { GroupEventType::MEMBER_LEFT };
    }
};

/**
 * @class LeaderChangedHandler
 * @brief Handles LEADER_CHANGED events
 *
 * Responsibilities:
 * - Update bot's leader reference
 * - Follow new leader if bot is following old leader
 * - Update assist target if leader is main assist
 * - Adjust bot behavior based on new leader's commands
 */
class TC_GAME_API LeaderChangedHandler : public GroupEventHandler
{
public:
    explicit LeaderChangedHandler(BotAI* botAI) : GroupEventHandler(botAI) { }

    bool HandleEvent(GroupEvent const& event);
    ::std::string GetHandlerName() const override { return "LeaderChangedHandler"; }
    ::std::vector<GroupEventType> GetSubscribedEvents() const override
    {
        return { GroupEventType::LEADER_CHANGED };
    }
};

/**
 * @class GroupDisbandedHandler
 * @brief Handles GROUP_DISBANDED events
 *
 * Responsibilities:
 * - Clean up all group-related state
 * - Stop following group members
 * - Clear combat coordination state
 * - Return to idle behavior
 */
class TC_GAME_API GroupDisbandedHandler : public GroupEventHandler
{
public:
    explicit GroupDisbandedHandler(BotAI* botAI) : GroupEventHandler(botAI) { }

    bool HandleEvent(GroupEvent const& event);
    ::std::string GetHandlerName() const override { return "GroupDisbandedHandler"; }
    ::std::vector<GroupEventType> GetSubscribedEvents() const override
    {
        return { GroupEventType::GROUP_DISBANDED };
    }
};

/**
 * @class LootMethodChangedHandler
 * @brief Handles LOOT_METHOD_CHANGED, LOOT_THRESHOLD_CHANGED, MASTER_LOOTER_CHANGED
 *
 * Responsibilities:
 * - Update bot's loot behavior
 * - Pass loot if not eligible under new rules
 * - Respect master looter assignments
 * - Handle round-robin loot rotation
 */
class TC_GAME_API LootMethodChangedHandler : public GroupEventHandler
{
public:
    explicit LootMethodChangedHandler(BotAI* botAI) : GroupEventHandler(botAI) { }

    bool HandleEvent(GroupEvent const& event);
    ::std::string GetHandlerName() const override { return "LootMethodChangedHandler"; }
    ::std::vector<GroupEventType> GetSubscribedEvents() const override
    {
        return {
            GroupEventType::LOOT_METHOD_CHANGED,
            GroupEventType::LOOT_THRESHOLD_CHANGED,
            GroupEventType::MASTER_LOOTER_CHANGED
        };
    }
};

/**
 * @class TargetIconChangedHandler
 * @brief Handles TARGET_ICON_CHANGED events
 *
 * Responsibilities:
 * - Update bot's target priority based on raid icons
 * - Skull = kill first, X = sheep/CC, etc.
 * - Coordinate focus fire on marked targets
 * - Clear old target if icon removed
 */
class TC_GAME_API TargetIconChangedHandler : public GroupEventHandler
{
public:
    explicit TargetIconChangedHandler(BotAI* botAI) : GroupEventHandler(botAI) { }

    bool HandleEvent(GroupEvent const& event);
    ::std::string GetHandlerName() const override { return "TargetIconChangedHandler"; }
    ::std::vector<GroupEventType> GetSubscribedEvents() const override
    {
        return { GroupEventType::TARGET_ICON_CHANGED };
    }
};

/**
 * @class ReadyCheckHandler
 * @brief Handles READY_CHECK_STARTED, READY_CHECK_RESPONSE, READY_CHECK_COMPLETED
 *
 * Responsibilities:
 * - Respond to ready checks automatically
 * - Check if bot is ready (health, mana, cooldowns)
 * - Notify group if not ready
 * - Prepare for encounter after successful ready check
 */
class TC_GAME_API ReadyCheckHandler : public GroupEventHandler
{
public:
    explicit ReadyCheckHandler(BotAI* botAI) : GroupEventHandler(botAI) { }

    bool HandleEvent(GroupEvent const& event);
    ::std::string GetHandlerName() const override { return "ReadyCheckHandler"; }
    ::std::vector<GroupEventType> GetSubscribedEvents() const override
    {
        return {
            GroupEventType::READY_CHECK_STARTED,
            GroupEventType::READY_CHECK_RESPONSE,
            GroupEventType::READY_CHECK_COMPLETED
        };
    }
};

/**
 * @class RaidConvertedHandler
 * @brief Handles RAID_CONVERTED events
 *
 * Responsibilities:
 * - Update bot formation (party formation vs raid subgroups)
 * - Enable/disable raid-specific abilities
 * - Update healing/buffing priorities for raid
 * - Adjust positioning for 25/40-man content
 */
class TC_GAME_API RaidConvertedHandler : public GroupEventHandler
{
public:
    explicit RaidConvertedHandler(BotAI* botAI) : GroupEventHandler(botAI) { }

    bool HandleEvent(GroupEvent const& event);
    ::std::string GetHandlerName() const override { return "RaidConvertedHandler"; }
    ::std::vector<GroupEventType> GetSubscribedEvents() const override
    {
        return { GroupEventType::RAID_CONVERTED };
    }
};

/**
 * @class SubgroupChangedHandler
 * @brief Handles SUBGROUP_CHANGED events
 *
 * Responsibilities:
 * - Update bot's subgroup awareness
 * - Adjust healing priority (prioritize own subgroup)
 * - Update buff distribution (chain heal, prayer of mending, etc.)
 * - Maintain proximity to subgroup members
 */
class TC_GAME_API SubgroupChangedHandler : public GroupEventHandler
{
public:
    explicit SubgroupChangedHandler(BotAI* botAI) : GroupEventHandler(botAI) { }

    bool HandleEvent(GroupEvent const& event);
    ::std::string GetHandlerName() const override { return "SubgroupChangedHandler"; }
    ::std::vector<GroupEventType> GetSubscribedEvents() const override
    {
        return { GroupEventType::SUBGROUP_CHANGED };
    }
};

/**
 * @class RoleAssignmentHandler
 * @brief Handles ASSISTANT_CHANGED, MAIN_TANK_CHANGED, MAIN_ASSIST_CHANGED
 *
 * Responsibilities:
 * - Update bot's role awareness
 * - Follow main tank if tank dies
 * - Assist main assist's target
 * - Enable leader assist powers if promoted
 */
class TC_GAME_API RoleAssignmentHandler : public GroupEventHandler
{
public:
    explicit RoleAssignmentHandler(BotAI* botAI) : GroupEventHandler(botAI) { }

    bool HandleEvent(GroupEvent const& event);
    ::std::string GetHandlerName() const override { return "RoleAssignmentHandler"; }
    ::std::vector<GroupEventType> GetSubscribedEvents() const override
    {
        return {
            GroupEventType::ASSISTANT_CHANGED,
            GroupEventType::MAIN_TANK_CHANGED,
            GroupEventType::MAIN_ASSIST_CHANGED
        };
    }
};

/**
 * @class DifficultyChangedHandler
 * @brief Handles DIFFICULTY_CHANGED events
 *
 * Responsibilities:
 * - Update bot's combat difficulty expectations
 * - Adjust cooldown usage for heroic/mythic
 * - Update consumable usage thresholds
 * - Warn if bot is undergeared for new difficulty
 */
class TC_GAME_API DifficultyChangedHandler : public GroupEventHandler
{
public:
    explicit DifficultyChangedHandler(BotAI* botAI) : GroupEventHandler(botAI) { }

    bool HandleEvent(GroupEvent const& event);
    ::std::string GetHandlerName() const override { return "DifficultyChangedHandler"; }
    ::std::vector<GroupEventType> GetSubscribedEvents() const override
    {
        return { GroupEventType::DIFFICULTY_CHANGED };
    }
};

// ============================================================================
// HANDLER FACTORY
// ============================================================================

/**
 * @class GroupEventHandlerFactory
 * @brief Factory for creating all group event handlers for a BotAI
 *
 * Provides a centralized way to create and register all handlers
 * for a bot when it joins a group.
 */
class TC_GAME_API GroupEventHandlerFactory
{
public:
    /**
     * Create all event handlers for a BotAI
     * @param botAI The BotAI to create handlers for
     * @return Vector of created handlers (owned by caller)
     */
    static ::std::vector<::std::unique_ptr<GroupEventHandler>> CreateAllHandlers(BotAI* botAI);

    /**
     * Register all handlers with GroupEventBus
     * @param handlers Vector of handlers to register
     * @param botAI The BotAI to register for
     */
    static void RegisterHandlers(::std::vector<::std::unique_ptr<GroupEventHandler>> const& handlers, BotAI* botAI);

    /**
     * Unregister all handlers from GroupEventBus
     * @param botAI The BotAI to unregister
     */
    static void UnregisterHandlers(BotAI* botAI);
};

} // namespace Playerbot

#endif // PLAYERBOT_GROUP_EVENT_HANDLER_H
