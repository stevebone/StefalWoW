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

#include "LiquidValidator.h"
#include "Unit.h"
#include "Map.h"
#include "MapManager.h"
#include "Position.h"
#include "Log.h"
#include "Optional.h"
#include <sstream>
#include <cmath>

LiquidType LiquidValidator::ConvertLiquidType(map_liquidHeaderTypeFlags flags)
{
    // Use bitwise operators for enum flag checking (raw enum doesn't have HasFlag method)
    if ((flags & map_liquidHeaderTypeFlags::Magma) != map_liquidHeaderTypeFlags{})
        return LiquidType::Magma;
    if ((flags & map_liquidHeaderTypeFlags::Slime) != map_liquidHeaderTypeFlags{})
        return LiquidType::Slime;
    if ((flags & map_liquidHeaderTypeFlags::Ocean) != map_liquidHeaderTypeFlags{})
        return LiquidType::Ocean;
    if ((flags & map_liquidHeaderTypeFlags::Water) != map_liquidHeaderTypeFlags{})
        return LiquidType::Water;

    return LiquidType::None;
}

LiquidInfo LiquidValidator::GetLiquidInfo(Unit const* unit)
{
    LiquidInfo info;

    if (!unit || !unit->IsInWorld())
        return info;

    Map const* map = unit->GetMap();
    if (!map)
        return info;

    return GetLiquidInfoAt(map, unit->GetPosition());
}

LiquidInfo LiquidValidator::GetLiquidInfoAt(Map const* map, Position const& pos)
{
    LiquidInfo info;

    if (!map)
        return info;

    LiquidData liquidData;
    PhaseShift emptyPhaseShift;

    ZLiquidStatus status = const_cast<Map*>(map)->GetLiquidStatus(
        emptyPhaseShift,
        pos.GetPositionX(),
        pos.GetPositionY(),
        pos.GetPositionZ(),
        Optional<map_liquidHeaderTypeFlags>(map_liquidHeaderTypeFlags::AllLiquids),
        &liquidData
    );

    if (status == LIQUID_MAP_NO_WATER)
        return info;

    info.isInLiquid = true;
    info.liquidSurfaceHeight = liquidData.level;
    info.depth = liquidData.level - pos.GetPositionZ();
    info.type = ConvertLiquidType(liquidData.type_flags);
    info.isDangerous = (info.type == LiquidType::Magma || info.type == LiquidType::Slime);
    info.requiresBreath = !info.isDangerous; // Dangerous liquids kill, water requires breath

    // Check if underwater (head below surface)
    info.isUnderwater = (status == LIQUID_MAP_UNDER_WATER);

    return info;
}

LiquidInfo LiquidValidator::GetLiquidInfoAt(uint32 mapId, Position const& pos)
{
    LiquidInfo info;

    Map const* map = sMapMgr->FindMap(mapId, 0);
    if (!map)
        return info;

    return GetLiquidInfoAt(map, pos);
}

bool LiquidValidator::IsSwimmingRequired(Unit const* unit)
{
    if (!unit || !unit->IsInWorld())
        return false;

    LiquidInfo info = GetLiquidInfo(unit);
    return info.ShouldSwim();
}

bool LiquidValidator::IsUnderwater(Unit const* unit)
{
    if (!unit || !unit->IsInWorld())
        return false;

    LiquidInfo info = GetLiquidInfo(unit);
    return info.isUnderwater;
}

bool LiquidValidator::IsInDangerousLiquid(Unit const* unit)
{
    if (!unit || !unit->IsInWorld())
        return false;

    LiquidInfo info = GetLiquidInfo(unit);
    return info.isDangerous;
}

float LiquidValidator::GetWaterDepth(Unit const* unit)
{
    if (!unit || !unit->IsInWorld())
        return 0.0f;

    LiquidInfo info = GetLiquidInfo(unit);
    return info.depth;
}

