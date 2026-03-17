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

#ifndef PLAYERBOT_IMANAGERBASE_H
#define PLAYERBOT_IMANAGERBASE_H

#include "Define.h"
#include "Events/BotEventTypes.h"
#include <string>

namespace Playerbot
{

/**
 * @brief Base interface for all manager classes in the Playerbot system
 *
 * This interface provides the foundation for the manager architecture,
 * enabling event-driven communication between observers and managers.
 * All managers (QuestManager, TradeManager, MovementManager, etc.) should
 * implement this interface to participate in the event system.
 *
 * Phase 7.1: Observer-Manager Integration Layer
 *
 * Architecture:
 * - Observers detect events (Phase 6)
 * - EventDispatcher routes events to managers (Phase 7.1)
 * - Managers implement this interface to handle events (Phase 7.1+)
 *
 * Thread Safety:
 * - All OnEvent() implementations MUST be thread-safe
 * - Events may be dispatched from any thread
 * - Use appropriate locking mechanisms when accessing shared state
 *
 * Performance Requirements:
 * - OnEvent() should complete in <1ms for most events
 * - Heavy processing should be deferred to Update() cycle
 * - Use event priority to control processing order
 */
class TC_GAME_API IManagerBase
{
public:
    virtual ~IManagerBase() = default;

    /**
     * @brief Initialize the manager
     *
     * Called once when the manager is created or when the bot AI is initialized.
     * Perform one-time setup, resource allocation, and initial state configuration.
     *
     * @return true if initialization succeeded, false otherwise
     *
     * Thread Safety: Called from main thread during bot creation
     * Performance: Should complete in <10ms
     */
    virtual bool Initialize() = 0;

    /**
     * @brief Shutdown the manager
     *
     * Called once when the manager is being destroyed or when the bot AI is shutting down.
     * Cleanup resources, save state if necessary, and perform graceful shutdown.
     *
     * Thread Safety: Called from main thread during bot cleanup
     * Performance: Should complete in <50ms
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Update manager state
     *
     * Called periodically by BotAI to allow managers to process deferred work,
     * update internal state, and perform time-based actions.
     *
     * @param diff Time elapsed since last update in milliseconds
     *
     * Thread Safety: Called from world update thread
     * Performance: Should complete in <0.1ms per call to maintain <0.1% CPU target
     *
     * Note: Managers should throttle expensive operations and spread work across
     * multiple update cycles to avoid CPU spikes.
     */
    virtual void Update(uint32 diff) = 0;

    /**
     * @brief Handle an event dispatched from an observer
     *
     * This is the primary integration point between Phase 6 observers and Phase 7+ managers.
     * When an observer detects an event, it dispatches it through the EventDispatcher,
     * which routes it to all subscribed managers via this method.
     *
     * @param event The event to handle
     *
     * Thread Safety: MUST be thread-safe - may be called from any thread
     * Performance: Should complete in <1ms for most events
     *
     * Implementation Guidelines:
     * - Use switch statements on event.type for efficient dispatch
     * - Validate event data before processing
     * - Defer heavy processing to Update() cycle
     * - Return quickly to avoid blocking the event queue
     * - Log errors appropriately using TC_LOG_ERROR
     *
     * Example:
     * @code
     * void QuestManager::OnEvent(BotEvent const& event)
     * {
     *     switch (event.type)
     *     {
     *         case StateMachine::EventType::QUEST_ACCEPTED:
     *             HandleQuestAccepted(event);
     *             break;
     *         case StateMachine::EventType::QUEST_COMPLETED:
     *             HandleQuestCompleted(event);
     *             break;
     *         default:
     *             break;
     *     }
     * }
     * @endcode
     */
    virtual void OnEvent(Events::BotEvent const& event) = 0;

    /**
     * @brief Get the manager's unique identifier
     *
     * Used by ManagerRegistry for tracking and debugging.
     * Should return a stable, unique string identifier.
     *
     * @return Manager identifier (e.g., "QuestManager", "TradeManager")
     *
     * Thread Safety: Must be thread-safe (const method)
     * Performance: Should be a simple string return
     */
    virtual ::std::string GetManagerId() const = 0;

    /**
     * @brief Check if the manager is currently active
     *
     * Inactive managers may skip expensive operations or event handling.
     * For example, a TradeManager might be inactive when the bot is in combat.
     *
     * @return true if the manager should process events and updates, false otherwise
     *
     * Thread Safety: Must be thread-safe (const method)
     * Performance: Should be a simple boolean check
     */
    virtual bool IsActive() const = 0;

    /**
     * @brief Get the manager's update interval in milliseconds
     *
     * Managers can specify different update frequencies based on their needs.
     * The BotAI system will throttle Update() calls to this interval.
     *
     * @return Update interval in milliseconds (e.g., 1000 for 1 second)
     *
     * Thread Safety: Must be thread-safe (const method)
     * Performance: Should be a simple integer return
     *
     * Recommended Intervals:
     * - Combat managers: 100-200ms (fast response)
     * - Movement managers: 100ms (smooth movement)
     * - Quest managers: 2000ms (infrequent checks)
     * - Trade managers: 1000ms (moderate response)
     * - Social managers: 5000ms (low priority)
     */
    virtual uint32 GetUpdateInterval() const = 0;

protected:
    /**
     * @brief Protected constructor - only derived classes can instantiate
     */
    IManagerBase() = default;

    /**
     * @brief Disable copy construction
     */
    IManagerBase(IManagerBase const&) = delete;

    /**
     * @brief Disable copy assignment
     */
    IManagerBase& operator=(IManagerBase const&) = delete;
};

} // namespace Playerbot

#endif // PLAYERBOT_IMANAGERBASE_H
