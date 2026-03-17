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

#include "ExampleManager.h"
#include "GameTime.h"
#include "BotAI.h"
#include "Player.h"
#include "Log.h"
#include "Timer.h"
#include <algorithm>

namespace Playerbot
{
    ExampleManager::ExampleManager(Player* bot, BotAI* ai)
        : BehaviorManager(bot, ai, 500, "ExampleManager") // 500ms update interval
    {
        // Reserve space for task queue to avoid allocations
        m_taskQueue.clear();

        // Initialize atomic flags
        m_hasTasks.store(false, ::std::memory_order_release);
        m_isProcessing.store(false, ::std::memory_order_release);
        m_currentTaskType.store(static_cast<uint32>(Task::TASK_IDLE), ::std::memory_order_release);
        m_taskCount.store(0, ::std::memory_order_release);

        LogDebug("Created for bot {}", bot ? bot->GetName() : "unknown");
    }

    ExampleManager::~ExampleManager()
    {
        // Call shutdown to clean up
        OnShutdown();

        // Log final statistics
    if (GetBot())
        {
            LogDebug("Shutting down for bot {} - Completed: {}, Failed: {}, Longest task: {}ms",
                    GetBot()->GetName(), m_completedTasks.load(), m_failedTasks.load(), m_longestTaskMs);
        }
    }

    bool ExampleManager::OnInitialize()
    {
        // Example initialization - check if bot is ready
        Player* bot = GetBot();
        if (!bot)
            return false;

        // Check if bot is fully loaded in world
    if (!bot->IsInWorld())
        {
            LogDebug("Bot {} not yet in world, deferring initialization", bot->GetName());
            return false; // Retry on next update
        }

        // Check if bot has required data loaded
    if (bot->GetLevel() == 0)
        {
            LogDebug("Bot {} level not loaded, deferring initialization", bot->GetName());
            return false; // Retry on next update
        }

        // Perform one-time setup
        LogDebug("Initialized successfully for bot {} (Level {})", bot->GetName(), bot->GetLevel());
        // Add an initial idle task
        Task idleTask(Task::TASK_IDLE, 0, 2000);
        AddTask(idleTask);

        return true;
    }

    void ExampleManager::OnShutdown()
    {
        // Clear all pending tasks
        ClearTasks();

        // Reset current task
        m_currentTask.reset();

        // Update flags
        m_isProcessing.store(false, ::std::memory_order_release);
        m_currentTaskType.store(static_cast<uint32>(Task::TASK_IDLE), ::std::memory_order_release);

        LogDebug("Shutdown complete - processed {} tasks total", m_completedTasks.load());
    }

