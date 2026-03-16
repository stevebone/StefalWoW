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

#ifndef TRINITY_MOVEMENTMETRICS_H
#define TRINITY_MOVEMENTMETRICS_H

#include "Define.h"

struct TC_GAME_API MovementMetrics
{
    uint64 totalPathsCalculated = 0;
    uint64 totalValidationFailures = 0;
    uint64 totalStuckIncidents = 0;
    uint64 totalRecoveryAttempts = 0;
    uint64 totalRecoverySuccesses = 0;

    void Reset()
    {
        totalPathsCalculated = 0;
        totalValidationFailures = 0;
        totalStuckIncidents = 0;
        totalRecoveryAttempts = 0;
        totalRecoverySuccesses = 0;
    }
};

#endif
