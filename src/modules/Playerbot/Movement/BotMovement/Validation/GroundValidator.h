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

#ifndef TRINITY_GROUNDVALIDATOR_H
#define TRINITY_GROUNDVALIDATOR_H

#include "ValidationResult.h"
#include "Define.h"
#include <unordered_map>

struct Position;
class Unit;
class Map;

struct GroundHeightCache
{
    float height = 0.0f;
    uint32 timestamp = 0;
};

class TC_GAME_API GroundValidator
{
public:
    GroundValidator();
    ~GroundValidator() = default;

    static float GetGroundHeight(Unit const* unit);

    static ValidationResult ValidateGroundHeight(Unit const* unit, float maxHeightDiff = 10.0f);

    static bool IsVoidPosition(Unit const* unit);
    static bool IsOnBridge(Unit const* unit);
    static bool IsUnsafeTerrain(Unit const* unit);

    void ClearCache();

private:
    static constexpr float VOID_HEIGHT = -500.0f;
    static constexpr float BOT_MAX_FALL_DISTANCE = 50.0f;
    static constexpr uint32 CACHE_LIFETIME_MS = 5000;

    static std::unordered_map<uint64, GroundHeightCache> _heightCache;

    static uint64 MakeCacheKey(uint32 mapId, float x, float y);
};

#endif
