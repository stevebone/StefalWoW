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

#ifndef TRINITY_LIQUIDVALIDATOR_H
#define TRINITY_LIQUIDVALIDATOR_H

#include "Define.h"
#include "ValidationResult.h"
#include "MapDefines.h"

struct Position;
class Unit;
class Map;

enum class LiquidType : uint8
{
    None = 0,
    Water,
    Ocean,
    Magma,
    Slime
};

struct LiquidInfo
{
    bool isInLiquid = false;
    bool isUnderwater = false;
    float liquidSurfaceHeight = 0.0f;
    float depth = 0.0f;                 // How deep underwater (negative if above surface)
    LiquidType type = LiquidType::None;
    bool requiresBreath = false;
    bool isDangerous = false;           // Magma or slime

    bool CanSwim() const { return isInLiquid && !isDangerous; }
    bool ShouldSwim() const { return isInLiquid && depth > 0.5f && !isDangerous; }
    bool NeedsAir() const { return isUnderwater && requiresBreath; }
};

enum class LiquidTransitionType : uint8
{
    None = 0,
    EnteringWater,
    ExitingWater,
    EnteringDangerous,
    ExitingDangerous,
    DepthChange
};

struct LiquidTransition
{
    LiquidTransitionType type = LiquidTransitionType::None;
    Position transitionPoint;
    LiquidType fromType = LiquidType::None;
    LiquidType toType = LiquidType::None;
    float depthChange = 0.0f;
};

class TC_GAME_API LiquidValidator
{
public:
    LiquidValidator() = default;
    ~LiquidValidator() = default;

    // Get liquid information at unit's current position
    static LiquidInfo GetLiquidInfo(Unit const* unit);

    // Get liquid information at a specific position
    static LiquidInfo GetLiquidInfoAt(Map const* map, Position const& pos);
    static LiquidInfo GetLiquidInfoAt(uint32 mapId, Position const& pos);

    // Check if unit is in water and should be swimming
    static bool IsSwimmingRequired(Unit const* unit);

    // Check if unit is underwater (needs breath)
    static bool IsUnderwater(Unit const* unit);

    // Check if unit is in dangerous liquid (magma/slime)
    static bool IsInDangerousLiquid(Unit const* unit);

    // Get depth below water surface (positive = underwater)
    static float GetWaterDepth(Unit const* unit);

    // Check liquid transition along a path
    static LiquidTransition CheckLiquidTransition(Unit const* unit, Position const& from, Position const& to);

    // Validate if movement through liquid is safe
    static ValidationResult ValidateLiquidPath(Unit const* unit, Position const& from, Position const& to);

    // Get the surface position above current liquid
    static Position GetSurfacePosition(Unit const* unit);

    // Check if unit needs to surface for air
    static bool NeedsToSurface(Unit const* unit, uint32 breathTimer);

    // Get recommended movement height in water
    static float GetSwimmingHeight(Unit const* unit);

private:
    // Threshold depth to consider swimming
    static constexpr float SWIMMING_DEPTH_THRESHOLD = 0.5f;

    // Depth at which unit is considered underwater for breathing
    static constexpr float UNDERWATER_THRESHOLD = 1.5f;

    // Maximum time underwater before needing to surface (in ms)
    static constexpr uint32 MAX_BREATH_TIME = 60000;

    // Convert TrinityCore liquid flags to LiquidType
    static LiquidType ConvertLiquidType(map_liquidHeaderTypeFlags flags);
};

#endif // TRINITY_LIQUIDVALIDATOR_H
