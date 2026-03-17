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

#include "GroundValidator.h"
#include "Unit.h"
#include "Map.h"
#include "Position.h"
#include "GridDefines.h"
#include "GameTime.h"
#include "MapDefines.h"
#include <sstream>
#include <cmath>

std::unordered_map<uint64, GroundHeightCache> GroundValidator::_heightCache;

GroundValidator::GroundValidator()
{
}

uint64 GroundValidator::MakeCacheKey(uint32 mapId, float x, float y)
{
    uint32 gridX = static_cast<uint32>(std::floor(x / 10.0f));
    uint32 gridY = static_cast<uint32>(std::floor(y / 10.0f));
    return (static_cast<uint64>(mapId) << 32) | (static_cast<uint64>(gridX) << 16) | gridY;
}

float GroundValidator::GetGroundHeight(Unit const* unit)
{
    if (!unit || !unit->IsInWorld())
        return INVALID_HEIGHT;

    Map* map = unit->GetMap();
    if (!map)
        return INVALID_HEIGHT;

    float x = unit->GetPositionX();
    float y = unit->GetPositionY();
    float z = unit->GetPositionZ();

    uint64 cacheKey = MakeCacheKey(map->GetId(), x, y);
    uint32 currentTime = GameTime::GetGameTimeMS();

    auto it = _heightCache.find(cacheKey);
    if (it != _heightCache.end() && (currentTime - it->second.timestamp) < CACHE_LIFETIME_MS)
    {
        return it->second.height;
    }

    float height = map->GetHeight(unit->GetPhaseShift(), x, y, z, true);

    GroundHeightCache cache;
    cache.height = height;
    cache.timestamp = currentTime;
    _heightCache[cacheKey] = cache;

    return height;
}

ValidationResult GroundValidator::ValidateGroundHeight(Unit const* unit, float maxHeightDiff)
{
    if (!unit)
    {
        return ValidationResult::Failure(
            ValidationFailureReason::InvalidPosition,
            "Unit is null");
    }

    if (!unit->IsInWorld())
    {
        return ValidationResult::Failure(
            ValidationFailureReason::InvalidPosition,
            "Unit is not in world");
    }

    float x = unit->GetPositionX();
    float y = unit->GetPositionY();
    float z = unit->GetPositionZ();

    float groundHeight = GetGroundHeight(unit);

    if (groundHeight == INVALID_HEIGHT)
    {
        std::ostringstream oss;
        oss << "No ground height found at position (" << x << ", " << y << ", " << z << ")";
        return ValidationResult::Failure(ValidationFailureReason::NoGroundHeight, oss.str());
    }

    if (groundHeight <= VOID_HEIGHT)
    {
        std::ostringstream oss;
        oss << "Void position detected at (" << x << ", " << y << ", " << z << "), ground height: " << groundHeight;
        return ValidationResult::Failure(ValidationFailureReason::VoidPosition, oss.str());
    }

    float heightDiff = std::abs(z - groundHeight);
    if (heightDiff > maxHeightDiff)
    {
        if (z < groundHeight - maxHeightDiff)
        {
            std::ostringstream oss;
            oss << "Position too far below ground at (" << x << ", " << y << ", " << z 
                << "), ground height: " << groundHeight << ", diff: " << heightDiff;
            return ValidationResult::Failure(ValidationFailureReason::InvalidPosition, oss.str());
        }
        else if (z > groundHeight + BOT_MAX_FALL_DISTANCE)
        {
            std::ostringstream oss;
            oss << "Position too far above ground at (" << x << ", " << y << ", " << z 
                << "), ground height: " << groundHeight << ", diff: " << heightDiff;
            return ValidationResult::Failure(ValidationFailureReason::InvalidPosition, oss.str());
        }
    }

    return ValidationResult::Success();
}

bool GroundValidator::IsVoidPosition(Unit const* unit)
{
    if (!unit || !unit->IsInWorld())
        return true;

    float groundHeight = GetGroundHeight(unit);
    return (groundHeight == INVALID_HEIGHT || groundHeight <= VOID_HEIGHT);
}

bool GroundValidator::IsOnBridge(Unit const* unit)
{
    if (!unit || !unit->IsInWorld())
        return false;

    Map* map = unit->GetMap();
    if (!map)
        return false;

    float x = unit->GetPositionX();
    float y = unit->GetPositionY();
    float z = unit->GetPositionZ();

    float heightWithVMap = map->GetHeight(unit->GetPhaseShift(), x, y, z, true);
    float heightWithoutVMap = map->GetHeight(unit->GetPhaseShift(), x, y, z, false);

    if (heightWithVMap == INVALID_HEIGHT || heightWithoutVMap == INVALID_HEIGHT)
        return false;

    return std::abs(heightWithVMap - heightWithoutVMap) > 1.0f;
}

bool GroundValidator::IsUnsafeTerrain(Unit const* unit)
{
    if (!unit || !unit->IsInWorld())
        return true;

    if (IsVoidPosition(unit))
        return true;

    Map* map = unit->GetMap();
    if (!map)
        return true;

    float x = unit->GetPositionX();
    float y = unit->GetPositionY();
    float z = unit->GetPositionZ();

    LiquidData liquidData;
    ZLiquidStatus liquidStatus = map->GetLiquidStatus(unit->GetPhaseShift(), x, y, z, map_liquidHeaderTypeFlags::AllLiquids, &liquidData);

    if (liquidStatus != LIQUID_MAP_NO_WATER)
    {
        if (liquidData.type_flags.HasFlag(map_liquidHeaderTypeFlags::Magma) || liquidData.type_flags.HasFlag(map_liquidHeaderTypeFlags::Slime))
            return true;
    }

    return false;
}

void GroundValidator::ClearCache()
{
    _heightCache.clear();
}