    void ExampleManager::OnUpdate(uint32 /*elapsed*/)
    {
        // Performance: This method aims to complete within 5-10ms
        uint32 updateStartTime = GameTime::GetGameTimeMS();
        m_tasksProcessedThisUpdate = 0;

        Player* bot = GetBot();
        if (!bot || !bot->IsInWorld())
        {
            LogWarning("Bot not available during update");
            return;
        }

        // Process current task if any
    if (m_currentTask)
        {
            bool completed = ProcessTask(*m_currentTask);

            if (completed)
            {
                // Task completed
                uint32 taskDuration = static_cast<uint32>(
                    ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                        ::std::chrono::steady_clock::now() - m_currentTask->startTime
                    ).count()
                );

                m_longestTaskMs = ::std::max(m_longestTaskMs, taskDuration);
                m_totalProcessingTimeMs += taskDuration;
                m_completedTasks.fetch_add(1, ::std::memory_order_acq_rel);

                LogDebug("Task completed: Type={}, Duration={}ms",
                        static_cast<uint32>(m_currentTask->type), taskDuration);

                // Clear current task
                m_currentTask.reset();
                m_isProcessing.store(false, ::std::memory_order_release);
                m_currentTaskType.store(static_cast<uint32>(Task::TASK_IDLE), ::std::memory_order_release);
                m_tasksProcessedThisUpdate++;
            }
            else
            {
                // Check for timeout
                auto now = ::std::chrono::steady_clock::now();
                auto taskAge = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                    now - m_currentTask->startTime
                ).count();

                if (taskAge > m_taskTimeoutMs)
                {
                    LogWarning("Task timed out after {}ms: Type={}",
                              taskAge, static_cast<uint32>(m_currentTask->type));

                    m_failedTasks.fetch_add(1, ::std::memory_order_acq_rel);
                    m_currentTask.reset();
                    m_isProcessing.store(false, ::std::memory_order_release);
                    m_currentTaskType.store(static_cast<uint32>(Task::TASK_IDLE), ::std::memory_order_release);
                }
            }
        }

        // Process queued tasks up to limit
    while (!m_taskQueue.empty() &&
               m_tasksProcessedThisUpdate < m_maxTasksPerUpdate &&
               !m_currentTask)
        {
            // Check update time budget (target: 5-10ms)
            uint32 updateElapsed = getMSTimeDiff(updateStartTime, GameTime::GetGameTimeMS());
            if (updateElapsed > 8) // Leave 2ms buffer
            {
                LogDebug("Update time budget exceeded ({}ms), deferring {} tasks",
                        updateElapsed, m_taskQueue.size());
                break;
            }

            // Start next task
            m_currentTask = ::std::make_unique<Task>(m_taskQueue.front());
            m_taskQueue.pop_front();

            m_isProcessing.store(true, ::std::memory_order_release);
            m_currentTaskType.store(static_cast<uint32>(m_currentTask->type), ::std::memory_order_release);

            LogDebug("Starting task: Type={}, Target={}",
                    static_cast<uint32>(m_currentTask->type), m_currentTask->targetId);

            // Update task count
            uint32 newCount = static_cast<uint32>(m_taskQueue.size());
            m_taskCount.store(newCount, ::std::memory_order_release);
        }

        // Update state flags based on current state
        UpdateStateFlags();

        // Log performance metrics periodically
    if (m_updateCount % 20 == 0) // Every 20 updates
        {
            uint32 avgTaskTime = m_completedTasks > 0 ?
                static_cast<uint32>(m_totalProcessingTimeMs / m_completedTasks) : 0;

            LogDebug("Stats - Queue: {}, Completed: {}, Failed: {}, Avg time: {}ms",
                    m_taskQueue.size(), m_completedTasks.load(),
                    m_failedTasks.load(), avgTaskTime);
        }

        // Final time check
        uint32 totalUpdateTime = getMSTimeDiff(updateStartTime, GameTime::GetGameTimeMS());
        if (totalUpdateTime > 10)
        {
            LogDebug("Update took {}ms (processed {} tasks)", totalUpdateTime, m_tasksProcessedThisUpdate);
        }
    }

    bool ExampleManager::AddTask(const Task& task)
    {
        // Check queue size limit
    if (m_taskQueue.size() >= m_maxQueueSize)
        {
            LogWarning("Task queue full ({} tasks), rejecting new task", m_maxQueueSize);
            return false;
        }

        // Add task to queue
        m_taskQueue.push_back(task);

        // Update atomic counters
        uint32 newCount = static_cast<uint32>(m_taskQueue.size());
        m_taskCount.store(newCount, ::std::memory_order_release);
        m_hasTasks.store(true, ::std::memory_order_release);

        // Request immediate update if this is the only task and we're idle
    if (newCount == 1 && !m_isProcessing.load(::std::memory_order_acquire))
        {
            m_needsUpdate.store(true, ::std::memory_order_release);
            LogDebug("Task added to empty queue, requesting immediate update");
        }

        LogDebug("Task added: Type={}, Queue size={}",
                static_cast<uint32>(task.type), newCount);

        return true;
    }

    void ExampleManager::ClearTasks()
    {
        uint32 clearedCount = static_cast<uint32>(m_taskQueue.size());
        m_taskQueue.clear();

        // Update atomic flags
        m_taskCount.store(0, ::std::memory_order_release);
        m_hasTasks.store(false, ::std::memory_order_release);

        if (clearedCount > 0)
        {
            LogDebug("Cleared {} pending tasks", clearedCount);
        }
    }

    bool ExampleManager::ProcessTask(const Task& task)
    {
        // Simulate task processing based on type
        // In a real implementation, this would perform actual game actions

        auto now = ::std::chrono::steady_clock::now();
        auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
            now - task.startTime
        ).count();

        switch (task.type)
        {
            case Task::TASK_IDLE:
                // Idle task completes after duration
                return elapsed >= task.duration;

            case Task::TASK_MOVE:
            {
                // Simulate movement task
                Player* bot = GetBot();
                if (!bot)
                    return true; // Abort task if bot invalid

                // In real implementation: check if bot reached destination
                // For example: return bot->IsWithinDist(targetPos, 3.0f);

                // Simulate completion after duration
                return elapsed >= task.duration;
            }

            case Task::TASK_INTERACT:
            {
                // Simulate interaction task
                Player* bot = GetBot();
                if (!bot)
                    return true; // Abort task if bot invalid

                // In real implementation: check if interaction completed
                // For example: return bot->GetCurrentSpell(CURRENT_GENERIC_SPELL) == nullptr;

                // Simulate completion after duration
                return elapsed >= task.duration;
            }

            case Task::TASK_WAIT:
                // Wait task simply waits for duration
                return elapsed >= task.duration;

            default:
                LogWarning("Unknown task type: {}", static_cast<uint32>(task.type));
                return true; // Complete unknown tasks immediately
        }
    }

    void ExampleManager::UpdateStateFlags()
    {
        // Update has tasks flag
        bool hasTasks = !m_taskQueue.empty() || m_currentTask != nullptr;
        m_hasTasks.store(hasTasks, ::std::memory_order_release);

        // Update task count
        uint32 count = static_cast<uint32>(m_taskQueue.size());
        m_taskCount.store(count, ::std::memory_order_release);

        // Set hasWork flag for base class if we have tasks to process
        m_hasWork.store(hasTasks, ::std::memory_order_release);

        // Request update if we have many pending tasks
    if (count > 10)
        {
            m_needsUpdate.store(true, ::std::memory_order_release);
        }
    }

} // namespace Playerbot