/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CombatContextDetector.h"
#include "Player.h"
#include "Group.h"
#include "Map.h"
#include "Battleground.h"

namespace Playerbot
{

CombatContext CombatContextDetector::Detect(Player* player)
{
    if (!player)
        return CombatContext::SOLO;

    // Check PvP contexts first (highest priority)
    if (player->InArena())
        return CombatContext::ARENA;

    if (player->InBattleground())
        return CombatContext::BATTLEGROUND;

    // Check group status
    Group* group = player->GetGroup();
    if (!group)
        return CombatContext::SOLO;

    // Check if in raid
    if (group->isRaidGroup())
        return CombatContext::RAID;

    // Check if in dungeon instance
    Map* map = player->GetMap();
    if (map && map->IsDungeon())
        return CombatContext::DUNGEON;

    // Default to group if in party but not instanced
    return CombatContext::GROUP;
}

} // namespace Playerbot
