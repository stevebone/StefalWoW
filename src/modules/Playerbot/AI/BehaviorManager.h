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

#ifndef TRINITYCORE_BEHAVIOR_MANAGER_H
#define TRINITYCORE_BEHAVIOR_MANAGER_H

#include "Define.h"
#include "Core/Managers/IManagerBase.h"
#include "Events/BotEventTypes.h"
#include <atomic>
#include <string>
#include <chrono>

// Forward declarations
class Player;

namespace Playerbot
{
    class BotAI;
    /**
     * @class BehaviorManager
     * @brief Base class for all bot behavior managers providing throttled update mechanism
     *
     * **PHASE 7.1 UPDATE**: Now implements IManagerBase for event-driven architecture integration
     *
     * This class implements a throttling system where Update() is called every frame but
     * OnUpdate() is only invoked at configured intervals. All bot managers (QuestManager,
     * TradeManager, CombatManager, etc.) should inherit from this base class.
     *
     * Features:
     * - Automatic update throttling to reduce CPU usage
     * - Atomic state flags for lock-free queries from strategies
     * - Performance monitoring with automatic slow-update detection
     * - Per-bot instance design (no singletons)
     *
     * Performance characteristics:
     * - Update() when throttled: <0.001ms (just timestamp check)
     * - OnUpdate() cost: 5-10ms acceptable (implementation dependent)
     * - Amortized per-frame cost: <0.2ms
     * - State queries: <0.001ms (atomic read operations)
     *
     * Usage example:
     * @code
     * class QuestManager : public BehaviorManager
     * {
     * protected:
     *     void OnUpdate(uint32 elapsed) override
     *     {
     *         // Perform quest-related updates
     *         UpdateQuestProgress();
     *         CheckQuestCompletion();
     *     }
     * };
     * @endcode
     */
    class TC_GAME_API BehaviorManager : public IManagerBase
    {
    public:
        /**
         * @brief Construct a new BehaviorManager
         * @param bot The bot player this manager belongs to (must not be null)
         * @param ai The bot AI controller (must not be null)
         * @param updateInterval Update interval in milliseconds (default: 1000ms)
         * @param managerName Name for logging purposes (e.g., "QuestManager")
         */
        explicit BehaviorManager(Player* bot, BotAI* ai, uint32 updateInterval = 1000,
                                 ::std::string managerName = "BehaviorManager");

        /**
         * @brief Virtual destructor for proper cleanup of derived classes
         */
        virtual ~BehaviorManager() = default;

        /**
         * @brief Called every frame to check if OnUpdate should be invoked
         *
         * This method performs minimal work (timestamp comparison) when update
         * is not needed, ensuring <0.001ms overhead when throttled.
         *
         * @param diff Time elapsed since last Update call in milliseconds
         */
        void Update(uint32 diff);

        /**
         * @brief Check if this manager is currently enabled
         * @return true if the manager is processing updates, false otherwise
         *
         * Thread-safe: Uses atomic operations for lock-free access
         * Performance: <0.001ms guaranteed
         */
        bool IsEnabled() const { return m_enabled.load(::std::memory_order_acquire); }

        /**
         * @brief Enable or disable this manager
         * @param enable true to enable updates, false to disable
         *
         * Thread-safe: Uses atomic operations
         */
        void SetEnabled(bool enable) { m_enabled.store(enable, ::std::memory_order_release); }

        /**
         * @brief Check if the manager is currently busy processing an update
         * @return true if OnUpdate() is currently executing, false otherwise
         *
         * Thread-safe: Uses atomic operations for lock-free access
         * Performance: <0.001ms guaranteed
         */
        bool IsBusy() const { return m_isBusy.load(::std::memory_order_acquire); }

        /**
         * @brief Get the configured update interval
         * @return Update interval in milliseconds
         */
        uint32 GetUpdateInterval() const { return m_updateInterval; }

        /**
         * @brief Set a new update interval
         * @param interval New interval in milliseconds (clamped to 50ms minimum)
         */
        void SetUpdateInterval(uint32 interval);

        /**
         * @brief Get the manager name for debugging/logging
         * @return Manager name string
         */
        const ::std::string& GetManagerName() const { return m_managerName; }

        /**
         * @brief Get time since last successful update
         * @return Milliseconds since last OnUpdate() completion
         */
        uint32 GetTimeSinceLastUpdate() const;

        /**
         * @brief Force an immediate update on next Update() call
         *
         * Use sparingly as this bypasses throttling mechanism
         */
        void ForceUpdate() { m_forceUpdate.store(true, ::std::memory_order_release); }

        /**
         * @brief Check if manager has been initialized
         * @return true if ready for updates, false otherwise
         *
         * Thread-safe: Uses atomic operations for lock-free access
         * Performance: <0.001ms guaranteed
         */
        bool IsInitialized() const { return m_initialized.load(::std::memory_order_acquire); }

        // Delete copy constructor and assignment operator to prevent copying
        BehaviorManager(const BehaviorManager&) = delete;
        BehaviorManager& operator=(const BehaviorManager&) = delete;

        // Delete move constructor and assignment operator for simplicity
        BehaviorManager(BehaviorManager&&) = delete;
        BehaviorManager& operator=(BehaviorManager&&) = delete;

