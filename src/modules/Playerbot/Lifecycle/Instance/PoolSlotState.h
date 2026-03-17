/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/**
 * @file PoolSlotState.h
 * @brief State enumerations and utilities for Instance Bot Pool system
 *
 * This file defines the core state machine for bot pool slots:
 * - PoolSlotState: Lifecycle states for individual bot slots
 * - PoolType: Classification of pool purposes (PvE vs PvP factions)
 * - BotRole: Tank/Healer/DPS classification
 * - InstanceType: Type of instanced content
 *
 * State Transitions:
 * ┌────────────────────────────────────────────────────────────────────────┐
 * │                      POOL SLOT STATE MACHINE                          │
 * ├────────────────────────────────────────────────────────────────────────┤
 * │                                                                        │
 * │   ┌───────┐     create     ┌──────────┐    login    ┌─────────┐       │
 * │   │ Empty │───────────────→│ Creating │────────────→│ Warming │       │
 * │   └───────┘                └──────────┘             └────┬────┘       │
 * │       ↑                                                  │            │
 * │       │ delete                                  warm_up  │            │
 * │       │                                                  ↓            │
 * │   ┌───────────┐  timeout   ┌──────────┐  ready    ┌─────────┐        │
 * │   │Maintenance│←───────────│ Cooldown │←──────────│  Ready  │←─┐     │
 * │   └───────────┘            └──────────┘           └────┬────┘  │     │
 * │       │                         ↑                      │       │     │
 * │       │ repair                  │ release              │ assign│     │
 * │       ↓                         │                      ↓       │     │
 * │   ┌─────────┐              ┌────┴─────┐  fulfill  ┌──────────┐│     │
 * │   │  Ready  │              │ Assigned │←──────────│ Reserved ││     │
 * │   └─────────┘              └──────────┘           └──────────┘│     │
 * │                                                        │       │     │
 * │                                                        └───────┘     │
 * │                                                       cancel         │
 * └────────────────────────────────────────────────────────────────────────┘
 *
 * Thread Safety:
 * - All enumerations are thread-safe (immutable)
 * - Utility functions are pure and stateless
 */

#pragma once

#include "Define.h"
#include <cstdint>
#include <string>

namespace Playerbot
{

// ============================================================================
// POOL SLOT STATES
// ============================================================================

/**
 * @brief Lifecycle states for pool bot slots
 *
 * Each pool slot progresses through states as bots are created,
 * warmed up, assigned to instances, and returned to the pool.
 */
enum class PoolSlotState : uint8
{
    Empty = 0,      ///< Slot has no bot (available for creation)
    Creating,       ///< Bot is being created (JIT factory)
    Warming,        ///< Bot is logging in / initializing
    Ready,          ///< Bot is fully ready for instant assignment
    Reserved,       ///< Bot is reserved for an upcoming instance
    Assigned,       ///< Bot is currently assigned to an active instance
    Cooldown,       ///< Bot returned from instance, on cooldown before reuse
    Maintenance,    ///< Bot is being repaired/updated (gear, talents, etc.)

    Max             ///< Sentinel value for iteration
};

// ============================================================================
// POOL TYPES
// ============================================================================

/**
 * @brief Classification of pool purposes
 *
 * PvE pools serve dungeons and raids (single faction).
 * PvP pools are faction-specific to enable proper battleground population.
 */
enum class PoolType : uint8
{
    PvE = 0,            ///< Dungeons and Raids (faction of requesting player)
    PvP_Alliance,       ///< Alliance battleground/arena bots
    PvP_Horde,          ///< Horde battleground/arena bots

    Max
};

// ============================================================================
// BOT ROLES
// ============================================================================

/**
 * @brief Tank/Healer/DPS classification for pool distribution
 */
enum class BotRole : uint8
{
    Tank = 0,
    Healer = 1,
    DPS = 2,

