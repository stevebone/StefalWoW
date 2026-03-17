/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <functional>

namespace Playerbot
{

// Spawn request structure
struct SpawnRequest
{
    enum Type { RANDOM, SPECIFIC_CHARACTER, SPECIFIC_ZONE, GROUP_MEMBER };

    Type type = RANDOM;
    uint32 accountId = 0;
    ObjectGuid characterGuid;
    uint32 zoneId = 0;
    uint32 mapId = 0;
    uint8 minLevel = 0;
    uint8 maxLevel = 0;
    uint8 classFilter = 0;
    uint8 raceFilter = 0;
    uint32 maxBotsPerZone = 50;
    bool bypassMaxBotsLimit = false;  // For pool/JIT bots that should bypass MaxBots config

    // Callback on spawn completion
    ::std::function<void(bool success, ObjectGuid guid)> callback;
};

} // namespace Playerbot