        // ========================================================================
        // PHASE 7.1: IManagerBase Interface Implementation
        // ========================================================================

        /**
         * @brief Initialize the manager (IManagerBase interface)
         * @return true if initialization successful, false otherwise
         *
         * This calls the virtual OnInitialize() method for derived class initialization
         */
        bool Initialize();

        /**
         * @brief Shutdown the manager (IManagerBase interface)
         *
         * This calls the virtual OnShutdown() method for derived class cleanup
         */
        void Shutdown();

        /**
         * @brief Handle an event from the EventDispatcher (IManagerBase interface)
         * @param event The event to handle
         *
         * This method delegates to the virtual OnEventInternal() for derived classes to override.
         */
        void OnEvent(Events::BotEvent const& event);

        /**
         * @brief Get the manager's unique identifier (IManagerBase interface)
         * @return Manager identifier string
         */
        ::std::string GetManagerId() const override { return m_managerName; }

        /**
         * @brief Check if the manager is currently active (IManagerBase interface)
         * @return true if active, false otherwise
         */
        bool IsActive() const override { return IsEnabled() && IsInitialized(); }

    protected:
        /**
         * @brief Virtual method called at throttled intervals for actual work
         *
         * Derived classes must implement this method to perform their specific
         * behavior updates. This method is only called when:
         * - The manager is enabled
         * - Sufficient time has passed since last update
         * - The bot and AI pointers are valid
         *
         * @param elapsed Time elapsed since last OnUpdate in milliseconds
         *
         * @note Implementations should aim for 5-10ms execution time
         * @note Updates taking >50ms will trigger performance warnings
         */
        virtual void OnUpdate(uint32 elapsed) = 0;

        /**
         * @brief Called once during first Update() for initialization
         *
         * Override this method to perform one-time initialization that
         * requires the bot to be fully loaded in the world.
         *
         * @return true if initialization successful, false to retry next update
         */
        virtual bool OnInitialize() { return true; }

        /**
         * @brief Called when manager is being shut down
         *
         * Override to perform cleanup operations. Called from destructor
         * or when bot is being removed from world.
         */
        virtual void OnShutdown() { }

        /**
         * @brief Called when an event is dispatched to this manager (Phase 7.1)
         *
         * Override this method in derived classes to handle specific events.
         * Default implementation does nothing - managers opt-in to event handling.
         *
         * @param event The event to handle
         *
         * Example:
         * @code
         * void QuestManager::OnEvent(Events::BotEvent const& event)
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
        virtual void OnEventInternal(Events::BotEvent const& /*event*/) { }

        /**
         * @brief Get the bot player this manager belongs to
         * @return Pointer to bot Player (may be null if bot disconnected)
         */
        Player* GetBot() const { return m_bot; }

        /**
         * @brief Get the bot AI controller
         * @return Pointer to BotAI (may be null if AI destroyed)
         */
        BotAI* GetAI() const { return m_ai; }

        /**
         * @brief Log a debug message with manager prefix
         * @param format Printf-style format string
         * @param args Format arguments
         */
        template<typename... Args>
        void LogDebug(const char* format, Args... args) const;

        /**
         * @brief Log a warning message with manager prefix
         * @param format Printf-style format string
         * @param args Format arguments
         */
        template<typename... Args>
        void LogWarning(const char* format, Args... args) const;

        // Protected state flags for derived classes (atomic for thread safety)
        ::std::atomic<bool> m_hasWork{false};        ///< Set when manager has pending work
        ::std::atomic<bool> m_needsUpdate{false};    ///< Set when immediate update needed
        ::std::atomic<uint32> m_updateCount{0};      ///< Total number of OnUpdate calls

    private:
        Player* m_bot;                              ///< The bot this manager belongs to
        BotAI* m_ai;                                ///< The AI controller
        ::std::string m_managerName;                  ///< Name for logging/debugging

        uint32 m_updateInterval;                    ///< Update interval in milliseconds
        uint32 m_lastUpdate;                        ///< Timestamp of last update (getMSTime)
        uint32 m_timeSinceLastUpdate;              ///< Accumulated time since last update

        ::std::atomic<bool> m_enabled{true};         ///< Whether manager is enabled
        ::std::atomic<bool> m_initialized{false};    ///< Whether OnInitialize has succeeded
        ::std::atomic<bool> m_isBusy{false};         ///< Whether currently in OnUpdate
        ::std::atomic<bool> m_forceUpdate{false};    ///< Force update on next tick

        uint32 m_slowUpdateThreshold{50};          ///< Threshold for slow update warning (ms)
        uint32 m_consecutiveSlowUpdates{0};        ///< Counter for consecutive slow updates
        uint32 m_totalSlowUpdates{0};              ///< Total slow updates since creation

        /**
         * @brief Internal update implementation with performance monitoring
         * @param elapsed Time since last update in milliseconds
         */
        void DoUpdate(uint32 elapsed);

        /**
         * @brief Check if bot and AI pointers are still valid
         * @return true if both pointers are valid, false otherwise
         */
        bool ValidatePointers() const;
    };

} // namespace Playerbot

#endif // TRINITYCORE_BEHAVIOR_MANAGER_H