    Max = 3
};

// ============================================================================
// INSTANCE TYPES
// ============================================================================

/**
 * @brief Types of instanced content supported by the pool system
 */
enum class InstanceType : uint8
{
    Dungeon = 0,        ///< 5-man dungeon
    Raid,               ///< 10-40 man raid
    Battleground,       ///< PvP battleground (requires both factions)
    Arena,              ///< PvP arena (requires opponents)

    Max
};

// ============================================================================
// FACTION ENUMERATION
// ============================================================================

/**
 * @brief WoW faction enumeration
 * Note: This mirrors TrinityCore's TeamId but is explicitly defined
 * for pool system independence
 */
enum class Faction : uint8
{
    Alliance = 0,
    Horde = 1,

    Max
};

// ============================================================================
// UTILITY FUNCTIONS - String Conversion
// ============================================================================

/**
 * @brief Convert PoolSlotState to human-readable string
 * @param state The state to convert
 * @return String representation
 */
inline char const* PoolSlotStateToString(PoolSlotState state)
{
    switch (state)
    {
        case PoolSlotState::Empty:       return "Empty";
        case PoolSlotState::Creating:    return "Creating";
        case PoolSlotState::Warming:     return "Warming";
        case PoolSlotState::Ready:       return "Ready";
        case PoolSlotState::Reserved:    return "Reserved";
        case PoolSlotState::Assigned:    return "Assigned";
        case PoolSlotState::Cooldown:    return "Cooldown";
        case PoolSlotState::Maintenance: return "Maintenance";
        default:                         return "Unknown";
    }
}

/**
 * @brief Convert PoolType to human-readable string
 * @param type The type to convert
 * @return String representation
 */
inline char const* PoolTypeToString(PoolType type)
{
    switch (type)
    {
        case PoolType::PvE:          return "PvE";
        case PoolType::PvP_Alliance: return "PvP_Alliance";
        case PoolType::PvP_Horde:    return "PvP_Horde";
        default:                     return "Unknown";
    }
}

/**
 * @brief Convert BotRole to human-readable string
 * @param role The role to convert
 * @return String representation
 */
inline char const* BotRoleToString(BotRole role)
{
    switch (role)
    {
        case BotRole::Tank:   return "Tank";
        case BotRole::Healer: return "Healer";
        case BotRole::DPS:    return "DPS";
        default:              return "Unknown";
    }
}

/**
 * @brief Convert string to BotRole enum
 * @param roleStr The role string (case-insensitive: "TANK", "Tank", "tank", etc.)
 * @return Corresponding BotRole enum value (defaults to DPS if unknown)
 */
inline BotRole StringToBotRole(std::string_view roleStr)
{
    if (roleStr.empty())
        return BotRole::DPS;

    // Check first character for fast path (case-insensitive)
    char first = static_cast<char>(std::toupper(static_cast<unsigned char>(roleStr[0])));

    if (first == 'T')  // TANK, Tank, tank
        return BotRole::Tank;
    if (first == 'H')  // HEALER, Healer, healer
        return BotRole::Healer;
    // DPS, Dps, dps, DAMAGE, or any other value defaults to DPS
    return BotRole::DPS;
}

/**
 * @brief Convert InstanceType to human-readable string
 * @param type The type to convert
 * @return String representation
 */
inline char const* InstanceTypeToString(InstanceType type)
{
    switch (type)
    {
        case InstanceType::Dungeon:      return "Dungeon";
        case InstanceType::Raid:         return "Raid";
        case InstanceType::Battleground: return "Battleground";
        case InstanceType::Arena:        return "Arena";
        default:                         return "Unknown";
    }
}

/**
 * @brief Convert Faction to human-readable string
 * @param faction The faction to convert
 * @return String representation
 */
inline char const* FactionToString(Faction faction)
{
    switch (faction)
    {
        case Faction::Alliance: return "Alliance";
        case Faction::Horde:    return "Horde";
        default:                return "Unknown";
    }
}

// ============================================================================
// UTILITY FUNCTIONS - State Queries
// ============================================================================

/**
 * @brief Check if state is available for new assignments
 * @param state The state to check
 * @return true if bot can be assigned to new content
 */
inline bool IsAvailableState(PoolSlotState state)
{
    return state == PoolSlotState::Ready;
}

/**
 * @brief Check if state is in active use
 * @param state The state to check
 * @return true if bot is currently being used
 */
inline bool IsActiveState(PoolSlotState state)
{
    return state == PoolSlotState::Reserved || state == PoolSlotState::Assigned;
}

/**
 * @brief Check if state is in transition (not stable)
 * @param state The state to check
 * @return true if bot is in a transient state
 */
inline bool IsTransitionalState(PoolSlotState state)
{
    return state == PoolSlotState::Creating ||
           state == PoolSlotState::Warming ||
           state == PoolSlotState::Cooldown ||
           state == PoolSlotState::Maintenance;
}

/**
 * @brief Check if state transition is valid
 * @param from Current state
 * @param to Target state
 * @return true if transition is allowed
 *
 * Valid transitions follow the state machine diagram above.
 */
inline bool CanTransitionTo(PoolSlotState from, PoolSlotState to)
{
    // Allow any transition to Empty (deletion) or Maintenance (error recovery)
    if (to == PoolSlotState::Empty || to == PoolSlotState::Maintenance)
        return true;

    switch (from)
    {
        case PoolSlotState::Empty:
            return to == PoolSlotState::Creating;

        case PoolSlotState::Creating:
            return to == PoolSlotState::Warming;

        case PoolSlotState::Warming:
            return to == PoolSlotState::Ready;

        case PoolSlotState::Ready:
            return to == PoolSlotState::Reserved || to == PoolSlotState::Assigned;

        case PoolSlotState::Reserved:
            return to == PoolSlotState::Assigned || to == PoolSlotState::Ready; // cancel

        case PoolSlotState::Assigned:
            return to == PoolSlotState::Cooldown;

        case PoolSlotState::Cooldown:
            return to == PoolSlotState::Ready;

        case PoolSlotState::Maintenance:
            return to == PoolSlotState::Ready;

        default:
            return false;
    }
}

// ============================================================================
// UTILITY FUNCTIONS - Pool Type Queries
// ============================================================================

/**
 * @brief Get the appropriate pool type for a faction in PvP
 * @param faction The faction
 * @return Corresponding PvP pool type
 */
inline PoolType GetPvPPoolType(Faction faction)
{
    return faction == Faction::Alliance ? PoolType::PvP_Alliance : PoolType::PvP_Horde;
}

/**
 * @brief Check if pool type is for PvP
 * @param type The pool type
 * @return true if pool is for PvP content
 */
inline bool IsPvPPoolType(PoolType type)
{
    return type == PoolType::PvP_Alliance || type == PoolType::PvP_Horde;
}

/**
 * @brief Get faction for a PvP pool type
 * @param type The pool type
 * @return Faction for the pool (Alliance for PvE as default)
 */
inline Faction GetFactionForPoolType(PoolType type)
{
    if (type == PoolType::PvP_Horde)
        return Faction::Horde;
    return Faction::Alliance; // PvE and PvP_Alliance both default to Alliance
}

// ============================================================================
// UTILITY FUNCTIONS - Instance Type Queries
// ============================================================================

/**
 * @brief Check if instance type requires both factions
 * @param type The instance type
 * @return true if both Alliance and Horde bots are needed
 */
inline bool RequiresBothFactions(InstanceType type)
{
    return type == InstanceType::Battleground || type == InstanceType::Arena;
}

/**
 * @brief Get typical player count for instance type
 * @param type The instance type
 * @return Minimum expected players
 */
inline uint32 GetMinPlayersForInstanceType(InstanceType type)
{
    switch (type)
    {
        case InstanceType::Dungeon:      return 5;
        case InstanceType::Raid:         return 10;
        case InstanceType::Battleground: return 10;  // Smallest BG (Warsong Gulch)
        case InstanceType::Arena:        return 2;   // 2v2
        default:                         return 1;
    }
}

/**
 * @brief Get maximum player count for instance type
 * @param type The instance type
 * @return Maximum expected players
 */
inline uint32 GetMaxPlayersForInstanceType(InstanceType type)
{
    switch (type)
    {
        case InstanceType::Dungeon:      return 5;
        case InstanceType::Raid:         return 40;  // Classic 40-man raids
        case InstanceType::Battleground: return 80;  // Alterac Valley (40v40)
        case InstanceType::Arena:        return 10;  // 5v5
        default:                         return 5;
    }
}

// ============================================================================
// UTILITY FUNCTIONS - Role Queries
// ============================================================================

/**
 * @brief Get recommended role distribution for group size
 * @param groupSize Total group size
 * @param[out] tanks Recommended tank count
 * @param[out] healers Recommended healer count
 * @param[out] dps Recommended DPS count
 */
inline void GetRecommendedRoleDistribution(uint32 groupSize, uint32& tanks, uint32& healers, uint32& dps)
{
    if (groupSize <= 5)
    {
        // 5-man: 1 tank, 1 healer, 3 DPS
        tanks = 1;
        healers = 1;
        dps = groupSize > 2 ? groupSize - 2 : 1;
    }
    else if (groupSize <= 10)
    {
        // 10-man: 2 tanks, 2-3 healers, rest DPS
        tanks = 2;
        healers = (groupSize >= 10) ? 3 : 2;
        dps = groupSize - tanks - healers;
    }
    else if (groupSize <= 25)
    {
        // 25-man: 2-3 tanks, 5-6 healers, rest DPS
        tanks = (groupSize >= 25) ? 3 : 2;
        healers = (groupSize >= 25) ? 6 : 5;
        dps = groupSize - tanks - healers;
    }
    else
    {
        // 40-man: 4+ tanks, 10+ healers, rest DPS
        tanks = 4 + (groupSize - 40) / 10;
        healers = 10 + (groupSize - 40) / 10;
        dps = groupSize - tanks - healers;
    }
}

/**
 * @brief Convert TrinityCore role flags to BotRole
 * @param roleFlags TrinityCore PLAYER_ROLE_* flags
 * @return Corresponding BotRole
 */
inline BotRole RoleFlagsToBotRole(uint8 roleFlags)
{
    // TrinityCore role flags: PLAYER_ROLE_TANK = 2, PLAYER_ROLE_HEALER = 4, PLAYER_ROLE_DAMAGE = 8
    constexpr uint8 PLAYER_ROLE_TANK = 2;
    constexpr uint8 PLAYER_ROLE_HEALER = 4;

    if (roleFlags & PLAYER_ROLE_TANK)
        return BotRole::Tank;
    if (roleFlags & PLAYER_ROLE_HEALER)
        return BotRole::Healer;
    return BotRole::DPS;
}

/**
 * @brief Convert BotRole to TrinityCore role flags
 * @param role The BotRole
 * @return TrinityCore PLAYER_ROLE_* flag
 */
inline uint8 BotRoleToRoleFlags(BotRole role)
{
    // TrinityCore role flags
    constexpr uint8 PLAYER_ROLE_TANK = 2;
    constexpr uint8 PLAYER_ROLE_HEALER = 4;
    constexpr uint8 PLAYER_ROLE_DAMAGE = 8;

    switch (role)
    {
        case BotRole::Tank:   return PLAYER_ROLE_TANK;
        case BotRole::Healer: return PLAYER_ROLE_HEALER;
        case BotRole::DPS:    return PLAYER_ROLE_DAMAGE;
        default:              return PLAYER_ROLE_DAMAGE;
    }
}

} // namespace Playerbot