LiquidTransition LiquidValidator::CheckLiquidTransition(Unit const* unit, Position const& from, Position const& to)
{
    LiquidTransition transition;

    if (!unit || !unit->IsInWorld())
        return transition;

    Map const* map = unit->GetMap();
    if (!map)
        return transition;

    LiquidInfo fromInfo = GetLiquidInfoAt(map, from);
    LiquidInfo toInfo = GetLiquidInfoAt(map, to);

    // No transition if both states are the same
    if (fromInfo.isInLiquid == toInfo.isInLiquid &&
        fromInfo.isDangerous == toInfo.isDangerous &&
        std::abs(fromInfo.depth - toInfo.depth) < 0.5f)
    {
        return transition;
    }

    transition.fromType = fromInfo.type;
    transition.toType = toInfo.type;
    transition.depthChange = toInfo.depth - fromInfo.depth;

    // Calculate transition point (midpoint for simplicity)
    transition.transitionPoint = Position(
        (from.GetPositionX() + to.GetPositionX()) / 2.0f,
        (from.GetPositionY() + to.GetPositionY()) / 2.0f,
        (from.GetPositionZ() + to.GetPositionZ()) / 2.0f,
        0.0f
    );

    // Determine transition type
    if (!fromInfo.isInLiquid && toInfo.isInLiquid)
    {
        transition.type = toInfo.isDangerous ?
            LiquidTransitionType::EnteringDangerous :
            LiquidTransitionType::EnteringWater;
    }
    else if (fromInfo.isInLiquid && !toInfo.isInLiquid)
    {
        transition.type = fromInfo.isDangerous ?
            LiquidTransitionType::ExitingDangerous :
            LiquidTransitionType::ExitingWater;
    }
    else if (fromInfo.isInLiquid && toInfo.isInLiquid)
    {
        if (fromInfo.isDangerous != toInfo.isDangerous)
        {
            transition.type = toInfo.isDangerous ?
                LiquidTransitionType::EnteringDangerous :
                LiquidTransitionType::ExitingDangerous;
        }
        else
        {
            transition.type = LiquidTransitionType::DepthChange;
        }
    }

    return transition;
}

ValidationResult LiquidValidator::ValidateLiquidPath(Unit const* unit, Position const& from, Position const& to)
{
    if (!unit || !unit->IsInWorld())
    {
        return ValidationResult::Failure(
            ValidationFailureReason::InvalidPosition,
            "Unit is null or not in world");
    }

    LiquidTransition transition = CheckLiquidTransition(unit, from, to);

    // Check if entering dangerous liquid
    if (transition.type == LiquidTransitionType::EnteringDangerous)
    {
        std::ostringstream oss;
        oss << "Path would enter dangerous liquid (";
        switch (transition.toType)
        {
            case LiquidType::Magma: oss << "Magma"; break;
            case LiquidType::Slime: oss << "Slime"; break;
            default: oss << "Unknown"; break;
        }
        oss << ")";

        return ValidationResult::Failure(
            ValidationFailureReason::LiquidDanger,
            oss.str());
    }

    return ValidationResult::Success();
}

Position LiquidValidator::GetSurfacePosition(Unit const* unit)
{
    if (!unit || !unit->IsInWorld())
        return Position();

    LiquidInfo info = GetLiquidInfo(unit);

    if (!info.isInLiquid)
        return unit->GetPosition();

    // Return position at liquid surface
    return Position(
        unit->GetPositionX(),
        unit->GetPositionY(),
        info.liquidSurfaceHeight + 0.5f, // Slightly above surface
        unit->GetOrientation()
    );
}

bool LiquidValidator::NeedsToSurface(Unit const* unit, uint32 breathTimer)
{
    if (!unit || !unit->IsInWorld())
        return false;

    LiquidInfo info = GetLiquidInfo(unit);

    if (!info.isUnderwater || !info.requiresBreath)
        return false;

    // Surface if running low on breath
    return breathTimer > (MAX_BREATH_TIME * 0.8f); // Surface at 80% breath used
}

float LiquidValidator::GetSwimmingHeight(Unit const* unit)
{
    if (!unit || !unit->IsInWorld())
        return 0.0f;

    LiquidInfo info = GetLiquidInfo(unit);

    if (!info.isInLiquid)
        return unit->GetPositionZ();

    // Swim at surface level minus small offset
    return info.liquidSurfaceHeight - 0.3f;
}
