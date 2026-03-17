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
#include "DBCEnums.h"

class Player;
struct ChrSpecializationEntry;

namespace Playerbot
{

enum class GroupRole : uint8
{
    TANK        = 0,
    HEALER      = 1,
    MELEE_DPS   = 2,
    RANGED_DPS  = 3,
    SUPPORT     = 4,
    UTILITY     = 5,
    NONE        = 6
};

enum class RoleCapability : uint8
{
    PRIMARY     = 0,  // Main specialization role
    SECONDARY   = 1,  // Off-spec capable
    HYBRID      = 2,  // Dual-role capable (e.g., Paladin tank/heal)
    EMERGENCY   = 3,  // Can fill role in emergency
    INCAPABLE   = 4   // Cannot perform this role
};

enum class RoleAssignmentStrategy : uint8
{
    OPTIMAL_ASSIGNMENT         = 0,  // Best possible role distribution
    BALANCED_ASSIGNMENT        = 1,  // Even distribution of capabilities
    FLEXIBLE_ASSIGNMENT        = 2,  // Adapt to group needs
    STRICT_PRIMARY_ONLY        = 3,  // Only assign primary roles
    HYBRID_CLASS_FRIENDLY      = 4,  // Favor hybrid classes
    DUNGEON_CONTENT_FOCUSED    = 5,  // Optimize for dungeon content
    RAID_CONTENT_FOCUSED       = 6,  // Optimize for raid content
    PVP_CONTENT_FOCUSED        = 7   // Optimize for PvP content
};

// ============================================================================
// Centralized Role Detection Utilities
// Uses TrinityCore's ChrSpecializationEntry to determine player roles
// based on their actual active specialization.
// ============================================================================

/**
 * @brief Get the base role (Tank/Healer/DPS) from a player's active specialization
 * @param player The player to analyze
 * @return GroupRole::TANK, GroupRole::HEALER, GroupRole::MELEE_DPS, GroupRole::RANGED_DPS, or GroupRole::NONE
 *
 * This function uses TrinityCore's authoritative ChrSpecializationEntry data to
 * determine the player's role based on their currently active specialization.
 */
TC_GAME_API GroupRole GetPlayerSpecRole(Player const* player);

/**
 * @brief Check if a player is currently specced as a tank
 * @param player The player to check
 * @return true if the player's active spec is a tank spec
 */
TC_GAME_API bool IsPlayerTank(Player const* player);

/**
 * @brief Check if a player is currently specced as a healer
 * @param player The player to check
 * @return true if the player's active spec is a healer spec
 */
TC_GAME_API bool IsPlayerHealer(Player const* player);

/**
 * @brief Check if a player is currently specced as DPS
 * @param player The player to check
 * @return true if the player's active spec is a DPS spec (melee or ranged)
 */
TC_GAME_API bool IsPlayerDPS(Player const* player);

/**
 * @brief Check if a player's spec is ranged
 * @param player The player to check
 * @return true if the player's active spec is flagged as ranged
 */
TC_GAME_API bool IsPlayerRanged(Player const* player);

/**
 * @brief Check if a player's spec is melee
 * @param player The player to check
 * @return true if the player's active spec is flagged as melee (or is DPS and not ranged)
 */
TC_GAME_API bool IsPlayerMelee(Player const* player);

/**
 * @brief Get the specialization index (0-3) for a player
 * @param player The player to analyze
 * @return The spec index (0, 1, 2, or 3), or 0 if no valid specialization
 *
 * Returns the index within the class's specialization list, useful for
 * indexing into _classSpecRoles maps.
 */
TC_GAME_API uint8 GetPlayerSpecIndex(Player const* player);

/**
 * @brief Get the specialization ID for a player
 * @param player The player to analyze
 * @return The ChrSpecialization enum value, or ChrSpecialization::None
 */
TC_GAME_API ChrSpecialization GetPlayerSpecialization(Player const* player);

} // namespace Playerbot
