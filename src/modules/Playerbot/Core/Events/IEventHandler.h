/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_IEVENT_HANDLER_H
#define PLAYERBOT_IEVENT_HANDLER_H

namespace Playerbot
{

/**
 * @brief Generic event handler interface
 *
 * Template interface for type-safe event handling. Each event type
 * requires a specialized handler implementation.
 *
 * **Design Pattern:** Template Interface Pattern
 * - Type safety at compile time
 * - Zero runtime overhead (inline/devirtualization)
 * - Clean separation of concerns
 *
 * **Usage:**
 * ```cpp
 * class BotAI <LootEvent>,
 *               public IEventHandler<QuestEvent>
 * {
 *     void HandleEvent(LootEvent const& event) override { ... }
 *     void HandleEvent(QuestEvent const& event) override { ... }
 * };
 * ```
 *
 * @tparam TEvent The event type this handler processes
 */
template<typename TEvent>
class IEventHandler
{
public:
    virtual ~IEventHandler() = default;

    /**
     * @brief Handle an event
     *
     * Called by EventBus when an event is dispatched to this subscriber.
     *
     * @param event The event to handle
     *
     * Thread Safety: Called from ProcessEvents() - typically single-threaded
     * Performance: Should be fast (<1ms typical)
     */
    virtual void HandleEvent(TEvent const& event) = 0;
};

} // namespace Playerbot

#endif // PLAYERBOT_IEVENT_HANDLER_H
