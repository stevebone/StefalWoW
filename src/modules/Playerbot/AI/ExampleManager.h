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

#ifndef TRINITYCORE_EXAMPLE_MANAGER_H
#define TRINITYCORE_EXAMPLE_MANAGER_H

#include "BehaviorManager.h"
#include <atomic>
#include <deque>
#include <memory>
#include <chrono>

class Player;

namespace Playerbot
{
    class BotAI;
    /**
     * @class ExampleManager
     * @brief Example implementation of BehaviorManager showing best practices
     *
     * This class demonstrates the proper way to implement a BehaviorManager subclass,
     * including:
     * - Proper use of atomic state flags for lock-free queries
     * - Efficient OnUpdate implementation
     * - State management patterns
     * - Performance-conscious design
     *
     * This example simulates a simple task queue manager that processes
     * tasks for the bot with proper throttling and state management.
     */
    class TC_GAME_API ExampleManager : public BehaviorManager
    {
    public:
        /**
         * @brief Example task structure
         */
        struct Task
        {
            enum Type
            {
                TASK_IDLE,
                TASK_MOVE,
                TASK_INTERACT,
                TASK_WAIT
            };

            Type type;
            uint32 targetId;
            uint32 duration;
            ::std::chrono::steady_clock::time_point startTime;

            Task(Type t, uint32 id = 0, uint32 dur = 1000)
                : type(t), targetId(id), duration(dur)
                , startTime(::std::chrono::steady_clock::now()) {}
        };

        /**
         * @brief Construct ExampleManager
         * @param bot The bot player (must not be null)
         * @param ai The bot AI controller (must not be null)
         */
        explicit ExampleManager(Player* bot, BotAI* ai);

        /**
         * @brief Destructor
         */
        ~ExampleManager();

        /**
         * @brief Add a task to the queue
         * @param task Task to add
         * @return true if task was added, false if queue is full
         *
         * Thread-safe: Can be called from any thread
         * Performance: <0.001ms
         */
        bool AddTask(const Task& task);

        /**
         * @brief Check if manager has pending tasks
         * @return true if tasks are queued or being processed
         *
         * Thread-safe: Lock-free atomic operation
         * Performance: <0.001ms guaranteed
         */
        bool HasPendingTasks() const { return m_hasTasks.load(::std::memory_order_acquire); }

        /**
         * @brief Check if currently processing a task
         * @return true if a task is active
         *
         * Thread-safe: Lock-free atomic operation
         * Performance: <0.001ms guaranteed
         */
        bool IsProcessingTask() const { return m_isProcessing.load(::std::memory_order_acquire); }

        /**
         * @brief Get current task type
         * @return Current task type or TASK_IDLE if none
         *
         * Thread-safe: Lock-free atomic operation
         * Performance: <0.001ms guaranteed
         */
        Task::Type GetCurrentTaskType() const
        {
            return static_cast<Task::Type>(m_currentTaskType.load(::std::memory_order_acquire));
        }

        /**
         * @brief Get number of pending tasks
         * @return Count of tasks in queue
         *
         * Thread-safe: Lock-free atomic operation
         * Performance: <0.001ms guaranteed
         */
        uint32 GetPendingTaskCount() const { return m_taskCount.load(::std::memory_order_acquire); }

        /**
         * @brief Get total tasks completed since creation
         * @return Total completed task count
         *
         * Thread-safe: Lock-free atomic operation
         * Performance: <0.001ms guaranteed
         */
        uint32 GetCompletedTaskCount() const { return m_completedTasks.load(::std::memory_order_acquire); }

        /**
         * @brief Clear all pending tasks
         *
         * Note: Does not interrupt current task
         */
        void ClearTasks();

        /**
         * @brief Check if the manager is idle (no current task, no pending tasks)
         * @return true if completely idle
         *
         * Thread-safe: Lock-free atomic operation
         * Performance: <0.001ms guaranteed
         */
        bool IsIdle() const
        {
            return !m_isProcessing.load(::std::memory_order_acquire) &&
                   !m_hasTasks.load(::std::memory_order_acquire);
        }

    protected:
        /**
         * @brief Process tasks during throttled update
         * @param elapsed Time since last update in milliseconds
         *
         * This method demonstrates:
         * - Efficient task processing with time budgeting
         * - Proper state flag updates
         * - Performance-conscious implementation
         */
        void OnUpdate(uint32 elapsed);

        /**
         * @brief Initialize manager on first update
         * @return true on success, false to retry
         */
        bool OnInitialize();

        /**
         * @brief Cleanup on shutdown
         */
        void OnShutdown();

    private:
        /**
         * @brief Process a single task
         * @param task Task to process
         * @return true if task completed, false if still processing
         */
        bool ProcessTask(const Task& task);

        /**
         * @brief Update atomic state flags based on queue state
         */
        void UpdateStateFlags();

        // Task queue and current task
        ::std::deque<Task> m_taskQueue;          ///< Queue of pending tasks
        ::std::unique_ptr<Task> m_currentTask;   ///< Currently processing task

        // Atomic state flags for lock-free queries from strategies
        ::std::atomic<bool> m_hasTasks{false};           ///< True if tasks are queued
        ::std::atomic<bool> m_isProcessing{false};       ///< True if processing a task
        ::std::atomic<uint32> m_currentTaskType{0};      ///< Current task type (Task::Type)
        ::std::atomic<uint32> m_taskCount{0};            ///< Number of pending tasks
        ::std::atomic<uint32> m_completedTasks{0};       ///< Total completed tasks
        ::std::atomic<uint32> m_failedTasks{0};          ///< Total failed tasks

        // Performance tracking
        uint32 m_maxQueueSize{100};                    ///< Maximum queue size
        uint32 m_tasksProcessedThisUpdate{0};          ///< Tasks processed in current update
        uint32 m_maxTasksPerUpdate{5};                 ///< Max tasks to process per update
        uint32 m_taskTimeoutMs{30000};                 ///< Task timeout in milliseconds

        // Statistics
        uint64 m_totalProcessingTimeMs{0};             ///< Total time spent processing
        uint32 m_longestTaskMs{0};                     ///< Longest task duration
    };

} // namespace Playerbot

#endif // TRINITYCORE_EXAMPLE_MANAGER_H