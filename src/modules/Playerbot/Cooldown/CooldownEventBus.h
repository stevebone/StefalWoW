/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_COOLDOWN_EVENT_BUS_H
#define PLAYERBOT_COOLDOWN_EVENT_BUS_H

#include "Core/Events/GenericEventBus.h"
#include "CooldownEvents.h"

namespace Playerbot
{

/**
 * @brief Type alias for CooldownEvent EventBus
 *
 * Provides convenient access to the cooldown event bus singleton.
 * Use this for subscribing to and publishing cooldown events.
 *
 * Usage:
 *   // Publish a cooldown event
 *   CooldownEventBus::instance()->PublishEvent(event);
 *
 *   // Subscribe to specific event types
 *   CooldownEventBus::instance()->Subscribe(botAI, {
 *       CooldownEventType::MAJOR_CD_USED,
 *       CooldownEventType::MAJOR_CD_AVAILABLE
 *   });
 */
using CooldownEventBus = EventBus<CooldownEvent>;

} // namespace Playerbot

#endif // PLAYERBOT_COOLDOWN_EVENT_BUS_H
