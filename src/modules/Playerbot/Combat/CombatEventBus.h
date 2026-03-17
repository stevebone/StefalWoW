/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_COMBAT_EVENT_BUS_H
#define PLAYERBOT_COMBAT_EVENT_BUS_H

#include "Core/Events/GenericEventBus.h"
#include "CombatEvents.h"

namespace Playerbot
{

/**
 * @brief Type alias for CombatEvent EventBus
 */
using CombatEventBus = EventBus<CombatEvent>;

} // namespace Playerbot

#endif // PLAYERBOT_COMBAT_EVENT_BUS_H
