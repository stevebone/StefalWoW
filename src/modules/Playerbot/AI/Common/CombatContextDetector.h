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

#pragma once

#include "Define.h"
#include "ActionScoringEngine.h"
#include "Player.h"
#include "Group.h"
#include "Map.h"
#include "InstanceScript.h"

namespace Playerbot {
namespace bot { namespace ai {

/**
 * @class CombatContextDetector
 * @brief Detects current combat context for utility-based action scoring
 *
 * Context detection logic:
 * - Solo:          Not in group (all solo activities)
 * - Group:         In group, open-world content (not in instance)
 * - DungeonTrash:  In 5-man instance, not fighting boss
 * - DungeonBoss:   In 5-man instance, boss encounter active
 * - RaidNormal:    In raid instance (10-40 players), normal/LFR difficulty
 * - RaidHeroic:    In raid instance, heroic/mythic difficulty
 * - PvPArena:      Battleground type = arena
 * - PvPBG:         Battleground type = battleground
 *
 * Performance: <0.1ms per detection, cached per update cycle
 * Thread Safety: Read-only, safe for concurrent access
 *
 * Usage Example:
 * @code
 * CombatContext context = CombatContextDetector::DetectContext(bot);
 * ActionScoringEngine engine(BotRole::HEALER, context);
 * @endcode
 */
class TC_GAME_API CombatContextDetector
{
public:
    /**
     * @brief Detect current combat context for a player
     * @param player Player instance
     * @return Detected combat context
     */
    static CombatContext DetectContext(Player const* player);

    /**
     * @brief Check if player is in a group
     * @param player Player instance
     * @return True if in group (party or raid)
     */
    static bool IsInGroup(Player const* player);

    /**
     * @brief Check if player is in a raid
     * @param player Player instance
     * @return True if in raid group
     */
    static bool IsInRaid(Player const* player);

    /**
     * @brief Check if player is in an instance
     * @param player Player instance
     * @return True if in instance (dungeon or raid)
     */
    static bool IsInInstance(Player const* player);

    /**
     * @brief Check if player is in a dungeon instance
     * @param player Player instance
     * @return True if in dungeon (5-man instance)
     */
    static bool IsInDungeon(Player const* player);

    /**
     * @brief Check if player is in a raid instance
     * @param player Player instance
     * @return True if in raid instance (10-40 players)
     */
    static bool IsInRaidInstance(Player const* player);

    /**
     * @brief Check if player is fighting a boss
     * @param player Player instance
     * @return True if currently engaged with boss encounter
     */
    static bool IsFightingBoss(Player const* player);

    /**
     * @brief Check if player is in PvP
     * @param player Player instance
     * @return True if in battleground or arena
     */
    static bool IsInPvP(Player const* player);

    /**
     * @brief Check if player is in arena
     * @param player Player instance
     * @return True if in arena battleground
     */
    static bool IsInArena(Player const* player);

    /**
     * @brief Check if player is in battleground
     * @param player Player instance
     * @return True if in battleground (not arena)
     */
    static bool IsInBattleground(Player const* player);

    /**
     * @brief Get instance difficulty
     * @param player Player instance
     * @return Difficulty ID (0=normal, 1=heroic, 2=mythic, etc.)
     */
    static uint32 GetInstanceDifficulty(Player const* player);

    /**
     * @brief Check if instance is heroic or mythic difficulty
     * @param player Player instance
     * @return True if heroic or mythic
     */
    static bool IsHeroicOrMythic(Player const* player);

    /**
     * @brief Get human-readable context description
     * @param context Combat context
     * @return Context description string
     */
    static std::string GetContextDescription(CombatContext context);

private:
    /**
     * @brief Check if player's current target is a boss
     * @param player Player instance
     * @return True if targeting a boss creature
     */
    static bool IsTargetingBoss(Player const* player);

    /**
     * @brief Check if any group member is fighting a boss
     * @param player Player instance
     * @return True if any party/raid member is in boss encounter
     */
    static bool IsGroupFightingBoss(Player const* player);
};

}} // namespace bot::ai
} // namespace Playerbot
