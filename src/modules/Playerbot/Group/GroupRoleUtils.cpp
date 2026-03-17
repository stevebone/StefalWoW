/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "GroupRoleEnums.h"
#include "Player.h"
#include "DB2Stores.h"
#include "DB2Structure.h"

namespace Playerbot
{

// ============================================================================
// Role Detection Utilities Implementation
// Uses TrinityCore's ChrSpecializationEntry to determine player roles
// ============================================================================

GroupRole GetPlayerSpecRole(Player const* player)
{
    if (!player)
        return GroupRole::NONE;

    // Get the player's active specialization entry from TrinityCore
    ChrSpecializationEntry const* specEntry = player->GetPrimarySpecializationEntry();
    if (!specEntry)
    {
        // No specialization set - return NONE (player might be low level)
        return GroupRole::NONE;
    }

    // Get the authoritative role from the DBC data
    ChrSpecializationRole chrRole = specEntry->GetRole();

    switch (chrRole)
    {
        case ChrSpecializationRole::Tank:
            return GroupRole::TANK;

        case ChrSpecializationRole::Healer:
            return GroupRole::HEALER;

        case ChrSpecializationRole::Dps:
        {
            // Determine if ranged or melee DPS using the spec flags
            EnumFlag<ChrSpecializationFlag> flags = specEntry->GetFlags();
            if (flags.HasFlag(ChrSpecializationFlag::Ranged))
                return GroupRole::RANGED_DPS;
            else
                return GroupRole::MELEE_DPS;
        }

        default:
            return GroupRole::NONE;
    }
}

bool IsPlayerTank(Player const* player)
{
    if (!player)
        return false;

    ChrSpecializationEntry const* specEntry = player->GetPrimarySpecializationEntry();
    return specEntry && specEntry->GetRole() == ChrSpecializationRole::Tank;
}

bool IsPlayerHealer(Player const* player)
{
    if (!player)
        return false;

    ChrSpecializationEntry const* specEntry = player->GetPrimarySpecializationEntry();
    return specEntry && specEntry->GetRole() == ChrSpecializationRole::Healer;
}

bool IsPlayerDPS(Player const* player)
{
    if (!player)
        return false;

    ChrSpecializationEntry const* specEntry = player->GetPrimarySpecializationEntry();
    return specEntry && specEntry->GetRole() == ChrSpecializationRole::Dps;
}

bool IsPlayerRanged(Player const* player)
{
    if (!player)
        return false;

    ChrSpecializationEntry const* specEntry = player->GetPrimarySpecializationEntry();
    if (!specEntry)
        return false;

    // Use the Ranged flag from the specialization entry
    return specEntry->GetFlags().HasFlag(ChrSpecializationFlag::Ranged);
}

bool IsPlayerMelee(Player const* player)
{
    if (!player)
        return false;

    ChrSpecializationEntry const* specEntry = player->GetPrimarySpecializationEntry();
    if (!specEntry)
        return false;

    // Check for explicit Melee flag first
    if (specEntry->GetFlags().HasFlag(ChrSpecializationFlag::Melee))
        return true;

    // If DPS role but not ranged, consider it melee
    // (Tank specs are also technically melee but handled separately)
    if (specEntry->GetRole() == ChrSpecializationRole::Dps &&
        !specEntry->GetFlags().HasFlag(ChrSpecializationFlag::Ranged))
        return true;

    // Tank specs are melee
    if (specEntry->GetRole() == ChrSpecializationRole::Tank)
        return true;

    return false;
}

uint8 GetPlayerSpecIndex(Player const* player)
{
    if (!player)
        return 0;

    ChrSpecializationEntry const* specEntry = player->GetPrimarySpecializationEntry();
    if (!specEntry)
        return 0;

    // The OrderIndex field contains the spec index within the class (0, 1, 2, 3)
    return static_cast<uint8>(specEntry->OrderIndex);
}

ChrSpecialization GetPlayerSpecialization(Player const* player)
{
    if (!player)
        return ChrSpecialization::None;

    return player->GetPrimarySpecialization();
}

} // namespace Playerbot
