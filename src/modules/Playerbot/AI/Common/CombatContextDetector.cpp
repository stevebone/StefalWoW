/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
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

#include "CombatContextDetector.h"
#include "Creature.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"

namespace Playerbot {
namespace bot { namespace ai {

CombatContext CombatContextDetector::DetectContext(Player const* player)
{
    if (!player)
        return CombatContext::SOLO;

    // Check PvP first (highest priority)
    if (IsInArena(player))
        return CombatContext::PVP_ARENA;

    if (IsInBattleground(player))
        return CombatContext::PVP_BG;

    // Check instances
    if (IsInInstance(player))
    {
        // Raid instances
    if (IsInRaidInstance(player))
        {
            if (IsHeroicOrMythic(player))
                return CombatContext::RAID_HEROIC;
            else
                return CombatContext::RAID_NORMAL;
        }

        // Dungeon instances (5-man)
    if (IsInDungeon(player))
        {
            if (IsFightingBoss(player))
                return CombatContext::DUNGEON_BOSS;
            else
                return CombatContext::DUNGEON_TRASH;
        }
    }

    // Open world
    if (IsInGroup(player))
        return CombatContext::GROUP;

    // Default: Solo
    return CombatContext::SOLO;
}

bool CombatContextDetector::IsInGroup(Player const* player)
{
    if (!player)
        return false;

    Group* group = player->GetGroup();
    return (group != nullptr);
}

bool CombatContextDetector::IsInRaid(Player const* player)
{
    if (!player)
        return false;

    Group* group = player->GetGroup();
    return (group && group->isRaidGroup());
}

bool CombatContextDetector::IsInInstance(Player const* player)
{
    if (!player)
        return false;

    Map* map = player->GetMap();
    if (!map)
        return false;

    return map->IsDungeon();
}

bool CombatContextDetector::IsInDungeon(Player const* player)
{
    if (!player)
        return false;

    Map* map = player->GetMap();
    if (!map)
        return false;

    // Dungeon = 5-man instance
    return map->IsDungeon() && !map->IsRaid();
}

bool CombatContextDetector::IsInRaidInstance(Player const* player)
{
    if (!player)
        return false;

    Map* map = player->GetMap();
    if (!map)
        return false;

    return map->IsRaid();
}

bool CombatContextDetector::IsFightingBoss(Player const* player)
{
    if (!player)
        return false;

    // Check if player is targeting a boss
    if (IsTargetingBoss(player))
        return true;

    // Check if any group member is fighting a boss
    if (IsGroupFightingBoss(player))
        return true;

    // Check instance script for active encounter
    Map* map = player->GetMap();
    if (map && map->IsDungeon())
    {
        InstanceScript* instance = map->ToInstanceMap()->GetInstanceScript();
        if (instance)
        {
            // Check if any boss encounter is in progress
    for (auto const& [bossId, data] : instance->GetBossSaveData())
            {
                if (data.state == IN_PROGRESS)
                    return true;
            }
        }
    }

    return false;
}

bool CombatContextDetector::IsInPvP(Player const* player)
{
    if (!player)
        return false;

    return player->InBattleground();
}

bool CombatContextDetector::IsInArena(Player const* player)
{
    if (!player)
        return false;

    Battleground* bg = player->GetBattleground();
    if (!bg)
        return false;

    return bg->isArena();
}

bool CombatContextDetector::IsInBattleground(Player const* player)
{
    if (!player)
        return false;

    Battleground* bg = player->GetBattleground();
    if (!bg)
        return false;

    return !bg->isArena(); // Battleground but not arena
}

uint32 CombatContextDetector::GetInstanceDifficulty(Player const* player)
{
    if (!player)
        return 0;

    Map* map = player->GetMap();
    if (!map || !map->IsDungeon())
        return 0;

    return static_cast<uint32>(map->GetDifficultyID());
}

bool CombatContextDetector::IsHeroicOrMythic(Player const* player)
{
    uint32 difficulty = GetInstanceDifficulty(player);

    // Difficulty IDs:
    // 0 = Normal
    // 1 = Heroic
    // 2 = Mythic
    // 3+ = Mythic+ / Other variants

    return (difficulty >= 1); // Heroic or higher
}

std::string CombatContextDetector::GetContextDescription(CombatContext context)
{
    switch (context)
    {
        case CombatContext::SOLO:
            return "Solo (questing, gathering, farming, professions)";
        case CombatContext::GROUP:
            return "Group (open-world group content, elite quests, world bosses)";
        case CombatContext::DUNGEON_TRASH:
            return "Dungeon Trash (5-man instance, non-boss)";
        case CombatContext::DUNGEON_BOSS:
            return "Dungeon Boss (5-man instance, boss encounter)";
        case CombatContext::RAID_NORMAL:
            return "Raid Normal (raid instance, normal/LFR difficulty)";
        case CombatContext::RAID_HEROIC:
            return "Raid Heroic/Mythic (raid instance, heroic/mythic difficulty)";
        case CombatContext::PVP_ARENA:
            return "PvP Arena";
        case CombatContext::PVP_BG:
            return "PvP Battleground";
        default:
            return "Unknown Context";
    }
}

bool CombatContextDetector::IsTargetingBoss(Player const* player)
{
    if (!player)
        return false;

    Unit* target = player->GetSelectedUnit();
    if (!target)
        return false;

    Creature* creature = target->ToCreature();
    if (!creature)
        return false;

    // Check if creature is a boss (dungeon/raid boss flag)
    return creature->isDungeonBoss() || creature->isWorldBoss();
}

bool CombatContextDetector::IsGroupFightingBoss(Player const* player)
{
    if (!player)
        return false;

    Group* group = player->GetGroup();
    if (!group)
        return false;

    // Check all group members
    for (GroupReference& groupRef : group->GetMembers())
    {
        Player* member = groupRef.GetSource();
        if (!member)
            continue;

        if (IsTargetingBoss(member))
            return true;
    }

    return false;
}

}} // namespace bot::ai
} // namespace Playerbot
