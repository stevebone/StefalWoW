/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_BOT_ACTION_MANAGER_H
#define PLAYERBOT_BOT_ACTION_MANAGER_H

#include "BotActionQueue.h"
#include "BotActionProcessor.h"
#include "Define.h"
#include <memory>

namespace Playerbot
{

/**
 * @class BotActionManager
 * @brief Global singleton managing bot action queue and processor
 *
 * USAGE:
 * - Worker threads: sBotActionMgr->QueueAction(action)
 * - Main thread (World::Update): sBotActionMgr->ProcessActions()
 *
 * THREAD SAFETY:
 * - QueueAction() is thread-safe (uses ProducerConsumerQueue internally)
 * - ProcessActions() must be called ONLY from main thread
 * - Initialize() must be called before any bot updates start
 */
class TC_GAME_API BotActionManager
{
public:
    static BotActionManager* Instance()
    {
        static BotActionManager instance;
        return &instance;
    }

    /**
     * @brief Initialize the action system
     * Call this during Playerbot module initialization
     */
    void Initialize()
    {
        TC_LOG_INFO("module.playerbot", "BotActionManager: Initializing action queue system");
        _initialized = true;
    }

    /**
     * @brief Shutdown the action system
     * Call this during Playerbot module shutdown
     */
    void Shutdown()
    {
        TC_LOG_INFO("module.playerbot", "BotActionManager: Shutting down action queue system");
        _initialized = false;
    }

    /**
     * @brief Queue action for main thread execution (thread-safe)
     *
     * Can be called from worker threads during parallel bot updates.
     * Fast path: <1μs due to lock-free queue push.
     *
     * @param action The action to queue
     */
    void QueueAction(BotAction const& action)
    {
        if (!_initialized)
        {
            TC_LOG_ERROR("playerbot.action",
                "BotActionManager::QueueAction called before Initialize()!");
            return;
        }

        _queue.Push(action);
    }

    /**
     * @brief Queue action (move semantics)
     */
    void QueueAction(BotAction&& action)
    {
        if (!_initialized)
        {
            TC_LOG_ERROR("playerbot.action",
                "BotActionManager::QueueAction called before Initialize()!");
            return;
        }

        _queue.Push(::std::move(action));
    }

    /**
     * @brief Process all pending actions (main thread only!)
     *
     * Called from World::Update() after bot worker threads complete.
     * Processes up to maxActions per frame to prevent frame spikes.
     *
     * @param maxActions Max actions to process this frame
     * @return Number of actions processed
     */
    uint32 ProcessActions(uint32 maxActions = 1000)
    {
        if (!_initialized)
            return 0;

        return _processor.ProcessActions(maxActions);
    }

    /**
     * @brief Get action queue statistics
     */
    uint64 GetTotalQueued() const { return _queue.GetTotalQueued(); }
    uint64 GetTotalProcessed() const { return _queue.GetTotalProcessed(); }
    uint64 GetTotalFailed() const { return _queue.GetTotalFailed(); }
    size_t GetQueueSize() const { return _queue.Size(); }
    bool IsQueueEmpty() const { return _queue.Empty(); }

private:
    BotActionManager() : _processor(_queue), _initialized(false) {}
    ~BotActionManager() = default;

    // Prevent copying
    BotActionManager(BotActionManager const&) = delete;
    BotActionManager& operator=(BotActionManager const&) = delete;

private:
    BotActionQueue _queue;
    BotActionProcessor _processor;
    bool _initialized;
};

} // namespace Playerbot

#define sBotActionMgr Playerbot::BotActionManager::Instance()

#endif // PLAYERBOT_BOT_ACTION_MANAGER_H
