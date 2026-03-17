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

#ifndef _LFGROLEDETECTOR_H
#define _LFGROLEDETECTOR_H

#include "Common.h"
#include "LFG.h"

class Player;

/**
 * @brief Automatically detects player roles based on spec, gear, and class
 *
 * This detector analyzes players to determine their most appropriate role for LFG:
 * - Primary detection via talent specialization
 * - Secondary detection via equipped gear stats (tank stats, healing power, DPS stats)
 * - Fallback detection via class capabilities
 * - Intelligent handling of hybrid classes
 *
 * Singleton implementation using Meyer's singleton pattern (thread-safe).
 */
class TC_GAME_API LFGRoleDetector final 
{
private:
    LFGRoleDetector();
    ~LFGRoleDetector();

public:
    // Singleton access
    static LFGRoleDetector* instance();

    // Delete copy/move constructors and assignment operators
    LFGRoleDetector(LFGRoleDetector const&) = delete;
    LFGRoleDetector(LFGRoleDetector&&) = delete;
    LFGRoleDetector& operator=(LFGRoleDetector const&) = delete;
    LFGRoleDetector& operator=(LFGRoleDetector&&) = delete;

    /**
     * @brief Detect a player's current role based on all available information
     *
     * Detection priority:
     * 1. Active specialization (if set)
     * 2. Equipped gear analysis
     * 3. Class default role
     *
     * @param player The player to analyze
     * @return Role bitmask (PLAYER_ROLE_TANK/HEALER/DAMAGE)
     */
    uint8 DetectPlayerRole(Player* player);

    /**
     * @brief Detect role specifically for a bot
     *
     * Optimized for bot role detection, considers bot AI configuration
     * and preferred roles if available.
     *
     * @param bot The bot player to analyze
     * @return Role bitmask (PLAYER_ROLE_TANK/HEALER/DAMAGE)
     */
    uint8 DetectBotRole(Player* bot);

    /**
     * @brief Check if a player can perform a specific role
     *
     * Validates that the player's class and spec can fulfill the role,
     * even if it's not their primary role.
     *
     * @param player The player to check
     * @param role The role to validate (single role, not bitmask)
     * @return true if player can perform this role, false otherwise
     */
    bool CanPerformRole(Player* player, uint8 role);

    /**
     * @brief Get the best role for a player based on current state
     *
     * Returns the single most appropriate role (not a bitmask).
     * Useful when a definitive role selection is needed.
     *
     * @param player The player to analyze
     * @return Single role value (PLAYER_ROLE_TANK/HEALER/DAMAGE)
     */
    uint8 GetBestRoleForPlayer(Player* player);

    /**
     * @brief Get all roles a player can perform
     *
     * Returns a bitmask of all roles the player is capable of performing
     * based on their class and available specs.
     *
     * @param player The player to analyze
     * @return Role bitmask of all performable roles
     */
    uint8 GetAllPerformableRoles(Player* player);

    /**
     * @brief Detect role from talent specialization ID
     *
     * @param player The player (for class context)
     * @param specId The specialization ID
     * @return Role for this spec (PLAYER_ROLE_TANK/HEALER/DAMAGE)
     */
    uint8 GetRoleFromSpecialization(Player* player, uint32 specId);

private:
    /**
     * @brief Detect role from player's active talent specialization
     *
     * @param player The player to analyze
     * @return Role based on spec, or PLAYER_ROLE_NONE if no spec
     */
    uint8 DetectRoleFromSpec(Player* player);

    /**
     * @brief Detect role from equipped gear statistics
     *
     * Analyzes gear for tank stats (stamina, armor, dodge, parry),
     * healer stats (intellect, spirit, spell power for healing classes),
     * and DPS stats (attack power, spell power, crit, haste).
     *
     * @param player The player to analyze
     * @return Role based on gear, or PLAYER_ROLE_NONE if unclear
     */
    uint8 DetectRoleFromGear(Player* player);

    /**
     * @brief Get default role for a class
     *
     * Returns the most common/default role for a class when other
     * detection methods fail.
     *
     * @param playerClass The class to check
     * @return Default role for this class
     */
    uint8 GetDefaultRoleForClass(uint8 playerClass);

    /**
     * @brief Calculate tank score from player stats
     *
     * Higher score indicates more tank-oriented gear.
     *
     * @param player The player to analyze
     * @return Tank score (0-1000+)
     */
    uint32 CalculateTankScore(Player* player);

    /**
     * @brief Calculate healer score from player stats
     *
     * Higher score indicates more healer-oriented gear.
     *
     * @param player The player to analyze
     * @return Healer score (0-1000+)
     */
    uint32 CalculateHealerScore(Player* player);

    /**
     * @brief Calculate DPS score from player stats
     *
     * Higher score indicates more DPS-oriented gear.
     *
     * @param player The player to analyze
     * @return DPS score (0-1000+)
     */
    uint32 CalculateDPSScore(Player* player);

    /**
     * @brief Check if a class can tank
     *
     * @param playerClass The class to check
     * @return true if class has tanking specs, false otherwise
     */
    bool ClassCanTank(uint8 playerClass);

    /**
     * @brief Check if a class can heal
     *
     * @param playerClass The class to check
     * @return true if class has healing specs, false otherwise
     */
    bool ClassCanHeal(uint8 playerClass);

    /**
     * @brief Check if a class can DPS
     *
     * @param playerClass The class to check
     * @return true if class has DPS specs, false otherwise (always true in practice)
     */
    bool ClassCanDPS(uint8 playerClass);

    /**
     * @brief Check if a spec ID is a tank spec
     *
     * @param specId The specialization ID
     * @return true if tank spec, false otherwise
     */
    bool IsTankSpec(uint32 specId);

    /**
     * @brief Check if a spec ID is a healer spec
     *
     * @param specId The specialization ID
     * @return true if healer spec, false otherwise
     */
    bool IsHealerSpec(uint32 specId);

    /**
     * @brief Check if a spec ID is a DPS spec
     *
     * @param specId The specialization ID
     * @return true if DPS spec, false otherwise
     */
    bool IsDPSSpec(uint32 specId);
};

#define sLFGRoleDetector LFGRoleDetector::instance()

#endif // _LFGROLEDETECTOR_H
